/******************************************************************************
* 
* CAEN SpA - Front End Division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
***************************************************************************//**
* \note TERMS OF USE:
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation. This program is distributed in the hope that it will be useful, 
* but WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the 
* software, documentation and results solely at his own risk.
*
* \file     WaveDump.c
* \brief    CAEN Front End - WaveDump (demo program for CAEN digitizers)
* \author   Carlo Tintori (c.tintori@caen.it)
* \version  2.1
* \date     2010/05/12
*
*  ------------------------------------------------------------------------
*  Modification history:
*  ------------------------------------------------------------------------
*  Version  | Author | Date       | Changes made
*  ------------------------------------------------------------------------
*  0.0      | CTIN   | 29.06.2007 | inital version.
*  1.0      | LCOL   | 20.07.2007 | Added plotting options.
*  1.1      | CTIN   | 25.07.2007 | Added extended description.
*  1.2      | LCOL   | 26.07.2007 | Added support for EventSizeReg polling.
*  1.3      | LCOL   | 01.08.2007 | Added BLT_SIZE configuration string.
*  ============================== | Implements auto memory allocation.
*  1.4      | LCOL   | 17.09.2007 | Added interrupts full support.
*  1.5      | LCOL   | 16.10.2007 | Fixed ExpectedEvSize for V1731 in DES
*  ============================== | mode.
*  1.6      | LCOL   | 29.10.2007 | Fixed WaveData buffer allocation bug.
*  1.7      | LCOL   | 05.02.2008 | Add Pattern to saved event infos.
*  1.8      | LCOL   | 15.02.2008 | Project structure changed.
*  ============================== | Added wrapper macros for portability.
*  1.9      | LCOL   | 03.06.2008 | Added V1740 support. Code cleanup and 
*  ============================== | refactoring.
*  2.0      | CTIN   | 17.12.2009 | Code completely rewritten using the 
*  ============================== | CAENDigitizer and CAENComm libraries
*  2.1      | NDA    | 12.05.2010 | Added interrupt support
*  ============================== | Added support for 8 bit data events
*  ============================== | Added support for binary file saving
*  ============================== | Added support for file saving with channel header (binary and ascii)
*  ------------------------------------------------------------------------
*
*  Description:
*  -----------------------------------------------------------------------------
*  This is a demo program that can be used with any model of the CAEN's 
*  digitizer family. The purpose of WaveDump is to configure the digitizer,
*  start the acquisition, read the data and write them into output files 
*  and/or plot the waveforms using 'gnuplot' as an external plotting tool.
*  The configuration of the digitizer (registers setting) is done by means of
*  a configuration file that contains a list of parameters.
*  This program uses the CAENDigitizer library which is then based on the 
*  CAENComm library for the access to the devices through any type of physical
*  channel (VME, Optical Link, USB, etc...). The CAENComm support the following
*  communication paths:
*  PCI => A2818 => OpticalLink => Digitizer (any type)
*  PCI => V2718 => VME => Digitizer (only VME models)
*  USB => Digitizer (only Desktop or NIM models)
*  USB => V1718 => VME => Digitizer (only VME models)
*  If you have want to use a VME digitizer with a different VME controller
*  you must provide the functions of the CAENComm library.
*
*  -----------------------------------------------------------------------------
*  Syntax: WaveDump [ConfigFile]
*  Default config file is "WaveDumpConfig.txt"
******************************************************************************/

// #define ENABLE_TEST_WAVEFORM		/* Define to enable the test waveform */

#define WaveDump_Release        "2.1"
#define WaveDump_Release_Date   "May 12, 2010"
#define DBG_TIME

#include <CAENDigitizer.h>
#include "WaveDump.h"
#include "WDplot.h"
#include "keyb.h"

/* Error messages */
typedef enum  {
	ERR_NONE= 0,
	ERR_CONF_FILE_NOT_FOUND,
	ERR_DGZ_OPEN,
	ERR_BOARD_INFO_READ,
	ERR_DGZ_PROGRAM,
	ERR_MALLOC,
	ERR_READOUT,
	ERR_EVENT_BUILD,
	ERR_UNHANDLED_BOARD,
	ERR_OUTFILE_WRITE,

	ERR_DUMMY_LAST,
} ERROR_CODES;
static char ErrMsg[ERR_DUMMY_LAST][100] = { 
	"No Error",                                         /* ERR_NONE */
	"Configuration File not found",                     /* ERR_CONF_FILE_NOT_FOUND */
	"Can't open the digitizer",                         /* ERR_DGZ_OPEN */
	"Can't read the Board Info",                        /* ERR_BOARD_INFO_READ */
	"Can't program the digitizer",                      /* ERR_DGZ_PROGRAM */
	"Can't allocate the memory for the readout buffer", /* ERR_MALLOC */
	"Readout Error",                                    /* ERR_READOUT */
	"Event Build Error",                                /* ERR_EVENT_BUILD */
	"Unhandled board type",								/* ERR_UNHANDLED_BOARD */
	"Output file write error",							/* ERR_OUTFILE_WRITE */
	
};

/* ###########################################################################
*  Functions
*  ########################################################################### */
/*! \fn      static long get_time()
*   \brief   Get time in milliseconds
*            
*   \return  time in msec
*/
static long get_time()
{
	long time_ms;
#ifdef WIN32
	struct _timeb timebuffer;
	_ftime( &timebuffer );
	time_ms = (long)timebuffer.time * 1000 + (long)timebuffer.millitm;
#else
	struct timeval t1;    
	struct timezone tz;
	gettimeofday(&t1, &tz);
	time_ms = (t1.tv_sec) * 1000 + t1.tv_usec / 1000;
#endif
	return time_ms;
}


/*! \fn      int ParseConfigFile(FILE *f_ini, WaveDumpConfig_t *WDcfg) 
*   \brief   Read the configuration file and set the WaveDump paremeters
*            
*   \param   f_ini        Pointer to the config file
*   \param   WDcfg:   Pointer to the WaveDumpConfig data structure
*   \return  0 = Success; negative numbers are error codes
*/
int ParseConfigFile(FILE *f_ini, WaveDumpConfig_t *WDcfg) 
{
	char str[400], str1[400];
	int i;

	/* Default settings */
	memset ( WDcfg, 0, sizeof( *WDcfg));

	WDcfg->RecordLength = (1024*16);
	WDcfg->PostTrigger = 80;
	WDcfg->NumEvents = 1023;
	WDcfg->ChannelEnableMask = 0xFF;
	WDcfg->GroupEnableMask= 0x01;
	WDcfg->GWn = 0;

	strcpy(WDcfg->GnuPlotPath, GNUPLOT_DEFAULT_PATH);
	for(i=0; i<MAX_CH; i++) {
		WDcfg->DCoffset[i] = 0x8000;
	}

	/* read config file and assign parameters */
	while(!feof(f_ini)) {
		int dummy;
		int read;
		read= fscanf(f_ini, "%s", str);
		if( !read|| ( read== EOF)) {
			continue;
		}
		if ((str[0] == '#')|| !strlen( str)) {
			fgets(str, 1000, f_ini);
			continue;
		}
		// OPEN: read the details of physical path to the digitizer
		if (strstr(str, "OPEN")!=NULL) {
			fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "USB")==0)
				WDcfg->LinkType = CAEN_DGTZ_USB;
			else if (strcmp(str1, "PCI")==0)
				WDcfg->LinkType = CAEN_DGTZ_PCI_OpticalLink;
			else
				return -1; 
			fscanf(f_ini, "%d", &WDcfg->LinkNum);
			fscanf(f_ini, "%d", &WDcfg->ConetNode);
			fscanf(f_ini, "%x", &WDcfg->BaseAddress);
			continue;
		}

		// Generic VME Write
		if ((strstr(str, "WRITE_REGISTER")!=NULL) && (WDcfg->GWn < MAX_GW)) {
			fscanf(f_ini, "%x", (int *)&WDcfg->GWaddr[WDcfg->GWn]);
			fscanf(f_ini, "%x", (int *)&WDcfg->GWdata[WDcfg->GWn]);
			WDcfg->GWn++;
			continue;
		}

		// DC offset
		if (strstr(str, "DC_OFFSET")!=NULL) {
			int ch;
			fscanf(f_ini, "%d", &ch);
			fscanf(f_ini, "%d", &WDcfg->DCoffset[ch]);
			continue;
		}

		// Acquisition Record Length (in number of samples)
		if (strstr(str, "RECORD_LENGTH")!=NULL) {
			fscanf(f_ini, "%d", &WDcfg->RecordLength);
			continue;
		}
		// Max. number of events for a block transfer
		if (strstr(str, "MAX_NUM_EVENTS_BLT")!=NULL) {
			fscanf(f_ini, "%d", &WDcfg->NumEvents);
			continue;
		}
		// GNUplot path
		if (strstr(str, "GNUPLOT_PATH")!=NULL) {
			fscanf(f_ini, "%s", WDcfg->GnuPlotPath);
			continue;
		}
		// Post Trigger (percent of the acquisition window)
		if (strstr(str, "POST_TRIGGER")!=NULL) {
			fscanf(f_ini, "%d", &WDcfg->PostTrigger);
			continue;
		}
		// Channel Enable Mask (or Group enable mask for the V1740)
		if (strstr(str, "CHANNEL_ENABLE")!=NULL) {
			fscanf(f_ini, "%x", &dummy);
			WDcfg->ChannelEnableMask= (char)dummy;
			WDcfg->GroupEnableMask= (char)dummy;
			continue;
		}
		// Save output file in BINARY format (otherwise the format is ASCII)
		if (strstr(str, "BINARY_OUTPUT_FILE")!=NULL) {
			WDcfg->OutFileFlags|= OFF_BINARY;
			continue;
		}
		// Save raw data into output file (otherwise only samples data will be saved)
		if (strstr(str, "SAVE_CH_HEADER")!=NULL) {
			WDcfg->OutFileFlags|= OFF_HEADER;
			continue;
		}

		// Use wait for interrupt
		/*
			syntax: USE_INTERRUPT level status_id event_number mode mask timeout
			where :
				level : the interrupt level (meaningful for VME devices only )
				status_id : the status id (meaningful for VME devices only )
				event_number : the number of events to wait for (>= 1)
				mode : 0 = RORA , 1 = ROAK
				mask : wait mask (hex format) (meaningful for VME devices only )
				timeout : wait timeout (msec)
		*/
		if (strstr(str, "USE_INTERRUPT")!=NULL) {
			WDcfg->Interrupt.Enable= 1;
			// level
			fscanf(f_ini, "%d", &WDcfg->Interrupt.Level);
			// status_id
			fscanf(f_ini, "%d", &WDcfg->Interrupt.StatusId);
			// event_number
			fscanf(f_ini, "%d", &WDcfg->Interrupt.EventNumber);
			// mode
			fscanf(f_ini, "%d", &dummy);
			WDcfg->Interrupt.Mode= dummy? CAEN_DGTZ_IRQ_MODE_ROAK: CAEN_DGTZ_IRQ_MODE_RORA;
			// mask
			fscanf(f_ini, "%x", &dummy);
			WDcfg->Interrupt.Mask= dummy;
			// Timeout
			fscanf(f_ini, "%d", &WDcfg->Interrupt.Timeout);
			continue;
		}
	}
	return 0;
}


/*! \fn      int ProgramDigitizer(int handle, WaveDumpConfig_t WDcfg) 
*   \brief   configure the digitizer according to the parameters read from
*            the cofiguration file and saved in the WDcfg data structure
*            
*   \param   handle   Digitizer handle
*   \param   WDcfg:   WaveDumpConfig data structure
*   \return  0 = Success; negative numbers are error codes
*/
int ProgramDigitizer(int handle, WaveDumpConfig_t WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo) 
{
	int i;

	/* reset the digitizer */
	CAEN_DGTZ_Reset(handle);

	
#ifdef ENABLE_TEST_WAVEFORM
	// Set the waveform test bit for debugging
	CAEN_DGTZ_WriteRegister(handle, CAEN_DGTZ_BROAD_CH_CONFIGBIT_SET_ADD, 1<<3);
#endif

	/* execute generic write commands */
	for(i=0; i<WDcfg.GWn; i++)
		CAEN_DGTZ_WriteRegister(handle, WDcfg.GWaddr[i], WDcfg.GWdata[i]);

	if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE) {
		CAEN_DGTZ_SetGroupEnableMask(handle, WDcfg.GroupEnableMask);
		for(i=0; i<8; i++)
			CAEN_DGTZ_SetGroupDCOffset(handle, i, WDcfg.DCoffset[i]);
	} else {
		CAEN_DGTZ_SetChannelEnableMask(handle, WDcfg.ChannelEnableMask);
		for(i=0; i<8; i++)
			CAEN_DGTZ_SetChannelDCOffset(handle, i, WDcfg.DCoffset[i]);
	}

	CAEN_DGTZ_SetRecordLength(handle, WDcfg.RecordLength);
	CAEN_DGTZ_SetPostTriggerSize(handle, WDcfg.PostTrigger);
	if( WDcfg.Interrupt.Enable) {
		// Interrrupt handling
		if( CAEN_DGTZ_SetInterruptConfig( handle, CAEN_DGTZ_ENABLE, 
			WDcfg.Interrupt.Level, WDcfg.Interrupt.StatusId, 
			WDcfg.Interrupt.EventNumber, WDcfg.Interrupt.Mode)!= CAEN_DGTZ_Success) {
			printf( "\n---> Error configuring interrupts\n\n");
		}
	}
	CAEN_DGTZ_SetMaxNumEventsBLT(handle, WDcfg.NumEvents); 
	CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);

	return 0;
}


/*! \fn      void CheckKeyboardCommands(WaveDumpRun_t *WDrun) 
*   \brief   check if there is a key pressed and execute the relevant command
*            
*   \param   WDrun:   Pointer to the WaveDumpRun_t data structure
*   \param   WDcfg:   Pointer to the WaveDumpConfig_t data structure
*/
void CheckKeyboardCommands(int handle, WaveDumpRun_t *WDrun, WaveDumpConfig_t *WDcfg)
{
	int c = 0;

	if(!kbhit())
		return;

	c = getch();
	if ((c < '9') && (c >= '0')) {
		int ch = c-'0';
		if( !( WDcfg->ChannelEnableMask& (1 << ch))) {
			printf("Channel %d not enabled for acquisition\n", ch);
		} else {
			WDrun->ChannelPlotMask ^= (1 << ch);
			if (WDrun->ChannelPlotMask & (1 << ch))
				printf("Channel %d enabled for plotting\n", ch);
			else
				printf("Channel %d disabled for plotting\n", ch);
		}
	} else {
		switch(c) {
			case 'g' :
				// Update the group plot index
				if( WDcfg->GroupEnableMask) {
					int orgPlotIndex= WDrun->GroupPlotIndex;
					do {
						WDrun->GroupPlotIndex= (++WDrun->GroupPlotIndex)%WDrun->Ngroup;
					} while( !(( 1<< WDrun->GroupPlotIndex)& WDcfg->GroupEnableMask));
					if( WDrun->GroupPlotIndex!= orgPlotIndex) {
						printf("Plot group setted to %d\n", WDrun->GroupPlotIndex);
					}
				}
				break;
			case 'q' :  WDrun->Quit = 1;
				break;
			case 't' :  if (!WDrun->ContinuousTrigger) {
				CAEN_DGTZ_SendSWtrigger(handle);
				printf("Single Software Trigger issued\n");
						}
						break;
			case 'T' :  WDrun->ContinuousTrigger ^= 1;
				if (WDrun->ContinuousTrigger)
					printf("Continuous trigger is enabled\n");
				else
					printf("Continuous trigger is disabled\n");
				break;
			case 'P' :  if (WDrun->ChannelPlotMask == 0)
							printf("No channel enabled for plotting\n");
						else
							WDrun->ContinuousPlot ^= 1;
				break;
			case 'p' :  if (WDrun->ChannelPlotMask == 0)
							printf("No channel enabled for plotting\n");
						else
							WDrun->SinglePlot = 1;
				break;
			case 'w' :  if (!WDrun->ContinuousWrite)
							WDrun->SingleWrite = 1;
				break;
			case 'W' :  WDrun->ContinuousWrite ^= 1;
				if (WDrun->ContinuousWrite)
					printf("Continuous writing is enabled\n");
				else
					printf("Continuous writing is disabled\n");
				break;
			case 's' :  if (WDrun->AcqRun == 0) {
				printf("Acquisition started\n");
				CAEN_DGTZ_SWStartAcquisition(handle);
				WDrun->AcqRun = 1;
						} else {
							printf("Acquisition stopped\n");
							CAEN_DGTZ_SWStopAcquisition(handle);
							WDrun->AcqRun = 0;
						}
						break;
			case 'h' :  printf("\nBindkey help\n");
				      //01234567890123456789012345678901234567890123456789012345678901234567890123456789
				printf("[q]   Quit\n");
				printf("[s]   Start/Stop acquisition\n");
				printf("[t]   Send a software trigger (single shot)\n");
				printf("[T]   Enable/Disable continuous software trigger\n");
				printf("[w]   Write one event to output file\n");
				printf("[W]   Enable/Disable continuous writing to output file\n");
				printf("[p]   Plot one event\n");
				printf("[P]   Enable/Disable continuous plot\n");
				printf("[g]   Change the index of the group to plot (XX740 family).\n");
				printf("      The group must be enabled for acquisition.\n");
				printf("[0-7] Enable/Disable one channel on the plot\n");
				printf("      The channel must be enabled for acquisition\n");
				printf("      For XX740 family this the plotted group's relative channel index\n");
				printf("[h]   This help\n\n");
				break;
			default :   break;
		}
	}
}


/*! \brief   Write the event data into the output files for 16 bit events' data
*            
*   \param   WDrun Pointer to the WaveDumpRun data structure
*   \param   WDcfg Pointer to the WaveDumpConfig data structure
*   \param   EventInfo Pointer to the EventInfo data structure
*   \param   Event Pointer to the Event to write
*/
int WriteOutputFiles16(const WaveDumpConfig_t *WDcfg, WaveDumpRun_t *WDrun, const CAEN_DGTZ_EventInfo_t *EventInfo, const CAEN_DGTZ_UINT16_EVENT_t *Event)
{
	int ch, j;
	ChannelEventHeader_t channelEventHeader;
	memset( &channelEventHeader, 0, sizeof( channelEventHeader));

	// Setup channelEventHeader fields common to each channel
	channelEventHeader.BoardId= EventInfo->BoardId;
	channelEventHeader.EventCounter= EventInfo->EventCounter;
	channelEventHeader.Pattern= EventInfo->Pattern;
	channelEventHeader.TriggerTimeTag= EventInfo->TriggerTimeTag;

	for(ch=0; ch<WDrun->Nch; ch++) {
		if (Event->ChSize[ch] <= 0) {
			continue;
		}
		// Setup channelEventHeader fields specific of each channel
		channelEventHeader.ChannelId= ch;
		channelEventHeader.ChannelEventSize= Event->ChSize[ch];

		// Check the file format type
		if( WDcfg->OutFileFlags& OFF_BINARY) {
			// Binary file format
			if (!WDrun->fout[ch]) {
				char fname[100];
				sprintf(fname, "wave%d.dat", ch);
				if ((WDrun->fout[ch] = fopen(fname, "wb")) == NULL)
					return -1;
			}
			if( WDcfg->OutFileFlags& OFF_HEADER) {
				// Write the Channel Header
				if( fwrite( &channelEventHeader, 1 , sizeof( channelEventHeader), WDrun->fout[ch])!= sizeof( channelEventHeader)) {
					// error writing to file
					fclose(WDrun->fout[ch]);
					WDrun->fout[ch]= NULL;
					return -1;
				}				
			}
			if( fwrite( Event->DataChannel[ch] , 1 , Event->ChSize[ch], WDrun->fout[ch])!= Event->ChSize[ch]) {
				// error writing to file
				fclose(WDrun->fout[ch]);
				WDrun->fout[ch]= NULL;
				return -1;
			}
		} else {
			// Ascii file format
			if (!WDrun->fout[ch]) {
				char fname[100];
				sprintf(fname, "wave%d.txt", ch);
				if ((WDrun->fout[ch] = fopen(fname, "w")) == NULL)
					return -1;
			}

			if( WDcfg->OutFileFlags& OFF_HEADER) {
				// Write the Channel Header
				fprintf( WDrun->fout[ch], "BID: % 4d  SZ: % 8d  CH: % 2d  EVT: % 8d  PTN: %08x  TTT: % 8d\n", 
					channelEventHeader.BoardId,
					channelEventHeader.ChannelEventSize,
					channelEventHeader.ChannelId,
					channelEventHeader.EventCounter,
					channelEventHeader.Pattern,
					channelEventHeader.TriggerTimeTag
					);
			}
			for(j=0; j<(int)Event->ChSize[ch]; j++)
				fprintf(WDrun->fout[ch], "%d\n", Event->DataChannel[ch][j]);
		}
		if (WDrun->SingleWrite) {
			fclose(WDrun->fout[ch]);
			WDrun->fout[ch]= NULL;
		}
	}
	return 0;

}


/*! \brief   Write the event data into the output files for 8 bit events' data
*            
*   \param   WDrun Pointer to the WaveDumpRun data structure
*   \param   WDcfg Pointer to the WaveDumpConfig data structure
*   \param   EventInfo Pointer to the EventInfo data structure
*   \param   Event Pointer to the Event to write
*/
int WriteOutputFiles8(const WaveDumpConfig_t *WDcfg, WaveDumpRun_t *WDrun, const CAEN_DGTZ_EventInfo_t *EventInfo, const CAEN_DGTZ_UINT8_EVENT_t *Event)
{
	int ch, j;
	ChannelEventHeader_t channelEventHeader;
	memset( &channelEventHeader, 0, sizeof( channelEventHeader));

	// Setup channelEventHeader fields common to each channel
	channelEventHeader.BoardId= EventInfo->BoardId;
	channelEventHeader.EventCounter= EventInfo->EventCounter;
	channelEventHeader.Pattern= EventInfo->Pattern;
	channelEventHeader.TriggerTimeTag= EventInfo->TriggerTimeTag;

	for(ch=0; ch<WDrun->Nch; ch++) {
		if (Event->ChSize[ch] <= 0) {
			continue;
		}
		// Setup channelEventHeader fields specific of each channel
		channelEventHeader.ChannelId= ch;
		channelEventHeader.ChannelEventSize= Event->ChSize[ch];

		// Check the file format type
		if( WDcfg->OutFileFlags& OFF_BINARY) {
			// Binary file format
			if (!WDrun->fout[ch]) {
				char fname[100];
				sprintf(fname, "wave%d.dat", ch);
				if ((WDrun->fout[ch] = fopen(fname, "wb")) == NULL)
					return -1;
			}
			if( WDcfg->OutFileFlags& OFF_HEADER) {
				// Write the Channel Header
				if( fwrite( &channelEventHeader, 1 , sizeof( channelEventHeader), WDrun->fout[ch])!= sizeof( channelEventHeader)) {
					// error writing to file
					fclose(WDrun->fout[ch]);
					WDrun->fout[ch]= NULL;
					return -1;
				}				
			}
			if( fwrite( Event->DataChannel[ch] , 1 , Event->ChSize[ch], WDrun->fout[ch])!= Event->ChSize[ch]) {
				// error writing to file
				fclose(WDrun->fout[ch]);
				WDrun->fout[ch]= NULL;
				return -1;
			}
		} else {
			// Ascii file format
			if (!WDrun->fout[ch]) {
				char fname[100];
				sprintf(fname, "wave%d.txt", ch);
				if ((WDrun->fout[ch] = fopen(fname, "w")) == NULL)
					return -1;
			}

			if( WDcfg->OutFileFlags& OFF_HEADER) {
				// Write the Channel Header
				fprintf( WDrun->fout[ch], "BID: % 4d  SZ: % 8d  CH: % 2d  EVT: % 8d  PTN: %08x  TTT: % 8d\n", 
					channelEventHeader.BoardId,
					channelEventHeader.ChannelEventSize,
					channelEventHeader.ChannelId,
					channelEventHeader.EventCounter,
					channelEventHeader.Pattern,
					channelEventHeader.TriggerTimeTag
					);
			}
			for(j=0; j<(int)Event->ChSize[ch]; j++)
				fprintf(WDrun->fout[ch], "%d\n", Event->DataChannel[ch][j]);
		}
		if (WDrun->SingleWrite) {
			fclose(WDrun->fout[ch]);
			WDrun->fout[ch]= NULL;
		}
	}
	return 0;
}


/* ########################################################################### */
/* MAIN                                                                        */
/* ########################################################################### */
int main(int argc, char *argv[])
{
	WaveDumpConfig_t   WDcfg;
	WaveDumpRun_t      WDrun;
	CAEN_DGTZ_ErrorCode ret=0;
	int  ret1=0, handle; 
	ERROR_CODES ErrCode= ERR_NONE;
	int  i, ch, Nb=0, Ne=0;
	UINT32 AllocatedSize, BufferSize, NumEvents;
	char *buffer = NULL;
	char *EventPtr = NULL;
	char ConfigFileName[100];
	int isVMEDevice= 0;

	uint64_t CurrentTime, PrevRateTime, ElapsedTime;
	int nCycles= 0;

	CAEN_DGTZ_BoardInfo_t       BoardInfo;
	CAEN_DGTZ_EventInfo_t       EventInfo;
	CAEN_DGTZ_UINT16_EVENT_t    *Event16=NULL; /* generic event struct with 16 bit data (10, 12, 14 and 16 bit digitizers */
	CAEN_DGTZ_UINT8_EVENT_t     *Event8=NULL;  /* generic event struct with 8 bit data (only for 8 bit digitizers) */
	WDPlot_t					*PlotVar=NULL;

	FILE *f_ini;

	memset(&WDrun, 0, sizeof(WDrun));

	printf("\n");
	printf("**************************************************************\n");
	printf("                        Wave Dump %s\n", WaveDump_Release);
	printf("**************************************************************\n"); 

	/* Open and parse configuration file */
	if (argc > 1)
		strcpy(ConfigFileName, argv[1]);
	else
		strcpy(ConfigFileName, DEFAULT_CONFIG_FILE);
	printf("Opening Configuration File %s\n", ConfigFileName);
	f_ini = fopen(ConfigFileName, "r");
	if (f_ini == NULL ) {
		ErrCode = ERR_CONF_FILE_NOT_FOUND; 
		goto QuitProgram;
	}

	ParseConfigFile(f_ini, &WDcfg);
	fclose(f_ini);

	isVMEDevice= WDcfg.BaseAddress? 1: 0;
	/* open the digitizer and read the board information */
	ret = CAEN_DGTZ_OpenDigitizer(WDcfg.LinkType, WDcfg.LinkNum, WDcfg.ConetNode, WDcfg.BaseAddress, &handle);
	if (ret) {
		ErrCode = ERR_DGZ_OPEN; 
		goto QuitProgram;
	}

	ret = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
	if (ret) {
		ErrCode = ERR_BOARD_INFO_READ; 
		goto QuitProgram;
	}

	printf("Connected to CAEN Digitizer Model %s\n", BoardInfo.ModelName);
	printf("ROC FPGA Release is %s\n", BoardInfo.ROC_FirmwareRel);
	printf("AMC FPGA Release is %s\n", BoardInfo.AMC_FirmwareRel);

	switch( BoardInfo.FamilyCode) {
		case CAEN_DGTZ_XX724_FAMILY_CODE:
		case CAEN_DGTZ_XX720_FAMILY_CODE:
		case CAEN_DGTZ_XX721_FAMILY_CODE:
		case CAEN_DGTZ_XX731_FAMILY_CODE:		// TODO Handle DES MODE for CAEN_DGTZ_XX731_FAMILY_CODE here !!!
			switch( BoardInfo.FormFactor) {
			case CAEN_DGTZ_VME64_FORM_FACTOR:
			case CAEN_DGTZ_VME64X_FORM_FACTOR:
				WDrun.Nch = 8;
				WDrun.NchPerGroup= 8;
				WDrun.Ngroup= 1;
				break;
			case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
			case CAEN_DGTZ_NIM_FORM_FACTOR:
				WDrun.Nch = 4;
				WDrun.NchPerGroup= 4;
				WDrun.Ngroup= 1;
				break;
			}
			break;
		case CAEN_DGTZ_XX740_FAMILY_CODE:
			switch( BoardInfo.FormFactor) {
			case CAEN_DGTZ_VME64_FORM_FACTOR:
			case CAEN_DGTZ_VME64X_FORM_FACTOR:
				WDrun.Nch = 64;
				WDrun.NchPerGroup= 8;
				WDrun.Ngroup= 8;
				break;
			case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
			case CAEN_DGTZ_NIM_FORM_FACTOR:
				WDrun.Nch = 32;
				WDrun.NchPerGroup= 8;
				WDrun.Ngroup= 4;
				break;
			}
			break;
		case CAEN_DGTZ_XX751_FAMILY_CODE:		// TODO Handle DES MODE for CAEN_DGTZ_XX751_FAMILY_CODE here !!!
			// TODO Handle 751 Family
		default:
			printf("ERROR : Unhandled digitizer family : exiting ....\n");
			return -1;
	}

	// Adjust the channel and group enable settings
	switch( BoardInfo.FamilyCode) {
		case CAEN_DGTZ_XX724_FAMILY_CODE:
		case CAEN_DGTZ_XX720_FAMILY_CODE:
		case CAEN_DGTZ_XX721_FAMILY_CODE:
		case CAEN_DGTZ_XX731_FAMILY_CODE:		// TODO Handle DES MODE for CAEN_DGTZ_XX731_FAMILY_CODE here !!!
		case CAEN_DGTZ_XX751_FAMILY_CODE:		// TODO Handle DES MODE for CAEN_DGTZ_XX751_FAMILY_CODE here !!!
			WDcfg.GroupEnableMask= 0x01;
			break;
		case CAEN_DGTZ_XX740_FAMILY_CODE:
			WDcfg.ChannelEnableMask= 0xff;
			break;
			// TODO Handle 751 Family
		default:
			printf("ERROR : Unhandled digitizer family : exiting ....\n");
			return -1;
	}

	/* program the digitizer */
	ret1 = ProgramDigitizer(handle, WDcfg, BoardInfo);
	if (ret1) {
		ErrCode = ERR_DGZ_PROGRAM; 
		goto QuitProgram;
	}

	ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer,&AllocatedSize); /* WARNING: This malloc must be done after the digitizer programming */
	if (ret) {
		ErrCode = ERR_MALLOC; 
		goto QuitProgram;
	}

	printf("[s] start/stop the acquisition, [q] quit [h] help\n");
	PrevRateTime = get_time();

	/* Readout Loop */    
	while(!WDrun.Quit) {
	    Sleep(1);

		// Check for keyboard commands (key pressed)
		CheckKeyboardCommands(handle, &WDrun, &WDcfg);
		if (WDrun.AcqRun == 0)
			continue;

		/* Send a software trigger */
		if (WDrun.ContinuousTrigger) {
			CAEN_DGTZ_SendSWtrigger(handle);
		}

		/*if (WDcfg.UseInterrupt) {
		Wait for interrupts here. HACK: to be done...

		Chiama la wait o VMEWait a seconda se BaseAddress== 0 o !=0 (VME)
		Se VME , il VMEHandler viene tornato dalla wait e deve essere passato alla VMEIrqCheck per il test del chiamante
		}*/
		if( WDcfg.Interrupt.Enable) {
			// Interrrupt handling
			if( isVMEDevice) {
				int32_t boardId;
				int VMEHandle;
				if( CAEN_DGTZ_VMEIRQWait( WDcfg.LinkType, WDcfg.LinkNum, WDcfg.ConetNode, 
					WDcfg.Interrupt.Mask, WDcfg.Interrupt.Timeout, &VMEHandle)!= CAEN_DGTZ_Success) {
					printf( "\n---> Interrupt wait timeout\n\n");
					continue;
				} 
				if( WDcfg.Interrupt.Mode== CAEN_DGTZ_IRQ_MODE_ROAK) {
					// IrqAck 
					if( CAEN_DGTZ_VMEIACKCycle(VMEHandle, WDcfg.Interrupt.Level, &boardId)!= CAEN_DGTZ_Success) {
						printf( "\n---> Interrupt ack error\n\n");
					}
				}
			} else {
				int32_t boardId;
				if( CAEN_DGTZ_IRQWait( handle, WDcfg.Interrupt.Timeout)!= CAEN_DGTZ_Success) {
					printf( "\n---> Interrupt wait timeout\n\n");
					continue;
				} 
				if( WDcfg.Interrupt.Mode== CAEN_DGTZ_IRQ_MODE_ROAK) {
					// IrqAck
					if( CAEN_DGTZ_IACKCycle( handle, &boardId)!= CAEN_DGTZ_Success) {
						printf( "\n---> Interrupt ack error\n\n");
					}
				}
			}
		}

		/* Read data from the board */
		ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &BufferSize);
		if (ret) {
			ErrCode = ERR_READOUT; 
			goto QuitProgram;
		}
		ret = CAEN_DGTZ_GetNumEvents(handle, buffer, BufferSize, &NumEvents);
		if (ret) {
			ErrCode = ERR_READOUT; 
			goto QuitProgram;
		}

		/* Calculate throughput and trigger rate (every second) */
		Nb += BufferSize;
		Ne += NumEvents;
		CurrentTime = get_time(); 
		ElapsedTime = CurrentTime - PrevRateTime;

		nCycles++;
		if (ElapsedTime > 1000) {
			if (Nb == 0)
				printf("No data...\n");
			else
				printf("Reading at %.2f MB/s (Trg Rate: %.2f Hz)\n", (float)Nb/((float)ElapsedTime*1048.576f), (float)Ne*1000.0f/(float)ElapsedTime);
#ifdef DBG_TIME
			printf( "Mean cycle time: %.2f ms\n", (float)ElapsedTime/ (float)nCycles);
#endif
			nCycles= 0;
			Nb = 0;
			Ne = 0;
			PrevRateTime = CurrentTime;
		}

		/* Analyze data */
		for(i = 0; i < (int)NumEvents; i++) {
		    Sleep(1);

			/* Get one event from the readout buffer */
			ret = CAEN_DGTZ_GetEventInfo(handle, buffer, BufferSize, i, &EventInfo, &EventPtr);
			if (ret) {
				ErrCode = ERR_EVENT_BUILD;
				goto QuitProgram;
			}
			switch( BoardInfo.FamilyCode) {
				case CAEN_DGTZ_XX721_FAMILY_CODE:
				case CAEN_DGTZ_XX731_FAMILY_CODE:
					if( CAEN_DGTZ_DecodeEvent(handle, EventPtr, (void**)&Event8)){
						ErrCode = ERR_EVENT_BUILD;
						goto QuitProgram;
					}
					break;
				case CAEN_DGTZ_XX720_FAMILY_CODE:
				case CAEN_DGTZ_XX724_FAMILY_CODE:
				case CAEN_DGTZ_XX740_FAMILY_CODE:
					if( CAEN_DGTZ_DecodeEvent(handle, EventPtr, (void**)&Event16)){
						ErrCode = ERR_EVENT_BUILD;
						goto QuitProgram;
					}
					break;
				case CAEN_DGTZ_XX751_FAMILY_CODE:
					// TODO Unhandled
				default:
					ErrCode = ERR_UNHANDLED_BOARD; 
					goto QuitProgram;
			}

			/* Write Event data to file */
			if (WDrun.ContinuousWrite || WDrun.SingleWrite) {
				// HACK: use a thread here to allow parallel readout and file writing
				if( Event8) {
					if( WriteOutputFiles8(&WDcfg, &WDrun, &EventInfo, Event8)){
						ErrCode = ERR_OUTFILE_WRITE; 
						goto QuitProgram;
					}
				}
				if( Event16) {
					if( WriteOutputFiles16(&WDcfg, &WDrun, &EventInfo, Event16)){
						ErrCode = ERR_OUTFILE_WRITE; 
						goto QuitProgram;
					}
				}
				if (WDrun.SingleWrite) {
					printf("Single Event saved to output files\n");
					WDrun.SingleWrite = 0;
				}
			}

			/* Plot Waveforms */
			if ((WDrun.ContinuousPlot || WDrun.SinglePlot) && !IsPlotterBusy(PlotVar)) {
				if (PlotVar == NULL) {
					PlotVar = OpenPlotter(WDcfg.GnuPlotPath);
				}				
				if (PlotVar == NULL) {
					printf("Can't open the plotter\n");
					WDrun.ContinuousPlot = 0;
					WDrun.SinglePlot = 0;
				} else {
					for(ch=0; ch< WDrun.NchPerGroup; ch++) {
						char TraceName[100];
						int absCh= WDrun.GroupPlotIndex* WDrun.NchPerGroup+ ch;
						if (!((WDrun.ChannelPlotMask >> ch) & 1)) {
							continue;
						}
						sprintf(TraceName, "CH %d", absCh);
						if( Event8) {
							PlotLoadTrace8(PlotVar, TraceName, Event8->DataChannel[absCh], Event8->ChSize[absCh]);
						}
						if( Event16) {
							PlotLoadTrace16(PlotVar, TraceName, Event16->DataChannel[absCh], Event16->ChSize[absCh]);
						}
					}
					if( PlotWaveforms(PlotVar)< 0) {
						WDrun.ContinuousPlot = 0;
						printf("Can't start the plotter thread\n");
					}
					WDrun.SinglePlot = 0;
				}
			}

			if( Event8) {
				CAEN_DGTZ_FreeEvent(handle, (void**)&Event8);
			}
			if( Event16) {
				CAEN_DGTZ_FreeEvent(handle, (void**)&Event16);
			}
		}
	}
	ErrCode = ERR_NONE;

QuitProgram:
	if (ErrCode) {
		printf("\a%s\n", ErrMsg[ErrCode]);
		/*if (ret)
		printf("CAENDigitizer Lib Error: %s\n", CAEN_DGTZ_DecodeError(ret));*/
	}

	/* stop the acquisition */
	CAEN_DGTZ_SWStopAcquisition(handle);

	/* close the plotter */
	if (PlotVar != NULL)
		ClosePlotter(PlotVar);

	/* close the output files */
	for(ch=0; ch<WDrun.Nch; ch++) {
		if( WDrun.fout[ch]) {
			fflush(WDrun.fout[ch]);
			fclose(WDrun.fout[ch]);
			WDrun.fout[ch]= NULL;
		}
	}
	/* close the device and free the buffers */
	CAEN_DGTZ_FreeReadoutBuffer(&buffer);
	CAEN_DGTZ_CloseDigitizer(handle);

	printf( "\n Hit Enter to exit ...");
	getchar();
	return 0;
}
