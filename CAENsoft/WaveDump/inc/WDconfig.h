/******************************************************************************
* 
* CAEN SpA - Front End Division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
***************************************************************************//**
* \note TERMS OF USE:
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation. This program is distributed in the hope that it will be useful, 
* but WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the 
* software, documentation and results solely at his own risk.
*
* \file     WDconfig.c
* \brief    WaveDump - Configuration Manager
* \author   Carlo Tintori (c.tintori@caen.it)
* \version  2.0
* \date     2009/12/17
*
*  Description:
*  -----------------------------------------------------------------------------
*  This library contains the functions that allow to read the configuration 
*  file, program the Digitizer and get commands from the keyboard (bindkeys)
******************************************************************************/

#include "WaveDump.h"


/* ###########################################################################
*  Functions
*  ########################################################################### */

/*! \fn      int ParseConfigFile(FILE *f_ini, WaveDumpConfig_t *WDcfg) 
*   \brief   Read the configuration file and set the WaveDump paremeters
*            
*   \param   f_ini        Pointer to the config file
*   \param   WDcfg:   Pointer to the WaveDumpConfig data structure
*   \return  0 = Success; negative numbers are error codes
*/
int ParseConfigFile(FILE *f_ini, WaveDumpConfig_t *WDcfg);


/*! \fn      int ProgramDigitizer(int handle, WaveDumpConfig_t WDcfg) 
*   \brief   configure the digitizer according to the parameters read from
*            the cofiguration file and saved in the WDcfg data structure
*            
*   \param   handle   Digitizer handle
*   \param   WDcfg:   WaveDumpConfig data structure
*   \return  0 = Success; negative numbers are error codes
*/
int ProgramDigitizer(int handle, WaveDumpConfig_t WDcfg);


/*! \fn      void CheckKeyboardCommands(WaveDumpRun_t *WDrun) 
*   \brief   check if there is a key pressed and execute the relevant command
*            
*   \param   WDrun:   Pointer to the WaveDumpRun data structure
*/
void CheckKeyboardCommands(WaveDumpRun_t *WDrun);
