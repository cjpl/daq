#include "wave.h"

/* ###########################################################################
*  Functions
*  ###########################################################################
*/

/* get time in milliseconds */
static long get_time()
{
long time_ms;

#ifdef WIN32
    struct _timeb timebuffer;

    _ftime( &timebuffer );
    time_ms = (long)timebuffer.time * 1000 + (long)timebuffer.millitm;
#else
    struct timeval t1;
    struct timezone tz;

    gettimeofday(&t1, &tz);
    time_ms = (t1.tv_sec) * 1000 + t1.tv_usec / 1000;
#endif

    return time_ms;
}

static void PrintChannelHeaderInfo(int ChannelGroup, int Channel, WAVE_CONFIG* wave_config, EVENT_INFO* event_info) {

    if (wave_config->OutputFormat > 0)
        fprintf(wave_config->ofile[ChannelGroup][Channel], "%d\n", (event_info->EventSize-4) );

    if (wave_config->OutputFormat > 1) {
        fprintf(wave_config->ofile[ChannelGroup][Channel], "%06X\n", event_info->EventCounter);
        fprintf(wave_config->ofile[ChannelGroup][Channel], "%4X\n" , event_info->Pattern);
        fprintf(wave_config->ofile[ChannelGroup][Channel], "%08X\n", event_info->TriggerTimeTag);
    }
}

/*
* Multiplicity: calculate the number of channels that partecipate to one event
* Input = Channel Mask
* Return: number of channels 
*/

static int Multiplicity(int mask)
{
    int i, m=0;

    for(i=0; i<8; i++)
        if((mask>>i) & 1)
            m++;
    return m;
}


static void Plot(FILE *gnuplot) {

    FILE *f_pltcmd; /* GNUPLOT script file*/

    if ((f_pltcmd = fopen("plot.plt", "r"))) {
      fprintf(gnuplot,"load 'plot.plt'\n"); /* launch Gnuplot script from file */
      fflush(gnuplot);
      fclose(f_pltcmd);
    }
    else {
        printf("Plot Info: cannot open Gnuplot script file plot.plt in current directory!\n");
    }

}


/*
** Size : numero (minimo) di locazioni da spacchettare.
** Nel V1720 va sempre spacchettato a gruppi di 2 longword.
** TODO: ora fa lo spacchettamento a gruppi di 4 long, ma potrebbe essere fatto a gruppi di 2 (5 samples).
*/
static int V1720UnpackEvent(int size, UINT32_T *datain, UINT32_T *dataout) {

    int rpnt = 0, wpnt = 0;

    while (rpnt < size) {

        /*
        * Map 10 samples (four packed event words) into 10 consecutive longword locations
        * 1 longword = right-ailgned 12-bit sample
        */        
        dataout[wpnt]    = datain[rpnt] & 0x00000FFF;             /* S0[11:0] */
        dataout[++wpnt]  = ((datain[rpnt] & 0x00FFF000) >> 12);   /* S1[11:0] */
        dataout[++wpnt]  = ((datain[rpnt] & 0x3F000000) >> 24);   /* S2[ 5:0] (low) */

        ++rpnt;
        
        dataout[wpnt]   |= ((datain[rpnt] & 0x0000003F) << 6);    /* S2[11:6] (high) */
        dataout[++wpnt]  = ((datain[rpnt] & 0x0003FFC0) >> 6);    /* S3[11:0] */
        dataout[++wpnt]  = ((datain[rpnt] & 0x3FFC0000) >> 18);   /* S4[11:0] */

        ++rpnt;

 //       dataout[++wpnt]  = ((datain[rpnt] & 0x00000FFF)) ;        /* S5[11:0] */
 //       dataout[++wpnt]  = ((datain[rpnt] & 0x00FFF000) >> 12);   /* S6[11:0] */
 //       dataout[++wpnt]  = ((datain[rpnt] & 0x3F000000) >> 24) ;  /* S7[ 5:0] (low) */
 //       ++rpnt;
        
 //       dataout[wpnt]   |= ((datain[rpnt] & 0x0000003F) << 6);    /* S7[11:6] (high) */

 //       dataout[++wpnt]  = ((datain[rpnt] & 0x0003FFC0) >> 6) ;   /* S8[11:0] */
 //       dataout[++wpnt]  = ((datain[rpnt] & 0x3FFC0000) >> 18);   /* S9[11:0] */

 //       ++rpnt;
        ++wpnt;
    }

    return wpnt;
}

/*
** Uncompress wave data (ZLE Zero Suppression Algorithm):
** Read the compressed data from the buffer datain and write
** the uncompressed data into dataout. Return the size of the
** uncompressed waveform in longwords (32-bit).
*/
static int WaveUncompress(BOARD_CONFIG *board_config, UINT32_T *datain, UINT32_T *dataout)
{
    int nwc, nw, rpnt, wpnt;

    rpnt = 0;
    wpnt = 0;
    nwc = datain[rpnt++]; /* number of words in the compressed buffer */
    while(rpnt < nwc) {
        nw = datain[rpnt] & 0x7FFFFFFF;
        if (datain[rpnt++] & 0x80000000) {   /* Good Samples */
            if (board_config->Pack25) {
              wpnt += V1720UnpackEvent( nw, datain+rpnt, dataout+wpnt);
              rpnt += nw;
            } else {
              memcpy(dataout+wpnt, datain+rpnt, nw*sizeof(UINT32_T));
              rpnt += nw;
              wpnt += nw;
            }
        } else {                             /* Suppressed Samples */
            if (board_config->Pack25) {
              memset(dataout+wpnt, 0, nw*2.5*sizeof(UINT32_T));
              wpnt += nw*2.5;
            } else {
              memset(dataout+wpnt, 0, nw*sizeof(UINT32_T));
              wpnt += nw;
            }
        }
    }
    return wpnt;
}



/*
** size = datain buffer length in long words.
*/
static int V1740UnpackEvent(int size, UINT32_T *datain, UINT32_T *dataout) {

    int i, rpnt = 0, wpnt = 0;
    long samples;

    UINT32_T *channel[8];

    samples = ((long) (size * 2.6667)) / 8; 

    for(i = 0; i < 8; i++) {
      channel[i] = &dataout[i*samples];
    }

    while (rpnt < size) {

        switch (rpnt % 9) {
            case 0 :
              channel[0][wpnt]    = datain[rpnt] & 0x00000FFF;             /* S0[11:0] - CH0 */
              channel[0][wpnt+1]  = (datain[rpnt] & 0x00FFF000) >> 12;     /* S1[11:0] - CH0 */
              channel[0][wpnt+2]  = (datain[rpnt] & 0xFF000000) >> 24;     /* S2[ 7:0] - CH0 */
              break;
          case 1 :
              channel[0][wpnt+2] |= (datain[rpnt] & 0x0000000F) << 8;      /* S2[11:8] - CH0 */
              channel[1][wpnt]    = (datain[rpnt] & 0x0000FFF0) >> 4;      /* S0[11:0] - CH1 */
              channel[1][wpnt+1]  = (datain[rpnt] & 0x0FFF0000) >> 16;     /* S1[11:0] - CH1 */
              channel[1][wpnt+2]  = (datain[rpnt] & 0xF0000000) >> 28;     /* S2[ 3:0] - CH1 */
              break;
          case 2 :
              channel[1][wpnt+2] |= (datain[rpnt] & 0x000000FF) << 4;      /* S2[11:4] - CH1 */
              channel[2][wpnt]    = (datain[rpnt] & 0x000FFF00) >> 8;      /* S0[11:0] - CH2 */
              channel[2][wpnt+1]  = (datain[rpnt] & 0xFFF00000) >> 20;     /* S1[11:0] - CH2 */
              break;
          case 3 :
              channel[2][wpnt+2]  = (datain[rpnt] & 0x00000FFF) ;          /* S2[11:0] - CH2 */
              channel[3][wpnt]    = (datain[rpnt] & 0x00FFF000) >> 12;     /* S0[11:0] - CH3 */
              channel[3][wpnt+1]  = (datain[rpnt] & 0xFF000000) >> 24;     /* S1[ 7:0] - CH3 */
              break;
          case 4 :
              channel[3][wpnt+1] |= (datain[rpnt] & 0x0000000F) << 8;      /* S1[11:8] - CH3 */
              channel[3][wpnt+2]  = (datain[rpnt] & 0x0000FFF0) >> 4;      /* S2[11:0] - CH3 */
              channel[4][wpnt]    = (datain[rpnt] & 0x0FFF0000) >> 16;     /* S0[11:0] - CH4 */
              channel[4][wpnt+1]  = (datain[rpnt] & 0xF0000000) >> 28;     /* S1[ 3:0] - CH4 */
              break;
          case 5 :
              channel[4][wpnt+1] |= (datain[rpnt] & 0x000000FF) << 4;      /* S1[11:4] - CH4 */
              channel[4][wpnt+2]  = (datain[rpnt] & 0x000FFF00) >> 8;      /* S2[11:0] - CH4 */
              channel[5][wpnt]    = (datain[rpnt] & 0xFFF00000) >> 20;     /* S0[11:0] - CH5 */
              break;
          case 6 :
              channel[5][wpnt+1]  = (datain[rpnt] & 0x00000FFF) ;          /* S1[11:0] - CH5 */
              channel[5][wpnt+2]  = (datain[rpnt] & 0x00FFF000) >> 12;     /* S2[11:0] - CH5 */
              channel[6][wpnt]    = (datain[rpnt] & 0xFF000000) >> 24 ;    /* S0[ 7:0] - CH6 */
              break;
          case 7 :
              channel[6][wpnt]   |= (datain[rpnt] & 0x0000000F) << 8 ;     /* S0[11:8] - CH6 */
              channel[6][wpnt+1]  = (datain[rpnt] & 0x0000FFF0) >> 4 ;     /* S1[11:0] - CH6 */
              channel[6][wpnt+2]  = (datain[rpnt] & 0x0FFF0000) >> 16 ;    /* S2[11:0] - CH6 */
              channel[7][wpnt]    = (datain[rpnt] & 0xF0000000) >> 28 ;    /* S0[ 3:0] - CH7 */
              break;
          case 8 :
              channel[7][wpnt]   |= (datain[rpnt] & 0x000000FF) << 4 ;     /* S0[11:4] - CH7 */
              channel[7][wpnt+1]  = (datain[rpnt] & 0x000FFF00) >> 8 ;     /* S1[11:0] - CH7 */
              channel[7][wpnt+2]  = (datain[rpnt] & 0xFFF00000) >> 20 ;    /* S2[11:0] - CH7 */
              wpnt+=3;
              break;
        }
          rpnt++;
   }
 
  return wpnt;
}

/*
*  Unpack event data if needed.
*  Returns the number of DWORDS the dataout is made of.
*/
static int WaveBuild(BOARD_CONFIG *board_config, EVENT_INFO *event_info, UINT32_T *datain, UINT32_T *dataout) {

    int nw;
    

    nw=(int)(event_info->EventSize-4)/Multiplicity(event_info->PartChannelMask);  /* Number of words (per channel) */

    switch(board_config->BoardType) {
      case V1724 :
      case V1721 :
      case V1731 :  
                    memcpy(dataout, datain, nw*sizeof(UINT32_T));
                    break;

      case V1740 :
                    V1740UnpackEvent(nw, datain, dataout);  
                    break;

      case V1720 :  
                    if (board_config->Pack25) 
                      nw=V1720UnpackEvent(nw, datain, dataout);    /* HACK DBG: verificare l'uso di nw  */                  
                    else 
                      memcpy(dataout, datain, nw*sizeof(UINT32_T));

                    break;

      default    : ; 
    }

    return nw;
}

static void WaveSetPackCoeff(BOARD_CONFIG* board_config) {

    switch(board_config->BoardType) {
      case V1724 :
      case V1721 :
      case V1731 :  
                    board_config->SamplesPackCoeff = 2.0; /* Two samples per dword */
                    break;

      case V1740 :
                    board_config->SamplesPackCoeff = 8.0/3.0; /* 8 samples ogni 3 dword */
                    break;

        case V1720 :  
                    if (board_config->Pack25) 
                      board_config->SamplesPackCoeff = 2.5;  // 5 samples ogni 2 parole                   
                    else 
                     board_config->SamplesPackCoeff = 2.0;  // 2 samples ogni 1 parola
                    break;

      default    : 
                    ; 
    
    }
}

/* TODO : documentare la funzione */
static UINT32_T GetChannelSamples(BoardModel model, UINT32_T memsize) {

    UINT32_T GroupSamples;

    switch(model) {
      case V1724 :
      case V1721 :
      case V1731 :  
                    GroupSamples = memsize; /* HACK TBC */
                    break;

      case V1740 :
                    GroupSamples = ((memsize/8) * 12) / 2;  /* HACK TBC */
                    break;

      case V1720 :  
                    GroupSamples = memsize; /* HACK TBC */
                    break;

      default    : ; 
    }

    return GroupSamples;
}

static void WaveMalloc(VME_ACCESS* vme, BOARD_CONFIG* board_config, EVENT_INFO* event_info, WAVE_CONFIG* wave_config) {

    int i;


    /* Allocate memory Buffers for VME readout */
    for( i= 0, board_config->PartChannels = 0; i< 8; i++) {
       if( ( board_config->ChannelEnableMask & ( 1<< i)))
         board_config->PartChannels++;
    }

    /* Set maximum buffer size for event readout based on current configuration */
    board_config->BufferSize = (board_config->EvAlign && board_config->EnableBerr) ? (board_config->ExpectedEvSize* board_config->EvAlign * 4) :  (board_config->ExpectedEvSize*4); /* Bytes */
    board_config->BufferSize += wave_config->BltSize; /* Allocate space for one more BLT */

    if ( (vme->vbuf = (UINT32_T *)malloc(board_config->BufferSize)) == NULL) {
        printf("Can't allocate memory buffer of %d KB\n", board_config->BufferSize/1024);
        exit(-3);
    }


    event_info->ChSamples = board_config->ChannelEventSize; /* HACK DBG : duplicato? */

  /*
  ** Allocating buffers for eight signal groups 
  ** (8 physical channels = 1 group in V1740; 1 physical channel = 1 group for other boards)
  ** One 32-bit word for every sample.
  **/  
    for(i=0; i < 8; i++) 
    if ( (event_info->chbuf[i] = (UINT32_T *)malloc(event_info->ChSamples *sizeof(UINT32_T))) == NULL) { 
        printf("Can't allocate channel memory buffer of %d KB\n", board_config->ChannelEventSize/1024);
        exit(-4);
    }

}

/******************/
/* Public methods */
/******************/

/*! \fn      void WavePrintVersion(const char *SwRelease);
*   \brief   WaveDump Information print. 
*            
*   \param   SwRelease    Software release.
*/
void WavePrintVersion(const char *SwRelease) {
    printf("\n");
    printf("**************************************************************\n");
    printf("                        Wave Dump %s                        \n", SwRelease);
    printf("**************************************************************\n"); 
}

/*! \fn      void WaveInit(int argc, char *argv[], VME_ACCESS* vme, WAVE_CONFIG* wave_config);
*   \brief   WaveDump Initialization. 
*            
*   \param   argc         Number of command line parameters.
*   \param   argv         pointer to array of command line parameters.
*   \param   vme          Pointer to VME access configuration.
*   \param   wave_config  Pointer to wave configuration.
*/
void WaveInit(int argc, char *argv[], VME_ACCESS* vme, WAVE_CONFIG* wave_config) {

    char ConfigFileName[100] = "/etc/wavedump/WaveDumpConfig.txt";
    char tmp[100], str[400];

    FILE *f_ini;

        /* 
    ************************************************************************
    ** Read configuration file
    ************************************************************************
    */
    if (argc > 1)
        strcpy(ConfigFileName, argv[1]);

    if ( (f_ini = fopen(ConfigFileName, "r")) == NULL ) {
        printf("Can't open Configuration File %s\n", ConfigFileName);
        exit(-1);
    }

    /* Setting the default base address */
    vme->BaseAddress = 0x32100000;


    printf("Reading Configuration File %s\n", ConfigFileName);
    while(!feof(f_ini)) {
        fscanf(f_ini, "%s", str);
        if (str[0] == '#')
            fgets(str, 1000, f_ini);
        else
            {
            // LINK: Open VME master
            if (strstr(str, "LINK")!=NULL) {
                fscanf(f_ini, "%s", tmp);
#ifdef __USE_CAEN_VME_BRIDGES__
                if (strstr(tmp, USBBRIDGE_STRING)!=NULL)
                    vme->BType = USBBRIDGE;
                else if (strstr(tmp, CONETBRIDGE_STRING)!=NULL)
                    vme->BType = CONETBRIDGE;
#else // __USE_CAEN_VME_BRIDGES__
                if (strstr(tmp, VMECPU_STRING)!=NULL)
                    BType = VMECPU;
#endif // __USE_CAEN_VME_BRIDGES__
                else {
                    printf("Invalid/Unsupported VME Bridge/CPU Type\n");
                    exit(-1);
                }
                fscanf(f_ini, "%d", &vme->link);
                fscanf(f_ini, "%d", &vme->bdnum);
                if (VME_INIT(vme->BType, vme->link, vme->bdnum, &vme->handle) != SUCCESS) {
                    printf("VME Bridge init failure\n");
                    exit(-1);
                }
            }

            // Data Check Mode
            // Bit 0 = Calculate and Print Throughput Rate
            // Bit 1 = Check Header Consistency
            // Bit 2 = Check Size
            // Bit 3 = Consecutive Event Counters
            if (strstr(str, "DATA_CHECK")!=NULL)
                fscanf(f_ini, "%x", &wave_config->CheckMode);

            // Write Data to output Files (0=Don't write, 1=On Request, 2=Continuous)
            if (strstr(str, "WRITE_TO_FILE")!=NULL) {
                fscanf(f_ini, "%d", &wave_config->WriteToFile);
                if (wave_config->WriteToFile == 2)
                    printf("[NOTE] Wavedump with CONTINOUS EVENT DUMP to file enabled!\n");
            }

            // 0=Overwrite file 1=append the waveform to the previous on
            if (strstr(str, "APPEND_MODE")!=NULL)
                fscanf(f_ini, "%d", &wave_config->AppendMode);

            // Output File Format (0=Data Only, 1=Data + Size, 2=Data + Header)
            if (strstr(str, "OUTPUT_FORMAT")!=NULL)
                fscanf(f_ini, "%d", &wave_config->OutputFormat);

            // Readout Blt Size (in Bytes)
            if (strstr(str, "BLT_SIZE")!=NULL)
                fscanf(f_ini, "%d", &wave_config->BltSize);

            // Base Address
            if (strstr(str, "BASE_ADDRESS")!=NULL)
                fscanf(f_ini, "%x", (int *)&vme->BaseAddress);

            // Readout Mode (0=Single D32, 1=BLT, 2=MBLT, 3=2eVME, 4=2eSST)
            if (strstr(str, "READOUT_MODE")!=NULL)
                fscanf(f_ini, "%d", &wave_config->ReadoutMode);

            // Plot Command

            // GNUplot path
            if (strstr(str, "GNUPLOT_PATH")!=NULL) {
                fscanf(f_ini, "%s", wave_config->GnuPlotPath);
                wave_config->UseGnuPlot = 1;
            }

            // Generic VME Write
            if (strstr(str, "WRITE_REGISTER")!=NULL) {
                fscanf(f_ini, "%x", (int *)&vme->addr);
                fscanf(f_ini, "%x", (int *)&vme->data);
                if (VME_WRITE32(vme->handle, vme->BaseAddress + vme->addr, &vme->data) != SUCCESS) {
                    printf("VME Write failure at address %08lX\n", vme->BaseAddress + vme->addr);
                    fclose (f_ini);
                    exit(-1);
                }
            }
        }
    }
}

/*! \fn      int WaveUserInput(VME_ACCESS* vme, WAVE_CONFIG* wave_config);
*   \brief   WaveDump read of current board configuration. 
*            
*   \param   vme          Pointer to VME access configuration.
*   \param   wave_config  Pointer to wave configuration.
*   \return  Return 1 if user's request to exit from program. 0 otherwise.
*/
int WaveUserInput(VME_ACCESS* vme, WAVE_CONFIG* wave_config) {

    char c;
    int ret = 0;
    UINT32_T data;

    FILE *f_wcfg;
 
    int continous_write        =  (wave_config->WriteToFile & 0x2);
    static int continous_plot  = 0;

    const char *cont_write_enabled  = "Enabled";
    const char *cont_write_disabled = "Disabled";



/* Check keyboard commands */
        c = 0;
        if(kbhit())
            c = getch();

        /* open gnuplot for the fisrt time */
        if((toupper(c) == 'P') && (wave_config->gnuplot == NULL)) {

            if (wave_config->UseGnuPlot) {
                sprintf(wave_config->GnuPlotExe, "%s%s", wave_config->GnuPlotPath, GNUPLOT_COMMAND);
               
                // open the pipe
                wave_config->gnuplot = popen(wave_config->GnuPlotExe, "w"); 

                if ((f_wcfg = fopen("init.plt", "r")) == NULL )
                    ; // No action if initial GNUPlot config file is not present
                else {
                  fclose(f_wcfg);
                  fprintf(wave_config->gnuplot,"load 'init.plt'\n");
                  fflush(wave_config->gnuplot);
                }

                Plot(wave_config->gnuplot);
            }
            else
                printf("GNUPlot path not set in Configuration file. Cannot plot!\n");
        }

        switch(c) {
                case 'q' :
                     ret = 1;
                     break;
    
                case 't' :
                     VME_WRITE32(vme->handle, vme->BaseAddress + SoftTriggerReg, &data);
                     break;
    
                case 'T' :
                     wave_config->TriggerMode = (wave_config->TriggerMode == 0);
                     break;
    
                case 'w' :
                     wave_config->WriteOneEvent = 1;
                     break;
    
                case 'W' :
                    wave_config->WriteToFile = continous_write ? wave_config->WriteToFile & ~(0x2) : wave_config->WriteToFile | 0x2;
                    continous_write    =  (wave_config->WriteToFile & 0x2);
                    printf("Toggling event write to file: Continous write %s\n", continous_write ? cont_write_enabled : cont_write_disabled);
                     break;

                case 'p' :
                    Plot(wave_config->gnuplot);
                    break;

                case 'P' :
                    continous_plot = ~continous_plot;
                    break;

                default : break;
        }

        if (continous_plot)
           Plot(wave_config->gnuplot);

        return ret;
}

/*! \fn      void WaveReadBoardConfiguration(VME_ACCESS* vme, BOARD_CONFIG* board_config);
*   \brief   WaveDump read of current board configuration. 
*            
*   \param   vme          Pointer to VME access configuration.
*   \param   board_config Pointer to board configuration.
*/
void WaveReadBoardConfiguration(VME_ACCESS* vme, BOARD_CONFIG* board_config) {


    VME_READ32(vme->handle, vme->BaseAddress + ChannelEnableMaskReg, &board_config->ChannelEnableMask);


    // Read Board Type and Memory Size
    VME_READ32(vme->handle, vme->BaseAddress + BoardInfoReg, &vme->data);
    board_config->BoardType  = vme->data & 0xFF;  // 0=V1724, 1=V1721, 2=V1731, 3=V1720, 4=V1740
    board_config->MemorySize = (vme->data >> 8) & 0xFF;  // Memory size in MBytes
    printf("Board Type : %s; Memory Size : %d MByte per channel\n", BoardTypeName[board_config->BoardType], board_config->MemorySize);

    // Read Acquisition Control Register
    VME_READ32(vme->handle, vme->BaseAddress + ChannelConfigReg, &vme->data);
    board_config->ZeroAlg = (vme->data>>16) & 0xF; // 0=Nothing, 1=Full Suppression, 2=Wave Compress, 3=On Threshold Suppress
    board_config->Pack25  = (vme->data>>11) & 1;  // Sample Packing 2.5 (V1720 only)
    board_config->DesMode = (vme->data>>12) & 1;  // V1731 Dual Edge Sampling Mode (1 GS/s x 4 channels)

    // Read Buffer Organization
    VME_READ32(vme->handle, vme->BaseAddress + BlockOrganizationReg, &board_config->BlockOrg);


    WaveSetPackCoeff(board_config);

    board_config->ChannelEventSize = (UINT32_T)( GetChannelSamples(board_config->BoardType, board_config->MemorySize*pow(2,20))  / (int)pow(2,board_config->BlockOrg)); /* samples per event */
    board_config->ExpectedEvSize   = (UINT32_T)( ( ((board_config->ChannelEventSize / board_config->SamplesPackCoeff) * 4)* Multiplicity(board_config->ChannelEnableMask) + 16) / 4); // Expected Event Size in Words (including Header)

    if ( (board_config->BoardType == V1731) && (board_config->DesMode == 1) )
      board_config->ExpectedEvSize *= 2;


    /* Read VME control Register */
    VME_READ32(vme->handle, vme->BaseAddress + VMEControlReg, &vme->data);
    board_config->EnableBerr   = ((vme->data>>4) & 0x1);
    board_config->EnableOLIrq  = (vme->data      & 0x8);
    board_config->EnableVMEIrq = (vme->data      & 0x7);
    board_config->Align64      = (vme->data>>5)  & 1;

    board_config->EnableInt    =  board_config->EnableVMEIrq | board_config->EnableOLIrq;

    if (board_config->EnableOLIrq)
      printf("OLINK Interrupt enabled.\n");

    if (board_config->EnableVMEIrq)
      printf("VME Interrupt %d enabled.\n", board_config->EnableVMEIrq);

    // Read BLT Event Number Register
    VME_READ32(vme->handle, vme->BaseAddress + BltEvNumReg, &vme->data);
    board_config->EvAlign = vme->data;

    // Read Firmware Revisions
    VME_READ32(vme->handle, vme->BaseAddress + ChannelFWRevision, &vme->data);
    printf("Mezzanine FW Revision : %ld.%ld (%lx/%ld/%ld)\n", vme->data>>8 & 0xFF, vme->data & 0xFF, vme->data>>16 & 0xFF,
           vme->data>>24 & 0xF, 2000 + (vme->data>>28 & 0xF));
    VME_READ32(vme->handle, vme->BaseAddress + MotherBoardFWRevision, &vme->data);
    printf("Mother Board FW Revision : %ld.%ld (%lx/%ld/%ld)\n", vme->data>>8 & 0xFF, vme->data & 0xFF, vme->data>>16 & 0xFF,
           vme->data>>24 & 0xF, 2000 + (vme->data>>28 & 0xF));

    // Read PostTrigger Setting
    VME_READ32(vme->handle, vme->BaseAddress + PostTriggerReg, &board_config->posttrig);
    printf("PostTrigger Setting= %d\n", board_config->posttrig);

    printf("Board Configured. Press a key to start the acquisition ('q' to quit)\n");
    if (getch()=='q')
        exit(0);

}


/*! \fn      void WaveRun(VME_ACCESS* vme, EVENT_INFO* event_info, WAVE_CONFIG* wave_config, BOARD_CONFIG* board_config);
*   \brief   WaveDump run main loop. 
*            
*   \param   vme          Pointer to VME access configuration.
*   \param   event_info   Pointer to event informations.
*   \param   wave_config  Pointer to waveform configuration.
*   \param   board_config Pointer to board configuration.
*/
void WaveRun(VME_ACCESS* vme, EVENT_INFO* event_info, WAVE_CONFIG* wave_config, BOARD_CONFIG* board_config) {

    long CurrentTime, PreviousTime, ElapsedTime;
    UINT64_T totnb=0, last_totnb=0;

    UINT32_T tottrg=0, last_tottrg=0;
    double TPrate, TRGrate;
    int blt_bytes;
    int EvCnt;
    int i, j, ch, pnt, nw, nb, quit;

    char OutFileName[100];

    /* Memory allocation for Vme transfers and event buffers */
      WaveMalloc(vme, board_config, event_info, wave_config);


    
      /*************************************************************************
    ** Readout
    *************************************************************************/
    /* Acquisition Start */
    VME_READ32(vme->handle, vme->BaseAddress + AcquisitionControlReg, &vme->data);
    vme->data |= 0x4;
    VME_WRITE32(vme->handle, vme->BaseAddress + AcquisitionControlReg, &vme->data);

    printf("[t] SW Trigger (shot) [T] Toggle SW Trigger mode (shot/cont)\n[w] write one event [W] Toggle event write mode (on command/continous)\n[p] plot one event [P] Continous event plot\n[q] quit\n");
    printf("\nAcquisition started.\n");
    PreviousTime = get_time();
    EvCnt = 0;
    quit = 0;

    /*
    ** Readout Loop
    */    
    while(!quit) {

 
        quit = WaveUserInput(vme,wave_config);
        

        /* Send a software trigger */
        if (wave_config->TriggerMode == 1)
            VME_WRITE32(vme->handle, vme->BaseAddress + SoftTriggerReg, &vme->data);


        /* 
        ** If enabled, wait for the interrupt request from the digitizer. In this mode,
        ** there is no CPU time wasting because the process sleeps until there are at least
        ** N event available for reading, where N is the content of the register Interrupt
        ** Event Number (0xEF18)
        */
        if (board_config->EnableInt) {

            if (board_config->EnableOLIrq)
                vme->IrqMask = 0x01; /* IRQ1 is used when interrupt generated via OLINK */
            else
                vme->IrqMask = 0xFF; /* All VME Interrupt Enabled */

      VME_IRQENABLE(vme->handle, vme->IrqMask); /* Enable IRQs */
            vme->ret = VME_IRQWAIT(vme->handle, vme->IrqMask, 1000); /* Wait for IRQ (max 1sec) */
            if(vme->ret)
                continue;

            vme->ret=VME_IRQCHECK(vme->handle, &vme->IrqCheckMask);
            VME_IRQDISABLE(vme->handle, vme->IrqMask); /* Disable IRQs */
        }

        /* Single D32 mode */
        if (wave_config->ReadoutMode == 0) {
            VME_READ32(vme->handle, vme->BaseAddress, vme->vbuf);
            nb = 0;
            if (vme->vbuf[0] != FILLER) {
                nb = NUM_WORD(vme->vbuf[0]) * 4;
                if (nb > board_config->BufferSize) {
                   printf("BUFFER_SIZE too small! Please increase it and run again.. \nPress any key to continue..\n");
                   continue; /* exit while loop */
                }
                for(i=1; i < (nb/4); i++)
                    VME_READ32(vme->handle, vme->BaseAddress, (vme->vbuf)+i);
            }
            totnb += nb;  /* Total number of bytes tranferred */
        } 
        else {
            /* Block Transfer Mode. There are two operating modes:
            ** 1) Bus Error enabled: the BLT size can be the whole buffer size; in fact, it is care
            **    of the slave (digitizer) to assert BERR and stop the BLT when the data are finished.
            ** 2) Bus Error Disabled: Read the event size in single D32 mode and then read the event
            **    in BLT mode setting the BLT size equal to the exact number of words
            */
            blt_bytes = 0;
            if (board_config->EnableBerr) {
                do {

                    switch(wave_config->ReadoutMode) {
                        case 1 : vme->ret = VME_FIFOBLTREAD(vme->handle, vme->BaseAddress, ((unsigned char*)vme->vbuf)+blt_bytes, wave_config->BltSize, &nb);
                                 break;
                        case 2 : vme->ret = VME_FIFOMBLTREAD(vme->handle, vme->BaseAddress, ((unsigned char*)vme->vbuf)+blt_bytes, wave_config->BltSize, &nb);
                                 break;
                        default: break;
                    }

                  if ((vme->ret != SUCCESS) && (vme->ret != BUSERROR)) {
                        printf("VME_FIFOBLTREAD Error = %d\n", vme->ret);
                        continue; /* exit while loop */
                  }
                  blt_bytes += nb;
                  if (blt_bytes > board_config->BufferSize) {
                    printf("BUFFER_SIZE too small! Please increase it and run again.. \nPress any key to continue..\n");
                    continue; /* exit while loop */
                  }

                  totnb     += nb; /* Total number of bytes tranferred */
                }
                while (vme->ret != BUSERROR);
                nb = blt_bytes;
            } else {
                do // Poll EventSizeReg until event size is <> 0
                    VME_READ32(vme->handle, vme->BaseAddress + EventSizeReg, &vme->data);
                while (vme->data == 0x00000000);

                // If a MBLT transfer with an odd number of long words, one more lword must be requested
                switch(wave_config->ReadoutMode) {
                   case 1 : board_config->EventSize = (vme->data * 4);
                            break;
                   case 2 : board_config->EventSize = (vme->data % 2) ? (vme->data+1)*4 : (vme->data * 4);
                            break;
                   default:break;
                }
                if (board_config->EventSize > board_config->BufferSize) {
                   printf("BUFFER_SIZE too small! Please increase it and run again.. \nPress any key to continue..\n");
                   continue; /* exit while loop */
                }

                // Read data until data read is >= EventSize
                do {
                    switch(wave_config->ReadoutMode) {
                        case 1 : vme->ret=VME_FIFOBLTREAD(vme->handle, vme->BaseAddress, ((unsigned char*)vme->vbuf)+blt_bytes, wave_config->BltSize, &nb);
                                 break;
                        case 2 : vme->ret=VME_FIFOMBLTREAD(vme->handle, vme->BaseAddress, ((unsigned char*)vme->vbuf)+blt_bytes, wave_config->BltSize, &nb);
                                 break;
                        default: break;
                    }

                  blt_bytes += nb;
                  totnb     += nb; // Total number of bytes tranferred
                }
                while (blt_bytes < board_config->EventSize);
                nb = blt_bytes;
            }
        }

        /* Calculate throughput rate (every second) */
        if (wave_config->CheckMode & 1) {
            CurrentTime = get_time(); // Time in milliseconds
            ElapsedTime = CurrentTime - PreviousTime;
            if (ElapsedTime > 1000) {
                TPrate = ((float)(totnb - last_totnb) / ElapsedTime)*1000.0;     // Bytes/second
                TRGrate = ((float)(tottrg - last_tottrg) / ElapsedTime)*1000.0;  // Triggers/second
                if (last_totnb == totnb)
                    printf("No data...\n");
                else
                    printf("%6lld MB read @ %.2fMB/s. TrgRate=%.4f Hz\n", totnb/1048576, TPrate/1048576, TRGrate);
                last_totnb = totnb;
                last_tottrg = tottrg;
                PreviousTime = CurrentTime;
            }
        }

        // Analyse data and write to file
        pnt = 0;
        while ((pnt < (nb/4)) && (vme->vbuf[pnt] != FILLER)) {
            // Check Header
            if (wave_config->CheckMode & 2) {
                if( DATA_TYPE(vme->vbuf[pnt]) != GLOB_HEADER ) {
                    printf("Invalid Header (%08X)! Readout aborted\n", vme->vbuf[pnt]);
                    quit = 1;
                    break;
                }
            }
            tottrg++;
            event_info->EventSize = NUM_WORD(vme->vbuf[pnt++]); // Total Number of Words
            if ((wave_config->CheckMode & 4) && (event_info->EventSize != board_config->ExpectedEvSize)) {
                printf("EventSize=%x; ExpectEvSize=%x\n",event_info->EventSize,board_config->ExpectedEvSize);
                printf("Unexpected Event Size! Readout aborted\n");
                quit = 1;
                break;
            }
            event_info->PartChannelMask = CH_MASK(vme->vbuf[pnt]); // Partecipating Channels
            event_info->Pattern         = PATTERN_MASK(vme->vbuf[pnt++]);
            event_info->EventCounter    = EV_NUM(vme->vbuf[pnt++]);  // Event Counter
            if ((wave_config->CheckMode & 8) && (event_info->EventCounter != EvCnt)) {
                printf("Not Consecutive Event Counter! Readout aborted\n");
                quit = 1;
                break;
            }
            EvCnt = (EvCnt + 1) & 0x00FFFFFF;
            event_info->TriggerTimeTag = vme->vbuf[pnt++];  // Trigger Time Tag

            /* Now pnt points to the first data word in event */

            /* Che if write data to file if requested*/
            if ((wave_config->WriteToFile & 0x2) || ((wave_config->WriteToFile == 1) && wave_config->WriteOneEvent)) {
                wave_config->WriteOneEvent = 0;

                if(wave_config->WriteToFile == 1)
                    printf("Dumping current event to file...\n");

                /* Loop through all channels (groups)
                ** If data are present, they are saved to file
                */
                for(ch=0; ch<8; ch++) {

                    /* Create a file for each enabled channel (and subchannel if V1740 is the target board)*/
                    if ((event_info->PartChannelMask >> ch) & 1) {

                        if (board_config->BoardType == V1740) { 
                            for(i=0; i < 8; i++) {
                                sprintf(OutFileName, "%s%d.txt", OUTFILENAME, ch*8+i);
                                wave_config->ofile[ch][i] = fopen(OutFileName, (wave_config->AppendMode ? "at" : "wt") ); 
                            }
                        }
                        else {
                            i = 0; /* only one channel per channel group */ 
                            sprintf(OutFileName, "%s%d.txt", OUTFILENAME, ch);
                            wave_config->ofile[ch][i] = fopen(OutFileName, (wave_config->AppendMode ? "at" : "wt") ); 

                        }

                    }

                    /* Save data to file if channel data are present in the current event */
                    if ((event_info->PartChannelMask >> ch) & 1) { // HACK Stesso controllo di prima -> fare un loop unico!

                        if (board_config->BoardType == V1740) { 
                            for(i=0; i < 8; i++) { /* Loop through all 8 channels of a V1740 group */
                                PrintChannelHeaderInfo(ch, i, wave_config, event_info);
                            }
                        }
                        else
                            PrintChannelHeaderInfo(ch, 0, wave_config, event_info);

                        /* Write Waveform Data Stream */
                        if (board_config->ZeroAlg != 2) {
                            nw=WaveBuild(board_config, event_info, vme->vbuf+pnt, event_info->chbuf[ch]);
                            pnt += nw; // HACK: To be checked
                        } else {
                            nw=WaveUncompress(board_config, vme->vbuf+pnt, event_info->chbuf[ch]);
                            pnt += vme->vbuf[pnt];
                        }


                        if (board_config->BoardType == V1724) {

                            for(i=0; i<nw; i++) {
                                fprintf(wave_config->ofile[ch][0], "%d\n", event_info->chbuf[ch][i] & 0x3FFF);
                                fprintf(wave_config->ofile[ch][0], "%d\n", (event_info->chbuf[ch][i]>>16) & 0x3FFF);
                            }
                        }
                        else if ((board_config->BoardType == V1721) || (board_config->BoardType == V1731)) {
                            for(i=0; i<nw; i++) {
                                fprintf(wave_config->ofile[ch][0], "%d\n", event_info->chbuf[ch][i] & 0xFF);
                                fprintf(wave_config->ofile[ch][0], "%d\n", (event_info->chbuf[ch][i]>>8)  & 0xFF);
                                fprintf(wave_config->ofile[ch][0], "%d\n", (event_info->chbuf[ch][i]>>16) & 0xFF);
                                fprintf(wave_config->ofile[ch][0], "%d\n", (event_info->chbuf[ch][i]>>24) & 0xFF);
                            }
                        }
                        else if (board_config->BoardType == V1720) {
                            for(i=0; i<nw; i++) {
                                fprintf(wave_config->ofile[ch][0], "%d\n", event_info->chbuf[ch][i] & 0xFFF);
                                if (board_config->Pack25 == 0) 
                                  fprintf(wave_config->ofile[ch][0], "%d\n", (event_info->chbuf[ch][i]>>16)  & 0xFFF);
                            }
                        }
                        else if (board_config->BoardType == V1740) { 
                            for(j=0; j < 8; j++)
                                for(i=0; i<(event_info->ChSamples/8); i++)  
                                    fprintf(wave_config->ofile[ch][j], "%d\n", event_info->chbuf[ch][(j*(event_info->ChSamples/8))+i] & 0xFFF);
                            }
                        }
                    else
                            /* Channel group is enabled but has been suppressed */
                            if (((board_config->ChannelEnableMask >> ch) & 1) && (wave_config->WriteToFile == 1)) 
                                printf("[NOTE]Empty event from channel %d\n", ch);

                        /* Close files opened for current channel loop */
                        if ((event_info->PartChannelMask >> ch) & 1)  {
                            if (board_config->BoardType == V1740)  
                                for(i=0; i<8;i++)
                                  fclose(wave_config->ofile[ch][i]);
                            else
                                  fclose(wave_config->ofile[ch][0]);
                                                }

                    }
            }       
            else {  
                pnt += (event_info->EventSize-4);  /* Move pointer (skip all waveform data) */
            } 
        } 
    } 
}

/*! \fn      void WaveClose(VME_ACCESS* vme, WAVE_CONFIG* wave_config, EVENT_INFO* event_info);
*   \brief   WaveDump closing and resource unallocation. 
*            
*   \param   vme         Pointer to VME access configuration.
*   \param   wave_config Pointer to waveform configuration.
*   \param   event_info  Pointer to event informations.
*/
void WaveClose(VME_ACCESS* vme, WAVE_CONFIG* wave_config, EVENT_INFO* event_info) {

    int i;

    
    /* Start Acquisition */
    vme->data &= 0xFFFFFFFB;
    VME_WRITE32(vme->handle, vme->BaseAddress + AcquisitionControlReg, &vme->data);

    /* Close gnuplot pipe */
    if (wave_config->gnuplot != NULL)
        pclose(wave_config->gnuplot);

    /* Free allocated buffers */
    if(!(vme->vbuf))             free(vme->vbuf);

    for(i=0; i < 8; i++) 
      if(!event_info->chbuf[i]) free(event_info->chbuf);

    /* Free VME board_info.handle */
    if (vme->handle != -1)
        VME_END(vme->handle);

}
