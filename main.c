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
  #ifndef HANDLE 
    #define HANDLE  int     // comm port handler is int
  #endif
  #include <fcntl.h>      // File control definitions
  #include <termios.h>    // Posix terminal control definitions
  #include <getopt.h>
  #include <errno.h>    /* Error number definitions */
  #include <dirent.h>
  #include <sys/ioctl.h>
  #if defined(__ARMEL__) && defined(USE_WIRING)
    #include <wiringPi.h>       // for reset via GPIO
  #endif // __ARMEL__ && USE_WIRING

#else
  #error OS not supported
#endif

#define _MAIN_
  #include "globals.h"
#undef _MAIN_
#include "main.h"
#include "misc.h"
#include "serial_comm.h"
#include "spi_spidev_comm.h"
#include "spi_Arduino_comm.h"
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
  uint8_t   physInterface;        // bootloader interface: 0=UART (default), 1=SPI via spidev, 2=SPI via Arduino
  uint8_t   uartMode;             // UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply, other=auto-detect
  char      portname[STRLEN];     // name of communication port
  HANDLE    ptrPort;              // handle to communication port
  int       baudrate;             // communication baudrate [Baud]
  uint8_t   resetSTM8;            // reset STM8: 0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8, 5=Raspi pin 12 (default: manual)
  uint8_t   enableBSL;            // don't enable ROM bootloader after upload (caution!)
  uint8_t   flashErase;           // erase P-flash and D-flash prior to upload
  uint8_t   jumpFlash;            // jump to flash after upload
  uint8_t   verifyUpload;         // verify memory after upload
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
  g_verbose               = 2;    // high verbosity
  g_pauseOnExit           = 0;    // no wait for <return> before terminating
  g_backgroungOperation   = 0;    // by default assume foreground application

  // initialize default arguments
  portname[0] = '\0';             // no default port name
  physInterface  = 0;             // bootloader interface: 0=UART (default), 1=SPI via spidev, 2=SPI via Arduino
  uartMode   = 255;               // UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply, other=auto-detect
  baudrate   = 19200;             // default baudrate
  resetSTM8  = 1;                 // manual reset of STM8
  flashErase = 0;                 // erase P-flash and D-flash prior to upload
  jumpFlash  = 1;                 // jump to flash after uploade
  enableBSL  = 1;                 // enable bootloader after upload
  verifyUpload = 1;               // verify memory content after upload
  fileIn[0] = '\0';               // no default file to upload to flash
  fileOut[0] = '\0';              // no default file to download from flash
  
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
  
  
  // debug: print arguments
  /*
  printf("\n\narguments:\n");
  for (i=0; i<argc; i++) { 
    //printf("  %d: '%s'\n", (int) i, argv[i]);
    printf("%s ", argv[i]);
  }
  printf("\n\n");
  exit(1);
  */

  ////////
  // parse commandline arguments
  ////////
  for (i=1; i<argc; i++) {
    
    // interface type: 0=UART (default); 1=SPI via spidev, 2=SPI via Arduino
    if (!strcmp(argv[i], "-i")) {
      if (i<argc-1) {
        sscanf(argv[++i], "%d", &j);
        physInterface = j;
      }
    }    

    // name of communication port
    else if (!strcmp(argv[i], "-p")) {
      if (i<argc-1)
        strncpy(portname, argv[++i], STRLEN-1);
    }

    // communication baudrate
    else if (!strcmp(argv[i], "-b")) {
      if (i<argc-1)
        sscanf(argv[++i],"%d",&baudrate);
    }
    
    // UART mode: 0=duplex, 1=1-wire, 2=2-wire reply, other=auto-detect
    else if (!strcmp(argv[i], "-u")) {
      if (i<argc-1) {
        sscanf(argv[++i], "%d", &j);
        uartMode = j;
      }
    }
    
    // reset STM8 method: 0=skip, 1=manual; 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8, 5=Raspi pin 12
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
        if (j<imageOutStart) {        // swap addresses if required
          int tmp = imageOutStart;
          imageOutStart = j;
          j = tmp;
        }
        imageOutBytes = j - imageOutStart + 1;
      }
      if (i<argc-1)
        strncpy(fileOut, argv[++i], STRLEN-1);
    }
    
    // don't jump to address after upload
    else if (!strcmp(argv[i], "-j")) {
      jumpFlash = 0;
    }

    // verbosity level (0..2)
    else if (!strcmp(argv[i], "-V")) {
      if (i<argc-1)
        sscanf(argv[++i],"%d",&g_verbose);
      if (g_verbose < 0) g_verbose = 0;
      if (g_verbose > 2) g_verbose = 2;
    }

    // optimize for background operation, e.g. skip prompts and colors
    else if (!strcmp(argv[i], "-B")) {
      g_backgroungOperation = 1;
    }

    // prompt for <return> prior to exit
    else if (!strcmp(argv[i], "-q")) {
      g_pauseOnExit = 1;
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

      printf("usage: %s [-h] [-i interface] [-p port] [-b rate] [-u mode] [-R ch] [-e] [-w infile] [-x] [-v] [-r start stop outfile] [-j] [-V verbose] [-B] [-q]\n", appname);
      printf("  -h                     print this help\n");
      #ifdef USE_SPIDEV
        printf("  -i interface           communication interface: 0=UART, 1=SPI via spidev, 2=SPI via Arduino (default: UART)\n");
      #else
        printf("  -i interface           communication interface: 0=UART, 2=SPI via Arduino (default: UART)\n");
      #endif
      printf("  -p port                name of communication port (default: list available ports)\n");
      printf("  -b rate                communication baudrate in Baud (default: 19200)\n");
      printf("  -u mode                UART mode: 0=duplex, 1=1-wire, 2=2-wire reply, other=auto-detect (default: auto-detect)\n");
      #if defined(__ARMEL__) && defined(USE_WIRING)
        printf("  -R ch                  reset STM8: 0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8, 5=Raspi pin 12 (default: manual)\n");
      #else
        printf("  -R ch                  reset STM8: 0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8 (default: manual)\n");
      #endif
      printf("  -e                     erase P-flash and D-flash prior to upload (default: skip)\n");
      printf("  -w infile              upload s19 or intel-hex file to flash (default: skip)\n");
      printf("    -x                   don't enable ROM bootloader after upload (default: enable)\n");
      printf("    -v                   don't verify code in flash after upload (default: verify)\n");
      printf("  -r start stop outfile  read memory range (in hex) to s19 file or table (default: skip)\n");
      printf("  -j                     don't jump to flash before exit (default: jump to flash)\n");
      printf("  -V                     verbosity level 0..2 (default: 2)\n");
      printf("  -B                     optimize for background operation, e.g. skip prompts and colors (default: interactive use)\n");
      printf("  -q                     prompt for <return> prior to exit (default: no prompt)\n");
      printf("\n");
      Exit(0, 0);
    }

  } // process commandline arguments
  

  ////////
  // some parameter post-processing
  ////////
  if ((physInterface == 1) || (physInterface == 2)) verifyUpload = 0; // read back after writing doesn't work for SPI (don't know why)
  if (g_backgroungOperation) g_pauseOnExit = 0;                       // for background operation avoid prompt on exit


  ////////
  // reset console color (needs to be called once for Win32)      
  ////////
  setConsoleColor(PRM_COLOR_DEFAULT);


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
    if (!g_backgroungOperation) {
      printf("  enter comm port name ( ");
      list_ports();
      printf(" ): ");
      scanf("%s", portname);
      getchar();
    }
    else {
      printf("  available comm ports ( ");
      list_ports();
      printf(" ), exit!");
      Exit(1, 0);
    }
  } // if no comm port name


  // If specified import hexfile - do it early here to be able to report file read errors before others
  if (strlen(fileIn) > 0) {
    
    // strip path from filename for readability
    #if defined(WIN32)
      const char *shortname = strrchr(fileIn, '\\');
    #else
      const char *shortname = strrchr(fileIn, '/');
    #endif
    if (!shortname)
      shortname = fileIn;
    else
      shortname++;

    // convert to memory image, depending on file type
    const char *dot = strrchr (fileIn, '.');
    if (dot && (!strcmp(dot, ".s19") || !strcmp(dot, ".S19"))) {
      if (g_verbose == 1)
        printf("  load file '%s' ... ", shortname);
      else if (g_verbose == 2)
        printf("  load Motorola S-record file '%s' ... ", shortname);
      fflush(stdout);
      load_hexfile(fileIn, fileBufIn, BUFSIZE);
      convert_s19(fileBufIn, &imageInStart, &imageInBytes, imageIn);
    }
    else if (dot && (!strcmp(dot, ".hex") || !strcmp(dot, ".HEX") || !strcmp(dot, ".ihx") || !strcmp(dot, ".IHX"))) {
      if (g_verbose == 1)
        printf("  load file '%s' ... ", shortname);
      else if (g_verbose == 2)
        printf("  load Intel hex file '%s' ... ", shortname);
      fflush(stdout);
      load_hexfile(fileIn, fileBufIn, BUFSIZE);
      convert_hex(fileBufIn, &imageInStart, &imageInBytes, imageIn);
    }
    else {
      if (g_verbose == 1)
        printf("  load file '%s' ... ", shortname);
      else if (g_verbose == 2)
        printf("  load binary file '%s' ... ", shortname);
      fflush(stdout);
      load_binfile(fileIn, imageIn, &imageInStart, &imageInBytes, BUFSIZE);
    }
    
    // print size of imported data
    if (g_verbose > 0) {
      if (imageInBytes>2048)
        printf("ok (%1.1fkB)\n", (float) imageInBytes/1024.0);
      else if (imageInBytes>0)
        printf("ok (%dB)\n", imageInBytes);
      else
        printf("ok, no data read\n");
      fflush(stdout);
    }

  } // import hexfile


  ////////
  // reset STM8
  // Note: prior to opening port to avoid flushing issue under Linux, see https://stackoverflow.com/questions/13013387/clearing-the-serial-ports-buffer
  ////////

  // skip reset of STM8
  if (resetSTM8 == 0) {

  }
  
  // manually reset STM8
  else if (resetSTM8 == 1) {
    if (!g_backgroungOperation) {
      printf("  reset STM8 and press <return>");
      fflush(stdout);
      fflush(stdin);
      getchar();
    }
    else {
      printf("  reset STM8 now\n");
      fflush(stdout);
    }
  }
  
  // HW reset STM8 using DTR line (USB/RS232)
  else if (resetSTM8 == 2) {
    printf("  reset via DTR ... ");
    fflush(stdout);
    pulse_DTR(ptrPort, 10);
    printf("ok\n");
    fflush(stdout);
    SLEEP(20);                        // allow BSL to initialize
  }
  
  // SW reset STM8 via command 'Re5eT!' at 115.2kBaud with (8,0,1) (requires respective STM8 SW)
  else if (resetSTM8 == 3) {
    printf("  reset via UART command ... ");
    fflush(stdout);
    sprintf(buf, "Re5eT!");           // reset command (same as in STM8 SW!)
    ptrPort = init_port(portname, 115200, 100, 8, 0, 1, 0, 0);
    for (i=0; i<6; i++) {
      send_port(ptrPort, 0, 1, buf+i);   // send reset command bytewise to account for possible slow handling on STM8 side
      SLEEP(10);
    }
    close_port(&ptrPort);
    printf("ok\n");
    fflush(stdout);
    SLEEP(20);                        // allow BSL to initialize
  }
  
  // HW reset STM8 using Arduino pin 8
  else if (resetSTM8 == 4) {
    printf("  reset via Arduino pin %d ... ", ARDUINO_RESET_PIN);
    fflush(stdout);
    setPin_Arduino(ptrPort, ARDUINO_RESET_PIN, 0);
    SLEEP(1);
    setPin_Arduino(ptrPort, ARDUINO_RESET_PIN, 1);
    printf("ok\n");
    fflush(stdout);
    SLEEP(20);                      // allow BSL to initialize
  }
  
  // HW reset STM8 using header pin 12 (only Raspberry Pi!)
  #if defined(__ARMEL__) && defined(USE_WIRING)
    else if (resetSTM8 == 5) {
      printf("  reset via Raspi pin 12 ... ");
      fflush(stdout);
      pulse_GPIO(12, 20);
      printf("ok\n");
      fflush(stdout);
      SLEEP(20);                      // allow BSL to initialize
    }
  #endif // __ARMEL__ && USE_WIRING

  // unknown reset method -> error
  else {
    setConsoleColor(PRM_COLOR_RED);
    #ifdef __ARMEL__
      fprintf(stderr, "\n\nerror: reset method %d not supported (0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8), 5=Raspi pin 12, exit!\n\n", resetSTM8);
    #else
      fprintf(stderr, "\n\nerror: reset method %d not supported (0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8), exit!\n\n", resetSTM8);
    #endif
    Exit(1, g_pauseOnExit);
  }
  
  

  ////////
  // open port with given properties
  ////////
  
  // UART interface (default)
  if (physInterface == 0) {
  
    if (g_verbose == 2)
      printf("  open serial port '%s' with %gkBaud ... ", portname, (float) baudrate / 1000.0);
    fflush(stdout);
    ptrPort = init_port(portname, baudrate, TIMEOUT, 8, 0, 1, 0, 0);   // start without parity, may be changed in bsl_sync()
    if (g_verbose == 2)
      printf("ok\n");
    fflush(stdout);
    
  } // UART
  
  // SPI via spidev
  #if defined(USE_SPIDEV)
    else if (physInterface == 1) {
  
      if (g_verbose == 1)
        printf("  open SPI '%s' ... ", portname);
      else if (g_verbose == 2) {
        if (baudrate < 1000000.0)
          printf("  open SPI '%s' with %gkBaud ... ", portname, (float) baudrate / 1000.0);
        else
          printf("  open SPI '%s' with %gMBaud ... ", portname, (float) baudrate / 1000000.0);
      }
      fflush(stdout);
      ptrPort = init_spi_spidev(portname, baudrate);
      if (g_verbose >= 0)
        printf("ok\n");
      fflush(stdout);
  
    } // SPI via spidev
  #endif // USE_SPIDEV

  // SPI via Arduino
  else if (physInterface == 2) {
  
    // open port
    if (g_verbose == 1)
      printf("  open Arduino port '%s' ... ", portname);
    else if (g_verbose == 2)
      printf("  open Arduino port '%s' with %gkBaud SPI ... ", portname, (float) ARDUINO_BAUDRATE / 1000.0);
    fflush(stdout);
    ptrPort = init_port(portname, ARDUINO_BAUDRATE, 100, 8, 0, 1, 0, 0);
    if (g_verbose > 0)
      printf("ok\n");
    fflush(stdout);
    
    // wait until after Arduino bootloader
    if (g_verbose == 2)
      printf("  wait for Arduino bootloader ... ");
    fflush(stdout);
    SLEEP(2000);
    if (g_verbose == 2)
      printf("ok\n");
    fflush(stdout);

    // init SPI interface and set NSS pin to high
    if (g_verbose == 2) {
      if (baudrate < 1000000L)
        printf("  init SPI with %gkBaud... ", (float) baudrate / 1000.0);
      else
        printf("  init SPI with %gMBaud... ", (float) baudrate / 1000000.0);
    }
    fflush(stdout);
    setPin_Arduino(ptrPort, ARDUINO_CSN_PIN, 1);
    configSPI_Arduino(ptrPort, baudrate, ARDUINO_MSBFIRST, ARDUINO_SPI_MODE0);
    if (g_verbose == 2)
      printf("ok\n");
    fflush(stdout);

  } // SPI via Arduino

  // unknown interface -> error
  else {
    setConsoleColor(PRM_COLOR_RED);
    #if defined(USE_SPIDEV)
      fprintf(stderr, "\n\nerror: interface %d not supported (0=UART, 1=SPI via spidev, 2=SPI via Arduino), exit!\n\n", physInterface);
    #else
      fprintf(stderr, "\n\nerror: interface %d not supported (0=UART, 2=SPI via Arduino), exit!\n\n", physInterface);
    #endif
    Exit(1, g_pauseOnExit);
  }
  
 
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
  // communicate with STM8 bootloader
  ////////

  // required to make flush work, for some reason
  usleep(200000);
  flush_port(ptrPort);
  
  // synchronize with bootloader. For UART also sync baudrate
  bsl_sync(ptrPort, physInterface);
  
  // for UART set or auto-detect UART mode (0=duplex, 1=1-wire, 2=2-wire reply, others=auto-detect)
  if (physInterface == 0) {
    if (uartMode == 0)
      set_parity(ptrPort, 2);
    else if (uartMode == 1)
      set_parity(ptrPort, 0);
    else if (uartMode == 2) {
      set_parity(ptrPort, 0);
      buf[0] = ACK;
      send_port(ptrPort, 0, 1, buf);    // need to reply ACK first to revert bootloader
    }
    else
      uartMode = bsl_getUartMode(ptrPort);
  } // UART interface
  
  // get bootloader info for selecting RAM w/e routines for flash
  bsl_getInfo(ptrPort, physInterface, uartMode, &flashsize, &versBSL, &family);

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

      if (g_verbose == 2)
        printf("  Uploading RAM routines ... ");
      fflush(stdout);
      bsl_memWrite(ptrPort, physInterface, uartMode, ramImageStart, numRamBytes, ramImage, -1);
      if (g_verbose == 2)
        printf("ok (%dB from 0x%04x)\n", numRamBytes, ramImageStart);
      fflush(stdout);
    }
  
  } // if STM8S or low-density STM8L -> upload RAM code



  // if flash mass erase
  if (flashErase) 
    bsl_flashMassErase(ptrPort, physInterface, uartMode);
    
        
        
  // if upload file to flash
  if (strlen(fileIn)>0) {
  
    // upload memory image to STM8
    //export_txt("write.txt", imageIn, imageInStart, imageInBytes);   // debug
    bsl_memWrite(ptrPort, physInterface, uartMode, imageInStart, imageInBytes, imageIn, g_verbose);
    
    // optionally verify upload
    if (verifyUpload==1) {
      bsl_memRead(ptrPort, physInterface, uartMode, imageInStart, imageInBytes, imageOut);
      //export_txt("read.txt", imageOut, imageInStart, imageInBytes);   // debug
      printf("  verify memory ... ");
      for (i=0; i<imageInBytes; i++) {
        if (imageIn[i] != imageOut[i]) {
          setConsoleColor(PRM_COLOR_RED);
          printf("\nfailed at address 0x%04x (0x%02x vs 0x%02x), exit!\n", (uint32_t) (imageInStart+i), (uint8_t) (imageIn[i]), (uint8_t) (imageOut[i]));
          Exit(1, g_pauseOnExit);
        }        
      }
      printf("ok\n");
    }
    
    
    // enable ROM bootloader after upload (option bytes always on same address)
    if (enableBSL==1) {
      if (g_verbose == 2)
        printf("  activate bootloader ... ");
      fflush(stdout);
      bsl_memWrite(ptrPort, physInterface, uartMode, 0x487E, 2, (char*)"\x55\xAA", -1);
      if (g_verbose == 2)
        printf("ok\n");
      fflush(stdout);
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
    bsl_memRead(ptrPort, physInterface, uartMode, imageOutStart, imageOutBytes, imageOut);
  
    // save to file, depending on file type
    const char *dot = strrchr (fileOut, '.');
    if (dot && !strcmp(dot, ".s19")) {
      printf("  save as Motorola S-record file '%s' ... ", shortname);
      fflush(stdout);
      export_s19(fileOut, imageOut, imageOutStart, imageOutBytes);
      printf("ok\n");
      fflush(stdout);
    }
    else if (dot && !strcmp(dot, ".txt")) {
      printf("  save as plain file to '%s' ... ", shortname);
      fflush(stdout);
      export_txt(fileOut, imageOut, imageOutStart, imageOutBytes);
      printf("ok\n");
      fflush(stdout);
    }
    else {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror: unsupported export type '%s', exit!\n\n", dot);
      Exit(1, g_pauseOnExit);
    }
  }
  

  // jump to flash start address after done (reset vector always on same address)
  if (jumpFlash) {

    // don't know why, but seems to be required for SPI
    if ((physInterface==1) || (physInterface==2))
      SLEEP(500);
    
    // jumpt to application
    if (g_verbose == 2)
      printf("  jump to address 0x%04x ... ", (int) PFLASH_START);
    fflush(stdout);
    bsl_jumpTo(ptrPort, physInterface, uartMode, PFLASH_START);
    if (g_verbose == 2)
      printf("ok\n");
    fflush(stdout);
  
  } // jump to flash

  ////////
  // clean up and exit
  ////////
  close_port(&ptrPort);
  printf("done with program\n");
  Exit(0, g_pauseOnExit);
  
  // avoid compiler warnings
  return(0);
  
} // main

