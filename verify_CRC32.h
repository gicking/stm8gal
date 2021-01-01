/**
  \file bootloader.h

  \author G. Icking-Konert
  \date 2020-12-25
  \version 0.1

  \brief declaration of verify via CRC32 routines

  declaration of routines to verify memory via comparing CRC32 checksums.
*/

// for including file only once
#ifndef _VERIFY32_H_
#define _VERIFY32_H_

// include files
#include <stdint.h>
#include "serial_comm.h"

// RAM routine parameters
#define START_CODE_CRC32  0x200    // start address of CRC32 routine in RAM
#define ADDR_START_CRC32  0x2F4    // location of CRC32 start address (@ 0x2F4 - 0x2F7)
#define ADDR_STOP_CRC32   0x2F8    // location of CRC32 stop address (@ 0x2F8 - 0x2FB)
#define RESULT_CRC32      0x2FC    // location of calculated CRC32 checksum (@ 0x2FC - 0x2FF)

// CRC32 polynom (for little-endian). TO DO: version for big-endian
#define CRC32_POLYNOM     0xEDB88320


/// upload RAM routine to calculate CRC32 checksum
uint8_t upload_crc32_code(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode);

/// upload address range to calculate CRC32 over
uint8_t upload_crc32_address(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addrStart, uint64_t addrStop);

/// read out CRC2 status and result from microcontroller
uint8_t read_crc32(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint32_t *CRC32);

/// calculate CRC2 checksum over RAM image
uint32_t calculate_crc32(uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop);

/// compare CRC32 over microcontroller memory vs. CRC32 over RAM image
uint8_t verify_crc32(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose);

#endif // _VERIFY32_H_

// end of file
