/*
        ----------------------------------------------------------------------

        --- Caen SpA - Computing Systems Division ---

        a2818.c

        Source file for the CAEN A2818 CONET board driver.

        June    2009:
            Version 1.13:
            - BUGFIX on proc entry registration
            - BUGFIX on a2818_init_board: memory was not well dealocated in case of errors
            - BUGFIX on cleanup_module: Bad deallocation of kernel memory

    		April	   2009:
		      	Version 1.12:
			      - added usage count management to avoid system hang unloading a busy module 
		
        September  2008:
            Version 1.11:
            - Added SAFE_MODE to avoid system hang with concurrent processes accessing 
	      more than one A2818 in the same PC.

        June  2008:
            Version 1.10:
            - Added conditional compiler directive to comply with kernel versions >= 2.6.23

        February 2008:
            Version 1.9:
            - a2818_link_reset: modified udelay value to 10 msec to comply with CAENLink devices
            - BUGFIX on a2818_init_board: s->timeout was setted to a fixed jiffes number: 
              now the timeout value is setted in msecs and converted to jiffes
            - Substitution of milliseconds udelay's calls with mdelay once

        March 2007:
            Version 1.8:
            - Register device only if HW is present otherwise the kernel
              crashes with a cat /proc/devices
            - ioctl:A2818_IOCTL_IRQ_WAIT now returns ETIMEDOUT instead of -ETIMEDOUT, in order to give
              to CAENVMElib a chance to detect IRQ timeout errors

        February 2007:
            Version 1.7:
            - Better error handling in the link fail: it did not reset correctly
              when disconnected during a communication.

        August 2006:
            Version 1.6:
            - Bug fix on a2818_ioctl, case A2818_IOCTL_SEND: added packet's
                header build step
            Contributions from Brent Casavant at Silicon Graphics, Inc. (bcasavan@sgi.com)
            - Port to 64-bit architectures
            - Ensure writes are flushed to I/O domain where necessary ( Linux Version>= 2.6.10)
            - Allocate DMA memory correctly for architectures with IOMMUs (Linux Version >= 2.6.9)
            - Unlocked ioctl() method alleviates system performacne problems ( Linux Version>= 2.6.11)


        January 2006:
            Version 1.5:
            - Kernel 2.6.9 porting
            - A2818_IOCTL_REV added: get driver revision

        December 2005:
            Version 1.4:
            - SMP compliance bug fixing.

        August 2005:
            Version 1.3:
            - SMP compliance.
            - Fixed the automatic link reset handling.

        May 2005:
            Version 1.2:
            - Fixed a bug on minor number handling.
            - Fixed the reading of status word at the and of the last packet.
            - Now we use kmap_atomic instead of page_address to solve problems
              with some installations.
            - Introduced the test on link fail condition and the link reset
              in order to avoid the need of driver reloading.

        June 2004: Created.

        ----------------------------------------------------------------------
*/
#define SAFE_MODE 1
#define DEBUG     0
#define SETCLR    1
#define MIN_DMA_SIZE (80)

#ifndef VERSION
	#define VERSION(ver,rel,seq) (((ver)<<16) | ((rel)<<8) | (seq))
#endif

/*
        Version Information
*/
#if SAFE_MODE
	#define DRIVER_VERSION "v1.13s"
#else
	#define DRIVER_VERSION "v1.13u"
#endif
#define DRIVER_AUTHOR "Stefano Coluccini <s.coluccini@caen.it>"
#define DRIVER_DESC "CAEN A2818 CONET board driver"

/* #include <linux/config.h> */
#ifdef CONFIG_SMP
#  define __SMP__
#endif
#include <linux/version.h>

// Rev 1.5
#if LINUX_VERSION_CODE < VERSION(2,5,0)
	#define MODULE

	#ifdef CONFIG_MODVERSIONS
        	#define MODVERSIONS
        	#include <linux/modversions.h>
	#endif
#else
	#include <linux/interrupt.h>
	#include <linux/pagemap.h>

#endif

#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/pci.h>
#include <linux/poll.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/delay.h>                // udelay
// Rev 1.5
#if LINUX_VERSION_CODE < VERSION(2,5,0)
	#include <linux/iobuf.h>                // for the kiobuf interface
#endif
// Rev. 1.6
#if LINUX_VERSION_CODE >= VERSION(2,6,11)
        #include <linux/mutex.h>		// for unlocked ioctl interface
#endif

#if LINUX_VERSION_CODE < VERSION(2,3,0)
typedef struct wait_queue *wait_queue_head_t;
#define init_waitqueue_head(head) *(head) = NULL
#endif

#include "a2818.h"

/*
        ----------------------------------------------------------------------

        a2818_mmiowb

        ----------------------------------------------------------------------
*/
#if LINUX_VERSION_CODE >= VERSION(2,6,10)
    #define a2818_mmiowb() 	mmiowb()
#else
    #define a2818_mmiowb()
#endif

#define DMAREAD_DONE (readl(s->plx_vir + PLX_DMACSR) & DMACSR_DONE_0)

#ifndef SETCLR
#define ENABLE_RX_INT \
{\
/* Rev 1.6 */ \
        uint32_t r = readl(s->baseaddr + A2818_IOCTL); \
        writel(r & ~A2818_INTDIS, s->baseaddr + A2818_IOCTL); \
/* Rev 1.6 */ \
        a2818_mmiowb(); \
}
#else
#define ENABLE_RX_INT \
/* Rev 1.6 */ \
{\
        writel(A2818_INTDIS, s->baseaddr + A2818_IOCTL_CLR);\
        a2818_mmiowb(); \
}
#endif

#ifndef SETCLR
#define DISABLE_RX_INT \
{\
/* Rev 1.6 */ \
        uint32_t r = readl(s->baseaddr + A2818_IOCTL); \
        writel(r | A2818_INTDIS, s->baseaddr + A2818_IOCTL); \
/* Rev 1.6 */ \
        a2818_mmiowb(); \
}
#else
#define DISABLE_RX_INT /* \
// 1.4    \
{\
        writel(A2818_INTDIS, s->baseaddr + A2818_IOCTL_SET);\
// Rev 1.6 \
        a2818_mmiowb(); \
}
*/
#endif

#ifndef SETCLR
#define ENABLE_VME_INT \
{\
/* Rev. 1.6 */\
        uint32_t r = readl(s->baseaddr + A2818_IOCTL); \
        writel(r & ~A2818_VINTDIS, s->baseaddr + A2818_IOCTL); \
/* Rev 1.6 */ \
        a2818_mmiowb(); \
}
#else
#define ENABLE_VME_INT /* \
// 1.4  \
{\
        writel(A2818_VINTDIS, s->baseaddr + A2818_IOCTL_CLR);\
// Rev 1.6 \
        a2818_mmiowb(); \
}
*/
#endif

#ifndef SETCLR
#define DISABLE_VME_INT \
{\
/* Rev 1.6 */ \
        uint32_t r = readl(s->baseaddr + A2818_IOCTL); \
        writel(r | A2818_VINTDIS, s->baseaddr + A2818_IOCTL); \
/* Rev 1.6 */ \
        a2818_mmiowb(); \
}
#else
#define DISABLE_VME_INT \
{\
        writel(A2818_VINTDIS, s->baseaddr + A2818_IOCTL_SET);\
/* Rev 1.6 */ \
        a2818_mmiowb(); \
}
#endif

/*
        ----------------------------------------------------------------------

        Function prototypes

        ----------------------------------------------------------------------
*/
static int a2818_open(struct inode *, struct file *);
static int a2818_release(struct inode *, struct file *);
static unsigned int a2818_poll(struct file *, poll_table *);
static int a2818_ioctl(struct inode *, struct file *, unsigned int,
                       unsigned long);
// Rev. 1.6
#if LINUX_VERSION_CODE >= VERSION(2,6,11)
static long a2818_ioctl_unlocked(struct file *, unsigned int, unsigned long);
#endif
// static void a2818_mmiowb( void);

static int a2818_procinfo(char *, char **, off_t, int, int *,void *);

static void a2818_handle_rx_pkt(struct a2818_state *s, int pp);
static void a2818_dispatch_pkt(struct a2818_state *s);
static void ReleaseBoards(void); // ver 1.13
#if !SAFE_MODE
// Rev 1.5
	#if LINUX_VERSION_CODE >= VERSION(2,5,0)
	static inline int sgl_map_user_pages(struct scatterlist *sgl, const unsigned int max_pages, 
			    	  unsigned long uaddr, size_t count, int rw);
	static inline int sgl_unmap_user_pages(struct scatterlist *sgl, const unsigned int nr_pages,
					int dirtied);
	#endif
#endif
/*
        ----------------------------------------------------------------------

        Global variables

        ----------------------------------------------------------------------
*/

// Rev 1.5
#if LINUX_VERSION_CODE < VERSION(2,5,0)
	static int entryCnt = 0;
#endif

static int opened[MAX_MINOR + 1];

static int a2818_major = 0;

static struct a2818_state *devs;

static struct proc_dir_entry *a2818_procdir;

static struct file_operations a2818_fops =
{
        poll:           a2818_poll,
        ioctl:          a2818_ioctl,
// Rev. 1.6
#if LINUX_VERSION_CODE >= VERSION(2,6,11)
        unlocked_ioctl: a2818_ioctl_unlocked,
#endif
        open:           a2818_open,
        release:        a2818_release
};

/*
        ----------------------------------------------------------------------

        a2818_enable_int (Usata solo nella init)

        ----------------------------------------------------------------------
*/
static void a2818_enable_int(struct a2818_state *s)
{
//Rev. 1.6
        uint32_t plx_intcsr;

        /* Enable interrupts on the PLX bridge */
        plx_intcsr = INTCSR_LOC_INT_ENA | INTCSR_PCI_INT_ENA |
                     INTCSR_DMA0_INT_ENA | INTCSR_DMA1_INT_ENA;
        writel(plx_intcsr, s->plx_vir + PLX_INTCSR);

        /* Enable VME interrupt to PCI interrupt routing */
        ENABLE_VME_INT

}

/*
        ----------------------------------------------------------------------

        a2818_dma_conf (Usata solo nella init)

        ----------------------------------------------------------------------
*/
static void a2818_dma_conf(struct a2818_state *s)
{
// Rev 1.6
        uint32_t plx_dmamode;

        /* Configure local to PCI (read) DMA channel (0) */
        plx_dmamode = DMAMODE_DW32 | DMAMODE_READY_ENA |
                      DMAMODE_BTERM_ENA | DMAMODE_BURST_ENA |
                      DMAMODE_DONE_INT_ENA | DMAMODE_LOC_ADR_HOLD |
                      DMAMODE_EOT_ENA | DMAMODE_INT_PCI | DMAMODE_FAST_TERM;
        writel(plx_dmamode, s->plx_vir + PLX_DMAMODE0);
// Rev. 1.6
#if LINUX_VERSION_CODE < VERSION(2,6,9)
        writel(virt_to_bus(s->i_buf), s->plx_vir + PLX_DMAPADR0);
#else
        writel(s->i_buf_pci, s->plx_vir + PLX_DMAPADR0);
#endif
        writel(A2818_RXFIFO, s->plx_vir + PLX_DMALADR0);
        writel(0xF, s->plx_vir + PLX_DMADPR0);

        /* Configure PCI to local (write) DMA channel (1) */
        plx_dmamode = DMAMODE_DW32 | DMAMODE_READY_ENA |
                      DMAMODE_BTERM_ENA | DMAMODE_BURST_ENA |
                      DMAMODE_DONE_INT_ENA | DMAMODE_LOC_ADR_HOLD |
                      DMAMODE_EOT_ENA | DMAMODE_INT_PCI | DMAMODE_FAST_TERM;
        writel(plx_dmamode, s->plx_vir + PLX_DMAMODE1);
        writel(A2818_TXFIFO, s->plx_vir + PLX_DMALADR1);
        writel(0x3, s->plx_vir + PLX_DMADPR1);
// Rev 1.6
        a2818_mmiowb();
}

/*
        ----------------------------------------------------------------------

        a2818_link_reset

        ----------------------------------------------------------------------
*/
static void a2818_link_reset(struct a2818_state *s)
{
#ifndef SETCLR
// Rev. 1.6
        uint32_t a2818_ioctl_reg;

        a2818_ioctl_reg = readl(s->baseaddr + A2818_IOCTL);
        writel(a2818_ioctl_reg & ~A2818_LNKRST, s->baseaddr + A2818_IOCTL);
// Rev 1.6
        a2818_mmiowb();
// Rev 1.9
        mdelay(10);
        writel(a2818_ioctl_reg | A2818_LNKRST, s->baseaddr + A2818_IOCTL);
#else
        writel(A2818_LNKRST, s->baseaddr + A2818_IOCTL_CLR);
// Rev 1.6
        a2818_mmiowb();
// Rev 1.9
        mdelay(10);
        writel(A2818_LNKRST, s->baseaddr + A2818_IOCTL_SET);
#endif
// Rev 1.6
        a2818_mmiowb();
}

/*
        ----------------------------------------------------------------------

        a2818_reset

        ----------------------------------------------------------------------
*/
static void a2818_reset(struct a2818_state *s)
{
// Rev 1.6
        uint32_t app;

        app = readl(s->plx_vir + PLX_CNTRL);

        /* Reset the PLX */
        writel(app | CNTRL_SW_RESET, s->plx_vir + PLX_CNTRL);
// Rev 1.6
        a2818_mmiowb();
        udelay(500);
        writel(app & ~CNTRL_SW_RESET, s->plx_vir + PLX_CNTRL);

        app = readl(s->plx_vir + PLX_CNTRL);

        /* Reload EPROM stored configuration on the PLX */
        writel(app & ~CNTRL_CONF_RELOAD, s->plx_vir + PLX_CNTRL);
        writel(app | CNTRL_CONF_RELOAD, s->plx_vir + PLX_CNTRL);
// Rev 1.6
        a2818_mmiowb();
        udelay(1000);

        /* Perform a link reset on the A2818/V2718 */
        a2818_link_reset(s);
}

/*
        ----------------------------------------------------------------------

        a2818_send_pkt

        ----------------------------------------------------------------------
*/
static int a2818_send_pkt(struct a2818_state *s, int slave, const char *buf, int count)
{
        int nlw, i;

        /* 32-bit word alignment */
        nlw = (count >> 2) + (count & 3 ? 1 : 0) + 1;

        /* Se ho da spedire poco non faccio il DMA */
        if( nlw * 4 < MIN_DMA_SIZE ) {
                for( i = 0; i < nlw; i++ )
                        writel(s->o_buf[slave][i], s->baseaddr + A2818_TXFIFO);
        } else {
// Rev. 1.6
#if LINUX_VERSION_CODE < VERSION(2,6,9)
                writel(virt_to_bus(s->o_buf[slave]), s->plx_vir + PLX_DMAPADR1);
#else
                writel(s->o_buf_pci[slave], s->plx_vir + PLX_DMAPADR1);
#endif

                writel(nlw * 4, s->plx_vir + PLX_DMASIZE1);

                // DMA start
                writel( DMACSR_ENA_0| DMACSR_ENA_1 | DMACSR_START_1, s->plx_vir + PLX_DMACSR);

                // Check for DMA write completion ?
// Rev 1.6
               a2818_mmiowb();
        }

        return count;
}

/*
        ----------------------------------------------------------------------

        a2818_recv_pkt

        ----------------------------------------------------------------------
*/
static int a2818_recv_pkt(struct a2818_state *s, int slave, int *status)
{
        int ret;
	// Rev 1.5
        // unsigned long flags;
        // int app;
	int i;
// Rev. 1.6
        uint32_t tr_stat;


        DISABLE_RX_INT
//  1.4: aggiunto spinlock
        spin_lock_irq( &s->Alock);
        if( !s->rx_ready[slave] && !s->DMAInProgress ) {
//        if( !s->rx_ready[slave] && DMAREAD_DONE ) {
                for( i = 0; i < 15; i++ ) {
                        tr_stat = readl(s->baseaddr + A2818_TRSTAT);

                        if( tr_stat & 0xFFFF0000 ) {
                                s->tr_stat = tr_stat;
                                a2818_handle_rx_pkt(s, 0);
                                break;
                        }
                }
        }
//  1.4: aggiunto spinlock
        spin_unlock_irq( &s->Alock);
#if DEBUG
printk(KERN_INFO PFX "s->il_buf_sz[%d] = %d - s->rx_ready[slave] = %d\n", slave, s->il_buf_sz[slave], s->rx_ready[slave] );
#endif
        ENABLE_RX_INT

        ret = wait_event_interruptible_timeout(
                        s->read_wait[slave],
                        s->rx_ready[slave],
                        s->timeout
              );

#if DEBUG
printk(KERN_INFO PFX "wait-ret = %d\n", ret );
#endif
        if( ret == 0 ) {
                /* Timeout reached */
                ret = -ETIMEDOUT;
                printk(KERN_ERR PFX "Timeout on RX\n");
                goto err_recv;
        } else if( ret < 0 ) {
                /* Interrupted by a signal */
                ret = -EINTR;
                goto err_recv;
        }
	
	ret = s->il_buf_sz[slave];
#if DEBUG
printk(KERN_INFO PFX "read-ret = %d\n", ret );
printk(KERN_INFO PFX "s->rx_status[%d] = %x\n", slave, s->rx_status[slave]  );
#endif

err_recv:
        return ret;
}

/*
        ----------------------------------------------------------------------

        a2818_procinfo

        ----------------------------------------------------------------------
*/
static int a2818_procinfo(char *buf, char **start, off_t fpos, int lenght,
                          int *eof, void *data)
{
        char *p;
        struct a2818_state* s = devs;
        int i = 0;

        p = buf;
        p += sprintf(p,"CAEN A2818 driver %s\n\n", DRIVER_VERSION);

        while( s ) {
                p += sprintf(p, "  CAEN A2818 CONET Board found.\n");
// Rev 1.6
                p += sprintf(p, "  Physical address = %p\n", (void *)s->phys);
                p += sprintf(p, "  Virtual address  = %p\n", s->baseaddr);
                p += sprintf(p, "  IRQ line         = %d\n", (int)s->irq);
                p += sprintf(p, "  Minor number     = %d\n", (int)s->minor);
                p += sprintf(p, "  Reads            = %i\n", s->reads);
                p += sprintf(p, "  Writes           = %i\n", s->writes);
                p += sprintf(p, "  Ioctls           = %i\n", s->ioctls);
//#if DEBUG
                p += sprintf(p, "  Board fw rev     = %08X\n",
                             readl(s->baseaddr + A2818_FWREL));
                p += sprintf(p, "  Board status     = %08X\n",
                             readl(s->baseaddr + A2818_STATUS));
                p += sprintf(p, "  Transfer status  = %08X\n",
                             readl(s->baseaddr + A2818_TRSTAT));
                p += sprintf(p, "  IOCTL            = %08X\n",
                             readl(s->baseaddr + A2818_IOCTL));
                p += sprintf(p, "  IRQSTAT0         = %08X\n",
                             readl(s->baseaddr + A2818_IRQSTAT0));
                p += sprintf(p, "  IRQSTAT1         = %08X\n",
                             readl(s->baseaddr + A2818_IRQSTAT1));
                p += sprintf(p, "  INTCSR           = %08X\n",
                             readl(s->plx_vir + PLX_INTCSR));
                p += sprintf(p, "  DMAMODE          = %08X\n",
                             readl(s->plx_vir + PLX_DMAMODE0));
                p += sprintf(p, "  CNTRL            = %08X\n",
                             readl(s->plx_vir + 0x6c));
                p += sprintf(p, "  PLX_DMACSR       = %08X\n",
                             readl(s->plx_vir + PLX_DMACSR));

//#endif
                p += sprintf(p,"\n");

                s = s->next;
                i++;
        }

        p += sprintf(p,"%d CAEN A2818 board(s) found.\n", i);

        *eof = 1;
        return p - buf;
}

/*
        ----------------------------------------------------------------------

        a2818_register_proc

        ----------------------------------------------------------------------
*/
static void a2818_register_proc(void)
{
        a2818_procdir = create_proc_entry("a2818", S_IFREG | S_IRUGO, 0);
        a2818_procdir->read_proc = a2818_procinfo;
}

/*
        ----------------------------------------------------------------------

        a2818_unregister_proc

        ----------------------------------------------------------------------
*/
static void a2818_unregister_proc(void)
{
        remove_proc_entry("a2818", 0);
}

/*
        ----------------------------------------------------------------------

        a2818_poll

        ----------------------------------------------------------------------
*/
static unsigned int a2818_poll(struct file* file, poll_table* wait)
{
        struct a2818_state *s = (struct a2818_state *)file->private_data;
        unsigned int minor = MINOR(file->f_dentry->d_inode->i_rdev);

        if( s->rx_ready[minor] )
                return POLLIN | POLLRDNORM;
        poll_wait(file, &s->read_wait[minor], wait);
        if( s->rx_ready[minor] )
                return POLLIN | POLLRDNORM;

        return 0;
}

/*
        ----------------------------------------------------------------------

        a2818_open

        ----------------------------------------------------------------------
*/
static int a2818_open(struct inode *inode, struct file *file)
{
        unsigned int minor = MINOR(inode->i_rdev);
        struct a2818_state *s = devs;
        int slave = minor & 0xF;

        /* If minor is out of range, return an error */
        if( minor > MAX_MINOR ) {
                return -ENODEV;
        }

        /* Search for the device linked to the minor */

        while( s && s->minor != (minor >> 4) )
                s = s->next;
        if( !s )
                return -ENODEV;

        /* v1.3d - Alloc in open can cause problems, kmalloc is now in init */
        if( !opened[minor] ) {
                /* Alloc maximum size for DMA (out) */
/*
                s->o_buf[slave] = kmalloc(A2818_MAX_PKT_SZ, GFP_KERNEL | GFP_DMA);
                if (!(s->o_buf[slave])) {
                        printk(KERN_ERR PFX "out of DMA memory\n");
                        return -ENODEV;
                }
*/
                s->o_buf_sz[slave] = 0;
                s->il_buf_sz[slave] = 0;
#if SAFE_MODE
				s->il_buf_p[slave] = 0;
#endif
                s->rx_ready[slave] = 0;
				
        }

        file->private_data = s;
        opened[minor]++;
#if DEBUG
        printk(KERN_INFO PFX "Open eseguita!\n");
#endif
		// rev 1.12
		#if LINUX_VERSION_CODE < VERSION(2,5,0)
		MOD_INC_USE_COUNT
		#else
		try_module_get(THIS_MODULE);
		#endif
        return(0);
}

/*
        ----------------------------------------------------------------------

        a2818_release

        ----------------------------------------------------------------------
*/
static int a2818_release(struct inode *inode, struct file *file)
{
        unsigned int minor = MINOR(inode->i_rdev);
        struct a2818_state *s = devs;
	// Rev 1.5
        // int slave = minor & 0xF;

        /* If minor is out of range, return an error */
        if( minor > MAX_MINOR ) {
                return -ENODEV;
        }

        /* Search for the device linked to the minor */
        while( s && s->minor != (minor >> 4) )
                s = s->next;
        if( !s )
                return -ENODEV;

        opened[minor]--;
//Rev 1.5
/*	
        if( !opened[minor] ) {
//                kfree(s->o_buf[slave]);
        }
*/
		// rev 1.12
		#if LINUX_VERSION_CODE < VERSION(2,5,0)
		MOD_DEC_USE_COUNT
		#else
		module_put(THIS_MODULE);
		#endif
        return 0;
}

/*
        ----------------------------------------------------------------------

        a2818_ioctl

        ----------------------------------------------------------------------
*/
static int a2818_ioctl(struct inode *inode,struct file *file,unsigned int cmd, unsigned long arg)
{
        unsigned int minor;
        struct a2818_state *s;
        int ret = 0, slave;
        a2818_reg_t reg;
        a2818_intr_t intr;
        a2818_comm_t comm;
	// Rev 1.5
        // int retL;

        minor = MINOR(inode->i_rdev);
        s = (struct a2818_state *)file->private_data;
        slave = minor & 0xf;

        s->ioctls++;

        switch (cmd) {
                case A2818_IOCTL_RESET:
                        a2818_reset(s);
                break;
				// Rev 1.5
                case A2818_IOCTL_REV:
					{
						a2818_rev_t rev;
                        if( copy_from_user(&rev, (a2818_rev_t *)arg, sizeof(rev)) > 0 ) {
                                ret = -EFAULT;
                                break;
                        }
						strcpy( rev.rev_buf, DRIVER_VERSION);
                        if( copy_to_user((a2818_rev_t *)arg, &rev, sizeof(rev)) > 0) {
                                ret = -EFAULT;
                                break;
                        }
					}
                break;
                case A2818_IOCTL_REG_RD:
                        if( copy_from_user(&reg, (a2818_reg_t *)arg, sizeof(reg)) > 0 ) {
                                ret = -EFAULT;
                                break;
                        }
                        reg.value = readl(s->baseaddr + reg.address);
                        if( copy_to_user((a2818_reg_t *)arg, &reg, sizeof(reg)) > 0) {
                                ret = -EFAULT;
                                break;
                        }
                break;
                case A2818_IOCTL_REG_WR:
                        if( copy_from_user(&reg, (a2818_reg_t *)arg, sizeof(reg)) > 0 ) {
                                ret = -EFAULT;
                                break;
                        }
                        writel(reg.value, s->baseaddr + reg.address);
// Rev 1.6
                       a2818_mmiowb();
                break;
                case A2818_IOCTL_IRQ_WAIT:
                        if( copy_from_user(&intr, (a2818_intr_t *)arg, sizeof(intr)) > 0 ) {
                                ret = -EFAULT;
                                break;
                        }
                        if( slave < 4 ) {
                                ret = wait_event_interruptible_timeout(
                                        s->intr_wait[slave],            // wait queue
                                        ((readl(s->baseaddr + A2818_IRQSTAT0) >> (slave * 8)) & 0xFF) & intr.Mask,     // condition
                                        (intr.Timeout * HZ) / 1000     // timeout from ms to jiffies
                              );
                        } else {
                                ret = wait_event_interruptible_timeout(
                                        s->intr_wait[slave],            // wait queue
                                        ((readl(s->baseaddr + A2818_IRQSTAT1) >> (slave * 8)) & 0xFF) & intr.Mask,     // condition
                                        (intr.Timeout * HZ) / 1000     // timeout from ms to jiffies
                              );
                        }
                        if( ret == 0 ) {
                                /* Timeout reached */
//Rev 1.8                                
                                /* !!!!! WARNING !!!!!!
                                * The CAENVMElib needs to handle the ioctl return value to detect
                                * IRQ timeout, so we must return a positive value to avoid Linux to
                                * wrap the ioctl's return code to -1
                                */
                                ret = ETIMEDOUT;
                        } else if( ret < 0 ) {
                                /* Interrupted by a signal */
                                ret = -EINTR;
                        } else if( ret > 0 )
                        ret = 0;
                break;
                case A2818_IOCTL_COMM:

                        if( copy_from_user(&comm, (a2818_comm_t *)arg, sizeof(comm)) > 0 ) {
                                ret = -EFAULT;
                                break;
                        }

                        if( copy_from_user(&(s->o_buf[slave][1]), comm.out_buf, comm.out_count) > 0) {
                                ret = -EFAULT;
                                break;
                        }
#if DEBUG
{
    int i;

    for( i = 0; i < comm.out_count; i += 4 )
        printk(KERN_INFO PFX "Dump pkt: %08lX\n", *((long *)(&comm.out_buf[i])));
}
#endif
#if !SAFE_MODE
	// Rev 1.5
	#if LINUX_VERSION_CODE >= VERSION(2,5,0)
                        s->il_buff_act_page_id[ slave]= 0;  // resetta indice pagina attuale
                        s->il_buff_pages[ slave]= 0;        // resetta numero pagine allocate
	#endif
                        if( comm.in_count > 0 ) {

	// Rev 1.5
	#if LINUX_VERSION_CODE < VERSION(2,5,0)
                                ret = map_user_kiobuf(READ, s->il_buf[slave], (u32)comm.in_buf, comm.in_count);
                                entryCnt++;

                                if( ret ) {
                                        ret = -EFAULT;
                                        break;
                                }
                                ret = lock_kiovec(MAX_V2718, s->il_buf, 1);
                                if( ret ) {
                                        printk("ioctl - lock_kiovec error.\n");
                                        unmap_kiobuf(s->il_buf[slave]);
                                        ret = -EFAULT;
                                        break;
                                }
	#else
	// Rev 1.6
                               if( ( s->il_buff_pages[ slave]= sgl_map_user_pages( s->il_buf[ slave], MAX_USER_PAGES, (unsigned long)comm.in_buf, comm.in_count, READ))<= 0)
                               {
                                   printk("ioctl send - sgl_map_user_pages (%d:%p %p %d).\n", ret, s->il_buf[slave],
                                   comm.in_buf, comm.in_count);
                                   ret = -EFAULT;
                                   break;
                               }
	#endif
                        }
#endif //SAFE_MDOE
                        /*
                            Ver. 1.2 - Link fail handling.
                        */
                        if( readl(s->baseaddr + A2818_STATUS) & A2818_LINK_FAIL ) {
#if DEBUG
printk(KERN_INFO PFX "Link fail detected! Trying to reset ...\n");
#endif
                            a2818_reset(s);
// Rev 1.9
                            mdelay(10);  // Wait 10 ms per ora
                            if( readl(s->baseaddr + A2818_STATUS) & A2818_LINK_FAIL ) {
                                        ret = -EFAULT;
                                        // Ver. 1.7 - Before there was a break and it was a bad error handling
                                        goto err_comm;
                            }
                        }
                        if( comm.out_count > 0 ) {
                                /* Build the header */
                                s->o_buf[slave][0] = (slave << 24) | (slave << 16) | ((comm.out_count >> 1) & 0xFFFF);
                                ret = a2818_send_pkt(s, slave, comm.out_buf, comm.out_count);
                                if( ret < 0 ) {
                                        ret = -EFAULT;
                                        goto err_comm;
                                }
                                if( comm.in_count > 0 ) {
                                    ret = a2818_recv_pkt(s, slave, comm.status);
                                    if( ret < 0 ) {
                                            ret = -EFAULT;
                                            goto err_comm;
                                    }
                                }
                        }
#if SAFE_MODE
        		put_user(s->rx_status[slave], comm.status);
			      copy_to_user(comm.in_buf, s->il_buf[slave], s->il_buf_sz[slave]);
#endif
err_comm:
			s->il_buf_p[slave] = 0;
        		s->rx_ready[slave] = 0;
			s->il_buf_sz[slave] = 0;
#if !SAFE_MODE
	// Rev 1.5
	#if LINUX_VERSION_CODE < VERSION(2,5,0)
                        unlock_kiovec(MAX_V2718, s->il_buf);
                        unmap_kiobuf(s->il_buf[slave]);
                        entryCnt--;
	#else
			sgl_unmap_user_pages( s->il_buf[ slave], s->il_buff_pages[ slave], 0);
	#endif
#endif
                break;
// Rev 1.7 - Better error handling for the link fail
                case A2818_IOCTL_SEND:

                        if( copy_from_user(&comm, (a2818_comm_t *)arg, sizeof(comm)) > 0 ) {
                                ret = -EFAULT;
                                break;
                        }

                        if( copy_from_user(&(s->o_buf[slave][1]), comm.out_buf, comm.out_count) > 0) {
                                ret = -EFAULT;
                                break;
                        }
#if DEBUG
{
    int i;

    for( i = 0; i < comm.out_count; i += 4 )
        printk(KERN_INFO PFX "Dump pkt: %08lX\n", *((long *)(&comm.out_buf[i])));
}
#endif
#if !SAFE_MODE
	// Rev 1.5
	#if LINUX_VERSION_CODE < VERSION(2,5,0)

                        ret = map_user_kiobuf(READ, s->il_buf[slave], (u32)comm.in_buf, comm.in_count);
                        if( ret ) {
                                printk("ioctl send - map_user_kiobuf error (%d:%p %p %d).\n", ret, s->il_buf[slave],
                                        comm.in_buf, comm.in_count);
                                ret = -EACCES;
                                break;
                        }
                        ret = lock_kiovec(MAX_V2718, s->il_buf, 1);
                        if( ret ) {
                                printk("ioctl - lock_kiovec error.\n");
                                unmap_kiobuf(s->il_buf[slave]);
                                ret = -EACCES;
                                break;
                        }
	#else
                        s->il_buff_act_page_id[ slave]= 0;		// resetta indice pagina attuale
	// Rev. 1.6
                        if( ( s->il_buff_pages[ slave]= sgl_map_user_pages( s->il_buf[ slave], MAX_USER_PAGES, (unsigned long)comm.in_buf, comm.in_count, READ))<= 0)
                        {
                            printk("ioctl send - sgl_map_user_pages (%d:%p %p %d).\n", ret, s->il_buf[slave],
                            comm.in_buf, comm.in_count);
                            ret = -EACCES;
                            break;
                        }

	#endif
#endif //SAFE_MODE
         /*
                            Ver. 1.2 - Link fail handling.

                        */
                        if( readl(s->baseaddr + A2818_STATUS) & A2818_LINK_FAIL ) {
#if DEBUG
printk(KERN_INFO PFX "Link fail detected! Trying to reset ...\n");
#endif
                            a2818_reset(s);
// Rev 1.9
                            mdelay(10);  // Wait 10 ms per ora
                            if( readl(s->baseaddr + A2818_STATUS) & A2818_LINK_FAIL ) {
                                ret = -EACCES;
                                goto err_send;
                            }
                        }

                        if( comm.out_count > 0 ) {
                                /* Build the header */
                                s->o_buf[slave][0] = (slave << 24) | (slave << 16) | ((comm.out_count >> 1) & 0xFFFF);
                                ret = a2818_send_pkt(s, slave, comm.out_buf, comm.out_count);
                                if( ret < 0 ) {
                                        ret = -EFAULT;
                                        goto err_send;
                                }

                        }
#if SAFE_MODE
			copy_to_user(comm.in_buf, s->il_buf[slave], comm.in_count);
#endif
err_send:
#if !SAFE_MODE
	// Rev 1.5
	#if LINUX_VERSION_CODE < VERSION(2,5,0)
                                        unlock_kiovec(MAX_V2718, s->il_buf);
                                        unmap_kiobuf(s->il_buf[slave]);
	#else
										sgl_unmap_user_pages( s->il_buf[ slave], s->il_buff_pages[ slave], 0);
	#endif
#endif
                break;
                case A2818_IOCTL_RECV:

                        ret = a2818_recv_pkt(s, slave, (int *)arg);
                        if( ret < 0 ) {
                                ret = -EFAULT;
                        }
#if !SAFE_MODE
	// Rev 1.5
	#if LINUX_VERSION_CODE < VERSION(2,5,0)
                        unlock_kiovec(MAX_V2718, s->il_buf);
                        unmap_kiobuf(s->il_buf[slave]);
	#else
						sgl_unmap_user_pages( s->il_buf[ slave], s->il_buff_pages[ slave], 0);
	#endif
#endif
                break;
        }

        return ret;
}

/*
        ----------------------------------------------------------------------

        a2818_ioctl_unlocked (Called in preference to a2818_ioctl on newer kernels)

        ----------------------------------------------------------------------
*/

#if LINUX_VERSION_CODE >= VERSION(2,6,11)
static long a2818_ioctl_unlocked(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct inode *inode = file->f_dentry->d_inode;
        struct a2818_state *s = (struct a2818_state *)file->private_data;
	long ret;

	/* ioctl() calls can cause the Big Kernel Lock (BKL) to be taken, which
	 * can have significant performance penalties system-wide.  By providing
	 * an unlocked ioctl() method the BKL will not be taken, but the driver
	 * becomes responsible for its own locking.  Furthermore, the lock can be
	 * broken down per A2818 so that multiple threads accessing different CONET
	 * chains do not contend with one another during ioctl() calls.
	 */
	mutex_lock(&s->ioctl_lock);
	ret = (long) a2818_ioctl(inode, file, cmd, arg);
	mutex_unlock(&s->ioctl_lock);

	return ret;
}
#endif

/*
        ----------------------------------------------------------------------

        a2818_handle_rx_pkt

        ----------------------------------------------------------------------
*/
static void a2818_handle_rx_pkt(struct a2818_state *s, int pp)
{
        int DMA_size, i;
	// Rev 1.5
        // unsigned long flags;

        /* Disable interrupt on local bus */
        DISABLE_RX_INT

        /* Check on the length */
        DMA_size = (s->tr_stat >> 16) * 4;

#if DEBUG
printk(KERN_INFO PFX "DMA_size = %d\n", DMA_size );
#endif

        if( DMA_size < MIN_DMA_SIZE ) {
                for( i = 0; i < DMA_size/4; i++ )
                        *(u32 *)(s->i_buf+i*4) = readl(s->baseaddr + A2818_RXFIFO);
                s->i_buf_sz = DMA_size;
                // Dispatch global buffer in per-slave buffers
                a2818_dispatch_pkt(s);
                ENABLE_RX_INT
        } else
        {
                if( DMA_size <= A2818_MAX_PKT_SZ ) {
                        s->DMAInProgress = 1;
                        writel(DMA_size, s->plx_vir + PLX_DMASIZE0);
                        // DMA start
                        writel(DMACSR_ENA_0 | DMACSR_ENA_1| DMACSR_START_0, s->plx_vir + PLX_DMACSR);
// Rev 1.6
                       a2818_mmiowb();
                } else {
                        printk(KERN_ERR PFX "PROBLEMA SU DMA_Size %d\n", DMA_size );
                }
        }

}

/*
        ----------------------------------------------------------------------

        a2818_dispatch_pkt

        ----------------------------------------------------------------------
*/

#if SAFE_MODE
static void a2818_dispatch_pkt(struct a2818_state *s)
{
        int i, pkt_sz, slave, nlw, last_pkt, pos;
        u32  mask = 0;
        char *i_buf = s->i_buf, *iobuf;

	// i e' l'indice nel buffer di DMA che ha un header di 4 byte
        i = 0;
        do {
                pkt_sz = (i_buf[i] & 0xFF) + ((i_buf[i+1] << 8) & 0x0100);
                last_pkt = i_buf[i+1] & 0x02;
                i += 3;
                slave = i_buf[i++] & 0xF;
                mask |= 1 << slave;
                nlw = (pkt_sz >> 1) + (pkt_sz & 1);

                if( last_pkt ) {

                        pkt_sz -= 1;    // per togliere lo status
                        s->rx_status[slave] = (i_buf[i + pkt_sz * 2 - 1] << 8) +
                                               i_buf[i + pkt_sz * 2];
#if DEBUG
printk(KERN_INFO PFX "status = %02x slave = %02x\n", s->rx_status[slave], slave);
#endif
                }

                if( slave >= MAX_V2718 ) {
#if DEBUG
printk(KERN_INFO PFX "Wrong packet %04x %02x\n", pkt_sz, slave);
#endif
                        return;
                }
		// buffer dello slave
                iobuf = s->il_buf[slave];
		// posizione da cui iniziare a copiare
		pos = s->il_buf_p[slave];
#if DEBUG
//printk(KERN_INFO PFX "pkt_sz 0x%04x - slave 0x%02x, %d\n", pkt_sz, slave, i );
#endif
                /* Some integrity checks */
                if(
                    (nlw <= (s->i_buf_sz / 4)) &&
                    (slave < MAX_V2718) &&
                    (pkt_sz >= 0) &&
                    (pkt_sz <= 0x100)
                  ) {
			// Copio i dati arrivati
			memcpy(&(iobuf[pos]), &(i_buf[i]), pkt_sz * 2);
			s->il_buf_sz[slave] += pkt_sz * 2;
			i += pkt_sz * 2;
			s->il_buf_p[slave] += pkt_sz * 2;
                        if( last_pkt ) {
                                i += 2; // to wipe out the status word
                                s->rx_ready[slave] = 1;
                                wake_up_interruptible(&s->read_wait[slave]);
                        }
                        i += i % 4;  // 32-bit alignment
                } else {
                        printk(KERN_ERR PFX "Unhandled packet - slave %d, pkt_sz %d\n", slave, pkt_sz);
//                        printk(KERN_ERR PFX "nr_pg %d, len %d, buf_sz %d\n", iobuf->nr_pages, iobuf->length, s->i_buf_sz);
                        break;
                }
        } while( i < s->i_buf_sz );
#if DEBUG
//        printk(KERN_INFO PFX "Esco dalla dispatch_pkt\n");
#endif
}
#else
// Rev 1.5
#if LINUX_VERSION_CODE < VERSION(2,5,0)
/************************************
* LINUX Rev. < 2.5.0
*************************************/
static void a2818_dispatch_pkt(struct a2818_state *s)
{
        int i, j, pkt_sz, slave, nlw, last_pkt;
        int pg_idx, pg_off, pg_sz, cp_sz, tmp_sz;
        u32 header, dummy, mask = 0;
        u8 *buf;
        struct kiobuf *iobuf;

        char *i_buf = s->i_buf;


        i = 0;
        do {
                pkt_sz = (i_buf[i] & 0xFF) + ((i_buf[i+1] << 8) & 0x0100);
                last_pkt = i_buf[i+1] & 0x02;
                i += 3;
                slave = i_buf[i++] & 0xF;
                mask |= 1 << slave;
                nlw = (pkt_sz >> 1) + (pkt_sz & 1);

                if( last_pkt ) {

                        pkt_sz -= 1;    // per togliere lo status
                        s->rx_status[slave] = (i_buf[i + pkt_sz * 2 - 1] << 8) +
                                               i_buf[i + pkt_sz * 2];
#if DEBUG
printk(KERN_INFO PFX "status = %02x slave = %02x\n", s->rx_status[slave], slave);
#endif
                }

                if( slave >= MAX_V2718 ) {
#if DEBUG
printk(KERN_INFO PFX "Wrong packet %04x %02x\n", pkt_sz, slave);
#endif
                        return;
                }

                iobuf = s->il_buf[slave];
#if DEBUG
//printk(KERN_INFO PFX "pkt_sz 0x%04x - slave 0x%02x, %d\n", pkt_sz, slave, i );
#endif
                /* Some integrity checks */
                if(
                    (iobuf->length > 0) &&
                    (nlw <= (s->i_buf_sz / 4)) &&
                    (slave < MAX_V2718) &&
                    (iobuf->nr_pages > 0) &&
                    (pkt_sz >= 0) &&
                    (pkt_sz <= 0x100)
                  ) {
                        tmp_sz = pkt_sz * 2;
                        while( tmp_sz > 0 ) {
                                // Calcolo l'indice di pagina
                                pg_idx = (s->il_buf_sz[slave] + iobuf->offset) / PAGE_SIZE;
                                // Calcolo l'offset nella pagina
                                pg_off = s->il_buf_sz[slave] - pg_idx * PAGE_SIZE + iobuf->offset;
                                // Calcolo quanto posto ho nella pagina
                                pg_sz = PAGE_SIZE - pg_off;
                                // Calcolo quanto posso copiare questo giro
                                cp_sz = MIN(pg_sz, tmp_sz);
#if DEBUG
//printk(KERN_INFO PFX "tmp_sz = %d, pg_off = %d, cp_sz = %d\n", tmp_sz, pg_off, cp_sz);
//printk(KERN_INFO PFX "pg_sz = %d, iobuf_offset = %d, pg_idx = %d\n", pg_sz, iobuf->offset, pg_idx);
#endif
                                // Ottengo il puntatore in user space
                                buf = (u8 *)(kmap_atomic(iobuf->maplist[pg_idx], KM_IRQ0) + pg_off);
                                // Copio i dati arrivati
                                memcpy(buf, &(i_buf[i]), cp_sz);
                                kunmap_atomic(buf, KM_IRQ0);

                                // Decremento tmp_sz di quello che ho copiato
                                tmp_sz -= cp_sz;
                                // Incremento il livello di utilizzo del buffer
                                s->il_buf_sz[slave] += cp_sz;
                                i += cp_sz;
                        }
                        if( last_pkt ) {
                                i += 2; // to wipe out the status word
                                s->rx_ready[slave] = 1;
                                wake_up_interruptible(&s->read_wait[slave]);
                        }
                        i += i % 4;  // 32-bit alignment
                } else {
                        printk(KERN_ERR PFX "Unhandled packet - slave %d, pkt_sz %d\n", slave, pkt_sz);
                        printk(KERN_ERR PFX "nr_pg %d, len %d, buf_sz %d\n", iobuf->nr_pages, iobuf->length, s->i_buf_sz);
                        break;
                }
        } while( i < s->i_buf_sz );
#if DEBUG
//        printk(KERN_INFO PFX "Esco dalla dispatch_pkt\n");
#endif
}
#else
/************************************
* LINUX Rev. >= 2.5.0
*************************************/
static void a2818_dispatch_pkt(struct a2818_state *s)
{
        int i, pkt_sz, slave, nlw, last_pkt;
        int tmp_sz;
        u32 mask = 0;
        char *i_buf = s->i_buf;

        i = 0;
        do {
                pkt_sz = (i_buf[i] & 0xFF) + ((i_buf[i+1] << 8) & 0x0100);
                last_pkt = i_buf[i+1] & 0x02;
                i += 3;
                slave = i_buf[i++] & 0xF;
                mask |= 1 << slave;
                nlw = (pkt_sz >> 1) + (pkt_sz & 1);

                if( last_pkt ) {
                        pkt_sz -= 1;    // per togliere lo status
                        s->rx_status[slave] = (i_buf[i + pkt_sz * 2 - 1] << 8) +
                                               i_buf[i + pkt_sz * 2];
#if DEBUG
printk(KERN_INFO PFX "status = %02x slave = %02x\n", s->rx_status[slave], slave);
#endif
                }

                if( slave >= MAX_V2718 ) {
#if DEBUG
printk(KERN_INFO PFX "Wrong packet %04x %02x\n", pkt_sz, slave);
#endif
                        return;
                }


#if DEBUG
//printk(KERN_INFO PFX "pkt_sz 0x%04x - slave 0x%02x, %d\n", pkt_sz, slave, i );
#endif
                /* Some integrity checks */
                if(
                    (nlw <= (s->i_buf_sz / 4)) &&
                    (slave < MAX_V2718) &&
		    		s->il_buf[slave] &&
                    (pkt_sz >= 0) &&
                    (pkt_sz <= 0x100)
                  ) {

                        tmp_sz = pkt_sz * 2;
						while( tmp_sz> 0)
						{
							int act_page_id= s->il_buff_act_page_id[ slave];
							int cp_sz;
							char* buf;
												
							if( act_page_id>= s->il_buff_pages[slave])
							{
		                        printk(KERN_ERR PFX "Bad page Id - slave: %d, page_id: %d length: %d offset: %d\n", slave, act_page_id, s->il_buf[slave][ act_page_id].length, s->il_buf[slave][act_page_id].offset);
								return;
								
							}
							
							cp_sz= (( PAGE_SIZE- s->il_buf[slave][act_page_id].offset)> tmp_sz)? tmp_sz: ( PAGE_SIZE- s->il_buf[slave][act_page_id].offset);
							
                            // Ottengo il puntatore in user space
                            buf = (u8 *)(kmap_atomic( s->il_buf[slave][ act_page_id].page, KM_IRQ0) + s->il_buf[slave][act_page_id].offset);
                            // Copio i dati arrivati
                            memcpy(buf, &(i_buf[i]), cp_sz);
                            kunmap_atomic(buf, KM_IRQ0);
                            tmp_sz -= cp_sz;
                        	s->il_buf_sz[slave] += cp_sz;
                        	i += cp_sz;
							s->il_buf[slave][act_page_id].offset+= cp_sz;

							if( s->il_buf[slave][act_page_id].offset>= PAGE_SIZE)
							{
								s->il_buff_act_page_id[ slave]++;
							}
						}

                        if( last_pkt ) {
                                i += 2; // to wipe out the status word
                                s->rx_ready[slave] = 1;
                                wake_up_interruptible(&s->read_wait[slave]);
                        }
                        i += i % 4;  // 32-bit alignment
                } else {
                        printk(KERN_ERR PFX "Unhandled packet - slave %d, pkt_sz %d\n", slave, pkt_sz);
// Rev. 1.6
                        printk(KERN_ERR PFX "nr_pg %d, len %d, page %p\n", s->il_buff_act_page_id[ slave], s->il_buf[slave][ s->il_buff_act_page_id[ slave]].length, s->il_buf[slave][ s->il_buff_act_page_id[ slave]].page);

                        break;
                }
        } while( i < s->i_buf_sz );
#if DEBUG
//        printk(KERN_INFO PFX "Esco dalla dispatch_pkt\n");
#endif
}
#endif
#endif // SAFE_MODE

/*
        ----------------------------------------------------------------------

        a2818_handle_vme_irq

        ----------------------------------------------------------------------
*/
static void a2818_handle_vme_irq(u32 irq0, u32 irq1, struct a2818_state *s)
{
        int i;

#if DEBUG
printk(KERN_INFO PFX "irq0 0x%08x - irq1 0x%08x\n", irq0, irq1);
#endif
        for( i = 0; i < MAX_V2718; i++ ) {
                /* Save IRQ levels on global struct */
                // Perche' ? non e' meglio andare a rileggere i registri nella ioctl ?
                s->intr[i] = ((i < 4 ? irq0 : irq1) & (0xFF << (i * 8))) >> (i * 8);
                if( s->intr[i] )
                        /* Qui bisognerebbe disabilitare le interrupt che sono
                           state ricevute, ma come e' ora il fw non e' banale
                        */
                        /* Wake up sleeping processes */
                        wake_up_interruptible(&s->intr_wait[i]);
        }
// Per ora disabilito tutto, poi forse potro' farlo singolarmente
        DISABLE_VME_INT
}

/*
        ----------------------------------------------------------------------

        a2818_interrupt

        ----------------------------------------------------------------------
*/

// Rev 1.5
#if LINUX_VERSION_CODE < VERSION(2,5,0)
static void a2818_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	#define IRQ_HANDLED
// Rev 1.10
#elif  LINUX_VERSION_CODE < VERSION(2,6,23)
static irqreturn_t a2818_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
#else
static irqreturn_t a2818_interrupt(int irq, void *dev_id)
{
#endif
        struct a2818_state *s = (struct a2818_state *)dev_id;
// Rev. 1.6
        uint32_t plx_intcsr = 0;
// Rev 1.5
//        unsigned long data, header;
//        int nw, nlw, i, app;
// Rev. 1.6
        uint32_t tr_stat;

// 1.4
        spin_lock( &s->Alock);

        /* Read PLX interrupt register */
        plx_intcsr = readl(s->plx_vir + PLX_INTCSR);

        if( !(plx_intcsr & (INTCSR_DMA1_INT_ACT | INTCSR_DMA0_INT_ACT | INTCSR_LOC_INT_ACT)))
        {
// 1.4
                spin_unlock( &s->Alock);
                return IRQ_HANDLED;
        }


#if DEBUG
        if( plx_intcsr & (INTCSR_DMA1_INT_ACT | INTCSR_DMA0_INT_ACT | INTCSR_LOC_INT_ACT) )
                printk(KERN_INFO PFX "Entro nell'interrupt - %.8x\n", plx_intcsr);
#endif
        /* DMA write interrupt ? */
        if( plx_intcsr & INTCSR_DMA1_INT_ACT ) {
                writel( DMACSR_ENA_0| DMACSR_ENA_1| DMACSR_CLRINT_1, s->plx_vir + PLX_DMACSR);
// Rev 1.6
                a2818_mmiowb();
                /* Per ora non faccio niente, poi si vedra' */
        }

        /* DMA read interrupt ? */
        if( plx_intcsr & INTCSR_DMA0_INT_ACT ) {

                writel(DMACSR_ENA_0| DMACSR_ENA_1| DMACSR_CLRINT_0, s->plx_vir + PLX_DMACSR);

                s->i_buf_sz = readl(s->plx_vir + PLX_DMASIZE0);
                s->DMAInProgress = 0;
#if DEBUG
printk(KERN_INFO PFX "DMA-size read = %d\n", s->i_buf_sz);
#endif
//printk(KERN_INFO PFX "DMA-size read = %d - minor = %d\n", s->i_buf_sz, s->minor);
                /* Re-enable interrupt on local bus */

                ENABLE_RX_INT

                /* Dispatch global buffer in per-slave buffers */
                a2818_dispatch_pkt(s);

#if DEBUG
        printk(KERN_INFO PFX "DMA done\n");
#endif
        }

       /* Check if the interrupt comes from the A2818 (pkt rx or VME irq)*/
        if( plx_intcsr & INTCSR_LOC_INT_ACT ) {
                u32 irq0, irq1;

                /* VME interrupt ? */
                irq0 = readl(s->baseaddr + A2818_IRQSTAT0);
                irq1 = readl(s->baseaddr + A2818_IRQSTAT1);
#if DEBUG
//        printk(KERN_INFO PFX "irq0 = %#lx - irq1 = %#lx\n", irq0, irq1);
#endif
                if( irq0 | irq1 ) {
                        a2818_handle_vme_irq(irq0, irq1, s);
                }

                if( !(s->DMAInProgress) ) {
//                if( DMAREAD_DONE ) {
                        /*
                        Check if the interrupt is due to the receive of a packet or
                        if during the previous DMA transfer a packet is arrived
                        */
                        tr_stat = readl(s->baseaddr + A2818_TRSTAT);
#if DEBUG
        printk(KERN_INFO PFX "trstat = %#lx\n", (long)tr_stat);
#endif
                        if( tr_stat & 0xFFFF0000 ) {
                                s->tr_stat = tr_stat;
                                a2818_handle_rx_pkt(s, 0);
                        }
                }
        }

#if DEBUG
//        if( plx_intcsr & (INTCSR_DMA1_INT_ACT | INTCSR_DMA0_INT_ACT | INTCSR_LOC_INT_ACT) )
//                printk(KERN_INFO PFX "Esco dall'interrupt - %.8x\n", plx_intcsr);
#endif

// 1.4
        spin_unlock( &s->Alock);
	
	return IRQ_HANDLED;
}

/*
        ----------------------------------------------------------------------

        a2818_init_board

        ----------------------------------------------------------------------
*/
static int a2818_init_board(struct pci_dev *pcidev, int index)
{
        struct a2818_state *s;
        int i;
        int ret = 0;

#if LINUX_VERSION_CODE >= VERSION(2,3,0)
        if (pci_enable_device(pcidev))
                return -1;
#endif
        if (pcidev->irq == 0)
                return -1 ;

        s = kmalloc(sizeof(*s), GFP_KERNEL);
        if (!s) {
                printk(KERN_ERR PFX "Out of memory\n");
                return -2;
        }

        memset(s, 0, sizeof(struct a2818_state));

        s->DMAInProgress = 0;
// Rev. 1.6
#if LINUX_VERSION_CODE >= VERSION(2,6,9)
        s->pcidev = pcidev;
#endif

// 1.4
        spin_lock_init( &s->Alock);
// Rev. 1.6
#if LINUX_VERSION_CODE >= VERSION(2,6,11)
        mutex_init(&s->ioctl_lock);
#endif
        for( i = 0; i < MAX_V2718; i++ ) {
                init_waitqueue_head(&s->read_wait[i]);
                init_waitqueue_head(&s->intr_wait[i]);
                /*
                        Alloc all the DMA output buffers
                        v1.3d - Moved here from open function
                */
// Rev. 1.6
#if LINUX_VERSION_CODE < VERSION(2,6,9)
                s->o_buf[i] = kmalloc(A2818_MAX_PKT_SZ, GFP_KERNEL | GFP_DMA);
#else
                s->o_buf[i] = dma_alloc_coherent(&pcidev->dev, A2818_MAX_PKT_SZ,
                                                 &s->o_buf_pci[i], GFP_KERNEL);
#endif
		//
		// Rev 1.5
        	if (!(s->o_buf[i])) {
                	printk(KERN_ERR PFX "Out of DMA memory s->o_buf i: %d\n", i);
                	goto err_obuf;
        	}
        }
	
        /* Alloc maximum size for DMA (in) */
// Rev. 1.6
#if LINUX_VERSION_CODE < VERSION(2,6,9)
        s->i_buf = kmalloc(A2818_MAX_PKT_SZ, GFP_KERNEL | GFP_DMA);
#else
        s->i_buf = dma_alloc_coherent(&pcidev->dev, A2818_MAX_PKT_SZ, &s->i_buf_pci, GFP_KERNEL);
#endif
        if (!(s->i_buf)) {
                printk(KERN_ERR PFX "Out of DMA memory s->i_buf\n");
                goto err_obuf;
        }

#if LINUX_VERSION_CODE >= VERSION(2,3,0)
        s->phys = pci_resource_start(pcidev, 2);
        if (s->phys == 0) {
                  // rev 1.13
                  goto err_region1;
                }
        if (!request_mem_region(s->phys, A2818_REGION_SIZE, "a2818")) {
                printk(KERN_ERR PFX "io mem %#lx-%#lx in use\n",
                s->phys, s->phys + A2818_REGION_SIZE - 1);
                goto err_region1;
        }
        /* Mapping of PLX control registers */
        s->plx_phy = pci_resource_start(pcidev, 0);
        if (!request_mem_region(s->plx_phy, PLX_REGION_SIZE, "a2818_plx")) {
                printk(KERN_ERR PFX "io mem %#lx-%#lx in use\n",
                s->plx_phy, s->plx_phy + PLX_REGION_SIZE - 1);
                goto err_region2;
        }
#else
        s->phys = pcidev->base_address[2] & PCI_BASE_ADDRESS_MEM_MASK;
        s->plx_phy = pcidev->base_address[0] & PCI_BASE_ADDRESS_MEM_MASK;
#endif

        s->plx_vir = (unsigned char *)ioremap(s->plx_phy, PLX_REGION_SIZE - 1);
        s->baseaddr = (char *)ioremap(s->phys, A2818_REGION_SIZE);
        printk(KERN_INFO PFX "found A2818 adapter at iomem %#08lx irq %u, PLX at %#08lx\n",
                s->phys, s->irq, s->plx_phy);

        s->irq = pcidev->irq;
        /* request irq */
// Rev 1.10
#if LINUX_VERSION_CODE < VERSION(2,6,23)
        if (request_irq(s->irq, a2818_interrupt, SA_SHIRQ, "a2818", s)) {
#else
        if (request_irq(s->irq, a2818_interrupt, IRQF_SHARED , "a2818", s)) {
#endif
			printk(KERN_ERR PFX "irq %u in use\n", s->irq);
			goto err_irq;
        }

        a2818_reset(s);

        a2818_enable_int(s);
        a2818_dma_conf(s);

#if DEBUG
        printk("04 = 0x%08x\n", readl(s->plx_vir + 0x04));
        printk("18 = 0x%08x\n", readl(s->plx_vir + 0x18));
        printk("0c = 0x%08x\n", readl(s->plx_vir + 0x0c));
        printk("68 = 0x%08x\n", readl(s->plx_vir + 0x68));
#endif
        printk(KERN_INFO PFX "  CAEN A2818 Loaded.\n");

        a2818_register_proc();
        s->i_buf_sz = 0;
        s->minor = index;
// Rev. 1.9		
#if LINUX_VERSION_CODE < VERSION(2,6,9)
         s->timeout = 1500;
#else
	s->timeout= msecs_to_jiffies( 15000);
#endif

#if SAFE_MODE
	{
		int i; 
		for( i= 0; i< MAX_V2718; i++) {
			s->il_buf[i] = (u8 *)vmalloc(1024*1024);
			if( s->il_buf[i]== 0)
			{
                		printk(KERN_ERR PFX "unable to alloc vmalloc buffers\n");
                		goto err_vmalloc_kiovec_kmalloc;
			}
		}
	}
#else
	// Rev 1.5	
	#if LINUX_VERSION_CODE < VERSION(2,5,0)
        	if (alloc_kiovec(MAX_V2718, s->il_buf) < 0) {
                	printk(KERN_ERR PFX "unable to alloc kiovec\n");
                	goto err_vmalloc_kiovec_kmalloc;
        	}
	#else
		{
			int i; 
			for( i= 0; i< MAX_V2718; i++)
			{
				s->il_buf[i]= kmalloc( sizeof( struct scatterlist)*MAX_USER_PAGES, GFP_KERNEL);
				if( s->il_buf[i]== 0)
				{
                	printk(KERN_ERR PFX "unable to alloc scatterlist buffers\n");
                	goto err_vmalloc_kiovec_kmalloc;
				}
			}
		}
	#endif
#endif // SAFE_MODE
        /* queue it for later freeing */
        s->next = devs;
        devs = s;
        return 0;

err_vmalloc_kiovec_kmalloc:
      ret = -6;
      printk(KERN_INFO PFX "Unable to alloc kernel memory. Please reboot your pc!"); 
      #if SAFE_MODE
      		{
      			int i; 
      			for( i= 0; i< MAX_V2718; i++)
      			{
      				if (s->il_buf[i]) vfree( s->il_buf[i]);
      			}
      		}
      #else
      	// Rev 1.5	
      	#if LINUX_VERSION_CODE < VERSION(2,5,0)
      	   if (s->il_buf) free_kiovec(MAX_V2718, s->il_buf)
      	#else
      		{
      			int i; 
      			for( i= 0; i< MAX_V2718; i++)
      			{
      				if (s->il_buf[i]) kfree( s->il_buf[i]);
      			}
      		}
      	#endif
      #endif // SAFE_MODE
      
      free_irq(s->irq, s); // release irq;

err_irq:
        if (ret == 0) ret = -5;
        printk(KERN_INFO PFX "Unable to find the device A2818 IRQ");
        iounmap(s->baseaddr);
        iounmap(s->plx_vir);
        
        #if LINUX_VERSION_CODE >= VERSION(2,3,0)
                release_mem_region(s->plx_phy, PLX_REGION_SIZE);
        #endif


err_region2:
        if (ret == 0) ret = -4;
        printk(KERN_INFO PFX "Unable to map the A2818 registry into the IO memory. Try to reboot the PC!");
        #if LINUX_VERSION_CODE >= VERSION(2,3,0)
                release_mem_region(s->phys, A2818_REGION_SIZE);
        #endif

err_region1:
        if (ret == 0) ret = -3;
        printk(KERN_INFO PFX "Kernel memory fragmented! Unable to insert driver. Please reboot your PC!");
        #if LINUX_VERSION_CODE < VERSION(2,6,9)
          kfree(s->i_buf);
        #else
          dma_free_coherent(&pcidev->dev, A2818_MAX_PKT_SZ, s->i_buf, s->i_buf_pci);
        #endif
err_obuf:    
        printk(KERN_INFO PFX "Kernel memory fragmented! Unable to insert driver. Please reboot your PC!"); 
        if (ret == 0) ret = -3;    
        /*
                Free all the DMA output buffer
                v1.3d - Moved here from open function
        */
        {
        int i;
        for( i = 0; i < MAX_V2718; i++ ) {
        // Rev. 1.6
            if (s->o_buf[i]) {

                #if LINUX_VERSION_CODE < VERSION(2,6,9)
                        kfree(s->o_buf[i]);
                #else
                        dma_free_coherent(&pcidev->dev, A2818_MAX_PKT_SZ,
                                          s->o_buf[i], s->o_buf_pci[i]);
                #endif
            }
        }
        }
        kfree(s);

        return ret;
}

/*
        ----------------------------------------------------------------------

        a2818_init

        ----------------------------------------------------------------------
*/
#if LINUX_VERSION_CODE < VERSION(2,3,0)
static int init_module(void)
#else
static int __init a2818_init(void)
#endif
{
        struct pci_dev *pcidev = NULL;
        int index = 0;
	
/*
  from 2.6 on pci_present is obsolete:
  From http://www.linux-m32r.org/lxr/http/source/Documentation/pci.txt?v=2.6.10 ...
' ... Since ages, you don't need to test presence
  of PCI subsystem when trying to talk to it.
  If it's not there, the list of PCI devices
  is empty and all functions for searching for
  devices just return NULL. '
*/
#if LINUX_VERSION_CODE < VERSION(2,5,0)
	#ifdef CONFIG_PCI
        	if (!pci_present())   /* No PCI bus in this machine! */
                	return -ENODEV;
	#endif
#endif
        printk(KERN_INFO PFX "CAEN A2818 CONET controller driver %s\n", DRIVER_VERSION);
        printk(KERN_INFO PFX "  Copyright 2004, CAEN SpA\n");

#if LINUX_VERSION_CODE < VERSION(2,3,0)
        while (index < MAX_MINOR && (
        (pcidev = pci_find_device(PCI_VENDOR_ID_PLX, PCI_DEVICE_ID_PLX_9054, pcidev)))) {
                unsigned short ss_id;

                pci_read_config_word(pcidev, PCI_SUBSYSTEM_ID, &ss_id);

                if( ss_id == PCI_SUBDEVICE_ID_CAEN_A2818 ) {
                        if (a2818_init_board(pcidev, index)) ReleaseBoards(); // ver 1.13
                        index++;
                }
        }
#elif LINUX_VERSION_CODE < VERSION(2,5,0)
        while (index < MAX_MINOR && (
               (pcidev = pci_find_subsys(PCI_VENDOR_ID_PLX,
                                         PCI_DEVICE_ID_PLX_9054,
                                         PCI_VENDOR_ID_PLX,
                                         PCI_SUBDEVICE_ID_CAEN_A2818,
                                        pcidev))) ) {
                if (a2818_init_board(pcidev, index)) ReleaseBoards(); // ver 1.13
                index++;
        }
#else
		while (index < MAX_MINOR && (
	    	 (pcidev = pci_get_subsys(PCI_VENDOR_ID_PLX,
                        	    	   PCI_DEVICE_ID_PLX_9054,
                        	    	   PCI_VENDOR_ID_PLX,
                        	    	   PCI_SUBDEVICE_ID_CAEN_A2818,
                        	    	  pcidev))) ) {
	    	  if (a2818_init_board(pcidev, index)) ReleaseBoards(); // ver 1.13
	    	  index++;
		}
		if( !index)
			// No PCI device found
        		return -ENODEV;

#endif
/* 
	   Ver. 1.8 - Register device only if HW is present otherwise the kernel
	              crashes with a cat /proc/devices
*/
    if( index > 0 ) {
        /* register device only if HW is present*/
        a2818_major = register_chrdev(0, "a2818", &a2818_fops);
        if ( a2818_major < 0 ) {
            printk(KERN_INFO PFX "  Error getting Major Number.\n");
            return -ENODEV;
        }
	  }
        printk(KERN_INFO PFX "  CAEN A2818: %d device(s) found.\n", index);

        return 0;
}

// Ver 1.13
static void ReleaseBoards() {

        struct a2818_state *s;
        int i;
        while ((s = devs)) {
                devs = devs->next;

                free_irq(s->irq, s);

                iounmap(s->plx_vir);
                iounmap(s->baseaddr);
#if LINUX_VERSION_CODE >= VERSION(2,3,0)
                release_mem_region(s->phys, A2818_REGION_SIZE);
                release_mem_region(s->plx_phy, PLX_REGION_SIZE);
#endif
// Rev. 1.6
#if LINUX_VERSION_CODE < VERSION(2,6,9)
                kfree(s->i_buf);
#else
                dma_free_coherent(&s->pcidev->dev, A2818_MAX_PKT_SZ, s->i_buf, s->i_buf_pci);
#endif
                for( i = 0; i < MAX_V2718; i++ ) {
                          // Rev. 1.6
                    if (s->o_buf[i]) {

                       #if LINUX_VERSION_CODE < VERSION(2,6,9)
                          kfree( s->o_buf[i]);
	                     #else
                          dma_free_coherent(&s->pcidev->dev, A2818_MAX_PKT_SZ,
                                            s->o_buf[i], s->o_buf_pci[i]);
                       #endif
                    }
                }
                
                #if SAFE_MODE 
                {
                			int i; 
                			for( i= 0; i< MAX_V2718; i++) {
                				if (s->il_buf[i]) vfree( s->il_buf[i]);
                			}
                }
                #else	
                  	#if LINUX_VERSION_CODE < VERSION(2,5,0)
                  	   if (s->il_buf) free_kiovec(MAX_V2718, s->il_buf)
                  	#else
                  		{
                  			int i; 
                  			for( i= 0; i< MAX_V2718; i++) {
                  				if (s->il_buf[i]) kfree( s->il_buf[i]);
                  			}
                  		}
                  	#endif
                #endif
        kfree(s);
        a2818_unregister_proc();        
        }
}

/*
        ----------------------------------------------------------------------

        a2818_cleanup

        ----------------------------------------------------------------------
*/
#if LINUX_VERSION_CODE < VERSION(2,3,0)
static void cleanup_module(void)
#else
static void __exit a2818_cleanup(void)
#endif
{
        ReleaseBoards();
        unregister_chrdev(a2818_major, "a2818");
        printk(KERN_INFO "CAEN A2818: unloading.\n");

}

#if !SAFE_MODE
#if LINUX_VERSION_CODE >= VERSION(2,5,0)
/*
        ----------------------------------------------------------------------

        sgl_map_user_pages: copied from /drivers/scsi/st.c

        ----------------------------------------------------------------------
*/
/* The following functions may be useful for a larger audience. */
static int sgl_map_user_pages(struct scatterlist *sgl, const unsigned int max_pages,
			      unsigned long uaddr, size_t count, int rw)
{
	int res, i, j;
	unsigned int nr_pages;
	struct page **pages;

	nr_pages = ((uaddr & ~PAGE_MASK) + count + ~PAGE_MASK) >> PAGE_SHIFT;

	// User attempted Overflow! 
	if ((uaddr + count) < uaddr)
		return -EINVAL;

	// Too big 
        if (nr_pages > max_pages)
		return -ENOMEM;

	// Hmm? 
	if (count == 0)
		return 0;

	if ((pages = kmalloc(max_pages * sizeof(*pages), GFP_KERNEL)) == NULL)
		return -ENOMEM;
        // Try to fault in all of the necessary pages 
	down_read(&current->mm->mmap_sem);
        // rw==READ means read from drive, write into memory area
	
	res = get_user_pages(
		current,
		current->mm,
		uaddr,
		nr_pages,
		rw == READ,
		0, 
		pages,
		NULL);
	up_read(&current->mm->mmap_sem);

	// Errors and no page mapped should return here
	if (res < nr_pages)
		goto out_unmap;

        for (i=0; i < nr_pages; i++) {
				//
                // FIXME: flush superflous for rw==READ,
                // probably wrong function for rw==WRITE
                //
		flush_dcache_page(pages[i]);
        }

	// Populate the scatter/gather list
	sgl[0].page = pages[0]; 
	sgl[0].offset = uaddr & ~PAGE_MASK;
	if (nr_pages > 1) {
		sgl[0].length = PAGE_SIZE - sgl[0].offset;
		count -= sgl[0].length;
		for (i=1; i < nr_pages ; i++) {
			sgl[i].offset = 0;
			sgl[i].page = pages[i]; 
			sgl[i].length = count < PAGE_SIZE ? count : PAGE_SIZE;
			count -= PAGE_SIZE;
		}
	}
	else {
		sgl[0].length = count;
	}

	kfree(pages);
	return nr_pages;

 out_unmap:
	if (res > 0) {
		for (j=0; j < res; j++)
			page_cache_release(pages[j]);
	}
	kfree(pages);
//NDA: modified from st.c	
//	return res ;
	return ( res> 0) ? -res: res ;
}


/*
        ----------------------------------------------------------------------

        sgl_unmap_user_pages: copied from /drivers/scsi/st.c

        ----------------------------------------------------------------------
*/
/* And unmap them... */
static int sgl_unmap_user_pages(struct scatterlist *sgl, const unsigned int nr_pages,
				int dirtied)
{
	int i;

	for (i=0; i < nr_pages; i++) {
 	  if(dirtied && !PageReserved(sgl[i].page))
			SetPageDirty(sgl[i].page);
 	    /* FIXME: cache flush missing for rw==READ
 	     * FIXME: call the correct reference counting function
 	     */
 	    page_cache_release(sgl[i].page);
	}

	return 0;
}
#endif
#endif // SAFE_MODE


#if LINUX_VERSION_CODE >= VERSION(2,3,0)
module_init(a2818_init);
module_exit(a2818_cleanup);
#endif
MODULE_LICENSE("GPL");

MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );

