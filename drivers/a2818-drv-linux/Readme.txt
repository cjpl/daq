
        ----------------------------------------------------------------------

                    --- CAEN SpA - Computing Systems Division --- 

        ----------------------------------------------------------------------
        
        A2818 Driver Readme file
        
        ----------------------------------------------------------------------

        Package for Linux kernels 2.4, 2.6

        June 2009


 The complete documentation can be found in the User's Manual on CAEN's web
 site at: http://www.caen.it.


 Content
 -------

 Readme.txt       : This file.

 ReleaseNotes.txt : Release Notes of the last software release.

 a2818.c          : The source file of the driver

 a2818.h          : The header file of the driver

 Makefile         : The Makefile to compile the driver

 a2818_load.2.x   : The script to load the driver


 System Requirements
 -------------------

 - CAEN A2818 PCI CARD
 - Linux kernel Rel. 2.4 or 2.6 with gnu C/C++ compiler


 Installation notes
 ------------------

  To install the A2818 device driver:

  - Excecute: cp ./Makefile.2.4 Makefile (for 2.4 kernel) cp ./Makefile.2.6 Makefile (for 2.6 kernel)

  - Execute: make

  - Execute: sh a2818_load.2.4 (for 2.4 kernel) 
             or sh a2818_load.2.6 (for 2.6 kernel)
