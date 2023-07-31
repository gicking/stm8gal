/**
   \file hexfile.h

   \author G. Icking-Konert

   \brief declaration of routines for HEX, S19 and table files

   declaration of routines for importing and exporting Motorola S19 and Intel HEX files,
   as well as plain ASCII tables.
   (format descriptions under http://en.wikipedia.org/wiki/SREC_(file_format) or
   http://www.keil.com/support/docs/1584.htm).
*/

// for including file only once
#ifndef _HEXFILE_H_
#define _HEXFILE_H_

/**********************
 INCLUDES
**********************/
#include "memory_image.h"


/**********************
 GLOBAL FUNCTIONS
**********************/

/// read Motorola s19 file into memory image
void  import_file_s19(const char *filename, MemoryImage_s *image, const uint8_t verbose);

/// read Intel hex file into memory image
void  import_file_ihx(const char *filename, MemoryImage_s *image, const uint8_t verbose);

/// read plain text table (hex addr / data) file into memory image
void  import_file_txt(const char *filename, MemoryImage_s *image, const uint8_t verbose);

/// read binary file into memory image
void  import_file_bin(const char *filename, const MEMIMAGE_ADDR_T addrStart, MemoryImage_s *image, const uint8_t verbose);


/// read Motorola s19 RAM buffer into memory image
void  import_buffer_s19(uint8_t *buf, MemoryImage_s *image, const uint8_t verbose);

/// read Intel hex RAM buffer into memory image
void  import_buffer_ihx(uint8_t *buf, MemoryImage_s *image, const uint8_t verbose);

/// read plain text table (hex addr / data) RAM buffer into memory image
void  import_buffer_txt(uint8_t *buf, MemoryImage_s *image, const uint8_t verbose);

/// read binary RAM buffer into memory image
void  import_buffer_bin(const uint8_t *buf, const uint64_t lenBuf, const MEMIMAGE_ADDR_T addrStart, MemoryImage_s *image, const uint8_t verbose);


/// export RAM image to file in Motorola s19 format
void  export_file_s19(char *filename, MemoryImage_s *image, const uint8_t verbose);

/// export RAM image to file in Intex hex format
void  export_file_ihx(char *filename, MemoryImage_s *image, const uint8_t verbose);

/// export memory image to plain text file or print to console
void  export_file_txt(char *filename, MemoryImage_s *image, const uint8_t verbose);

/// export RAM image to binary file (w/o address)
void  export_file_bin(char *filename, MemoryImage_s *image, const uint8_t verbose);


/// fill data in memory image with fixed value
void  fill_image(MemoryImage_s *image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrStop, const uint8_t value, const uint8_t verbose);

/// fill data in memory image with random values in 0..255
void  fill_image_random(MemoryImage_s *image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrStop, const uint8_t verbose);

/// clip memory image to specified window
void  clip_image(MemoryImage_s *image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrStop, const uint8_t verbose);

/// cut data from memory image
void  cut_image(MemoryImage_s *image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrStop, const uint8_t verbose);

/// copy data in memory image to new address
void  copy_image(MemoryImage_s *image, const MEMIMAGE_ADDR_T srcStart, const MEMIMAGE_ADDR_T srcStop, const MEMIMAGE_ADDR_T destStart, const uint8_t verbose);

/// move data in memory image to new address
void  move_image(MemoryImage_s *image, const MEMIMAGE_ADDR_T srcStart, const MEMIMAGE_ADDR_T srcStop, const MEMIMAGE_ADDR_T destStart, const uint8_t verbose);

#endif // _HEXFILE_H_
