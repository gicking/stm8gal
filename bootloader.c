/**
  \file bootloader.c
   
  \author G. Icking-Konert
  \date 2014-03-14
  \version 0.1
   
  \brief implementation of STM bootloader routines
   
  implementation of of STM bootloader routines
*/


#include "bootloader.h"
#include "serial_comm.h"
#include "misc.h"
#include "globals.h"


/**
  \fn uint8_t bsl_sync(HANDLE ptrPort)
   
  \brief synchronize to microcontroller BSL
   
  \param[in] ptrPort    handle to communication port

  \return synchronization status (0=ok, 1=fail)
  
  synchronize to microcontroller BSL, e.g. baudrate. If already synchronized
  checks for NACK
*/
uint8_t bsl_sync(HANDLE ptrPort) {
  
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
  
  
  // purge input buffer
  flush_port(ptrPort); 
  
  // construct SYNC command
  lenTx = 1;
  Tx[0] = SYNCH;
  lenRx = 1;  
  
  count = 0;
  do {
    
    // send command
    len = send_port(ptrPort, lenTx, Tx);
    if (len != lenTx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_sync()': sending command failed, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }
        
    // receive response with timeout
    len = receive_port(ptrPort, lenRx, Rx);

    // increase retry counter
    count++;
    
    // just to make sure
    SLEEP(10);
    
    //printf("test %d\n", count);
    
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
    fprintf(stderr, "\n\nerror in 'bsl_sync()': wrong response 0x%02x from BSL, exit!\n\n", Rx[0]);
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
  \fn uint8_t bsl_getInfo(HANDLE ptrPort, int *flashsize, uint8_t *vers))
   
  \brief get microcontroller type and BSL version (for correct w/e routines)
   
  \param[in]  ptrPort     handle to communication port
  \param[out] flashsize   size of flashsize in kB (required for correct W/E routines)
  \param[out] vers        BSL version number (required for correct W/E routines)
  \param[out] family      STM8 family (STM8S=1, STM8L=2)
  
  \return communication status (0=ok, 1=fail)
  
  query microcontroller type and BSL version info. This information is required
  to select correct version of flash write/erase routines
*/
uint8_t bsl_getInfo(HANDLE ptrPort, int *flashsize, uint8_t *vers, uint8_t *family) {
  
  int   i;
  int   lenTx, lenRx, len;
  char  Tx[1000], Rx[1000];

  // print message
  if (g_verbose) {
    printf("  determine device ... ");
    fflush(stdout);
  }

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
  SLEEP(50);              // required for some reason
  
  
  /////////
  // determine device flash size for selecting w/e routines (flash starts at PFLASH_START)
  /////////

  // reduce timeout for faster check
  set_timeout(ptrPort, 200);
  
  // check address of EEPROM. STM8L starts at 0x1000, STM8S starts at 0x4000
  if (bsl_memCheck(ptrPort, 0x004000))       // STM8S
  {
    *family = STM8S;
#ifdef DEBUG
    printf("family STM8S\n");
#endif
  }
  else if (bsl_memCheck(ptrPort, 0x00100))   // STM8L
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
  if (bsl_memCheck(ptrPort, 0x047FFF))       // extreme density (256kB)
    *flashsize = 256;
  else if (bsl_memCheck(ptrPort, 0x027FFF))  // high density (128kB)
    *flashsize = 128;
  else if (bsl_memCheck(ptrPort, 0x00FFFF))  // medium density (32kB)
    *flashsize = 32;
  else if (bsl_memCheck(ptrPort, 0x009FFF))  // low density (8kB)
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
  set_timeout(ptrPort, 1000);
  
  
  /////////
  // get BSL version
  /////////
  
  // construct command
  lenTx = 2;
  Tx[0] = GET;
  Tx[1] = (Tx[0] ^ 0xFF);
  lenRx = 9;
  
  // send command
  len = send_port(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': sending command failed, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }
    
  // receive response with timeout
  len = receive_port(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': ACK timeout, exit!\n\n");
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
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': wrong GET code (expect 0x%02x), exit!\n\n", GET);
    Exit(1, g_pauseOnExit);
  }
  if (Rx[4] != READ) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': wrong READ code (expect 0x%02x), exit!\n\n", READ);
    Exit(1, g_pauseOnExit);
  }
  if (Rx[5] != GO) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': wrong GO code (expect 0x%02x), exit!\n\n", GO);
    Exit(1, g_pauseOnExit);
  }
  if (Rx[6] != WRITE) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': wrong WRITE code (expect 0x%02x), exit!\n\n", WRITE);
    Exit(1, g_pauseOnExit);
  }
  if (Rx[7] != ERASE) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_getInfo()': wrong ERASE code (expect 0x%02x), exit!\n\n", ERASE);
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
  if (g_verbose) {
    if (*family == STM8S)
      printf("ok (STM8S; %dkB flash; BSL v%x.%x)\n", *flashsize, (((*vers)&0xF0)>>4), ((*vers) & 0x0F));
    else
      printf("ok (STM8L; %dkB flash; BSL v%x.%x)\n", *flashsize, (((*vers)&0xF0)>>4), ((*vers) & 0x0F));
    fflush(stdout);
  }
  
  // avoid compiler warnings
  return(0);
  
} // bsl_getInfo



/**
  \fn uint8_t bsl_memRead(HANDLE ptrPort, uint32_t addrStart, uint32_t numBytes, char *buf)
   
  \brief read from microcontroller memory
   
  \param[in] ptrPort    handle to communication port
  \param[in] addrStart  starting address to read from
  \param[in] numBytes   number of bytes to read
  \param[in] buf        buffer to store data to
  
  \return communication status (0=ok, 1=fail)
  
  read from microcontroller memory via READ command
*/
uint8_t bsl_memRead(HANDLE ptrPort, uint32_t addrStart, uint32_t numBytes, char *buf, uint8_t verbose) {

  int       i, lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];
  uint32_t  addrTmp, addrStep, idx=0;


  // print message
  if (verbose) {
    if (numBytes > 1024)
      printf("  read  %1.1fkB starting from 0x%04x ", (float) numBytes/1024.0, (int) addrStart);
    else
      printf("  read  %dB starting from 0x%04x ", numBytes, (int) addrStart);
    fflush(stdout);
  }
  
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
    len = send_port(ptrPort, lenTx, Tx);
    if (len != lenTx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': sending command failed, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }
    
    // receive response with timeout
    len = receive_port(ptrPort, lenRx, Rx);
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
    len = send_port(ptrPort, lenTx, Tx);
    if (len != lenTx) {      
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': sending address failed, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }

    // receive response with timeout
    len = receive_port(ptrPort, lenRx, Rx);
    if (len != lenRx) {      
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': ACK2 timeout, exit!\n\n");
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
    len = send_port(ptrPort, lenTx, Tx);
    if (len != lenTx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': sending range failed, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }

    // receive response with timeout
    len = receive_port(ptrPort, lenRx, Rx);
    if (len != lenRx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memRead()': data timeout, exit!\n\n");
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
      //printf("%d 0x%02x\n", i, (uint8_t) (Rx[i])); fflush(stdout);
    }
    
    // print progress
    if (verbose) {
      if ((idx % 1024) == 0) {
        if (numBytes > 1024)
          printf("%c  read  %1.1fkB starting from 0x%04x ", '\r', (float) idx/1024.0, (int) addrStart);
        else
          printf("%c  read  %dB starting from 0x%04x ", '\r', idx, (int) addrStart);
        fflush(stdout);
      }
    }

  } // loop over address range 
  
  
  // print message
  if (verbose) {
    if (numBytes > 1024)
      printf("%c  read  %1.1fkB starting from 0x%04x ... ", '\r', (float) idx/1024.0, (int) addrStart);
    else
      printf("%c  read  %dB starting from 0x%04x ... ", '\r', idx, (int) addrStart);
    printf("ok\n");
    fflush(stdout);
  }
  
  
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
  \fn uint8_t bsl_memCheck(HANDLE ptrPort, uint32_t addr)
   
  \brief check if address exists
      
  \param[in] ptrPort    handle to communication port
  \param[in] addr       address to check
  
  \return communication status (0=ok, 1=fail)
  
  check if microcontrolles address exists. Specifically read 1B from microcontroller 
  memory via READ command. If it fails, memory doesn't exist. Used to get STM8 type
*/
uint8_t bsl_memCheck(HANDLE ptrPort, uint32_t addr) {

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
  len = send_port(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': sending command failed, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }
    
  // receive response with timeout
  len = receive_port(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': ACK1 timeout, exit!\n\n");
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
  len = send_port(ptrPort, lenTx, Tx);
  if (len != lenTx) {      
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': sending address failed, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  // receive response with timeout
  len = receive_port(ptrPort, lenRx, Rx);
  if (len != lenRx) {      
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': ACK2 timeout, exit!\n\n");
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
  len = send_port(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': sending range failed, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  // receive response with timeout
  len = receive_port(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_memCheck()': data timeout, exit!\n\n");
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
  \fn uint8_t bsl_flashSectorErase(HANDLE ptrPort, uint32_t addr)
   
  \brief erase one microcontroller flash sector
  
  \param[in] ptrPort      handle to communication port
  \param[in] addr         adress within 1kB sector to erase
  
  \return communication status (0=ok, 1=fail)
  
  sector erase for microcontroller flash
*/
uint8_t bsl_flashSectorErase(HANDLE ptrPort, uint32_t addr) {

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
  len = send_port(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashSectorErase()': sending command failed, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  // receive response with timeout
  len = receive_port(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashSectorErase()': ACK1 timeout, exit!\n\n");
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
  Tx[0] = 0x00;
  Tx[1] = sector;
  Tx[2] = (Tx[0] ^ Tx[1]);
  lenRx = 1;

  // send command
  len = send_port(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashSectorErase()': sending sector failed, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  
  // wait for erase to avoid communication timeout
  //SLEEP(10000);
  
  
  // receive response with timeout
  len = receive_port(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashSectorErase()': ACK2 timeout, exit!\n\n");
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
  \fn uint8_t bsl_flashMassErase(HANDLE ptrPort)
   
  \brief mass erase microcontroller flash
  
  \param[in] ptrPort      handle to communication port
  
  \return communication status (0=ok, 1=fail)
  
  mass erase microcontroller P-flash and D-flash/EEPROM
*/
uint8_t bsl_flashMassErase(HANDLE ptrPort) {

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
  len = send_port(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': sending command failed, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  // receive response with timeout
  len = receive_port(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': ACK1 timeout, exit!\n\n");
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
  len = send_port(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': sending trigger failed, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  
  // wait for erase to avoid communication timeout
  //SLEEP(10000);
  
  // mass erase takes longer -> increase timeout
  set_timeout(ptrPort, 5000);
  
  // receive response with timeout
  len = receive_port(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': ACK2 timeout, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }
  
  // check acknowledge
  if (Rx[0]!=ACK) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_flashMassErase()': ACK2 failure, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  // restore timeout
  set_timeout(ptrPort, 1000);

    
  // print message
  printf("ok\n");
  fflush(stdout);
  
  // avoid compiler warnings
  return(0);

} // bsl_flashMassErase



/**
  \fn uint8_t bsl_memWrite(HANDLE ptrPort, uint32_t addrStart, uint32_t numBytes, char *buf)
   
  \brief upload to microcontroller flash or RAM
   
  \param[in] ptrPort    handle to communication port
  \param[in] addrStart  starting address to upload to
  \param[in] numBytes   number of bytes to upload
  \param[in] buf        buffer containing data
  \param[in] verbose    print output to console?
  
  \return communication status (0=ok, 1=fail)
  
  upload data to microcontroller memory via WRITE command
*/
uint8_t bsl_memWrite(HANDLE ptrPort, uint32_t addrStart, uint32_t numBytes, char *buf, uint8_t verbose) {

  int       i, lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];
  uint32_t  addrTmp, addrStep, idx=0, idx2=0;
  uint8_t   chk, flagEmpty;


  // print message
  if (verbose) {
    if (numBytes > 1024)
      printf("  write %1.1fkB starting from 0x%04x ", (float) idx2/1024.0, (int) addrStart);
    else
      printf("  write %dB starting from 0x%04x ", idx2, (int) addrStart);
    fflush(stdout);
  }
  
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
    len = send_port(ptrPort, lenTx, Tx);
    if (len != lenTx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': sending command failed, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }
    
    // receive response with timeout
    len = receive_port(ptrPort, lenRx, Rx);
    if (len != lenRx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK1 timeout, exit!\n\n");
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
    len = send_port(ptrPort, lenTx, Tx);
    if (len != lenTx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': sending address failed, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }
    
    // receive response with timeout
    len = receive_port(ptrPort, lenRx, Rx);
    if (len != lenRx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK2 timeout, exit!\n\n");
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
    len = send_port(ptrPort, lenTx, Tx);
    if (len != lenTx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': sending data failed, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }
    
    // receive response with timeout
    len = receive_port(ptrPort, lenRx, Rx);
    if (len != lenRx) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK3 timeout, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }
    
    // check acknowledge
    if (Rx[0]!=ACK) {
      setConsoleColor(PRM_COLOR_RED);
      fprintf(stderr, "\n\nerror in 'bsl_memWrite()': ACK3 failure, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }
    
    // print progress
    if (((idx2 % 1024) == 0) && (verbose)){
      if (numBytes > 1024)
        printf("%c  write %1.1fkB starting from 0x%04x ", '\r', (float) idx2/1024.0, (int) addrStart);
      else
        printf("%c  write %dB starting from 0x%04x ", '\r', idx2, (int) addrStart);
      fflush(stdout);
    }

  } // loop over address range 
  
  // print message
  if (verbose) {
    if (numBytes > 1024)
      printf("%c  write %1.1fkB starting from 0x%04x ... ok   \n", '\r', (float) idx2/1024.0, (int) addrStart);
    else
      printf("%c  write %dB starting from 0x%04x ... ok   \n", '\r', idx2, (int) addrStart);
    fflush(stdout);
  }
  
  // avoid compiler warnings
  return(0);
  
} // bsl_memWrite



/**
  \fn uint8_t bsl_jumpTo(HANDLE ptrPort, uint32_t addr)
   
  \brief jump to flash or RAM
   
  \param[in] ptrPort    handle to communication port
  \param[in] addr       address to jump to
  
  \return communication status (0=ok, 1=fail)
  
  jump to address and continue code execution. Generally RAM or flash
  starting address
*/
uint8_t bsl_jumpTo(HANDLE ptrPort, uint32_t addr) {

  int       i;
  int       lenTx, lenRx, len;
  char      Tx[1000], Rx[1000];


  // print message
  printf("  jump to address 0x%04x ... ", (int) addr);
  fflush(stdout);
  
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
  len = send_port(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': sending command failed, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }
    
  // receive response with timeout
  len = receive_port(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': ACK1 timeout, exit!\n\n");
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
  len = send_port(ptrPort, lenTx, Tx);
  if (len != lenTx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': sending address failed, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }
  
  // receive response with timeout
  len = receive_port(ptrPort, lenRx, Rx);
  if (len != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': ACK2 timeout, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }
  
  // check acknowledge
  if (Rx[0]!=ACK) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'bsl_jumpTo()': ACK2 failure, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

    
  // print message
  printf("ok\n");
  fflush(stdout);
  
  // avoid compiler warnings
  return(0);
  
} // bsl_jumpTo


// end of file
