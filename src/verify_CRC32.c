/**
  \file verify_CRC32.c

  \author G. Icking-Konert

  \brief implementation of verify via CRC32 routines

  implementation of routines to verify memory via comparing CRC32 checksums.
*/

// include files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "main.h"
#include "hexfile.h"
#include "bootloader.h"
#include "misc.h"
#include "verify_CRC32.h"

// include RAM routines for supported devices
#include "verify_CRC32_STM8L_8k_v1.0.h"
#include "verify_CRC32_STM8L_32k_v1.1.h"
#include "verify_CRC32_STM8L_32k_v1.2.h"
#include "verify_CRC32_STM8L_64k_v1.1.h"

#include "verify_CRC32_STM8S_32k_v1.2.h"
#include "verify_CRC32_STM8S_32k_v1.3.h"
#include "verify_CRC32_STM8S_128k_v2.1.h"
#include "verify_CRC32_STM8S_128k_v2.2.h"


uint8_t upload_crc32_code(HANDLE ptrPort, uint8_t family, int flashsize, uint8_t versBSL, uint8_t physInterface, uint8_t uartMode)
{
  char            *ptrRAM = NULL;     // pointer to array with RAM routines
  int             lenRAM;             // length of RAM array
  MemoryImage_s   image;              // memory image for RAM routines

  // initialize memory image
  MemoryImage_init(&image);

  // identify device dependent CRC32 RAM ihx file
  if ((family == STM8L) && (flashsize==8) && (versBSL==0x10))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8L_8k_v1_0_ihx\n");
    #endif
    ptrRAM = (char*) bin_verify_CRC32_STM8L_8k_v1_0_ihx;
    lenRAM = bin_verify_CRC32_STM8L_8k_v1_0_ihx_len;
  }
  else if ((family == STM8L) && ((flashsize==16) || (flashsize==32)) && (versBSL==0x11))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8L_32k_v1_1_ihx\n");
    #endif
    ptrRAM = (char*) bin_verify_CRC32_STM8L_32k_v1_1_ihx;
    lenRAM = bin_verify_CRC32_STM8L_32k_v1_1_ihx_len;
  }
  else if ((family == STM8L) && ((flashsize==16) || (flashsize==32)) && (versBSL==0x12))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8L_32k_v1_2_ihx\n");
    #endif
    ptrRAM = (char*) bin_verify_CRC32_STM8L_32k_v1_2_ihx;
    lenRAM = bin_verify_CRC32_STM8L_32k_v1_2_ihx_len;
  }
  else if ((family == STM8L) && (flashsize==64) && (versBSL==0x11))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8L_64k_v1_1_ihx\n");
    #endif
    ptrRAM = (char*) bin_verify_CRC32_STM8L_64k_v1_1_ihx;
    lenRAM = bin_verify_CRC32_STM8L_64k_v1_1_ihx_len;
  }
  else if ((family == STM8S) && (flashsize==32) && (versBSL==0x12))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8S_32k_v1_2_ihx\n");
    #endif
    ptrRAM = (char*) bin_verify_CRC32_STM8S_32k_v1_2_ihx;
    lenRAM = bin_verify_CRC32_STM8S_32k_v1_2_ihx_len;
  }
  else if ((family == STM8S) && (flashsize==32) && (versBSL==0x13))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8S_32k_v1_3_ihx\n");
    #endif
    ptrRAM = (char*) bin_verify_CRC32_STM8S_32k_v1_3_ihx;
    lenRAM = bin_verify_CRC32_STM8S_32k_v1_3_ihx_len;
  }
  else if ((family == STM8S) && ((flashsize==64) || (flashsize==128)) && (versBSL==0x21))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8S_128k_v2_1_ihx\n");
    #endif
    ptrRAM = (char*) bin_verify_CRC32_STM8S_128k_v2_1_ihx;
    lenRAM = bin_verify_CRC32_STM8S_128k_v2_1_ihx_len;
  }
  else if ((family == STM8S) && ((flashsize==64) || (flashsize==128)) && (versBSL==0x22))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8S_128k_v2_2_ihx\n");
    #endif
    ptrRAM = (char*) bin_verify_CRC32_STM8S_128k_v2_2_ihx;
    lenRAM = bin_verify_CRC32_STM8S_128k_v2_2_ihx_len;
  }
  else
  {
    Error("bootloader does not support CRC32 verify, use read-out instead (family=%d, flash=%dkB, BL v%d)", (int) family, (int) flashsize, (int) versBSL);
  }

  // convert matching RAM array containing ihx file to RAM image. Assert C-string termination
  ptrRAM[lenRAM] = '\0';
  import_buffer_ihx((uint8_t*) ptrRAM, &image, MUTE);

  // upload RAM routines to STM8
  bsl_memWrite(ptrPort, physInterface, uartMode, &image, MUTE);

  // release memory image
  MemoryImage_free(&image);

  // avoid compiler warnings
  return(0);

} // upload_crc32_code()


uint8_t upload_crc32_address(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addrStart, uint64_t addrStop)
{
  MemoryImage_s   image;

  // initialize memory image
  MemoryImage_init(&image);

  // store start addresses for CRC32 at fixed RAM address (STM8 = big endian = MSB first)
  assert(MemoryImage_addData(&image, (MEMIMAGE_ADDR_T) ADDR_START_CRC32+0, (uint8_t) (addrStart >> 24)));
  assert(MemoryImage_addData(&image, (MEMIMAGE_ADDR_T) ADDR_START_CRC32+1, (uint8_t) (addrStart >> 16)));
  assert(MemoryImage_addData(&image, (MEMIMAGE_ADDR_T) ADDR_START_CRC32+2, (uint8_t) (addrStart >>  8)));
  assert(MemoryImage_addData(&image, (MEMIMAGE_ADDR_T) ADDR_START_CRC32+3, (uint8_t) (addrStart >>  0)));

  // store last addresses for CRC32 at fixed RAM address (STM8 = big endian = MSB first)
  assert(MemoryImage_addData(&image, (MEMIMAGE_ADDR_T) ADDR_STOP_CRC32+0, (uint8_t) (addrStop >> 24)));
  assert(MemoryImage_addData(&image, (MEMIMAGE_ADDR_T) ADDR_STOP_CRC32+1, (uint8_t) (addrStop >> 16)));
  assert(MemoryImage_addData(&image, (MEMIMAGE_ADDR_T) ADDR_STOP_CRC32+2, (uint8_t) (addrStop >>  8)));
  assert(MemoryImage_addData(&image, (MEMIMAGE_ADDR_T) ADDR_STOP_CRC32+3, (uint8_t) (addrStop >>  0)));

  // upload RAM routines to STM8
  bsl_memWrite(ptrPort, physInterface, uartMode, &image, MUTE);

  // release memory image
  MemoryImage_free(&image);

  // avoid compiler warnings
  return(0);

} // upload_crc32_address()


uint8_t read_crc32(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t *CRC32)
{
  MemoryImage_s   image;
  uint8_t         value;

  // initialize memory image
  MemoryImage_init(&image);

  // read back CRC32 result (4B)
  bsl_memRead(ptrPort, physInterface, uartMode, RESULT_CRC32, RESULT_CRC32+3, &image, MUTE);

  // get CRC32 result from image buffer (STM8 = big endian = MSB first)
  MemoryImage_getData(&image, (MEMIMAGE_ADDR_T) RESULT_CRC32  , &value); *CRC32  = ((uint32_t) value) << 24;
  MemoryImage_getData(&image, (MEMIMAGE_ADDR_T) RESULT_CRC32+1, &value); *CRC32 += ((uint32_t) value) << 16;
  MemoryImage_getData(&image, (MEMIMAGE_ADDR_T) RESULT_CRC32+2, &value); *CRC32 += ((uint32_t) value) <<  8;
  MemoryImage_getData(&image, (MEMIMAGE_ADDR_T) RESULT_CRC32+3, &value); *CRC32 += ((uint32_t) value) <<  0;

  // release memory image
  MemoryImage_free(&image);

  // avoid compiler warnings
  return(0);

} // read_crc32()


/// compare CRC32-IEEE over microcontroller memory vs. CRC32 over memory image
uint8_t verify_crc32(HANDLE ptrPort, uint8_t family, int flashsize, uint8_t versBSL, uint8_t physInterface, uint8_t uartMode, const MemoryImage_s *image, uint8_t verbose)
{
  uint32_t  crc32_uC, crc32_PC;
  //uint64_t  tStart, tStop;        // measure time [ms]

  // print collective message
  if ((verbose == SILENT) || (verbose == INFORM))
    printf("  CRC32 check ... ");
  fflush(stdout);

  // upload CRC32 RAM routine
  upload_crc32_code(ptrPort, family, flashsize, versBSL, physInterface, uartMode);

  // get start time [ms]
  //tStart = millis();

  // for each consecutive memory range compare CRC32 checksums
  MEMIMAGE_ADDR_T address = 0x00;
  size_t          idxStart, idxEnd;
  while (MemoryImage_getMemoryBlock(image, address, &idxStart, &idxEnd)) {
  
    MEMIMAGE_ADDR_T  addrStart = image->memoryEntries[idxStart].address;
    MEMIMAGE_ADDR_T  addrEnd   = image->memoryEntries[idxEnd].address;

    // print verbose message for each block
    if (verbose == CHATTY)
      printf("  CRC32 check 0x%" PRIX64 " to 0x%" PRIX64 " ... ", (uint64_t) addrStart, (uint64_t) addrEnd);
    fflush(stdout);

    // upload address range for STM8 CRC32 calculation
    upload_crc32_address(ptrPort, physInterface, uartMode, addrStart, addrEnd);

    // jump to CRC32 routine in RAM
    bsl_jumpTo(ptrPort, physInterface, uartMode, (MEMIMAGE_ADDR_T) START_CODE_CRC32, MUTE);

    // for SPI interface (1=SPI_ARDUINO, 2=SPI_SPIDEV) wait sufficiently long (measured empirically)
    if ((physInterface == 1) || (physInterface == 2))
    {
      //fprintf(stderr,"\ntest: %d\n", (int) (25L*(addrEnd - addrStart)/1024L));
      SLEEP(500L + 25L*(addrEnd - addrStart)/1024L);
    }

    // re-synchronize after re-start of ROM-BSL
    bsl_sync(ptrPort, physInterface, MUTE);

    // For UART reset command state machine sending 0x00 until a NACK is received
    // Procedure depends on UART mode (0=duplex, 1=1-wire, 2=2-wire reply). Tested empirically and ugly...
    if (physInterface == 0)
    {
      char      Tx[2] = {0x00, 0x00}, Rx;
      int       lenRx;

      // send (wrong) GET command until NACK is received. Then state machine is ready to receive next command
      set_timeout(ptrPort, 100);
      for (int i=0; i<5; i++)
      {
        if ((uartMode == 0) || (uartMode == 2))
          send_port(ptrPort, 0, 1, Tx);         // duplex and 2-wire reply
        else
          send_port(ptrPort, 0, 2, Tx);         // 1-wire
        lenRx = receive_port(ptrPort, 0, 1, &Rx);
        SLEEP(10);
        if ((lenRx == 1) && (Rx == NACK))
          break;
      }
      set_timeout(ptrPort, TIMEOUT);

      // for UART 2-wire reply mode reply NACK echo
      if (uartMode == 2)
        send_port(ptrPort, 0, 1, &Rx);

      // required for 1-wire reply mode
      if (uartMode == 1)
      {
        SLEEP(10);
        flush_port(ptrPort);
      }

    } // UART interface

    // read out CRC32 checksum from STM8
    read_crc32(ptrPort, physInterface, uartMode, &crc32_uC);
    //printf("\nuC CRC = 0x%08" PRIX32 "\n", crc32_uC);

    // calculate CRC32 checksum over range memory image
    crc32_PC = MemoryImage_checksum_crc32(image, idxStart, idxEnd);
    //printf("\nPC CRC = 0x%08" PRIX32 "\n", crc32_PC);

    // check if CRC32 checksums match
    if (crc32_uC != crc32_PC)
      Error("in 'verify_crc32()': CRC32 checksum mismatch (0x%08" PRIX32 " vs. 0x%08" PRIX32 ")", crc32_uC, crc32_PC);

    // print verbose message
    if (verbose == CHATTY)
      printf("passed (0x%08" PRIX32 ")\n", crc32_uC);
    fflush(stdout);

    // set start address for next block search
    address = addrEnd + 1;

  } // loop over consecutive memory blocks

  // print collective message
  if ((verbose == SILENT) || (verbose == INFORM))
    printf("passed\n");
  fflush(stdout);

  // get end time [ms]
  //tStop = millis();
  //printf("time (%1.1fs)\n", (float) (tStop - tStart)*0.001);

  // avoid compiler warnings
  return(0);

} // verify_crc32()


// end of file
