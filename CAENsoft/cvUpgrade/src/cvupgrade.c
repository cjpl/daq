/************************************************************************

 CAEN SpA - Viareggio
 www.caen.it

 Program: cvupgrade
 Date:    15/02/2010
 Author:  CAEN Computing Division (support.computing@caen.it)

 ------------------------------------------------------------------------
 Description
 ------------------------------------------------------------------------
 This program allows to upgrade the firmware of any CAEN module that
 supports the firmware download through the USB/CONET cable.(except for the
 bridges V1718 and V2718 that use the program 'CAENVMEUpgrade').
 The firmware upgrade consists in writing the configuration file (Altera
 Raw Binary Format) into the flash memory of the board. Usually the flash
 memory contains two images of the configuration file, called 'standard'
 and 'backup'. The 'standard' image only will normally be overwritten ; 
 the new firmware will be loaded by the FPGA after the first power cycle.
 If an error occurs while upgrading, it may happen that the FPGA is not
 properly configured and the board does not respond to the VME access.
 In this case you can restore the old firmware (i.e. the 'backup' image)
 just moving the relevant jumper from STD to BKP and power cycling the
 board. Now you can retry to upgrade the 'standard' image.
 Warning: never upgrade the 'backup' image until you are sure that the
 'standard' works properly.
 This program reads some parameters that define the type of the board
 being upgraded from a file called CVupgrade_params.txt that must be
 in the same directory of CVupgrade. There is one CVupgrade_params file
 for each type of board that can be upgraded.

 ------------------------------------------------------------------------

 ------------------------------------------------------------------------
 Portability
 ------------------------------------------------------------------------
 This program is ANSI C and can be compilated on any platform, except
 for the functions that allow to initialize and access the VME bus.
 For VME Boards, if CAEN's VME bridges (both V1718 and V2718) are used as VME masters,
 a proper installation of the CAENComm library DLL on PC is the only requirement.
 If a different VME Bridge/CPU is used, the CAENComm function calls MUST be reimplemented.

*************************************************************************/
/*------------------------------------------------------------------------
  Modification history:
  ------------------------------------------------------------------------
  Version  | Author | Date       | Changes made
  ------------------------------------------------------------------------
  1.0      | CTIN   | 01.02.2008 | inital version.
  1.1      | LCOL   | 29.05.2008 | Updated WriteFlashPage to account for 
           |        |            | a longer flash erase times 
           |        |            | (increase from 20 to 40 ms).
  1.2      | LCOL   | 25.06.2008 | Added flash enable polarity parameter.         
  1.3      | CTIN   | 26.01.2009 | Extended compatibility to 16, 32 and 64
           |        |            | Mbit flash size;
           |        |            | Replaced fixed wait time (page erasing)
           |        |            | with busy polling;
           |        |            | Add -link and -bdnum command line options
  2.0      | ALUK   | 03.10.2009 | Add the CAENComm as reference library for the 
		    |		 |			 | low level access.
  2.1      | ALUK   | 14.02.2010 | Add Keyinfo and KeyWrite functionalities 
		    |            |            |(only Digitizer boards)
  ------------------------------------------------------------------------*/

#define REVISION  "2.1"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "flash.h"

typedef struct UpgradeParams {
  unsigned int			Type  ;
  unsigned int			Link  ;
  unsigned int			BdNum ;
  unsigned long			BaseAddress;  
  unsigned int			FirstPageStd;         // First page of the Standard image
  unsigned int			FirstPageBck;         // First page of the Backup image
  uint64_t				key; 
  unsigned int			image     ;
  unsigned int			noverify  ;
  unsigned int			verifyonly;
  char					ConfFile[1000];
  char					Models[1000];
} cvUpgradeParams;


static void Usage() {
        printf("Syntax: cvUpgrade ConfFile ConnType[USB|PCI_OPTLINK|PCIE_OPTLINK] [ConfigOptions][DeviceOptions]\n");
		printf("Syntax: cvUpgrade -KeyInfo ConnType[USB|PCI_OPTLINK|PCIE_OPTLINK] [DeviceOptions]\n\n");
        printf("Description: write the file 'ConfFile' (Altera Raw Binary Format) into\n");
        printf("the flash memory of board connected by the specified connection type OR\n");
        printf("retrieve the keyinfo data from the board and store it on KeyInfo.dat file\n\n");
        printf("ConfigOptions:\n\n");
        printf("-backup: write backup image\n\n");
		printf("-key    N: The key value to enable the DPP firmware\n\n"); 
        printf("-no_verify: disable verify (don't read and compare the flash content)\n\n");
        printf("-verify_only: read the flash content and compare with the ConfFile file\n\n");
		printf("DeviceOptions:\n\n");
        printf("-param filename: allow to specify the file that contain the parameters for the\n");
        printf("                 board that is being upgraded (default is cvUpgrade_params.txt)\n\n");
        printf("-link   N: when using CONET, it is the optical link number to be used\n");
		printf("           when using USB, it is the USB device number to be used (default is 0)\n\n");
        printf("-bdnum  N: select board number in a chainable link (V2718 only)\n\n");
		printf("-VMEbaseaddress   N: The base address of the VME slave board to be upgraded (Hex 32 bit)\n");
}


//******************************************************************************
// ReadConfigurationROM
//******************************************************************************
static int ReadConfigurationROM(cvFlashAccess *Flash, CROM_MAP *crom)
{
  unsigned char data[AT45_PAGE_SIZE];
  int res = 0;

  res = ReadFlashPage(Flash, data, ROM_FLASH_PAGE);

  if (res == 0) {
      crom->crom_chksum     = (uint8_t)(data[0]);
      crom->crom_chksum_len = (uint32_t) ((data[1] << 16) | (data[2] << 8) | data[3]);
      crom->crom_const      = (uint32_t) ((data[4] << 16) | (data[5] << 8) | data[6]);
      crom->crom_c_code     = (uint8_t)(data[7]);
      crom->crom_r_code     = (uint8_t)(data[8]);
      crom->crom_OUI        = (uint32_t) ((data[9] << 16) | (data[10] << 8) | data[11]);
      crom->crom_version    = (uint8_t)(data[12]);
      crom->crom_board_id   = (uint32_t) ((data[13] << 16) | (data[14] << 8) | data[15]);
      crom->crom_revision   = (uint32_t) ( (data[16] << 24) | (data[17] << 16) | (data[18] << 8) | data[19]);
      crom->crom_serial     = (uint16_t) ((data[32] << 8) | (data[33]));
      crom->crom_VCXO_type  = (uint8_t)(data[34]);
  }
  return res;
}


//******************************************************************************
// PrintCROM2File
//******************************************************************************
static void PrintCROM2File(FILE *fp, const CROM_MAP *map) {
    fprintf(fp,"Checksum        = %X\n", map->crom_chksum);
    fprintf(fp,"Checksum Length = %X\n", map->crom_chksum_len);
    fprintf(fp,"Constant field  = %X\n", map->crom_const);
    fprintf(fp,"C Code          = %X\n", map->crom_c_code);
    fprintf(fp,"R Code          = %X\n", map->crom_r_code);
    fprintf(fp,"OUI             = %X\n", map->crom_OUI);
    fprintf(fp,"Version         = %X\n", map->crom_version);
    fprintf(fp,"Board ID        = %X\n", map->crom_board_id);
    fprintf(fp,"PCB Revision    = %d\n", map->crom_revision);
    fprintf(fp,"Serial Number   = %d\n", map->crom_serial);
    fprintf(fp,"VCXO Type ID    = %d\n", map->crom_VCXO_type);
}

//******************************************************************************
// KeyInfo
//******************************************************************************
static int KeyInfo(cvUpgradeParams *Config, cvFlashAccess *Flash) {
	int i;
	unsigned char security_vme[AT45_IDREG_LENGTH];
	unsigned long vboard_base_address;
	char KeyFilename[200];
	FILE *cf = NULL;
	int res;

	CROM_MAP rom;

    /* initialize the connection */
    res = CAENComm_OpenDevice(Config->Type, Config->Link, Config->BdNum,  Config->BaseAddress, &Flash->Handle);
    if (res != CAENComm_Success) {
        printf("Cannot open the Device!\n");
        return(CvUpgrade_FileAccessError);
    }
   
	if (ReadFlashSecurityReg(Flash, security_vme)) {
      printf("Error while reading on-board flash memory!\n");
	  return(CvUpgrade_FileAccessError);
	}
	if (ReadConfigurationROM(Flash, &rom)) {
      printf("Error while reading on-board flash memory!\n");
	  return(CvUpgrade_FileAccessError);
	}

    sprintf(KeyFilename, "KeyInfo-%d.dat", rom.crom_serial);
    cf = fopen(KeyFilename,"w");
	if(cf == 0) {
      printf("Cannot open KeyInfo.dat file! Exiting ...\n");
      return(CvUpgrade_FileAccessError);
	}

    printf("Writing output file %s\n", KeyFilename);
	for(i=0; i < 64; i++) {
	  fprintf(cf, "%02X", security_vme[64+i]);
	  fprintf(cf, (i < 63) ? ":" : "\n");
	}
	PrintCROM2File(cf, &rom);

	printf("\n");
	printf("Program exits succcessfully.\n");
    fclose(cf);
	return 0;
}

//******************************************************************************
// cvUpgrade
//******************************************************************************
int cvUpgrade(cvUpgradeParams *Config, cvFlashAccess *Flash)
{
    unsigned int i, j, page, pa,  NumPages, err=0, done;    
    unsigned int CFsize;
    unsigned char c, *CFdata;
    unsigned char pdr[2048];
	unsigned char key_page[8];
    int res;
    int k;

    FILE *cf;

    // ************************************************
    // Open Binary Configuration File
    // ************************************************

    // open and read the configuration file
    cf = fopen(Config->ConfFile,"rb");
    if(cf == 0) {
        printf("Can't open file %s\n",Config->ConfFile);
        Usage();
        return(-3);
    }
    // calculate the size
    fseek (cf, 0, SEEK_END);
    CFsize = ftell (cf);
    fseek (cf, 0, SEEK_SET);
    if ( (CFdata = (unsigned char *)malloc(CFsize)) == NULL ) {
        printf("Can't allocate %d bytes\n",CFsize);
        return(-3);
    }

    for(i=0; i<CFsize; i++) {
        // read one byte from file and mirror it (lsb becomes msb)
        c = (unsigned char)fgetc(cf);
        CFdata[i]=0;
        for(j=0; j<8; j++)
            CFdata[i] |= ( (c >> (7-j)) & 1) << j;
    }
    fclose(cf);

    NumPages = (CFsize % Flash->PageSize == 0) ? (CFsize / Flash->PageSize) : (CFsize / Flash->PageSize) + 1;

    /* initialize the connection */
    res = CAENComm_OpenDevice(Config->Type, Config->Link, Config->BdNum,  Config->BaseAddress, &Flash->Handle);
    if (res != CAENComm_Success) {
        printf("Cannot open the Device!\n");
        return(CvUpgrade_FileAccessError);
    }

    printf("Board Types: %s\n", Config->Models);
    if (Config->image == 0) {
        if(!Config->verifyonly)
          printf("Overwriting Standard image of the firmware with %s\n", Config->ConfFile);
        else
            printf("Verifying Standard image of the firmware with %s\n", Config->ConfFile);
        pa = Config->FirstPageStd;
    }
    else {
        if(!Config->verifyonly)
            printf("Overwriting Backup image of the firmware with %s\n", Config->ConfFile);
        else
            printf("Verifying Backup image of the firmware with %s\n", Config->ConfFile);
        pa = Config->FirstPageBck;
    }

    printf("0%% Done\n");
    done = 10;

    // ************************************************
    // Start for loop
    // ************************************************
    for(page=0; page < NumPages; page++)  {
        if(!Config->verifyonly) {
            // Write Page
            if (WriteFlashPage(Flash, CFdata + page*Flash->PageSize, pa + page) < 0) {
                printf("\nCommunication Error: the board at Base Address %08X does not respond\n", Config->BaseAddress);
                err = 1;
                break;
                }
        }

        if(!Config->noverify) {
            // Read Page
            if (ReadFlashPage(Flash, pdr, pa + page) < 0) {
                printf("\nCommunication Error: the board at Base Address %08X does not respond\n", Config->BaseAddress);
                err = 1;
                break;
            }
            // Verify Page
            for(i=0; (i<Flash->PageSize) && ((page*Flash->PageSize+i) < CFsize); i++)  {
                if(pdr[i] != CFdata[page*Flash->PageSize + i])  {
                    printf("\nFlash verify error (byte %d of page %d)!\n", i, pa + page);
                    if ((Config->image == 0) && !Config->verifyonly)
                        printf("The STD image can be corrupted! \nMove the jumper to Backup image and cycle the power\n");
                    err = 1;
                    break;
                }
            }
        }
        if (err)
            break;

        if (page == (NumPages-1)) {
            printf("100%% Done\n");
        } else if ( page >= (NumPages*done/100) ) {
            printf("%d%% Done\n", done);
            done += 10;
        }
    }  // end of for loop

    if(!err) {
        if (Config->verifyonly) {
            printf("\nFirmware verified successfully. Read %d bytes\n", CFsize);
        } else {
            printf("\nFirmware updated successfully. Written %d bytes\n", CFsize);
            printf("The new firmware will be loaded after a power cycle\n");
        }
    }

    if (CFdata != NULL) 
        free(CFdata);
    if ((Config->key != 0) && 
		( 
          (!strncmp(Config->Models,"DIGITIZERS",1000)) || 
		  (!strncmp(Config->Models,"V1495",1000)) )
        ) {
		memset(key_page, 0, 8);
		for (k = 7; k > -1; k--) {
		  key_page[7-k] = (unsigned char) ((Config->key >> (k * 8)) & 0xFF);
		}	
		WriteFlashPage(Flash, key_page, 2);
	}
    CAENComm_CloseDevice(Flash->Handle);
    Wait_ms(1000);
 
    return err;

}


//******************************************************************************
// MAIN
//******************************************************************************
int main(int argc,char *argv[])
{
    int i, err=0;
	int key = 0;
	int ret;
    cvUpgradeParams Params;
    cvFlashAccess   FlashInfo;
    char ParamFile[1000] = "cvUpgrade_params.txt";

    FILE *bdf;

    printf("\n");
    printf("********************************************************\n");
    printf(" CAEN SpA - Front-End Division                          \n");
    printf("--------------------------------------------------------\n");
    printf(" CAEN Board Firmware Upgrade                              \n");
    printf(" Version %s                                             \n", REVISION);
    printf("********************************************************\n\n");

    // Check command arguments (must be at least 2)
    if (argc < 3)  {
        Usage();
        return(-1);
    }

    // Inizialize defaults 
    FlashInfo.FlashEnable = 0;
    Params.Link = 0;
    Params.BdNum = 0;
    Params.image = 0; // Default = standard
	Params.noverify = 0;
	Params.verifyonly = 0;
    Params.BaseAddress = 0;
    Params.key = 0;

    strstr(Params.ConfFile, "cvUpgrade_params.txt");
	if (strcmp(argv[1],"-KeyInfo") == 0 ) {
		key = 1;
	} else {
      sprintf(Params.ConfFile, argv[1]);
	}
    if (strcmp(argv[2],"USB") == 0) Params.Type = CAENComm_USB;
	if (strcmp(argv[2],"PCI_OPTLINK") == 0) Params.Type = CAENComm_PCI_OpticalLink;
	if (strcmp(argv[2],"PCIE_OPTLINK") == 0) Params.Type = CAENComm_PCIE_OpticalLink;
    for (i=3; i<argc; i++) {
        if ( strcmp(argv[i],"-backup") == 0 )
            Params.image = 1;
        if ( strcmp(argv[i],"-no_verify") == 0 )
            Params.noverify = 1;
        if ( strcmp(argv[i],"-verify_only") == 0 )
            Params.verifyonly = 1;        
        if ( strcmp(argv[i],"-param") == 0 )
            sprintf(ParamFile, "%s", argv[++i]);
		if ( strcmp(argv[i],"-VMEbaseaddress") == 0 )
			sscanf(argv[++i], "%x", &Params.BaseAddress);
		if ( strcmp(argv[i],"-key") == 0 )
			sscanf(argv[++i], "%llx", &Params.key); 
        if ( strcmp(argv[i],"-link") == 0 )
            Params.Link = atoi(argv[++i]);  
        if ( strcmp(argv[i],"-bdnum") == 0 )
            Params.BdNum = atoi(argv[++i]);  
    }

    // open the board descriptor file
    bdf = fopen(ParamFile,"r");
    if(bdf == 0) {
        printf("Can't open file %s\n", ParamFile);
        Usage();
        return (-2); 
    }

    fscanf(bdf, "%s", &Params.Models);
    fscanf(bdf, "%x", &FlashInfo.Sel_Flash);
    fscanf(bdf, "%x", &FlashInfo.RW_Flash);
    fscanf(bdf, "%d", &FlashInfo.RegSize);
    fscanf(bdf, "%d", &FlashInfo.PageSize);
    fscanf(bdf, "%d", &Params.FirstPageStd);
    fscanf(bdf, "%d", &Params.FirstPageBck);
    if (!feof(bdf)) {
        int tmp;
        if (fscanf(bdf, "%d", &tmp) > 0)
            FlashInfo.FlashEnable = tmp;
    }
    fclose(bdf);

    // Call cvUpgrade function
	if (key == 0)
      err = cvUpgrade(&Params, &FlashInfo);
	else {
		if ((!strncmp(Params.Models,"DIGITIZERS",1000)) || 
			(!strncmp(Params.Models,"V1495",1000)) ) {
				err = KeyInfo(&Params, &FlashInfo);
		}
	}
    if (err)
        return (CvUpgrade_GenericError);
    else
        return 0;
}


