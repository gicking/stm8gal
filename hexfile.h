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

/// buffer size [B] for memory image
#define  LENIMAGEBUF  50*1024*1024


/// read next line from RAM buffer
char  *get_line(char **buf, char *line);

/// read file into memory buffer
void  load_file(const char *filename, char **fileBuf, uint64_t *lenFileBuf, uint8_t verbose);

/// convert Motorola s19 format in memory buffer to memory image
void  convert_s19(char *fileBuf, uint64_t lenFileBuf, uint16_t *imageBuf, uint8_t verbose);

/// convert Intel hex format in memory buffer to memory image
void  convert_ihx(char *fileBuf, uint64_t lenFileBuf, uint16_t *imageBuf, uint8_t verbose);

/// convert plain text table (hex addr / data) in memory buffer to memory image
void  convert_txt(char *fileBuf, uint64_t lenFileBuf, uint16_t *imageBuf, uint8_t verbose);

/// convert binary data in memory buffer to memory image
void  convert_bin(char *fileBuf, uint64_t lenFileBuf, uint64_t addrStart, uint16_t *imageBuf, uint8_t verbose);


/// get min/max address and number of data bytes in memory image
void  get_image_size(uint16_t *imageBuf, uint64_t scanStart, uint64_t scanStop, uint64_t *addrStart, uint64_t *addrStop, uint64_t *numData);

/// fill data in memory image with fixed value
void  fill_image(uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t value, uint8_t verbose);

/// clip memory image to specified window
void  clip_image(uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose);

/// cut data from memory image
void  cut_image(uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose);

/// copy data in memory image to new address
void  copy_image(uint16_t *imageBuf, uint64_t sourceStart, uint64_t sourceStop, uint64_t destinationStart, uint8_t verbose);

/// move data in memory image to new address
void  move_image(uint16_t *imageBuf, uint64_t sourceStart, uint64_t sourceStop, uint64_t destinationStart, uint8_t verbose);


/// export RAM image to file in Motorola s19 format
void  export_s19(char *filename, uint16_t *imageBuf, uint8_t verbose);

/// export RAM image to file in Intex hex format
void  export_ihx(char *filename, uint16_t *imageBuf, uint8_t verbose);

/// export RAM image to plain text file or print to console
void  export_txt(char *filename, uint16_t *imageBuf, uint8_t verbose);

/// export RAM image to binary file (w/o address)
void  export_bin(char *filename, uint16_t *imageBuf, uint8_t verbose);

#endif // _HEXFILE_H_
