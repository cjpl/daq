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

  Created on:   Wed Aug 25 21:20:50 2010

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
  struct {
    float     threshold;
    float     dac_offset;
  } ch0;
  struct {
    float     threshold;
    float     dac_offset;
  } ch1;
  struct {
    float     threshold;
    float     dac_offset;
  } ch2;
  struct {
    float     threshold;
    float     dac_offset;
  } ch3;
  struct {
    float     threshold;
    float     dac_offset;
  } ch4;
  struct {
    float     threshold;
    float     dac_offset;
  } ch5;
  struct {
    float     threshold;
    float     dac_offset;
  } ch6;
  struct {
    float     threshold;
    float     dac_offset;
  } ch7;
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
"",\
"[CH0]",\
"Threshold = FLOAT : 0",\
"DAC Offset = FLOAT : 0",\
"",\
"[CH1]",\
"Threshold = FLOAT : 0",\
"DAC Offset = FLOAT : 0",\
"",\
"[CH2]",\
"Threshold = FLOAT : 0",\
"DAC Offset = FLOAT : 0",\
"",\
"[CH3]",\
"Threshold = FLOAT : 0",\
"DAC Offset = FLOAT : 0",\
"",\
"[CH4]",\
"Threshold = FLOAT : 0",\
"DAC Offset = FLOAT : 0",\
"",\
"[CH5]",\
"Threshold = FLOAT : 0",\
"DAC Offset = FLOAT : 0",\
"",\
"[CH6]",\
"Threshold = FLOAT : 0",\
"DAC Offset = FLOAT : 0",\
"",\
"[CH7]",\
"Threshold = FLOAT : 0",\
"DAC Offset = FLOAT : 0",\
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

