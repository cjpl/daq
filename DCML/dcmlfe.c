/**********************************************************************\
 * $Name$ - $Id$
 * Name:       dcmlfe.c
 * Created by: Exaos Lee
 * Contents:   Frontend code for using V1724 with CAENVMElib
 * Hardware:
       1. CAEN V2718 VME Bus controller
       2. CAEN V1724 100 MS/s Digitizer
 * History
   <2010-08-13> First created
\**********************************************************************/

#include <fcntl.h>
#include <sys/ioctl.h>

#include "dcml.h"
#include "cvt_V1724.h"

/*-- Globals ----------------------------------------------------*/

/* frontend name */
char *frontend_name      = "DCML frontend";
char *frontend_file_name = __FILE__;
BOOL  frontend_call_loop = FALSE;

INT display_period       = 3000;  /* micro-seconds, = 3 s */

/* MIDAS buffer settings */
INT max_event_size       = 0x10 * 1024 * 1024; /* 16 MB */
INT max_event_size_frag  = 0x10 * 1024 * 1024; /* EQ_FRAGMENTED: 16 MB */
INT event_buffer_size    = 0x10 * 1024 * 1024; /* 16 MB */

/* VME Settings */
#define VME_TYPE       cvV2718   /* Link: A2818 <--> V2718 <--> VME master bus */
#define VME_BOARD_NUM  0         /* VME board number in the chaisy chain */
INT  *m_vhdl;  /* The VME handle: int32_t */

const CVT_V17XX_TYPES  digi_type = CVT_V1724;
cvt_V1724_data  *m_p_v1724;     /* data handler of V1724: buffer, info, etc. */

/* for MIDAS ODB handle */
HNDLE           hSet;
extern INT      run_state;
extern HNDLE    hDB;
DIGITIZER_SETTINGS  digi_set; /* Digitizer settings in MIDAS ODB */

/*-- Function declarations -----------------------------------------*/
INT frontend_init();
INT frontend_exit();
INT  begin_of_run(INT rnum, char *error);
INT    end_of_run(INT rnum, char *error);
INT     pause_run(INT rnum, char *error);
INT    resume_run(INT rnum, char *error);
INT frontend_loop();

INT read_trigger_event(char *pevent, INT off);

/*---  Bank definitions --------------------------------------------*/
/* WARNING: must use "bk_init32(void*)" before "bk_create()" */
BANK_LIST digitizer_bank_list[] = {
  {"STTT", TID_DWORD, 1, NULL}, /* Trigger Time Tag (TTT) of sample */
  {"BDID", TID_BYTE,  1, NULL}, /* Board ID */
  {"ECNT", TID_DWORD, 1, NULL}, /* Event counter */

  {"CH0S", TID_WORD,  V1724_MAX_CH_SAMPLES, NULL},  /* CH0 sample */
  {"CH1S", TID_WORD,  V1724_MAX_CH_SAMPLES, NULL},  /* CH1 sample */
  {"CH2S", TID_WORD,  V1724_MAX_CH_SAMPLES, NULL},  /* CH2 sample */
  {"CH3S", TID_WORD,  V1724_MAX_CH_SAMPLES, NULL},  /* CH3 sample */
  {"CH4S", TID_WORD,  V1724_MAX_CH_SAMPLES, NULL},  /* CH4 sample */
  {"CH5S", TID_WORD,  V1724_MAX_CH_SAMPLES, NULL},  /* CH5 sample */
  {"CH6S", TID_WORD,  V1724_MAX_CH_SAMPLES, NULL},  /* CH6 sample */
  {"CH7S", TID_WORD,  V1724_MAX_CH_SAMPLES, NULL},  /* CH7 sample */

  /* NULL end */
  {""},
};

/*-- Equipment list ------------------------------------------------*/

/* not use interrupt mode */
#undef USE_INT
EQUIPMENT equipment[] = {
  { "Digitizer",        // Equipment name

    { 5, 0,             // Event ID, Trigger mask

      "SYSTEM",         // Event buffer

      EQ_POLLED,

      0,                // Event source
      "MIDAS",          // Format: MIDAS
      TRUE,             // Enabled
      RO_RUNNING
      | RO_ODB,         // Read only when running and update ODB
      1,                // poll for 1ms
      0,                // Stop run after this event limit
      0,                // Number of sub-events
      0,                // don't log history

      "", "", "", },

    read_digitizer_event, // readout routine

    NULL, NULL,

    digitizer_bank_list,  // Bank list
  },

  {""}
};

/*---- sequencer callback info --------------------------*/
void seq_callback(INT hDB, INT hseq, void *info)
{
  printf("odb ... digitizer settings touched\n");
}

/*---- Frontend Initialize ------------------------------*/
INT frontend_init()
{
  char  set_char[80];
  INT   status, size;

  CVErrorCodes cv_error;

  /* ODB Setup */
  DIGITIZER_SETTINGS_STR(digi_settings_str);
  sprintf(set_str, "/Equipment/Digitizer/Settings");
  status = db_create_record(hDB, 0, set_str, strcomb(digi_settings_str));
  status = db_find_key(hDB, 0, set_str, &hSet);
  if( status != DB_SUCCESS )
    cm_msg(MINFO, "FE", "Key %s not found!", set_str);

  /* Enable hot-link on settings of the equipment */
  size = sizeof(DIGITIZER_SETTINGS);
  if(  (status = db_open_record(hDB, hSet, &digi_set, size, MODE_READ,
				seq_callback, NULL)) != DB_SUCCESS )
    return status;

  /* Read ODB settings */
  
  /* Initialize VME interface to get the handle: m_vhdl */
  if( (cv_error = CAENVME_Init(VME_TYPE, VME_BOARD_NUM, m_vhdl)) != cvSuccess ) {
    cm_msg(MERROR, "FE", "Failed to open VME interface!");
    return FE_ERROR;
  }

  /* open board */
  if( cvt_V1724_open(m_p_v1724, digi_set.base_address, *m_vhdl, digi_type)
      != _TRUE ) {
    cm_msg(MERROR, "FE", "Failed to open V1724 board!");
    return FE_ERROR;
  }

  return SUCCESS;
}

/*---- Frontend Exit -----------------------------------*/
INT frontend_exit()
{
  CVErrorCodes cv_error;

  if( (cv_error = CAENVME_End(m_vhdl)) != cvSuccess ) {
    return FE_ERROR;
  }

  return SUCCESS;
}

/*---- Begin of Run ------------------------------------*/
INT begin_of_run( INT rnum, char *error) 
{
  BOOL  isEdgeFalling = _FALSE;
  BOOL  isExtTrig     = _FALSE;
  BOOL  isSoftTrig    = _FALSE;
  BOOL  isTrigOverlap = _TRUE;
  INT   ch_threshold, dac_offset;
  int   i=0;

  /* Read ODB settings about V1724 and apply them */

  /* Clock: PLL settings */
  /* if( (digi_set.pll_file != NULL) && */
  /*     (cvt_V1724_pll_upgrade(m_p_v1724, digi_set.pll_file) != _TRUE)) { */
  /*   cm_msg(MERROR, "FE", "Failed to load PLL settings from file %s!", digi_set.pll_file); */
  /*   return FE_ERROR; */
  /* } */

  /* Trigger type: digi_set.trig.type
       bit0: 1 -- auto
       bit1: 1 -- external
       bit2: 1 -- force
       The value can be OR-ed.
     Post trigger: digi_set.trig.post_trig = 0..2**32
     Trigger edge: digi_set.trig.edge
       = 0  --  No Trig
       = 1  --  Rising
       = 2  --  Falling
  */
  if( digi_set.trig.edge == 2 ) isEdgeFalling = _TRUE;
  if( digi_set.trig.type & 0x2 ) isExtTrig = _TRUE;
  if( digi_set.trig.type & 0x4 ) isSoftTrig = _TRUE;
  
  if( cvt_V1724_set_trigger_mode( m_p_v1724, isEdgeFalling, isExtTrig,
				  isSoftTrig, /* <BOOL soft trig en> */
				  digi_set.channel_mask & 0xFF, /* <UINT8 ch_trig_mask> */
				  isTrigOverlap,
				  digi_set.trig.post_trig ) != _TRUE ) {
    cm_msg(MERROR, "FE", "Failed to set trigger mode for V1724!");
    return FE_ERROR;
  }

  /* Settings for enabled channels: i=0..7
     a. Threshold:  digi_set.trig_threshold[i]
     b. DAC offset: digi_set.dac_offset[i]
     c. Sample threshold: digi_set.sample_thres[i]
  */
  for(i=0; i<8; i++) {
    if( !(digi_set.channel_mask & (1<<i)) ) continue; /* Channel i not opened */

    ch_threshold = (INT) (digi_set.trig_threshold[i] * V1724_THRES_SCALE);
    cvt_V1724_set_channel_trigger( m_p_v1724, 1<<i, ch_threshold,
				   digi_set.sample_thres[i] );

    dac_offset = (INT)(digi_set.dac_offset[i] * V1724_DAC_FACTOR);
    cvt_V1724_set_channel_offset( m_p_v1724, 1<<i, dac_offset );
  }

  /* Initialize V1724 and start acquisition */
  cvt_V1724_start_acquisition(m_p_v1724, digi_set.channel_mask);

  return SUCCESS;
}

/*---- End of Run --------------------------------------*/
INT end_of_run( INT rnum, char *error)
{
  /* Close V1724 */
  cvt_V1724_stop_acquisition(m_p_v1724);

  return SUCCESS;
}

/*---- Pause Run --------------------------------------*/
INT pause_run( INT rnum, char *error)
{
  /* Stop V1724  */
  cvt_V1724_stop_acquisition(m_p_v1724);

  return SUCCESS;
}

/*---- Resume Run ------------------------------------*/
INT resume_run( INT rnum, char *error)
{
  /* Start V1724 */
  cvt_V1724_start_acquisition(m_p_v1724, digi_set.channel_mask);
  return SUCCESS;
}

/*---- Frontend Loop ---------------------------------*/
INT frontend_loop() { return SUCCESS; }

/*---- Digitizer event routines ------------------------*/
INT poll_event( INT source, INT count, BOOL test)
{
  /* Check V1724 status, if data avaliable, return OK */
  BOOL isMEBnotEmpty = _FALSE;
  BOOL isMEBfull = _FALSE;
  BOOL isRunning = _TRUE;
  BOOL isSomeEventReady = _FALSE;
  BOOL isEventFull = _FALSE;
  BOOL isP_S_IN = _FALSE;

  if( cvt_V1724_get_acquisition_status(m_p_v1724, &isMEBnotEmpty, &isMEBfull,
				       &isRunning, &isSomeEventReady,
				       &isEventFull, &isP_S_IN) == _TRUE ) {
    if( isRunning && (isMEBfull || isSomeEventReady || isEventFull )
	return 1;
  }

  return 0;
}

/*---- Interrupt configuration ----------------------*/
INT interrupt_configure( INT cmd, INT source, POINTER_T adr)
{
  return SUCCESS;
}

/*---- Event Readout -------------------------------*/
INT read_digitizer_event( char *pevent, INT off)
{
  INT      ch_max_samples;
  INT      num_events;
  UINT8   *p_board_id;
  UINT16  *p_buff[8];
  UINT32  *p_trig_tim_tag;
  UINT32  *p_event_cnt;

  WORD  *pdata;

  int i=0;

  /* Prepare big banks */
  bk_init32(pevent);

  /* First: read data to cache */
  if( cvt_V1724_read_data(m_p_v1724, &ch_max_samples, &num_events) != _TRUE )
    return 0;

  /* Reformat cache data; create banks */

  return bk_size(pevent); /* return bank size ... */
}

/*---- Scaler event --------------------------------*/
INT read_scaler_event( char *pevent, INT off )
{
  return bk_size(pevent); /* return bank size ... */
}

