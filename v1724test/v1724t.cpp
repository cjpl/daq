/********************************************************************\
 *  v1729t.cpp  
 *  Purpose: a general V1724 testing code using MIDAS mvmestd.h
 *  Dependences:
 *    1. MIDAS
 *    2. ROOT
 *    3. ROOTANA
 *  Log:
 *    2010-08-02    Created by Exaos Lee <Exaos.Lee(at)gmail.com>
 *
\********************************************************************/

#define __CODEVER__   "0.1.200912"

#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <getopt.h>

#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;
using std::cerr;

#define __OUTFILE__ "v1724t.root"
#define __PEDFILE__ "v1724.ped"

//********** for Hardware ***********************
// They are pure C routines
#ifdef __cplusplus
extern "C" {
#include "midas.h"
#include "mvmestd.h"
#include "cv1724.h"
}
#endif

//********** for ROOT and rootana **************
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"

//*********** for signal processing *************
#include <csignal>

int sig=0;
void sig_handler(int si)
{  sig = 1;  }

//************* CLI options ********************
static char* pname;
static const char* optStr="o:vVa:T:p:c:RFh?";
static const struct option opts[] = {
  {"output",  required_argument,NULL,'o'},
  {"verbose", no_argument,NULL,'v'},
  {"version", no_argument,NULL,'V'},
  {"address", required_argument,NULL,'a'},
  {"trigger", required_argument,NULL,'T'},
  {"pilot",   required_argument,NULL,'p'},
  {"vrange",  required_argument,NULL,0},
  {"channels",required_argument,NULL,'c'},
  {"pre",     required_argument,NULL,0},
  {"post",    required_argument,NULL,0},
  {"postlatency",required_argument,NULL,},
  {"nbofcols",   required_argument,NULL,},
  {"pedfile", required_argument,NULL,0},
  {"falling", no_argument,NULL,'F'},
  {"raw",     no_argument,NULL,'R'},
  {"fastread",no_argument,NULL,0},
  {"vme",     required_argument,NULL,0},
  {"help",    no_argument,NULL,'h'},
};

enum TrigSource {SIG=1, EXT, AUTO, NORM, RAND};
enum PilotClock {P50M=16, P100M, PEXT};
enum ProgMode   {C_PED=32, ACQ, TEST, RESET, UNKNOWN};
struct gArgs_t {
  char* output;  // output file
  bool  isVerbose; // Verbosely?
  const char* version;

  u_int32_t  addr;  // Module's base address

  TrigSource trig;  // Trigger source, Default is SIG
  PilotClock pilot; // Pilot clock, Default is I1G

  int   vrange;  // Voltage range
  int   ch_mask; // Channel mask, --channels
  int   pre;     // pre-trig
  int   post;    // post-trig
  int   postlat; // POST_STOP_LATENCY
  int   nbofcols;// NB_OF_COLS_READ
  char* pedfile; // pedestal file name

  bool  isEdgeF; // trigger at falling edge?
  bool  isRaw;   // record data without calibration?
  bool  isFastR; // Fast read mode or not?
  bool  isStopR; // Sequence from the STOP (fast) or 1st column?

  int   vmeidx;  // VME interface index (default = 0)

  ProgMode mode; // Default is TEST
} gArgs;

void usage()
{
  cout << endl << "Usage: " << pname << " [Options] [mode]" << endl << endl;
  cout <<"General Options:" << endl
       << "   --output <fn>" << endl
       << "   -o <fn>        Output filename, default is \"v1724t.root\"" << endl
       << "   --verbose, -v  Verbose output" << endl
       << "   --version, -V  Display version info and exit" << endl
       << "   --help, -h     Help, show this usage" << endl << endl;
  cout << "Options for the module:" << endl
       << "   --address,-a <base>" << endl
       << "                  Module base address in hexidemal (0xAAAABBBB)"
       << endl
       << "   --trigger,-T <trig>" << endl
       << "     <trig>       Trigger source" << endl
       << "       = sig      Trigger on signal (default)" << endl
       << "       = ext      External trigger" << endl
       << "       = auto     Automatic trigger mode" << endl
       << "       = norm     Logic OR between signal and automatic trigger"
       << endl << "       = rand     Internal random trigger" << endl << endl
       << "   --pilot,-p <clk>" << endl
       << "     <clk>        Setup the pilot clock as" << endl
       << "       = 50M       Fp = 50 MHz" << endl
       << "       = 100M      Fp = 100 MHz" << endl
       << "       = EXT       Fp = External"  << endl << endl
       << "   --falling,-F   Using falling edge to trigger, default is rising"
       << endl
       << "   --channels,-c <CHNs>" << endl
       << "     <CHNs>       Using channels in the <CHNs> seperated by \",\""
       << endl
       << "   --vrange <r>   Voltage range (0x000-0xFFF)" << endl
       << "   --pre <pt>     Pre-trig number (1~65535)" << endl
       << "   --post <pt>    Post-trig number (1~65535)" << endl
       << "   --postlatency <postlat>" << endl
       << "                  Setup the POST_STOP_LATENCY as <postlat>" << endl
       << "   --nbofcols <nb>" << endl
       << "                  Setup NB_OF_COLS_READ as <nb>" << endl
       << "   --pedfile <fn> File containing pedestrals for correction"  << endl
       << "   --raw          Only record raw data, default data is corrected"
       << endl
       << "   --fastread     Use fast read mode (sequence without TRIG_REC)"
       << endl
       << "   --stopread     Use fast read mode (sequence from STOP)" << endl
       << "   --vme <idx>    VME interface index (default = 0)" << endl
       << endl;
  cout << "Modes:" << endl
       << "  c_ped  ----  Pedestal calibration" << endl
       << "  acq    ----  Acquisition mode" << endl
       << "  test   ----  Testing mode" << endl
       << "  reset  ----  Reset the module" << endl
       << endl;
}

void parsing_opts(int argc, char** argv)
{
  int opt=0, longIdx=0;
  int mask = 0;

  //********* initialize global arguments *************
  pname = argv[0];

  gArgs.output    = NULL;
  gArgs.isVerbose = false;
  gArgs.version   = __CODEVER__;

  gArgs.addr     = 0x17240000; // Default address

  gArgs.trig     = AUTO;  // Auto trig as default
  gArgs.pilot    = P100M; // Default: Fp = 100 MHz, Fe = 2GHz

  gArgs.vrange   = 0;     // Default volt: -0.5V
  gArgs.ch_mask  = 0xF;   // Default all 4 channels is enabled
  gArgs.pre      = 15000; // Default: PreTrig = 150 \mu s * Fp (in MHz)
  gArgs.post     = 64;    // Default: 64 means trigger in the middle of window
  gArgs.postlat  = 4;     // Default: 4*2.5 \mu s = 10 \mu s
  gArgs.nbofcols = 128;   // Default read all the matrix
  gArgs.pedfile  = NULL;

  gArgs.isEdgeF = false; // Default: using the trigger's rising edge
  gArgs.isRaw   = false; // Default: data is calibrated
  gArgs.isFastR = false; // Fast read mode?
  gArgs.isStopR = false; // Default read from the 1st column (normal)

  gArgs.vmeidx  = 0;
  gArgs.mode    = UNKNOWN; // Default in UNKNOWN mode

  //************ parsing options *************************
  do {
    opt = getopt_long(argc, argv, optStr, opts, &longIdx);
    switch( opt ) {
    case 'o': gArgs.output = optarg; break; // --output <fn>
    case 'v': gArgs.isVerbose = true; break; // --verbose
    case 'V': // --version
      cout << gArgs.version << endl;
      exit(EXIT_SUCCESS);
      break;
    case 'a': gArgs.addr = strtoul(optarg, NULL, 16); break; // --address
    case 'T': // --trigger <trig>
      if( strcasecmp(optarg, "SIG") == 0 )
	gArgs.trig = SIG;
      else if( strcasecmp(optarg, "EXT") == 0 )
	gArgs.trig = EXT;
      else if( strcasecmp(optarg, "AUTO") == 0 )
	gArgs.trig = AUTO;
      else if( strcasecmp(optarg, "NORM") == 0 )
	gArgs.trig = NORM;
      else if( strcasecmp(optarg, "RAND") == 0 )
	gArgs.trig = RAND;
      else {
	cerr << "** ERROR: Unkown trigger mode <" << optarg << ">" << endl
	     << "** Using auto as default." << endl;
	gArgs.trig = AUTO;
      }
      break;
    case 'p': // --pilot <clk>
      if( strcasecmp(optarg, "50M") == 0 )
	gArgs.pilot = P50M;
      else if( strcasecmp(optarg, "100M") == 0 )
	gArgs.pilot = P100M;
      else if( strcasecmp(optarg, "EXT") == 0 )
	gArgs.pilot = PEXT;
      else {
	cerr << "** ERROR: Unkown pilot clock <" << optarg << ">" << endl
	     << "** Using default pilot clock: Fp = 100 MHz" << endl;
	gArgs.pilot = P100M;
      }
      break;
    case 'c': // --channels
      if(index(optarg, '1'))  mask = mask | 0x1;
      if(index(optarg, '2'))  mask = mask | 0x2;
      if(index(optarg, '3'))  mask = mask | 0x4;
      if(index(optarg, '4'))  mask = mask | 0x8;
      gArgs.ch_mask = mask;
      break;
    case 'R': gArgs.isRaw = true; break; // --raw
    case 'F': gArgs.isEdgeF = true; break; // --falling, -F
    case 'h': // --help, -h, -?
    case '?': usage();  break;
    case 0:
      if( strcmp("vrange", opts[longIdx].name ) == 0 ) { // --pre=<pt>
	gArgs.vrange = atoi(optarg);
      } else if( strcmp("pre", opts[longIdx].name ) == 0 ) { // --pre=<pt>
	gArgs.pre  = atoi(optarg);
      } else if ( strcmp("post", opts[longIdx].name) == 0 ) { // --post=<pt>
	gArgs.post = atoi(optarg);
      } else if ( strcmp("postlatency", opts[longIdx].name) == 0 ) {
	gArgs.postlat = atoi(optarg);
      } else if ( strcmp("nbofcols", opts[longIdx].name) == 0 ) {
	gArgs.nbofcols = atoi(optarg);
      } else if (strcmp("pedfile", opts[longIdx].name)==0) { //--pedfile=<fn>
	gArgs.pedfile = optarg;
      } else if ( strcmp("vme", opts[longIdx].name) == 0 ) {
	gArgs.vmeidx = atoi(optarg);
      } else if ( strcmp("fastread", opts[longIdx].name) == 0 ) {
	gArgs.isFastR = true;
      } else if ( strcmp("stopread", opts[longIdx].name) == 0 ) {
	gArgs.isStopR = true;
      }
      break;
    default: break;
    }
  } while( opt != -1 );

  //******** Check the running mode *******************
  if( (argc - optind) == 1 ) {
    if( strcasecmp(argv[optind], "C_PED") == 0 ) {
      gArgs.mode = C_PED;
      gArgs.trig  = AUTO; 
      gArgs.ch_mask = 0xF;
      gArgs.isRaw = true;
    } else if( strcasecmp(argv[optind], "ACQ") == 0 ) {
      gArgs.mode = ACQ;
    } else if( strcasecmp(argv[optind], "TEST") == 0 ) {
      gArgs.mode = TEST;
    } else if( strcasecmp(argv[optind], "RESET") == 0 ) {
      gArgs.mode = RESET;
    } else {
      cerr << "** ERROR: Unkown mode <" << argv[optind] <<">" << endl;
      gArgs.mode = UNKNOWN;
    }
  }
}

//*** Print options ***********************************
void print_opts()
{
  cout.setf (cout.hex , cout.basefield);
  cout.setf (cout.showbase);

  cout << "============ Settings for " << pname << " ===============" << endl
       << "      Base Address: " << gArgs.addr << endl
       << "    Trigger Source: ";
  switch(gArgs.trig) {
  case SIG:  cout << "On Signal" << endl;  break;
  case EXT:  cout << "External"  << endl;  break;
  case AUTO: cout << "Automatic" << endl;  break;
  case NORM: cout << "Normal"    << endl;  break;
  default:   cout << "Unknown"   << endl;  break;
  }
  cout << "       Pilot clock: ";
  switch(gArgs.pilot) {
  case P50M:  cout << "Fp = 50 MHz (Fe = 1 GHz)"   << endl;  break;
  case P100M: cout << "Fp = 100 MHz (Fe = 2 GHz)"  << endl;  break;
  case PEXT:  cout << "Fp = External (Fe = 20*Fp)" << endl;  break;
  default:    cout << "Unknown" << endl; break;
  }
  cout << "   Opened channels:";
  if(gArgs.ch_mask & 0x1) cout << " 1";
  if(gArgs.ch_mask & 0x2) cout << " 2";
  if(gArgs.ch_mask & 0x4) cout << " 3";
  if(gArgs.ch_mask & 0x8) cout << " 4";
  cout << " (channel mask = "  << gArgs.ch_mask << ")" << endl;
  cout << "           PRETRIG: " << gArgs.pre << endl;
  cout << "          POSTTRIG: " << gArgs.post << endl;
  cout << " POST_STOP_LATENCY: " << gArgs.postlat << endl;
  cout << "   NB_OF_COLS_READ: " << gArgs.nbofcols << endl;
  cout << "     Pedestal File: ";
  if(gArgs.pedfile)  cout << gArgs.pedfile << endl;
  else  cout << "Not set" << endl;
  cout << "      Trigger edge: ";
  if(gArgs.isEdgeF) cout << "Falling";
  else cout << "Rising";
  cout << endl;
  cout << "   Data calibrated: ";
  if(gArgs.isRaw) cout << "No";
  else cout << "Yes";
  cout << endl;

  cout << "     Sequence read: ";
  if(gArgs.isFastR) cout << "without TRIG_REC (fast)" << endl;
  else cout << "with TRIG_REC (normal)" << endl;
  cout << "    Sequence start: ";
  if(gArgs.isStopR) cout << "from STOP (fast)" << endl;
  else cout << "from the 1st column (normal)" << endl;
}

//************** Apply options  ********************************
void apply_v1724_opts(MVME_INTERFACE *mvme)
{
  int       status;
  u_int32_t par, tmp;

  cv1724_Reset(mvme, gArgs.addr);

  cout.setf (cout.hex , cout.basefield);
  cout.setf (cout.showbase);
  if(gArgs.isVerbose) {
    cout << endl
	 << "============== CAEN V1724 Sampling ADC ==============" << endl
	 << "Setup registers ...." << endl << endl;
  }

  // Trigger type: source + edge
  par = 0;
  switch(gArgs.trig) {
  case SIG:  par = 0x1; break;
  case EXT:  par = 0x2; break;
  case AUTO: par = 0x0; break;
  case NORM: par = 0x3; break;
  case RAND: par = 0x3 + 0x8 + 0x10; break;
  default: break;
  }
  if(gArgs.isEdgeF) par += 0x4;
  if(gArgs.isVerbose) {
    cv1724_GetTrigType(mvme, gArgs.addr, &tmp);
    cout << "  Old trigger type: " << tmp << endl
	 << "  New trigger type: " << par << endl;
  }
  cv1724_SetTrigType(mvme, gArgs.addr, par);

  // pilot
  par = 0;
  switch(gArgs.pilot) {
  case P50M:  par = 0x1; break;
  case P100M: par = 0x2; break;
  case PEXT:
  default: break;
  }
  if(gArgs.isVerbose) {
    cv1724_GetPilot(mvme, gArgs.addr, &tmp);
    cout << "  Old pilot reg: " << tmp << endl
	 << "  New pilot reg: " << par << endl;
  }
  cv1724_SetPilot(mvme, gArgs.addr, par); // ?? (on-board S7)

  // Channel mask
  if(gArgs.isVerbose) {
    cv1724_GetChannelMask(mvme, gArgs.addr, &tmp);
    cout << "  Old channel mask: " << tmp << endl
	 << "  New channel mask: " << gArgs.ch_mask << endl;
  }
  cv1724_SetChannelMask(mvme, gArgs.addr, gArgs.ch_mask);

  // Voltage range
  if(gArgs.isVerbose) {
    cv1724_GetVoltRange(mvme, gArgs.addr, &tmp);
    cout << "  Old volt range: " << tmp << endl
	 << "  New volt range: " << gArgs.vrange << endl;
  }
  cv1724_SetVoltRange(mvme, gArgs.addr, gArgs.vrange);

  // Values: PRE_TRIG, POST_TRIG, POST_STOP_LATENCY, NB_OF_COLS_READ
  if(gArgs.nbofcols > 128 || gArgs.nbofcols <0) gArgs.nbofcols = 128;
  if(gArgs.isVerbose) {
    cv1724_GetPreTrig(mvme, gArgs.addr, &tmp);
    cout << "  Old PRE_TRIG: " << tmp << endl
	 << "  New PRE_TRIG: " << gArgs.pre << endl;
    cv1724_GetPostTrig(mvme, gArgs.addr, &tmp);
    cout << "  Old POST_TRIG: " << tmp << endl
	 << "  New POST_TRIG: " << gArgs.post << endl;
    cv1724_GetPostStopLatency(mvme, gArgs.addr, &tmp);
    cout << "  Old POST_STOP_LATENCY: " << tmp << endl
	 << "  New POST_STOP_LATENCY: " << gArgs.postlat << endl;
    cv1724_GetNBofColsRead(mvme, gArgs.addr, &tmp);
    cout << "  Old NB_OF_COLS_READ: " << tmp << endl
	 << "  New NB_OF_COLS_READ: " << gArgs.nbofcols << endl;
  }
  cv1724_SetPreTrig(mvme, gArgs.addr, gArgs.pre);
  cv1724_SetPostTrig(mvme, gArgs.addr, gArgs.post);
  cv1724_SetPostStopLatency(mvme, gArgs.addr, gArgs.postlat);
  cv1724_SetNBofColsRead(mvme, gArgs.addr, gArgs.nbofcols);

  // Fast read mode & TRIG_REC?
  par = 0x0;
  if(gArgs.isFastR) par = par | 0x1;
  if(gArgs.isStopR) par = par | 0x2;
  if(gArgs.isVerbose) {
    cv1724_GetFastReadModes(mvme, gArgs.addr, &tmp);
    cout << "  Old READ_FAST_MODES: " << tmp << endl;
    if(tmp & 0x1)
      cout << "\tSequence without reading TRIG_REC (fast)" << endl;
    else
      cout << "\tSequence with reading TRIG_REC (normal)" << endl;
    cout << "\tSequence departing from ";
    if(tmp & 0x2)
      cout << "the first column (normal)" << endl;
    else
      cout << "STOP (fast)" << endl;
    cout << "  NEW READ_FAST_MODES: " << par << endl;
  }
  cv1724_SetFastReadModes(mvme, gArgs.addr, par);

  // Init
  cv1724_Init(mvme, gArgs.addr);
}

//************** Pedestals calibration  ************************

TTree* get_pedcali_tree(MVME_INTERFACE *mvme, const int maxloop=10)
{
  int i,j,k, dat[4][CV1724_MAX_CHN_SIZE];
  TTree* cali = new TTree("cali","Calibration data");
  cali->Branch("ch0", dat[0], "ch0[2560]/I");
  cali->Branch("ch1", dat[1], "ch1[2560]/I");
  cali->Branch("ch2", dat[2], "ch2[2560]/I");
  cali->Branch("ch3", dat[3], "ch3[2560]/I");

  cout << "Waiting for internal soft trigger ..." << endl;
  cout.setf (cout.dec , cout.basefield);
  i = 0;
  cv1724_Start(mvme, gArgs.addr);
  usleep(2000);
  do {
    do {
      usleep(100);
    } while(!cv1724_IsDataReady(mvme, gArgs.addr));

    for(j=0; j<4; j++)
      for(k=0; k<CV1724_MAX_CHN_SIZE; k++)
	dat[j][k] = 0;

    // cv1724_GetRawData(mvme, gArgs.addr, rdat, &nch);
    cv1724_GetRawData(mvme, gArgs.addr);
    for(j=0; j<4; j++) {
      if(cv1724_ch[j]>0) {
	for(k=0; k<CV1724_MAX_CHN_SIZE; k++)
	  dat[j][k] = cv1724_rawdata[j][k];
      }
    }
    if(cv1724_nch>0) cali->Fill();

    cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
	 << "Event: " << std::fixed << std::setw(10) << ++i;
  } while( i<maxloop );
  cout << endl;

  return cali;
}

void cv1724_pedcali(MVME_INTERFACE *mvme)
{
  gArgs.trig = AUTO;
  gArgs.isRaw   = true;
  gArgs.isFastR = true;
  gArgs.isVerbose = true;
  gArgs.vrange = 0x1;
  gArgs.nbofcols = 0x0; // Read full matrix

  // Set FAST read mode for V1724
  cv1724_SetFastReadModes(mvme, gArgs.addr, 0x1);

  // Initializing ROOT file for event recording
  if(gArgs.output == NULL) gArgs.output = "v1724cali.root";

  TFile *fcali = new TFile(gArgs.output, "RECREATE");
  TTree *cali;

  // Get data at Fe=1GHz
  gArgs.pilot = P50M;
  apply_v1724_opts(mvme);
  // print_hw_info(mvme);
  cv1724_Status(mvme, gArgs.addr);
  cali = get_pedcali_tree(mvme, 20);
  cali->SetName("pcali_1g");
  cali->Write();
  delete cali;

  // Get data at Fe=2GHz
  gArgs.pilot = P100M;
  apply_v1724_opts(mvme);
  // print_hw_info(mvme);
  cv1724_Status(mvme, gArgs.addr);
  cali = get_pedcali_tree(mvme, 20);
  cali->SetName("pcali_2g");
  cali->Write();
  delete cali;

  fcali->Close();
}

//************** Main procedure  *******************************
int main(int argc, char** argv)
{
  MVME_INTERFACE *pvme;
  int       status;
  u_int32_t par;

  //********** Processing CLI parameters ***********************
  parsing_opts(argc, argv);
  if( gArgs.mode == UNKNOWN ) {
    usage();
    return EXIT_SUCCESS;
  }

  // Verbosing
  if(gArgs.isVerbose || gArgs.mode == TEST) print_opts();

  //***************************************************
  // Assigne singal handler
  signal(SIGQUIT, sig_handler);

  // Initialize VME bus
  status = mvme_open( &pvme, 0 );
  if( status != MVME_SUCCESS ) {
    cerr << "[FE]: ** ERROR: Failed to open the 1st VME interface! **" << endl;
    return EXIT_FAILURE;
  } else
    cout << "[FE]: The 1st VME interface opened successfully." << endl;

  cout << "[FE]: Resetting the VME bus ... ";
  status = mvme_sysreset(pvme);
  if( status != MVME_SUCCESS ) {
    cout << " FAILED! [*ERROR*]" << endl;
    cerr << "[FE]: ** ERROR: VME bus cannot be initialized!" << endl;
    return EXIT_FAILURE;
  } else {
    cout << "SUCCESS!" << endl;
  }
 
  //***************************************************
  // Setup pamaraters for V1724
  //***************************************************
  if( gArgs.mode == C_PED ) {
    cv1724_pedcali(pvme);
  } else if( gArgs.mode == RESET ) {
    cv1724_Reset(pvme, gArgs.addr);
  } else if( gArgs.mode == TEST ) {
    apply_v1724_opts(pvme);
    cv1724_Status(pvme, gArgs.addr);
  }
  if(gArgs.mode != ACQ ) return EXIT_SUCCESS;

  // Initlizing before ACQ
  apply_v1724_opts(pvme);

  int i,j,k;
  char stmp1[128],stmp2[128];
  int chlen = gArgs.nbofcols * 20;
  int dat[4][CV1724_MAX_CHN_SIZE];

  if(gArgs.output == NULL) gArgs.output = __OUTFILE__;
  TFile *f = new TFile(gArgs.output, "RECREATE");
  TTree *T = new TTree("t_v1724", "V1724 data");
  T->Branch("chlen", &chlen, "chlen/I");
  for(i=0; i<4; i++) {
    if(cv1724_ch[i]>0) {
      sprintf(stmp1,"ch%d",i);
      sprintf(stmp2,"ch%d[chlen]/I");
      T->Branch(stmp1, dat[i], stmp2);
    }
  }

  cv1724_Start(pvme, gArgs.addr);
  do {
    do {
      usleep(100);
    } while(!cv1724_IsDataReady(pvme, gArgs.addr));

    for(j=0; j<4; j++)
      if(cv1724_ch[j]>0)
	for(k<0; k<chlen; k++)
	  dat[j][k] = 0;

    cv1724_GetRawData(pvme, gArgs.addr);

    for(j=0; j<4; j++)
      if(cv1724_ch[j]>0)
	for(k=0; k<chlen; k++)
	  dat[j][k] = cv1724_rawdata[j][k];

    T->Fill();    
  } while(cv1724_nch>0 && !sig);

  T->Write();
  delete T;
  f->Close();
  delete f;

  return EXIT_SUCCESS;
}

