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

/**/
INT max_event_size       = 0x10 * 1024 * 1024; /* 16 MB */
INT max_event_size_frag  = 0x10 * 1024 * 1024; /* EQ_FRAGMENTED: 16 MB */
INT event_buffer_size    = 0x10 * 1024 * 1024; /* 16 MB */

/* VME crate handle */
MVME_INTERFACE *pvme;

HNDLE             hSet;
extern INT        run_state;
extern HNDLE      hDB;
TRIGGER_SETTINGS  trig_set; /* Trigger Settings */

/* VME hardware address: Using TRIGGER_SETTINGS */

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
  {"CH1S", TID_WORD,  V1724_MAX_CH_SAMPLES, NULL},  /* CH0 sample */
  {"CH2S", TID_WORD,  V1724_MAX_CH_SAMPLES, NULL},  /* CH0 sample */
  {"CH3S", TID_WORD,  V1724_MAX_CH_SAMPLES, NULL},  /* CH0 sample */
  {"CH4S", TID_WORD,  V1724_MAX_CH_SAMPLES, NULL},  /* CH0 sample */
  {"CH5S", TID_WORD,  V1724_MAX_CH_SAMPLES, NULL},  /* CH0 sample */
  {"CH6S", TID_WORD,  V1724_MAX_CH_SAMPLES, NULL},  /* CH0 sample */
  {"CH7S", TID_WORD,  V1724_MAX_CH_SAMPLES, NULL},  /* CH0 sample */

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
      "ROOT",           // Format: MIDAS | ROOT
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

  return SUCCESS;
}

/*---- Frontend Exit -----------------------------------*/
INT frontend_exit() { return SUCCESS; }

/*---- Begin of Run ------------------------------------*/
INT begin_of_run( INT rnum, char *error) 
{

  return SUCCESS;
}

/*---- End of Run --------------------------------------*/
INT end_of_run( INT rnum, char *error)
{

  return SUCCESS;
}

/*---- Pause Run --------------------------------------*/
INT pause_run( INT rnum, char *error) {  return SUCCESS; }

/*---- Resume Run ------------------------------------*/
INT resume_run( INT rnum, char *error) { return SUCCESS; }

/*---- Frontend Loop ---------------------------------*/
INT frontend_loop() {  return SUCCESS; }

/*---- Digitizer event routines ------------------------*/
INT poll_event( INT source, INT count, BOOL test)
{

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

  return bk_size(pevent); /* return bank size ... */
}

/*---- Scaler event --------------------------------*/
INT read_scaler_event( char *pevent, INT off )
{
  return bk_size(pevent); /* return bank size ... */
}

