/*
        ----------------------------------------------------------------------

        --- CAEN SpA - Computing Systems Division ---

        a2818.h

        Header file for the CAEN A2818 CONET board driver.

        June  2004 :   Created.

        ----------------------------------------------------------------------
*/
#ifndef _a2818_H
#define _a2818_H

#ifndef VERSION
	#define VERSION(ver,rel,seq) (((ver)<<16) | ((rel)<<8) | (seq))
#endif	

// Rev 1.5
#if LINUX_VERSION_CODE < VERSION(2,5,0)

	/*
        	----------------------------------------------------------------------
        	The following implements an interruptible wait_event
        	with a timeout.  This is used instead of the function
        	interruptible_sleep_on_timeout() since this is susceptible
        	to race conditions.
        	----------------------------------------------------------------------
	*/
	#define __wait_event_interruptible_timeout(wq, condition, ret)   	\
	do {                                                          		\
        	wait_queue_t __wait;                                  		\
        	init_waitqueue_entry(&__wait, current);               		\
                                                              			\
        	add_wait_queue(&wq, &__wait);                         		\
        	for (;;) {                                            		\
                	set_current_state(TASK_INTERRUPTIBLE);        		\
                	if (condition)                                          \
                        	break;                                		\
                	if (!signal_pending(current)) {               		\
                        	ret = schedule_timeout(ret);          		\
                        	if (!ret)                             		\
                                	break;                        		\
                        	continue;                             		\
                	}                                             		\
                	ret = -ERESTARTSYS;                           		\
                	break;                                        		\
        	}                                                     		\
        	current->state = TASK_RUNNING;                        		\
        	remove_wait_queue(&wq, &__wait);                      		\
	} while (0)

	#define wait_event_interruptible_timeout(wq, condition, timeout)	                \
	({                                                              	                \
        	long __ret = timeout;                                   	                \
        	if (!(condition))                                       	                \
                	__wait_event_interruptible_timeout(wq, condition, __ret);        \
        	__ret;                                                                          \
	})
	#define MIN(a, b)       ( (a) < (b) ? (a) : (b) )
#endif

/*
        Defines for the a2818
*/
#define MIN_DMA_SIZE                    (80)
#define MAX_MINOR                       (256)
#define PFX                             "a2818: "

#define MAX_V2718                       (8)

#define A2818_MAX_PKT_SZ                (131072)

#define A2818_RXFIFO_SZ                 (8192)
#define A2818_REGION_SIZE               (0x100)
#define PLX_REGION_SIZE                 (0x100)

#define PCI_DEVICE_ID_PLX_9054          (0x9054)
#define PCI_SUBDEVICE_ID_CAEN_A2818	(0x3057)

#define PCI_SIZE_8                      (0x0001)
#define PCI_SIZE_16                     (0x0002)
#define PCI_SIZE_32                     (0x0003)

#define A2818_MAGIC                     '8'

#define A2818_IOCTL_RESET               _IO(A2818_MAGIC, 0)
#define A2818_IOCTL_COMM                _IOWR(A2818_MAGIC, 1, a2818_comm_t)
#define A2818_IOCTL_REG_WR              _IOW(A2818_MAGIC, 2, a2818_reg_t)
#define A2818_IOCTL_REG_RD              _IOWR(A2818_MAGIC, 3, a2818_reg_t)
#define A2818_IOCTL_IRQ_WAIT            _IOW(A2818_MAGIC, 4, a2818_intr_t)
#define A2818_IOCTL_SEND                _IOWR(A2818_MAGIC, 5, a2818_comm_t)
#define A2818_IOCTL_RECV                _IOWR(A2818_MAGIC, 6, int)
#define A2818_IOCTL_REV                 _IOWR(A2818_MAGIC, 7, a2818_rev_t)

#define PCI_ID                          (0x0000)
#define PCI_CSR                         (0x0004)
#define PCI_CLASS                       (0x0008)
#define PCI_MISC0                       (0x000C)
#define PCI_BS                          (0x0010)
#define PCI_MISC1                       (0x003C)


/*
        A2818 Registers offsets
*/
#define A2818_TXFIFO                    (0x00)
#define A2818_RXFIFO                    (0x04)
#define A2818_IOCTL                     (0x08)
#       define A2818_LNKRST                     (1)
#       define A2818_INTDIS                     (1 << 1)
#       define A2818_SERVICE                    (1 << 2)
#       define A2818_VINTDIS                    (1 << 3)
#define A2818_STATUS                    (0x0C)
#       define A2818_RXFIFO_EMPTY               (1)
#       define A2818_TXFIFO_FULL                (1 << 1)
#       define A2818_LINK_FAIL                  (1 << 8)    // Ver 1.2
#       define A2818_LINT                       (1 << 9)    // NDA
#define A2818_FWDLD                     (0x10)
#define A2818_FLENA                     (0x14)
#define A2818_TRSTAT                    (0x18)
#define A2818_FWREL                     (0x1C)
#define A2818_DEBUG                     (0x20)
#define A2818_IRQSTAT0                  (0x24)
#define A2818_IRQSTAT1                  (0x28)
#define A2818_IRQSLAVES                 (0x2C)
#define A2818_IRQMASK0_SET              (0x30)
#define A2818_IRQMASK0_CLR              (0x34)
#define A2818_IRQMASK1_SET              (0x38)
#define A2818_IRQMASK1_CLR              (0x3C)
#define A2818_IOCTL_SET                 (0xA0)
#define A2818_IOCTL_CLR                 (0xA4)

/*
        PLX9054 Registers offsets
*/
#define PLX_INTCSR                      (0x68)
#       define INTCSR_PCI_INT_ENA               (1 <<  8)
#       define INTCSR_LOC_INT_ENA               (1 << 11)
#       define INTCSR_LOC_INT_ACT               (1 << 15)
#       define INTCSR_DMA0_INT_ENA              (1 << 18)
#       define INTCSR_DMA0_INT_ACT              (1 << 21)
#       define INTCSR_DMA1_INT_ENA              (1 << 19)
#       define INTCSR_DMA1_INT_ACT              (1 << 22)
#define PLX_CNTRL                       (0x6C)
#       define CNTRL_CONF_RELOAD                (1 << 29)
#       define CNTRL_SW_RESET                   (1 << 30)
#define PLX_DMAMODE0                    (0x80)
#define PLX_DMAMODE1                    (0x94)
#       define DMAMODE_DW16                    (1)
#       define DMAMODE_DW32                    (1 <<  1)
#       define DMAMODE_READY_ENA               (1 <<  6)
#       define DMAMODE_BTERM_ENA               (1 <<  7)
#       define DMAMODE_BURST_ENA               (1 <<  8)
#       define DMAMODE_SG_ENA                  (1 <<  9)
#       define DMAMODE_DONE_INT_ENA            (1 << 10)
#       define DMAMODE_LOC_ADR_HOLD            (1 << 11)
#       define DMAMODE_EOT_ENA                 (1 << 14)
#       define DMAMODE_FAST_TERM               (1 << 15)
#       define DMAMODE_INT_PCI                 (1 << 17)
#define PLX_DMAPADR0                    (0x84)
#define PLX_DMAPADR1                    (0x98)
#define PLX_DMALADR0                    (0x88)
#define PLX_DMALADR1                    (0x9C)
#define PLX_DMASIZE0                    (0x8C)
#define PLX_DMASIZE1                    (0xA0)
#define PLX_DMADPR0                     (0x90)
#define PLX_DMADPR1                     (0xA4)
#define PLX_DMACSR                      (0xA8)
#       define DMACSR_ENA_0                    (1)
#       define DMACSR_START_0                  (1 <<  1)
#       define DMACSR_ABORT_0                  (1 <<  2)
#       define DMACSR_CLRINT_0                 (1 <<  3)
#       define DMACSR_DONE_0                   (1 <<  4)
#       define DMACSR_ENA_1                    (1 <<  8)
#       define DMACSR_START_1                  (1 <<  9)
#       define DMACSR_ABORT_1                  (1 << 10)
#       define DMACSR_CLRINT_1                 (1 << 11)
#       define DMACSR_DONE_1                   (1 << 12)

#ifdef __KERNEL__

#if LINUX_VERSION_CODE >= VERSION(2,5,0)
		#define MAX_USER_PAGES  (( 16*1024*1024 + ~PAGE_MASK) >> PAGE_SHIFT)
#endif


/*
        ----------------------------------------------------------------------

        Types

        ----------------------------------------------------------------------
*/
struct a2818_state {
        /* Common globals */
        unsigned char          *baseaddr;
        unsigned long           phys;
        unsigned char          *plx_vir;
        unsigned long           plx_phy;
// Rev. 1.6
        struct pci_dev	       *pcidev;
        int                     irq;
        int                     minor;
        int                     timeout;
#ifdef PINGPONG
        unsigned char          *i_buf[2];          // buffer per i dma. CPU address.
// Rev. 1.6
#if LINUX_VERSION_CODE >= VERSION(2,6,9)
        dma_addr_t              i_buf_pci[2];      // PCI address of i_buf.
#endif
        int                     i_buf_sz[2];
// Rev. 1.6
        uint32_t                tr_stat[2];
        int                     pp;
#else
// Rev. 1.6
        unsigned char          *i_buf;          // buffer per i dma. CPU address.
#if LINUX_VERSION_CODE >= VERSION(2,6,9)
        dma_addr_t              i_buf_pci;      // PCI address of i_buf.
#endif
        int                     i_buf_sz;
// Rev. 1.6
        uint32_t                tr_stat;
#endif
        unsigned int            reads;
        unsigned int            writes;
        unsigned int            ioctls;

        unsigned int            DMAInProgress;
        unsigned long           flags;
// Rev. 1.4
        spinlock_t              Alock;
// Rev 1.6
#if LINUX_VERSION_CODE >= VERSION(2,6,11)
        struct mutex           ioctl_lock;
#endif

        /* Per slave globals */
// Rev 1.6
        uint32_t               *o_buf[MAX_V2718];
#if LINUX_VERSION_CODE >= VERSION(2,6,9)
        dma_addr_t              o_buf_pci[MAX_V2718];
#endif
        int                     o_buf_sz[MAX_V2718];
        wait_queue_head_t       read_wait[MAX_V2718];
        wait_queue_head_t       intr_wait[MAX_V2718];
#if SAFE_MODE
        unsigned char          *il_buf[MAX_V2718];
		int		   				il_buf_p[MAX_V2718];
#else 
	// Rev 1.5	
	#if LINUX_VERSION_CODE < VERSION(2,5,0)
        struct kiobuf          *il_buf[MAX_V2718];
	#else

		struct scatterlist		*il_buf[MAX_V2718];
        int          	        il_buff_pages[MAX_V2718];				// numero di pagine allocate in il_buff
        int          	        il_buff_act_page_id[MAX_V2718];			// indice di pagina attuale in il_buff
	#endif	
#endif // SAFE MODE
        int                     il_buf_sz[MAX_V2718];
        int                     rx_ready[MAX_V2718];
        int                     rx_status[MAX_V2718];
        unsigned int            intr[MAX_V2718];

        /* we keep a2818 cards in a linked list */
        struct                  a2818_state *next;
};
#endif
/*
	Struct for communication argument in ioctl calls
*/
typedef struct a2818_comm {
        const char *out_buf;
        int         out_count;
        char       *in_buf;
        int         in_count;
        int        *status;
} a2818_comm_t;

/*
	Struct for register argument in ioctl calls
*/
typedef struct a2818_reg {
// Rev. 1.6
	uint32_t address;
	uint32_t value;
} a2818_reg_t;

/*
	Struct for interrupt argument in ioctl calls
*/
typedef struct a2818_intr {
        unsigned int Mask;
        unsigned int Timeout;
} a2818_intr_t;

// Rev 1.5
/*
	Struct for revision argument in ioctl calls
*/
#define A2818_DRIVER_VERSION_LEN	20
typedef struct a2818_rev {
        char 		rev_buf[A2818_DRIVER_VERSION_LEN];
} a2818_rev_t;

#endif
