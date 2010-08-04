#ifndef _WAVE_H_
#define _WAVE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef WIN32


    #include <time.h>
    #include <sys/timeb.h>
    #include <conio.h>
    #include <process.h>

    #define popen  _popen    /* redefine POSIX 'deprecated' popen as _popen */
    #define pclose  _pclose  /* redefine POSIX 'deprecated' pclose as _pclose */
	#define getch _getch     /* redefine POSIX 'deprecated' */
	#define kbhit _kbhit     /* redefine POSIX 'deprecated' */

    #define UINT64_T INT64
    #define UINT32_T INT32

#else
    #include <unistd.h>
    #include <stdint.h>   /* C99 compliant compilers: uint64_t */
    #include <ctype.h>    /* toupper() */
    #include <sys/time.h>

    #define UINT64_T uint64_t
    #define UINT32_T uint32_t

#endif

#include "keyb.h"
#include "vme_wrapper.h"


#define OUTFILENAME "wave"  /* The actual file name is wave_n.txt, where n is the channel */

/* Executable gnuplot. NOTE: use pgnuplot instead of wgnuplot in Windows, otherwise
   the pipe will not work.
*/
#ifdef WIN32
    #define GNUPLOT_COMMAND  "pgnuplot"
#else
    #define GNUPLOT_COMMAND  "gnuplot"
#endif

#define MAX_GNUPLOT_CMD_LENGTH 200


#define FILLER   0xFFFFFFFF


/* ###########################################################################
** Digitizer Register Map (the list in uncomplete)
** ###########################################################################
*/
#define BoardInfoReg                0x8140
#define SoftTriggerReg              0x8108
#define AcquisitionControlReg       0x8100
#define PostTriggerReg              0x8114
#define ChannelEnableMaskReg        0x8120
#define TriggerSourceMaskReg        0x810C
#define ChannelFWRevision           0x108C
#define MotherBoardFWRevision       0x8124
#define EventSizeReg                0x814C
#define BlockOrganizationReg        0x800C
#define ChannelConfigReg            0x8000
#define VMEControlReg               0xEF00
#define BltEvNumReg                 0xEF1C

/* Masks definitions for header */
#define FILLER                      0xFFFFFFFF
#define GLOB_HEADER                 0xA
#define BOARD_ID(r)                 (((r)>>23) & 0x1F)
#define NUM_WORD(r)                 (((r)      & 0xFFFFFFF))
#define CH_MASK(r)                  (((r)      & 0xFF))
#define PATTERN_MASK(r)             (((r)>>8) & 0xFFFF)
#define EV_NUM(r)                   (((r)      & 0xFFFFFFFF))

#define DATA_TYPE(r)                (((r)>>28) & 0xF)

/* ###########################################################################
   Typedefs
   ###########################################################################
*/

typedef enum BOARDNAMES {V1724=0, V1721=1, V1731=2, V1720=3, V1740=4} BoardModel;

typedef struct EVENT_INFO
{
    UINT32_T  EventSize;
    UINT32_T  PartChannelMask;
    UINT32_T  EventCounter;
    UINT32_T  Pattern;
    UINT32_T  TriggerTimeTag;
    UINT32_T  ChSamples;
    UINT32_T* chbuf[8];       /* Buffer to hold a channel data */
} EVENT_INFO;


// BOARD Informations for VME access stucture.
// Its field data types are specific to the CAENVMELib
typedef struct VME_ACCESS {
	HANDLE_TYPE  handle;
	BOARD_TYPE   BType;
	ADDR_TYPE    BaseAddress;
	IRQMASK_TYPE IrqMask;
	IRQMASK_TYPE IrqCheckMask;
	LINK_TYPE    link;
	BDNUM_TYPE   bdnum;
    DATA32_TYPE  data;
	ADDR32_TYPE  addr;
    UINT32_T*    vbuf; /* VME buffer */
	RETURN_TYPE  ret;
} VME_ACCESS;

typedef struct WAVE_CONFIG {
    int  BltSize;
    int  WriteToFile;
	int  WriteOneEvent;
    int  AppendMode;
    int  OutputFormat;
	int  ReadoutMode;
	int  TriggerMode;
	int  CheckMode;
	int  UseGnuPlot;
	char GnuPlotPath[200];
	char GnuPlotExe[200];
	FILE* gnuplot;	
	FILE* ofile[8][8]; /* Files structures to hold saved channel data */

} WAVE_CONFIG;

typedef struct BOARD_CONFIG {
    BoardModel BoardType;
    UINT32_T   BufferSize;
    int        ZeroAlg;
    int        Pack25;
    double     SamplesPackCoeff;    /* Number of samples per dword (32 bit) */
    UINT32_T   ChannelEnableMask;
	UINT32_T   BlockOrg;
	int        MemorySize;
	int        EnableInt;
	int        EnableOLIrq;
	int        EnableVMEIrq;
	int        EnableBerr;
	int        EvAlign;
	int        Align64;
	int        DesMode;
	int        posttrig;
    UINT32_T   EventSize;
	UINT32_T   ExpectedEvSize;
	UINT32_T   PartChannels;
	UINT32_T   ChannelEventSize;

} BOARD_CONFIG;

/* ##########################################################################*/
/* Global variables                                                          */
/* ##########################################################################*/

   extern const char BoardTypeName[5][6];

/* ###########################################################################
// Functions
// ##########################################################################*/

void      WaveInit(int argc, char *argv[], VME_ACCESS* vme, WAVE_CONFIG* wave_config);

void      WavePrintVersion(const char *SwRelease);

int       WaveUserInput(VME_ACCESS* vme, WAVE_CONFIG* wave_config);

void      WaveReadBoardConfiguration(VME_ACCESS* vme, BOARD_CONFIG* board_config);

void      WaveRun(VME_ACCESS* vme, EVENT_INFO* event_info, WAVE_CONFIG* wave_config, BOARD_CONFIG* board_config);

void      WaveClose(VME_ACCESS* vme, WAVE_CONFIG* wave_config, EVENT_INFO* event_info);

#endif /* _WAVE__H */
