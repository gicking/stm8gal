/**
  \file bootloader.c

  \author G. Icking-Konert

  \brief implementation of STM8 bootloader routines

  implementation of STM8 bootloader routines
*/


#include "bootloader.h"
#include "main.h"
#include "hexfile.h"
#include "serial_comm.h"
#include "spi_spidev_comm.h"
#include "spi_Arduino_comm.h"
#include "misc.h"
#if defined(__ARMEL__) && defined(USE_WIRING)
  #include <wiringPi.h>
#endif

// device dependent flash w/e routines (from https://github.com/basilhussain/stm8-bootloader-erase-write)
#include "erase_write_verL_8k_1.0_inc.h"
#include "erase_write_ver_32k_1.0_inc.h"
#include "erase_write_ver_32k_1.2_inc.h"
#include "erase_write_ver_32k_1.3_inc.h"
#include "erase_write_ver_128k_2.0_inc.h"
#include "erase_write_ver_128k_2.1_inc.h"
#include "erase_write_ver_128k_2.2_inc.h"


/**
  \fn uint8_t bsl_sync(HANDLE ptrPort, uint8_t physInterface, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return synchronization status (0=ok, 1=fail)

  synchronize with microcontroller bootloader. For UART synchronize baudrate.
*/
uint8_t bsl_sync(HANDLE ptrPort, uint8_t physInterface, uint8_t verbose)
{
  int   i, count;
  int   lenTx, lenRx, len = 0;
  char  Tx[1000], Rx[1000];

  // print message
  if (verbose >= SILENT)
    printf("  synchronize ... ");
  fflush(stdout);

  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort)
    Error("in 'bsl_sync()': port not open");

  // purge UART input buffer and set low receive timeout
  if (physInterface == UART)
  {
    flush_port(ptrPort);
    set_timeout(ptrPort, 100);
  }

  // construct SYNC command. Note: SYNC has even parity -> works in all UART modes
  lenTx = 1;
  Tx[0] = SYNCH;
  lenRx = 1;

  // synchronize. Repeat until NACK received (problems re-synchronizing with ACK -> wait for NACK)
  count = 0;
  do
  {
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
      Error("in 'bsl_sync()': sending command failed (expect %d, sent %d)", (int) lenTx, (int) len);

    // receive response
    if (physInterface == UART)
    {
      len = receive_port(ptrPort, 0, lenRx, Rx);
      if ((len==1) && (Rx[0]== Tx[0]))
      {              // check for 1-wire echo
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
  if ((len==lenRx) && (Rx[0]==ACK))
  {
    if (verbose == SILENT)
      printf("done\n");
    else if (verbose > SILENT)
      printf("done (ACK)\n");
  }
  else if ((len==lenRx) && (Rx[0]==NACK))
  {
    if (verbose == SILENT)
      printf("done\n");
    else if (verbose > SILENT)
      printf("done (NACK)\n");
  }
  else if (len==lenRx)
    Error("in 'bsl_sync()': wrong response 0x%02" PRIX8 " from BSL", (uint8_t) (Rx[0]));
  else
    Error("in 'bsl_sync()': no response from BSL");
  fflush(stdout);

  // purge PC input buffer
  flush_port(ptrPort);

  // restore receive timeout
  if (physInterface == UART)
    set_timeout(ptrPort, TIMEOUT);

  // return success
  return 0;

} // bsl_sync



/**
  \fn uint8_t bsl_getUartMode(HANDLE ptrPort, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return UART mode (0=duplex, 1=1-wire, 2=2-wire reply)

  auto-detect UART bootloader mode (see AppNote UM0560). This information is required
  to set the correct data parity and determine if local echo is required.
*/
uint8_t bsl_getUartMode(HANDLE ptrPort, uint8_t verbose)
{
  int   len, lenTx, lenRx;
  char  Tx[1000], Rx[1000];
  uint8_t uartMode = 255;

  // print message
  if (verbose == CHATTY)
    printf("  check UART mode ... ");
  fflush(stdout);

  // reduce timeout for faster check
  set_timeout(ptrPort, 100);

  // detect UART mode
  set_parity(ptrPort, 2);
  lenTx = 2; lenRx = 1;
  Tx[0] = 0x00; Tx[1] = (Tx[0] ^ 0xFF); Rx[0] = 0x00;
  do
  {
    len = send_port(ptrPort, 0, lenTx, Tx);
    len = receive_port(ptrPort, 0, lenRx, Rx);
    //printf("\nmode 1: %d  0x%02" PRIX8 "\n", (int) len, (uint8_t) (Rx[0]));
    SLEEP(10);
  } while (len==0);

  // tested empirically...
  if (Rx[0] == ACK)                 // UART mode 0: 2-wire duplex, no SW reply, even parity
  {
    uartMode = 0;
    set_parity(ptrPort, 2);
  }
  else if (Rx[0] == Tx[0])          // UART mode 1: 1-wire reply, no SW reply, no parity
  {
    uartMode = 1;
    set_parity(ptrPort, 0);
  }
  else if (Rx[0] == NACK)           // UART mode 2: 2-wire reply, SW reply, no parity
  {
    uartMode = 2;
    set_parity(ptrPort, 0);
  }
  else
    Error("in 'bsl_getUartMode()': cannot determine UART mode");

  // revert timeout
  set_timeout(ptrPort, TIMEOUT);

  // purge PC input buffer
  flush_port(ptrPort);

  // print message
  if (verbose == CHATTY)
  {
    if (uartMode == 0)
      printf("done (duplex)\n");
    else if (uartMode == 1)
      printf("done (1-wire)\n");
    else
      printf("done (2-wire reply)\n");
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

  upload RAM routines required for flash write & erase. Respective RAM routines depend on device family, bootloader version and flash size.
  Used RAM routines under OSS license are from https://github.com/basilhussain/stm8-bootloader-erase-write.
  Optionally proprietary STM RAM routines are available from http://www.st.com/web/catalog/tools/FM147/CL1794/SC1807/SS1754/PF258008
*/
uint8_t bsl_uploadWriteErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, int flashsize, uint8_t versBSL, uint8_t family, uint8_t verbose)
{
  char            *ptrRAM = NULL;                 // pointer to array with RAM routines
  int             lenRAM;                         // length of RAM array
  MemoryImage_s   image;                       // memory image for RAM routines

  // initialize memory image
  MemoryImage_init(&image);

  // STM8L >8kB does not need to upload RAM routines -> Skip
  if ((family == STM8L) && (flashsize>8))
    return 0;

  // for STM8S and STM8L 8kB identify device dependent RAM routines
  if ((flashsize==8) && (versBSL==0x10))
  {
    #ifdef DEBUG
      printf("header STM8_Routines_E_W_ROUTINEs_8K_verL_1_0_s19 \n");
    #endif
    ptrRAM = (char*) bin_erase_write_verL_8k_1_0_ihx;
    lenRAM = bin_erase_write_verL_8k_1_0_ihx_len;
  }
  else if ((flashsize==32) && (versBSL==0x10))
  {
    #ifdef DEBUG
      printf("header STM8_Routines_E_W_ROUTINEs_32K_ver_1_0_s19 \n");
    #endif
    ptrRAM = (char*) bin_erase_write_ver_32k_1_0_ihx;
    lenRAM = bin_erase_write_ver_32k_1_0_ihx_len;
  }
  else if ((flashsize==32) && (versBSL==0x12))
  {
    #ifdef DEBUG
      printf("header STM8_Routines_E_W_ROUTINEs_32K_ver_1_2_s19 \n");
    #endif
    ptrRAM = (char*) bin_erase_write_ver_32k_1_2_ihx;
    lenRAM = bin_erase_write_ver_32k_1_2_ihx_len;
  }
  else if ((flashsize==32) && (versBSL==0x13))
  {
    #ifdef DEBUG
      printf("header STM8_Routines_E_W_ROUTINEs_32K_ver_1_3_s19 \n");
    #endif
    ptrRAM = (char*) bin_erase_write_ver_32k_1_3_ihx;
    lenRAM = bin_erase_write_ver_32k_1_3_ihx_len;
  }
  else if (((flashsize==64) || (flashsize==128)) && (versBSL==0x20))
  {
    #ifdef DEBUG
      printf("header STM8_Routines_E_W_ROUTINEs_128K_ver_2_0_s19 \n");
    #endif
    ptrRAM = (char*) bin_erase_write_ver_128k_2_0_ihx;
    lenRAM = bin_erase_write_ver_128k_2_0_ihx_len;
  }
  else if (((flashsize==64) || (flashsize==128)) && (versBSL==0x21))
  {
    #ifdef DEBUG
      printf("header STM8_Routines_E_W_ROUTINEs_128K_ver_2_1_s19 \n");
    #endif
    ptrRAM = (char*) bin_erase_write_ver_128k_2_1_ihx;
    lenRAM = bin_erase_write_ver_128k_2_1_ihx_len;
  }
  else if (((flashsize==64) || (flashsize==128)) && (versBSL==0x22))
  {
    #ifdef DEBUG
      printf("header STM8_Routines_E_W_ROUTINEs_128K_ver_2_2_s19 \n");
    #endif
    ptrRAM = (char*) bin_erase_write_ver_128k_2_2_ihx;
    lenRAM = bin_erase_write_ver_128k_2_2_ihx_len;
  }
  else
    Error("unsupported device");


  // convert matching RAM array containing ihx file to RAM image. Assert C-string termination
  ptrRAM[lenRAM] = '\0';
  import_buffer_ihx((uint8_t*) ptrRAM, &image, MUTE);

  // upload RAM routines to STM8
  if (verbose == CHATTY)
    printf("  upload RAM routines ... ");
  fflush(stdout);
  bsl_memWrite(ptrPort, physInterface, uartMode, &image, MUTE);
  if (verbose == CHATTY)
    printf("done (%dB in 0x%04" PRIX64 " - 0x%04" PRIX64 ")\n", (int) image.numEntries, 
      (uint64_t) image.memoryEntries[0].address, (uint64_t) image.memoryEntries[image.numEntries-1].address);
  fflush(stdout);
    
  // release memory image
  MemoryImage_free(&image);

  // return success
  return 0;

} // bsl_uploadWriteErase


/**
  \fn uint8_t bsl_getInfo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, int *flashsize, uint8_t *versBSL, uint8_t *family, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[out] flashsize      size of flashsize in kB (required for correct W/E routines)
  \param[out] versBSL        BSL version number (required for correct W/E routines)
  \param[out] family         STM8 family (STM8S=1, STM8L=2)
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (0=ok, 1=fail)

  query microcontroller type and BSL version info. This information is required
  to select correct version of flash write/erase routines
*/
uint8_t bsl_getInfo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, int *flashsize, uint8_t *versBSL, uint8_t *family, uint8_t verbose)
{
  int   i;
  int   lenTx, lenRx, len = 0;
  char  Tx[1000], Rx[1000];

  // print message
  if (verbose >= SILENT)
    printf("  get device info ... ");
  fflush(stdout);

  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort)
    Error("in 'bsl_getInfo()': port not open");

  // purge input buffer
  flush_port(ptrPort);


  /////////
  // determine device flash size for selecting w/e routines (flash starts at PFLASH_START)
  /////////

  // reduce timeout for faster check
  if (physInterface == UART)
  {
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
  else if (bsl_memCheck(ptrPort, physInterface, uartMode, 0x017FFF, SILENT)) // high density (64kB)
    *flashsize = 64;
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
  if (physInterface == UART)
  {
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
    Error("in 'bsl_getInfo()': sending command failed (expect %d, sent %d)", (int) lenTx, (int) len);

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
    Error("in 'bsl_getInfo()': ACK timeout (expect %d, received %d)", (int) lenRx, (int) len);

  // check 2x ACKs
  if (Rx[0]!=ACK)
    Error("in 'bsl_getInfo()': start ACK failure (expect 0x%02" PRIX8 ", read 0x%02" PRIX8 ")", (uint8_t) ACK, (uint8_t) (Rx[0]));
  if (Rx[8]!=ACK)
    Error("in 'bsl_getInfo()': end ACK failure (expect 0x%02" PRIX8 ", read 0x%02" PRIX8 ")", (uint8_t) ACK, (uint8_t) (Rx[8]));


  // check if command codes are correct (just to be sure)
  if (Rx[3] != GET)
    Error("in 'bsl_getInfo()': wrong GET code (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint8_t) GET, (uint8_t) (Rx[3]));
  if (Rx[4] != READ)
    Error("in 'bsl_getInfo()': wrong READ code (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint8_t) READ, (uint8_t) (Rx[4]));
  if (Rx[5] != GO)
    Error("in 'bsl_getInfo()': wrong GO code (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint8_t) GO, (uint8_t) (Rx[5]));
  if (Rx[6] != WRITE)
    Error("in 'bsl_getInfo()': wrong WRITE code (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint8_t) WRITE, (uint8_t) (Rx[6]));
  if (Rx[7] != ERASE)
    Error("in 'bsl_getInfo()': wrong ERASE code (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint8_t) ERASE, (uint8_t) (Rx[7]));

// print BSL data
#ifdef DEBUG
  printf("    version 0x%02" PRIX8 "\n", (uint8_t) (Rx[2]));
  printf("    command codes:\n");
  printf("      GET   0x%02" PRIX8 "\n", (uint8_t) (Rx[3]));
  printf("      READ  0x%02" PRIX8 "\n", (uint8_t) (Rx[4]));
  printf("      GO    0x%02" PRIX8 "\n", (uint8_t) (Rx[5]));
  printf("      WRITE 0x%02" PRIX8 "\n", (uint8_t) (Rx[6]));
  printf("      ERASE 0x%02" PRIX8 "\n", (uint8_t) (Rx[7]));
  fflush(stdout);
#endif

  // copy version number
  *versBSL = Rx[2];

  // print message
  if (*family == STM8S)
  {
    if (verbose == SILENT)
      printf("done (STM8S; %dkB)\n", (int) (*flashsize));
    else if (verbose == INFORM)
      printf("done (STM8S; %dkB flash)\n", (int) (*flashsize));
    else if (verbose == CHATTY)
      printf("done (STM8S; %dkB flash; BSL v%x.%x)\n", (int) (*flashsize), (uint8_t) (((*versBSL)&0xF0)>>4), (uint8_t) ((*versBSL) & 0x0F));
  }
  else
  {
    if (verbose == SILENT)
      printf("done (STM8L; %dkB)\n", (int) (*flashsize));
    else if (verbose == INFORM)
      printf("done (STM8L; %dkB flash)\n", (int) (*flashsize));
    else if (verbose == CHATTY)
      printf("done (STM8L; %dkB flash; BSL v%x.%x)\n", (int) (*flashsize), (uint8_t) (((*versBSL)&0xF0)>>4), (uint8_t) ((*versBSL) & 0x0F));
  }
  fflush(stdout);

  // avoid compiler warnings
  return 0;

} // bsl_getInfo



/**
  \fn uint8_t bsl_memRead(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, MEMIMAGE_ADDR_T addrStart, MEMIMAGE_ADDR_T addrStop, MemoryImage_s *image, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  addrStart      first address to read
  \param[in]  addrStop       last address to read
  \param[out] imageBuf       read memory image
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (0=ok, 1=fail)

  read from microcontroller memory via READ command.
*/
uint8_t bsl_memRead(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, MEMIMAGE_ADDR_T addrStart, MEMIMAGE_ADDR_T addrStop, MemoryImage_s *image, uint8_t verbose)
{
  int               i, lenTx, lenRx, len = 0;
  char              Tx[1000], Rx[1000];
  MEMIMAGE_ADDR_T   addr, addrStep;
  int               numBytes, countBytes;

  // simple checks of scan window
  if (addrStart > addrStop)
    Error("start address 0x%04" PRIX64 " higher than end address 0x%04" PRIX64, (uint64_t) addrStart, (uint64_t) addrStop);
  
  // get number of bytes to read
  numBytes = addrStop - addrStart + 1;

  // print message
  if (verbose == SILENT)
  {
    if (numBytes > 1024)
      printf("  read %1.1fkB ", (float) numBytes/1024.0);
    else
      printf("  read %dB ", (int) numBytes);
  }
  else if (verbose == INFORM)
  {
    if (numBytes > 1024)
      printf("  read %1.1fkB ", (float) numBytes/1024.0);
    else
      printf("  read %dB ", (int) numBytes);
  }
  else if (verbose == CHATTY)
  {
    if (numBytes > 1024)
      printf("  read %1.1fkB in 0x%04" PRIX64 " to 0x%04" PRIX64 " ", (float) numBytes/1024.0, (uint64_t) addrStart, (uint64_t) addrStop);
    else
      printf("  read %dB in 0x%04" PRIX64 " to 0x%04" PRIX64 " ", (int) numBytes, (uint64_t) addrStart, (uint64_t) addrStop);
  }
  fflush(stdout);

  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort)
    Error("in 'bsl_memRead()': port not open");


  // loop over addresses in 128B steps (required by SPI via Arduino)
  countBytes = 0;
  addrStep = 128;
  for (addr=addrStart; addr<=addrStop; addr+=addrStep)
  {
    // if addr too close to end of range reduce stepsize
    if (addr+128 > addrStop)
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
    if (physInterface == UART)
      len = send_port(ptrPort, uartMode, lenTx, Tx);
    else if (physInterface == SPI_ARDUINO)
      len = send_spi_Arduino(ptrPort, lenTx, Tx);
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
        len = send_spi_spidev(ptrPort, lenTx, Tx);
    #endif
    if (len != lenTx)
      Error("in 'bsl_memRead()': at 0x%04" PRIX64 " sending command failed (expect %d, sent %d)", (uint64_t) addr, (int) lenTx, (int) len);

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
      Error("in 'bsl_memRead()': at at 0x%04" PRIX64 " ACK1 timeout", (uint64_t) addr);

    // check acknowledge
    if (Rx[0]!=ACK)
      Error("in 'bsl_memRead()': at at 0x%04" PRIX64 " ACK1 failure (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint64_t) addr, (uint8_t) ACK, (uint8_t) (Rx[0]));


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
      Error("in 'bsl_memRead()': at 0x%04" PRIX64 " sending address failed (expect %d, sent %d)", (uint64_t) addr, (int) lenTx, (int) len);

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
      Error("in 'bsl_memRead()': at 0x%04" PRIX64 " ACK2 timeout (expect %d, received %d)", (uint64_t) addr, (int) lenRx, (int) len);

    // check acknowledge
    if (Rx[0]!=ACK)
      Error("in 'bsl_memRead()': at 0x%04" PRIX64 " ACK2 failure (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint64_t) addr, (uint8_t) ACK, (uint8_t) (Rx[0]));


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
      Error("in 'bsl_memRead()': at 0x%04" PRIX64 " sending range failed (expect %d, sent %d)", (uint64_t) addr, (int) lenTx, (int) len);


    // receive response
    if (physInterface == UART)
      len = receive_port(ptrPort, uartMode, lenRx, Rx);
    else if (physInterface == SPI_ARDUINO)
    {
      len = receive_spi_Arduino(ptrPort, lenRx, Rx);
      //fprintf(stderr, "%d\n", (int) lenRx);
    }
    #if defined(USE_SPIDEV)
      else if (physInterface == SPI_SPIDEV)
      {
        len = receive_spi_spidev(ptrPort, lenRx, Rx);
        //fprintf(stderr, "%d\n", (int) lenRx);
      }
    #endif
    if (len != lenRx)
      Error("in 'bsl_memRead()': at 0x%04" PRIX64 " data timeout (expect %d, received %d)", (uint64_t) addr, (int) lenRx, (int) len);

    // check acknowledge
    if (Rx[0]!=ACK)
      Error("in 'bsl_memRead()': at 0x%04" PRIX64 " ACK3 failure (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint64_t) addr, (uint8_t) ACK, (uint8_t) (Rx[0]));

    // copy data to buffer
    for (i=1; i<lenRx; i++)
    {
      assert(MemoryImage_addData(image, addr+i-1, Rx[i]));
      //printf("%d 0x%02" PRIX8 "\n", i, (uint8_t) (Rx[i])); fflush(stdout); getchar();
      countBytes++;
    }

    // print progress
    if ((countBytes % 1024) == 0)
    {
      if (verbose == SILENT)
      {
        printf(".");
        if ((countBytes % (10*1024)) == 0)
          printf(" ");
      }
      else if (verbose == INFORM)
      {
        if (numBytes > 1024)
          printf("%c  read %1.1fkB / %1.1fkB ", '\r', (float) countBytes/1024.0, (float) numBytes/1024.0);
        else
          printf("%c  read %dB / %dB ", '\r', (int) countBytes, (int) numBytes);
      }
      else if (verbose == CHATTY)
      {
        if (numBytes > 1024)
          printf("%c  read %1.1fkB / %1.1fkB from 0x%" PRIX64 " to 0x%" PRIX64 " ", '\r', (float) countBytes/1024.0, (float) numBytes/1024.0, (uint64_t) addrStart, (uint64_t) addrStop);
        else
          printf("%c  read %dB / %dB from 0x%" PRIX64 " to 0x%" PRIX64 " ", '\r', (int) countBytes, (int) numBytes, (uint64_t) addrStart, (uint64_t) addrStop);
      }
      fflush(stdout);
    }

  } // loop over address range


  // print message
  if (verbose == SILENT)
    printf(" done\n");
  else if (verbose == INFORM)
  {
    if (numBytes > 1024)
      printf("%c  read %1.1fkB / %1.1fkB ... done   \n", '\r', (float) countBytes/1024.0, (float) numBytes/1024.0);
    else
      printf("%c  read %dB / %dB ... done   \n", '\r', (int) countBytes, (int) numBytes);
  }
  else if (verbose == CHATTY)
  {
    if (numBytes > 1024)
      printf("%c  read %1.1fkB / %1.1fkB from 0x%" PRIX64 " to 0x%" PRIX64 " ... done   \n", '\r', (float) countBytes/1024.0, (float) numBytes/1024.0, (uint64_t) addrStart, (uint64_t) addrStop);
    else
      printf("%c  read %dB / %dB from 0x%" PRIX64 " to 0x%" PRIX64 " ... done   \n", '\r', (int) countBytes, (int) numBytes, (uint64_t) addrStart, (uint64_t) addrStop);
  }
  fflush(stdout);

  // avoid compiler warnings
  return 0;

} // bsl_memRead



/**
  \fn uint8_t bsl_memCheck(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, MEMIMAGE_ADDR_T addr, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  addr           address to check
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (0=ok, 1=fail)

  check if microcontrolles address exists. Specifically read 1B from microcontroller
  memory via READ command. If it fails, memory doesn't exist. Used to get STM8 type
*/
uint8_t bsl_memCheck(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, MEMIMAGE_ADDR_T addr, uint8_t verbose)
{
  int       i, lenTx, lenRx, len = 0;
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
    Error("in 'bsl_memCheck()': at 0x%04" PRIX64 " sending command failed (expect %d, sent %d)", (uint64_t) addr, (int) lenTx, (int) len);

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
    Error("in 'bsl_memCheck()': at 0x%04" PRIX64 " ACK1 timeout (expect %d, received %d)", (uint64_t) addr, (int) lenRx, (int) len);

  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_memCheck()': at 0x%04" PRIX64 " ACK1 failure (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint64_t) addr, (uint8_t) ACK, (uint8_t) (Rx[0]));


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
    Error("in 'bsl_memCheck()': at 0x%04" PRIX64 " sending address failed (expect %d, sent %d)", (uint64_t) addr, (int) lenTx, (int) len);

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
    Error("in 'bsl_memCheck()': at 0x%04" PRIX64 " ACK2 timeout (expect %d, received %d)", (uint64_t) addr, (int) lenRx, (int) len);

  // check acknowledge -> on NACK memory cannot be read -> return 0
  if (Rx[0]!=ACK)
  {
    return 0;
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
    Error("in 'bsl_memCheck()': at 0x%04" PRIX64 " sending range failed (expect %d, sent %d)", (uint64_t) addr, (int) lenTx, (int) len);

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
    Error("in 'bsl_memCheck()': at 0x%04" PRIX64 " data timeout (expect %d, received %d)", (uint64_t) addr, (int) lenRx, (int) len);

  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_memCheck()': at 0x%04" PRIX64 " ACK3 failure (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint64_t) addr, (uint8_t) ACK, (uint8_t) (Rx[0]));

  // memory read succeeded -> memory exists
  return(1);

} // bsl_memCheck



/**
  \fn uint8_t bsl_flashSectorErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, MEMIMAGE_ADDR_T addr, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  addr           adress within 1kB sector to erase
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (0=ok, 1=fail)

  sector erase for microcontroller flash. Use with care!
*/
uint8_t bsl_flashSectorErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, MEMIMAGE_ADDR_T addr, uint8_t verbose)
{
  int       i;
  int       lenTx, lenRx, len = 0;
  char      Tx[1000], Rx[1000];
  uint8_t   sector;
  uint64_t  tStart, tStop;        // measure time [ms] for erase (for COMM timeout)

  // calculate sector code
  sector = (addr - PFLASH_START)/PFLASH_BLOCKSIZE;

  // print message
  if (verbose == SILENT)
  {
    printf("  erase sector ... ");
  }
  else if (verbose == INFORM)
  {
    printf("  erase flash sector %d ... ", (int) sector);
  }
  else if (verbose == CHATTY)
  {
    if (addr>0xFFFFFF)
      printf("  erase flash sector %d at 0x%" PRIX64 " ... ", (int) sector, (uint64_t) addr);
    else if (addr>0xFFFF)
      printf("  erase flash sector %d at 0x%" PRIX64 " ... ", (int) sector, (uint64_t) addr);
    else
      printf("  erase flash sector %d at 0x%" PRIX64 " ... ", (int) sector, (uint64_t) addr);
  }
  fflush(stdout);



  // print message
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
    Error("in 'bsl_flashSectorErase()': sending command failed (expect %d, sent %d)", (int) lenTx, (int) len);


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
    Error("in 'bsl_flashSectorErase()': ACK1 timeout (expect %d, received %d)", (int) lenRx, (int) len);

  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_flashSectorErase()': ACK1 failure (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint8_t) ACK, (uint8_t) (Rx[0]));


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

  // debug: construct pattern for sector N & (N+1)
  /*
  lenTx = 4;
  Tx[0] = 0x01;      // number of sectors to erase -1 (here 2 sectors)
  Tx[1] = sector;
  Tx[2] = sector+1;
  Tx[3] = (Tx[0] ^ Tx[1] ^ Tx[2]);
  lenRx = 1;
  */

  // measure time for sector erase
  tStart = millis();

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
    Error("in 'bsl_flashSectorErase()': sending sector failed (expect %d, sent %d)", (int) lenTx, (int) len);


  // receive response
  if (physInterface == UART)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  else if (physInterface == SPI_ARDUINO)
  {
    SLEEP(40);                              // wait >30ms*(N=0+1) for sector erase before requesting response (see UM0560, SPI timing)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  }
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
    {
      SLEEP(40);                              // wait >30ms*(N=0+1) for sector erase before requesting response (see UM0560, SPI timing)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
    }
  #endif
  if (len != lenRx)
    Error("in 'bsl_flashSectorErase()': ACK2 timeout (expect %d, received %d)", (int) lenTx, (int) len);

  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_flashSectorErase()': ACK2 failure (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint8_t) ACK, (uint8_t) (Rx[0]));

  // measure time for sector erase
  tStop = millis();

  // restore timeout
  set_timeout(ptrPort, TIMEOUT);


  // print message
  if (verbose == SILENT)
  {
    printf("done\n");
  }
  else if ((verbose == INFORM) || (verbose == CHATTY))
  {
    printf("done, time %dms\n", (int)(tStop-tStart));
  }
  fflush(stdout);

  // avoid compiler warnings
  return 0;

} // bsl_flashSectorErase



/**
  \fn uint8_t bsl_flashMassErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (0=ok, 1=fail)

  mass erase microcontroller P-flash and D-flash/EEPROM. Use with care!
*/
uint8_t bsl_flashMassErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint8_t verbose)
{
  int       i, lenTx, lenRx, len = 0;
  char      Tx[1000], Rx[1000];
  uint64_t  tStart, tStop;        // measure time [ms] for erase (for COMM timeout)

  // print message
  if (verbose == SILENT)
  {
    printf("  mass erase ... ");
  }
  else if ((verbose == INFORM) || (verbose == CHATTY))
  {
    printf("  flash mass erase ... ");
  }
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
    Error("in 'bsl_flashMassErase()': sending command failed (expect %d, sent %d)", (int) lenTx, (int) len);

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
    Error("in 'bsl_flashMassErase()': ACK1 timeout (expect %d, received %d)", (int) lenRx, (int) len);

  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_flashMassErase()': ACK1 failure (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint8_t) ACK, (uint8_t) (Rx[0]));


  /////
  // send 0xFF+0x00 to trigger mass erase
  /////

  // increase timeout for long erase. Measured 3.3s for 128kB STM8 -> set to 5s
  set_timeout(ptrPort, 10000);

  // construct pattern
  lenTx = 2;
  Tx[0] = 0xFF;
  Tx[1] = 0x00;
  lenRx = 1;

  // measure time for mass erase
  tStart = millis();

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
    Error("in 'bsl_flashMassErase()': sending trigger failed (expect %d, sent %d)", (int) lenTx, (int) len);


  // receive response
  if (physInterface == UART)
    len = receive_port(ptrPort, uartMode, lenRx, Rx);
  else if (physInterface == SPI_ARDUINO)
  {
    SLEEP(1100);                              // wait >30ms*(N=32+1) for sector erase before requesting response (see UM0560, SPI timing)
    len = receive_spi_Arduino(ptrPort, lenRx, Rx);
  }
  #if defined(USE_SPIDEV)
    else if (physInterface == SPI_SPIDEV)
    {
      SLEEP(1100);                              // wait >30ms*(N=32+1) for sector erase before requesting response (see UM0560, SPI timing)
      len = receive_spi_spidev(ptrPort, lenRx, Rx);
    }
  #endif
  if (len != lenRx)
    Error("in 'bsl_flashMassErase()': ACK2 timeout (expect %d, received %d)", (int) lenRx, (int) len);

  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_flashMassErase()': ACK2 failure (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint8_t) ACK, (uint8_t) (Rx[0]));

  // measure time for mass erase
  tStop = millis();

  // restore timeout
  set_timeout(ptrPort, TIMEOUT);


  // print message
  if (verbose == SILENT)
  {
    printf("done\n");
  }
  else if ((verbose == INFORM) || (verbose == CHATTY))
  {
    printf("done, time %1.1fs\n", (float)(tStop-tStart)/1000.0);
  }
  fflush(stdout);

  // avoid compiler warnings
  return 0;

} // bsl_flashMassErase



/**
  \fn uint8_t bsl_memWrite(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint16_t *imageBuf, MEMIMAGE_ADDR_T addrStart, MEMIMAGE_ADDR_T addrStop, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[out] imageBuf       memory image of data to write
  \param[in]  addrStart      first address to write to
  \param[in]  addrStop       last address to write to
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (0=ok, 1=fail)

  upload data to microcontroller memory via WRITE command
*/
uint8_t bsl_memWrite(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, const MemoryImage_s *image, uint8_t verbose)
{
  int               countBytes, countPage;              // size of memory image
  const int         maxPage = 128;                      // max. length of write (aka page)
  MEMIMAGE_ADDR_T   addrBlock, addrPage, addrStart, addrEnd;
  size_t            idxStart, idxEnd;
  char              Tx[1000], Rx[1000];                 // communication buffers
  int               lenTx, lenRx, len = 0;              // frame lengths
  uint8_t           chk;                                // frame checksum
  int               i, j;


  // print message
  if (verbose == SILENT)
  {
    if (image->numEntries > 1024)
      printf("  write %1.1fkB ", (float) image->numEntries/1024.0);
    else if (image->numEntries > 0)
      printf("  write %dB ", (int) image->numEntries);
    else {
      printf("  no data to write\n" );
      return 0;
    }
  }
  else if (verbose == INFORM)
  {
    if (image->numEntries > 1024)
      printf("  write %1.1fkB ", (float) image->numEntries/1024.0);
    else if (image->numEntries > 0)
      printf("  write %dB ", (int) image->numEntries);
    else {
      printf("  no data to write\n" );
      return 0;
    }
  }
  else if (verbose == CHATTY)
  {
    if (image->numEntries > 1024)
      printf("  write %1.1fkB in 0x%04" PRIX64 " to 0x%04" PRIX64 " ", (float) image->numEntries/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 0)
      printf("  write %dB in 0x%04" PRIX64 " to 0x%04" PRIX64 " ", (int) image->numEntries, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else {
      printf("  no data to write\n" );
      return 0;
    }
  }
  fflush(stdout);

  // init receive buffer
  for (i=0; i<1000; i++)
    Rx[i] = 0;

  // check if port is open
  if (!ptrPort)
    Error("in 'bsl_memWrite()': port not open");


  // loop over consecutive memory blocks in image
  addrBlock = 0x00;
  countBytes = 0;
  countPage = 0;
  while (MemoryImage_getMemoryBlock(image, addrBlock, &idxStart, &idxEnd)) {

    addrStart = image->memoryEntries[idxStart].address;
    addrEnd   = image->memoryEntries[idxEnd].address;
    //printf("addrStart: 0x%x - addrEnd 0x%x\n", addrStart, addrEnd);

    // loop over memory block and upload in pages of max. 128B
    addrPage = addrStart;
    while (addrPage <= addrEnd) {
        
      // get length of next page to upload (max. 128B). Align with 128 for programming speed (see UM0560 section 3.4)
      int lenPage = 1;
      while ((lenPage < maxPage) && ((addrPage+lenPage) <= addrEnd) && ((addrPage+lenPage) % maxPage))
        lenPage++;
      //printf("0x%04" PRIX64 "  %d\n", (uint64_t) addrPage, lenPage);

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
        Error("in 'bsl_memWrite()': at 0x%04" PRIX64 " sending command failed (expect %d, sent %d)", (uint64_t) addrPage, (int) lenTx, (int) len);

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
        Error("in 'bsl_memWrite()': at 0x%04" PRIX64 " ACK1 timeout (expect %d, received %d)", (uint64_t) addrPage, (int) lenRx, (int) len);

      // check acknowledge
      if (Rx[0]!=ACK)
        Error("in 'bsl_memWrite()': at 0x%04" PRIX64 " ACK1 failure (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint64_t) addrPage, (uint8_t) ACK, (uint8_t) (Rx[0]));


      /////
      // send address
      /////

      // construct address + checksum (XOR over address)
      lenTx = 5;
      Tx[0] = (char) (addrPage >> 24);
      Tx[1] = (char) (addrPage >> 16);
      Tx[2] = (char) (addrPage >> 8);
      Tx[3] = (char) (addrPage);
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
        Error("in 'bsl_memWrite()': at 0x%04" PRIX64 " sending address failed (expect %d, sent %d)", (uint64_t) addrPage, (int) lenTx, (int) len);


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
        Error("in 'bsl_memWrite()': at 0x%04" PRIX64 " ACK2 timeout (expect %d, received %d)", (uint64_t) addrPage, (int) lenRx, (int) len);

      // check acknowledge
      if (Rx[0]!=ACK)
        Error("in 'bsl_memWrite()': at 0x%04" PRIX64 " ACK2 failure (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint64_t) addrPage, (uint8_t) ACK, (uint8_t) (Rx[0]));


      /////
      // send number of bytes and data
      /////

      // construct number of bytes + data + checksum
      lenTx = 0;
      Tx[lenTx++] = lenPage-1;     // -1 from BSL
      chk         = lenPage-1;
      for (j=0; j<lenPage; j++)
      {
        MemoryImage_getData(image, addrPage+j, (uint8_t*) (Tx+lenTx));
        chk ^= Tx[lenTx];
        lenTx++;
        countBytes++;
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
        Error("in 'bsl_memWrite()': at 0x%04" PRIX64 " sending data failed (expect %d, sent %d)", (uint64_t) addrPage, (int) lenTx, (int) len);


      // receive response
      if (physInterface == UART)
        len = receive_port(ptrPort, uartMode, lenRx, Rx);
      else if (physInterface == SPI_ARDUINO)
      {
        if ((addrBlock >= PFLASH_START) && (addrBlock % 128))  // wait for flash write finished before requesting response (see UM0560, SPI timing)
          SLEEP(1200);                               // for not 128-aligned data wait >1.1s
        else
          SLEEP(20);                                 // for 128-aligned data wait >8.5ms
        len = receive_spi_Arduino(ptrPort, lenRx, Rx);
      }
      #if defined(USE_SPIDEV)
        else if (physInterface == SPI_SPIDEV)
        {
          if ((addrBlock >= PFLASH_START) && (addrBlock % 128))  // wait for flash write finished before requesting response (see UM0560, SPI timing)
            SLEEP(1200);                               // for not 128-aligned data wait >1.1s
          else
            SLEEP(20);                                 // for 128-aligned data wait >8.5ms
          len = receive_spi_spidev(ptrPort, lenRx, Rx);
        }
      #endif
      if (len != lenRx)
        Error("in 'bsl_memWrite()': at 0x%04" PRIX64 " ACK3 timeout (expect %d, received %d)", (uint64_t) addrPage, (int) lenRx, (int) len);

      // check acknowledge
      if (Rx[0]!=ACK)
        Error("in 'bsl_memWrite()': at 0x%04" PRIX64 " ACK3 failure (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint64_t) addrPage, (uint8_t) ACK, (uint8_t) (Rx[0]));

      // print progress
      if (((++countPage) % 8) == 0)
      {
        if (verbose == SILENT)
        {
          printf(".");
          if ((countPage % (10*8)) == 0)
            printf(" ");
        }
        else if (verbose == INFORM)
        {
          if (image->numEntries > 1024)
            printf("%c  write %1.1fkB / %1.1fkB ", '\r', (float) countBytes/1024.0, (float) image->numEntries/1024.0);
          else
            printf("%c  write %dB / %dB ", '\r', (int) countBytes, (int) image->numEntries);
        }
        else if (verbose == CHATTY)
        {
          if (image->numEntries > 1024)
            printf("%c  write %1.1fkB / %1.1fkB in 0x%04" PRIX64 " to 0x%04" PRIX64 " ", '\r', (float) countBytes/1024.0, (float) image->numEntries/1024.0, 
              (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
          else
            printf("%c  write %dB / %dB in 0x%04" PRIX64 " to 0x%04" PRIX64 " ", '\r', (int) countBytes, (int) image->numEntries, 
              (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
        }
        fflush(stdout);
      }

      // go to next page
      addrPage += lenPage;

    } // loop address over memory block

    // start address for searching next memory block 
    addrBlock = addrEnd + 1;

  } // loop over memory blocks in image

  // print message
  if (verbose == SILENT)
    printf(" done\n");
  else if (verbose == INFORM)
  {
    if (image->numEntries > 1024)
      printf("%c  write %1.1fkB / %1.1fkB ... done   \n", '\r', (float) countBytes/1024.0, (float) image->numEntries/1024.0);
    else
      printf("%c  write %dB / %dB ... done   \n", '\r', (int) countBytes, (int) image->numEntries);
  }
  else if (verbose == CHATTY)
  {
    if (image->numEntries > 1024)
      printf("%c  write %1.1fkB / %1.1fkB in 0x%04" PRIX64 " to 0x%04" PRIX64 " ... done   \n", '\r', (float) countBytes/1024.0, (float) image->numEntries/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else
      printf("%c  write %dB / %dB in 0x%04" PRIX64 " to 0x%04" PRIX64 " ... done   \n", '\r', (int) countBytes, (int) image->numEntries, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
  }

  // avoid compiler warnings
  return 0;

} // bsl_memWrite



/**
  \fn uint8_t bsl_memVerifyRead(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, const MemoryImage_s *image, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[out] image          memory image to verify
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (0=ok, 1=fail)

  Read microntroller flash memory and compare to specified RAM image.
*/
uint8_t bsl_memVerifyRead(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, const MemoryImage_s *image, uint8_t verbose)
{
  MEMIMAGE_ADDR_T   addrBlock, addrStart, addrEnd;
  size_t            idxStart, idxEnd;

  // initialize temporary memory image for flash read 
  MemoryImage_s tmpImage;
  MemoryImage_init(&tmpImage);

  // loop over consecutive memory blocks in image
  addrBlock = 0x00;
  while (MemoryImage_getMemoryBlock(image, addrBlock, &idxStart, &idxEnd)) {

    addrStart = image->memoryEntries[idxStart].address;
    addrEnd   = image->memoryEntries[idxEnd].address;

    // read STM8 memory into temporary memory image
    bsl_memRead(ptrPort, physInterface, uartMode, addrStart, addrEnd, &tmpImage, verbose);

    // start address for searching next memory block 
    addrBlock = addrEnd + 1;

  } // loop over memory blocks in image

  // print messgage
  if (verbose != MUTE)
    printf("  verify memory ... ");
  fflush(stdout);


  // loop over memory image
  for (size_t i = 0; i < image->numEntries; i++)
  {
    // compare data entry. Address is asserted by above flash read
    if ((image->memoryEntries[i].data) != (tmpImage.memoryEntries[i].data)) {
        Error("verify failed at address 0x%04" PRIX64 " (expect 0x%02" PRIX8 ", read 0x%02" PRIX8 ")", (uint64_t) image->memoryEntries[i].address, 
          (uint8_t) image->memoryEntries[i].data, (uint8_t) tmpImage.memoryEntries[i].data);
    }
  } // loop over image

  // print messgage
  if (verbose != MUTE)
    printf("done, passed\n");
  fflush(stdout);

  // release temporary memory image
  MemoryImage_free(&tmpImage);

  // avoid compiler warnings
  return 0;

} // bsl_memVerify



/**
  \fn uint8_t bsl_jumpTo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, MEMIMAGE_ADDR_T addr, uint8_t verbose)

  \param[in]  ptrPort        handle to communication port
  \param[in]  physInterface  bootloader interface: 0=UART (default), 1=SPI via Arduino, 2=SPI via SPIDEV
  \param[in]  uartMode       UART bootloader mode: 0=duplex, 1=1-wire, 2=2-wire reply
  \param[in]  addr           address to jump to
  \param[in]  verbose        verbosity level (0=SILENT, 1=INFORM, 2=CHATTY)

  \return communication status (0=ok, 1=fail)

  jump to address and continue code execution. Generally RAM or flash starting address
*/
uint8_t bsl_jumpTo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, MEMIMAGE_ADDR_T addr, uint8_t verbose)
{
  int       i;
  int       lenTx, lenRx, len = 0;
  char      Tx[1000], Rx[1000];

  // print message
  if (verbose == INFORM)
    printf("  jump to 0x%04" PRIX64 " ... ", (uint64_t) addr);
  else if (verbose == CHATTY)
    printf("  jump to address 0x%04" PRIX64 " ... ", (uint64_t) addr);
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
    Error("in 'bsl_jumpTo()': sending command failed (expect %d, sent %d)", (int) lenTx, (int) len);

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
    Error("in 'bsl_jumpTo()': ACK1 timeout (expect %d, received %d)", (int) lenRx, (int) len);

  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_jumpTo()': ACK1 failure (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint8_t) ACK, (uint8_t) (Rx[0]));


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
    Error("in 'bsl_jumpTo()': sending address failed (expect %d, sent %d)", (int) lenTx, (int) len);

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
    Error("in 'bsl_jumpTo()': ACK2 timeout (expect %d, received %d)", (int) lenRx, (int) len);

  // check acknowledge
  if (Rx[0]!=ACK)
    Error("in 'bsl_jumpTo()': ACK2 failure (expect 0x%02" PRIX8 ", received 0x%02" PRIX8 ")", (uint8_t) ACK, (uint8_t) (Rx[0]));

  // print message
  if ((verbose == INFORM) || (verbose == CHATTY))
    printf("done\n");
  fflush(stdout);

  // avoid compiler warnings
  return 0;

} // bsl_jumpTo

// end of file
