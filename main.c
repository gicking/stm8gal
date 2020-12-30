/**
   \file main.c

   \author G. Icking-Konert
   \date 2019-01-14
   \version 0.3

   \brief implementation of main routine

   this is the main file containing browsing the input parameters,
   calling the import, programming, and check routines.

   \note program not yet fully tested!

*/

// include files
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdbool.h>

#if !defined(_MSC_VER)
  #include <unistd.h>
  #include <sys/time.h>
#endif

#include <time.h>

// OS specific: Windows
#if defined(WIN32) || defined(WIN64)
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
#include "serial_comm.h"
#if defined(USE_SPIDEV)
  #include "spi_spidev_comm.h"
#endif
#if defined (USE_SPI_ARDUINO)
  #include "spi_Arduino_comm.h"
#endif
#include "bootloader.h"
#include "hexfile.h"
#include "verify_CRC32.h"
#include "version.h"
#include "console.h"
#include "timer.h"


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

/// wait for \<return\> prior to closing console window
static bool           g_pauseOnExit;

/**
  \fn void get_version(uint16_t vers, uint8_t *major, uint8_t *minor, uint8_t *build, uint8_t *status)

  \param[in]  vers      16b revision number in format xx.xxxxxxxx.xxxxx.x
  \param[out] major     major revision number [15:14] -> 0..3
  \param[out] minor     minor revision number [13:6] -> 0..255
  \param[out] build     build number [5:1] -> 0..31
  \param[out] status    status [0] -> 0=beta; 1=released

  extract major / minor / build revision number from 16b identifier
  in format xx.xxxxxxxx.xxxxx.x
*/
void get_version(uint16_t vers, uint8_t *major, uint8_t *minor, uint8_t *build, uint8_t *status) {

  // major version ([15:14] -> 0..7)
  *major = (uint8_t) ((vers & 0xC000) >> 14);

  // minor version ([13:6] -> 0..255)
  *minor = (uint8_t) ((vers & 0x3FC0) >> 6);

  // build number ([5:1] -> 0..31)
  *build = (uint8_t) ((vers&0x003E) >> 1);

  // release status ([0] -> 0=beta; 1=released)
  *status = (uint8_t) (vers&0x0001);

} // get_version



/**
  \fn void get_app_name(char *appFull, uint16_t versID, char *appName, char *versStr)

  \param[in]  appFull   name of application incl. path
  \param[in]  versID    16b version identifier
  \param[out] appName   name of application w/o path
  \param[out] versStr   version number as string

  print application name and major / minor / build revision numbers. Remove path from app name
*/
void get_app_name(char *appFull, uint16_t versID, char *appName, char *versStr) {

  int32_t   i;
  char      *tmp;
  uint8_t   major, minor, build, status;

  // find position of last path delimiter '/' (Posix) or '\' (Win)
  tmp = appFull;
  for (i=0; i<strlen(appFull); i++) {
    if ((appFull[i] == '/') || (appFull[i] == '\\'))
      tmp = appFull+i+1;
  }

  // copy app name w/o path
  sprintf(appName, "%s", tmp);


  // extract major / minor / build revision number
  get_version(versID, &major, &minor, &build, &status);

  // copy version data to string
  if (status==0)
    sprintf(versStr, "v%d.%d.%d beta", major, minor, build);
  else
    sprintf(versStr, "%d.%d.%d", major, minor, build);

} // get_app_name



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
  char      portname[STRLEN]="";  // name of communication port
  HANDLE    ptrPort = 0;          // handle to communication port
  int       baudrate;             // communication baudrate [Baud]
  uint8_t   uartMode;             // UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply, other=auto-detect
  int       resetSTM8;            // reset STM8: 0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8, 5=Raspi pin 12, 6=RTS line (RS232) (default: manual)
  uint16_t  *imageBuf;            // global RAM image buffer (high byte != 0 indicates value is set)
  int       verifyUpload;         // verify method after upload (0=skip, 1=CRC32, 2=read-out)
  uint64_t  jumpAddr;             // address to jump to before exit program
  bool      printHelp;            // flag for printing help page
  int       paramHelp=-1;         // parameter index to print help for
  int       i, j;                 // generic variables
  char      tmp[STRLEN];          // misc buffer
  uint64_t  addrStart, addrStop, numData;  // image data range

  // STM8 device properties
  int       flashsize;            // size of flash (kB) for w/e routines
  uint8_t   versBSL;              // BSL version for w/e routines
  uint8_t   family;               // device family, currently STM8S and STM8L


  // initialize global variables
  g_pauseOnExit         = false;  // no wait for <return> before terminating (dummy)
  Console_SetBackgroundOperation( false );  // assume foreground application

  // initialize default arguments
  portname[0]    = '\0';          // no default port name
  physInterface  = UART;          // bootloader interface: 0=UART (default), 1=SPI_ARDUINO, 2=SPI_SPIDEV
  uartMode       = 255;           // UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply, other=auto-detect
  baudrate       = 115200;        // default baudrate
  verbose        = INFORM;        // verbosity level medium
  resetSTM8      = 1;             // manual reset of STM8
  verifyUpload   = 2;             // read back memory after upload  (0=skip, 1=CRC32, 2=read-out)
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


  // initialize time-keeping (1st call stores launch time)
  micros();

  // get app name & version, and change console title
  get_app_name(argv[0], VERSION, appname, version);


  /////////////////
  // 1st pass of commandline arguments: set global parameters, no upload/download/erase yet
  /////////////////

  printHelp = false;
  for (i=1; i<argc; i++) {

    // print help
    if ((!strcmp(argv[i], "-h")) || (!strcmp(argv[i], "-help"))) {

      // set flag for printing help
      printHelp = true;
      break;

    } // help


    // set verbosity level (0..2)
    else if ((!strcmp(argv[i], "-v")) || (!strcmp(argv[i], "-verbose"))) {

      // get verbosity level
      if (i+1<argc)
        sscanf(argv[++i],"%d",&verbose);
      else {
        printHelp = true;
        break;
      }
      if (verbose < MUTE)   verbose = MUTE;
      if (verbose > CHATTY) verbose = CHATTY;

    } // verbose


    // optimize for background operation, e.g. skip prompts and colors
    else if ((!strcmp(argv[i], "-B")) || (!strcmp(argv[i], "-background"))) {
      Console_SetBackgroundOperation(true);
    } // background


    // prompt for <return> prior to exit
    else if ((!strcmp(argv[i], "-q")) || (!strcmp(argv[i], "-exit-prompt"))) {
      g_pauseOnExit = true;
    } // exit-prompt


    // reset method: 0=skip, 1=manual; 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8, 5=Raspi pin 12, 6=RTS line (RS232)
    else if ((!strcmp(argv[i], "-R")) || (!strcmp(argv[i], "-reset"))) {

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


    // get interface type: 0=UART (default), 1=SPI_ARDUINO, 2=SPI_SPIDEV
    else if ((!strcmp(argv[i], "-i")) || (!strcmp(argv[i], "-interface"))) {

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


    // UART mode
    else if ((!strcmp(argv[i], "-u")) || (!strcmp(argv[i], "-uart-mode"))) {

      // get UART mode
      if (i+1<argc) {
        sscanf(argv[++i], "%d", &j);
        uartMode = j;
      }
      else {
        printHelp = true;
        break;
      }

    } // uart_mode


    // name of communication port
    else if ((!strcmp(argv[i], "-p")) || (!strcmp(argv[i], "-port"))) {

      // get port name
      if (i+1<argc)
        strncpy(portname, argv[++i], STRLEN-1);
      else {
        printHelp = true;
        break;
      }

    } // port


    // communication baudrate
    else if ((!strcmp(argv[i], "-b")) || (!strcmp(argv[i], "-baudrate"))) {

      // get communication baudrate
      if (i+1<argc)
        sscanf(argv[++i],"%d",&baudrate);
      else {
        printHelp = true;
        break;
      }

    } // baudrate


    // set verify method after upload
    else if ((!strcmp(argv[i], "-V")) || (!strcmp(argv[i], "-verify"))) {

      // get memory verify method
      if (i+1<argc)
        sscanf(argv[++i],"%d",&verifyUpload);
      else {
        printHelp = true;
        break;
      }
      if (verifyUpload < 0) verifyUpload = 0;
      if (verifyUpload > 2) verifyUpload = 2;

    } // verify method


    // jump adress before program termination (-1 or 0xFFFFFFFF == skip jump)
    else if ((!strcmp(argv[i], "-j")) || (!strcmp(argv[i], "-jump-addr"))) {

      // get jump address (0x indicates hex, else decimal)
      if (i+1<argc) {
        uint64_t addr;
        strncpy(tmp, argv[++i], STRLEN-1);
        if (strstr(tmp, "0x") != NULL)
          sscanf(tmp, "%" SCNx64, &addr);   // read as hex
        else
          sscanf(tmp, "%" SCNu64, &addr);   // read as dec
        //printf("\n0x%" PRIx64 "\t%d\n", addr, (int) addr); exit(1);
        jumpAddr = addr;
      }
      else {
        printHelp = true;
        break;
      }

    } // jump-address


    // skip file upload. Just check parameter number
    else if ((!strcmp(argv[i], "-w")) || (!strcmp(argv[i], "-write-file"))) {

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


    // skip writing single value. Just check parameter number
    else if ((!strcmp(argv[i], "-W")) || (!strcmp(argv[i], "-write-byte"))) {
      if (i+2<argc)
        i+=2;
      else {
        printHelp = true;
        break;
      }
    } // write-byte


    // skip reading address range. Just check parameter number
    else if ((!strcmp(argv[i], "-r")) || (!strcmp(argv[i], "-read"))) {
      if (i+3<argc)
        i+=3;
      else {
        printHelp = true;
        paramHelp = i;
        break;
      }
    } // read


    // skip flash sector erase. Just check parameter number
    else if ((!strcmp(argv[i], "-e")) || (!strcmp(argv[i], "-erase-sector"))) {
      if (i+1<argc)
        i+=1;
      else {
        printHelp = true;
        break;
      }
    } // erase-sector


    // skip flash mass erase
    else if ((!strcmp(argv[i], "-E")) || (!strcmp(argv[i], "-erase-full"))) {
      // dummy
    } // erase-full


    // else print help
    else {
      printHelp = true;
      paramHelp = i;
      break;
    }

  } // 1st pass over commandline arguments


  // on request (-h) or in case of parameter error print help page
  if ((printHelp==true) || (argc == 1)) {

    sprintf(tmp, "%s (v%s)", appname, version);
    setConsoleTitle(tmp);

    printf("\n");
    printf("\n%s (v%s)\n\n", appname, version);
    printf("Program or read STM8 memory via built-in UART or SPI bootloader.\n");
    printf("For more information see https://github.com/gicking/stm8gal\n");
    printf("\n");
    printf("usage: %s with following options/commands:\n", appname);
    printf("    -h/-help                        print this help\n");
    printf("    -v/-verbose [level]             set verbosity level 0..3 (default: 2)\n");
    printf("    -B/-background                  skip prompts and colors for background operation (default: foreground)\n");
    printf("    -q/-exit-prompt                 prompt for <return> prior to exit (default: no prompt)\n");
    #if defined(__ARMEL__) && defined(USE_WIRING)
      printf("    -R/-reset [rst]                 reset for STM8: 0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin pin 8, 5=Raspi pin 12, 6=RTS line (RS232) (default: manual)\n");
    #else
      printf("    -R/-reset [rst]                 reset for STM8: 0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin pin 8, 6=RTS line (RS232) (default: manual)\n");
    #endif
    printf("    -i/-interface [line]            communication interface: 0=UART");
    #if defined(USE_SPI_ARDUINO)
      printf(", 1=SPI via Arduino");
    #endif
    #if defined(USE_SPIDEV)
      printf(", 2=SPI via spidev");
    #endif
    printf(" (default: UART)\n");
    printf("    -u/-uart-mode [mode]            UART mode: 0=duplex, 1=1-wire, 2=2-wire reply, other=auto-detect (default: auto-detect)\n");
    printf("    -p/-port [name]                 communication port (default: list available ports)\n");
    printf("    -b/-baudrate [speed]            communication baudrate in Baud (default: 115200)\n");
    printf("    -V/-verify                      verify flash content after upload: 0=skip, 1=CRC32 checksum, 2=read back (default: read back)\n");
    printf("    -j/-jump-addr [address]         jump to address (as dec or hex) before exit of %s, or -1 for skip (default: flash)\n", appname);
    printf("    -w/-write-file [file [addr]]    upload file from PC to uController. For binary file (*.bin) with address offset (as dec or hex)\n");
    printf("    -W/-write-byte [addr value]     change value at given address (both as dec or hex)\n");
    printf("    -r/-read [start stop output]    read memory range (as dec or hex) and save to file or print (output=console)\n");
    printf("    -e/-erase-sector [addr]         erase flash sector containing given address (as dec or hex). Use carefully!\n");
    printf("    -E/-erase-full                  mass erase complete flash. Use carefully!\n");
    printf("\n");
    printf("Supported import formats:\n");
    printf("  - Motorola S19 (*.s19), see https://en.wikipedia.org/wiki/SREC_(file_format)\n");
    printf("  - Intel Hex (*.hex, *.ihx), see https://en.wikipedia.org/wiki/Intel_HEX\n");
    printf("  - ASCII table (*.txt) consisting of lines with 'addr  value' (dec or hex). Lines starting with '#' are ignored\n");
    printf("  - Binary data (*.bin) with an additional starting address\n");
    printf("\n");
    printf("Supported export formats:\n");
    printf("  - print to stdout (console)\n");
    printf("  - Motorola S19 (*.s19)\n");
    printf("  - Intel Hex (*.hex, *.ihx)\n");
    printf("  - ASCII table (*.txt) with 'hexAddr  hexValue'\n");
    printf("  - Binary data (*.bin) without starting address\n");
    printf("\n");
    printf("Data is uploaded and exported in the specified order, i.e. later uploads may\n");
    printf("overwrite previous uploads. Also exports only contain the previous uploads, i.e.\n");
    printf("intermediate exports only contain the memory content up to that point in time.\n");
    printf("\n");

    // in case of a wrong parameter print index
    if (paramHelp > 0)
      printf("\nerror occurred in parameter %d\n", paramHelp);

    Exit(0,0);
  }


  ////////
  // perform some misc tasks
  ////////

  // read back after writing doesn't work for SPI (don't know why)
  #if defined(USE_SPIDEV)
    if (physInterface == SPI_SPIDEV)
      verifyUpload = 0;
  #endif
  #if defined(USE_SPI_ARDUINO)
    if (physInterface == SPI_ARDUINO)
      verifyUpload = 0;
  #endif

  // for background operation avoid prompt on exit
  if (Console_GetBackgroundOperation())
    g_pauseOnExit = false;

  if (Console_GetBackgroundOperation()) {
    sprintf(tmp, "%s (v%s)", appname, version);
    setConsoleTitle(tmp);
  }

  // reset console color (needs to be called once for Win32)
  setConsoleColor(PRM_COLOR_DEFAULT);

  // allocate and init global RAM image (>1MByte requires dynamic allocation)
  if (!(imageBuf = malloc((LENIMAGEBUF + 1) * sizeof(*imageBuf))))
    Error("Cannot allocate image buffer, try reducing LENIMAGEBUF");
  memset(imageBuf, 0, (LENIMAGEBUF + 1) * sizeof(*imageBuf));


  /////////////////
  // initiate communication with STM8 bootloader
  /////////////////

  // print message
  if (verbose != MUTE)
    printf("\n%s (v%s)\n", appname, version);


  ////////
  // if no port name is given, list all available ports and query
  ////////
  if (strlen(portname) == 0) {
    if (!Console_GetBackgroundOperation()) {
      printf("  enter comm port name ( ");
      if (list_ports() != STM8GAL_SERIALCOMMS_NO_ERROR)
        Error("Fatal Error.");
      printf(" ): ");
      scanf("%s", portname);
      getchar();
    }
    else {
      printf("  available comm ports ( ");
      if (list_ports() != STM8GAL_SERIALCOMMS_NO_ERROR)
        Error("Fatal Error.");
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
    if (!Console_GetBackgroundOperation()) {
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
    if (verbose != MUTE)
      printf("  reset via DTR ... ");
    fflush(stdout);
    if (init_port(&ptrPort, portname, 115200, 100, 8, 0, 1, 0, 0) != STM8GAL_SERIALCOMMS_NO_ERROR)
      Error("Fatal Error.");
    pulse_DTR(ptrPort, 10);
    close_port(&ptrPort);
    if (verbose != MUTE)
      printf("ok\n");
    fflush(stdout);
    SLEEP(20);                        // allow BSL to initialize
  }

  // SW reset STM8 via command 'Re5eT!' at 115.2kBaud with (8,0,1) (requires respective STM8 SW)
  else if (resetSTM8 == 3) {
    char buf[10] = "Re5eT!";          // reset command (same as in STM8 SW!)
    if (verbose != MUTE)
      printf("  reset via UART command ... ");
    fflush(stdout);
    if (init_port(&ptrPort, portname, 115200, 100, 8, 0, 1, 0, 0) != STM8GAL_SERIALCOMMS_NO_ERROR)
      Error("Fatal Error");
    for (i=0; i<6; i++) {
      uint32_t tmp32;
      // send reset command bytewise to account for possible slow handling on STM8 side
      if (send_port(ptrPort, 0, 1, buf+i, &tmp32) != STM8GAL_SERIALCOMMS_NO_ERROR)
        Error("Fatal Error");
      SLEEP(10);
    }
    close_port(&ptrPort);
    if (verbose != MUTE)
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
      if (verbose != MUTE)
        printf("  reset via Raspi pin 12 ... ");
      fflush(stdout);
      pulse_GPIO(12, 20);
      if (verbose != MUTE)
        printf("ok\n");
      fflush(stdout);
      SLEEP(20);                      // allow BSL to initialize
    }
  #endif // __ARMEL__ && USE_WIRING

  // HW reset STM8 using RTS line (USB/RS232)
    else if(resetSTM8 == 6)
    {
        if(verbose != MUTE)
            printf("  reset via RTS ... ");
        fflush(stdout);
        if(init_port(&ptrPort, portname, 115200, 100, 8, 0, 1, 0, 0) != STM8GAL_SERIALCOMMS_NO_ERROR)
          Error("Fatal Error.");
        pulse_RTS(ptrPort, 10);
        close_port(&ptrPort);
        if(verbose != MUTE)
            printf("ok\n");
        fflush(stdout);
        SLEEP(20); // allow BSL to initialize
    }

  // unknown reset method -> error
  else {
    #ifdef __ARMEL__
      Error("reset method %d not supported (0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8, 5=Raspi pin 12, 6=RTS line (RS232))", resetSTM8);
    #else
      Error("reset method %d not supported (0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8, 6=RTS line (RS232))", resetSTM8);
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
    // start without parity, may be changed in bsl_sync()
    if (init_port(&ptrPort, portname, baudrate, TIMEOUT, 8, 0, 1, 0, 0) != STM8GAL_SERIALCOMMS_NO_ERROR)
      Error("Fatal Error");
    if ((verbose == INFORM) || (verbose == CHATTY))
      printf("done\n");
    fflush(stdout);

  } // UART

  // SPI via Arduino
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO) {

      // open port
      if (verbose == INFORM)
        printf("  open Arduino port '%s' ... ", portname);
      else if (verbose == CHATTY)
        printf("  open Arduino port '%s' with %gkBaud SPI ... ", portname, (float) ARDUINO_BAUDRATE / 1000.0);
      fflush(stdout);
      if (init_port(&ptrPort, portname, ARDUINO_BAUDRATE, 100, 8, 0, 1, 0, 0) != STM8GAL_SERIALCOMMS_NO_ERROR)
          Error("Fatal Error");
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
  #endif // USE_SPI_ARDUINO

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
    #if defined(USE_SPI_ARDUINO) && defined(USE_SPIDEV)
      Error("interface %d not supported (0=UART, 1=SPI via Arduino, 2=SPI via spidev)", physInterface);
    #elif defined(USE_SPI_ARDUINO)
      Error("interface %d not supported (0=UART, 1=SPI via Arduino)", physInterface);
    #elif defined(USE_SPIDEV)
      Error("interface %d not supported (0=UART, 2=SPI via spidev)", physInterface);
    #else
	  Error("interface %d not supported (0=UART)", physInterface);
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
  SLEEP(200);
  flush_port(ptrPort);

  // synchronize with bootloader. For UART also sync baudrate
  if (bsl_sync(ptrPort, physInterface, verbose) != STM8GAL_BOOTLOADER_NO_ERROR)
    Error("Fatal Error");

  // for UART set or auto-detect UART mode (0=duplex, 1=1-wire, 2=2-wire reply, others=auto-detect)
  if (physInterface == UART) {
    if (uartMode == 0) {
      set_parity(ptrPort, 2);
      if (verbose != MUTE)
        printf("  set UART mode: duplex\n");
    }
    else if (uartMode == 1) {
      set_parity(ptrPort, 0);
      if (verbose != MUTE)
        printf("  set UART mode: 1-wire\n");
    }
    else if (uartMode == 2) {
      char c = ACK;      // need to reply ACK first to revert bootloader
      uint32_t tmp32;
      set_parity(ptrPort, 0);
      if (send_port(ptrPort, 0, 1, &c, &tmp32) != STM8GAL_SERIALCOMMS_NO_ERROR)
        Error("Fatal Error");
      if (verbose != MUTE)
        printf("  set UART mode: 2-wire reply\n");
    }
    else
      if (bsl_getUartMode(ptrPort, &uartMode, verbose) != STM8GAL_BOOTLOADER_NO_ERROR )
        Error("Fatal Error");
  } // UART interface
  fflush(stdout);

  // get bootloader info for selecting RAM w/e routines for flash
  if (bsl_getInfo(ptrPort, physInterface, uartMode, &flashsize, &versBSL, &family, verbose) != STM8GAL_BOOTLOADER_NO_ERROR)
    Error("Fatal Error");


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


    // clear image buffer
    memset(imageBuf, 0, (LENIMAGEBUF + 1) * sizeof(*imageBuf));

    // convert correct array containing s19 file to RAM image
    if ( hexfile_convertS19(ptrRAM, lenRAM, imageBuf, MUTE) != STM8GAL_HEXFILE_NO_ERROR )
      Error("Fatal Error");

    // get image size
    if ( hexfile_getImageSize(imageBuf, 0, LENIMAGEBUF, &addrStart, &addrStop, &numData) != STM8GAL_HEXFILE_NO_ERROR )
      Error("Fatal Error");

    // upload RAM routines to STM8
    if (verbose == CHATTY)
      printf("  upload RAM routines ... ");
    fflush(stdout);
    if (bsl_memWrite(ptrPort, physInterface, uartMode, imageBuf, addrStart, addrStop, MUTE) != STM8GAL_BOOTLOADER_NO_ERROR)
      Error("Fatal Error");

    if (verbose == CHATTY)
      printf("done (%dB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (int) numData, addrStart, addrStop);
    fflush(stdout);

    // clear memory image again
    memset(imageBuf, 0, (LENIMAGEBUF + 1) * sizeof(*imageBuf));

  } // if STM8S or low-density STM8L -> upload RAM code



  /////////////////
  // 2nd pass of commandline arguments: execute actions, e.g. upload and download files
  /////////////////

  for (i=1; i<argc; i++) {

    // debug
    //printf("\nargv[%d] = '%s'\n", i, argv[i]);

    // skip print help (already treated in 1st pass)
    if ((!strcmp(argv[i], "-h")) || (!strcmp(argv[i], "-help"))) {
      i += 0;   // dummy
    } // help


    // skip verbosity level and parameters (already treated in 1st pass)
    else if ((!strcmp(argv[i], "-v")) || (!strcmp(argv[i], "-verbose"))) {
        i+=1;
    } // verbose


    // skip background flag w/o parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "-B")) || (!strcmp(argv[i], "-background"))) {
      i += 0;   // dummy
    }


    // skip exit prompt flag w/o parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "-q")) || (!strcmp(argv[i], "-exit-prompt"))) {
      i += 0;   // dummy
    }

    // skip reset method with 1 parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "-R")) || (!strcmp(argv[i], "-reset"))) {
      i += 1;
    }


    // skip interface with 1 parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "-i")) || (!strcmp(argv[i], "-interface"))) {
      i += 1;
    }


    // skip UART mode with 1 parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "-u")) || (!strcmp(argv[i], "-uart-mode"))) {
      i += 1;
    }


    // skip communication port with 1 parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "-p")) || (!strcmp(argv[i], "-port"))) {
      i += 1;
    }


    // skip communication baudrate with 1 parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "-b")) || (!strcmp(argv[i], "-baudrate"))) {
      i += 1;
    }


    // skip verify method with 1 parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "-V")) || (!strcmp(argv[i], "-verify"))) {
      i += 1;
    }


    // skip jump adress with 1 parameter, is handled in 1st run
    else if ((!strcmp(argv[i], "-j")) || (!strcmp(argv[i], "-jump-addr"))) {
      i += 1;
    }


    // upload file -> perform here
    else if ((!strcmp(argv[i], "-w")) || (!strcmp(argv[i], "-write-file"))) {

      // intermediate variables
      char      infile[STRLEN]="";     // name of input file
      char      *fileBuf;              // RAM buffer for input file
      uint64_t  lenFile;               // length of file in fileBuf

      // allocate intermediate buffers (>1MByte requires dynamic allocation)
      if (!(fileBuf = malloc(LENFILEBUF * sizeof(*fileBuf))))
        Error("Cannot allocate file buffer, try reducing LENFILEBUF");

      // get file name
      strncpy(infile, argv[++i], STRLEN-1);

      // for binary file also get starting address
      if (strstr(infile, ".bin") != NULL) {
        strncpy(tmp, argv[++i], STRLEN-1);
        if (strstr(tmp, "0x") != NULL)
          sscanf(tmp, "%" SCNx64, &addrStart);   // read as hex
        else
          sscanf(tmp, "%" SCNu64, &addrStart);   // read as dec
        //printf("\n0x%" PRIx64 "\t%d\n", addrStart, (int) addrStart); exit(1);
      }

      // import file into string buffer (no interpretation, yet)
      if (hexfile_loadFile(infile, fileBuf, &lenFile, verbose) != STM8GAL_HEXFILE_NO_ERROR)
          Error("Fatal Error");

      // clear image buffer
      memset(imageBuf, 0, (LENIMAGEBUF + 1) * sizeof(*imageBuf));

      // convert to memory image, depending on file type
      if (strstr(infile, ".s19") != NULL) { // Motorola S-record format
        if (hexfile_convertS19(fileBuf, lenFile, imageBuf, verbose) != STM8GAL_HEXFILE_NO_ERROR)
          Error("Fatal Error");
      }
      else if ((strstr(infile, ".hex") != NULL) || (strstr(infile, ".ihx") != NULL)) { // Intel HEX-format
        if (hexfile_convertIHex(fileBuf, lenFile, imageBuf, verbose) != STM8GAL_HEXFILE_NO_ERROR)
          Error("Fatal Error");
      }
      else if (strstr(infile, ".txt") != NULL) { // text table (Addr / Data)
        if (hexfile_convertTxt(fileBuf, lenFile, imageBuf, verbose) != STM8GAL_HEXFILE_NO_ERROR)
          Error("Fatal Error");
      }
      else if (strstr(infile, ".bin") != NULL) { // binary file
        if (hexfile_convertBin(fileBuf, lenFile, addrStart, imageBuf, verbose) != STM8GAL_HEXFILE_NO_ERROR)
          Error("Fatal Error");
      }
      else
        Error("Input file %s has unsupported format (*.s19, *.hex, *.ihx, *.txt, *.bin)", infile);

      // get image size
      if ( hexfile_getImageSize(imageBuf, 0, LENIMAGEBUF, &addrStart, &addrStop, &numData) != STM8GAL_HEXFILE_NO_ERROR )
        Error("Fatal Error");

      // upload memory image to STM8
      if (bsl_memWrite(ptrPort, physInterface, uartMode, imageBuf, addrStart, addrStop, verbose) != STM8GAL_BOOTLOADER_NO_ERROR)
        Error("Fatal Error");

      // optionally verify upload
      if (verifyUpload == 0)        // skip verify
        ;
      else if (verifyUpload == 1)   // compare CRC32 checksums
        verify_crc32(ptrPort, physInterface, uartMode, imageBuf, addrStart, addrStop, verbose);
      else if (verifyUpload == 2)   // read back memory and compare
      {
        if (bsl_memVerify(ptrPort, physInterface, uartMode, imageBuf, addrStart, addrStop, verbose) != STM8GAL_BOOTLOADER_NO_ERROR)
          Error("Fatal Error");
      }
      else
        Error("Unknown memory verify method %d", verifyUpload);

      // clear memory image again
      memset(imageBuf, 0, (LENIMAGEBUF + 1) * sizeof(*imageBuf));

    } // write


    // set value at given address -> perform here
    else if ((!strcmp(argv[i], "-W")) || (!strcmp(argv[i], "-write-byte"))) {

      // intermediate variables
      uint64_t  addr;
      uint8_t   val;

      // clear image buffer
      memset(imageBuf, 0, (LENIMAGEBUF + 1) * sizeof(*imageBuf));

      // get address
      strncpy(tmp, argv[++i], STRLEN-1);
      if (strstr(tmp, "0x") != NULL)
        sscanf(tmp, "%" SCNx64, &addr);   // read as hex
      else
        sscanf(tmp, "%" SCNu64, &addr);   // read as dec
      //printf("\n0x%" PRIx64 "\t%d\n", addr, (int) addr); exit(1);

      // get address and value and store to parameters for bsl_memWrite
      strncpy(tmp, argv[++i], STRLEN-1);
      if (strstr(tmp, "0x") != NULL)
        sscanf(tmp, "%" SCNx8, &val);   // read as hex
      else
        sscanf(tmp, "%" SCNu8, &val);   // read as dec
      //printf("\n0x%" PRIx64 "\t%d\n", addr, (int) addr); exit(1);

      // get address and value and store to parameters for bsl_memWrite
      imageBuf[addr] = (uint16_t) (val | 0xFF00);

      // get image size
      if ( hexfile_getImageSize(imageBuf, 0, LENIMAGEBUF, &addrStart, &addrStop, &numData) != STM8GAL_HEXFILE_NO_ERROR )
        Error("Fatal Error");

      // upload memory image to STM8
      if (bsl_memWrite(ptrPort, physInterface, uartMode, imageBuf, addrStart, addrStop, verbose) != STM8GAL_BOOTLOADER_NO_ERROR)
        Error("Fatal Error");

      // optionally verify upload
      if (verifyUpload == 0)        // skip verify
        ;
      else if (verifyUpload == 1)   // compare CRC32 checksums
        bsl_memVerify(ptrPort, physInterface, uartMode, imageBuf, addrStart, addrStop, verbose);
      else if (verifyUpload == 2)   // read back memory and compare
      {
        if (bsl_memVerify(ptrPort, physInterface, uartMode, imageBuf, addrStart, addrStop, verbose) != STM8GAL_BOOTLOADER_NO_ERROR)
          Error("Fatal Error");
      }
      else
        Error("Unknown memory verify method %d", verifyUpload);

      // clear memory image again
      memset(imageBuf, 0, (LENIMAGEBUF + 1) * sizeof(*imageBuf));

    } // set


    // read address range -> perform here
    else if ((!strcmp(argv[i], "-r")) || (!strcmp(argv[i], "-read"))) {

      // intermediate variables
      char      outfile[STRLEN]="";     // name of export file
      uint64_t  addrTmp;

      // get start address
      strncpy(tmp, argv[++i], STRLEN-1);
      if (strstr(tmp, "0x") != NULL)
      {
        sscanf(tmp, "%" SCNx64, &addrTmp);   // read as hex
        addrStart = addrTmp;
      }
      else
      {
        sscanf(tmp, "%" SCNu64, &addrTmp);   // read as dec
        addrStart = addrTmp;
      }
      //printf("\n0x%" PRIx64 "\t%d\n", addrStart, (int) addrStart); exit(1);

      // get stop address
      strncpy(tmp, argv[++i], STRLEN-1);
      if (strstr(tmp, "0x") != NULL)
      {
        sscanf(tmp, "%" SCNx64, &addrTmp);   // read as hex
        addrStop = addrTmp;
      }
      else
      {
        sscanf(tmp, "%" SCNu64, &addrTmp);   // read as dec
        addrStop = addrTmp;
      }
      //printf("\n0x%" PRIx64 "\t%d\n", addrStop, (int) addrStop); exit(1);

      // get export filename
      strncpy(outfile, argv[++i], STRLEN-1);

      // clear image buffer
      memset(imageBuf, 0, (LENIMAGEBUF + 1) * sizeof(*imageBuf));

      // read memory
      if (bsl_memRead(ptrPort, physInterface, uartMode, addrStart, addrStop, imageBuf, verbose) != STM8GAL_BOOTLOADER_NO_ERROR)
        Error("Fatal Error");

      // export in format depending on file extension
      if (strstr(outfile, ".s19") != NULL) {   // Motorola S-record format
        if (hexfile_exportS19(outfile, imageBuf, verbose) != STM8GAL_HEXFILE_NO_ERROR)
          Error("Fatal Error");
      }
      else if ((strstr(outfile, ".hex") != NULL) || (strstr(outfile, ".ihx") != NULL)) { // Intel HEX-format
        if (hexfile_exportIHex(outfile, imageBuf, verbose) != STM8GAL_HEXFILE_NO_ERROR)
          Error("Fatal Error");
      }
      else if (strstr(outfile, ".txt") != NULL) { // text table (hexAddr / hexData)
        if (hexfile_exportTxt(outfile, imageBuf, verbose) != STM8GAL_HEXFILE_NO_ERROR)
          Error("Fatal Error");
      }
      else if (strstr(outfile, ".bin") != NULL) { // binary format
        if (hexfile_exportBin(outfile, imageBuf, verbose) != STM8GAL_HEXFILE_NO_ERROR)
          Error("Fatal Error");
      }
      else {                                      // print
        if (hexfile_exportTxt("console", imageBuf, verbose) != STM8GAL_HEXFILE_NO_ERROR)
          Error("Fatal Error");
      }

      // clear image buffer
      memset(imageBuf, 0, (LENIMAGEBUF + 1) * sizeof(*imageBuf));

    } // read


    // sector erase flash -> perform here
    else if ((!strcmp(argv[i], "-e")) || (!strcmp(argv[i], "-erase-sector"))) {

      // get address of sector to erase. See respective STM8 datasheet
      uint64_t addr;
      strncpy(tmp, argv[++i], STRLEN-1);
      if (strstr(tmp, "0x") != NULL)
        sscanf(tmp, "%" SCNx64, &addr);   // read as hex
      else
        sscanf(tmp, "%" SCNu64, &addr);   // read as dec
      //printf("\n0x%" PRIx64 "\t%d\n", addr, (int) addr); exit(1);

      // trigger flash mass erase
      if (bsl_flashSectorErase(ptrPort, physInterface, uartMode, addr, verbose) != STM8GAL_BOOTLOADER_NO_ERROR)
        Error("Fatal Error");

    } // sector_erase


    // mass erase flash -> perform here
    else if ((!strcmp(argv[i], "-E")) || (!strcmp(argv[i], "-erase-full"))) {

      // trigger flash mass erase
      if (bsl_flashMassErase(ptrPort, physInterface, uartMode, verbose) != STM8GAL_BOOTLOADER_NO_ERROR)
        Error("Fatal Error");

    } // mass_erase


    // dummy parameter: skip, is treated in 1st pass
    else {
      // dummy
      //printf("\ntest: '%s'\n", argv[i]);
    }

  } // 2nd pass over commandline arguments


  ////////
  // jump to address prior to exit (default: beginning of P-flash=0x8000). Skip if 0xFFFFFFFFFFFFFFFF
  ////////
  if (jumpAddr != 0xFFFFFFFFFFFFFFFF) {

    // don't know why, but seems to be required for SPI
    #if defined(USE_SPIDEV)
      if (physInterface==SPI_SPIDEV)
        SLEEP(500);
    #endif
    #if defined(USE_SPI_ARDUINO)
      if (physInterface==SPI_ARDUINO)
        SLEEP(500);
    #endif

    // jump to application
    if (bsl_jumpTo(ptrPort, physInterface, uartMode, jumpAddr, verbose) != STM8GAL_BOOTLOADER_NO_ERROR)
      Error("Fatal Error");

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
