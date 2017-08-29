/**
   \file main.c

   \author G. Icking-Konert
   \date 2014-03-14
   \version 0.1
   
   \brief implementation of main routine
   
   this is the main file containing browsing the input parameters,
   calling the import, programming, and check routines.
   
   \note program not yet fully tested!
*/

// include files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

// OS specific: Win32
#if defined(WIN32)
  #include <windows.h>
  #include <malloc.h>

// OS specific: Posix
#elif defined(__APPLE__) || defined(__unix__)
  #define HANDLE  int     // comm port handler is int
  #include <fcntl.h>      // File control definitions
  #include <termios.h>    // Posix terminal control definitions
  #include <getopt.h>
  #include <errno.h>    /* Error number definitions */
  #include <dirent.h>
  #include <sys/ioctl.h>

#else
  #error OS not supported
#endif

#define _MAIN_
  #include "globals.h"
#undef _MAIN_
#include "main.h"
#include "misc.h"
#include "serial_comm.h"
#include "bootloader.h"
#include "hexfile.h"
#include "version.h"


// device dependent flash w/e routines
#include "E_W_ROUTINEs_8K_verL_1.0.h"
#include "E_W_ROUTINEs_32K_ver_1.0.h"
#include "E_W_ROUTINEs_32K_ver_1.2.h"
#include "E_W_ROUTINEs_32K_ver_1.3.h"
#include "E_W_ROUTINEs_32K_ver_1.4.h"
//#include "E_W_ROUTINEs_32K_verL_1.0.h"  // empty
#include "E_W_ROUTINEs_128K_ver_2.0.h"
#include "E_W_ROUTINEs_128K_ver_2.1.h"
#include "E_W_ROUTINEs_128K_ver_2.2.h"
#include "E_W_ROUTINEs_128K_ver_2.4.h"
#include "E_W_ROUTINEs_256K_ver_1.0.h"

// buffer sizes
#define  STRLEN   1000
#define  BUFSIZE  10000000



/**
   \fn int main(int argc, char *argv[])
   
   \brief main routine
   
   \param argc      number of commandline arguments + 1
   \param argv      string array containing commandline arguments (argv[0] contains name of executable)
   
   \return dummy return code (not used)
   
   Main routine for import, programming, and check routines
*/
int main(int argc, char ** argv) {
 
  char      *appname;             // name of application without path
  char      portname[STRLEN];     // name of communication port
  int       baudrate;             // communication baudrate [Baud]
  uint8_t   resetSTM8;            // 0=no reset; 1=HW reset via DTR (RS232/USB) or GPIO18 (Raspi); 2=SW reset by sending 0x55+0xAA
  uint8_t   enableBSL;            // don't enable ROM bootloader after upload (caution!)
  uint8_t   flashErase;           // erase P-flash and D-flash prior to upload
  uint8_t   jumpFlash;            // jump to flash after upload
  uint8_t   verifyUpload;         // verify memory after upload
  uint8_t   pauseOnLaunch;        // prompt for <return> prior to upload
  HANDLE    ptrPort;              // handle to communication port
  char      *ptr=NULL;            // pointer to memory
  int       i, j;                 // generic variables  
  char      buf[1000];            // misc buffer
  //char      Tx[100], Rx[100];     // debug: buffer for tests
  
  // STM8 propoerties
  int       flashsize;            // size of flash (kB) for w/e routines
  uint8_t   versBSL;              // BSL version for w/e routines
  uint8_t   family;               // device family, currently STM8S and STM8L
  
  // for upload to flash
  char      fileIn[STRLEN];       // name of file to upload to STM8
  char      *fileBufIn;           // buffer for hexfiles
  char      *imageIn;             // memory buffer for upload hexfile
  uint32_t  imageInStart;         // starting address of imageIn
  uint32_t  imageInBytes;         // number of bytes in imageIn
  
  // for download from flash
  char      fileOut[STRLEN];      // name of file to download from STM8
  char      *imageOut;            // memory buffer for download hexfile
  uint32_t  imageOutStart;        // starting address of imageOut
  uint32_t  imageOutBytes;        // number of bytes in imageOut

  
  // initialize global variables
  g_verbose     = false;        // verbose output when requested only
  g_pauseOnExit = 0;            // no wait for <return> before terminating
  g_UARTmode    = 0;            // 2-wire interface with UART duplex mode
  
  // initialize default arguments
  portname[0] = '\0';           // no default port name
  baudrate   = 230400;          // default baudrate
  resetSTM8  = 0;               // don't automatically reset STM8
  flashErase = 0;               // erase P-flash and D-flash prior to upload
  jumpFlash  = 1;               // jump to flash after uploade
  pauseOnLaunch = 1;            // prompt for return prior to upload
  enableBSL  = 1;               // enable bootloader after upload
  verifyUpload = 1;             // verify memory content after upload
  fileIn[0] = '\0';             // no default file to upload to flash
  fileOut[0] = '\0';            // no default file to download from flash
  
  // required for strncpy()
  portname[STRLEN-1] = '\0';
  fileIn[STRLEN-1]   = '\0';
  fileOut[STRLEN-1]  = '\0';
    
  // allocate buffers (can't be static for large buffers)
  imageIn   = (char*) malloc(BUFSIZE);
  imageOut  = (char*) malloc(BUFSIZE);
  fileBufIn = (char*) malloc(BUFSIZE);
  if ((!imageIn) || (!imageOut) || (!fileBufIn)) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror: cannot allocate memory buffers, exit!\n\n");
    Exit(1, 1);
  }
  
  
  // reset console color (needs to be called once for Win32)      
  setConsoleColor(PRM_COLOR_DEFAULT);

  ////////
  // parse commandline arguments
  ////////
  for (i=1; i<argc; i++) {
    
    // debug: print argument
    //printf("arg %d: '%s'\n", (int) i, argv[i]);
    
    // name of communication port
    if (!strcmp(argv[i], "-p")) {
      if (i<argc-1)
        strncpy(portname, argv[++i], STRLEN-1);
    }

    // communication baudrate
    else if (!strcmp(argv[i], "-b")) {
      if (i<argc-1)
        sscanf(argv[++i],"%d",&baudrate);
    }
    
    // UART mode: 0=duplex, 1=1-wire reply, 2=2-wire reply (default: duplex)\n");
    else if (!strcmp(argv[i], "-u")) {
      if (i<argc-1) {
        sscanf(argv[++i], "%d", &j);
        g_UARTmode = j;
      }
    }

    // HW reset STM8 via DTR line (RS232/USB) or GPIO18 (Raspi only)
    else if (!strcmp(argv[i], "-R")) {
      if (i<argc-1) {
        sscanf(argv[++i], "%d", &j);
        resetSTM8 = j;
      }
    }    

    // erase P-flash and D-flash prior to upload
    else if (!strcmp(argv[i], "-e")) {
      flashErase = 1;
    }

    // name of file to upload
    else if (!strcmp(argv[i], "-w")) {
      if (i<argc-1)
        strncpy(fileIn, argv[++i], STRLEN-1);
    }

    // don't enable ROM bootloader after upload (caution!)
    else if (!strcmp(argv[i], "-x")) {
      enableBSL = 0;
    }

    // skip verify memory content after upload
    else if (!strcmp(argv[i], "-v")) {
      verifyUpload = 0;
    }
    
    // memory range to read and file to save to
    else if (!strcmp(argv[i], "-r")) {
      if (i<argc-1) {
        sscanf(argv[++i],"%x",&j);
        imageOutStart = j;
      }
      if (i<argc-1) {
        sscanf(argv[++i],"%x",&j);
        imageOutBytes = j - imageOutStart + 1;
      }
      if (i<argc-1)
        strncpy(fileOut, argv[++i], STRLEN-1);
    }
    
    // don't jump to address after upload
    else if (!strcmp(argv[i], "-j")) {
      jumpFlash = 0;
    }

    // don't prompt for <return> prior to upload
    else if (!strcmp(argv[i], "-Q")) {
      pauseOnLaunch = 0;
    }

    // prompt for <return> prior to exit
    else if (!strcmp(argv[i], "-q")) {
      g_pauseOnExit = 1;
    }

    // g_verbose output
    else if (!strcmp(argv[i], "-V")) {
      g_verbose = true;
    }

    // else print list of commandline arguments and language commands
    else {
      if (strrchr(argv[0],'\\'))
        appname = strrchr(argv[0],'\\')+1;         // windows
      else if (strrchr(argv[0],'/'))
        appname = strrchr(argv[0],'/')+1;          // Posix
      else
        appname = argv[0];
      printf("\n");

      printf("usage: %s [-h] [-p port] [-b rate] [-u mode] [-R ch] [-e] [-w infile] [-x] [-v] [-r start stop outfile] [-j] [-Q] [-q] [-V]\n", appname);
      printf("  -h                     print this help\n");
      printf("  -p port                name of communication port (default: list available ports)\n");
      printf("  -b rate                communication baudrate in Baud (default: 230400)\n");
      printf("  -u mode                UART mode: 0=duplex, 1=1-wire reply, 2=2-wire reply (default: duplex)\n");
      #ifdef __ARMEL__
        printf("  -R ch                  reset STM8: 1=DTR line (RS232), 2=send 'Re5eT!' @ 115.2kBaud, 3=GPIO18 pin (Raspi) (default: no reset)\n");
      #else
        printf("  -R ch                  reset STM8: 1=DTR line (RS232), 2=send 'Re5eT!' @ 115.2kBaud (default: no reset)\n");
      #endif
      printf("  -e                     erase P-flash and D-flash prior to upload (default: skip)\n");
      printf("  -w infile              upload s19 or intel-hex file to flash (default: skip)\n");
      printf("    -x                   don't enable ROM bootloader after upload (default: enable)\n");
      printf("    -v                   don't verify code in flash after upload (default: verify)\n");
      printf("  -r start stop outfile  read memory range (in hex) to s19 file or table (default: skip)\n");
      printf("  -j                     don't jump to flash before exit (default: jump to flash)\n");
      printf("  -Q                     don't prompt for <return> prior to bootloader entry (default: prompt)\n");
      printf("  -q                     prompt for <return> prior to exit (default: no prompt)\n");
      printf("  -V                     verbose output\n");
      printf("\n");
      Exit(0, 0);
    }

  } // process commandline arguments
  
  

  ////////
  // print app name & version, and change console title
  ////////
  get_app_name(argv[0], VERSION, buf);
  printf("\n%s\n", buf);
  setConsoleTitle(buf);  
  
  
  ////////
  // if no port name is given, list all available ports and query
  ////////
  if (strlen(portname) == 0) {
    printf("  enter comm port name ( ");
    list_ports();
    printf(" ): ");
    scanf("%s", portname);
    getchar();
  } // if no comm port name


  // If specified import hexfile - do it early here to be able to report file read errors before others
  if (strlen(fileIn) > 0) {
    const char *shortname = strrchr(fileIn, '/');
    if (!shortname)
      shortname = fileIn;

    // convert to memory image, depending on file type
    const char *dot = strrchr (fileIn, '.');
    if (dot && !strcmp(dot, ".s19")) {
      if (g_verbose)
        printf("  load Motorola S-record file '%s' ... ", shortname);
      load_hexfile(fileIn, fileBufIn, BUFSIZE);
      convert_s19(fileBufIn, &imageInStart, &imageInBytes, imageIn);
    }
    else if (dot && (!strcmp(dot, ".hex") || !strcmp(dot, ".ihx"))) {
      if (g_verbose)
        printf("  load Intel hex file '%s' ... ", shortname);
      load_hexfile(fileIn, fileBufIn, BUFSIZE);
      convert_hex(fileBufIn, &imageInStart, &imageInBytes, imageIn);
    }
    else {
      if (g_verbose)
        printf("  load binary file '%s' ... ", shortname);
      load_binfile(fileIn, imageIn, &imageInStart, &imageInBytes, BUFSIZE);
    }
  }


  ////////
  // open port with given properties
  ////////
  if (g_verbose) {
    printf("  open port '%s' with %gkBaud ... ", portname, (float) baudrate / 1000.0);
    fflush(stdout);
  }
  if (g_UARTmode == 0)
    ptrPort = init_port(portname, baudrate, 1000, 8, 2, 1, 0, 0);   // use even parity
  else
    ptrPort = init_port(portname, baudrate, 1000, 8, 0, 1, 0, 0);   // use no parity
  if (g_verbose) {
    printf("ok\n");
    fflush(stdout);
  }
  
  // flush receive buffer
  flush_port(ptrPort);

 
  // debug: communication test (echo+1 test-SW on STM8)
  /*
  printf("open: %d\n", ptrPort);
  for (i=0; i<254; i++) {
    Tx[0] = i;
    send_port(ptrPort, 1, Tx);
    receive_port(ptrPort, 1, Rx);
	printf("%d  %d\n", (int) Tx[0], (int) Rx[0]);
  }
  printf("ok\n");
  Exit(1,0);
  */
  

  ////////
  // reset STM8
  ////////

  // manually put STM8 into bootloader mode
  if (pauseOnLaunch) {
    printf("  activate STM8 bootloader and press <return>");
    fflush(stdout);
    fflush(stdin);
    getchar();
  }

  // HW reset STM8 using DTR line (USB/RS232)
  if (resetSTM8 == 1) {
    printf("  reset via DTR ... ");
    pulse_DTR(ptrPort, 10);
    printf("ok\n");
    SLEEP(5);                       // allow BSL to initialize
  }
  
  // SW reset STM8 via command 'Re5eT!' at 115.2kBaud (requires respective STM8 SW)
  else if (resetSTM8 == 2) {
    set_baudrate(ptrPort, 115200);    // expect STM8 SW to receive at 115.2kBaud
    printf("  reset via UART command ... ");
    sprintf(buf, "Re5eT!");           // reset command (same as in STM8 SW!)
    for (i=0; i<6; i++) {
      send_port(ptrPort, 1, buf+i);   // send reset command bytewise to account for slow handling
      SLEEP(10);
    }
    printf("ok\n");
    set_baudrate(ptrPort, baudrate);  // restore specified baudrate
  }
  
  // HW reset STM8 using GPIO18 pin (only Raspberry Pi!)
  #ifdef __ARMEL__
    else if (resetSTM8 == 3) {
      printf("  reset via GPIO18 ... ");
      pulse_GPIO(18, 10);
      printf("ok\n");
      SLEEP(5);                       // allow BSL to initialize
    }
  #endif // __ARMEL__
  
  

  ////////
  // communicate with STM8 bootloader
  ////////

  // synchronize baudrate
  bsl_sync(ptrPort);
  

  // get bootloader info for selecting RAM w/e routines for flash
  bsl_getInfo(ptrPort, &flashsize, &versBSL, &family);


  // for STM8S and 8kB STM8L upload RAM routines, else skip
  if ((family == STM8S) || (flashsize==8)) {

    // select device dependent flash routines for upload
    if ((flashsize==8) && (versBSL==0x10)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_8K_verL_1_0_s19 \n");
      #endif
      ptr = (char*) STM8_Routines_E_W_ROUTINEs_8K_verL_1_0_s19;
      ptr[STM8_Routines_E_W_ROUTINEs_8K_verL_1_0_s19_len]=0;
    }
    else if ((flashsize==32) && (versBSL==0x10)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_32K_ver_1_0_s19 \n");
      #endif
      ptr = (char*) STM8_Routines_E_W_ROUTINEs_32K_ver_1_0_s19;
      ptr[STM8_Routines_E_W_ROUTINEs_32K_ver_1_0_s19_len]=0;
    }
    else if ((flashsize==32) && (versBSL==0x12)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_32K_ver_1_2_s19 \n");
      #endif
      ptr = (char*) STM8_Routines_E_W_ROUTINEs_32K_ver_1_2_s19;
      ptr[STM8_Routines_E_W_ROUTINEs_32K_ver_1_2_s19_len]=0;
    }
    else if ((flashsize==32) && (versBSL==0x13)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_32K_ver_1_3_s19 \n");
      #endif
      ptr = (char*) STM8_Routines_E_W_ROUTINEs_32K_ver_1_3_s19;
      ptr[STM8_Routines_E_W_ROUTINEs_32K_ver_1_3_s19_len]=0;
    }
    else if ((flashsize==32) && (versBSL==0x14)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_32K_ver_1_4_s19 \n");
      #endif
      ptr = (char*) STM8_Routines_E_W_ROUTINEs_32K_ver_1_4_s19;
      ptr[STM8_Routines_E_W_ROUTINEs_32K_ver_1_4_s19_len]=0;
    }
    else if ((flashsize==128) && (versBSL==0x20)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_128K_ver_2_0_s19 \n");
      #endif
      ptr = (char*) STM8_Routines_E_W_ROUTINEs_128K_ver_2_0_s19;
      ptr[STM8_Routines_E_W_ROUTINEs_128K_ver_2_0_s19_len]=0;
    }
/*
    else if ((flashsize==128) && (versBSL==0x20)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_32K_verL_1_0_s19 \n");
      #endif
      ptr = (char*) STM8_Routines_E_W_ROUTINEs_32K_verL_1_0_s19;
      ptr[STM8_Routines_E_W_ROUTINEs_32K_verL_1_0_s19_len]=0;
    }
*/
    else if ((flashsize==128) && (versBSL==0x21)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_128K_ver_2_1_s19 \n");
      #endif
      ptr = (char*) STM8_Routines_E_W_ROUTINEs_128K_ver_2_1_s19;
      ptr[STM8_Routines_E_W_ROUTINEs_128K_ver_2_1_s19_len]=0;
    }
    else if ((flashsize==128) && (versBSL==0x22)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_128K_ver_2_2_s19 \n");
      #endif
      ptr = (char*) STM8_Routines_E_W_ROUTINEs_128K_ver_2_2_s19;
      ptr[STM8_Routines_E_W_ROUTINEs_128K_ver_2_2_s19_len]=0;
    }
    else if ((flashsize==128) && (versBSL==0x24)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_128K_ver_2_4_s19 \n");
      #endif
      ptr = (char*) STM8_Routines_E_W_ROUTINEs_128K_ver_2_4_s19;
      ptr[STM8_Routines_E_W_ROUTINEs_128K_ver_2_4_s19_len]=0;
    }
    else if ((flashsize==256) && (versBSL==0x10)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_256K_ver_1_0_s19 \n");
      #endif
      ptr = (char*) STM8_Routines_E_W_ROUTINEs_256K_ver_1_0_s19;
      ptr[STM8_Routines_E_W_ROUTINEs_256K_ver_1_0_s19_len]=0;
    }
    else {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror: unsupported device, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }

    // upload respective RAM routines to STM8
    {
      char      ramImage[8192];
      uint32_t  ramImageStart;
      uint32_t  numRamBytes;

      convert_s19(ptr, &ramImageStart, &numRamBytes, ramImage);

      if (g_verbose)
        printf("  Uploading RAM routines ... ");
      bsl_memWrite(ptrPort, ramImageStart, numRamBytes, ramImage, 0);
      if (g_verbose)
        printf("ok\n");
    }
  
  } // if STM8S or low-density STM8L -> upload RAM code



  // if flash mass erase
  if (flashErase) 
    bsl_flashMassErase(ptrPort);
    
        
        
  // if upload file to flash
  if (strlen(fileIn)>0) {
  
    // upload memory image to STM8
    bsl_memWrite(ptrPort, imageInStart, imageInBytes, imageIn, 1);


    // optionally verify upload
    if (verifyUpload==1) {
      bsl_memRead(ptrPort, imageInStart, imageInBytes, imageOut, 1);
      printf("  verify memory ... ");
      for (i=0; i<imageInBytes; i++) {
        if (imageIn[i] != imageOut[i]) {
          printf("failed at address 0x%04x (0x%02x vs 0x%02x), exit!\n", (uint32_t) (imageInStart+i), (uint8_t) (imageIn[i]), (uint8_t) (imageOut[i]));
          Exit(1, g_pauseOnExit);
        }        
      }
      printf("ok\n");
    }
    
    
    // enable ROM bootloader after upload (option bytes always on same address)
    if (enableBSL==1) {
      if (g_verbose)
        printf("  activate bootloader ... ");
      bsl_memWrite(ptrPort, 0x487E, 2, (char*)"\x55\xAA", 0);
      if (g_verbose)
        printf("ok\n");
    }
  
  } // if file upload to flash
  
  
  
  ////////////////////
  // read memory and dump to file
  ////////////////////
  if (strlen(fileOut)>0) {

    const char *shortname = strrchr(fileOut, '/');
    if (!shortname)
      shortname = fileOut;

    // read memory
    bsl_memRead(ptrPort, imageOutStart, imageOutBytes, imageOut, 1);
  
    // save to file, depending on file type
    const char *dot = strrchr (fileOut, '.');
    if (dot && !strcmp(dot, ".s19")) {
      if (g_verbose)
        printf("  save as Motorola S-record file '%s' ... ", shortname);
      else
        printf("  save to '%s' ... ", shortname);
      export_s19(fileOut, imageOut, imageOutStart, imageOutBytes);
      printf("ok\n");
    }
    else if (dot && !strcmp(dot, ".txt")) {
      if (g_verbose)
        printf("  save as plain file to '%s' ... ", shortname);
      else
        printf("  save to '%s' ... ", shortname);
      export_txt(fileOut, imageOut, imageOutStart, imageOutBytes);
      printf("ok\n");
    }
    else {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror: unsupported export type '%s', exit!\n\n", dot);
      Exit(1, g_pauseOnExit);
    }
  }
  
  

  // jump to flash start address after done (reset vector always on same address)
  if (jumpFlash)
    bsl_jumpTo(ptrPort, PFLASH_START);


  ////////
  // clean up and exit
  ////////
  close_port(&ptrPort);
  printf("done with program\n");
  Exit(0, g_pauseOnExit);
  
  // avoid compiler warnings
  return(0);
  
} // main

