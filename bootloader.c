/**
  \file bootloader.c
   
  \author G. Icking-Konert
  \date 2014-03-14
  \version 0.1
   
  \brief implementation of STM bootloader routines
   
  implementation of of STM bootloader routines
*/


#include "bootloader.h"
#include "main.h"
#include "serial_comm.h"
#include "spi_spidev_comm.h"
#include "spi_Arduino_comm.h"
#include "misc.h"


/**
  \fn uint8_t bsl_sync(HANDLE ptrPort, uint8_t physInterface, uint8_t verbose)
   
  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return synchronization status (0=ok, 1=fail)
  
  synchronize with microcontroller bootloader. For UART synchronize baudrate.
*/
uint8_t bsl_sync(HANDLE ptrPort, uint8_t physInterface, uint8_t verbose) {
  
  int   i, count;
  int   lenTx, lenRx, len;
  char  Tx[1000], Rx[1000];

  // print message
  if (verbose > SILENT)
    printf("  synchronize ... ");
  fflush(stdout);
  
  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort)
    Error("in 'bsl_sync()': port not open");
  
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
    if (physInterface == UART)
      len   = send_port(ptrPort, 0, lenTx, Tx);
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    if (len != lenTx)
      Error("in 'bsl_sync()': sending command failed (expect %d, sent %d)", lenTx, len);
        
    // receive response
    if (physInterface == UART) {
      len = receive_port(ptrPort, 0, lenRx, Rx);
      if ((len==1) && (Rx[0]== Tx[0])) {              // check for 1-wire echo 
        len = receive_port(ptrPort, 0, lenRx, Rx);
      }
    }
    else if (physInterface == SPI_ARDUINO)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
    #endif
    
    // increase retry counter
    count++;

    // avoid flooding the STM8
    SLEEP(10);
    
  } while ((count<50) && ((len!=lenRx) || ((Rx[0]!=ACK) && (Rx[0]!=NACK))));

  // check if ok
  if ((len==lenRx) && (Rx[0]==ACK)) {
    if (verbose > SILENT)
      printf("ok (ACK)\n");
  }
  else if ((len==lenRx) && (Rx[0]==NACK)) {
    if (verbose > SILENT)
      printf("ok (NACK)\n");
  }
  else if (len==lenRx)
    Error("in 'bsl_sync()': wrong response 0x%02x from BSL", (uint8_t) (Rx[0]));
  else
    Error("in 'bsl_sync()': no response from BSL");
  fflush(stdout);

  // purge PC input buffer
  flush_port(ptrPort); 
  SLEEP(50);              // seems to be required for some reason

  // return success
  return(0);

} // bsl_sync



/**
  \fn uint8_t bsl_getUartMode(HANDLE ptrPort, uint8_t verbose)
   
  \param[in]  ptrPort        handle to communication port
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)
  
  \return UART mode (0=duplex, 1=1-wire, 2=2-wire reply)
  
  auto-detect UART bootloader mode (see AppNote UM0560). This information is required
  to set the correct data parity and determine if local echo is required. 
*/
uint8_t bsl_getUartMode(HANDLE ptrPort, uint8_t verbose) {
  
  int   len, lenTx, lenRx;
  char  Tx[1000], Rx[1000];
  uint8_t uartMode = 255;

  // print message
  if (verbose > SILENT)
    printf("  check UART mode ... ");
  fflush(stdout);
  
  // reduce timeout for faster check
  set_timeout(ptrPort, 100);


  // check UART mode 1 via LIN echo: 1-wire, no SW reply, no parity
  set_parity(ptrPort, 2);
 
  lenTx = 2; lenRx = 1;
  Tx[0] = 0x00; Tx[1] = (Tx[0] ^ 0xFF); Rx[0] = 0x00;
  do {
    len = send_port(ptrPort, 0, lenTx, Tx);
    len = receive_port(ptrPort, 0, lenRx, Rx);
    //printf("\nmode 1: %d  0x%02x\n", len, Rx[0]);
    SLEEP(10);
  } while (len==0);

  // tested empirically...
  if (Rx[0] == ACK) {              // UART mode 0: 2-wire duplex, no SW reply, even parity
    uartMode = 0;
    set_parity(ptrPort, 2);
  }
  else if (Rx[0] == Tx[0]) {       // UART mode 1: 1-wire reply, no SW reply, no parity
    uartMode = 1;
    set_parity(ptrPort, 0);
  }
  else if (Rx[0] == NACK) {        // UART mode 2: 2-wire reply, SW reply, no parity
    uartMode = 2;
    set_parity(ptrPort, 0);
  }
  else 
    Error("in 'bsl_getUartMode()': cannot determine UART mode");

  // revert timeout
  set_timeout(ptrPort, TIMEOUT);

  // purge PC input buffer
  flush_port(ptrPort); 
  SLEEP(50);              // seems to be required for some reason
  
  // print message
  if (verbose > SILENT) {
    if (uartMode == 0)
      printf("duplex\n");
    else if (uartMode == 1)
      printf("1-wire\n");
    else
      printf("2-wire reply\n");
  }
  fflush(stdout);

  // return found mode
  return(uartMode);
 
} // bsl_getUartMode



/**
  \fn uint8_t bsl_getInfo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, int *flashsize, uint8_t *vers, uint8_t *family, uint8_t verbose)
   
  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[out] flashsize      size of flashsize in kB (required for correct W/E routines)
  \param[out] vers           BSL version number (required for correct W/E routines)
  \param[out] family         STM8 family (STM8S=1, STM8L=2)
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)
  
  \return communication status (0=ok, 1=fail)
  
  query microcontroller type and BSL version info. This information is required
  to select correct version of flash write/erase routines
*/
uint8_t bsl_getInfo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, int *flashsize, uint8_t *vers, uint8_t *family, uint8_t verbose) {
  
  int   i;
  int   lenTx, lenRx, len;
  char  Tx[1000], Rx[1000];

  // print message
  if ((verbose == INFORM) || (verbose == CHATTY))
    printf("  determine device ... ");
  fflush(stdout);

  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort)
    Error("in 'bsl_getInfo()': port not open");
  
  
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
  if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x004000, SILENT))       // STM8S
  {
    *family = STM8S;
    #ifdef DEBUG
      printf("family STM8S\n");
    #endif
  }
  else if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x00100, SILENT))   // STM8L
  {
    *family = STM8L;
    #ifdef DEBUG
      printf("family STM8L\n");
    #endif
  }
  else
    Error("in 'bsl_getInfo()': cannot identify family");


  // check if adress in flash exists. Check highest flash address to determine size
  if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x047FFF, SILENT))       // extreme density (256kB)
    *flashsize = 256;
  else if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x027FFF, SILENT))  // high density (128kB)
    *flashsize = 128;
  else if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x00FFFF, SILENT))  // medium density (32kB)
    *flashsize = 32;
  else if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x009FFF, SILENT))  // low density (8kB)
    *flashsize = 8;
  else
    Error("in 'bsl_getInfo()': cannot identify device");
  #ifdef DEBUG
    printf("flash size: %d\n", (int) (*flashsize));
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
  if (physInterface == UART)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  else if (physInterface == SPI_ARDUINO)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  if (len != lenTx)
    Error("in 'bsl_getInfo()': sending command failed (expect %d, sent %d)", lenTx, len);
    
  // receive response
  if (physInterface == UART)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  else if (physInterface == SPI_ARDUINO)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  if (len != lenRx)
    Error("in 'bsl_getInfo()': ACK timeout (expect %d, received %d)", lenRx, len);
    
  // check 2x ACKs
  if (Rx[0]!=ACK)
    Error("in 'bsl_getInfo()': start ACK failure (read 0x%2x)", Rx[0]);
  if (Rx[8]!=ACK)
    Error("in 'bsl_getInfo()': end ACK failure (read 0x%2x)", Rx[8]);

  
  // check if command codes are correct (just to be sure)
  if (Rx[3] != GET)
    Error("in 'bsl_getInfo()': wrong GET code (expect 0x%02x, received 0x%02x)", GET, Rx[3]);
  if (Rx[4] != READ)
    Error("in 'bsl_getInfo()': wrong READ code (expect 0x%02x, received 0x%02x)", READ, Rx[4]);
  if (Rx[5] != GO)
    Error("in 'bsl_getInfo()': wrong GO code (expect 0x%02x, received 0x%02x)", GO, Rx[5]);
  if (Rx[6] != WRITE)
    Error("in 'bsl_getInfo()': wrong WRITE code (expect 0x%02x, received 0x%02x)", WRITE, Rx[6]);
  if (Rx[7] != ERASE)
    Error("in 'bsl_getInfo()': wrong ERASE code (expect 0x%02x, received 0x%02x)", ERASE, Rx[7]);
  
// print BSL data
#ifdef DEBUG
  printf("    version 0x%02x\n", Rx[2]);
  printf("    command codes:\n");
  printf("      GET   0x%02x\n", Rx[3]);
  printf("      READ  0x%02x\n", Rx[4]);
  printf("      GO    0x%02x\n", Rx[5]);
  printf("      WRITE 0x%02x\n", Rx[6]);
  printf("      ERASE 0x%02x\n", Rx[7]);
  fflush(stdout);
#endif

  // copy version number
  *vers = Rx[2];
  
  // print message
  if ((verbose == INFORM) || (verbose == CHATTY)) {
    if (*family == STM8S)
      printf("ok (STM8S; %dkB flash; BSL v%x.%x)\n", *flashsize, (((*vers)&0xF0)>>4), ((*vers) & 0x0F));
    else
      printf("ok (STM8L; %dkB flash; BSL v%x.%x)\n", *flashsize, (((*vers)&0xF0)>>4), ((*vers) & 0x0F));
  }
  fflush(stdout);
  
  // avoid compiler warnings
  return(0);
  
} // bsl_getInfo



/**
  \fn uint8_t bsl_memRead(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addrStart, uint32_t addrStop, uint16_t *buf, uint8_t verbose)
   
  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  addrStart      first address to read
  \param[in]  addrStop       last address to read
  \param[out] buf            read data as 16-bit array. HB!=0 indicates content. Index 0 corresponds to addrStart
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (0=ok, 1=fail)
  
  read from microcontroller memory via READ command.
*/
uint8_t bsl_memRead(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addrStart, uint32_t addrStop, uint16_t *buf, uint8_t verbose) {

  int       i, lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];
  uint32_t  addrTmp, addrStep, numBytes, idx=0;

  // get number of bytes to read
  numBytes = addrStop - addrStart + 1;

  // print message
  if (verbose == SILENT) {
    printf("  read ");
  }
  else if (verbose == INFORM) {
    if (numBytes > 1024)
      printf("  read %1.1fkB ", (float) numBytes/1024.0);
    else
      printf("  read %dB ", numBytes);
  }
  else if (verbose == CHATTY) {
    if (numBytes > 1024)
      printf("  read %1.1fkB (0x%04x to 0x%04x) ", (float) numBytes/1024.0, (int) addrStart, (int) addrStop);
    else
      printf("  read %dB (0x%04x to 0x%04x) ", (int) numBytes, (int) addrStart, (int) addrStop);
  }
  fflush(stdout);
  
  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort)
    Error("in 'bsl_memRead()': port not open");
  
  // init data buffer
  for (i=0; i<numBytes; i++)
    buf[i] = 0;


  // loop over addresses in <=256B steps
  idx = 0;
  addrStep = 256;
  for (addrTmp=addrStart; addrTmp<=addrStop; addrTmp+=addrStep) {  
    
    // if addr too close to end of range reduce stepsize
    if (addrTmp+256 > addrStart+numBytes)
      addrStep = addrStart+numBytes-addrTmp;

  
    /////
    // send read command
    /////
  
    // construct command
    lenTx = 2;
    Tx[0] = READ;
    Tx[1] = (Tx[0] ^ 0xFF);
    lenRx = 1;
  
    // send command
    if (physInterface == UART)
      len = send_port(ptrPort, uartMode, lenTx, Tx);
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    if (len != lenTx)
      Error("in 'bsl_memRead()': sending command failed (expect %d, sent %d)", lenTx, len);
    
    // receive response
    if (physInterface == UART)
      len = receive_port(ptrPort, uartMode, lenRx, Rx);
    else if (physInterface == SPI_ARDUINO)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
    #endif
    if (len != lenRx)
      Error("in 'bsl_memRead()': ACK1 timeout");
    
    // check acknowledge
    if (Rx[0]!=ACK)
      Error("in 'bsl_memRead()': ACK1 failure (read 0x%2x)", Rx[0]);

  
    /////
    // send address
    /////
  
    // construct address + checksum (XOR over address)
    lenTx = 5;
    Tx[0] = (char) (addrTmp >> 24);
    Tx[1] = (char) (addrTmp >> 16);
    Tx[2] = (char) (addrTmp >> 8);
    Tx[3] = (char) (addrTmp);
    Tx[4] = (Tx[0] ^ Tx[1] ^ Tx[2] ^ Tx[3]);
    lenRx = 1;
  
    // send command
    if (physInterface == UART)
      len = send_port(ptrPort, uartMode, lenTx, Tx);
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    if (len != lenTx)     
      Error("in 'bsl_memRead()': sending address failed (expect %d, sent %d)", lenTx, len);

    // receive response
    if (physInterface == UART)
      len = receive_port(ptrPort, uartMode, lenRx, Rx);
    else if (physInterface == SPI_ARDUINO)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
    #endif
    if (len != lenRx)
      Error("in 'bsl_memRead()': ACK2 timeout (expect %d, received %d)", lenRx, len);
    
    // check acknowledge
    if (Rx[0]!=ACK)
      Error("in 'bsl_memRead()': ACK2 failure (read 0x%2x)", Rx[0]);

  
    /////
    // send number of bytes
    /////
  
    // construct number of bytes + checksum
    lenTx = 2;
    Tx[0] = addrStep-1;     // -1 from BSL
    Tx[1] = (Tx[0] ^ 0xFF);
    lenRx = addrStep + 1;
  
    // send command
    if (physInterface == UART)
      len = send_port(ptrPort, uartMode, lenTx, Tx);
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    if (len != lenTx)
      Error("in 'bsl_memRead()': sending range failed (expect %d, sent %d)", lenTx, len);

     
    // receive response
    if (physInterface == UART)
      len = receive_port(ptrPort, uartMode, lenRx, Rx);
    else if (physInterface == SPI_ARDUINO)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV) {
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
        //printf("0x%02x  0x%02x  0x%02x\n", Rx[0], Rx[1], Rx[2]); fflush(stdout); getchar();
      }
    #endif
    if (len != lenRx)
      Error("in 'bsl_memRead()': data timeout (expect %d, received %d)", lenRx, len);
    
    // check acknowledge
    if (Rx[0]!=ACK)
      Error("in 'bsl_memRead()': ACK3 failure (read 0x%2x)", Rx[0]);

    // copy data to buffer. Set HB to indicate data read
    for (i=1; i<lenRx; i++) {
      buf[idx++] = ((uint16_t) (Rx[i]) | 0xFF00);
      //printf("%d 0x%02x\n", i, (uint8_t) (Rx[i])); fflush(stdout); getchar();
    }
    
    // print progress
    if ((idx % 1024) == 0) {
      if (verbose == SILENT) {
        printf(".");
        if ((idx % (10*1024)) == 0)
          printf(" ");
      }
      else if (verbose == INFORM) {
        if (numBytes > 1024)
          printf("%c  read %1.1fkB ", '\r', (float) idx/1024.0);
        else
          printf("%c  read %dB ", '\r', idx);
      }
      else if (verbose == CHATTY) {
        if (numBytes > 1024)
          printf("%c  read %1.1fkB (0x%04x to 0x%04x) ", '\r', (float) idx/1024.0, (int) addrStart, (int) addrStop);
        else
          printf("%c  read %dB (0x%04x to 0x%04x) ", '\r', (int) idx, (int) addrStart, (int) addrStop);
      }
      fflush(stdout);
    }

  } // loop over address range 
  
  
  // print message
  if (verbose == SILENT)
    printf(" ok\n");
  else if (verbose == INFORM) {
    if (numBytes > 1024)
      printf("%c  read %1.1fkB ... ", '\r', (float) idx/1024.0);
    else
      printf("%c  read %dB ... ", '\r', idx);
    printf("ok\n");
  }
  else if (verbose == CHATTY) {
    if (numBytes > 1024)
      printf("%c  read %1.1fkB (0x%04x to 0x%04x) ... ", '\r', (float) idx/1024.0, (int) addrStart, (int) addrStop);
    else
      printf("%c  read %dB (0x%04x to 0x%04x) ... ", '\r', (int) idx, (int) addrStart, (int) addrStop);
    printf("ok\n");
  }
  fflush(stdout);
  
  
  // debug: print buffer
  /*
  printf("\n");
  printf("idx  addr  value\n");
  for (i=0; i<numBytes; i++) {
    printf("%3d   0x%04x    0x%02x\n", i+1, (int) (addrStart+i), (uint8_t) (buf[i]));
  }
  printf("\n");
  fflush(stdout);
  */
  
  
  // avoid compiler warnings
  return(0);
  
} // bsl_memRead



/**
  \fn uint8_t bsl_memCheck(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addr, uint8_t verbose)
   
  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  addr           address to check
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)
  
  \return communication status (0=ok, 1=fail)
  
  check if microcontrolles address exists. Specifically read 1B from microcontroller 
  memory via READ command. If it fails, memory doesn't exist. Used to get STM8 type
*/
uint8_t bsl_memCheck(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addr, uint8_t verbose) {

  int       i, lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];


  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort)
    Error("in 'bsl_memCheck()': port not open");
  

  /////
  // send read command
  /////
  
  // construct command
  lenTx = 2;
  Tx[0] = READ;
  Tx[1] = (Tx[0] ^ 0xFF);
  lenRx = 1;

  // send command
  if (physInterface == UART)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  else if (physInterface == SPI_ARDUINO)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  if (len != lenTx)
    Error("in 'bsl_memCheck()': sending command failed (expect %d, sent %d)", lenTx, len);
  
  // receive response
  if (physInterface == UART)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  else if (physInterface == SPI_ARDUINO)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  if (len != lenRx)
    Error("in 'bsl_memCheck()': ACK1 timeout (expect %d, received %d)", lenRx, len);

  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_memCheck()': ACK1 failure (read 0x%2x)", Rx[0]);

  
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
  if (physInterface == UART)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  else if (physInterface == SPI_ARDUINO)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  if (len != lenTx)
    Error("in 'bsl_memCheck()': sending address failed (expect %d, sent %d)", lenTx, len);

  // receive response
  if (physInterface == UART)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  else if (physInterface == SPI_ARDUINO)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  if (len != lenRx)
    Error("in 'bsl_memCheck()': ACK2 timeout (expect %d, received %d)", lenRx, len);
    
  // check acknowledge -> on NACK memory cannot be read -> return 0
  if (Rx[0]!=ACK) {
    return(0);
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
  if (physInterface == UART)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  else if (physInterface == SPI_ARDUINO)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  if (len != lenTx)
    Error("in 'bsl_memCheck()': sending range failed (expect %d, sent %d)", lenTx, len);

  // receive response
  if (physInterface == UART)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  else if (physInterface == SPI_ARDUINO)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  if (len != lenRx)
    Error("in 'bsl_memCheck()': data timeout (expect %d, received %d)", lenRx, len);
    
  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_memCheck()': ACK3 failure (read 0x%2x)", Rx[0]);

  // memory read succeeded -> memory exists
  return(1);
  
} // bsl_memCheck



/**
  \fn uint8_t bsl_flashSectorErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addr, uint8_t verbose)
   
  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  addr           adress within 1kB sector to erase
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)
  
  \return communication status (0=ok, 1=fail)
  
  \bug flash erase fails with timeout
  
  sector erase for microcontroller flash. Use with care!
*/
uint8_t bsl_flashSectorErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addr, uint8_t verbose) {

  int       i;
  int       lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];
  uint8_t   sector;

  // calculate sector code
  sector = (addr - PFLASH_START)/PFLASH_BLOCKSIZE;


  // print message
  if (addr>0xFFFFFF)
    printf("  erase flash address 0x%08x (code 0x%02x) ... ", addr, sector);
  else if (addr>0xFFFF)
    printf("  erase flash address 0x%06x (code 0x%02x) ... ", addr, sector);
  else
    printf("  erase flash address 0x%04x (code 0x%02x) ... ", addr, sector);
  fflush(stdout);
  
  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort)
    Error("in 'bsl_flashSectorErase()': port not open");
  

  /////
  // send erase command
  /////
  
  // construct command
  lenTx = 2;
  Tx[0] = ERASE;
  Tx[1] = (Tx[0] ^ 0xFF);
  lenRx = 1;
  
  // send command
  if (physInterface == UART)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  else if (physInterface == SPI_ARDUINO)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  if (len != lenTx)
    Error("in 'bsl_flashSectorErase()': sending command failed (expect %d, sent %d)", lenTx, len);


  // receive response
  if (physInterface == UART)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  else if (physInterface == SPI_ARDUINO)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  if (len != lenRx)
    Error("in 'bsl_flashSectorErase()': ACK1 timeout (expect %d, received %d)", lenRx, len);
  
  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_flashSectorErase()': ACK1 failure (read 0x%2x)", Rx[0]);

  
  /////
  // send code of sector to erase
  /////

  // increase timeout for long erase
  set_timeout(ptrPort, 1200);

  // construct pattern
  lenTx = 3;
  Tx[0] = 0x00;      // number of sectors to erase -1 (here only 1 sector)
  Tx[1] = sector;
  Tx[2] = (Tx[0] ^ Tx[1]);
  lenRx = 1;

  // send command
  if (physInterface == UART)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  else if (physInterface == SPI_ARDUINO)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  if (len != lenTx)
    Error("in 'bsl_flashSectorErase()': sending sector failed (expect %d, sent %d)", lenTx, len);
  
  
  // receive response
  if (physInterface == UART)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  else if (physInterface == SPI_ARDUINO) {
    SLEEP(40);                              // wait >30ms*(N=0+1) for sector erase before requesting response (see UM0560, SPI timing)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  }
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV) {
      SLEEP(40);                              // wait >30ms*(N=0+1) for sector erase before requesting response (see UM0560, SPI timing)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
    }
  #endif
  if (len != lenRx)
    Error("in 'bsl_flashSectorErase()': ACK2 timeout (expect %d, received %d)", lenTx, len);
  
  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_flashSectorErase()': ACK2 failure (read 0x%2x)", Rx[0]);
    
  // print message
  printf("ok\n");
  fflush(stdout);

  // restore timeout
  set_timeout(ptrPort, TIMEOUT);
  
  // avoid compiler warnings
  return(0);
  
} // bsl_flashSectorErase



/**
  \fn uint8_t bsl_flashMassErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint8_t verbose)
   
  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)
  
  \return communication status (0=ok, 1=fail)
  
  \bug flash erase fails with timeout
  
  mass erase microcontroller P-flash and D-flash/EEPROM. Use with care!
*/
uint8_t bsl_flashMassErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint8_t verbose) {

  int       i, lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];


  // print message
  printf("  mass erase flash ... ");
  fflush(stdout);
  
  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort)
    Error("in 'bsl_flashMassErase()': port not open");
  

  /////
  // send erase command
  /////
  
  // construct command
  lenTx = 2;
  Tx[0] = ERASE;
  Tx[1] = (Tx[0] ^ 0xFF);
  lenRx = 1;
  
  // send command
  if (physInterface == UART)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  else if (physInterface == SPI_ARDUINO)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  if (len != lenTx)
    Error("in 'bsl_flashMassErase()': sending command failed (expect %d, sent %d)", lenTx, len);

  // receive response
  if (physInterface == UART)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  else if (physInterface == SPI_ARDUINO)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  if (len != lenRx)
    Error("in 'bsl_flashMassErase()': ACK1 timeout (expect %d, received %d)", lenRx, len);
  
  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_flashMassErase()': ACK1 failure (read 0x%2x)", Rx[0]);

  
  /////
  // send 0xFF+0x00 to trigger mass erase
  /////

  // increase timeout for long erase
  set_timeout(ptrPort, 5000);

  // construct pattern
  lenTx = 2;
  Tx[0] = 0xFF;
  Tx[1] = 0x00;
  lenRx = 1;

  // send command
  if (physInterface == UART)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  else if (physInterface == SPI_ARDUINO)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  if (len != lenTx)
    Error("in 'bsl_flashMassErase()': sending trigger failed (expect %d, sent %d)", lenTx, len);
  
  
  // receive response
  if (physInterface == UART)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  else if (physInterface == SPI_ARDUINO) {
    SLEEP(1100);                              // wait >30ms*(N=32+1) for sector erase before requesting response (see UM0560, SPI timing)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  }
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV) {
      SLEEP(1100);                              // wait >30ms*(N=32+1) for sector erase before requesting response (see UM0560, SPI timing)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
    }
  #endif
  if (len != lenRx)
    Error("in 'bsl_flashMassErase()': ACK2 timeout (expect %d, received %d)", lenRx, len);
  
  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_flashMassErase()': ACK2 failure (read 0x%2x)", Rx[0]);

  // print message
  printf("ok\n");
  fflush(stdout);

  // restore timeout
  set_timeout(ptrPort, TIMEOUT);
  
  // avoid compiler warnings
  return(0);

} // bsl_flashMassErase



/**
  \fn uint8_t bsl_memWrite(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addrStart, uint32_t addrStop, uint16_t *buf, uint8_t verbose)
   
  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  addrStart      first address to write to
  \param[in]  addrStop       last address to write to
  \param[out] buf            data to write as 16-bit array. HB!=0 indicates content. Index 0 corresponds to addrStart
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)
  
  \return communication status (0=ok, 1=fail)
  
  upload data to microcontroller memory via WRITE command
*/
uint8_t bsl_memWrite(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addrStart, uint32_t addrStop, uint16_t *buf, uint8_t verbose) {

  int       i, lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];
  uint32_t  addrTmp, addrStep, numBytes, idx=0, idx2=0;
  uint8_t   chk, flagEmpty;

  // get number of bytes to read
  numBytes = addrStop - addrStart + 1;

  // print message
  if (verbose == SILENT) {
    printf("  write ");
  }
  else if (verbose == INFORM) {
    if (numBytes > 1024)
      printf("  write %1.1fkB ", (float) numBytes/1024.0);
    else
      printf("  write %dB ", numBytes);
  }
  else if (verbose == CHATTY) {
    if (numBytes > 1024)
      printf("  write %1.1fkB (0x%04x to 0x%04x) ", (float) numBytes/1024.0, (int) addrStart, (int) addrStop);
    else
      printf("  write %dB (0x%04x to 0x%04x) ", (int) numBytes, (int) addrStart, (int) addrStop);
  }
  fflush(stdout);
  
  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort)
    Error("in 'bsl_memWrite()': port not open");


  // loop over addresses in <=128B steps
  idx = 0;
  idx2 = 0;
  addrStep = 128;
  for (addrTmp=addrStart; addrTmp<=addrStop; addrTmp+=addrStep) {
  
    // if addr too close to end of range reduce stepsize
    if (addrTmp+128 > addrStart+numBytes)
      addrStep = addrStart+numBytes-addrTmp;

    // check if next block contains data (indicated by HB != 0x00). If not, skip complete block
    flagEmpty = 1;
    for (i=0; i<addrStep; i++) {
      if (buf[idx+i] & 0xFF00) {
        flagEmpty = 0;
        break;
      }
    }
    if (flagEmpty) {
      idx += addrStep;
      continue;
    }
      

    /////
    // send write command
    /////
  
    // construct command
    lenTx = 2;
    Tx[0] = WRITE;
    Tx[1] = (Tx[0] ^ 0xFF);
    lenRx = 1;
  
    // send command
    if (physInterface == UART)
      len = send_port(ptrPort, uartMode, lenTx, Tx);
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    if (len != lenTx)
      Error("in 'bsl_memWrite()': sending command failed (expect %d, sent %d)", lenTx, len);
    
    // receive response
    if (physInterface == UART)
      len = receive_port(ptrPort, uartMode, lenRx, Rx);
    else if (physInterface == SPI_ARDUINO)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
    #endif
    if (len != lenRx)
      Error("in 'bsl_memWrite()': ACK1 timeout (expect %d, received %d)", lenRx, len);
    
    // check acknowledge
    if (Rx[0]!=ACK)
      Error("in 'bsl_memWrite()': ACK1 failure (read 0x%2x)", Rx[0]);
 
  
    /////
    // send address
    /////
  
    // construct address + checksum (XOR over address)
    lenTx = 5;
    Tx[0] = (char) (addrTmp >> 24);
    Tx[1] = (char) (addrTmp >> 16);
    Tx[2] = (char) (addrTmp >> 8);
    Tx[3] = (char) (addrTmp);
    Tx[4] = (Tx[0] ^ Tx[1] ^ Tx[2] ^ Tx[3]);
    lenRx = 1;
  
    // send command
    if (physInterface == UART)
      len = send_port(ptrPort, uartMode, lenTx, Tx);
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    if (len != lenTx)
      Error("in 'bsl_memWrite()': sending address failed (expect %d, sent %d)", lenTx, len);
    
  
    // receive response
    if (physInterface == UART)
      len = receive_port(ptrPort, uartMode, lenRx, Rx);
    else if (physInterface == SPI_ARDUINO)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
    #endif
    if (len != lenRx)
      Error("in 'bsl_memWrite()': ACK2 timeout (expect %d, received %d)", lenRx, len);
    
    // check acknowledge
    if (Rx[0]!=ACK)
      Error("in 'bsl_memWrite()': ACK2 failure (read 0x%2x)", Rx[0]);

  
    /////
    // send number of bytes and data
    /////
  
    // construct number of bytes + data + checksum
    lenTx = 0;
    Tx[lenTx++] = addrStep-1;     // -1 from BSL
    chk         = addrStep-1;
    for (i=0; i<addrStep; i++) {
      Tx[lenTx] = (uint8_t) (buf[idx++] & 0x00FF);  // only LB, HB idicates data
      idx2++;                                       // only used for printing
      chk ^= Tx[lenTx];
      lenTx++;
    }
    Tx[lenTx++] = chk;
    lenRx = 1;

      
    // send command
    if (physInterface == UART)
      len = send_port(ptrPort, uartMode, lenTx, Tx);
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    if (len != lenTx)
      Error("in 'bsl_memWrite()': sending data failed (expect %d, sent %d)", lenTx, len);
    
        
    // receive response
    if (physInterface == UART)
      len = receive_port(ptrPort, uartMode, lenRx, Rx);
    else if (physInterface == SPI_ARDUINO) {
      if ((addrTmp >= 0x8000) && (addrTmp % 128))  // wait for flash write finished before requesting response (see UM0560, SPI timing)
        SLEEP(1200);                               // for not 128-aligned data wait >1.1s
      else
        SLEEP(20);                                 // for 128-aligned data wait >8.5ms
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    }
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV) {
        if ((addrTmp >= 0x8000) && (addrTmp % 128))  // wait for flash write finished before requesting response (see UM0560, SPI timing)
          SLEEP(1200);                               // for not 128-aligned data wait >1.1s
        else
          SLEEP(20);                                 // for 128-aligned data wait >8.5ms
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
      }
    #endif
    if (len != lenRx)
      Error("in 'bsl_memWrite()': ACK3 timeout (expect %d, received %d)", lenRx, len);
    
    // check acknowledge
    if (Rx[0]!=ACK)
      Error("in 'bsl_memWrite()': ACK3 failure (read 0x%2x)", Rx[0]);
    
    // print progress
    if ((idx2 % 1024) == 0) {
      if (verbose == SILENT) {
        printf(".");
        if ((idx2 % (10*1024)) == 0)
          printf(" ");
      }
      else if (verbose == INFORM) {
        if (numBytes > 1024)
          printf("%c  write %1.1fkB ", '\r', (float) idx2/1024.0);
        else
          printf("%c  write %dB ", '\r', (int) idx2);
      }
      else if (verbose == CHATTY) {
        if (numBytes > 1024)
          printf("%c  write %1.1fkB (0x%04x to 0x%04x) ", '\r', (float) idx2/1024.0, (int) addrStart, (int) addrStop);
        else
          printf("%c  write %dB (0x%04x to 0x%04x) ", '\r', (int) idx2, (int) addrStart, (int) addrStop);
      }
      fflush(stdout);
    }
    
  } // loop over address range 
  
  // print message
  if (verbose == SILENT)
    printf(" ok\n");
  else if (verbose == INFORM) {
    if (numBytes > 1024)
      printf("%c  write %1.1fkB ... ok   \n", '\r', (float) idx2/1024.0);
    else
      printf("%c  write %dB ... ok   \n", '\r', idx2);
  }
  else if (verbose == CHATTY) {
    if (numBytes > 1024)
      printf("%c  write %1.1fkB (0x%04x to 0x%04x) ... ok   \n", '\r', (float) idx2/1024.0, (int) addrStart, (int) addrStop);
    else
      printf("%c  write %dB (0x%04x to 0x%04x) ... ok   \n", '\r', idx2, (int) addrStart, (int) addrStop);
  }
  
  // avoid compiler warnings
  return(0);
  
} // bsl_memWrite



/**
  \fn uint8_t bsl_jumpTo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addr, uint8_t verbose)
   
  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  addr           address to jump to
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)
  
  \return communication status (0=ok, 1=fail)
  
  jump to address and continue code execution. Generally RAM or flash starting address
*/
uint8_t bsl_jumpTo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addr, uint8_t verbose) {

  int       i;
  int       lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];

  // print message
  if (verbose == INFORM)
    printf("  jump to 0x%04x ... ", addr);
  else if (verbose == CHATTY)
    printf("  jump to address 0x%04x ... ", addr);
  fflush(stdout);

  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort)
    Error("in 'bsl_jumpTo()': port not open");
  

  /////
  // send go command
  /////
  
  // construct command
  lenTx = 2;
  Tx[0] = GO;
  Tx[1] = (Tx[0] ^ 0xFF);
  lenRx = 1;
  
  // send command
  if (physInterface == UART)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  else if (physInterface == SPI_ARDUINO)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  if (len != lenTx)
    Error("in 'bsl_jumpTo()': sending command failed (expect %d, sent %d)", lenTx, len);
    
  // receive response
  if (physInterface == UART)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  else if (physInterface == SPI_ARDUINO)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  if (len != lenRx)
    Error("in 'bsl_jumpTo()': ACK1 timeout (expect %d, received %d)", lenRx, len);
  
  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_jumpTo()': ACK1 failure (read 0x%2x)", Rx[0]);

  
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
  if (physInterface == UART)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  else if (physInterface == SPI_ARDUINO)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  if (len != lenTx)
    Error("in 'bsl_jumpTo()': sending address failed (expect %d, sent %d)", lenTx, len);
  
  // receive response
  if (physInterface == UART)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  else if (physInterface == SPI_ARDUINO)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  if (len != lenRx)
    Error("in 'bsl_jumpTo()': ACK2 timeout (expect %d, received %d)", lenRx, len);
  
  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_jumpTo()': ACK2 failure (read 0x%2x)", Rx[0]);

  // print message
  if ((verbose == INFORM) || (verbose == CHATTY))
    printf("ok\n");
  fflush(stdout);
    
  // avoid compiler warnings
  return(0);
  
} // bsl_jumpTo


// end of file
