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
* \date     2010/05/12
* \todo		Porting su Linux
*
*  ------------------------------------------------------------------------
*  Modification history:
*  ------------------------------------------------------------------------
*  Version  | Author | Date       | Changes made
*  ------------------------------------------------------------------------
*  0.0      | CTIN   | 17.12.2007 | inital version.
*  0.1      | NDA    | 12.05.2010 | Added separate thread for GnuPlot data pipe handling
*  ============================== | Changed data synchronization with caller
*
* WDplot is a library that allows WaveDump to plot the waveforms.
* It saves the data read from the digitizer into a text file and sends the 
* plotting commands to gnuplot through a pipe. Thus, WDplot is just a simple 
* server that transfers the data and the commands from WaveDump to gnuplot, 
* which is the actual plotting tool.
******************************************************************************/
#ifdef WIN32
#else
	#include <unistd.h>
	#include <pthread.h>
#endif

#include "WDplot.h"


/* Defines */
#define PLOT_DATA_FILE   "PlotData.txt"

#ifdef WIN32
	/******************************************************************************
	* Executable file for 'gnuplot'
	* NOTE: use pgnuplot instead of wgnuplot under Windows, otherwise the pipe 
	*       will not work
	******************************************************************************/
    #define GNUPLOT_COMMAND  "pgnuplot"
#else
    #define GNUPLOT_COMMAND  "gnuplot"
#endif

/* Global Variables */
FILE *gnuplot = NULL;
#ifdef WIN32
	HANDLE ThreadHandler = 0;
#else
	pthread_t ThreadHandler;
#endif

#ifdef WIN32
#else
	void Sleep( long sleepTimeMs) {
		usleep( sleepTimeMs* 1000);
	}
#endif

#ifdef FREE_RUN_THREAD
int SetPlotOptions(volatile WDPlot_t *Var)
#else
int SetPlotOptions(volatile WDPlot_t *Var)
#endif
{
    fprintf(gnuplot, "set xlabel '%s'\n", Var->Xlabel);
    fprintf(gnuplot, "set ylabel '%s'\n", Var->Ylabel);
    fprintf(gnuplot, "set title '%s'\n", Var->Title);
    fprintf(gnuplot, "Xs = %f\n", Var->Xscale);
    fprintf(gnuplot, "Ys = %f\n", Var->Yscale);
    fprintf(gnuplot, "Xmax = %f\n", Var->Xmax);
    fprintf(gnuplot, "Ymax = %f\n", Var->Ymax);
    fprintf(gnuplot, "Xmin = %f\n", Var->Xmin);
    fprintf(gnuplot, "Ymin = %f\n", Var->Ymin);
    //fprintf(gnuplot, "load 'PlotSettings.cfg'\n");
	fflush(gnuplot);
    return 0;
}


#ifdef FREE_RUN_THREAD
#ifdef WIN32
unsigned int __stdcall PlotThread(void* data)
#else
void *PlotThread(void* data)
#endif
{
	volatile WDPlot_t *plotData= (volatile WDPlot_t *)data;
	while( !plotData->Exit) {
		int i, s=0, ntr, comma=0, c, npts=0, WaitTime;
		FILE *fplot;
		while( !plotData->DataReady) {
			if( plotData->Exit) {
				goto exitPoint;
			}
			Sleep( 10);
		}
		plotData->DataReady= 0;

		SetPlotOptions(plotData);
		fplot = fopen(PLOT_DATA_FILE, "w");
		if (fplot == NULL) {
			goto exitPoint;
		}
		ntr = plotData->NumTraces;
		while(ntr > 0) {
			fprintf(fplot, "%d\t", s);
			for(i=0; i<plotData->NumTraces; i++) {
				if (s < plotData->TraceSize[i]) {
					fprintf(fplot, "%d\t", plotData->TraceData[i][s]);
					npts++;
				}
				if (plotData->TraceSize[i] == (s-1))
					ntr--;
			}
			s++;
			fprintf(fplot, "\n");
		}
		fclose(fplot);

		/* str contains the plot command for gnuplot */
		fprintf(gnuplot, "plot ");
		c = 2; /* first column of data */
		for(i=0; i<plotData->NumTraces; i++) {
			if (comma)
				fprintf(gnuplot, ", ");
			fprintf(gnuplot, "'%s' using ($1*%f):($%d*%f) title '%s' with lines %d ", PLOT_DATA_FILE, plotData->Xscale, c++, plotData->Yscale, plotData->TraceName[i], i+1);
			comma = 1;
		}
		fprintf(gnuplot, "\n"); 
		fflush(gnuplot);

		/* wait for gnuplot to finish */
		WaitTime = npts/100;
		if (WaitTime < 100)
			WaitTime = 100;
		Sleep(WaitTime);

	exitPoint:
		// Free the traces storage
		for(i=0; i<plotData->NumTraces; i++)
			free( plotData->TraceData[i]);
		plotData->NumTraces= 0;
		plotData->Busy= 0;
	}
#ifdef WIN32
	_endthreadex( 0 );
    return 0;
#else
	return (void*)0;
#endif
}
#else
unsigned int __stdcall PlotThread(void* data)
{
	uint32_t retVal= 0;
	WDPlot_t *plotData= (WDPlot_t *)data;
	int i, s=0, ntr, comma=0, c, npts=0, WaitTime;
	FILE *fplot;

	SetPlotOptions(plotData);
	fplot = fopen(PLOT_DATA_FILE, "w");
	if (fplot == NULL) {
		retVal= -1;
		goto exitPoint;
	}
	ntr = plotData->NumTraces;
	while(ntr > 0) {
		fprintf(fplot, "%d\t", s);
		for(i=0; i<plotData->NumTraces; i++) {
			if (s < plotData->TraceSize[i]) {
				fprintf(fplot, "%d\t", plotData->TraceData[i][s]);
				npts++;
			}
			if (plotData->TraceSize[i] == (s-1))
				ntr--;
		}
		s++;
		fprintf(fplot, "\n");
	}
	fclose(fplot);

	/* str contains the plot command for gnuplot */
	fprintf(gnuplot, "plot ");
	c = 2; /* first column of data */
	for(i=0; i<plotData->NumTraces; i++) {
		if (comma)
			fprintf(gnuplot, ", ");
		fprintf(gnuplot, "'%s' using ($1*%f):($%d*%f) title 'ch%d' with lines %d", PLOT_DATA_FILE, plotData->Xscale, c++, plotData->Yscale, i, i+1);
		comma = 1;
	}
	fprintf(gnuplot, "\n"); 
	fflush(gnuplot);

	/* wait for gnuplot to finish */
	// TODO verificare documentazione gnuPlot per vedere se c'è modo di capire quando gnuPlot ha finito
	WaitTime = npts/100;
	if (WaitTime < 100)
		WaitTime = 100;
	Sleep(WaitTime);

exitPoint:
	// Free the traces storage
	for(i=0; i<plotData->NumTraces; i++)
		free( plotData->TraceData[i]);
	plotData->NumTraces= 0;
	_endthreadex( retVal );
    return retVal;
}
#endif

/**************************************************************************//**
* \fn      int OpenPlotter(void)
* \brief   Open the plotter (i.e. open gnuplot with a pipe)
* \return  0: Success; -1: Error
******************************************************************************/
WDPlot_t *OpenPlotter(char *Path)
{
    char str[1000];
    int i;
    WDPlot_t *Var;

    strcpy(str, Path);
    strcat(str, GNUPLOT_COMMAND);
    if ((gnuplot = popen(str, "w")) == NULL) // open the pipe
        return NULL;
    Var = malloc(sizeof(WDPlot_t));
    if (Var == NULL)
        return NULL;

    /* send some plot settings */
    fprintf(gnuplot, "system 'echo Start > plot.log'\n");
    fprintf(gnuplot, "set grid\n");
    fprintf(gnuplot, "set mouse\n");
    fprintf(gnuplot, "bind y 'set yrange [Ymin:Ymax]'\n");
    fprintf(gnuplot, "bind x 'set xrange [Xmin:Xmax]'\n");
    fflush(gnuplot);

    /* set default parameters */
    strcpy(Var->Title, "");
    strcpy(Var->Xlabel, "");
    strcpy(Var->Ylabel, "");
    for(i=0; i<MAX_NUM_TRACES; i++)
        strcpy(Var->TraceName[i], "");
    Var->Xscale = 1.0;
    Var->Yscale = 1.0;
    Var->Xmax = 16384;
    Var->Ymax = 16384;
    Var->Xmin = 0;
    Var->Ymin = 0;
    Var->NumTraces = 0;
#ifdef FREE_RUN_THREAD
	Var->Busy= 0;
	Var->DataReady= 0;
	Var->Exit= 0;
#else
#endif
    return Var;
}

/**************************************************************************//**
* \brief   Plot the waveforms of the enabled channels (as 16 bits data)
*
* \param   Var Pointer to the plot struct
* \param   TraceName Name of the trace to plot
* \param   Data Pointer to sample buffer (16 bits words)
* \param   Size Number of samples stored into the buffer to plot
ì******************************************************************************/
int PlotLoadTrace16(WDPlot_t *Var, char *TraceName, uint16_t *Data, uint32_t Size)
{
	int TracePnt= Var->NumTraces;
    if (TracePnt >= MAX_NUM_TRACES)
        return -1;

    Var->TraceSize[TracePnt] = Size;
    Var->TraceData[TracePnt] = (int16_t *)malloc(Size * sizeof(**Var->TraceData));
    if (Var->TraceData[TracePnt] == NULL)
        return -1;
    memcpy(Var->TraceData[TracePnt], Data, Size * sizeof(**Var->TraceData));
    strcpy(Var->TraceName[TracePnt], TraceName);
    TracePnt++;
    Var->NumTraces = TracePnt;
    return 0;
}

/**************************************************************************//**
* \brief   Plot the waveforms of the enabled channels (as 8 byts data)
*
* \param   Var Pointer to the plot struct
* \param   TraceName Name of the trace to plot
* \param   Data Pointer to sample buffer (8 bits words)
* \param   Size Number of samples stored into the buffer to plot
ì******************************************************************************/
int PlotLoadTrace8(WDPlot_t *Var, char *TraceName, uint8_t *Data, uint32_t Size)
{
	int TracePnt= Var->NumTraces;
    if (TracePnt >= MAX_NUM_TRACES)
        return -1;

    Var->TraceSize[TracePnt] = Size;
    Var->TraceData[TracePnt] = (int16_t *)malloc(2* Size * sizeof(**Var->TraceData));
    if (Var->TraceData[TracePnt] == NULL)
        return -1;
	while( Size-->= 0) {
		Var->TraceData[TracePnt][Size]= Data[Size];
	}
    strcpy(Var->TraceName[TracePnt], TraceName);
    TracePnt++;
    Var->NumTraces = TracePnt;
    return 0;
}

/**************************************************************************//**
* \brief   Plot the waveforms of the enabled channels
* \param   Var Pointer to the plotting data struct
******************************************************************************/
#ifdef FREE_RUN_THREAD
	int PlotWaveforms(WDPlot_t *Var)
	{
		if (gnuplot == NULL)
			return -1;
		if (Var->NumTraces == 0)
			return 0;
		if( IsPlotterBusy( Var)) {
			return -1;
		}
		Var->Busy= 1;
		Var->DataReady= 1;
		if( ThreadHandler<= 0) {
		#ifdef WIN32
			ThreadHandler= (HANDLE)_beginthreadex( 
				NULL,	// security,
				0,		// stack_size,
				&PlotThread,
				Var,	// arglist,
				0,		// initflag,
				NULL	// thrdaddr 
				); 
			if( (ThreadHandler) <= 0)  {
				// Error while starting thread
				return -1;
			}
		#else
			if( pthread_create( &ThreadHandler, NULL, PlotThread, (void*) Var)) {
				return -1;
			}
		#endif
		}

		return 0;
	}
#else
	int PlotWaveforms(WDPlot_t *Var)
	{
		if (gnuplot == NULL)
			return -1;
		if (Var->NumTraces == 0)
			return 0;
		if( IsPlotterBusy(Var)) {
			return -1;
		}
	#ifdef WIN32
		if( ThreadHandler> 0) {
			CloseHandle( ThreadHandler);
			ThreadHandler= 0;
		}

		// TODO rifare sincronizzandosi su un thread già in run
		ThreadHandler= (HANDLE)_beginthreadex( 
				NULL,	// security,
				0,		// stack_size,
				&PlotThread,
				Var,	// arglist,
				0,		// initflag,
				NULL	// thrdaddr 
				); 
		if( ((int)ThreadHandler) <= 0)  {
			// Error while starting thread
			return -1;
		}
	#else
		HACK: call thread for Linux
	#endif

		return 0;
	}
#endif


/**************************************************************************//**
* \fn      void ClosePlotter(void)
* \brief   Close the plotter (gnuplot pipe)
******************************************************************************/
int ClosePlotter(WDPlot_t *PlotVar)
{
	if ((PlotVar != NULL)&& ThreadHandler> 0) {
#ifdef FREE_RUN_THREAD
		PlotVar->Exit= 1;
#else
#endif
#ifdef WIN32
		WaitForSingleObject( ThreadHandler, 10000);
#else
		pthread_join( ThreadHandler, NULL);
#endif
	}
    if (gnuplot != NULL)
        pclose(gnuplot);
	if (PlotVar != NULL)
        free(PlotVar);
    return 0;
}

/**************************************************************************//**
* \brief   Check if plot has finished 
******************************************************************************/
#ifdef FREE_RUN_THREAD
	int IsPlotterBusy( WDPlot_t *PlotVar)
	{
		if( ( ThreadHandler<= 0)|| !PlotVar) {
			return 0;
		}
		return PlotVar->Busy;
	}
#else
	int IsPlotterBusy( WDPlot_t *PlotVar)
	{
		if( ThreadHandler<= 0) {
			return 0;
		}
	#ifdef WIN32
		switch( WaitForSingleObject( ThreadHandler, 0)) {
			case WAIT_OBJECT_0:
				return 0;
		}
	#else
		HACK: Check for thread completion here for Linux
	#endif
		return 1;
	}
#endif
