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
#include "globals.h"


/**
  \fn uint8_t bsl_sync(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode)
   
  \brief synchronize to microcontroller BSL
   
  \param[in] ptrPort        handle to communication port
  \param[in] physInterface  bootloader interface: 0=UART (default), 1=SPI
  \param[in] uartMode       UART bootloader mode: 0=duplex, 1=1-wire reply, 2=2-wire reply

  \return synchronization status (0=ok, 1=fail)
  
  synchronize to microcontroller BSL, e.g. baudrate. If already synchronized
  checks for NACK
*/
uint8_t bsl_sync(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode) {
  
  int   i, count;
  int   lenTx, lenRx, len;
  char  Tx[1000], Rx[1000];

  // print message
  printf("  synchronize ... ");
  fflush(stdout);
  
  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_sync()': port not open, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }
  
  
  // purge UART input buffer
  if (physInterface == 0) {
    flush_port(ptrPort); 
  }
  
  // construct SYNC command
  lenTx = 1;
  Tx[0] = SYNCH;
  lenRx = 1;  
  
  count = 0;
  do {
    
    // send command
    if (physInterface == 0)
      len = send_port(ptrPort, uartMode, lenTx, Tx);
    #if defined(USE_SPIDEV)
      else if (physInterface == 1)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    else if (physInterface == 2)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
    if (len != lenTx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_sync()': sending command failed (expect %d, sent %d), exit!\n\n", lenTx, len);
      Exit(1, g_pauseOnExit);
    }
        
    // receive response
    if (physInterface == 0)
      len = receive_port(ptrPort, uartMode, lenRx, Rx);
    #if defined(USE_SPIDEV)
      else if (physInterface == 1)
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
    #endif
    else if (physInterface == 2)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    
    // increase retry counter
    count++;

    // avoid flooding the STM8
    SLEEP(10);
    
  } while ((count<15) && ((len!=lenRx) || ((Rx[0]!=ACK) && (Rx[0]!=NACK))));
  
  // check if ok
  if ((len==lenRx) && (Rx[0]==ACK)) {
    printf("ok (ACK)\n");
    fflush(stdout);
  }
  else if ((len==lenRx) && (Rx[0]==NACK)) {
    printf("ok (NACK)\n");
    fflush(stdout);
  }
  else if (len==lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_sync()': wrong response 0x%02x from BSL, exit!\n\n", (uint8_t) (Rx[0]));
    Exit(1, g_pauseOnExit);
  }
  else {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_sync()': no response from BSL, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  // return success
  return(0);

} // bsl_sync



/**
  \fn uint8_t bsl_getInfo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, int *flashsize, uint8_t *vers, uint8_t *family)
   
  \brief get microcontroller type and BSL version (for correct w/e routines)
   
  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire reply, 2=2-wire reply
  \param[out] flashsize      size of flashsize in kB (required for correct W/E routines)
  \param[out] vers           BSL version number (required for correct W/E routines)
  \param[out] family         STM8 family (STM8S=1, STM8L=2)
  
  \return communication status (0=ok, 1=fail)
  
  query microcontroller type and BSL version info. This information is required
  to select correct version of flash write/erase routines
*/
uint8_t bsl_getInfo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, int *flashsize, uint8_t *vers, uint8_t *family) {
  
  int   i;
  int   lenTx, lenRx, len;
  char  Tx[1000], Rx[1000];

  // print message
  if (g_verbose == 2)
    printf("  determine device ... ");
  fflush(stdout);

  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': port not open, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }
  
  
  // purge input buffer
  flush_port(ptrPort); 
  SLEEP(50);              // seems to be required for some reason
  
  
  /////////
  // determine device flash size for selecting w/e routines (flash starts at PFLASH_START)
  /////////

  // reduce timeout for faster check
  if (physInterface == 0) {
    set_timeout(ptrPort, 200);
  }
  
  // check address of EEPROM. STM8L starts at 0x1000, STM8S starts at 0x4000
  if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x004000))       // STM8S
  {
    *family = STM8S;
    #ifdef DEBUG
      printf("family STM8S\n");
    #endif
  }
  else if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x00100))   // STM8L
  {
    *family = STM8L;
    #ifdef DEBUG
      printf("family STM8L\n");
    #endif
  }
  else {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': cannot identify family, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }


  // check if adress in flash exists. Check highest flash address to determine size
  if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x047FFF))       // extreme density (256kB)
    *flashsize = 256;
  else if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x027FFF))  // high density (128kB)
    *flashsize = 128;
  else if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x00FFFF))  // medium density (32kB)
    *flashsize = 32;
  else if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x009FFF))  // low density (8kB)
    *flashsize = 8;
  else {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': cannot identify device, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }
  #ifdef DEBUG
    printf("flash size: %d\n", (int) (*flashsize));
  #endif
  

  // restore timeout to avoid timeouts during flash operation
  if (physInterface == 0) {
    set_timeout(ptrPort, 1000);
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
  if (physInterface == 0)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else if (physInterface == 2)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': sending command failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    Exit(1, g_pauseOnExit);
  }
    
  // receive response
  if (physInterface == 0)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else if (physInterface == 2)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': ACK timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    Exit(1, g_pauseOnExit);
  }
    
  // check 2x ACKs
  if ((Rx[0]!=ACK) || (Rx[8]!=ACK)) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': ACK failure, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  
  // check if command codes are correct (just to be sure)
  if (Rx[3] != GET) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': wrong GET code (expect 0x%02x, received 0x%02x), exit!\n\n", GET, Rx[3]);
    Exit(1, g_pauseOnExit);
  }
  if (Rx[4] != READ) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': wrong READ code (expect 0x%02x, received 0x%02x), exit!\n\n", READ, Rx[4]);
    Exit(1, g_pauseOnExit);
  }
  if (Rx[5] != GO) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': wrong GO code (expect 0x%02x, received 0x%02x), exit!\n\n", GO, Rx[5]);
    Exit(1, g_pauseOnExit);
  }
  if (Rx[6] != WRITE) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': wrong WRITE code (expect 0x%02x, received 0x%02x), exit!\n\n", WRITE, Rx[6]);
    Exit(1, g_pauseOnExit);
  }
  if (Rx[7] != ERASE) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': wrong ERASE code (expect 0x%02x, received 0x%02x), exit!\n\n", ERASE, Rx[7]);
    Exit(1, g_pauseOnExit);
  }
  
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
  if (g_verbose == 1) {
    if (*family == STM8S)
      printf("  found STM8S; %dkB flash; BSL v%x.%x\n", *flashsize, (((*vers)&0xF0)>>4), ((*vers) & 0x0F));
    else
      printf("  found STM8L; %dkB flash; BSL v%x.%x\n", *flashsize, (((*vers)&0xF0)>>4), ((*vers) & 0x0F));
  }
  else if (g_verbose == 2) {
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
  \fn uint8_t bsl_memRead(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addrStart, uint32_t numBytes, char *buf) 
   
  \brief read from microcontroller memory
   
  \param[in] ptrPort        handle to communication port
  \param[in] physInterface  bootloader interface: 0=UART (default), 1=SPI
  \param[in] uartMode       UART bootloader mode: 0=duplex, 1=1-wire reply, 2=2-wire reply
  \param[in] addrStart      starting address to read from
  \param[in] numBytes       number of bytes to read
  \param[in] buf            buffer to store data to

  \return communication status (0=ok, 1=fail)
  
  read from microcontroller memory via READ command
*/
uint8_t bsl_memRead(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addrStart, uint32_t numBytes, char *buf) {

  int       i, lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];
  uint32_t  addrTmp, addrStep, idx=0;


  // print message
  if (g_verbose == 2) {
    if (numBytes > 1024)
      printf("  read  %1.1fkB (0x%04x to 0x%04x) ", (float) numBytes/1024.0, (int) addrStart, (int) (addrStart+numBytes));
    else
      printf("  read  %dB (0x%04x to 0x%04x) ", numBytes, (int) addrStart, (int) (addrStart+numBytes));
  }
  else if (g_verbose == 1) {
    if (numBytes > 1024)
      printf("  read  %1.1fkB ", (float) numBytes/1024.0);
    else
      printf("  read  %dB ", numBytes);
  }
  else if (g_verbose == 0) {
    printf("  read ");
  }
  fflush(stdout);
  
  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memRead()': port not open, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }
  
  // init data buffer
  for (i=0; i<numBytes; i++)
    buf[i] = 0;


  // loop over addresses in <=256B steps
  idx = 0;
  addrStep = 256;
  for (addrTmp=addrStart; addrTmp<addrStart+numBytes; addrTmp+=addrStep) {  
    
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
    if (physInterface == 0)
      len = send_port(ptrPort, uartMode, lenTx, Tx);
    #if defined(USE_SPIDEV)
      else if (physInterface == 1)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    else if (physInterface == 2)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
    if (len != lenTx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': sending command failed (expect %d, sent %d), exit!\n\n", lenTx, len);
      Exit(1, g_pauseOnExit);
    }
    
    // receive response
    if (physInterface == 0)
      len = receive_port(ptrPort, uartMode, lenRx, Rx);
    #if defined(USE_SPIDEV)
      else if (physInterface == 1)
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
    #endif
    else if (physInterface == 2)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    if (len != lenRx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': ACK1 timeout, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }
    
    // check acknowledge
    if (Rx[0]!=ACK) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': ACK1 failure 0x%2x, exit!\n\n", Rx[0]);
      Exit(1, g_pauseOnExit);
    }

  
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
    if (physInterface == 0)
      len = send_port(ptrPort, uartMode, lenTx, Tx);
    #if defined(USE_SPIDEV)
      else if (physInterface == 1)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    else if (physInterface == 2)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
    if (len != lenTx) {      
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': sending address failed (expect %d, sent %d), exit!\n\n", lenTx, len);
      Exit(1, g_pauseOnExit);
    }

    // receive response
    if (physInterface == 0)
      len = receive_port(ptrPort, uartMode, lenRx, Rx);
    #if defined(USE_SPIDEV)
      else if (physInterface == 1)
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
    #endif
    else if (physInterface == 2)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    if (len != lenRx) {      
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': ACK2 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
      Exit(1, g_pauseOnExit);
    }
    
    // check acknowledge
    if (Rx[0]!=ACK) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': ACK2 failure, exit!\n\n");
      Exit(1, g_pauseOnExit);
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
    if (physInterface == 0)
      len = send_port(ptrPort, uartMode, lenTx, Tx);
    #if defined(USE_SPIDEV)
      else if (physInterface == 1)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    else if (physInterface == 2)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
    if (len != lenTx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': sending range failed (expect %d, sent %d), exit!\n\n", lenTx, len);
      Exit(1, g_pauseOnExit);
    }

     
    // receive response
    if (physInterface == 0)
      len = receive_port(ptrPort, uartMode, lenRx, Rx);
    #if defined(USE_SPIDEV)
      else if (physInterface == 1) {
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
        //printf("0x%02x  0x%02x  0x%02x\n", Rx[0], Rx[1], Rx[2]); fflush(stdout); getchar();
      }
    #endif
    else if (physInterface == 2)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    if (len != lenRx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': data timeout (expect %d, received %d), exit!\n\n", lenRx, len);
      Exit(1, g_pauseOnExit);
    }
    
    // check acknowledge
    if (Rx[0]!=ACK) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': ACK3 failure, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }

    // copy data to buffer
    for (i=1; i<lenRx; i++) {
      buf[idx++] = Rx[i];
      //printf("%d 0x%02x\n", i, (uint8_t) (Rx[i])); fflush(stdout); getchar();
    }
    
    // print progress
    if ((idx % 1024) == 0) {
      if (g_verbose == 2) {
        if (numBytes > 1024)
          printf("%c  read  %1.1fkB (0x%04x to 0x%04x) ", '\r', (float) idx/1024.0, (int) addrStart, (int) (addrStart+numBytes));
        else
          printf("%c  read  %dB (0x%04x to 0x%04x) ", '\r', idx, (int) addrStart, (int) (addrStart+numBytes));
      }
      else if (g_verbose == 1) {
        if (numBytes > 1024)
          printf("%c  read  %1.1fkB ", '\r', (float) idx/1024.0);
        else
          printf("%c  read  %dB ", '\r', idx);
      }
      else if (g_verbose == 0) {
        printf(".");
      }
      fflush(stdout);
    }

  } // loop over address range 
  
  
  // print message
  if (g_verbose == 2) {
    if (numBytes > 1024)
      printf("%c  read  %1.1fkB (0x%04x to 0x%04x) ... ", '\r', (float) idx/1024.0, (int) addrStart, (int) (addrStart+numBytes));
    else
      printf("%c  read  %dB (0x%04x to 0x%04x) ... ", '\r', idx, (int) addrStart, (int) (addrStart+numBytes));
    printf("ok\n");
  }
  else if (g_verbose == 1) {
    if (numBytes > 1024)
      printf("%c  read  %1.1fkB ... ", '\r', (float) idx/1024.0);
    else
      printf("%c  read  %dB ... ", '\r', idx);
    printf("ok\n");
  }
  else if (g_verbose == 0) {
    printf(" ok\n");
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
  \fn uint8_t bsl_memCheck(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addr)
   
  \brief check if address exists
      
  \param[in] ptrPort        handle to communication port
  \param[in] physInterface  bootloader interface: 0=UART (default), 1=SPI
  \param[in] uartMode       UART bootloader mode: 0=duplex, 1=1-wire reply, 2=2-wire reply
  \param[in] addr           address to check
  
  \return communication status (0=ok, 1=fail)
  
  check if microcontrolles address exists. Specifically read 1B from microcontroller 
  memory via READ command. If it fails, memory doesn't exist. Used to get STM8 type
*/
uint8_t bsl_memCheck(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addr) {

  int       i, lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];


  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': port not open, exit!\n\n");
    Exit(1, g_pauseOnExit);
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
  if (physInterface == 0)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else if (physInterface == 2)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': sending command failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    Exit(1, g_pauseOnExit);
  }
    
  // receive response
  if (physInterface == 0)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else if (physInterface == 2)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': ACK1 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    Exit(1, g_pauseOnExit);
  }
    
  // check acknowledge
  if (Rx[0]!=ACK) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': ACK1 failure 0x%2x, exit!\n\n", Rx[0]);
    Exit(1, g_pauseOnExit);
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
  if (physInterface == 0)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else if (physInterface == 2)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  if (len != lenTx) {      
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': sending address failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    Exit(1, g_pauseOnExit);
  }

  // receive response
  if (physInterface == 0)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else if (physInterface == 2)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  if (len != lenRx) {      
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': ACK2 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    Exit(1, g_pauseOnExit);
  }
    
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
  if (physInterface == 0)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else if (physInterface == 2)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': sending range failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    Exit(1, g_pauseOnExit);
  }

  // receive response
  if (physInterface == 0)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else if (physInterface == 2)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': data timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    Exit(1, g_pauseOnExit);
  }
    
  // check acknowledge
  if (Rx[0]!=ACK) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': ACK3 failure, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  // memory read succeeded -> memory exists
  return(1);
  
} // bsl_memCheck



/**
  \fn uint8_t bsl_flashSectorErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addr)
   
  \brief erase one microcontroller flash sector
  
  \param[in] ptrPort        handle to communication port
  \param[in] physInterface  bootloader interface: 0=UART (default), 1=SPI
  \param[in] uartMode       UART bootloader mode: 0=duplex, 1=1-wire reply, 2=2-wire reply
  \param[in] addr           adress within 1kB sector to erase
  
  \return communication status (0=ok, 1=fail)
  
  sector erase for microcontroller flash
*/
uint8_t bsl_flashSectorErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addr) {

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
  if (!ptrPort) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashSectorErase()': port not open, exit!\n\n");
    Exit(1, g_pauseOnExit);
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
  if (physInterface == 0)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else if (physInterface == 2)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashSectorErase()': sending command failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    Exit(1, g_pauseOnExit);
  }


  // receive response
  if (physInterface == 0)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else if (physInterface == 2)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashSectorErase()': ACK1 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    Exit(1, g_pauseOnExit);
  }
  
  // check acknowledge
  if (Rx[0]!=ACK) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashSectorErase()': ACK1 failure, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  
  /////
  // send code of sector to erase
  /////

  // construct pattern
  lenTx = 3;
  Tx[0] = 0x00;      // number of sectors to erase -1 (here only 1 sector)
  Tx[1] = sector;
  Tx[2] = (Tx[0] ^ Tx[1]);
  lenRx = 1;

  // send command
  if (physInterface == 0)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else if (physInterface == 2)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashSectorErase()': sending sector failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    Exit(1, g_pauseOnExit);
  }
  
  
  // receive response
  if (physInterface == 0)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1) {
      SLEEP(40);                              // wait >30ms*(N=0+1) for sector erase before requesting response (see UM0560, SPI timing)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
    }
  #endif
  else if (physInterface == 2) {
    SLEEP(40);                              // wait >30ms*(N=0+1) for sector erase before requesting response (see UM0560, SPI timing)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  }
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashSectorErase()': ACK2 timeout (expect %d, received %d), exit!\n\n", lenTx, len);
    Exit(1, g_pauseOnExit);
  }
  
  // check acknowledge
  if (Rx[0]!=ACK) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashSectorErase()': ACK2 failure, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

    
  // print message
  printf("ok\n");
  fflush(stdout);
  
  // avoid compiler warnings
  return(0);
  
} // bsl_flashSectorErase



/**
  \fn uint8_t bsl_flashMassErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode)
   
  \brief mass erase microcontroller flash
  
  \param[in] ptrPort        handle to communication port
  \param[in] physInterface  bootloader interface: 0=UART (default), 1=SPI
  \param[in] uartMode       UART bootloader mode: 0=duplex, 1=1-wire reply, 2=2-wire reply
  
  \return communication status (0=ok, 1=fail)
  
  mass erase microcontroller P-flash and D-flash/EEPROM
*/
uint8_t bsl_flashMassErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode) {

  int       i, lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];


  // print message
  printf("  mass erase flash ... ");
  fflush(stdout);
  
  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': port not open, exit!\n\n");
    Exit(1, g_pauseOnExit);
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
  if (physInterface == 0)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else if (physInterface == 2)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': sending command failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    Exit(1, g_pauseOnExit);
  }

  // receive response
  if (physInterface == 0)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else if (physInterface == 2)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': ACK1 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    Exit(1, g_pauseOnExit);
  }
  
  // check acknowledge
  if (Rx[0]!=ACK) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': ACK1 failure, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  
  /////
  // send 0xFF+0x00 to trigger mass erase
  /////

  // construct pattern
  lenTx = 2;
  Tx[0] = 0xFF;
  Tx[1] = 0x00;
  lenRx = 1;

  // send command
  if (physInterface == 0)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else if (physInterface == 2)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': sending trigger failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    Exit(1, g_pauseOnExit);
  }
  
  
  // receive response
  if (physInterface == 0)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1) {
      SLEEP(1100);                              // wait >30ms*(N=32+1) for sector erase before requesting response (see UM0560, SPI timing)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
    }
  #endif
  else if (physInterface == 2) {
    SLEEP(1100);                              // wait >30ms*(N=32+1) for sector erase before requesting response (see UM0560, SPI timing)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  }
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': ACK2 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    Exit(1, g_pauseOnExit);
  }
  
  // check acknowledge
  if (Rx[0]!=ACK) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': ACK2 failure, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  // print message
  printf("ok\n");
  fflush(stdout);
  
  // avoid compiler warnings
  return(0);

} // bsl_flashMassErase



/**
  \fn uint8_t bsl_memWrite(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addrStart, uint32_t numBytes, char *buf, int verbose)
   
  \brief upload to microcontroller flash or RAM
   
  \param[in] ptrPort        handle to communication port
  \param[in] physInterface  bootloader interface: 0=UART (default), 1=SPI
  \param[in] uartMode       UART bootloader mode: 0=duplex, 1=1-wire reply, 2=2-wire reply
  \param[in] addrStart      starting address to upload to
  \param[in] numBytes       number of bytes to upload
  \param[in] buf            buffer containing data
  \param[in] verbose        verbosity of console output (-1=none)
  
  \return communication status (0=ok, 1=fail)
  
  upload data to microcontroller memory via WRITE command
*/
uint8_t bsl_memWrite(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addrStart, uint32_t numBytes, char *buf, int verbose) {

  int       i, lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];
  uint32_t  addrTmp, addrStep, idx=0, idx2=0;
  uint8_t   chk, flagEmpty;

  // print message
  if (verbose == 2) {
    if (numBytes > 1024)
      printf("  write %1.1fkB (0x%04x to 0x%04x) ", (float) numBytes/1024.0, (int) addrStart, (int) (addrStart+numBytes));
    else
      printf("  write %dB (0x%04x to 0x%04x) ", numBytes, (int) addrStart, (int) (addrStart+numBytes));
  }
  else if (verbose == 1) {
    if (numBytes > 1024)
      printf("  write %1.1fkB ", (float) numBytes/1024.0);
    else
      printf("  write %dB ", numBytes);
  }
  else if (verbose == 0)
    printf("  write ");
  fflush(stdout);
  
  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memWrite()': port not open, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }


  // loop over addresses in <=128B steps
  idx = 0;
  idx2 = 0;
  addrStep = 128;
  for (addrTmp=addrStart; addrTmp<addrStart+numBytes; addrTmp+=addrStep) {
  
    // if addr too close to end of range reduce stepsize
    if (addrTmp+128 > addrStart+numBytes)
      addrStep = addrStart+numBytes-addrTmp;

    // check if next block contains data. If not, skip complete block
    flagEmpty = 1;
    for (i=0; i<addrStep; i++) {
      if (buf[idx+i]) {
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
    if (physInterface == 0)
      len = send_port(ptrPort, uartMode, lenTx, Tx);
    #if defined(USE_SPIDEV)
      else if (physInterface == 1)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    else if (physInterface == 2)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
    if (len != lenTx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': sending command failed (expect %d, sent %d), exit!\n\n", lenTx, len);
      Exit(1, g_pauseOnExit);
    }
    
    // receive response
    if (physInterface == 0)
      len = receive_port(ptrPort, uartMode, lenRx, Rx);
    #if defined(USE_SPIDEV)
      else if (physInterface == 1)
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
    #endif
    else if (physInterface == 2)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    if (len != lenRx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK1 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
      Exit(1, g_pauseOnExit);
    }
    
    // check acknowledge
    if (Rx[0]!=ACK) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK1 failure, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }
 
  
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
    if (physInterface == 0)
      len = send_port(ptrPort, uartMode, lenTx, Tx);
    #if defined(USE_SPIDEV)
      else if (physInterface == 1)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    else if (physInterface == 2)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
    if (len != lenTx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': sending address failed (expect %d, sent %d), exit!\n\n", lenTx, len);
      Exit(1, g_pauseOnExit);
    }
    
  
    // receive response
    if (physInterface == 0)
      len = receive_port(ptrPort, uartMode, lenRx, Rx);
    #if defined(USE_SPIDEV)
      else if (physInterface == 1)
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
    #endif
    else if (physInterface == 2)
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    if (len != lenRx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK2 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
      Exit(1, g_pauseOnExit);
    }
    
    // check acknowledge
    if (Rx[0]!=ACK) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK2 failure, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }

  
    /////
    // send number of bytes and data
    /////
  
    // construct number of bytes + data + checksum
    lenTx = 0;
    Tx[lenTx++] = addrStep-1;     // -1 from BSL
    chk         = addrStep-1;
    for (i=0; i<addrStep; i++) {
      Tx[lenTx] = buf[idx++];
      idx2++;                     // only used for printing
      chk ^= Tx[lenTx];
      lenTx++;
    }
    Tx[lenTx++] = chk;
    lenRx = 1;

      
    // send command
    if (physInterface == 0)
      len = send_port(ptrPort, uartMode, lenTx, Tx);
    #if defined(USE_SPIDEV)
      else if (physInterface == 1)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    else if (physInterface == 2)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
    if (len != lenTx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': sending data failed (expect %d, sent %d), exit!\n\n", lenTx, len);
      Exit(1, g_pauseOnExit);
    }
    
        
    // receive response
    if (physInterface == 0)
      len = receive_port(ptrPort, uartMode, lenRx, Rx);
    #if defined(USE_SPIDEV)
      else if (physInterface == 1) {
        if ((addrTmp >= 0x8000) && (addrTmp % 128))  // wait for flash write finished before requesting response (see UM0560, SPI timing)
          SLEEP(1200);                               // for not 128-aligned data wait >1.1s
        else
          SLEEP(20);                                 // for 128-aligned data wait >8.5ms
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
      }
    #endif
    else if (physInterface == 2) {
      if ((addrTmp >= 0x8000) && (addrTmp % 128))  // wait for flash write finished before requesting response (see UM0560, SPI timing)
        SLEEP(1200);                               // for not 128-aligned data wait >1.1s
      else
        SLEEP(20);                                 // for 128-aligned data wait >8.5ms
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
    }
    if (len != lenRx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK3 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
      Exit(1, g_pauseOnExit);
    }
    
    // check acknowledge
    if (Rx[0]!=ACK) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK3 failure, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }
    
    // print progress
    if ((idx2 % 1024) == 0) {
      if (verbose == 2) {
        if (numBytes > 1024)
          printf("%c  write %1.1fkB (0x%04x to 0x%04x) ", '\r', (float) idx2/1024.0, (int) addrStart, (int) (addrStart+numBytes));
        else
          printf("%c  write %dB (0x%04x to 0x%04x) ", '\r', idx2, (int) addrStart, (int) (addrStart+numBytes));
      }
      else if (verbose == 1) {
        if (numBytes > 1024)
          printf("%c  write %1.1fkB ", '\r', (float) idx2/1024.0);
        else
          printf("%c  write %dB ", '\r', idx2);
      }
      else if (verbose == 0)
        printf(".");
      fflush(stdout);
    }
    
  } // loop over address range 
  
  // print message
  if (verbose == 2) {
    if (numBytes > 1024)
      printf("%c  write %1.1fkB (0x%04x to 0x%04x) ... ok   \n", '\r', (float) idx2/1024.0, (int) addrStart, (int) (addrStart+numBytes));
    else
      printf("%c  write %dB (0x%04x to 0x%04x) ... ok   \n", '\r', idx2, (int) addrStart, (int) (addrStart+numBytes));
  }
  else if (verbose == 1) {
    if (numBytes > 1024)
      printf("%c  write %1.1fkB ... ok   \n", '\r', (float) idx2/1024.0);
    else
      printf("%c  write %dB ... ok   \n", '\r', idx2);
  }
  else if (verbose == 0)
    printf(" ok\n");
  fflush(stdout);
  
  // avoid compiler warnings
  return(0);
  
} // bsl_memWrite



/**
  \fn uint8_t bsl_jumpTo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addr)
   
  \brief jump to flash or RAM
   
  \param[in] ptrPort        handle to communication port
  \param[in] physInterface  bootloader interface: 0=UART (default), 1=SPI
  \param[in] uartMode       UART bootloader mode: 0=duplex, 1=1-wire reply, 2=2-wire reply
  \param[in] addr           address to jump to
  
  \return communication status (0=ok, 1=fail)
  
  jump to address and continue code execution. Generally RAM or flash
  starting address
*/
uint8_t bsl_jumpTo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t addr) {

  int       i;
  int       lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];


  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': port not open, exit!\n\n");
    Exit(1, g_pauseOnExit);
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
  if (physInterface == 0)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else if (physInterface == 2)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': sending command failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    Exit(1, g_pauseOnExit);
  }
    
  // receive response
  if (physInterface == 0)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else if (physInterface == 2)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': ACK1 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    Exit(1, g_pauseOnExit);
  }
  
  // check acknowledge
  if (Rx[0]!=ACK) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': ACK1 failure, exit!\n\n");
    Exit(1, g_pauseOnExit);
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
  if (physInterface == 0)
    len = send_port(ptrPort, uartMode, lenTx, Tx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = send_spi_spidev(ptrPort, lenTx, Tx);
  #endif
  else if (physInterface == 2)
    len = send_spi_Arduino(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': sending address failed (expect %d, sent %d), exit!\n\n", lenTx, len);
    Exit(1, g_pauseOnExit);
  }
  
  // receive response
  if (physInterface == 0)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  #if defined(USE_SPIDEV)
    else if (physInterface == 1)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
  #endif
  else if (physInterface == 2)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': ACK2 timeout (expect %d, received %d), exit!\n\n", lenRx, len);
    Exit(1, g_pauseOnExit);
  }
  
  // check acknowledge
  if (Rx[0]!=ACK) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': ACK2 failure, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

    
  // avoid compiler warnings
  return(0);
  
} // bsl_jumpTo


// end of file
