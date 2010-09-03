#ifndef _WAVEDUMP_H_
#define _WAVEDUMP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

#ifdef WIN32

    #include <time.h>
    #include <sys/timeb.h>
    #include <conio.h>
    #include <process.h>

	#define getch _getch     /* redefine POSIX 'deprecated' */
	#define kbhit _kbhit     /* redefine POSIX 'deprecated' */

	#define		_PACKED_
	#define		_INLINE_		

#else
    #include <unistd.h>
    #include <stdint.h>   /* C99 compliant compilers: uint64_t */
    #include <ctype.h>    /* toupper() */
    #include <sys/time.h>

	#define		_PACKED_		__attribute__ ((packed, aligned(1)))
	#define		_INLINE_		__inline__ 

#endif

#ifdef LINUX
#define DEFAULT_CONFIG_FILE  "/etc/wavedump/WaveDumpConfig.txt"
#define GNUPLOT_DEFAULT_PATH ""
#else
#define DEFAULT_CONFIG_FILE  "WaveDumpConfig.txt"  /* local directory */
#define GNUPLOT_DEFAULT_PATH ""
#endif

#define OUTFILENAME "wave"  /* The actual file name is wave_n.txt, where n is the channel */
#define MAX_CH  64          /* max. number of channels */
#define MAX_GW  1000        /* max. number of generic write commads */

#define PLOT_REFRESH_TIME 1000


/* ###########################################################################
   Typedefs
   ###########################################################################
*/

typedef enum {
	OFF_BINARY=	0x00000001,			// If setted the output file format is BINARY, otherwise it is ASCII
	OFF_HEADER= 0x00000002,			// If setted the output file contains a channel data header read , otherwise just samples data
} OUTFILE_FLAGS;

typedef struct {
	int Enable;					// Enable				
	int Level;					// Level
	int StatusId;				// Status id		
	int EventNumber;			// event_number
	CAEN_DGTZ_IRQMode_t Mode;	// Interrupt mode
	uint8_t	Mask;				// VME Irq wait mask
	int Timeout;				// wait timeout (msec)
} IntConfig_t;

typedef struct WaveDumpConfig_t {
    int LinkType;
    int LinkNum;
    int ConetNode;
    uint32_t BaseAddress;
    int NumEvents;
    int RecordLength;
    int PostTrigger;
    uint8_t ChannelEnableMask;
    uint8_t GroupEnableMask;
    char GnuPlotPath[200];
    uint32_t DCoffset[MAX_CH];

    int GWn;
    uint32_t GWaddr[MAX_GW];
    uint32_t GWdata[MAX_GW];
	OUTFILE_FLAGS OutFileFlags;

	IntConfig_t Interrupt;			// Interrupt configuration
} WaveDumpConfig_t;


typedef struct WaveDumpRun_t {
    int Quit;
    int AcqRun;
    int Nch;
    int NchPerGroup;
    int Ngroup;
    int ContinuousTrigger;
    int ContinuousWrite;
    int SingleWrite;
    int ContinuousPlot;
    int SinglePlot;
    int GroupPlotIndex;
    char ChannelPlotMask;
    FILE *fout[MAX_CH];
} WaveDumpRun_t;

#if defined(WIN32)
	#pragma pack(push)
	#pragma pack(1)
#endif

typedef struct ChannelEventHeader_t {
	uint32_t ChannelEventSize;
	uint32_t BoardId;
	uint32_t Pattern;
	uint32_t ChannelId;
	uint32_t EventCounter;
	uint32_t TriggerTimeTag;
} _PACKED_ ChannelEventHeader_t;

#if defined(WIN32)
	#pragma pack(push)
#endif

#endif /* _WAVEDUMP__H */
