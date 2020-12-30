/**
  \file bootloader.h

  \author G. Icking-Konert
  \date 2014-03-14
  \version 0.1

  \brief declaration of STM8 bootloader routines

  declaration of STM8 bootloader routines
*/

// for including file only once
#ifndef _BOOTLOADER_H_
#define _BOOTLOADER_H_

#ifdef __cplusplus
extern "C"
{
#endif


// include files
#include "serial_comm.h"
#include <stdbool.h>

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

/// max. number of bootloader synchronization attempts
#define  SYNC_RETRIES   50

/// UART communication timeout
#define  UART_TIMEOUT  1000

/// physical bootloader interface
#if defined(USE_SPIDEV) && defined(USE_SPI_ARDUINO)
  typedef enum {UART=0, SPI_ARDUINO=1, SPI_SPIDEV=2} physInterface_t;
#elif defined(USE_SPI_ARDUINO)
  typedef enum {UART=0, SPI_ARDUINO=1} physInterface_t;
#elif defined(USE_SPIDEV)
  typedef enum {UART=0, SPI_SPIDEV=2} physInterface_t;
#else
  typedef enum {UART=0} physInterface_t;
#endif

typedef enum STM8gal_Bootloader_errors
{
    STM8GAL_BOOTLOADER_NO_ERROR = 0,
    STM8GAL_BOOTLOADER_PORT_NOT_OPEN,
    STM8GAL_BOOTLOADER_TOO_MANY_SYNC_ATTEMPTS,
    STM8GAL_BOOTLOADER_UNKNOWN_INTERFACE,
    STM8GAL_BOOTLOADER_SEND_COMMAND_FAILED,
    STM8GAL_BOOTLOADER_RECEIVE_COMMAND_FAILED,
    STM8GAL_BOOTLOADER_RESPONSE_TIMEOUT,
    STM8GAL_BOOTLOADER_RESPONSE_UNEXPECTED,
    STM8GAL_BOOTLOADER_CANNOT_SEND_TO_PORT,
    STM8GAL_BOOTLOADER_CANNOT_DETERMINE_UART_MODE,
    STM8GAL_BOOTLOADER_CANNOT_IDENTIFY_FAMILY,
    STM8GAL_BOOTLOADER_CANNOT_IDENTIFY_DEVICE,
    STM8GAL_BOOTLOADER_CANNOT_ALLOCATE_BUFFER,
    STM8GAL_BOOTLOADER_VERIFICATION_FAILED,
    STM8GAL_BOOTLOADER_INCORRECT_GET_CODE,
    STM8GAL_BOOTLOADER_INCORRECT_READ_CODE,
    STM8GAL_BOOTLOADER_INCORRECT_GO_CODE,
    STM8GAL_BOOTLOADER_INCORRECT_WRITE_CODE,
    STM8GAL_BOOTLOADER_INCORRECT_ERASE_CODE,
    STM8GAL_BOOTLOADER_ADDRESS_START_GREATER_END,
    STM8GAL_BOOTLOADER_ADDRESS_START_GREATER_BUFFER,
    STM8GAL_BOOTLOADER_ADDRESS_END_GREATER_BUFFER,
    STM8GAL_BOOTLOADER_HEXFILE_ERROR,
} STM8gal_BootloaderErrors_t;

/// synchronize to microcontroller BSL
STM8gal_BootloaderErrors_t bsl_sync(HANDLE ptrPort, uint8_t physInterface, uint8_t verbose);

/// determine UART mode
STM8gal_BootloaderErrors_t bsl_getUartMode(HANDLE ptrPort, uint8_t *mode, uint8_t verbose);

/// get microcontroller type and BSL version
STM8gal_BootloaderErrors_t bsl_getInfo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, int *flashsize, uint8_t *vers, uint8_t *family, uint8_t verbose);

/// read from microcontroller memory
STM8gal_BootloaderErrors_t bsl_memRead(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addrStart, uint64_t addrStop, uint16_t *imageBuf, uint8_t verbose);

/// check if address exists
bool bsl_memCheck(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addr, uint8_t verbose);

/// erase microcontroller flash sector
STM8gal_BootloaderErrors_t bsl_flashSectorErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addr, uint8_t verbose);

/// mass erase microcontroller P- and D-flash
STM8gal_BootloaderErrors_t bsl_flashMassErase(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint8_t verbose);

/// upload to microcontroller flash or RAM
STM8gal_BootloaderErrors_t bsl_memWrite(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose);

/// verify microcontroller memory content vs. or RAM image
STM8gal_BootloaderErrors_t bsl_memVerify(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose);

/// jump to flash or RAM
STM8gal_BootloaderErrors_t bsl_jumpTo(HANDLE ptrPort, uint8_t physInterface, uint8_t uartMode, uint64_t addr, uint8_t verbose);

/// return last error in the Bootloader module
STM8gal_BootloaderErrors_t Bootloader_GetLastError(void);

/// return last error string in the Bootloader module
const char * Bootloader_GetLastErrorString(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _BOOTLOADER_H_

// end of file
