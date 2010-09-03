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
* \file     WDplot.c
* \brief    CAEN Front End - WaveDump plotter
* \author   Carlo Tintori (c.tintori@caen.it)
* \version  0.1
* \date     2009/12/17
*
* WDplot is a library that allows WaveDump to plot the waveforms.
* It saves the data read from the digitizer into a text file and sends the 
* plotting commands to gnuplot through a pipe. Thus, WDplot is just a simple 
* server that transfers the data and the commands from WaveDump to gnuplot, 
* which is the actual plotting tool.
******************************************************************************/

#ifndef __WDPLOT_H
#define __WDPLOT_H

#define FREE_RUN_THREAD


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CAENDigitizerType.h"

#ifdef WIN32
    #include <windows.h>
    #include <process.h>
    #define popen  _popen    /* redefine POSIX 'deprecated' popen as _popen */
    #define pclose  _pclose  /* redefine POSIX 'deprecated' pclose as _pclose */
#else
    
#endif


#define MAX_NUM_TRACES    8  /* Maximum number of traces in a plot */

typedef struct WDPlot_t
{
    char        Title[100];
    char        TraceName[MAX_NUM_TRACES][100];
    char        Xlabel[100];
    char        Ylabel[100];
    float       Xscale;
    float       Yscale;
    float       Xmax;
    float       Ymax;
    float       Xmin;
    float       Ymin;
    int         NumTraces;
    int         TraceSize[MAX_NUM_TRACES];
    int16_t     *TraceData[MAX_NUM_TRACES];
#ifdef FREE_RUN_THREAD
	volatile int Busy;
	volatile int DataReady;
	volatile int Exit;
#else
#endif
} WDPlot_t;

#ifdef WIN32
#else
	void Sleep( long sleepTimeMs);
#endif

/* Functions */
WDPlot_t *OpenPlotter(char *Path);
int PlotLoadTrace8(WDPlot_t *PlotVar, char *TraceName, uint8_t *Data, uint32_t Size);
int PlotLoadTrace16(WDPlot_t *PlotVar, char *TraceName, uint16_t *Data, uint32_t Size);
int PlotWaveforms(WDPlot_t *PlotVar);
int IsPlotterBusy(WDPlot_t *PlotVar);
int ClosePlotter(WDPlot_t *PlotVar);

#endif
