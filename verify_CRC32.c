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
#include "verify_CRC32_ihx.h"


uint8_t upload_crc32_code(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode)
{
	uint64_t		addrStart, addrStop, numData;

	// allocate and clear temporary RAM buffer (>1MByte requires dynamic allocation)
  uint16_t  *imageBuf;            // RAM image buffer (high byte != 0 indicates value is set)
  if (!(imageBuf = malloc(LENIMAGEBUF * sizeof(*imageBuf))))
    Error("Cannot allocate image buffer, try reducing LENIMAGEBUF");

	// clear image buffer
	memset(imageBuf, 0, (LENIMAGEBUF + 1) * sizeof(*imageBuf));

	// convert array containing ihx file to RAM image
	convert_ihx((char*) bin_verify_CRC32_ihx, bin_verify_CRC32_ihx_len, imageBuf, MUTE);

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
	uint64_t		AddrStart, AddrStop, NumData;

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



uint8_t read_crc32(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint8_t *status, uint32_t *CRC32)
{
	// allocate and clear temporary RAM buffer (>1MByte requires dynamic allocation)
  uint16_t  *imageBuf;            // RAM image buffer (high byte != 0 indicates value is set)
  if (!(imageBuf = malloc(LENIMAGEBUF * sizeof(*imageBuf))))
    Error("Cannot allocate image buffer, try reducing LENIMAGEBUF");

	// clear image buffer
	memset(imageBuf, 0, (LENIMAGEBUF + 1) * sizeof(*imageBuf));

	// read status and CRC result
	bsl_memRead(ptrPort, physInterface, uartMode, STATUS_CRC32, RESULT_CRC32+3, imageBuf, MUTE);

  // get status and CRC result from image buffer
	*status = (uint8_t) (imageBuf[STATUS_CRC32]);
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
uint8_t verify_crc32(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose)
{
	uint8_t		status;
	uint32_t	crc32_uC, crc32_PC;
	//uint64_t  tStart, tStop;        // measure time [ms]

	// print collective message
	if ((verbose == SILENT) || (verbose == INFORM))
		printf("  CRC32 check ... ");
	fflush(stdout);

	// upload CRC32 RAM routine
	upload_crc32_code(ptrPort, physInterface, uartMode);

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
			printf("  CRC32 check 0x%" PRIx64 " to 0x%" PRIx64 " ... ", addr, addr+lenCheck);
		fflush(stdout);

		// upload address range for CRC32 calculation
		upload_crc32_address(ptrPort, physInterface, uartMode, addr, addr+lenCheck-1);

		// jump to CRC32 routine in RAM
		bsl_jumpTo(ptrPort, physInterface, uartMode, START_CODE_CRC32, MUTE);

		// wait before re-synchronizing (time checked empirically)
		SLEEP(4500*lenCheck/(128*1024));

		// re-synchronize after re-start of ROM-BSL
		bsl_sync(ptrPort, physInterface, MUTE);

		// for 2-wire reply mode reply NACK echo
		if (uartMode == 2)
		{
			char Tx = NACK;
			send_port(ptrPort, 0, 1, &Tx);
		}

		// read out CRC32 status and checksum
		read_crc32(ptrPort, physInterface, uartMode, &status, &crc32_uC);
		if (status != 0)
			Error("in 'verify_crc32()': CRC32 status from uC failed (expect 0, received %d)", status);
		//printf("\nuC status = %d, CRC = 0x%08x\n", (int) status, crc32_uC);

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

	// get end time [ms]
	//tStop = millis();
	//printf("\ntime (%1.1fs)\n", (float) (tStop - tStart)*0.001);

	// print collective message
  if ((verbose == SILENT) || (verbose == INFORM))
		printf("passed\n");
	fflush(stdout);

	// avoid compiler warnings
  return(0);

} // verify_crc32()


// end of file
