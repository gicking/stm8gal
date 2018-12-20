/**
   \file main.c

   \author G. Icking-Konert
   \date 2018-12-14
   \version 0.2
   
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
  #include "main.h"
#undef _MAIN_
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

// max length of filenames
#define  STRLEN   1000


/**
   \fn int main(int argc, char *argv[])
   
   \param argc      number of commandline arguments + 1
   \param argv      string array containing commandline arguments (argv[0] contains name of executable)
   
   \return dummy return code (not used)
   
   Main routine for import, programming, and check routines
*/
int main(int argc, char ** argv) {
 
  // local variables
  char      appname[STRLEN];      // name of application without path
  char      version[100];         // version as string
  int       verbose;              // verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)
  int       physInterface;        // bootloader interface: 0=UART (default), 1=SPI_ARDUINO, 2=SPI_SPIDEV
  char      portname[STRLEN] = ""; // name of communication port
  HANDLE    ptrPort;              // handle to communication port
  int       baudrate;             // communication baudrate [Baud]
  int       uartMode;             // UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply, other=auto-detect
  int       resetSTM8;            // reset STM8: 0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8, 5=Raspi pin 12 (default: manual)
  uint16_t  *imageBuf;            // global RAM image buffer (high byte != 0 indicates value is set)
  uint32_t  addrStart;            // start address for image buffer (corresponds to imageBuf[0])
  uint32_t  addrStop;             // highest address in image buffer (corresponds to imageBuf[addrStop-addrStart])
  bool      verifyUpload;         // verify memory after upload
  uint32_t  jumpAddr;             // address to jump to before exit program
  bool      printHelp;            // flag for printing help page
  int       i, j;                 // generic variables  
  char      tmp[STRLEN];          // misc buffer
  
  // STM8 propoerties
  int       flashsize;            // size of flash (kB) for w/e routines
  uint8_t   versBSL;              // BSL version for w/e routines
  uint8_t   family;               // device family, currently STM8S and STM8L
  

  // initialize global variables
  g_pauseOnExit         = false;  // no wait for <return> before terminating (dummy)
  g_backgroundOperation = false;  // assume foreground application

  // initialize default arguments
  portname[0]    = '\0';          // no default port name
  physInterface  = UART;          // bootloader interface: 0=UART (default), 1=SPI_ARDUINO, 2=SPI_SPIDEV
  uartMode       = 255;           // UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply, other=auto-detect
  baudrate       = 115200;        // default baudrate
  verbose        = INFORM;        // verbosity level medium
  resetSTM8      = 1;             // manual reset of STM8
  verifyUpload   = true;          // verify memory content after upload
  jumpAddr       = PFLASH_START;  // by default jump to start of P-flash (see bootloader.h)


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
  
  
  // get app name & version, and change console title
  get_app_name(argv[0], VERSION, appname, version);
  sprintf(tmp, "%s (%s)", appname, version);
  setConsoleTitle(tmp);  

  
  /////////////////
  // 1st pass of commandline arguments: set global parameters, no upload/download/erase yet
  /////////////////

  printHelp = false;
  for (int i=1; i<argc; i++) {
    
    // get interface type: 0=UART (default), 1=SPI_ARDUINO, 2=SPI_SPIDEV
    if ((!strcmp(argv[i], "--interface")) || (!strcmp(argv[i], "-i"))) {

      // get interface
      if (i+1<argc) {
        sscanf(argv[++i], "%d", &j);
        physInterface = j;
      }
      else {
        printHelp = true;
        break;
      }

    } // interface
    

    // name of communication port
    else if ((!strcmp(argv[i], "--port")) || (!strcmp(argv[i], "-p"))) {

      // get port name
      if (i+1<argc)
        strncpy(portname, argv[++i], STRLEN-1);
      else {
        printHelp = true;
        break;
      }

    } // port

      
    // communication baudrate
    else if ((!strcmp(argv[i], "--baudrate")) || (!strcmp(argv[i], "-b"))) {
      
      // get communication baudrate
      if (i+1<argc)
        sscanf(argv[++i],"%d",&baudrate);
      else {
        printHelp = true;
        break;
      }

    } // baudrate

      
    // UART mode
    else if ((!strcmp(argv[i], "--uartmode")) || (!strcmp(argv[i], "-u"))) {
      
      // get UART mode
      if (i+1<argc) {
        sscanf(argv[++i], "%d", &j);
        uartMode = j;
      }
      else {
        printHelp = true;
        break;
      }

    } // uartmode

      
    // reset method: 0=skip, 1=manual; 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8, 5=Raspi pin 12
    else if ((!strcmp(argv[i], "--reset")) || (!strcmp(argv[i], "-R"))) {
      
      // get reset STM8 method
      if (i+1<argc) {
        sscanf(argv[++i], "%d", &j);
        resetSTM8 = j;
      }
      else {
        printHelp = true;
        break;
      }

    } // reset

      
    // no verify of memory content after upload
    else if ((!strcmp(argv[i], "--noverify")) || (!strcmp(argv[i], "-v"))) {
      verifyUpload = 0;
    } // noverify
    

    // skip file upload. Just check parameter number
    else if ((!strcmp(argv[i], "--write")) || (!strcmp(argv[i], "-w"))) {

      // get file name
      if (i+1<argc) {
        if (strstr(argv[++i], ".bin") != NULL) {  // for binary file skip additionaly address
          if (i+1<argc)
            i+=1;
          else {
            printHelp = true;
            break;
          }
        }
      }
      else {
        printHelp = true;
        break;
      }

    } // write
      

    // skip setting value at given address. Just check parameter number
    else if ((!strcmp(argv[i], "--set")) || (!strcmp(argv[i], "-s"))) {
      if (i+2<argc)
        i+=2;
      else {
        printHelp = true;
        break;
      }
    } // set
      

    // skip reading address range. Just check parameter number
    else if ((!strcmp(argv[i], "--read")) || (!strcmp(argv[i], "-r"))) {
      if (i+3<argc)
        i+=3;
      else {
        printHelp = true;
        break;
      }
    } // read


    // skip mass erase
    else if ((!strcmp(argv[i], "--erase")) || (!strcmp(argv[i], "-e"))) {
      // dummy
    } // erase

      
    // jump adress before program termination (-1 or 0xFFFFFFFF == skip jump)
    else if ((!strcmp(argv[i], "--jump")) || (!strcmp(argv[i], "-j"))) {
      
      // get jump address (0x
      if (i+1<argc) {
        int addrTmp;
        sscanf(argv[++i], "%x", &addrTmp);
        jumpAddr = addrTmp;
      }
      else {
        printHelp = true;
        break;
      }

    } // jump


    // set verbosity level (0..2)
    else if ((!strcmp(argv[i], "--verbose")) || (!strcmp(argv[i], "-V"))) {
      
      // get verbosity level
      if (i+1<argc)
        sscanf(argv[++i],"%d",&verbose);
      else {
        printHelp = true;
        break;
      }
      if (verbose < MUTE)   verbose = MUTE;
      if (verbose > CHATTY) verbose = CHATTY;

    } // verbosity


    // optimize for background operation, e.g. skip prompts and colors
    else if ((!strcmp(argv[i], "--background")) || (!strcmp(argv[i], "-B"))) {
      g_backgroundOperation = 1;
    }


    // prompt for <return> prior to exit
    else if ((!strcmp(argv[i], "--exitPrompt")) || (!strcmp(argv[i], "-q"))) {
      g_pauseOnExit = 1;
    }


    // else print help
    else {
      printHelp = true;
      break;
    }

  } // 1st pass over commandline arguments

  
  // on request (-h) or in case of parameter error print help page
  if ((printHelp==true) || (argc == 1)) {
    printf("\n");
    printf("\n%s (%s)\n\n", appname, version);
    printf("Modify or read STM8 memory content by bootloader.\n");
    printf("For more information see https://github.com/gicking/stm8gal\n");
    printf("\n");
    printf("usage: %s [-h] [-i interface] [-p port] [-b rate] [-u mode] [-R ch] [-v] [-w infile [addr]] [-s addr value] [-r addrStart addrStop outfile] [-e] [-j addr] [-V verbose] [-B] [-q]\n", appname);
    printf("    -h / --help           print this help\n");
    #ifdef USE_SPIDEV
      printf("    -i / --interface      communication interface: 0=UART, 1=SPI via Arduino, 2=SPI via spidev (default: UART)\n");
    #else
      printf("    -i / --interface      communication interface: 0=UART, 1=SPI via Arduino (default: UART)\n");
    #endif
    printf("    -p / --port           name of communication port (default: list available ports)\n");
    printf("    -b / --baudrate       communication baudrate in Baud (default: 115200)\n");
    printf("    -u / --uartmode       UART mode: 0=duplex, 1=1-wire, 2=2-wire reply, other=auto-detect (default: auto-detect)\n");
    #if defined(__ARMEL__) && defined(USE_WIRING)
      printf("    -R / --reset          reset STM8: 0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8, 5=Raspi pin 12 (default: manual)\n");
    #else
      printf("    -R / --reset          reset STM8: 0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8 (default: manual)\n");
    #endif
    printf("    -v / --noverify       don't verify code in flash after upload (default: verify)\n");
    printf("    -w / --write          upload file from PC to uController. For binary file (*.bin) add address offset (default: skip)\n");
    printf("    -s / --set            change content of (any) address to given value (default: skip)\n");
    printf("    -r / --read           read memory range (address in hex) and print to console or save to file (default: skip)\n");
    printf("    -e / --erase          mass erase uController flash. Use carefully! (default: skip)\n");
    printf("    -j / --jump           jump to given address before exit (-1: skip jump, default: jump to flash)\n");
    printf("    -V / --verbose        verbosity level 0..3 (default: 2)\n");
    printf("    -B / --background     optimize for background operation, e.g. skip prompts and colors (default: interactive use)\n");
    printf("    -q / --exitPrompt     prompt for <return> prior to exit (default: no prompt)\n");
    printf("\n");
    printf("Supported upload formats:\n");
    printf("  - Motorola S19 (*.s19), for a description see https://en.wikipedia.org/wiki/SREC_(file_format)\n");
    printf("  - Intel Hex (*.hex, *.ihx), for a description see https://en.wikipedia.org/wiki/Intel_HEX\n");
    printf("  - ASCII table (*.txt) consisting of lines with 'hexAddr  value'. Lines starting with '#' are ignored\n");
    printf("  - Binary (*.bin) with an additional starting address\n");
    printf("\n");
    printf("Supported export formats:\n");
    printf("  - print to stdout (console)\n");
    printf("  - Motorola S19 (*.s19)\n");
    printf("  - ASCII table (*.txt) with 'hexAddr  hexValue'\n");
    printf("  - Binary (*.bin) without starting address\n");
    printf("\n");
    printf("Data is uploaded and exported in the specified order, i.e. later uploads may\n");
    printf("overwrite previous uploads. Also exports only contain the previous uploads, i.e.\n");
    printf("intermediate exports only contain the memory content up to that point in time.\n");
    printf("\n");
    Exit(0,0);
  }


  ////////
  // perform some misc tasks
  ////////
  
  // read back after writing doesn't work for SPI (don't know why)
  #if defined(USE_SPIDEV)
    if ((physInterface == SPI_ARDUINO) || (physInterface == SPI_SPIDEV))
      verifyUpload = 0; 
  #else
    if (physInterface == SPI_ARDUINO)
      verifyUpload = 0; 
  #endif  

  // for background operation avoid prompt on exit
  if (g_backgroundOperation)
    g_pauseOnExit = 0;

  // reset console color (needs to be called once for Win32)      
  setConsoleColor(PRM_COLOR_DEFAULT);

  // allocate and init global RAM image (>1MByte requires dynamic allocation)
  if (!(imageBuf = malloc(LENIMAGEBUF * sizeof(*imageBuf))))
    Error("Cannot allocate image buffer, try reducing LENIMAGEBUF");
  memset(imageBuf, 0, LENIMAGEBUF * sizeof(*imageBuf));


  /////////////////
  // initiate communication with STM8 bootloader
  /////////////////
  
  // print message
  if (verbose != MUTE)
    printf("\n%s (%s)\n", appname, version);


  ////////
  // if no port name is given, list all available ports and query
  ////////
  if (strlen(portname) == 0) {
    if (!g_backgroundOperation) {
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


  ////////
  // reset STM8
  // Note: prior to opening port to avoid flushing issue under Linux, see https://stackoverflow.com/questions/13013387/clearing-the-serial-ports-buffer
  ////////

  // skip reset of STM8
  if (resetSTM8 == 0) {

  }
  
  // manually reset STM8
  else if (resetSTM8 == 1) {
    if (!g_backgroundOperation) {
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
    char buf[10] = "Re5eT!";          // reset command (same as in STM8 SW!)
    printf("  reset via UART command ... ");
    fflush(stdout);
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
  
  // HW reset STM8 using Arduino pin 8 -> delay until Arduino port is open 
  else if (resetSTM8 == 4) {
    
    // dummy

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
    #ifdef __ARMEL__
      Error("reset method %d not supported (0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8), 5=Raspi pin 12", resetSTM8);
    #else
      Error("reset method %d not supported (0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8)", resetSTM8);
    #endif
  }
  

  ////////
  // open port with given properties
  ////////
  
  // UART interface (default)
  if (physInterface == UART) {
  
    if (verbose == INFORM)
      printf("  open serial port '%s' ... ", portname);
    else if (verbose == CHATTY)
      printf("  open serial port '%s' with %gkBaud ... ", portname, (float) baudrate / 1000.0);
    fflush(stdout);
    ptrPort = init_port(portname, baudrate, TIMEOUT, 8, 0, 1, 0, 0);   // start without parity, may be changed in bsl_sync()
    if ((verbose == INFORM) || (verbose == CHATTY))
      printf("ok\n");
    fflush(stdout);
    
  } // UART

  // SPI via Arduino
  else if (physInterface == SPI_ARDUINO) {
  
    // open port
    if (verbose == INFORM)
      printf("  open Arduino port '%s' ... ", portname);
    else if (verbose == CHATTY)
      printf("  open Arduino port '%s' with %gkBaud SPI ... ", portname, (float) ARDUINO_BAUDRATE / 1000.0);
    fflush(stdout);
    ptrPort = init_port(portname, ARDUINO_BAUDRATE, 100, 8, 0, 1, 0, 0);
    if ((verbose == INFORM) || (verbose == CHATTY))
      printf("ok\n");
    fflush(stdout);
    
    // wait until after Arduino bootloader
    if ((verbose == INFORM) || (verbose == CHATTY))
      printf("  wait for Arduino bootloader ... ");
    fflush(stdout);
    SLEEP(2000);
    if ((verbose == INFORM) || (verbose == CHATTY))
      printf("ok\n");
    fflush(stdout);

    // init SPI interface and set NSS pin to high
    if (verbose == CHATTY) {
      if (baudrate < 1000000L)
        printf("  init SPI with %gkBaud... ", (float) baudrate / 1000.0);
      else
        printf("  init SPI with %gMBaud... ", (float) baudrate / 1000000.0);
    }
    fflush(stdout);
    setPin_Arduino(ptrPort, ARDUINO_CSN_PIN, 1);
    configSPI_Arduino(ptrPort, baudrate, ARDUINO_MSBFIRST, ARDUINO_SPI_MODE0);
    if (verbose == CHATTY)
      printf("ok\n");
    fflush(stdout);

    // HW reset STM8 using Arduino pin 8 -> delay until Arduino port is open 
    if (resetSTM8 == 4) {
      if ((verbose == INFORM) || (verbose == CHATTY))
        printf("  reset via Arduino pin %d ... ", ARDUINO_RESET_PIN);
      fflush(stdout);
      setPin_Arduino(ptrPort, ARDUINO_RESET_PIN, 0);
      SLEEP(1);
      setPin_Arduino(ptrPort, ARDUINO_RESET_PIN, 1);
      if ((verbose == INFORM) || (verbose == CHATTY))
        printf("ok\n");
      fflush(stdout);
      SLEEP(20);                      // allow BSL to initialize
    }

  } // SPI via Arduino
  
  // SPI via spidev
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV) {
  
      if (verbose == INFORM)
        printf("  open SPI '%s' ... ", portname);
      else if (verbose == CHATTY) {
        if (baudrate < 1000000.0)
          printf("  open SPI '%s' with %gkBaud ... ", portname, (float) baudrate / 1000.0);
        else
          printf("  open SPI '%s' with %gMBaud ... ", portname, (float) baudrate / 1000000.0);
      }
      fflush(stdout);
      ptrPort = init_spi_spidev(portname, baudrate);
      if ((verbose == INFORM) || (verbose == CHATTY))
        printf("ok\n");
      fflush(stdout);
  
    } // SPI via spidev
  #endif // USE_SPIDEV

  // unknown interface -> error
  else {
    #if defined(USE_SPIDEV)
      Error("interface %d not supported (0=UART, 1=SPI via Arduino, 2=SPI via spidev)", physInterface);
    #else
      Error("interface %d not supported (0=UART, 1=SPI via Arduino)", physInterface);
    #endif
  }
  
 
  // debug: communication test (echo+1 test-SW on STM8)
  #ifdef DEBUG
    printf("open: %d\n", ptrPort);
    for (i=0; i<254; i++) {
      Tx[0] = i;
      send_port(ptrPort, 1, Tx);
      receive_port(ptrPort, 1, Rx);
      printf("%d  %d\n", (int) Tx[0], (int) Rx[0]);
    }
    printf("ok\n");
    Exit(1,0);
  #endif


  ////////
  // communicate with STM8 bootloader
  ////////

  // required to make flush work, for some reason
  usleep(200000);
  flush_port(ptrPort);
  
  // synchronize with bootloader. For UART also sync baudrate
  bsl_sync(ptrPort, physInterface, verbose);
  
  // for UART set or auto-detect UART mode (0=duplex, 1=1-wire, 2=2-wire reply, others=auto-detect)
  if (physInterface == UART) {
    if (uartMode == 0) {
      set_parity(ptrPort, 2);
      printf("  set UART mode: duplex\n");
	}
    else if (uartMode == 1) {
      set_parity(ptrPort, 0);
      printf("  set UART mode: 1-wire\n");
	}
    else if (uartMode == 2) {
      char c = ACK;      // need to reply ACK first to revert bootloader
      set_parity(ptrPort, 0);
      send_port(ptrPort, 0, 1, &c);
      printf("  set UART mode: 2-wire reply\n");
    }
    else
      uartMode = bsl_getUartMode(ptrPort, verbose);
  } // UART interface
  fflush(stdout);
  
  // get bootloader info for selecting RAM w/e routines for flash
  bsl_getInfo(ptrPort, physInterface, uartMode, &flashsize, &versBSL, &family, verbose);

  // for STM8S and 8kB STM8L upload RAM routines, else skip
  if ((family == STM8S) || (flashsize==8)) {
    
    char   *ptrRAM = NULL;          // pointer to array with RAM routines
    int    lenRAM;                  // length of RAM array

    // select device dependent flash routines for upload
    if ((flashsize==8) && (versBSL==0x10)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_8K_verL_1_0_s19 \n");
      #endif
      ptrRAM = (char*) STM8_Routines_E_W_ROUTINEs_8K_verL_1_0_s19;
      lenRAM = STM8_Routines_E_W_ROUTINEs_8K_verL_1_0_s19_len;
    }
    else if ((flashsize==32) && (versBSL==0x10)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_32K_ver_1_0_s19 \n");
      #endif
      ptrRAM = (char*) STM8_Routines_E_W_ROUTINEs_32K_ver_1_0_s19;
      lenRAM = STM8_Routines_E_W_ROUTINEs_32K_ver_1_0_s19_len;
    }
    else if ((flashsize==32) && (versBSL==0x12)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_32K_ver_1_2_s19 \n");
      #endif
      ptrRAM = (char*) STM8_Routines_E_W_ROUTINEs_32K_ver_1_2_s19;
      lenRAM = STM8_Routines_E_W_ROUTINEs_32K_ver_1_2_s19_len;
    }
    else if ((flashsize==32) && (versBSL==0x13)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_32K_ver_1_3_s19 \n");
      #endif
      ptrRAM = (char*) STM8_Routines_E_W_ROUTINEs_32K_ver_1_3_s19;
      lenRAM = STM8_Routines_E_W_ROUTINEs_32K_ver_1_3_s19_len;
    }
    else if ((flashsize==32) && (versBSL==0x14)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_32K_ver_1_4_s19 \n");
      #endif
      ptrRAM = (char*) STM8_Routines_E_W_ROUTINEs_32K_ver_1_4_s19;
      lenRAM = STM8_Routines_E_W_ROUTINEs_32K_ver_1_4_s19_len;
    }
    else if ((flashsize==128) && (versBSL==0x20)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_128K_ver_2_0_s19 \n");
      #endif
      ptrRAM = (char*) STM8_Routines_E_W_ROUTINEs_128K_ver_2_0_s19;
      lenRAM = STM8_Routines_E_W_ROUTINEs_128K_ver_2_0_s19_len;
    }
    #ifdef DONIX
    else if ((flashsize==128) && (versBSL==0x20)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_32K_verL_1_0_s19 \n");
      #endif
      ptrRAM = (char*) STM8_Routines_E_W_ROUTINEs_32K_verL_1_0_s19;
      lenRAM = STM8_Routines_E_W_ROUTINEs_32K_verL_1_0_s19_len;
    }
    #endif // DONIX
    else if ((flashsize==128) && (versBSL==0x21)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_128K_ver_2_1_s19 \n");
      #endif
      ptrRAM = (char*) STM8_Routines_E_W_ROUTINEs_128K_ver_2_1_s19;
      lenRAM = STM8_Routines_E_W_ROUTINEs_128K_ver_2_1_s19_len;
    }
    else if ((flashsize==128) && (versBSL==0x22)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_128K_ver_2_2_s19 \n");
      #endif
      ptrRAM = (char*) STM8_Routines_E_W_ROUTINEs_128K_ver_2_2_s19;
      lenRAM = STM8_Routines_E_W_ROUTINEs_128K_ver_2_2_s19_len;
    }
    else if ((flashsize==128) && (versBSL==0x24)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_128K_ver_2_4_s19 \n");
      #endif
      ptrRAM = (char*) STM8_Routines_E_W_ROUTINEs_128K_ver_2_4_s19;
      lenRAM = STM8_Routines_E_W_ROUTINEs_128K_ver_2_4_s19_len;
    }
    else if ((flashsize==256) && (versBSL==0x10)) {
      #ifdef DEBUG
        printf("header STM8_Routines_E_W_ROUTINEs_256K_ver_1_0_s19 \n");
      #endif
      ptrRAM = (char*) STM8_Routines_E_W_ROUTINEs_256K_ver_1_0_s19;
      lenRAM = STM8_Routines_E_W_ROUTINEs_256K_ver_1_0_s19_len;
    }
    else
      Error("unsupported device");


    // convert correct array containing s19 file to RAM image
    convert_s19(ptrRAM, lenRAM, imageBuf, &addrStart, &addrStop, MUTE);
      
    // upload RAM routines to STM8
    if ((verbose == INFORM) || (verbose == CHATTY))
      printf("  upload RAM routines ... ");
    fflush(stdout);
    bsl_memWrite(ptrPort, physInterface, uartMode, addrStart, addrStop, imageBuf, -1);
    if (verbose == INFORM)
      printf("ok\n");
    if (verbose == CHATTY)
      printf("ok (%dB from 0x%04x)\n", addrStop-addrStart+1, addrStart);
    fflush(stdout);
  
  } // if STM8S or low-density STM8L -> upload RAM code



  /////////////////
  // 2nd pass of commandline arguments: execute actions, e.g. upload and download files
  /////////////////

  for (int i=1; i<argc; i++) {
    
    // debug
    //printf("\nargv[%d] = '%s'\n", i, argv[i]);

    // skip interface with 1 parameter, is handled in 1st run
    if ((!strcmp(argv[i], "--interface")) || (!strcmp(argv[i], "-i"))) {
      i += 1;
    }


    // skip communication port with 1 parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "--port")) || (!strcmp(argv[i], "-p"))) {
      i += 1;
    }

      
    // skip communication baudrate with 1 parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "--baudrate")) || (!strcmp(argv[i], "-b"))) {
      i += 1;
    }

      
    // skip UART mode with 1 parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "--uartmode")) || (!strcmp(argv[i], "-u"))) {
      i += 1;
    }

      
    // skip reset method with 1 parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "--reset")) || (!strcmp(argv[i], "-R"))) {
      i += 1;
    }

      
    // skip verify flag w/o parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "--noverify")) || (!strcmp(argv[i], "-v"))) {
      i += 0;   // dummy
    }
    

    // upload file -> perform here
    else if ((!strcmp(argv[i], "--write")) || (!strcmp(argv[i], "-w"))) {

      // intermediate variables
      char      infile[STRLEN]="";     // name of input file
      char      *fileBuf;              // RAM buffer for input file
      uint32_t  lenFile;               // length of file in fileBuf
      
      // allocate intermediate buffers (>1MByte requires dynamic allocation)
      if (!(fileBuf = malloc(LENFILEBUF * sizeof(*fileBuf))))
        Error("Cannot allocate file buffer, try reducing LENFILEBUF");

      // get file name
      strncpy(infile, argv[++i], STRLEN-1);
      
      // for binary file also get starting address
      if (strstr(infile, ".bin") != NULL) {
        strncpy(tmp, argv[++i], STRLEN-1);
        sscanf(tmp, "%x", &addrStart);
      }

      // import file into string buffer (no interpretation, yet)
      load_file(infile, fileBuf, &lenFile, verbose);

      // convert to memory image, depending on file type 
      if (strstr(infile, ".s19") != NULL)   // Motorola S-record format
        convert_s19(fileBuf, lenFile, imageBuf, &addrStart, &addrStop, verbose);
      else if ((strstr(infile, ".hex") != NULL) || (strstr(infile, ".ihx") != NULL))   // Intel HEX-format
        convert_ihx(fileBuf, lenFile, imageBuf, &addrStart, &addrStop, verbose);
      else if (strstr(infile, ".txt") != NULL)   // text table (hex addr / data)
        convert_txt(fileBuf, lenFile, imageBuf, &addrStart, &addrStop, verbose);
      else if (strstr(infile, ".bin") != NULL)   // binary file
        convert_bin(fileBuf, lenFile, imageBuf, addrStart, &addrStop, verbose);
      else
        Error("Input file %s has unsupported format (*.s19, *.hex, *.ihx, *.txt, *.bin)", infile);

      // upload memory image to STM8
      bsl_memWrite(ptrPort, physInterface, uartMode, addrStart, addrStop, imageBuf, verbose);
    
      // optionally verify upload
      if (verifyUpload) {
        
        // allocate temporary RAM buffer (>1MByte requires dynamic allocation)
        uint16_t  *tmpImageBuf;            // RAM image buffer (high byte != 0 indicates value is set)
        if (!(tmpImageBuf = malloc(LENIMAGEBUF * sizeof(*tmpImageBuf))))
          Error("Cannot allocate image buffer, try reducing LENIMAGEBUF");

        // read back from STM8 
        bsl_memRead(ptrPort, physInterface, uartMode, addrStart, addrStop, tmpImageBuf, verbose);
        
        // debug
        //print_console(imageBuf, addrStart, addrStop, INFORM);
        //print_console(tmpImageBuf, addrStart, addrStop, INFORM);

        // compare data
        if (verbose != MUTE)
          printf("  verify memory ... ");
        for (int idx=0; idx<addrStop-addrStart+1; idx++) {
          if ((imageBuf[idx] & 0xFF) != (tmpImageBuf[idx] & 0xFF))
            Error("verify failed at address 0x%04x (0x%02x vs 0x%02x)", (uint32_t) (addrStart+idx), (uint8_t) (imageBuf[idx]&0xFF), (uint8_t) (tmpImageBuf[idx]&0xFF));
        }
        if (verbose != MUTE)
          printf("ok\n");
        
        // release temporary RAM buffer
        free(tmpImageBuf);

      } // verify

    } // write
      

    // set value at given address -> perform here
    else if ((!strcmp(argv[i], "--set")) || (!strcmp(argv[i], "-s"))) {

      // intermediate variables
      int    valTmp;

      // get address and value and store to parameters for bsl_memWrite
      sscanf(argv[++i], "%x", &valTmp);   addrStart = valTmp; addrStop = addrStart;
      sscanf(argv[++i], "%x", &valTmp);   imageBuf[0] = (uint16_t) (valTmp | 0xFF00);

      // upload memory image to STM8
      bsl_memWrite(ptrPort, physInterface, uartMode, addrStart, addrStop, imageBuf, verbose);
    
      // optionally verify upload
      if (verifyUpload) {
        
        // allocate temporary RAM buffer (>1MByte requires dynamic allocation)
        uint16_t  *tmpImageBuf;            // RAM image buffer (high byte != 0 indicates value is set)
        if (!(tmpImageBuf = malloc(LENIMAGEBUF * sizeof(*tmpImageBuf))))
          Error("Cannot allocate image buffer, try reducing LENIMAGEBUF");

        // read back from STM8 
        bsl_memRead(ptrPort, physInterface, uartMode, addrStart, addrStop, tmpImageBuf, verbose);
        
        // debug
        //print_console(imageBuf, addrStart, addrStop, INFORM);
        //print_console(tmpImageBuf, addrStart, addrStop, INFORM);

        // compare data
        if (verbose != MUTE)
          printf("  verify memory ... ");
        for (int idx=0; idx<addrStop-addrStart+1; idx++) {
          if ((imageBuf[idx] & 0xFF) != (tmpImageBuf[idx] & 0xFF))
            Error("verify failed at address 0x%04x (0x%02x vs 0x%02x)", (uint32_t) (addrStart+idx), (uint8_t) (imageBuf[idx]&0xFF), (uint8_t) (tmpImageBuf[idx]&0xFF));
        }
        if (verbose != MUTE)
          printf("ok\n");
        
        // release temporary RAM buffer
        free(tmpImageBuf);

      } // verify

    } // set
      

    // read address range -> perform here
    else if ((!strcmp(argv[i], "--read")) || (!strcmp(argv[i], "-r"))) {

      // intermediate variables
      char   outfile[STRLEN]="";     // name of export file
      int    addrTmp;

      // get start & stop address, and export filename
      sscanf(argv[++i], "%x", &addrTmp);   addrStart = addrTmp;
      sscanf(argv[++i], "%x", &addrTmp);   addrStop  = addrTmp;
      strncpy(outfile, argv[++i], STRLEN-1);
      
      // read memory
      bsl_memRead(ptrPort, physInterface, uartMode, addrStart, addrStop, imageBuf, verbose);
  
      // export in format depending on file extension 
      if (strstr(outfile, ".s19") != NULL)   // Motorola S-record format
        export_s19(outfile, imageBuf, addrStart, addrStop, verbose);
      else if (strstr(outfile, ".txt") != NULL)   // text table (hex addr / data)
        export_txt(outfile, imageBuf, addrStart, addrStop, verbose);
      else if (strstr(outfile, ".bin") != NULL)   // binary format
        export_bin(outfile, imageBuf, addrStart, addrStop, verbose);
      else if (!strcmp(outfile, "console"))       // print
        print_console(imageBuf, addrStart, addrStop, verbose);
      else
        Error("Unsupported output file extension of '%s' (*.s19, *.txt)", outfile);

    } // read


    // mass erase -> perform here
    else if ((!strcmp(argv[i], "--erase")) || (!strcmp(argv[i], "-e"))) {

      // trigger flash mass erase
      bsl_flashMassErase(ptrPort, physInterface, uartMode, verbose);

    } // erase

      
    // skip jump adress with 1 parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "--jump")) || (!strcmp(argv[i], "-j"))) {
      i += 1;
    }


    // skip set verbosity level with 1 parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "--verbose")) || (!strcmp(argv[i], "-V"))) {
      i += 1;
    }


    // skip background flag w/o parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "--background")) || (!strcmp(argv[i], "-B"))) {
      i += 0;   // dummy
    }


    // skip exit prompt flag w/o parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "--exitPrompt")) || (!strcmp(argv[i], "-q"))) {
      i += 0;   // dummy
    }

    // dummy parameter: skip, is treated in 1st pass
    else {
      // dummy
      printf("\ntest: '%s'\n", argv[i]);
    }

  } // 2nd pass over commandline arguments
  

  ////////
  // jump to address prior to exit (default: beginning of P-flash=0x8000). Skip if 0xFFFFFFFF
  ////////
  if (jumpAddr != 0xFFFFFFFF) {

    // don't know why, but seems to be required for SPI
    #if defined(USE_SPIDEV)
      if ((physInterface==SPI_SPIDEV) || (physInterface==SPI_ARDUINO))
        SLEEP(500);
    #else
      if (physInterface==SPI_ARDUINO)
        SLEEP(500);
    #endif
    
    // jumpt to application
    bsl_jumpTo(ptrPort, physInterface, uartMode, jumpAddr, verbose);
  
  } // jump to STM8 address


  // print message
  if (verbose != MUTE)
    printf("done with program\n");

  // release global buffer
  free(imageBuf);

  // close communication port
  close_port(&ptrPort);

  // terminate program
  Exit(0, g_pauseOnExit);

  // avoid compiler warnings
  return(0);
  
} // main

