/**
  \file bootloader.h

  \author G. Icking-Konert

  \brief declaration of verify via CRC32 routines

  declaration of routines to verify memory via comparing CRC32 checksums.
*/

// for including file only once
#ifndef _VERIFY32_H_
#define _VERIFY32_H_

// include files
#include <stdint.h>
#include "serial_comm.h"
#include "memory_image.h"

// RAM routine parameters
#define START_CODE_CRC32  0x210    // start address of CRC32 routine in RAM. Note: STM8L 8kB ROM-BL v1.0 uses 0x00..0x206 (BL manual: 0x00..0x200)
#define ADDR_START_CRC32  0x2F4    // location of CRC32 start address (@ 0x2F4 - 0x2F7)
#define ADDR_STOP_CRC32   0x2F8    // location of CRC32 stop address (@ 0x2F8 - 0x2FB)
#define RESULT_CRC32      0x2FC    // location of calculated CRC32 checksum (@ 0x2FC - 0x2FF)

/// compare CRC32 over microcontroller memory vs. CRC32 over RAM image
uint8_t verify_crc32(HANDLE ptrPort, uint8_t family, int flashsize, uint8_t versBSL, uint8_t physInterface, uint8_t uartMode, const MemoryImage_s *image, uint8_t verbose);

#endif // _VERIFY32_H_

// end of file
