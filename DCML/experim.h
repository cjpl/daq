/********************************************************************\

  Name:         experim.h
  Created by:   ODBedit program

  Contents:     This file contains C structures for the "Experiment"
                tree in the ODB and the "/Analyzer/Parameters" tree.

                Additionally, it contains the "Settings" subtree for
                all items listed under "/Equipment" as well as their
                event definition.

                It can be used by the frontend and analyzer to work
                with these information.

                All C structures are accompanied with a string represen-
                tation which can be used in the db_create_record function
                to setup an ODB structure which matches the C structure.

  Created on:   Thu Aug 26 14:04:37 2010

\********************************************************************/

#ifndef EXCL_DIGITIZER

#define DIGITIZER_SETTINGS_DEFINED

typedef struct {
  WORD      base_address;
  BYTE      channel_mask;
  char      pll_file[256];
  struct {
    BYTE      type;
    INT       post_trig;
    BYTE      edge;
  } trig;
  WORD      buffer_size;
  float     trig_threshold[8];
  float     dac_offset[8];
  float     sample_thres[8];
} DIGITIZER_SETTINGS;

#define DIGITIZER_SETTINGS_STR(_name) const char *_name[] = {\
"[.]",\
"Base Address = WORD : 0",\
"Channel Mask = BYTE : 0",\
"PLL File = STRING : [256] ",\
"",\
"[Trig]",\
"Type = BYTE : 0",\
"Post Trig = INT : 0",\
"Edge = BYTE : 0",\
"",\
"[.]",\
"Buffer Size = WORD : 0",\
"Trig Threshold = FLOAT[8] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"[4] 0",\
"[5] 0",\
"[6] 0",\
"[7] 0",\
"DAC Offset = FLOAT[8] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"[4] 0",\
"[5] 0",\
"[6] 0",\
"[7] 0",\
"Sample Thres = FLOAT[8] :",\
"[0] 0",\
"[1] 0",\
"[2] 0",\
"[3] 0",\
"[4] 0",\
"[5] 0",\
"[6] 0",\
"[7] 0",\
"",\
NULL }

#define DIGITIZER_COMMON_DEFINED

typedef struct {
  char      format[80];
} DIGITIZER_COMMON;

#define DIGITIZER_COMMON_STR(_name) const char *_name[] = {\
"[.]",\
"Format = STRING : [80] ",\
"",\
NULL }

#endif

