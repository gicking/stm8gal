/**
  \file verify_CRC32.c

  \author G. Icking-Konert
  \date 2020-12-25
  \version 0.1

  \brief implementation of verify via CRC32 routines

  implementation of routines to verify memory via comparing CRC32 checksums.
*/

// include files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
  uint64_t  addrStart, addrStop, numData;

  // allocate and clear temporary RAM buffer (>1MByte requires dynamic allocation)
  uint16_t  *imageBuf;            // RAM image buffer (high byte != 0 indicates value is set)
  if (!(imageBuf = malloc(LENIMAGEBUF * sizeof(*imageBuf))))
    Error("Cannot allocate image buffer, try reducing LENIMAGEBUF");

  // clear image buffer
  memset(imageBuf, 0, (LENIMAGEBUF + 1) * sizeof(*imageBuf));

  // convert array containing CRC32 RAM ihx file to RAM image
  if ((family == STM8L) && (flashsize==8) && (versBSL==0x10))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8L_8k_v1_0_ihx\n");
    #endif
    convert_ihx((char*) bin_verify_CRC32_STM8L_8k_v1_0_ihx, bin_verify_CRC32_STM8L_8k_v1_0_ihx_len, imageBuf, MUTE);      // STM8L / STM8AL "low density" BL v1.0
  }
  else if ((family == STM8L) && ((flashsize==16) || (flashsize==32)) && (versBSL==0x11))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8L_32k_v1_1_ihx\n");
    #endif
    convert_ihx((char*) bin_verify_CRC32_STM8L_32k_v1_1_ihx, bin_verify_CRC32_STM8L_32k_v1_1_ihx_len, imageBuf, MUTE);    // STM8L / STM8AL "medium density" BL v1.1
  }
  else if ((family == STM8L) && ((flashsize==16) || (flashsize==32)) && (versBSL==0x12))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8L_32k_v1_2_ihx\n");
    #endif
    convert_ihx((char*) bin_verify_CRC32_STM8L_32k_v1_2_ihx, bin_verify_CRC32_STM8L_32k_v1_2_ihx_len, imageBuf, MUTE);    // STM8L / STM8AL "medium density" BL v1.2
  }
  else if ((family == STM8L) && (flashsize==64) && (versBSL==0x11))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8L_64k_v1_1_ihx\n");
    #endif
    convert_ihx((char*) bin_verify_CRC32_STM8L_64k_v1_1_ihx, bin_verify_CRC32_STM8L_64k_v1_1_ihx_len, imageBuf, MUTE);    // STM8L / STM8AL "high/medium+ density" BL v1.1
  }
  else if ((family == STM8S) && (flashsize==32) && (versBSL==0x12))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8S_32k_v1_2_ihx\n");
    #endif
    convert_ihx((char*) bin_verify_CRC32_STM8S_32k_v1_2_ihx, bin_verify_CRC32_STM8S_32k_v1_2_ihx_len, imageBuf, MUTE);    // STM8S / STM8AF "medium density" BL v1.2
  }
  else if ((family == STM8S) && (flashsize==32) && (versBSL==0x13))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8S_32k_v1_3_ihx\n");
    #endif
    convert_ihx((char*) bin_verify_CRC32_STM8S_32k_v1_3_ihx, bin_verify_CRC32_STM8S_32k_v1_3_ihx_len, imageBuf, MUTE);    // STM8S / STM8AF "medium density" BL v1.3
  }
  else if ((family == STM8S) && ((flashsize==64) || (flashsize==128)) && (versBSL==0x21))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8S_128k_v2_1_ihx\n");
    #endif
    convert_ihx((char*) bin_verify_CRC32_STM8S_128k_v2_1_ihx, bin_verify_CRC32_STM8S_128k_v2_1_ihx_len, imageBuf, MUTE);  // STM8S / STM8AF "high density" BL v2.1
  }
  else if ((family == STM8S) && ((flashsize==64) || (flashsize==128)) && (versBSL==0x22))
  {
    #ifdef DEBUG
      printf("header verify_CRC32_STM8S_128k_v2_2_ihx\n");
    #endif
    convert_ihx((char*) bin_verify_CRC32_STM8S_128k_v2_2_ihx, bin_verify_CRC32_STM8S_128k_v2_2_ihx_len, imageBuf, MUTE);  // STM8S / STM8AF "high density" BL v2.2
  }
  else
  {
    Error("bootloader does not support CRC32 verify, use read-out instead (family=%d, flash=%dkB, BL v%d)", (int) family, (int) flashsize, (int) versBSL);
  }

  // get image size
  get_image_size(imageBuf, 0, LENIMAGEBUF, &addrStart, &addrStop, &numData);

  // upload RAM routines to STM8
  bsl_memWrite(ptrPort, physInterface, uartMode, imageBuf, addrStart, addrStop, MUTE);

  // release temporary memory image
  free(imageBuf);

  // avoid compiler warnings
  return(0);

} // upload_crc32_code()



uint8_t upload_crc32_address(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addrStart, uint64_t addrStop)
{
  uint64_t  AddrStart, AddrStop, NumData;

  // allocate and clear temporary RAM buffer (>1MByte requires dynamic allocation)
  uint16_t  *imageBuf;            // RAM image buffer (high byte != 0 indicates value is set)
  if (!(imageBuf = malloc(LENIMAGEBUF * sizeof(*imageBuf))))
    Error("Cannot allocate image buffer, try reducing LENIMAGEBUF");

  // clear image buffer
  memset(imageBuf, 0, (LENIMAGEBUF + 1) * sizeof(*imageBuf));

  // store start addresses for CRC32 at fixed RAM address (big endian = MSB first)
  imageBuf[ADDR_START_CRC32]   = ((uint16_t) (addrStart >> 24)) | 0xFF00; // start address, byte 3
  imageBuf[ADDR_START_CRC32+1] = ((uint16_t) (addrStart >> 16)) | 0xFF00; // start address, byte 2
  imageBuf[ADDR_START_CRC32+2] = ((uint16_t) (addrStart >>  8)) | 0xFF00; // start address, byte 1
  imageBuf[ADDR_START_CRC32+3] = ((uint16_t) (addrStart      )) | 0xFF00; // start address, byte 0

  // store last addresses for CRC32 at fixed RAM address (big endian = MSB first)
  imageBuf[ADDR_STOP_CRC32]    = ((uint16_t) (addrStop >> 24)) | 0xFF00;  // stop address, byte 3
  imageBuf[ADDR_STOP_CRC32+1]  = ((uint16_t) (addrStop >> 16)) | 0xFF00;  // stop address, byte 2
  imageBuf[ADDR_STOP_CRC32+2]  = ((uint16_t) (addrStop >>  8)) | 0xFF00;  // stop address, byte 1
  imageBuf[ADDR_STOP_CRC32+3]  = ((uint16_t) (addrStop      )) | 0xFF00;  // stop address, byte 0

  // get image size
  get_image_size(imageBuf, 0, LENIMAGEBUF, &AddrStart, &AddrStop, &NumData);

  // upload RAM routines to STM8
  bsl_memWrite(ptrPort, physInterface, uartMode, imageBuf, AddrStart, AddrStop, MUTE);

  // release temporary memory image
  free(imageBuf);

  // avoid compiler warnings
  return(0);

} // upload_crc32_address()



uint8_t read_crc32(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t *CRC32)
{
  // allocate and clear temporary RAM buffer (>1MByte requires dynamic allocation)
  uint16_t  *imageBuf;            // RAM image buffer (high byte != 0 indicates value is set)
  if (!(imageBuf = malloc(LENIMAGEBUF * sizeof(*imageBuf))))
    Error("Cannot allocate image buffer, try reducing LENIMAGEBUF");

  // clear image buffer
  memset(imageBuf, 0, (LENIMAGEBUF + 1) * sizeof(*imageBuf));

  // read back CRC32 result (4B)
  bsl_memRead(ptrPort, physInterface, uartMode, RESULT_CRC32, RESULT_CRC32+3, imageBuf, MUTE);

  // get CRC32 result from image buffer
  *CRC32  = ((uint32_t) (imageBuf[RESULT_CRC32]   & 0x00FF)) << 24;
  *CRC32 += ((uint32_t) (imageBuf[RESULT_CRC32+1] & 0x00FF)) << 16;
  *CRC32 += ((uint32_t) (imageBuf[RESULT_CRC32+2] & 0x00FF)) << 8;
  *CRC32 += ((uint32_t) (imageBuf[RESULT_CRC32+3] & 0x00FF));

  // release temporary memory image
  free(imageBuf);

  // avoid compiler warnings
  return(0);

} // read_crc32()


// from https://www.mikrocontroller.net/attachment/61520/crc32_v1.c
uint32_t calculate_crc32(uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop)
{
  uint32_t  crc32;

  // initialize CRC32 checksum
  crc32 = 0xffffffff;

  for(uint64_t addr=addrStart; addr<=addrStop; addr++)
  {
  uint8_t value = (uint8_t) (imageBuf[addr] & 0x00FF);
  for (int i=0; i<8; ++i)
  {
    if ((crc32 & 1) != (value & 1))
    crc32 = (crc32 >> 1) ^ CRC32_POLYNOM;
    else
      crc32 >>= 1;
    value >>= 1;
  }
  }

  // finalize CRC32 checksum
  crc32 ^= 0xffffffff;

  // return result
  return(crc32);

} // calculate_crc32()



/// compare CRC32 over microcontroller memory vs. CRC32 over RAM image
uint8_t verify_crc32(HANDLE ptrPort, uint8_t family, int flashsize, uint8_t versBSL, uint8_t physInterface, uint8_t uartMode, uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose)
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

  // loop over image and check all consecutive data blocks. Skip undefined data
  uint64_t addr = addrStart;
  while (addr <= addrStop) {

  // find next data byte in image (=start address for next read)
  while (((imageBuf[addr] & 0xFF00) == 0) && (addr <= addrStop))
    addr++;

  // end address reached -> done
  if (addr > addrStop)
    break;

  // set length of next check
  uint64_t lenCheck = 1;
  while (((addr+lenCheck) <= addrStop) && (imageBuf[addr+lenCheck] & 0xFF00)) {
    lenCheck++;
  }

  // print verbose message for each block
  if (verbose == CHATTY)
    printf("  CRC32 check 0x%" PRIx64 " to 0x%" PRIx64 " ... ", addr, addr+lenCheck-1);
  fflush(stdout);

  // upload address range for CRC32 calculation
  upload_crc32_address(ptrPort, physInterface, uartMode, addr, addr+lenCheck-1);

  // jump to CRC32 routine in RAM
  bsl_jumpTo(ptrPort, physInterface, uartMode, START_CODE_CRC32, MUTE);

  // for SPI interface (1=SPI_ARDUINO, 2=SPI_SPIDEV) wait sufficiently long (measured empirically)
  if ((physInterface == 1) || (physInterface == 2))
  {
    //fprintf(stderr,"\ntest: %d\n", (int) (25L*lenCheck/1024L));
    SLEEP(500L + 25L*lenCheck/1024L);
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
  //printf("\nuC CRC = 0x%08x\n", crc32_uC);

  // calculate CRC32 checksum over corresponding range in RAM image
  crc32_PC = calculate_crc32(imageBuf, addr, addr+lenCheck-1);
  //printf("\nPC CRC = 0x%08x\n", crc32_PC);

  // check if CRC32 checksums match
  if (crc32_uC != crc32_PC)
    Error("in 'verify_crc32()': CRC32 checksum mismatch (0x%08X vs. 0x%08X)", crc32_uC, crc32_PC);

  // go to next potential block
  addr += lenCheck;

  // print verbose message
  if (verbose == CHATTY)
    printf("passed (0x%08X)\n", crc32_uC);
  fflush(stdout);

  } // loop over image

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
