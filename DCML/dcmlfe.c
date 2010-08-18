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


#ifdef __cplusplus
}
#endif
