/**
  \file bootloader.h
   
  \author G. Icking-Konert
  \date 2014-03-14
  \version 0.1
   
  \brief declaration of STM bootloader routines
   
  declaration of of STM bootloader routines
*/

// for including file only once
#ifndef _BOOTLOADER_H_
#define _BOOTLOADER_H_


// include files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "serial_comm.h"


// STM8 family
#define STM8S   1         //< STM8S family
#define STM8L   2         //< STM8L family

// BSL command codes
#define GET     0x00      //< gets version and commands supported by the BSL
#define READ    0x11      //< read up to 256 bytes of memory 
#define ERASE   0x43      //< erase flash program memory/data EEPROM sectors
#define WRITE   0x31      //< write up to 128 bytes to RAM or flash
#define GO      0x21      //< jump to a specified address e.g. flash

// BSL return codes
#define SYNCH   0x7F      //< Synchronization byte
#define ACK     0x79      //< Acknowledge
#define NACK    0x1F      //< No acknowledge
#define BUSY    0xAA      //< Busy flag status

#define PFLASH_START      0x8000    //< starting address of flash (same for all STM8 devices)
#define PFLASH_BLOCKSIZE  1024      //< size of flash block for erase or block write (same for all STM8 devices)


/// synchronize to microcontroller BSL
uint8_t bsl_sync(HANDLE ptrPort, uint8_t physInterface, uint8_t verbose);

/// determine UART mode
uint8_t bsl_getUartMode(HANDLE ptrPort, uint8_t verbose);

/// get microcontroller type and BSL version
uint8_t bsl_getInfo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, int *flashsize, uint8_t *vers, uint8_t *family, uint8_t verbose);

/// read from microcontroller memory
uint8_t bsl_memRead(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addrStart, uint64_t addrStop, uint16_t *imageBuf, uint8_t verbose);

/// check if address exists
uint8_t bsl_memCheck(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addr, uint8_t verbose);

/// erase microcontroller flash sector
uint8_t bsl_flashSectorErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addr, uint8_t verbose);

/// mass erase microcontroller P- and D-flash
uint8_t bsl_flashMassErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint8_t verbose);

/// upload to microcontroller flash or RAM
uint8_t bsl_memWrite(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose);

/// verify microcontroller memory content vs. or RAM image
uint8_t bsl_memVerify(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose);

/// jump to flash or RAM
uint8_t bsl_jumpTo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addr, uint8_t verbose);

#endif // _BOOTLOADER_H_

// end of file
