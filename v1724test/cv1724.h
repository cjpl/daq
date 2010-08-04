/**********************************************************************
 * Name:       cv1724.h
 * Created by: Exaos Lee, 2010-08-02
 * 
 * Contents: CAEN V1724, 8 Channel 14 bit 100 MS/s Digitizer
 **********************************************************************/

#ifndef __CV1724_H__
#define __CV1724_H__  1

#ifdef OS_LINUX
#include <unistd.h>
#endif
#include <sys/types.h>

#include "midas.h"
#include "mvmestd.h"

/**********************************************************************
 * Definitions of registers: all are accessible in D32 mode
 **********************************************************************/
#define CV1724_CHN_CONFIG_RW          0x8000  /* Channel configuration */
#define CV1724_CHN_CONFIG_BIT_SET_WO  0x8004  /* Channel configuration bit set */
#define CV1724_CHN_CONFIG_BIT_CLR_WO  0x8008  /* Channel configuration bit clear */
#define CV1724_BUFFER_ORG_RW          0x800C  /* Buffer organization */
#define CV1724_BUFFER_FREE_RW         0x8010  /* Buffer free */
#define CV1724_CUSTOM_SIZE_RW         0x8020  /* Custom size */
#define CV1724_ANAMON_POLSH_RW        0x802A  /* Analog monitor polarity and shift */
#define CV1724_ACQ_CTRL_RW            0x8100  /* Acquisition control */
#define CV1724_ACQ_STATUS_RO          0x8104  /* Acquisition status */
#define CV1724_SW_TRIG_WO             0x8108  /* Software trigger */
#define CV1724_TRIG_SRC_EN_MASK_RW    0x810C  /* Trigger source enable mask */
#define CV1724_FP_TRIG_OUT_EN_MASK_RW 0x8110  /* Front panel trigger out enable mask */
#define CV1724_POST_TRIG_RW           0x8114  /* Post trigger settings */
#define CV1724_FP_IO_DATA_RW          0x8118  /* Front panel I/O data */
#define CV1724_FP_IO_CTRL_RW          0x811C  /* Front panel I/O control */
#define CV1724_CHN_EN_MASK_RW         0x8120  /* Channel enable mask */
#define CV1724_ROC_FPGA_REV_RO        0x8124  /* ROC FPGA firmware revision */
#define CV1724_DOWNSAMPLE_FACTOR_RW   0x8128  /* Down-sample factor */
#define CV1724_EVENT_STORED_RO        0x812C  /* Event stored */
#define CV1724_SET_MON_DAS_RW         0x8138  /* Set monitor DAC */
#define CV1724_BOARD_INFO_RO          0x8140  /* Board info */
#define CV1724_MON_MODE_RW            0x8144  /* Monitor mode */
#define CV1724_EVENT_SIZE_RO          0x814C  /* Event size */
#define CV1724_ANAMON_RW              0x8150  /* Analog monitor */

#define CV1724_VME_CTRL_RW            0xEF00  /* VME control */
#define CV1724_VME_STATUS_RO          0xEF04  /* VME status */
#define CV1724_BOARD_ID_RW            0xEF08  /* Board ID */
#define CV1724_MULTICAST_ADDR_RW      0xEF0C  /* Multicast base address & control */
#define CV1724_RELOC_ADDR_RW          0xEF10  /* Relocation address */
#define CV1724_INT_STATUS_ID_RW       0xEF14  /* Interrupt status ID */
#define CV1724_INT_EVENT_NUM_RW       0xEF18  /* Interrupt event number */
#define CV1724_BLT_EVENT_NUM_RW       0xEF1C  /* BLT event number */
#define CV1724_SCRATCH_RW             0xEF20  /* Scratch */
#define CV1724_SW_RESET_WO            0xEF24  /* Software reset */
#define CV1724_SW_CLEAR_WO            0xEF28  /* Software clear */
#define CV1724_FLASH_EN_RW            0xEF2C  /* Flash enable */
#define CV1724_FLASH_DATA_RW          0xEF30  /* Flash data */
#define CV1724_CONFIG_RELOAD_WO       0xEF34  /* Configuration reload */

#define CV1724_CONFIG_ROM_START_RO    0xF000  /* Configuration ROM: 0xF000~0xF3FC */

/* Registers for channel i: 0x10HH + 0x0i00; i=[0..7] */
#define CV1724_CHN0_ZS_THRES_RW       0x1024  /* ZS_THRES for channel 0 */
#define CV1724_CHN0_ZS_NSAMP_RW       0x1028  /* ZS_NSAMP for channel 0 */
#define CV1724_CHN0_THRES_RW          0x1080  /* Threshold for channel 0 */
#define CV1724_CHN0_T_OV_UN_THRES_RW  0x1084  /* Time over/under threshold for channel 0 */
#define CV1724_CHN0_STATUS_RO         0x1088  /* Status of channel 0 */
#define CV1724_CHN0_AMC_FPGA_REV_RO   0x108C  /* AMC FPGA firmware revision of channel 0 */
#define CV1724_CHN0_BUFF_OCCUP_RO     0x1094  /* Buffer occupancy of channel 0 */
#define CV1724_CHN0_DAC_RW            0x1098  /* DAC of channel 0 */
#define CV1724_CHN0_ADC_CONFIG_RW     0x109C  /* ADC configuration of channel 0 */

/**********************************************************************
 * Information
 **********************************************************************/
/* Verbose output baord information */
void cv1724_PrintBoardInfo(MVME_INTERFACE *mvme, DWORD base);

/**********************************************************************
 * Configurations
 **********************************************************************/

/* Trigger */

/* Interrupt */

/* Thresholds */

/* Channel i configuration */
void cv1724_PrintChnConfig(MVME_INTERFACE *mvme, DWORD base, int chn);

/**********************************************************************
 * Start/Stop/Reset/Status/Interrupt
 **********************************************************************/
int cv1724_Start (MVME_INTERFACE *mvme, DWORD base);
int cv1724_Stop  (MVME_INTERFACE *mvme, DWORD base);
int cv1724_Reset (MVME_INTERFACE *mvme, DWORD base);
int cv1724_Status(MVME_INTERFACE *mvme, DWORD base);
int cv1724_ChnStatus(MVME_INTERFACE *mvme, DWORD base, int chn);
int cv1724_IsDataReady(MVME_INTERFACE *mvme, DWORD base);
void cv1724_GetInterrupt(MVME_INTERFACE *mvme, DWORD base, u_int32_t *interrupt);

/**********************************************************************
 * Data Handling
 **********************************************************************/
void cv1724_GetRawData(MVME_INTERFACE *mvme, DWORD base, u_int32_t *data);

#endif /* __CV1724_H__ */
