#ifndef _VME_WRAPPER_
#define _VME_WRAPPER_

#include "CAENVMElib.h"

// User defined target VME CPU
// User defined target VME CPU string in configuration file
//#undef __USE_CAEN_VME_BRIDGES__
#ifdef __USE_CAEN_VME_BRIDGES__

    // Type definitions
    // Defines each CAENVMELib argument type
	typedef long              HANDLE_TYPE;
	typedef HANDLE_TYPE*      PHANDLE_TYPE;
	typedef CAENVME_API       RETURN_TYPE;
	typedef CVAddressModifier AM_TYPE;
	typedef CVDataWidth       DW_TYPE;
	typedef CVBoardTypes      BOARD_TYPE;
	typedef int               LINK_TYPE;
	typedef int               BDNUM_TYPE;
	typedef unsigned long     ADDR_TYPE;
	typedef void              DATA_TYPE;
	typedef void*             PDATA_TYPE;
	typedef int               BLTSIZE_TYPE;
	typedef int*              PNB_TYPE;
	typedef unsigned long     IRQTIMEOUT_TYPE;
	typedef CAEN_BYTE         IRQMASK_TYPE;
	typedef CAEN_BYTE*        PIRQMASK_TYPE;

	typedef unsigned long     ADDR32_TYPE;
	typedef unsigned long     DATA32_TYPE;

	// Return values
	#define SUCCESS   cvSuccess
	#define BUSERROR  cvBusError

	// CAEN Bridge types
	#define USBBRIDGE          cvV1718 // CAENVMELib CVBoardTypes
	#define CONETBRIDGE        cvV2718 // CAENVMELib CVBoardTypes
	#define USBBRIDGE_STRING   "V1718"
	#define CONETBRIDGE_STRING "V2718"

	/*********************************************************************************************
	*
	*  VME Access functions macro definition
	*
	*  CAENVMElib functions are used in this case.
	*********************************************************************************************/
	//Init/End APIs
	#define VME_INIT(BTYPE, LINK, BDNUM, PHANDLE)          CAENVME_Init(BTYPE, LINK, BDNUM, PHANDLE)
	#define VME_END(HANDLE)                                CAENVME_End(HANDLE)

	// Read/Write APIs
	#define VME_READ32(HANDLE, ADDR, PDATA)                CAENVME_ReadCycle(HANDLE, ADDR, PDATA, cvA32_U_DATA, cvD32)
	#define VME_WRITE32(HANDLE, ADDR, PDATA)               CAENVME_WriteCycle(HANDLE, ADDR, PDATA, cvA32_U_DATA, cvD32)
	#define VME_FIFOBLTREAD(HANDLE, ADDR, PDATA, BLTSIZE, PNB) \
														   CAENVME_FIFOBLTReadCycle(HANDLE, ADDR, PDATA, BLTSIZE, cvA32_U_BLT, cvD32, PNB)
	#define VME_FIFOMBLTREAD(HANDLE, ADDR, PDATA, BLTSIZE, PNB) \
														   CAENVME_FIFOBLTReadCycle(HANDLE, ADDR, PDATA, BLTSIZE, cvA32_U_MBLT, cvD64, PNB)

	// IRQ APIs
	#define VME_IRQENABLE(HANDLE, IRQMASK)                 CAENVME_IRQEnable(HANDLE, IRQMASK)
	#define VME_IRQWAIT(HANDLE, IRQMASK, IRQTIMEOUT)       CAENVME_IRQWait(HANDLE, IRQMASK, IRQTIMEOUT)
	#define VME_IRQCHECK(HANDLE, PIRQMASK)                 CAENVME_IRQCheck(HANDLE, PIRQMASK)
	#define VME_IRQDISABLE(HANDLE, IRQMASK)                CAENVME_IRQDisable(HANDLE, IRQMASK)

#else

	typedef long              HANDLE_TYPE;
	typedef HANDLE_TYPE*      PHANDLE_TYPE;
	typedef CAENVME_API       RETURN_TYPE;
	typedef CVAddressModifier AM_TYPE;
	typedef CVDataWidth       DW_TYPE;
	typedef CVBoardTypes      BOARD_TYPE;
	typedef int               LINK_TYPE;
	typedef int               BDNUM_TYPE;
	typedef unsigned long     ADDR_TYPE;
	typedef void              DATA_TYPE;
	typedef void*             PDATA_TYPE;
	typedef int               BLTSIZE_TYPE;
	typedef int*              PNB_TYPE;
	typedef unsigned long     IRQTIMEOUT_TYPE;
	typedef CAEN_BYTE         IRQMASK_TYPE;
	typedef CAEN_BYTE*        PIRQMASK_TYPE;

	typedef unsigned long     ADDR32_TYPE;
	typedef unsigned long     DATA32_TYPE;

	// Return values
	#define SUCCESS   cvSuccess
	#define BUSERROR  cvBusError

    // Target VME CPU macro definition
    #define VMECPU               1       // Assign a custom ID, to be used as BOARD_TYPE argument in function calls
	#define VMECPU_STRING      "VMECPU"  // Name of the VME master in WaveDump configuration file

	// Vme Functions templates (using C typedef)
	typedef RETURN_TYPE TVmeInit      (BOARD_TYPE, LINK_TYPE,     BDNUM_TYPE,     PHANDLE_TYPE) ;
	typedef RETURN_TYPE TVmeEnd       (HANDLE_TYPE) ;
	typedef RETURN_TYPE TVmeRead      (HANDLE_TYPE, ADDR_TYPE,    PDATA_TYPE, AM_TYPE, DW_TYPE);
	typedef RETURN_TYPE TVmeWrite     (HANDLE_TYPE, ADDR_TYPE,    PDATA_TYPE, AM_TYPE, DW_TYPE);
	typedef RETURN_TYPE TVmeBltRead   (HANDLE_TYPE, ADDR_TYPE,    PDATA_TYPE,     BLTSIZE_TYPE, AM_TYPE, DW_TYPE, PNB_TYPE);
	typedef RETURN_TYPE TVmeIrqEnable (HANDLE_TYPE, IRQMASK_TYPE);
	typedef RETURN_TYPE TVmeIrqWait   (HANDLE_TYPE, IRQMASK_TYPE, IRQTIMEOUT_TYPE);
	typedef RETURN_TYPE TVmeIrqCheck  (HANDLE_TYPE, PIRQMASK_TYPE);
	typedef RETURN_TYPE TVmeIrqDisable(HANDLE_TYPE, IRQMASK_TYPE);

    TVmeInit       WrapperVME_Init;
	TVmeEnd        WrapperVME_End;
	TVmeRead       WrapperVME_ReadCycle;
	TVmeWrite      WrapperVME_WriteCycle;
	TVmeBltRead    WrapperVME_FIFOBLTReadCycle;
	TVmeBltRead    WrapperVME_FIFOBLTReadCycle;
	TVmeIrqEnable  WrapperVME_IRQEnable;
	TVmeIrqWait	   WrapperVME_IRQWait;
	TVmeIrqCheck   WrapperVME_IRQCheck;
	TVmeIrqDisable WrapperVME_IRQDisable;


	/*********************************************************************************************
	*
	*  VME Access functions macro definition
	*
	*********************************************************************************************/
	//Init/End APIs
	#define VME_INIT(BTYPE, LINK, BDNUM, PHANDLE)          WrapperVME_Init(BTYPE, LINK, BDNUM, PHANDLE)
	#define VME_END(HANDLE)                                WrapperVME_End(HANDLE)

	// Read/Write APIs
	#define VME_READ32(HANDLE, ADDR, PDATA)                WrapperVME_ReadCycle(HANDLE, ADDR, PDATA, cvA32_U_DATA, cvD32)
	#define VME_WRITE32(HANDLE, ADDR, PDATA)               WrapperVME_WriteCycle(HANDLE, ADDR, PDATA, cvA32_U_DATA, cvD32)
	#define VME_FIFOBLTREAD(HANDLE, ADDR, PDATA, BLTSIZE, PNB) \
														   WrapperVME_FIFOBLTReadCycle(HANDLE, ADDR, PDATA, BLTSIZE, cvA32_U_BLT, cvD32, PNB)
	#define VME_FIFOMBLTREAD(HANDLE, ADDR, PDATA, BLTSIZE, PNB) \
														   WrapperVME_FIFOBLTReadCycle(HANDLE, ADDR, PDATA, BLTSIZE, cvA32_U_MBLT, cvD64, PNB)

	// IRQ APIs
	#define VME_IRQENABLE(HANDLE, IRQMASK)                 WrapperVME_IRQEnable(HANDLE, IRQMASK)
	#define VME_IRQWAIT(HANDLE, IRQMASK, IRQTIMEOUT)       WrapperVME_IRQWait(HANDLE, IRQMASK, IRQTIMEOUT)
	#define VME_IRQCHECK(HANDLE, PIRQMASK)                 WrapperVME_IRQCheck(HANDLE, PIRQMASK)
	#define VME_IRQDISABLE(HANDLE, IRQMASK)                WrapperVME_IRQDisable(HANDLE, IRQMASK)

	RETURN_TYPE WrapperVME_Init(BOARD_TYPE   board, 
		                       LINK_TYPE    link, 
							   BDNUM_TYPE   num, 
							   PHANDLE_TYPE phandle) {

		/* TODO: Insert your VME board inizialization implementation here */

	}

	RETURN_TYPE WrapperVME_End(HANDLE_TYPE handle) {

		/* TODO: Insert your VME board end implementation here */

	}

	RETURN_TYPE WrapperVME_ReadCycle(HANDLE_TYPE handle, 
		                            ADDR_TYPE   addr, 
									PDATA_TYPE  pdata, 
									AM_TYPE     am, 
									DW_TYPE     dw) {

		/* TODO: Insert your VME board single register read implementation here */

	}

	RETURN_TYPE WrapperVME_WriteCycle(HANDLE_TYPE handle, 
		                             ADDR_TYPE   addr, 
									 PDATA_TYPE  pdata, 
									 AM_TYPE     am, 
									 DW_TYPE     dw) {

		/* TODO: Insert your VME board single register write implementation here */

	}

	RETURN_TYPE WrapperVME_FIFOBLTReadCycle(HANDLE_TYPE  handle, 
		                                   ADDR_TYPE    addr, 
										   PDATA_TYPE   pdata, 
		                                   BLTSIZE_TYPE bltsize,
		                                   AM_TYPE      am, 
										   DW_TYPE      dw, 
										   PNB_TYPE     pnb) {

			/* TODO: Insert your VME board block transfer read (with address autoincrement off) 
			         implementation here */

	}


	RETURN_TYPE WrapperVME_IRQEnable(HANDLE_TYPE handle, 
		                            IRQMASK_TYPE mask) {

		/* TODO: Insert your VME board interrupt enable implementation here */

	}

	RETURN_TYPE WrapperVME_IRQCheck(HANDLE_TYPE handle, 
		                           PIRQMASK_TYPE pmask) {

		/* TODO: Insert your VME board interrupt check implementation here */

	}

	RETURN_TYPE WrapperVME_IRQWait(HANDLE_TYPE     handle, 
		                          IRQMASK_TYPE    mask, 
								  IRQTIMEOUT_TYPE timeout) {

		/* TODO: Insert your VME board interrupt wait implementation here */

	}

	RETURN_TYPE WrapperVME_IRQDisable(HANDLE_TYPE  handle, 
		                             IRQMASK_TYPE mask) {

		/* TODO: Insert your VME board interrupt disable implementation here */

	}

#endif

#endif