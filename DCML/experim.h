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

  Created on:   Thu Sep  2 17:40:54 2010

\********************************************************************/

#ifndef EXCL_DIGITIZER

#define DIGITIZER_COMMON_DEFINED

typedef struct {
  WORD      event_id;
  WORD      trigger_mask;
  char      buffer[32];
  INT       type;
  INT       source;
  char      format[8];
  BOOL      enabled;
  INT       read_on;
  INT       period;
  double    event_limit;
  DWORD     num_subevents;
  INT       log_history;
  char      frontend_host[32];
  char      frontend_name[32];
  char      frontend_file_name[256];
  char      status[256];
  char      status_color[32];
} DIGITIZER_COMMON;

#define DIGITIZER_COMMON_STR(_name) const char *_name[] = {\
"[.]",\
"Event ID = WORD : 5",\
"Trigger mask = WORD : 0",\
"Buffer = STRING : [32] SYSTEM",\
"Type = INT : 2",\
"Source = INT : 0",\
"Format = STRING : [8] MIDAS",\
"Enabled = BOOL : y",\
"Read on = INT : 257",\
"Period = INT : 1",\
"Event limit = DOUBLE : 0",\
"Num subevents = DWORD : 0",\
"Log history = INT : 0",\
"Frontend host = STRING : [32] cjpl-daq-1",\
"Frontend name = STRING : [32] DCML frontend",\
"Frontend file name = STRING : [256] dcmlfe.c",\
"Status = STRING : [256] DCML frontend@cjpl-daq-1",\
"Status color = STRING : [32] #00FF00",\
"",\
NULL }

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
  INT       sample_thres[8];
} DIGITIZER_SETTINGS;

#define DIGITIZER_SETTINGS_STR(_name) const char *_name[] = {\
"[.]",\
"Base Address = WORD : 5930",\
"Channel Mask = BYTE : 255",\
"PLL File = STRING : [256] ",\
"",\
"[Trig]",\
"Type = BYTE : 3",\
"Post Trig = INT : 0",\
"Edge = BYTE : 1",\
"",\
"[.]",\
"Buffer Size = WORD : 512",\
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
"Sample Thres = INT[8] :",\
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

#endif

