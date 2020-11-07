/**
  \file bootloader.c

  \author G. Icking-Konert
  \date 2014-03-14
  \version 0.1

  \brief implementation of STM bootloader routines

  implementation of of STM bootloader routines
*/


#include "bootloader.h"
#include "console.h"
#include "hexfile.h"
#include "timer.h"
#include "serial_comm.h"
#if defined(USE_SPIDEV)
  #include "spi_spidev_comm.h"
#endif
#if defined (USE_SPI_ARDUINO)
  #include "spi_Arduino_comm.h"
#endif

STM8gal_BootloaderErrors_t g_bootloaderLastError = STM8GAL_BOOTLOADER_NO_ERROR;

char * g_bootloaderErrorStrings[STM8GAL_BOOTLOADER_HEXFILE_ERROR+1] = 
{ 
    "No Error",                             // STM8GAL_BOOTLOADER_NO_ERROR = 0,
    "Port Not Opem",                        // STM8GAL_BOOTLOADER_PORT_NOT_OPEN,
    "Too Many Sync Attempts",               // STM8GAL_BOOTLOADER_TOO_MANY_SYNC_ATTEMPTS,
    "Unknown Interface",                    // STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE,
    "Send Command Failed",                  // STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED,
    "Receive Command Failed",               // STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED,
    "Response Timeout",                     // STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT,
    "Response Unexpected",                  // STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED,
    "Cannot Send to Port",                  // STM8GAL_BOOTLOADER_CANNOT_SEND_TO_PORT,
    "Cannot Determine Uart Mode",           // STM8GAL_BOOTLOADER_CANNOT_DETERMINE_UART_MODE,
    "Cannot Identify Family",               // STM8GAL_BOOTLOADER_CANNOT_IDENTIFY_FAMILY,
    "Cannot Identify Device",               // STM8GAL_BOOTLOADER_CANNOT_IDENTIFY_DEVICE,
    "Cannot Allocate Local Buffer",         // STM8GAL_BOOTLOADER_CANNOT_ALLOCATE_BUFFER,
    "Incorrect GET Code",                   // STM8GAL_BOOTLOADER_INCORRECT_GET_CODE,
    "Incorrect READ Code",                  // STM8GAL_BOOTLOADER_INCORRECT_READ_CODE,
    "Incorrect GO Code",                    // STM8GAL_BOOTLOADER_INCORRECT_GO_CODE,
    "Incorrect WRITE Code",                 // STM8GAL_BOOTLOADER_INCORRECT_WRITE_CODE,
    "Incorrect ERASE Code",                 // STM8GAL_BOOTLOADER_INCORRECT_ERASE_CODE,
    "Start Address Greater than End",       // STM8GAL_BOOTLOADER_ADDRESS_START_GREATER_END,
    "Start Address Greater than Buffer",    // STM8GAL_BOOTLOADER_ADDRESS_START_GREATER_BUFFER,
    "Start Address Greater than Bugger",    // STM8GAL_BOOTLOADER_ADDRESS_END_GREATER_BUFFER,
    "Hexfile Error",                        // STM8GAL_BOOTLOADER_HEXFILE_ERROR,
};

/**
  \fn STM8gal_BootloaderErrors_t bsl_sync(HANDLE ptrPort, uint8_t physInterface, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return synchronization status (0=ok, 1=fail)

  synchronize with microcontroller bootloader. For UART synchronize baudrate.
*/
STM8gal_BootloaderErrors_t bsl_sync(HANDLE ptrPort, uint8_t physInterface, uint8_t verbose) {

  int        i, count;
  uint32_t   lenTx, lenRx, len;
  char       Tx[1000], Rx[1000];

  g_bootloaderLastError = STM8GAL_BOOTLOADER_NO_ERROR;

  // print message
  if (verbose >= SILENT)
    console_print(STDOUT, "  synchronize ... ");

  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort) {
    Error("port not open");
    g_bootloaderLastError = STM8GAL_BOOTLOADER_PORT_NOT_OPEN;
    return(g_bootloaderLastError);
  }

  // purge UART input buffer
  if (physInterface == UART)
    flush_port(ptrPort);


  // construct SYNC command. Note: SYNC has even parity -> works in all UART modes
  lenTx = 1;
  Tx[0] = SYNCH;
  lenRx = 1;

  count = 0;
  do {

    // send command
    if (physInterface == UART) {
      if (send_port(ptrPort, 0, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
        g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
        return(g_bootloaderLastError);
      }
    }
    #if defined(USE_SPI_ARDUINO)
      else if (physInterface == SPI_ARDUINO)
        len = send_spi_Arduino(ptrPort, lenTx, Tx);
    #endif
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    else {
      console_print(STDOUT, "unknown interface %d", (int) physInterface );
      g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
      return(g_bootloaderLastError);
    }
    if (len != lenTx) {
      console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }

    // receive response
    if (physInterface == UART) {
      if (receive_port(ptrPort, 0, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
        g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
        return(g_bootloaderLastError);
      }
      if ((len==1) && (Rx[0]== Tx[0])) {              // check for 1-wire echo
        if (receive_port(ptrPort, 0, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
          g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
          return(g_bootloaderLastError);
        }
      }
    }
    #if defined(USE_SPI_ARDUINO)
	  else if (physInterface == SPI_ARDUINO)
        len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    #endif
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
    #endif
    else {
      console_print(STDOUT, "unknown interface %d", (int) physInterface );
      g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
      return(g_bootloaderLastError);
    }

    // increase retry counter
    count++;

    // avoid flooding the STM8
    SLEEP(10);

  } while ((count<50) && ((len!=lenRx) || ((Rx[0]!=ACK) && (Rx[0]!=NACK))));

  // check if ok
  if ((len==lenRx) && (Rx[0]==ACK)) {
    if (verbose == SILENT)
      console_print(STDOUT, "done\n");
    else if (verbose > SILENT)
      console_print(STDOUT, "done (ACK)\n");
  }
  else if ((len==lenRx) && (Rx[0]==NACK)) {
    if (verbose == SILENT)
      console_print(STDOUT, "done\n");
    else if (verbose > SILENT)
      console_print(STDOUT, "done (NACK)\n");
  }
  else if (count >= 50) {
    console_print(STDOUT, "too many sync retires");
    g_bootloaderLastError = STM8GAL_BOOTLOADER_TOO_MANY_SYNC_ATTEMPTS;
    return(g_bootloaderLastError);
  }
  else if (len==lenRx) {
    console_print(STDOUT, "wrong response 0x%02x from BSL", (uint8_t) (Rx[0]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
    return(g_bootloaderLastError);
  }
  else {
    console_print(STDOUT, "no response from BSL");
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
    return(g_bootloaderLastError);
  }

  // purge PC input buffer
  flush_port(ptrPort);
  SLEEP(50);              // seems to be required for some reason

  // return success
  return(g_bootloaderLastError);

} // bsl_sync



/**
  \fn STM8gal_BootloaderErrors_t bsl_getUartMode(HANDLE ptrPort, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[out] mode           UART mode (0=duplex, 1=1-wire, 2=2-wire reply)
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return Error/Success

  auto-detect UART bootloader mode (see AppNote UM0560). This information is required
  to set the correct data parity and determine if local echo is required.
*/
STM8gal_BootloaderErrors_t bsl_getUartMode(HANDLE ptrPort, uint8_t *mode, uint8_t verbose) {

  uint32_t   len, lenTx, lenRx;
  char       Tx[1000], Rx[1000];

  g_bootloaderLastError = STM8GAL_BOOTLOADER_NO_ERROR;

  *mode = 255;

  // print message
  if (verbose == CHATTY)
    console_print(STDOUT, "  check UART mode ... ");

  // reduce timeout for faster check
  set_timeout(ptrPort, 100);

  // detect UART mode
  set_parity(ptrPort, 2);
  lenTx = 2; lenRx = 1;
  Tx[0] = 0x00; Tx[1] = (Tx[0] ^ 0xFF); Rx[0] = 0x00;
  do {
    if (send_port(ptrPort, 0, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      set_timeout(ptrPort, TIMEOUT);
      return(g_bootloaderLastError);
    }
    if (receive_port(ptrPort, 0, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
      set_timeout(ptrPort, TIMEOUT);
      return(g_bootloaderLastError);
    }
    //console_print(STDOUT, "\nmode 1: %d  0x%02x\n", len, (uint8_t) (Rx[0]));
    SLEEP(10);
  } while (len==0);

  // tested empirically...
  if (Rx[0] == ACK) {              // UART mode 0: 2-wire duplex, no SW reply, even parity
    *mode = 0;
    set_parity(ptrPort, 2);
  }
  else if (Rx[0] == Tx[0]) {       // UART mode 1: 1-wire reply, no SW reply, no parity
    *mode = 1;
    set_parity(ptrPort, 0);
  }
  else if (Rx[0] == NACK) {        // UART mode 2: 2-wire reply, SW reply, no parity
    *mode = 2;
    set_parity(ptrPort, 0);
  }
  else {
    Error("cannot determine UART mode");
    g_bootloaderLastError = STM8GAL_BOOTLOADER_CANNOT_DETERMINE_UART_MODE;
    set_timeout(ptrPort, TIMEOUT);
    return(g_bootloaderLastError);
  }

  // revert timeout
  set_timeout(ptrPort, TIMEOUT);

  // purge PC input buffer
  flush_port(ptrPort);
  SLEEP(50);              // seems to be required for some reason

  // print message
  if (verbose == CHATTY) {
    if (*mode == 0)
      console_print(STDOUT, "done (duplex)\n");
    else if (*mode == 1)
      console_print(STDOUT, "done (1-wire)\n");
    else
      console_print(STDOUT, "done (2-wire reply)\n");
  }

  // return found mode
  return(g_bootloaderLastError);

} // bsl_getUartMode



/**
  \fn STM8gal_BootloaderErrors_t bsl_getInfo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, int *flashsize, uint8_t *vers, uint8_t *family, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[out] flashsize      size of flashsize in kB (required for correct W/E routines)
  \param[out] vers           BSL version number (required for correct W/E routines)
  \param[out] family         STM8 family (STM8S=1, STM8L=2)
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (STM8gal_BootloaderErrors_t)

  query microcontroller type and BSL version info. This information is required
  to select correct version of flash write/erase routines
*/
STM8gal_BootloaderErrors_t bsl_getInfo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, int *flashsize, uint8_t *vers, uint8_t *family, uint8_t verbose) {

  int        i;
  uint32_t   lenTx, lenRx, len;
  char       Tx[1000], Rx[1000];

  g_bootloaderLastError = STM8GAL_BOOTLOADER_NO_ERROR;

  // print message
  if (verbose >= SILENT)
    console_print(STDOUT, "  get device info ... ");

  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort) {
    Error("port not open");
    g_bootloaderLastError = STM8GAL_BOOTLOADER_PORT_NOT_OPEN;
    return(g_bootloaderLastError);
  }


  // purge input buffer
  flush_port(ptrPort);
  SLEEP(50);              // seems to be required for some reason


  /////////
  // determine device flash size for selecting w/e routines (flash starts at PFLASH_START)
  /////////

  // reduce timeout for faster check
  if (physInterface == UART) {
    set_timeout(ptrPort, 200);
  }

  // check address of EEPROM. STM8L starts at 0x1000, STM8S starts at 0x4000
  if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x004000, SILENT) == true) {     // STM8S
    *family = STM8S;
    #ifdef DEBUG
      console_print(STDOUT, "family STM8S\n");
    #endif
  }
  else if (g_bootloaderLastError != STM8GAL_BOOTLOADER_NO_ERROR)
    return(g_bootloaderLastError);
  else if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x00100, SILENT) == true) { // STM8L
    if (g_bootloaderLastError != STM8GAL_BOOTLOADER_NO_ERROR) {
      if (physInterface == UART) {
        set_timeout(ptrPort, TIMEOUT);
      }
      return(g_bootloaderLastError);
    }
    *family = STM8L;
    #ifdef DEBUG
      console_print(STDOUT, "family STM8L\n");
    #endif
  }
  else {
    console_print(STDERR, "cannot identify family");
    g_bootloaderLastError = STM8GAL_BOOTLOADER_CANNOT_IDENTIFY_FAMILY;
    if (physInterface == UART) {
    set_timeout(ptrPort, TIMEOUT);
    }
    return(g_bootloaderLastError);
  }


  // check if adress in flash exists. Check highest flash address to determine size
  if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x047FFF, SILENT))       // extreme density (256kB)
    *flashsize = 256;
  else if (g_bootloaderLastError != STM8GAL_BOOTLOADER_NO_ERROR)    // Check for error
    return(g_bootloaderLastError);
  else if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x027FFF, SILENT))  // high density (128kB)
    *flashsize = 128;
  else if (g_bootloaderLastError != STM8GAL_BOOTLOADER_NO_ERROR)    // Check for error
    return(g_bootloaderLastError);
  else if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x00FFFF, SILENT))  // medium density (32kB)
    *flashsize = 32;
  else if (g_bootloaderLastError != STM8GAL_BOOTLOADER_NO_ERROR)    // Check for error
    return(g_bootloaderLastError);
  else if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x009FFF, SILENT))  // low density (8kB)
    *flashsize = 8;
  else if (g_bootloaderLastError != STM8GAL_BOOTLOADER_NO_ERROR)    // Check for error
    return(g_bootloaderLastError);
  else {
    console_print(STDERR, "cannot identify device");
    g_bootloaderLastError = STM8GAL_BOOTLOADER_CANNOT_IDENTIFY_DEVICE;
    return(g_bootloaderLastError);
  }
  #ifdef DEBUG
    console_print(STDOUT, "flash size: %d\n", (int) (*flashsize));
  #endif

  // restore timeout to avoid timeouts during flash operation
  if (physInterface == UART) {
    set_timeout(ptrPort, TIMEOUT);
  }


  /////////
  // get BSL version
  /////////

  // construct command
  lenTx = 2;
  Tx[0] = GET;
  Tx[1] = (Tx[0] ^ 0xFF);
  lenRx = 9;

  // send command
  if (physInterface == UART) {
    if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface );
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(g_bootloaderLastError);
  }
  if (len != lenTx) {
    console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
    return(g_bootloaderLastError);
  }

  // receive response
  if (physInterface == UART) {
    if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface );
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(g_bootloaderLastError);
  }
  if (len != lenRx) {
    console_print(STDOUT, "ACK timeout (expect %d, received %d)", lenRx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
    return(g_bootloaderLastError);
  }

  // check 2x ACKs
  if (Rx[0]!=ACK) {
    console_print(STDOUT, "start ACK failure (expect 0x%02x, read 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[0]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
    return(g_bootloaderLastError);
  }
  if (Rx[8]!=ACK) {
    console_print(STDOUT, "end ACK failure (expect 0x%02x, read 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[8]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
    return(g_bootloaderLastError);
  }


  // check if command codes are correct (just to be sure)
  if (Rx[3] != GET) {
    console_print(STDOUT, "wrong GET code (expect 0x%02x, received 0x%02x)", (uint8_t) GET, (uint8_t) (Rx[3]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_INCORRECT_GET_CODE;
    return(g_bootloaderLastError);
  }
  if (Rx[4] != READ) {
    console_print(STDOUT, "wrong READ code (expect 0x%02x, received 0x%02x)", (uint8_t) READ, (uint8_t) (Rx[4]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_INCORRECT_READ_CODE;
    return(g_bootloaderLastError);
  }
  if (Rx[5] != GO) {
    console_print(STDOUT, "wrong GO code (expect 0x%02x, received 0x%02x)", (uint8_t) GO, (uint8_t) (Rx[5]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_INCORRECT_GO_CODE;
    return(g_bootloaderLastError);
  }
  if (Rx[6] != WRITE) {
    console_print(STDOUT, "wrong WRITE code (expect 0x%02x, received 0x%02x)", (uint8_t) WRITE, (uint8_t) (Rx[6]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_INCORRECT_WRITE_CODE;
    return(g_bootloaderLastError);
  }
  if (Rx[7] != ERASE) {
    console_print(STDOUT, "wrong ERASE code (expect 0x%02x, received 0x%02x)", (uint8_t) ERASE, (uint8_t) (Rx[7]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_INCORRECT_ERASE_CODE;
    return(g_bootloaderLastError);
  }

// print BSL data
#ifdef DEBUG
  console_print(STDOUT, "    version 0x%02x\n", (uint8_t) (Rx[2]));
  console_print(STDOUT, "    command codes:\n");
  console_print(STDOUT, "      GET   0x%02x\n", (uint8_t) (Rx[3]));
  console_print(STDOUT, "      READ  0x%02x\n", (uint8_t) (Rx[4]));
  console_print(STDOUT, "      GO    0x%02x\n", (uint8_t) (Rx[5]));
  console_print(STDOUT, "      WRITE 0x%02x\n", (uint8_t) (Rx[6]));
  console_print(STDOUT, "      ERASE 0x%02x\n", (uint8_t) (Rx[7]));
#endif

  // copy version number
  *vers = Rx[2];

  // print message
  if (*family == STM8S) {
    if (verbose == SILENT)
      console_print(STDOUT, "done (STM8S; %dkB)\n", *flashsize);
    else if (verbose == INFORM)
      console_print(STDOUT, "done (STM8S; %dkB flash)\n", *flashsize);
    else if (verbose == CHATTY)
      console_print(STDOUT, "done (STM8S; %dkB flash; BSL v%x.%x)\n", *flashsize, (((*vers)&0xF0)>>4), ((*vers) & 0x0F));
  }
  else {
    if (verbose == SILENT)
      console_print(STDOUT, "done (STM8L; %dkB)\n", *flashsize);
    else if (verbose == INFORM)
      console_print(STDOUT, "done (STM8L; %dkB flash)\n", *flashsize);
    else if (verbose == CHATTY)
      console_print(STDOUT, "done (STM8L; %dkB flash; BSL v%x.%x)\n", *flashsize, (((*vers)&0xF0)>>4), ((*vers) & 0x0F));
  }

  // avoid compiler warnings
  return(g_bootloaderLastError);

} // bsl_getInfo



/**
  \fn STM8gal_BootloaderErrors_t bsl_memRead(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addrStart, uint64_t addrStop, uint16_t *imageBuf, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  addrStart      first address to read
  \param[in]  addrStop       last address to read
  \param[out] imageBuf       memory buffer containing read data (16-bit array. HB!=0 indicates content)
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (STM8gal_BootloaderErrors_t)

  read from microcontroller memory via READ command.
*/
STM8gal_BootloaderErrors_t bsl_memRead(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addrStart, uint64_t addrStop, uint16_t *imageBuf, uint8_t verbose) {

  int       i;
  uint32_t  lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];
  uint64_t  addr, addrStep, numBytes, countBytes;

  g_bootloaderLastError = STM8GAL_BOOTLOADER_NO_ERROR;

  // get number of bytes to read
  numBytes = addrStop - addrStart + 1;

  // print message
  if (verbose == SILENT) {
    if (numBytes > 1024)
      console_print(STDOUT, "  read %1.1fkB ", (float) numBytes/1024.0);
    else
      console_print(STDOUT, "  read %dB ", (int) numBytes);
  }
  else if (verbose == INFORM) {
    if (numBytes > 1024)
      console_print(STDOUT, "  read %1.1fkB ", (float) numBytes/1024.0);
    else
      console_print(STDOUT, "  read %dB ", (int) numBytes);
  }
  else if (verbose == CHATTY) {
    if (numBytes > 1024)
      console_print(STDOUT, "  read %1.1fkB in 0x%" PRIx64 " to 0x%" PRIx64 " ", (float) numBytes/1024.0, addrStart, addrStop);
    else
      console_print(STDOUT, "  read %dB in 0x%" PRIx64 " to 0x%" PRIx64 " ", (int) numBytes, addrStart, addrStop);
  }

  // simple checks of scan window
  if (addrStart > addrStop) {
    console_print(STDOUT, "start address 0x%" PRIx64 " higher than end address 0x%" PRIx64, addrStart, addrStop);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_ADDRESS_START_GREATER_END;
    return(g_bootloaderLastError);
  }
  if (addrStart > LENIMAGEBUF) {
    console_print(STDOUT, "start address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, addrStart, LENIMAGEBUF);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_ADDRESS_START_GREATER_BUFFER;
    return(g_bootloaderLastError);
  }
  if (addrStop > LENIMAGEBUF) {
    console_print(STDOUT, "end address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, addrStop, LENIMAGEBUF);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_ADDRESS_END_GREATER_BUFFER;
    return(g_bootloaderLastError);
  }


  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort) {
    Error("port not open");
    g_bootloaderLastError = STM8GAL_BOOTLOADER_PORT_NOT_OPEN;
    return(g_bootloaderLastError);
  }

  // init data buffer
  for (i=addrStart; i<=addrStop; i++)
    imageBuf[i] = 0;


  // loop over addresses in <=256B steps
  countBytes = 0;
  addrStep = 256;
  for (addr=addrStart; addr<=addrStop; addr+=addrStep) {

    // if addr too close to end of range reduce stepsize
    if (addr+256 > addrStop)
      addrStep = addrStop - addr + 1;


    /////
    // send read command
    /////

    // construct command
    lenTx = 2;
    Tx[0] = READ;
    Tx[1] = (Tx[0] ^ 0xFF);
    lenRx = 1;

    // send command
    if (physInterface == UART) {
      if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
        g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
        return(g_bootloaderLastError);
      }
    }
    #if defined(USE_SPI_ARDUINO)
	  else if (physInterface == SPI_ARDUINO)
        len = send_spi_Arduino(ptrPort, lenTx, Tx);
    #endif
	#if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    else {
      console_print(STDOUT, "unknown interface %d", (int) physInterface);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
      return(g_bootloaderLastError);
    }
    if (len != lenTx) {
      console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }

    // receive response
    if (physInterface == UART) {
      if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
        g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
        return(g_bootloaderLastError);
      }
    }
    #if defined(USE_SPI_ARDUINO)
	  else if (physInterface == SPI_ARDUINO)
        len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    #endif
	#if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
    #endif
    else {
      console_print(STDOUT, "unknown interface %d", (int) physInterface);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
      return(g_bootloaderLastError);
    }

    if (len != lenRx) {
      console_print(STDOUT, "ACK1 timeout");
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
      return(g_bootloaderLastError);
    }

    // check acknowledge
    if (Rx[0]!=ACK) {
      console_print(STDOUT, "ACK1 failure (expect 0x%02x, received 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[0]));
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
      return(g_bootloaderLastError);
    }


    /////
    // send address
    /////

    // construct address + checksum (XOR over address)
    lenTx = 5;
    Tx[0] = (char) (addr >> 24);
    Tx[1] = (char) (addr >> 16);
    Tx[2] = (char) (addr >> 8);
    Tx[3] = (char) (addr);
    Tx[4] = (Tx[0] ^ Tx[1] ^ Tx[2] ^ Tx[3]);
    lenRx = 1;

    // send command
    if (physInterface == UART) {
      if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
        g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
        return(g_bootloaderLastError);
      }
    }
    #if defined(USE_SPI_ARDUINO)
      else if (physInterface == SPI_ARDUINO)
        len = send_spi_Arduino(ptrPort, lenTx, Tx);
    #endif
	#if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    else {
      console_print(STDOUT, "unknown interface %d", (int) physInterface);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
      return(g_bootloaderLastError);
    }
    if (len != lenTx) {
      console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }

    // receive response
    if (physInterface == UART) {
      if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
        g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
        return(g_bootloaderLastError);
      }
    }
    #if defined(USE_SPI_ARDUINO)
      else if (physInterface == SPI_ARDUINO)
        len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    #endif
	#if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
    #endif
    else {
      console_print(STDOUT, "unknown interface %d", (int) physInterface);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
      return(g_bootloaderLastError);
    }
    if (len != lenRx) {
      console_print(STDOUT, "ACK2 timeout (expect %d, received %d)", lenRx, len);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
      return(g_bootloaderLastError);
    }

    // check acknowledge
    if (Rx[0]!=ACK) {
      console_print(STDOUT, "ACK2 failure (expect 0x%02x, received 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[0]));
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
      return(g_bootloaderLastError);
    }

    /////
    // send number of bytes
    /////

    // construct number of bytes + checksum
    lenTx = 2;
    Tx[0] = addrStep-1;     // -1 from BSL
    Tx[1] = (Tx[0] ^ 0xFF);
    lenRx = addrStep + 1;

    // send command
    if (physInterface == UART) {
      if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
        g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
        return(g_bootloaderLastError);
      }
    }
    #if defined(USE_SPI_ARDUINO)
      else if (physInterface == SPI_ARDUINO)
        len = send_spi_Arduino(ptrPort, lenTx, Tx);
    #endif
	#if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    else {
      console_print(STDOUT, "unknown interface %d", (int) physInterface);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
      return(g_bootloaderLastError);
    }
    if (len != lenTx) {
      console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }

    // receive response
    if (physInterface == UART) {
      if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
        g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
        return(g_bootloaderLastError);
      }
    }
    #if defined(USE_SPI_ARDUINO)
      else if (physInterface == SPI_ARDUINO)
        len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    #endif
	#if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV) {
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
        //console_print(STDOUT, "0x%02x  0x%02x  0x%02x\n", (uint8_t) (Rx[0]), (uint8_t) (Rx[1]), (uint8_t) (Rx[2])); getchar();
      }
    #endif
    else {
      console_print(STDOUT, "unknown interface %d", (int) physInterface);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
      return(g_bootloaderLastError);
    }
    if (len != lenRx) {
      console_print(STDOUT, "data timeout (expect %d, received %d)", lenRx, len);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
      return(g_bootloaderLastError);
    }

    // check acknowledge
    if (Rx[0]!=ACK) {
      console_print(STDOUT, "ACK3 failure (expect 0x%02x, received 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[0]));
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
      return(g_bootloaderLastError);
    }

    // copy data to buffer. Set HB to indicate data read
    for (i=1; i<lenRx; i++) {
      imageBuf[addr+i-1] = ((uint16_t) (Rx[i]) | 0xFF00);
      //console_print(STDOUT, "%d 0x%02x\n", i, (uint8_t) (Rx[i])); getchar();
      countBytes++;
    }

    // print progress
    if ((countBytes % 1024) == 0) {
      if (verbose == SILENT) {
        console_print(STDOUT, ".");
        if ((countBytes % (10*1024)) == 0)
          console_print(STDOUT, " ");
      }
      else if (verbose == INFORM) {
        if (numBytes > 1024)
          console_print(STDOUT, "%c  read %1.1fkB / %1.1fkB ", '\r', (float) countBytes/1024.0, (float) numBytes/1024.0);
        else
          console_print(STDOUT, "%c  read %dB / %dB ", '\r', (int) countBytes, (int) numBytes);
      }
      else if (verbose == CHATTY) {
        if (numBytes > 1024)
          console_print(STDOUT, "%c  read %1.1fkB / %1.1fkB from 0x%" PRIx64 " to 0x%" PRIx64 " ", '\r', (float) countBytes/1024.0, (float) numBytes/1024.0, addrStart, addrStop);
        else
          console_print(STDOUT, "%c  read %dB / %dB from 0x%" PRIx64 " to 0x%" PRIx64 " ", '\r', (int) countBytes, (int) numBytes, addrStart, addrStop);
      }
    }

  } // loop over address range


  // print message
  if (verbose == SILENT)
    console_print(STDOUT, " done\n");
  else if (verbose == INFORM) {
    if (numBytes > 1024)
      console_print(STDOUT, "%c  read %1.1fkB / %1.1fkB ... done   \n", '\r', (float) countBytes/1024.0, (float) numBytes/1024.0);
    else
      console_print(STDOUT, "%c  read %dB / %dB ... done   \n", '\r', (int) countBytes, (int) numBytes);
  }
  else if (verbose == CHATTY) {
    if (numBytes > 1024)
      console_print(STDOUT, "%c  read %1.1fkB / %1.1fkB from 0x%" PRIx64 " to 0x%" PRIx64 " ... done   \n", '\r', (float) countBytes/1024.0, (float) numBytes/1024.0, addrStart, addrStop);
    else
      console_print(STDOUT, "%c  read %dB / %dB from 0x%" PRIx64 " to 0x%" PRIx64 " ... done   \n", '\r', (int) countBytes, (int) numBytes, addrStart, addrStop);
  }

  // avoid compiler warnings
  return(g_bootloaderLastError);

} // bsl_memRead



/**
  \fn STM8gal_BootloaderErrors_t bsl_memCheck(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addr, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  addr           address to check
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return true if the memory exists

  check if microcontrolles address exists. Specifically read 1B from microcontroller
  memory via READ command. If it fails, memory doesn't exist. Used to get STM8 type
*/
bool bsl_memCheck(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addr, uint8_t verbose) {

  int       i;
  uint32_t  lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];


  g_bootloaderLastError = STM8GAL_BOOTLOADER_NO_ERROR;

  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort) {
    Error("port not open");
    g_bootloaderLastError = STM8GAL_BOOTLOADER_PORT_NOT_OPEN;
    return(false);
  }


  /////
  // send read command
  /////

  // construct command
  lenTx = 2;
  Tx[0] = READ;
  Tx[1] = (Tx[0] ^ 0xFF);
  lenRx = 1;

  // send command
  if (physInterface == UART) {
    if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      return(false);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(false);
  }
  if (len != lenTx) {
    console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
    return(false);
  }

  // receive response
  if (physInterface == UART) {
    if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
      return(false);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(false);
  }
  if (len != lenRx) {
    console_print(STDOUT, "ACK1 timeout (expect %d, received %d)", lenRx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
    return(false);
  }

  // check acknowledge
  if (Rx[0]!=ACK) {
    console_print(STDOUT, "ACK1 failure (expect 0x%02x, received 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[0]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
    return(false);
  }

  /////
  // send address
  /////

  // construct address + checksum (XOR over address)
  lenTx = 5;
  Tx[0] = (char) (addr >> 24);
  Tx[1] = (char) (addr >> 16);
  Tx[2] = (char) (addr >> 8);
  Tx[3] = (char) (addr);
  Tx[4] = (Tx[0] ^ Tx[1] ^ Tx[2] ^ Tx[3]);
  lenRx = 1;

  // send command
  if (physInterface == UART) {
    if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      return(false);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(false);
  }
  if (len != lenTx) {
    console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
    return(false);
  }

  // receive response
  if (physInterface == UART) {
    if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
      return(false);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(false);
  }
  if (len != lenRx) {
    console_print(STDOUT, "ACK2 timeout (expect %d, received %d)", lenRx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
    return(false);
  }

  // check acknowledge -> on NACK memory cannot be read -> return 0
  if (Rx[0]!=ACK) {
    return(false);
  }

  /////
  // send number of bytes to read
  /////

  // construct number of bytes + checksum
  lenTx = 2;
  Tx[0] = 1-1;            // -1 from BSL
  Tx[1] = (Tx[0] ^ 0xFF);
  lenRx = 2;

  // send command
  if (physInterface == UART) {
    if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      return(false);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(false);
  }
  if (len != lenTx) {
    console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
    return(false);
  }

  // receive response
  if (physInterface == UART) {
    if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
      return(false);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(false);
  }
  if (len != lenRx) {
    console_print(STDOUT, "data timeout (expect %d, received %d)", lenRx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
    return(false);
  }

  // check acknowledge
  if (Rx[0]!=ACK) {
    console_print(STDOUT, "ACK3 failure (expect 0x%02x, received 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[0]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
    return(false);
  }

  // memory read succeeded -> memory exists
  return(true);

} // bsl_memCheck



/**
  \fn STM8gal_BootloaderErrors_t bsl_flashSectorErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addr, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  addr           adress within 1kB sector to erase
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (STM8gal_BootloaderErrors_t)

  sector erase for microcontroller flash. Use with care!
*/
STM8gal_BootloaderErrors_t bsl_flashSectorErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addr, uint8_t verbose) {

  int       i;
  uint32_t  lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];
  uint8_t   sector;
  uint64_t  tStart, tStop;        // measure time [ms] for erase (for COMM timeout)

  g_bootloaderLastError = STM8GAL_BOOTLOADER_NO_ERROR;

  // calculate sector code
  sector = (addr - PFLASH_START)/PFLASH_BLOCKSIZE;

  // print message
  if (verbose == SILENT) {
    console_print(STDOUT, "  erase sector ... ");
  }
  else if (verbose == INFORM) {
    console_print(STDOUT, "  erase flash sector %d ... ", (int) sector);
  }
  else if (verbose == CHATTY) {
    if (addr>0xFFFFFF)
      console_print(STDOUT, "  erase flash sector %d @ 0x%" PRIx64 " ... ", (int) sector, addr);
    else if (addr>0xFFFF)
      console_print(STDOUT, "  erase flash sector %d @ 0x%" PRIx64 " ... ", (int) sector, addr);
    else
      console_print(STDOUT, "  erase flash sector %d @ 0x%" PRIx64 " ... ", (int) sector, addr);
  }


  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort) {
    Error("port not open");
    g_bootloaderLastError = STM8GAL_BOOTLOADER_PORT_NOT_OPEN;
    return(g_bootloaderLastError);
  }


  /////
  // send erase command
  /////

  // construct command
  lenTx = 2;
  Tx[0] = ERASE;
  Tx[1] = (Tx[0] ^ 0xFF);
  lenRx = 1;

  // send command
  if (physInterface == UART) {
    if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(g_bootloaderLastError);
  }
  if (len != lenTx) {
    console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
    return(g_bootloaderLastError);
  }

  // receive response
  if (physInterface == UART) {
    if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(g_bootloaderLastError);
  }
  if (len != lenRx) {
    console_print(STDOUT, "ACK1 timeout (expect %d, received %d)", lenRx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
    return(g_bootloaderLastError);
  }

  // check acknowledge
  if (Rx[0]!=ACK) {
    console_print(STDOUT, "ACK1 failure (expect 0x%02x, received 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[0]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
    return(g_bootloaderLastError);
  }

  /////
  // send code of sector to erase
  /////

  // increase timeout for long erase
  if (physInterface == UART) {
    set_timeout(ptrPort, 1200);
  }

  // construct pattern
  lenTx = 3;
  Tx[0] = 0x00;      // number of sectors to erase -1 (here only 1 sector)
  Tx[1] = sector;
  Tx[2] = (Tx[0] ^ Tx[1]);
  lenRx = 1;

  // measure time for sector erase
  tStart = millis();

  // send command
  if (physInterface == UART) {
    if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      set_timeout(ptrPort, TIMEOUT);
      return(g_bootloaderLastError);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(g_bootloaderLastError);
  }
  if (len != lenTx) {
    console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
    if (physInterface == UART) {
      set_timeout(ptrPort, TIMEOUT);
    }
    return(g_bootloaderLastError);
  }

  // receive response
  if (physInterface == UART) {
    if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
      set_timeout(ptrPort, TIMEOUT);
      return(g_bootloaderLastError);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO) {
      SLEEP(40);                              // wait >30ms*(N=0+1) for sector erase before requesting response (see UM0560, SPI timing)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    }
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV) {
      SLEEP(40);                              // wait >30ms*(N=0+1) for sector erase before requesting response (see UM0560, SPI timing)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
    }
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(g_bootloaderLastError);
  }
  if (len != lenRx) {
    console_print(STDOUT, "ACK2 timeout (expect %d, received %d)", lenTx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
    if (physInterface == UART) {
      set_timeout(ptrPort, TIMEOUT);
    }
    return(g_bootloaderLastError);
  }

  // check acknowledge
  if (Rx[0]!=ACK) {
    console_print(STDOUT, "ACK2 failure (expect 0x%02x, received 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[0]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
    if (physInterface == UART) {
      set_timeout(ptrPort, TIMEOUT);
    }
    return(g_bootloaderLastError);
  }

  // measure time for sector erase
  tStop = millis();

  // restore timeout
  set_timeout(ptrPort, TIMEOUT);


  // print message
  if (verbose == SILENT) {
    console_print(STDOUT, "done\n");
  }
  else if ((verbose == INFORM) || (verbose == CHATTY)) {
    console_print(STDOUT, "done, time %dms\n", (int)(tStop-tStart));
  }

  // avoid compiler warnings
  return(g_bootloaderLastError);

} // bsl_flashSectorErase



/**
  \fn STM8gal_BootloaderErrors_t bsl_flashMassErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (STM8gal_BootloaderErrors_t)

  mass erase microcontroller P-flash and D-flash/EEPROM. Use with care!
*/
STM8gal_BootloaderErrors_t bsl_flashMassErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint8_t verbose) {

  int       i;
  uint32_t  lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];
  uint64_t  tStart, tStop;        // measure time [ms] for erase (for COMM timeout)

  g_bootloaderLastError = STM8GAL_BOOTLOADER_NO_ERROR;

  // print message
  if (verbose == SILENT) {
    console_print(STDOUT, "  mass erase ... ");
  }
  else if ((verbose == INFORM) || (verbose == CHATTY)) {
    console_print(STDOUT, "  flash mass erase ... ");
  }


  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort) {
    Error("port not open");
    g_bootloaderLastError = STM8GAL_BOOTLOADER_PORT_NOT_OPEN;
    return(g_bootloaderLastError);
  }


  /////
  // send erase command
  /////

  // construct command
  lenTx = 2;
  Tx[0] = ERASE;
  Tx[1] = (Tx[0] ^ 0xFF);
  lenRx = 1;

  // send command
  if (physInterface == UART) {
    if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(g_bootloaderLastError);
  }
  if (len != lenTx) {
    console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
    return(g_bootloaderLastError);
  }

  // receive response
  if (physInterface == UART) {
    if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(g_bootloaderLastError);
  }
  if (len != lenRx) {
    console_print(STDOUT, "in 'bsl_flashMassErase()': ACK1 timeout (expect %d, received %d)", lenRx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
    return(g_bootloaderLastError);
  }

  // check acknowledge
  if (Rx[0]!=ACK) {
    console_print(STDOUT, "in 'bsl_flashMassErase()': ACK1 failure (expect 0x%02x, received 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[0]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
    return(g_bootloaderLastError);
  }

  /////
  // send 0xFF+0x00 to trigger mass erase
  /////

  // increase timeout for long erase. Measured 3.3s for 128kB STM8 -> set to 4s
  if (physInterface == UART) {
    set_timeout(ptrPort, 4000);
  }

  // construct pattern
  lenTx = 2;
  Tx[0] = 0xFF;
  Tx[1] = 0x00;
  lenRx = 1;

  // measure time for mass erase
  tStart = millis();

  // send command
  if (physInterface == UART) {
    if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      set_timeout(ptrPort, TIMEOUT);
      return(g_bootloaderLastError);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(g_bootloaderLastError);
  }
  if (len != lenTx) {
    console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
    if (physInterface == UART) {
      set_timeout(ptrPort, TIMEOUT);
    }
    return(g_bootloaderLastError);
  }

  // receive response
  if (physInterface == UART) {
    if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
      set_timeout(ptrPort, TIMEOUT);
      return(g_bootloaderLastError);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO) {
      SLEEP(1100);                              // wait >30ms*(N=32+1) for sector erase before requesting response (see UM0560, SPI timing)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    }
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV) {
      SLEEP(1100);                              // wait >30ms*(N=32+1) for sector erase before requesting response (see UM0560, SPI timing)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
    }
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(g_bootloaderLastError);
  }
  if (len != lenRx) {
    console_print(STDOUT, "in 'bsl_flashMassErase()': ACK2 timeout (expect %d, received %d)", lenRx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
    if (physInterface == UART) {
      set_timeout(ptrPort, TIMEOUT);
    }
    return(g_bootloaderLastError);
  }

  // check acknowledge
  if (Rx[0]!=ACK) {
    console_print(STDOUT, "in 'bsl_flashMassErase()': ACK2 failure (expect 0x%02x, received 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[0]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
    if (physInterface == UART) {
      set_timeout(ptrPort, TIMEOUT);
    }
    return(g_bootloaderLastError);
  }

  // measure time for mass erase
  tStop = millis();

  // restore timeout
  set_timeout(ptrPort, TIMEOUT);


  // print message
  if (verbose == SILENT) {
    console_print(STDOUT, "done\n");
  }
  else if ((verbose == INFORM) || (verbose == CHATTY)) {
    console_print(STDOUT, "done, time %1.1fs\n", (float)(tStop-tStart)/1000.0);
  }

  // avoid compiler warnings
  return(g_bootloaderLastError);

} // bsl_flashMassErase



/**
  \fn STM8gal_BootloaderErrors_t bsl_memWrite(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[out] imageBuf       memory image of data to write (16-bit array. HB!=0 indicates content)
  \param[in]  addrStart      first address to write to
  \param[in]  addrStop       last address to write to
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (STM8gal_BootloaderErrors_t)

  upload data to microcontroller memory via WRITE command
*/
STM8gal_BootloaderErrors_t bsl_memWrite(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose) {

  uint64_t         numData, countBytes, countBlock;    // size of memory image
  const uint64_t   maxBlock = 128;                      // max. length of write block
  char             Tx[1000], Rx[1000];                  // communication buffers
  uint32_t         lenTx, lenRx, len;                   // frame lengths
  uint8_t          chk;                                 // frame checksum
  int              i, j;

  g_bootloaderLastError = STM8GAL_BOOTLOADER_NO_ERROR;

  // update min/max addresses and number of bytes to write (HB!=0x00) for printout
  if ( hexfile_getImageSize(imageBuf, addrStart, addrStop, &addrStart, &addrStop, &numData) != STM8GAL_HEXFILE_NO_ERROR ) {
    g_bootloaderLastError = STM8GAL_BOOTLOADER_HEXFILE_ERROR;
    return(g_bootloaderLastError);
  }

  // print message
  if (verbose == SILENT) {
    if (numData > 1024)
      console_print(STDOUT, "  write %1.1fkB ", (float) numData/1024.0);
    else
      console_print(STDOUT, "  write %dB ", (int) numData);
  }
  else if (verbose == INFORM) {
    if (numData > 1024)
      console_print(STDOUT, "  write %1.1fkB ", (float) numData/1024.0);
    else
      console_print(STDOUT, "  write %dB ", (int) numData);
  }
  else if (verbose == CHATTY) {
    if (numData > 1024)
      console_print(STDOUT, "  write %1.1fkB in 0x%" PRIx64 " to 0x%" PRIx64 " ", (float) numData/1024.0, addrStart, addrStop);
    else
      console_print(STDOUT, "  write %dB in 0x%" PRIx64 " to 0x%" PRIx64 " ", (int) numData, addrStart, addrStop);
  }

  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort) {
    Error("port not open");
    g_bootloaderLastError = STM8GAL_BOOTLOADER_PORT_NOT_OPEN;
    return(g_bootloaderLastError);
  }


  // loop over specified address range
  // Write only defined bytes (HB!=0x00) and align to 128 to minimize write time (see UM0560 section 3.4)
  countBytes = 0;
  countBlock = 0;
  uint64_t addr = addrStart;
  while (addr <= addrStop) {

    // find next data byte (=start address of next block)
    while (((imageBuf[addr] & 0xFF00) == 0) && (addr <= addrStop))
      addr++;
    uint64_t addrBlock = addr;

    // end address reached -> done
    if (addr > addrStop)
      break;

    // set length of next data block: max 128B and align with 128 for speed (see UM0560 section 3.4)
    int lenBlock = 1;
    while ((lenBlock < maxBlock) && ((addr+lenBlock) <= addrStop) && (imageBuf[addr+lenBlock] & 0xFF00) && ((addr+lenBlock) % maxBlock)) {
      lenBlock++;
    }
    //console_print(STDOUT, "0x%04x   0x%04x   %d\n", addrBlock, addrBlock+lenBlock-1, lenBlock);

    /////
    // send write command
    /////

    // construct command
    lenTx = 2;
    Tx[0] = WRITE;
    Tx[1] = (Tx[0] ^ 0xFF);
    lenRx = 1;

    // send command
    if (physInterface == UART) {
      if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
        g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
        return(g_bootloaderLastError);
      }
    }
    #if defined(USE_SPI_ARDUINO)
      else if (physInterface == SPI_ARDUINO)
        len = send_spi_Arduino(ptrPort, lenTx, Tx);
    #endif
	#if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    else {
      console_print(STDOUT, "unknown interface %d", (int) physInterface);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
      return(g_bootloaderLastError);
    }
    if (len != lenTx) {
      console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }

    // receive response
    if (physInterface == UART) {
      if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
        g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
        return(g_bootloaderLastError);
      }
    }
    #if defined(USE_SPI_ARDUINO)
      else if (physInterface == SPI_ARDUINO)
        len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    #endif
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
    #endif
    else {
      console_print(STDOUT, "unknown interface %d", (int) physInterface);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
      return(g_bootloaderLastError);
    }
    if (len != lenRx) {
      console_print(STDOUT, "ACK1 timeout (expect %d, received %d)", lenRx, len);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
      return(g_bootloaderLastError);
    }

    // check acknowledge
    if (Rx[0]!=ACK) {
      console_print(STDOUT, "ACK1 failure (expect 0x%02x, received 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[0]));
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
      return(g_bootloaderLastError);
    }


    /////
    // send address
    /////

    // construct address + checksum (XOR over address)
    lenTx = 5;
    Tx[0] = (char) (addrBlock >> 24);
    Tx[1] = (char) (addrBlock >> 16);
    Tx[2] = (char) (addrBlock >> 8);
    Tx[3] = (char) (addrBlock);
    Tx[4] = (Tx[0] ^ Tx[1] ^ Tx[2] ^ Tx[3]);
    lenRx = 1;

    // send command
    if (physInterface == UART) {
      if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
        g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
        return(g_bootloaderLastError);
      }
    }
    #if defined(USE_SPI_ARDUINO)
      else if (physInterface == SPI_ARDUINO)
        len = send_spi_Arduino(ptrPort, lenTx, Tx);
    #endif
	#if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    else {
      console_print(STDOUT, "unknown interface %d", (int) physInterface);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
      return(g_bootloaderLastError);
    }
    if (len != lenTx) {
      console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }

    // receive response
    if (physInterface == UART) {
      if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
        g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
        return(g_bootloaderLastError);
      }
    }
    #if defined(USE_SPI_ARDUINO)
      else if (physInterface == SPI_ARDUINO)
        len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    #endif
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
    #endif
    else {
      console_print(STDOUT, "unknown interface %d", (int) physInterface);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
      return(g_bootloaderLastError);
    }
    if (len != lenRx) {
      console_print(STDOUT, "ACK2 timeout (expect %d, received %d)", lenRx, len);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
      return(g_bootloaderLastError);
    }

    // check acknowledge
    if (Rx[0]!=ACK) {
      console_print(STDOUT, "ACK2 failure (expect 0x%02x, received 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[0]));
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
      return(g_bootloaderLastError);
    }


    /////
    // send number of bytes and data
    /////

    // construct number of bytes + data + checksum
    lenTx = 0;
    Tx[lenTx++] = lenBlock-1;     // -1 from BSL
    chk         = lenBlock-1;
    for (j=0; j<lenBlock; j++) {
      Tx[lenTx] = (uint8_t) (imageBuf[addrBlock+j] & 0x00FF);  // only LB, HB indicates "defined"
      chk ^= Tx[lenTx];
      lenTx++;
      countBytes++;
    }
    Tx[lenTx++] = chk;
    lenRx = 1;


    // send command
    if (physInterface == UART) {
      if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
        g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
        return(g_bootloaderLastError);
      }
    }
    #if defined(USE_SPI_ARDUINO)
      else if (physInterface == SPI_ARDUINO)
        len = send_spi_Arduino(ptrPort, lenTx, Tx);
    #endif
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    else {
      console_print(STDOUT, "unknown interface %d", (int) physInterface);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
      return(g_bootloaderLastError);
    }
    if (len != lenTx) {
      console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }


    // receive response
    if (physInterface == UART) {
      if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
        g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
        return(g_bootloaderLastError);
      }
    }
    #if defined(USE_SPI_ARDUINO)
      else if (physInterface == SPI_ARDUINO) {
        if ((addrBlock >= PFLASH_START) && (addrBlock % 128))  // wait for flash write finished before requesting response (see UM0560, SPI timing)
          SLEEP(1200);                               // for not 128-aligned data wait >1.1s
        else
          SLEEP(20);                                 // for 128-aligned data wait >8.5ms
        len = receive_spi_Arduino(ptrPort, lenRx, Rx);
      }
    #endif
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV) {
        if ((addrTmp >= 0x8000) && (addrTmp % 128))  // wait for flash write finished before requesting response (see UM0560, SPI timing)
          SLEEP(1200);                               // for not 128-aligned data wait >1.1s
        else
          SLEEP(20);                                 // for 128-aligned data wait >8.5ms
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
      }
    #endif
    else {
      console_print(STDOUT, "unknown interface %d", (int) physInterface);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
      return(g_bootloaderLastError);
    }
    if (len != lenRx) {
      console_print(STDOUT, "ACK3 timeout (expect %d, received %d)", lenRx, len);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
      return(g_bootloaderLastError);
    }

    // check acknowledge
    if (Rx[0]!=ACK) {
      console_print(STDOUT, "ACK3 failure (expect 0x%02x, received 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[0]));
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
      return(g_bootloaderLastError);
    }

    // print progress
    if (((++countBlock) % 8) == 0) {
      if (verbose == SILENT) {
        console_print(STDOUT, ".");
        if ((countBlock % (10*8)) == 0)
          console_print(STDOUT, " ");
      }
      else if (verbose == INFORM) {
        if (numData > 1024)
          console_print(STDOUT, "%c  write %1.1fkB / %1.1fkB ", '\r', (float) countBytes/1024.0, (float) numData/1024.0);
        else
          console_print(STDOUT, "%c  write %dB / %dB ", '\r', (int) countBytes, (int) numData);
      }
      else if (verbose == CHATTY) {
        if (numData > 1024)
          console_print(STDOUT, "%c  write %1.1fkB / %1.1fkB in 0x%" PRIx64 " to 0x%" PRIx64 " ", '\r', (float) countBytes/1024.0, (float) numData/1024.0, addrStart, addrStop);
        else
          console_print(STDOUT, "%c  write %dB / %dB in 0x%" PRIx64 " to 0x%" PRIx64 " ", '\r', (int) countBytes, (int) numData, addrStart, addrStop);
      }
    }

    // go to next potential block
    addr += lenBlock;

  } // loop over address range

  // print message
  if (verbose == SILENT)
    console_print(STDOUT, " done\n");
  else if (verbose == INFORM) {
    if (numData > 1024)
      console_print(STDOUT, "%c  write %1.1fkB / %1.1fkB ... done   \n", '\r', (float) countBytes/1024.0, (float) numData/1024.0);
    else
      console_print(STDOUT, "%c  write %dB / %dB ... done   \n", '\r', (int) countBytes, (int) numData);
  }
  else if (verbose == CHATTY) {
    if (numData > 1024)
      console_print(STDOUT, "%c  write %1.1fkB / %1.1fkB in 0x%" PRIx64 " to 0x%" PRIx64 " ... done   \n", '\r', (float) countBytes/1024.0, (float) numData/1024.0, addrStart, addrStop);
    else
      console_print(STDOUT, "%c  write %dB / %dB in 0x%" PRIx64 " to 0x%" PRIx64 " ... done   \n", '\r', (int) countBytes, (int) numData, addrStart, addrStop);
  }

  // avoid compiler warnings
  return(g_bootloaderLastError);

} // bsl_memWrite



/**
  \fn STM8gal_BootloaderErrors_t bsl_memVerify(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[out] imageBuf       memory image to verify (16-bit array. HB!=0 indicates content)
  \param[in]  addrStart      first address to verify
  \param[in]  addrStop       last address to verify
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (STM8gal_BootloaderErrors_t)

  Read microntroller flash memory and compare to specified RAM image.
*/
STM8gal_BootloaderErrors_t bsl_memVerify(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose) {

  // allocate and clear temporary RAM buffer (>1MByte requires dynamic allocation)
  uint16_t  *tmpImageBuf;            // RAM image buffer (high byte != 0 indicates value is set)

  g_bootloaderLastError = STM8GAL_BOOTLOADER_NO_ERROR;

  if (!(tmpImageBuf = malloc(LENIMAGEBUF * sizeof(*tmpImageBuf)))) {
    console_print(STDOUT, "Cannot allocate image buffer, try reducing LENIMAGEBUF");
    g_bootloaderLastError = STM8GAL_BOOTLOADER_CANNOT_ALLOCATE_BUFFER;
    return(g_bootloaderLastError);
  }
  memset(tmpImageBuf, 0, LENIMAGEBUF * sizeof(*tmpImageBuf));


  // loop over image and read all consecutive data blocks. Skip undefined data to avoid illegal read.
  uint64_t addr = addrStart;
  while (addr <= addrStop) {

    // find next data byte in image (=start address for next read)
    while (((imageBuf[addr] & 0xFF00) == 0) && (addr <= addrStop))
      addr++;

    // end address reached -> done
    if (addr > addrStop)
      break;

    // set length of next read-out
    int lenRead = 1;
    while (((addr+lenRead) <= addrStop) && (imageBuf[addr+lenRead] & 0xFF00)) {
      lenRead++;
    }
    //console_print(STDOUT, "0x%04x   0x%04x   %d\n", addr, addr+lenBlock-1, lenRead);

    // read back from STM8
    bsl_memRead(ptrPort, physInterface, uartMode, addr, addr+lenRead-1, tmpImageBuf, verbose);

    // go to next potential block
    addr += lenRead;

  } // loop over image


  // print messgage
  if (verbose != MUTE)
    console_print(STDOUT, "  verify memory ... ");

  // compare defined data data entries (HB!=0x00)
  for (addr=addrStart; addr<=addrStop; addr++) {
    if (imageBuf[addr] & 0xFF00) {
      if ((imageBuf[addr] & 0xFF) != (tmpImageBuf[addr] & 0xFF))
        console_print(STDOUT, "verify failed at address 0x%" PRIx64 " (0x%02x vs 0x%02x)", addr, (uint8_t) (imageBuf[addr]&0xFF), (uint8_t) (tmpImageBuf[addr]&0xFF));
    } // if data defined
  } // loop over address

  // print messgage
  if (verbose != MUTE)
    console_print(STDOUT, "done\n");

  // release temporary RAM buffer
  free(tmpImageBuf);

  // avoid compiler warnings
  return(g_bootloaderLastError);

} // bsl_memVerify



/**
  \fn STM8gal_BootloaderErrors_t bsl_jumpTo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addr, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  addr           address to jump to
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (STM8gal_BootloaderErrors_t)

  jump to address and continue code execution. Generally RAM or flash starting address
*/
STM8gal_BootloaderErrors_t bsl_jumpTo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addr, uint8_t verbose) {

  int       i;
  uint32_t  lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];

  g_bootloaderLastError = STM8GAL_BOOTLOADER_NO_ERROR;

  // print message
  if (verbose == INFORM)
    console_print(STDOUT, "  jump to 0x%" PRIx64 " ... ", addr);
  else if (verbose == CHATTY)
    console_print(STDOUT, "  jump to address 0x%" PRIx64 " ... ", addr);

  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort) {
    Error("port not open");
    g_bootloaderLastError = STM8GAL_BOOTLOADER_PORT_NOT_OPEN;
    return(g_bootloaderLastError);
  }


  /////
  // send go command
  /////

  // construct command
  lenTx = 2;
  Tx[0] = GO;
  Tx[1] = (Tx[0] ^ 0xFF);
  lenRx = 1;

  // send command
  if (physInterface == UART) {
    if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(g_bootloaderLastError);
  }
  if (len != lenTx) {
    console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
    return(g_bootloaderLastError);
  }

  // receive response
  if (physInterface == UART) {
    if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR)
      return(STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED);
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else
    {
      console_print(STDOUT, "unknown interface %d", (int) physInterface);
      g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
      return(g_bootloaderLastError);
    }
  if (len != lenRx) {
    console_print(STDOUT, "ACK1 timeout (expect %d, received %d)", lenRx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
    return(g_bootloaderLastError);
  }

  // check acknowledge
  if (Rx[0]!=ACK) {
    console_print(STDOUT, "ACK1 failure (expect 0x%02x, received 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[0]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
    return(g_bootloaderLastError);
  }

  /////
  // send address
  /////

  // construct address + checksum (XOR over address)
  lenTx = 5;
  Tx[0] = (char) (addr >> 24);
  Tx[1] = (char) (addr >> 16);
  Tx[2] = (char) (addr >> 8);
  Tx[3] = (char) (addr);
  Tx[4] = (Tx[0] ^ Tx[1] ^ Tx[2] ^ Tx[3]);
  lenRx = 1;

  // send command
  if (physInterface == UART) {
    if (send_port(ptrPort, uartMode, lenTx, Tx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(g_bootloaderLastError);
  }
  if (len != lenTx) {
    console_print(STDOUT, "sending command failed (expect %d, sent %d)", lenTx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED;
    return(g_bootloaderLastError);
  }

  // receive response
  if (physInterface == UART) {
    if (receive_port(ptrPort, uartMode, lenRx, Rx, &len) != STM8GAL_SERIALCOMMS_NO_ERROR) {
      g_bootloaderLastError = STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED;
      return(g_bootloaderLastError);
    }
  }
  #if defined(USE_SPI_ARDUINO)
    else if (physInterface == SPI_ARDUINO)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #endif
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else {
    console_print(STDOUT, "unknown interface %d", (int) physInterface);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE;
    return(g_bootloaderLastError);
  }
  if (len != lenRx) {
    console_print(STDOUT, "ACK2 timeout (expect %d, received %d)", lenRx, len);
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT;
    return(g_bootloaderLastError);
  }

  // check acknowledge
  if (Rx[0]!=ACK) {
    console_print(STDOUT, "ACK2 failure (expect 0x%02x, received 0x%02x)", (uint8_t) ACK, (uint8_t) (Rx[0]));
    g_bootloaderLastError = STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED;
    return(g_bootloaderLastError);
  }

  // print message
  if ((verbose == INFORM) || (verbose == CHATTY))
    console_print(STDOUT, "done\n");

  // avoid compiler warnings
  return(g_bootloaderLastError);

} // bsl_jumpTo

/**
  \fn STM8gal_BootloaderErrors_t Bootloader_GetLastError(void)

  return last error in the Bootloader module
*/
STM8gal_BootloaderErrors_t Bootloader_GetLastError(void) {

  return(g_bootloaderLastError);
}

/**
  \fn const char * Bootloader_GetLastErrorString(void)
   
  return last error string in the Bootloader module
*/
const char * Bootloader_GetLastErrorString(void)
{
    return(g_bootloaderErrorStrings[Bootloader_GetLastError()]);
}


// end of file
