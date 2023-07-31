/**
   \file main.c

   \author G. Icking-Konert

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
#include "hexfile.h"
#include "misc.h"
#include "serial_comm.h"
#include "spi_spidev_comm.h"
#include "spi_Arduino_comm.h"
#include "bootloader.h"
#include "verify_CRC32.h"
#include "version.h"
#define _MAIN_
  #include "main.h"
#undef _MAIN_


/**
   \fn int main(int argc, char *argv[])

   \param argc      number of commandline arguments + 1
   \param argv      string array containing commandline arguments (argv[0] contains name of executable)

   \return dummy return code (not used)

   Main routine for import, programming, and check routines
*/
int main(int argc, char ** argv) {

  // local variables
  char            appname[STRLEN];      // name of application without path
  char            version[100];         // version as string
  int             verbose;              // verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)
  MemoryImage_s   image;                // memory image buffer as list of (addr, value)
  int             printHelp = -1;       // parameter index to print help for
  char            tmp[STRLEN+106];      // misc string buffer
  int             physInterface;        // bootloader interface: 0=UART (default), 1=SPI_ARDUINO, 2=SPI_SPIDEV
  char            portname[STRLEN]="";  // name of communication port
  HANDLE          ptrPort = 0;          // handle to communication port
  int             baudrate;             // communication baudrate [Baud]
  int             uartMode;             // UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply, other=auto-detect
  int             resetSTM8;            // reset STM8: 0=skip, 1=manual, 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8, 5=Raspi pin 12, 6=RTS line (RS232) (default: manual)
  int             verifyUpload;         // verify method after upload (0=skip, 1=CRC32, 2=read-out)
  uint64_t        jumpAddr;             // address to jump to before exit program
  int             i, j;                 // loop variables

  // STM8 device properties
  int             flashsize;            // size of flash (kB) for w/e routines
  uint8_t         versBSL;              // BSL version for w/e routines
  uint8_t         family;               // device family, currently STM8S and STM8L


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


  // get app name & version, and change console title
  get_app_name(argv[0], VERSION, appname, version);

  // initialize time-keeping (1st call stores launch time)
  micros();

  // reset console color (needs to be called once for Windows)
  setConsoleColor(PRM_COLOR_DEFAULT);

  // initialize memory image
  MemoryImage_init(&image);


  /////////////////
  // 1st pass of commandline arguments: set global parameters, no upload/download/erase yet
  /////////////////

  for (i=1; i<argc; i++) {

    // print help
    if ((!strcmp(argv[i], "-h")) || (!strcmp(argv[i], "-help"))) {

      // set flag for printing help
      printHelp = 0;
      break;

    } // help


    // set verbosity level (0..3)
    else if ((!strcmp(argv[i], "-v")) || (!strcmp(argv[i], "-verbose"))) {

      // get verbosity level
      if (i+1<argc) {
        i++;
        if ((!isDecString(argv[i])) || (sscanf(argv[i],"%d", &verbose) <= 0) || (verbose < 0) || (verbose > 3))
        {
          printf("\ncommand '-v/-verbose' requires a decimal parameter (0..3)\n");
          printHelp = i;
          break;
        }
      }
      else {
        printf("\ncommand '-v/-verbose' requires a decimal parameter (0..3)\n");
        printHelp = i;
        break;
      }
      if (verbose < MUTE)   verbose = MUTE;
      if (verbose > CHATTY) verbose = CHATTY;

    } // verbosity


    // optimize for background operation, e.g. skip prompts and colors
    else if ((!strcmp(argv[i], "-B")) || (!strcmp(argv[i], "-background"))) {
      g_backgroundOperation = true;
    } // background


    // prompt for <return> prior to exit
    else if ((!strcmp(argv[i], "-q")) || (!strcmp(argv[i], "-exit-prompt"))) {
      g_pauseOnExit = true;
    } // exit-prompt


    // reset method: 0=skip, 1=manual; 2=DTR line (RS232), 3=send 'Re5eT!' @ 115.2kBaud, 4=Arduino pin 8, 5=Raspi pin 12, 6=RTS line (RS232)
    else if ((!strcmp(argv[i], "-R")) || (!strcmp(argv[i], "-reset"))) {

      // get STM8 reset method
      if (i+1<argc) {
        i++;
        if ((!isDecString(argv[i])) || (sscanf(argv[i],"%d", &j) <= 0) || (j < 0) || (j > 6))
        {
          printf("\ncommand '-R/-reset' requires a decimal parameter (0..6)\n");
          printHelp = i;
          break;
        }
      }
      else {
        printf("\ncommand '-R/-reset' requires a decimal parameter (0..6)\n");
        printHelp = i;
        break;
      }
      resetSTM8 = j;

    } // reset method


    // get communication interface: 0=UART (default), 1=SPI_ARDUINO, 2=SPI_SPIDEV
    else if ((!strcmp(argv[i], "-i")) || (!strcmp(argv[i], "-interface"))) {

      // get interface
      if (i+1<argc) {
        i++;
        if ((!isDecString(argv[i])) || (sscanf(argv[i],"%d", &j) <= 0) || (j < 0) || (j > 2))
        {
          printf("\ncommand '-i/-interface' requires a decimal parameter (0..2)\n");
          printHelp = i;
          break;
        }
      }
      else {
        printf("\ncommand '-i/-interface' requires a decimal parameter (0..2)\n");
        printHelp = i;
        break;
      }
      physInterface = j;

    } // interface


    // UART mode
    else if ((!strcmp(argv[i], "-u")) || (!strcmp(argv[i], "-uart-mode"))) {

      // get interface
      if (i+1<argc) {
        i++;
        if ((!isDecString(argv[i])) || (sscanf(argv[i],"%d", &j) <= 0))
        {
          printf("\ncommand '-u/-uart-mode' requires a decimal parameter\n");
          printHelp = i;
          break;
        }
      }
      else {
        printf("\ncommand '-u/-uart-mode' requires a decimal parameter\n");
        printHelp = i;
        break;
      }
      uartMode = j;

    } // uart_mode


    // name of communication port
    else if ((!strcmp(argv[i], "-p")) || (!strcmp(argv[i], "-port"))) {

      // get port name
      if (i+1<argc) {
        i+=1;
        strncpy(portname, argv[i], STRLEN-1);
      }
      else {
        printf("\ncommand '-p/-port' requires a name\n");
        printHelp = i;
        break;
      }

    } // port


    // communication baudrate
    else if ((!strcmp(argv[i], "-b")) || (!strcmp(argv[i], "-baudrate"))) {

      // get communication baudrate
      if (i+1<argc) {
        i++;
        if ((!isDecString(argv[i])) || (sscanf(argv[i],"%d", &j) <= 0))
        {
          printf("\ncommand '-b/-baudrate' requires a decimal parameter\n");
          printHelp = i;
          break;
        }
      }
      else {
        printf("\ncommand '-b/-baudrate' requires a decimal parameter\n");
        printHelp = i;
        break;
      }
      baudrate = j;

    } // baudrate


    // set verify method after upload
    else if ((!strcmp(argv[i], "-V")) || (!strcmp(argv[i], "-verify"))) {

      // get memory verify method
      if (i+1<argc) {
        i++;
        if ((!isDecString(argv[i])) || (sscanf(argv[i],"%d", &j) <= 0) || (j < 0) || (j > 2))
        {
          printf("\ncommand '-V/-verify' requires a decimal parameter (0..2)\n");
          printHelp = i;
          break;
        }
      }
      else {
        printf("\ncommand '-V/-verify' requires a decimal parameter (0..2)\n");
        printHelp = i;
        break;
      }
      verifyUpload = j;

    } // verify method


    // jump adress before program termination (-1 or 0xFFFFFFFF == skip jump)
    else if ((!strcmp(argv[i], "-j")) || (!strcmp(argv[i], "-jump-addr"))) {

      // get jump address (0x indicates hex, else decimal)
      if (i+1<argc) {
        i+=1;
        uint64_t addr;
        strncpy(tmp, argv[i], STRLEN-1);
        if (isHexString(tmp))
          sscanf(tmp, "%" SCNx64, &addr);   // read as hex
        else if (isDecString(tmp))
          sscanf(tmp, "%" SCNu64, &addr);   // read as dec
        else {
          printf("\ncommand '-j/-jump' requires a hex or decimal parameter\n");
          printHelp = i;
          break;
        }
        jumpAddr = addr;
      }
      else {
        printf("\ncommand '-j/-jump' requires a hex or decimal parameter\n");
        printHelp = i;
        break;
      }

    } // jump-address


    // skip file upload. Just check parameter number and offset (bin only)
    else if ((!strcmp(argv[i], "-w")) || (!strcmp(argv[i], "-write-file"))) {

      // get file name
      if (i+1<argc) {
        i+=1;
        char *p = strrchr(argv[i], '.');
        if ((p != NULL ) && ((!strcmp(p, ".bin")) || (!strcmp(p, ".BIN")))) {   // for binary file assert additional address
          if (i+1<argc) {
            i+=1;
            if (!isHexString(argv[i])) {
              printf("\ncommand '-w/-write-file' requires a hex offset for binary\n");
              printHelp = i;
              break;
            }
          }
          else {
            printf("\ncommand '-w/-write-file' requires a hex offset for binary\n");
            printHelp = i;
            break;
          }
        }
      }
      else {
        printf("\ncommand '-w/-write-file' requires a filename\n");
        printHelp = i;
        break;
      }

    } // write


    // skip writing single value. Just check parameter number
    else if ((!strcmp(argv[i], "-W")) || (!strcmp(argv[i], "-write-byte"))) {
      if (i+2<argc) {
        i++;
        if (!isHexString(argv[i]) && !isDecString(argv[i])) {
          printf("\ncommand '-W/-write-byte' requires two hex or decimal parameters\n");
          printHelp = i;
          break;
        }
        i++;
        if (!isHexString(argv[i]) && !isDecString(argv[i])) {
          printf("\ncommand '-W/-write-byte' requires two hex or decimal parameters\n");
          printHelp = i;
          break;
        }
      }
      else {
        printf("\ncommand '-W/-write-byte' requires two hex or decimal parameters\n");
        printHelp = i;
        break;
      }

    } // write-byte

  
    // skip reading address range. Just check parameter number
    else if ((!strcmp(argv[i], "-r")) || (!strcmp(argv[i], "-read"))) {
      if (i+3<argc) {
        i++;
        if (!isHexString(argv[i]) && !isDecString(argv[i])) {
          printf("\ncommand '-r/-read' requires two hex or decimal parameters and a filename\n");
          printHelp = i;
          break;
        }
        i++;
        if (!isHexString(argv[i]) && !isDecString(argv[i])) {
          printf("\ncommand '-r/-read' requires two hex or decimal parameters and a filename\n");
          printHelp = i;
          break;
        }
        i++; 
        // no check for filename
      }
      else {
        printf("\ncommand '-r/-read' requires two hex or decimal parameters and a filename\n");
        printHelp = i;
        break;
      }

    } // read


    // skip flash sector erase. Just check parameter number
    else if ((!strcmp(argv[i], "-e")) || (!strcmp(argv[i], "-erase-sector"))) {

      // get sector address (0x indicates hex, else decimal)
      if (i+1<argc) {
        i+=1;
        if (!isHexString(argv[i]) && !isDecString(argv[i])) {
          printf("\ncommand '-e/-erase-sector' requires a hex or decimal parameter\n");
          printHelp = i;
          break;
        }
      }
      else {
        printf("\ncommand '-e/-erase-sector' requires a hex or decimal parameter\n");
        printHelp = i;
        break;
      }

    } // erase-sector


    // skip flash mass erase
    else if ((!strcmp(argv[i], "-E")) || (!strcmp(argv[i], "-erase-full"))) {
      
      // dummy
    
    } // erase-full


    // else print help
    else {
      printf("\nunknown command '%s' \n", argv[i]);
      printHelp = i;
      break;
    }

  } // 1st pass over commandline arguments


  // on request (-h) or in case of parameter error print help page
  if ((printHelp >= 0) || (argc == 1)) {

    snprintf(tmp, sizeof(tmp), "%s (v%s)", appname, version);
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
    #ifdef USE_SPIDEV
      printf("    -i/-interface [line]            communication interface: 0=UART, 1=SPI via Arduino, 2=SPI via spidev (default: UART)\n");
    #else
      printf("    -i/-interface [line]            communication interface: 0=UART, 1=SPI via Arduino (default: UART)\n");
    #endif
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
    if (printHelp > 0)
      printf("\nerror occurred in parameter %d\n", printHelp);

    Exit(0,0);
  }


  ////////
  // perform some misc tasks
  ////////

  // for background operation avoid prompt on exit
  if (g_backgroundOperation)
    g_pauseOnExit = false;

  if (!g_backgroundOperation) {
    snprintf(tmp, sizeof(tmp), "%s (v%s)", appname, version);
    setConsoleTitle(tmp);
  }


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
    if (verbose != MUTE)
      printf("  reset via DTR ... ");
    fflush(stdout);
    ptrPort = init_port(portname, 115200, 100, 8, 0, 1, 0, 0);
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
    ptrPort = init_port(portname, 115200, 100, 8, 0, 1, 0, 0);
    for (i=0; i<6; i++) {
      send_port(ptrPort, 0, 1, buf+i);   // send reset command bytewise to account for possible slow handling on STM8 side
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
        ptrPort = init_port(portname, 115200, 100, 8, 0, 1, 0, 0);
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
    ptrPort = init_port(portname, baudrate, TIMEOUT, 8, 0, 1, 0, 0);   // start without parity, may be changed in bsl_sync()
    if ((verbose == INFORM) || (verbose == CHATTY))
      printf("done\n");
    fflush(stdout);

  } // UART

  // SPI via Arduino
  else if (physInterface == SPI_ARDUINO) {

    // open port
    if (verbose == INFORM)
      printf("  open Arduino port '%s' ... ", portname);
    else if (verbose == CHATTY)
      printf("  open Arduino port '%s' with %gkBaud ... ", portname, (float) ARDUINO_BAUDRATE / 1000.0);
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
  #ifdef DONIX
    char Tx[1000];
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
  bsl_sync(ptrPort, physInterface, verbose);

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
      set_parity(ptrPort, 0);
      send_port(ptrPort, 0, 1, &c);
      if (verbose != MUTE)
        printf("  set UART mode: 2-wire reply\n");
    }
    else
      uartMode = bsl_getUartMode(ptrPort, verbose);
  } // UART interface
  fflush(stdout);

  // get bootloader info for selecting RAM w/e routines for flash
  bsl_getInfo(ptrPort, physInterface, uartMode, &flashsize, &versBSL, &family, verbose);

  // upload RAM routines for flash write & erase
  bsl_uploadWriteErase(ptrPort, physInterface, uartMode, flashsize, versBSL, family, verbose);


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
      uint64_t  addrStart;             // address offset for binary file

      // get file name
      strncpy(infile, argv[++i], STRLEN-1);

      // for binary file also get starting address
      char *p = strrchr(infile, '.');
      if ((p != NULL ) && ((strstr(p, ".bin")) || (strstr(p, ".BIN")))) {
        strncpy(tmp, argv[++i], STRLEN-1);
        sscanf(tmp, "%" SCNx64, &addrStart);
      }

      // import file to memory image, depending on type
      if ((p != NULL ) && ((!strcmp(p, ".s19")) || (!strcmp(p, ".S19")))) {        // Motorola S-record format
        import_file_s19(infile, &image, verbose);
      }
      else if ((p != NULL ) && (!strcmp(p, ".hex") || (!strcmp(p, ".HEX")) || (!strcmp(p, ".ihx")) || (!strcmp(p, ".IHX")))) {  // Intel hex format
        import_file_ihx(infile, &image, verbose);
      }
      else if ((p != NULL ) && ((!strcmp(p, ".txt")) || (!strcmp(p, ".TXT")))) {   // text table (hex addr / data)
        import_file_txt(infile, &image, verbose);
      }
      else if ((p != NULL ) && ((!strcmp(p, ".bin")) || (!strcmp(p, ".BIN")))) {   // binary file
        import_file_bin(infile, addrStart, &image, verbose);
      }
      else {
        MemoryImage_free(&image);
        Error("Input file %s has unsupported format (*.s19, *.hex, *.ihx, *.txt, *.bin)", infile);
      }

      // upload memory image to STM8
      bsl_memWrite(ptrPort, physInterface, uartMode, &image, verbose);

      // optionally verify upload
      if (verifyUpload == 0)        // skip verify
        ;
      else if (verifyUpload == 1)   // compare CRC32 checksums. Requires re-uploading w/e routines, which are cleared by ROM-BL by "GO" command
      {
        verify_crc32(ptrPort, family, flashsize, versBSL, physInterface, uartMode, &image, verbose);
        bsl_uploadWriteErase(ptrPort, physInterface, uartMode, flashsize, versBSL, family, MUTE);
      }
      else if (verifyUpload == 2)   // read back memory and compare
        bsl_memVerifyRead(ptrPort, physInterface, uartMode,  &image, verbose);
      else
        Error("Unknown memory verify method %d", verifyUpload);

      // clear memory image
      MemoryImage_free(&image);

    } // write


    // set value at given address -> perform here
    else if ((!strcmp(argv[i], "-W")) || (!strcmp(argv[i], "-write-byte"))) {

      // intermediate variables
      uint64_t  addr;
      uint8_t   val;

      // clear memory image
      MemoryImage_free(&image);

      // get address
      strncpy(tmp, argv[++i], STRLEN-1);
      if (isHexString(tmp))
        sscanf(tmp, "%" SCNx64, &addr);   // read as hex
      else
        sscanf(tmp, "%" SCNu64, &addr);   // read as dec
      //printf("\n0x%" PRIX64 "\t%d\n", addr, (int) addr); exit(1);

      // get value
      strncpy(tmp, argv[++i], STRLEN-1);
      if (isHexString(tmp))
        //sscanf(tmp, "%" SCNx8, &val);   // read as hex
        sscanf(tmp, "%hhx", &val);   // read as hex
      else
        sscanf(tmp, "%" SCNu8, &val);   // read as dec
      //printf("\n0x%" PRIX8 "\t%d\n", val, (int) val); exit(1);

      // store value in memory image for bsl_memWrite()
      assert(MemoryImage_addData(&image, (MEMIMAGE_ADDR_T) addr, (uint8_t) val));

      // upload memory image to STM8
      bsl_memWrite(ptrPort, physInterface, uartMode, &image, verbose);

      // optionally verify upload
      if (verifyUpload == 0)        // skip verify
        ;
      else if (verifyUpload == 1)   // compare CRC32 checksums
        verify_crc32(ptrPort, family, flashsize, versBSL, physInterface, uartMode, &image, verbose);
      else if (verifyUpload == 2)   // read back memory and compare
        bsl_memVerifyRead(ptrPort, physInterface, uartMode, &image, verbose);
      else
        Error("Unknown memory verify method %d", verifyUpload);

      // clear memory image
      MemoryImage_free(&image);

    } // set


    // read address range -> perform here
    else if ((!strcmp(argv[i], "-r")) || (!strcmp(argv[i], "-read"))) {

      // intermediate variables
      char      outfile[STRLEN]="";     // name of export file
      uint64_t  addrStart, addrStop;    // address range to read

      // clear memory image
      MemoryImage_free(&image);

      // get start address
      strncpy(tmp, argv[++i], STRLEN-1);
      if (isHexString(tmp))
        sscanf(tmp, "%" SCNx64, &addrStart);   // read as hex
      else
        sscanf(tmp, "%" SCNu64, &addrStart);   // read as dec
      //printf("\n0x%" PRIX64 "\t%d\n", addrStart, (int) addrStart); exit(1);

      // get stop address
      strncpy(tmp, argv[++i], STRLEN-1);
      if (isHexString(tmp))
        sscanf(tmp, "%" SCNx64, &addrStop);   // read as hex
      else
        sscanf(tmp, "%" SCNu64, &addrStop);   // read as dec
      //printf("\n0x%" PRIX64 "\t%d\n", addrStop, (int) addrStop); exit(1);

      // get export filename
      strncpy(outfile, argv[++i], STRLEN-1);

      // read memory
      bsl_memRead(ptrPort, physInterface, uartMode, (MEMIMAGE_ADDR_T) addrStart, (MEMIMAGE_ADDR_T) addrStop, &image, verbose);

      // export in format depending on file extension
      char *p = strrchr(outfile, '.');
      if ((p != NULL ) && ((!strcmp(p, ".s19")) || (!strcmp(p, ".S19"))))          // Motorola S-record format
        export_file_s19(outfile, &image, verbose);
      else if ((p != NULL ) && ((!strcmp(p, ".hex")) || (!strcmp(p, ".HEX")) || (!strcmp(p, ".ihx")) || (!strcmp(p, ".IHX"))))  // Intel hex format
        export_file_ihx(outfile, &image, verbose);
      else if ((p != NULL ) && ((!strcmp(p, ".txt")) || (!strcmp(p, ".TXT"))))     // text table (hex addr / data)
        export_file_txt(outfile, &image, verbose);
      else if ((p != NULL ) && ((!strcmp(p, ".bin")) || (!strcmp(p, ".BIN"))))     // binary file
        export_file_bin(outfile, &image, verbose);
      else                                                                         // print
        export_file_txt("console", &image, verbose);

      // clear memory image
      MemoryImage_free(&image);

    } // read


    // sector erase flash -> perform here
    else if ((!strcmp(argv[i], "-e")) || (!strcmp(argv[i], "-erase-sector"))) {

      // get address of sector to erase. See respective STM8 datasheet
      uint64_t addr;
      strncpy(tmp, argv[++i], STRLEN-1);
      if (isHexString(tmp))
        sscanf(tmp, "%" SCNx64, &addr);   // read as hex
      else
        sscanf(tmp, "%" SCNu64, &addr);   // read as dec
      //printf("\n0x%" PRIX64 "\t%d\n", addr, (int) addr); exit(1);

      // trigger flash mass erase
      bsl_flashSectorErase(ptrPort, physInterface, uartMode, addr, verbose);

    } // sector_erase


    // mass erase flash -> perform here
    else if ((!strcmp(argv[i], "-E")) || (!strcmp(argv[i], "-erase-full"))) {

      // trigger flash mass erase
      bsl_flashMassErase(ptrPort, physInterface, uartMode, verbose);

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
      if ((physInterface==SPI_SPIDEV) || (physInterface==SPI_ARDUINO))
        SLEEP(500);
    #else
      if (physInterface==SPI_ARDUINO)
        SLEEP(500);
    #endif

    // jump to application
    bsl_jumpTo(ptrPort, physInterface, uartMode, jumpAddr, verbose);

  } // jump to STM8 address


  // print message
  if (verbose != MUTE)
    printf("done with program\n");

  // clear memory image
  MemoryImage_free(&image);

  // close communication port
  close_port(&ptrPort);

  // terminate program
  Exit(0, g_pauseOnExit);

  // avoid compiler warnings
  return(0);

} // main
