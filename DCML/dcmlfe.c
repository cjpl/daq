/**********************************************************************\
 * $Name$ - $Id$
 * Name:       dcmlfe.c
 * Created by: Exaos Lee, 2010-08-18
 * Contents:   Frontend code for using V1724 with CAENVMElib
 * Hardware:
       1. CAEN V2718 VME Bus controller
       2. CAEN V1724 100 MS/s Digitizer
\**********************************************************************/

#include <fcntl.h>
#include <sys/ioctl.h>

#include "dcml.h"
#include "vme_wrapper.h"

/* Pure C stuff, work with MIDAS. */
#ifdef __cplusplus
extern "C" {
#endif

  /*-- Globals ----------------------------------------------------*/

  /* frontend name */
  char *frontend_name      = "FE with CAENVMElib";
  char *frontend_file_name = __FILE__;
  BOOL  frontend_call_loop = FALSE;

  INT display_period       = 3000; /* micro-seconds, = 3 s */
  INT max_event_size       = 10000;
  INT max_event_size_frag  = 0x10 * 1024 * 1024; /* EQ_FRAGMENTED */
  INT event_buffer_size    = 10 * 10000;

  /* VME crate handle */
  MVME_INTERFACE *pvme;

  HNDLE             hSet;
  extern INT        run_state;
  extern HNDLE      hDB;
  TRIGGER_SETTINGS  trig_set; /* Trigger Settings */

  /* VME hardware address: Using TRIGGER_SETTINGS */
  BOOL  isAdcEnabled = FALSE;
  BOOL  isTdcEnabled = FALSE;

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
  BANK_LIST trigger_bank_list[] = {
    /* {"TDC1", TID_DWORD, N_TDC, NULL}, */
    {""},
  };

  /*-- Equipment list ------------------------------------------------*/

  /* not use interrupt mode */
#undef USE_INT
  EQUIPMENT equipment[] = {
    { "Digitizer",          // Equipment name

      { 1, 0,             // Event ID, Trigger mask

	"SYSTEM",         // Event buffer

#ifdef USE_INT
	EQ_INTERRUPT,     // Equipment type
#else
	EQ_POLLED,
#endif

	0,                // Event source
	"MIDAS",          // format
	TRUE,             // Enabled
	RO_RUNNING 
	| RO_ODB,         // Read only when running and update ODB
	1,                // poll for 1ms
	0,                // Stop run after this event limit
	0,                // Number of sub-events
	0,                // don't log history

	"", "", "", },

      read_trigger_event, // readout routine

      NULL, NULL,

      trigger_bank_list,  // Bank list
    },

    {""}
  };

#ifdef __cplusplus
}
#endif


/*---- sequencer callback info --------------------------*/
void seq_callback(INT hDB, INT hseq, void *info)
{
  printf("odb ... trigger settings touched\n");
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

/*---- Trigger event routines ------------------------*/
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
INT read_trigger_event( char *pevent, INT off)
{

  return bk_size(pevent); /* return bank size ... */
}

/*---- Scaler event --------------------------------*/
INT read_scaler_event( char *pevent, INT off )
{
  return bk_size(pevent); /* return bank size ... */
}

