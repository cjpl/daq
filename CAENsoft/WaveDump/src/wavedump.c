/******************************************************************************

  CAEN SpA - Viareggio
  www.caen.it

  Program: Wave Dump
  Date:    29/06/2007
  Author: Carlo Tintori (c.tintori@caen.it)

  ------------------------------------------------------------------------
  Modification history:
  ------------------------------------------------------------------------
  Version  | Author | Date       | Changes made
  ------------------------------------------------------------------------
  0.0      | CTIN   | 29.06.2007 | inital version.
  1.0      | LCOL   | 20.07.2007 | Added plotting options.
  1.1      | CTIN   | 25.07.2007 | Added extended description.
  1.2      | LCOL   | 26.07.2007 | Added support for EventSizeReg polling.
  1.3      | LCOL   | 01.08.2007 | Added BLT_SIZE configuration string.
  ============================== | Implements auto memory allocation.
  1.4      | LCOL   | 17.09.2007 | Added interrupts full support.
  1.5      | LCOL   | 16.10.2007 | Fixed ExpectedEvSize for V1731 in DES
  ============================== | mode.
  1.6      | LCOL   | 29.10.2007 | Fixed WaveData buffer allocation bug.
  1.7      | LCOL   | 05.02.2008 | Add Pattern to saved event infos.
  1.8      | LCOL   | 15.02.2008 | Project structure changed.
  ============================== | Added wrapper macros for portability.
  1.9      | LCOL   | 03.06.2008 | Added V1740 support. Code cleanup and 
  ============================== | refactoring.
  ------------------------------------------------------------------------

  Description:
  -----------------------------------------------------------------------------
  This program reads any model of the CAEN's digitizer family (V1724, V1721,
  V1720, V1740) and save the data into a memory buffer allocated for this purpose.
  Optionally, the data can be written into a file. It is also possible to
  enable some data integrity check during the readout. Giving the proper
  path for the plotting program, it is possible to plot the wave files
  during the readout (for example, GNUplot is a free program that can be
  downloaded from the WEB for Windows and Linux)
  The configuration of the digitizer (registers setting) is done by means of
  a configuration file that contains a list of generic write accesses to the
  registers of the board.
  The configuartion file contains also some other parameters (declared with
  specific keywords) that define the operating mode of the WaveDump program.
  This program uses the CAENVMELib library for the VME access functions and
  it can work with the V1718 and V2718 bridges. If you have a different VME
  controller, you must change the CAENVME functions for the VME access or
  provide a wrapper library. The following CAENVME functions are used by this
  program:
     .) CAENVME_Init()
     .) CAENVME_ReadCycle()
     .) CAENVME_WriteCycle()
     .) CAENVME_FIFOBLTReadCycle()
     .) CAENVME_FIFOMBLTReadCycle()
     .) CAENVME_IRQEnable()
     .) CAENVME_IRQWait()
	 .) CAENVME_IRQCheck()
     .) CAENVME_IRQDisable()
     .) CAENVME_End()

  There is a runtime menu that is enabled while the readout is running:
  q  => quit the program
  t  => send a software trigger (one shot)
  T  => enable/disable the continuous generation of the software triggers
  w  => save one event into the files
  p  => plot the waveform in the wave file (a path to the plotting program
        must be provided in the configuration file)

  -----------------------------------------------------------------------------
  Syntax: WaveDump [ConfigFile]
  Default config file is "WaveDumpConfig.txt"

  -----------------------------------------------------------------------------
  Keyword list and syntax for the configuration file:

  LINK  type  bdnum  linknum
    Description: define the VME controller
    Parameters:  type = V1728 or V2718

  DATA_CHECK  X
    Description: enable data checks. If X=0, no check is done.
    Parameters:  X is an exadecimal number; the bits of X enable specific
                 checks:
                 Bit 0 = Calculate and Print Throughput Rate
                 Bit 1 = Check Header Consistency
                 Bit 2 = Check Size
                 Bit 3 = Check Consecutive Event Counters

  BASE_ADDRESS  A
    Description: define the base address of the board (rotary switches)
    Parameters:  A is a 32bit hex number. A[31:16] = Rotary-Switches

  WRITE_TO_FILE n
    Description: allow to write the readout data into the wave files
    Parameters:  n=0 => don't write any file
                 n=1 => write a wave file only when "w" is pressed during the
                        readout
                 n=2 => write the wave file continuously

  APPEND_MODE n
    Description: define whether the wave files are written in append mode
    Parameters:  n=0 => overwrite the waveform in the wave file
                 n=1 => append the waveform to the previous one in the wave
                        file

  OUTPUT_FORMAT n
    Description: define the wave file format
    Parameters:  n=0 => write only data (list of decimal ascii numbers)
                 n=1 => write size + data
                 n=2 => write size + header + data

  BLT_SIZE n
    Description: define the size of each Block Transfer during the readout
    Parameters:  n is the BLT size in bytes

  READOUT_MODE n
    Description: define the readout mode on the VME bus
    Parameters:  n=0 => Single D32 read
                 n=1 => 32 bit BLT
                 n=2 => 64 bit MBLT
                 n=3 => 2eVME (not yet implemented)
                 n=4 => 2eSST (not yet implemented)

  GNUPLOT_PATH string
    Description: define the gnuplot command path in the host system.
    Parameters : string with path string.
                 For instance, 
				    "c:\programms\gnuplot\bin\"

  WRITE_REGISTER A D
    Description: write a register of the digitizer.
    Parameters:  A is the register offset (16 bit hex number). The actual VME
                 address will be BASE_ADDRESS+A
                 D is the data to write (32 bit hex number)



******************************************************************************/
#ifndef _CRT_SECURE_NO_DEPRECATE
  #define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "keyb.h"
#include "vme_wrapper.h"
#include "wave.h"


const char BoardTypeName[5][6] = {"V1724", "V1721", "V1731", "V1720", "V1740"};
const char SwRelease[4] = "1.9"; 

// ###########################################################################
// MAIN
// ###########################################################################
int main(int argc, char *argv[])
{

  EVENT_INFO   event_info;
	WAVE_CONFIG  wave_config = {256,0,0,0,0,0,0,0, 1, "", "", NULL}; 
	BOARD_CONFIG board_config;
	VME_ACCESS   vme;


  /* Print program header and current version */
	WavePrintVersion(SwRelease);

  /* Initialize board and acquisition */
	WaveInit(argc, argv, &vme, &wave_config);

  /* Read Board configuration after initialisation */
	WaveReadBoardConfiguration(&vme, &board_config);

	/* Start Acquisition and stores events in memory */
	/* If dump to file is enabled, a copy of the     */
  /* acquired waveform is saved to file            */
  WaveRun(&vme, &event_info, &wave_config, &board_config);

	/* Close acquisition by deallocating open resources */
	WaveClose(&vme, &wave_config, &event_info);

  return 0;
}	



