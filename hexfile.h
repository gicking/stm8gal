/**
   \file hexfile.h

   \author G. Icking-Konert
   \date 2018-12-14
   \version 0.2

   \brief declaration of routines for HEX, S19 and table files

   declaration of routines for importing and exporting Motorola S19 and Intel HEX files,
   as well as plain ASCII tables.
   (format descriptions under http://en.wikipedia.org/wiki/SREC_(file_format) or
   http://www.keil.com/support/docs/1584.htm).
*/

// for including file only once
#ifndef _HEXFILE_H_
#define _HEXFILE_H_

#ifdef __cplusplus
extern "C"
{
#endif

// include files
#include <stdint.h>

typedef enum STM8gal_HexFileErrors
{
    STM8GAL_HEXFILE_NO_ERROR = 0,
    STM8GAL_HEXFILE_FAILED_OPEN_FILE,
    STM8GAL_HEXFILE_FAILED_CREATE_FILE,
    STM8GAL_HEXFILE_FILE_BUFFER_SIZE_EXCEEDED,
    STM8GAL_HEXFILE_S_RECORD_INVALID_START,
    STM8GAL_HEXFILE_S_RECORD_ADDRESS_BUFFER_EXCEEDED,
    STM8GAL_HEXFILE_S_RECORD_CHKSUM_ERROR,
    STM8GAL_HEXFILE_HEX_FILE_INVALID_START,
    STM8GAL_HEXFILE_HEX_FILE_ADDRESS_BUFFER_EXCEEDED,
    STM8GAL_HEXFILE_HEX_FILE_ADDRESS_EXCEEDED_SEGMENT,
    STM8GAL_HEXFILE_HEX_FILE_UNSUPPORTED_RECORD_TYPE,
    STM8GAL_HEXFILE_HEX_FILE_CHKSUM_ERROR,
    STM8GAL_HEXFILE_INVALID_CHAR,
    STM8GAL_HEXFILE_FILE_ADDRESS_INVALID,
    STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER,
} STM8gal_HexFileErrors_t;

/// buffer size [B] for files
#define  LENFILEBUF   50*1024*1024

/// buffer size [B] for memory image
#define  LENIMAGEBUF  50*1024*1024


/// read next line from RAM buffer
char  *get_line(char **buf, char *line);

/// read file into memory buffer
STM8gal_HexFileErrors_t  hexfile_loadFile(const char *filename, char *fileBuf, uint64_t *lenFileBuf, uint8_t verbose);

/// convert Motorola s19 format in memory buffer to memory image
STM8gal_HexFileErrors_t  hexfile_convertS19(char *fileBuf, uint64_t lenFileBuf, uint16_t *imageBuf, uint8_t verbose);

/// convert Intel hex format in memory buffer to memory image
STM8gal_HexFileErrors_t  hexfile_convertIHex(char *fileBuf, uint64_t lenFileBuf, uint16_t *imageBuf, uint8_t verbose);

/// convert plain text table (hex addr / data) in memory buffer to memory image
STM8gal_HexFileErrors_t  hexfile_convertTxt(char *fileBuf, uint64_t lenFileBuf, uint16_t *imageBuf, uint8_t verbose);

/// convert binary data in memory buffer to memory image
STM8gal_HexFileErrors_t  hexfile_convertBin(char *fileBuf, uint64_t lenFileBuf, uint64_t addrStart, uint16_t *imageBuf, uint8_t verbose);


/// get min/max address and number of data bytes in memory image
STM8gal_HexFileErrors_t  hexfile_getImageSize(uint16_t *imageBuf, uint64_t scanStart, uint64_t scanStop, uint64_t *addrStart, uint64_t *addrStop, uint64_t *numData);

/// fill data in memory image with fixed value
STM8gal_HexFileErrors_t  hexfile_fillImage(uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t value, uint8_t verbose);

/// clip memory image to specified window
STM8gal_HexFileErrors_t  hexfile_clipImage(uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose);

/// cut data from memory image
STM8gal_HexFileErrors_t  hexfile_cutImage(uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose);

/// copy data in memory image to new address
STM8gal_HexFileErrors_t  hexfile_copyImage(uint16_t *imageBuf, uint64_t sourceStart, uint64_t sourceStop, uint64_t destinationStart, uint8_t verbose);

/// move data in memory image to new address
STM8gal_HexFileErrors_t  hexfile_moveImage(uint16_t *imageBuf, uint64_t sourceStart, uint64_t sourceStop, uint64_t destinationStart, uint8_t verbose);

/// export RAM image to file in Motorola s19 format
STM8gal_HexFileErrors_t  hexfile_exportS19(char *filename, uint16_t *imageBuf, uint8_t verbose);

/// export RAM image to file in Intex hex format
STM8gal_HexFileErrors_t  hexfile_exportIHex(char *filename, uint16_t *imageBuf, uint8_t verbose);

/// export RAM image to plain text file or print to console
STM8gal_HexFileErrors_t  hexfile_exportTxt(char *filename, uint16_t *imageBuf, uint8_t verbose);

/// export RAM image to binary file (w/o address)
STM8gal_HexFileErrors_t  hexfile_exportBin(char *filename, uint16_t *imageBuf, uint8_t verbose);

/// return last error in the Hexfile module
STM8gal_HexFileErrors_t Hexfile_GetLastError(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _HEXFILE_H_
