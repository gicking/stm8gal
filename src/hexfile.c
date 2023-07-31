/**
   \file hexfile.c

   \author G. Icking-Konert

   \brief implementation of routines for HEX, S19 and table files

   implementation of routines for importing and exporting Motorola S19 and Intel HEX files,
   as well as plain ASCII tables.
   (format descriptions under http://en.wikipedia.org/wiki/SREC_(file_format) or
   http://www.keil.com/support/docs/1584.htm).
*/

// include files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include "hexfile.h"
#include "main.h"
#include "misc.h"

/**
  \fn void import_file_s19(const char *filename, MemoryImage_s *image, const uint8_t verbose)

  \param[in]  filename    full name of file to read 
  \param      image       pointer to memory image. Must be initialized. Existing content is overwritten
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Read Motorola s19 hexfile into memory image. For description of
  Motorola S19 file format see http://en.wikipedia.org/wiki/SREC_(file_format)
*/
void import_file_s19(const char *filename, MemoryImage_s *image, const uint8_t verbose) {

  FILE      *fp;

  // strip path from filename for readability
  #if defined(WIN32)
    const char *shortname = strrchr(filename, '\\');
  #else
    const char *shortname = strrchr(filename, '/');
  #endif
  if (!shortname)
    shortname = filename;
  else
    shortname++;

  // print message
  if (verbose == SILENT)
    printf("  import file '%s' ... ", shortname);    
  else if (verbose == INFORM)
    printf("  import S19 file '%s' ... ", shortname);
  else if (verbose == CHATTY)
    printf("  import Motorola S19 file '%s' ... ", shortname);
  fflush(stdout);

  // open file to read
  if (!(fp = fopen(filename, "rb"))) {
    MemoryImage_free(image);
    Error("Failed to open file %s with error [%s]", filename, strerror(errno));
  }


  //=====================
  // start data import
  //=====================

  char              line[STRLEN], tmp[STRLEN];
  int               linecount = 0, idx, len;
  uint8_t           type, chkRead, chkCalc;
  MEMIMAGE_ADDR_T   address = 0; 
  int               value = 0;

  // read data line by line
  while (fgets(line, STRLEN, fp)) {

    // increase line counter
    linecount++;

    // check 1st char (must be 'S')
    if (line[0] != 'S') {
      MemoryImage_free(image);
      Error("Line %u in Motorola S-record: line does not start with 'S'", linecount);
    }

    // record type
    type = line[1]-48;

    // skip if line contains no data, i.e. line doesn't start with S1, S2 or S3
    if ((type != 1) && (type != 2) && (type != 3))
      continue;

    // record length (address + data + checksum)
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+2, 2);
    sscanf(tmp, "%x", &value);
    len = value;
    chkCalc = value;

    // address (S1=16bit, S2=24bit, S3=32bit)
    address = 0;
    for (int i=0; i<type+1; i++) {
      sprintf(tmp,"0x00");
      tmp[2] = line[4+(i*2)];
      tmp[3] = line[5+(i*2)];
      sscanf(tmp, "%x", &value);
      address *= (uint64_t) 256;
      address += (uint64_t) value;
      chkCalc += (uint8_t) value;
    }

    // read record data
    idx = 6+(type*2);                   // start at position 8, 10, or 12, depending on record type
    len = len-1-(1+type);               // substract chk and address length
    for (MEMIMAGE_ADDR_T i=0; i<len; i++) {
      
      // get next value
      sprintf(tmp,"0x00");
      strncpy(tmp+2, line+idx, 2);      // get next 2 chars as string
      sscanf(tmp, "%x", &value);        // interpret as hex data

      // store data byte in memory image
      assert(MemoryImage_addData(image, address+i, (uint8_t) value));

      chkCalc += (uint8_t) value;       // increase checksum
      idx+=2;                           // advance 2 chars in line
    }

    // read checksum
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+idx, 2);
    sscanf(tmp, "%x", &value);
    chkRead = (uint8_t) value;

    // assert checksum (0xFF xor (sum over all except record type)
    chkCalc ^= 0xFF;                 // invert checksum
    if (chkCalc != chkRead) {
      MemoryImage_free(image);
      Error("Line %u in Motorola S-record: checksum error (0x%02" PRIX8 " vs. 0x%02" PRIX8 ")", linecount, (uint8_t) chkRead, (uint8_t) chkCalc);
    }

  } // while !EOF

  //=====================
  // end data import
  //=====================


  // close file again
  fclose(fp);

  // print message
  if (verbose == SILENT) {
    printf("done\n");
  }
  else if (verbose == INFORM) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB)\n", (float) image->numEntries/1024.0/1024.0);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB)\n", (float) image->numEntries/1024.0);
    else if (image->numEntries > 0)
      printf("done (%dB)\n", (int) image->numEntries);
    else
      printf("done, no data\n");
  }
  else if (verbose == CHATTY) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 0)
      printf("done (%dB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (int) image->numEntries, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else
      printf("done, no data\n");
  }
  fflush(stdout);

} // import_file_s19()



/**
  \fn void import_file_ihx(const char *filename, MemoryImage_s *image, const uint8_t verbose)

  \param[in]  filename    full name of file to read 
  \param      image       pointer to memory image. Must be initialized. Existing content is overwritten
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Read Intel hexfile into memory image. For description of
  Intel hex file format see http://en.wikipedia.org/wiki/Intel_HEX
*/
void import_file_ihx(const char *filename, MemoryImage_s *image, const uint8_t verbose) {

  FILE      *fp;

  // strip path from filename for readability
  #if defined(WIN32)
    const char *shortname = strrchr(filename, '\\');
  #else
    const char *shortname = strrchr(filename, '/');
  #endif
  if (!shortname)
    shortname = filename;
  else
    shortname++;

  // print message
  if (verbose == SILENT)
    printf("  import file '%s' ... ", shortname);    
  else if (verbose == INFORM)
    printf("  import IHX file '%s' ... ", shortname);
  else if (verbose == CHATTY)
    printf("  import Intel IHX file '%s' ... ", shortname);
  fflush(stdout);

  // open file to read
  if (!(fp = fopen(filename, "rb"))) {
    MemoryImage_free(image);
    Error("Failed to open file %s with error [%s]", filename, strerror(errno));
  }


  //=====================
  // start data import
  //=====================

  char              line[STRLEN], tmp[STRLEN];
  int               linecount = 0, idx, len;
  uint8_t           type, chkRead, chkCalc;
  MEMIMAGE_ADDR_T   address = 0; 
  uint64_t          addrOffset, addrJumpStart;
  int               value = 0;

  // avoid compiler warning (variable not yet used). See https://stackoverflow.com/questions/3599160/unused-parameter-warnings-in-c
  (void) (addrJumpStart);

  // read data line by line
  addrOffset = 0x0000000000000000;
  while (fgets(line, STRLEN, fp)) {

    // increase line counter
    linecount++;

    // check 1st char (must be ':')
    if (line[0] != ':') {
      MemoryImage_free(image);
      Error("Line %u in Intel hex record: line does not start with ':'", linecount);
    }

    // record length (address + data + checksum)
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+1, 2);
    sscanf(tmp, "%x", &value);
    len = value;
    chkCalc = len;

    // 16b address
    sprintf(tmp,"0x0000");
    strncpy(tmp+2, line+3, 4);
    sscanf(tmp, "%x", &value);
    chkCalc += (uint8_t) (value >> 8);
    chkCalc += (uint8_t)  value;
    address = (uint64_t) (value + addrOffset);    // add offset for >64kB addresses

    // record type
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+7, 2);
    sscanf(tmp, "%x", &value);
    type = value;
    chkCalc += type;

    // record contains data
    if (type==0) {

      // get data
      idx = 9;                            // start at index 9
      for (int i=0; i<len; i++) {
        
        // get next value
        sprintf(tmp,"0x00");
        strncpy(tmp+2, line+idx, 2);      // get next 2 chars as string
        sscanf(tmp, "%x", &value);        // interpret as hex data
        
        // store data byte in memory image
        assert(MemoryImage_addData(image, address+i, (uint8_t) value));
        
        chkCalc += value;                 // increase checksum
        idx+=2;                           // advance 2 chars in line
      }

    } // type==0

    // EOF indicator
    else if (type==1)
      continue;

    // extended segment addresses not yet supported
    else if (type==2) {
      MemoryImage_free(image);
      Error("Line %u in Intel hex record: extended segment address type 2 not supported", linecount);
    }

    // start segment address (only relevant for 80x86 processor, ignore here)
    else if (type==3)
      continue;

    // extended address (=upper 16b of address for following data records)
    else if (type==4) {
      idx = 13;                       // start at index 13
      sprintf(tmp,"0x0000");
      strncpy(tmp+2, line+9, 4);      // get next 4 chars as string
      sscanf(tmp, "%x", &value);      // interpret as hex data
      chkCalc += (uint8_t) (value >> 8);
      chkCalc += (uint8_t)  value;
      addrOffset = ((uint64_t) value) << 16;
    } // type==4

    // start linear address records. Can be ignored, see http://www.keil.com/support/docs/1584/
    else if (type==5)
      continue;

    // unsupported record type -> error
    else {
      MemoryImage_free(image);
      Error("Line %u in Intel hex record: unsupported type %d", linecount, type);
    }


    // checksum
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+idx, 2);
    sscanf(tmp, "%x", &value);
    chkRead = value;

    // assert checksum (0xFF xor (sum over all except record type))
    chkCalc = 255 - chkCalc + 1;                 // calculate 2-complement
    if (chkCalc != chkRead) {
      MemoryImage_free(image);
      Error("Line %u in Intel hex record: checksum error (0x%02" PRIX8 " vs. 0x%02" PRIX8 ")", linecount, (uint8_t) chkRead, (uint8_t) chkCalc);
    }

    // debug
    //printf("%ld\n", linecount);
    //fflush(stdout);

  } // while !EOF

  //=====================
  // end data import
  //=====================


  // close file again
  fclose(fp);

  // print message
  if (verbose == SILENT){
    printf("done\n");
  }
  else if (verbose == INFORM) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB)\n", (float) image->numEntries/1024.0/1024.0);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB)\n", (float) image->numEntries/1024.0);
    else if (image->numEntries > 0)
      printf("done (%dB)\n", (int) image->numEntries);
    else
      printf("done, no data\n");
  }
  else if (verbose == CHATTY) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 0)
      printf("done (%dB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (int) image->numEntries, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else
      printf("done, no data\n");
  }
  fflush(stdout);

} // import_file_ihx()



/**
  \fn void import_file_txt(const char *filename, MemoryImage_s *image, const uint8_t verbose)

  \param[in]  filename    full name of file to read 
  \param      image       pointer to memory image. Must be initialized. Existing content is overwritten
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Read plain table (address / value) file into image memory.
  Address and value may be decimal (plain numbers) or hexadecimal (starting with '0x').
  Lines starting with '#' are ignored. No syntax check is performed.
*/
void import_file_txt(const char *filename, MemoryImage_s *image, const uint8_t verbose) {

  FILE      *fp;

  // strip path from filename for readability
  #if defined(WIN32)
    const char *shortname = strrchr(filename, '\\');
  #else
    const char *shortname = strrchr(filename, '/');
  #endif
  if (!shortname)
    shortname = filename;
  else
    shortname++;

  // print message
  if (verbose == SILENT)
    printf("  import file '%s' ... ", shortname);    
  else if (verbose == INFORM)
    printf("  import table file '%s' ... ", shortname);
  else if (verbose == CHATTY)
    printf("  import ASCII table file '%s' ... ", shortname);
  fflush(stdout);

  // open file to read
  if (!(fp = fopen(filename, "rb"))) {
    MemoryImage_free(image);
    Error("Failed to open file %s with error [%s]", filename, strerror(errno));
  }


  //=====================
  // start data import
  //=====================

  char            line[STRLEN];
  int             linecount  = 0;
  char            sAddr[STRLEN], sValue[STRLEN];
  uint64_t        address = 0; 
  unsigned int    value = 0;

  // read data line by line
  while (fgets(line, STRLEN, fp)) {

    // increase line counter
    linecount++;

    // if line starts with '#' ignore as comment
    if (line[0] == '#')
      continue;

    // get address and value as string
    sscanf(line, "%s %s", sAddr, sValue);


    //////////
    // extract address
    //////////

    // if string is in hex format, read it
    if (isHexString(sAddr))
      sscanf(sAddr, "%" SCNx64, &address);

    // if string is in decimal format, read it
    else if (isDecString(sAddr))
      sscanf(sAddr, "%" SCNu64, &address);

    // invalid string format
    else {
      MemoryImage_free(image);
      Error("Line %u in table: invalid address '%s'", linecount, sAddr);
    }


    //////////
    // extract value
    //////////

    // if string is in hex format, read it
    if (isHexString(sValue))
      sscanf(sValue, "%x", &value);

    // if string is in decimal format, read it
    else if (isDecString(sValue))
      sscanf(sValue, "%d", &value);

    // invalid string format
    else {
      MemoryImage_free(image);
      Error("Line %u in table: invalid value '%s'", linecount, sValue);
    }


    // store data byte in memory image
    assert(MemoryImage_addData(image, (MEMIMAGE_ADDR_T) address, (uint8_t) value));

  } // while !EOF

  //=====================
  // end data import
  //=====================


  // close file again
  fclose(fp);

  // print message
  if (verbose == SILENT){
    printf("done\n");
  }
  else if (verbose == INFORM) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB)\n", (float) image->numEntries/1024.0/1024.0);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB)\n", (float) image->numEntries/1024.0);
    else if (image->numEntries > 0)
      printf("done (%dB)\n", (int) image->numEntries);
    else
      printf("done, no data\n");
  }
  else if (verbose == CHATTY) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 0)
      printf("done (%dB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (int) image->numEntries, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else
      printf("done, no data\n");
  }
  fflush(stdout);

} // import_file_txt()



/**
  \fn void import_file_bin(const char *filename, const MEMIMAGE_ADDR_T addrStart, MemoryImage_s *image, const uint8_t verbose)

  \param[in]  filename    full name of file to read 
  \param[in]  addrStart   address offset for binary import
  \param      image       pointer to memory image. Must be initialized. Existing content is overwritten
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Read binary file into memory image. Binary data contains no absolute addresses, just data.
  Therefore a starting address must also be provided.
*/
void import_file_bin(const char *filename, const MEMIMAGE_ADDR_T addrStart, MemoryImage_s *image, const uint8_t verbose) {

  FILE      *fp;

  // strip path from filename for readability
  #if defined(WIN32)
    const char *shortname = strrchr(filename, '\\');
  #else
    const char *shortname = strrchr(filename, '/');
  #endif
  if (!shortname)
    shortname = filename;
  else
    shortname++;

  // print message
  if (verbose == SILENT)
    printf("  import file '%s' ... ", shortname);    
  else if (verbose == INFORM)
    printf("  import BIN file '%s' ... ", shortname);
  else if (verbose == CHATTY)
    printf("  import binary file '%s' ... ", shortname);
  fflush(stdout);

  // open file to read
  if (!(fp = fopen(filename, "rb"))) {
    MemoryImage_free(image);
    Error("Failed to open file %s with error [%s]", filename, strerror(errno));
  }


  //=====================
  // start data import
  //=====================

  // read bytes and store to image
  MEMIMAGE_ADDR_T  address = addrStart;
  uint8_t  value;
  while (!feof(fp)) {
    
    // read next byte
    fread(&value, sizeof(uint8_t), 1, fp);

    // store in memory image
    if (!feof(fp))
      assert(MemoryImage_addData(image, (MEMIMAGE_ADDR_T) address, (uint8_t) value));

    // increment address
    address++;

  } // while !EOF

  //=====================
  // end data import
  //=====================


  // close file again
  fclose(fp);

  // print message
  if (verbose == SILENT){
    printf("done\n");
  }
  else if (verbose == INFORM) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB)\n", (float) image->numEntries/1024.0/1024.0);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB)\n", (float) image->numEntries/1024.0);
    else if (image->numEntries > 0)
      printf("done (%dB)\n", (int) image->numEntries);
    else
      printf("done, no data\n");
  }
  else if (verbose == CHATTY) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 0)
      printf("done (%dB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (int) image->numEntries, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else
      printf("done, no data\n");
  }
  fflush(stdout);

} // import_file_bin()



/**
  \fn void import_buffer_s19(uint8_t *buf, MemoryImage_s *image, const uint8_t verbose)

  \param[in]  buf         RAM buffer to read (NUL terminated)
  \param      image       pointer to memory image. Must be initialized. Existing content is overwritten
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Import RAM buffer containing Motorola s19 hexfile into memory image. For description of
  Motorola S19 file format see http://en.wikipedia.org/wiki/SREC_(file_format)
*/
void import_buffer_s19(uint8_t *buf, MemoryImage_s *image, const uint8_t verbose) {

  // print message
  if (verbose == SILENT)
    printf("  import buffer ... ");    
  else if (verbose == INFORM)
    printf("  import S19 buffer ... ");
  else if (verbose == CHATTY)
    printf("  import Motorola S19 buffer ... ");
  fflush(stdout);


  //=====================
  // start data import
  //=====================

  char              *line, tmp[STRLEN];
  int               linecount = 0, idx, len;
  uint8_t           type, chkRead, chkCalc;
  MEMIMAGE_ADDR_T   address = 0; 
  int               value = 0;

  // read buffer line by line
  line = strtok((char*) buf, "\n\r");
  while (line != NULL) {

    // increase line counter
    linecount++;

    // check 1st char (must be 'S')
    if (line[0] != 'S') {
      MemoryImage_free(image);
      Error("Line %u in Motorola S-record: line does not start with 'S'", linecount);
    }

    // record type
    type = line[1]-48;

    // skip if line contains no data, i.e. line doesn't start with S1, S2 or S3
    if ((type != 1) && (type != 2) && (type != 3)) {
      line  = strtok(NULL, "\n\r");
      continue;
    }

    // record length (address + data + checksum)
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+2, 2);
    sscanf(tmp, "%x", &value);
    len = value;
    chkCalc = value;

    // address (S1=16bit, S2=24bit, S3=32bit)
    address = 0;
    for (int i=0; i<type+1; i++) {
      sprintf(tmp,"0x00");
      tmp[2] = line[4+(i*2)];
      tmp[3] = line[5+(i*2)];
      sscanf(tmp, "%x", &value);
      address *= (uint64_t) 256;
      address += (uint64_t) value;
      chkCalc += (uint8_t) value;
    }

    // read record data
    idx = 6+(type*2);                   // start at position 8, 10, or 12, depending on record type
    len = len-1-(1+type);               // substract chk and address length
    for (MEMIMAGE_ADDR_T i=0; i<len; i++) {
      
      // get next value
      sprintf(tmp,"0x00");
      strncpy(tmp+2, line+idx, 2);      // get next 2 chars as string
      sscanf(tmp, "%x", &value);        // interpret as hex data

      // store data byte in memory image
      assert(MemoryImage_addData(image, address+i, (uint8_t) value));

      chkCalc += (uint8_t) value;       // increase checksum
      idx+=2;                           // advance 2 chars in line
    }

    // read checksum
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+idx, 2);
    sscanf(tmp, "%x", &value);
    chkRead = (uint8_t) value;

    // assert checksum (0xFF xor (sum over all except record type)
    chkCalc ^= 0xFF;                 // invert checksum
    if (chkCalc != chkRead) {
      MemoryImage_free(image);
      Error("Line %u in Motorola S-record: checksum error (0x%02" PRIX8 " vs. 0x%02" PRIX8 ")", linecount, (uint8_t) chkRead, (uint8_t) chkCalc);
    }

    // get next line
    line = strtok(NULL, "\n\r");

  } // while ! end of buffer

  //=====================
  // end data import
  //=====================


  // print message
  if (verbose == SILENT){
    printf("done\n");
  }
  else if (verbose == INFORM) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB)\n", (float) image->numEntries/1024.0/1024.0);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB)\n", (float) image->numEntries/1024.0);
    else if (image->numEntries > 0)
      printf("done (%dB)\n", (int) image->numEntries);
    else
      printf("done, no data\n");
  }
  else if (verbose == CHATTY) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 0)
      printf("done (%dB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (int) image->numEntries, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else
      printf("done, no data\n");
  }
  fflush(stdout);

} // import_buffer_s19()



/**
  \fn void import_buffer_ihx(uint8_t *buf, MemoryImage_s *image, const uint8_t verbose)

  \param[in]  buf         RAM buffer to read (NUL terminated)
  \param      image       pointer to memory image. Must be initialized. Existing content is overwritten
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Read RAM buffer containing Intel hexfile into memory image. For description of
  Intel hex file format see http://en.wikipedia.org/wiki/Intel_HEX
*/
void import_buffer_ihx(uint8_t *buf, MemoryImage_s *image, const uint8_t verbose) {

  // print message
  if (verbose == SILENT)
    printf("  import buffer ... ");    
  else if (verbose == INFORM)
    printf("  import IHX buffer ... ");
  else if (verbose == CHATTY)
    printf("  import Intel IHX buffer ... ");
  fflush(stdout);


  //=====================
  // start data import
  //=====================

  char              *line, tmp[STRLEN];
  int               linecount = 0, idx, len;
  uint8_t           type, chkRead, chkCalc;
  MEMIMAGE_ADDR_T   address = 0; 
  uint64_t          addrOffset, addrJumpStart;
  int               value = 0;

  // avoid compiler warning (variable not yet used). See https://stackoverflow.com/questions/3599160/unused-parameter-warnings-in-c
  (void) (addrJumpStart);

  // read data line by line
  addrOffset = 0x0000000000000000;
  line = strtok((char*) buf, "\n\r");
  while (line != NULL) {

    // increase line counter
    linecount++;

    // check 1st char (must be ':')
    if (line[0] != ':') {
      MemoryImage_free(image);
      Error("Line %u in Intel hex record: line does not start with ':'", linecount);
    }

    // record length (address + data + checksum)
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+1, 2);
    sscanf(tmp, "%x", &value);
    len = value;
    chkCalc = len;

    // 16b address
    sprintf(tmp,"0x0000");
    strncpy(tmp+2, line+3, 4);
    sscanf(tmp, "%x", &value);
    chkCalc += (uint8_t) (value >> 8);
    chkCalc += (uint8_t)  value;
    address = (uint64_t) (value + addrOffset);    // add offset for >64kB addresses

    // record type
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+7, 2);
    sscanf(tmp, "%x", &value);
    type = value;
    chkCalc += type;

    // record contains data
    if (type==0) {

      // get data
      idx = 9;                            // start at index 9
      for (int i=0; i<len; i++) {
        
        // get next value
        sprintf(tmp,"0x00");
        strncpy(tmp+2, line+idx, 2);      // get next 2 chars as string
        sscanf(tmp, "%x", &value);        // interpret as hex data
        
        // store data byte in memory image
        assert(MemoryImage_addData(image, address+i, (uint8_t) value));
        
        chkCalc += value;                 // increase checksum
        idx+=2;                           // advance 2 chars in line
      }

    } // type==0

    // EOF indicator
    else if (type==1) {
      line  = strtok(NULL, "\n\r");
      continue;
    }
  
    // extended segment addresses not yet supported
    else if (type==2) {
      MemoryImage_free(image);
      Error("Line %u in Intel hex record: extended segment address type 2 not supported", linecount);
    }

    // start segment address (only relevant for 80x86 processor, ignore here)
    else if (type==3) {
      line  = strtok(NULL, "\n\r");
      continue;
    }

    // extended address (=upper 16b of address for following data records)
    else if (type==4) {
      idx = 13;                       // start at index 13
      sprintf(tmp,"0x0000");
      strncpy(tmp+2, line+9, 4);      // get next 4 chars as string
      sscanf(tmp, "%x", &value);      // interpret as hex data
      chkCalc += (uint8_t) (value >> 8);
      chkCalc += (uint8_t)  value;
      addrOffset = ((uint64_t) value) << 16;
    } // type==4

    // start linear address records. Can be ignored, see http://www.keil.com/support/docs/1584/
    else if (type==5) {
      line  = strtok(NULL, "\n\r");
      continue;
    }

    // unsupported record type -> error
    else {
      MemoryImage_free(image);
      Error("Line %u in Intel hex record: unsupported type %d", linecount, type);
    }


    // checksum
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+idx, 2);
    sscanf(tmp, "%x", &value);
    chkRead = value;

    // assert checksum (0xFF xor (sum over all except record type))
    chkCalc = 255 - chkCalc + 1;                 // calculate 2-complement
    if (chkCalc != chkRead) {
      MemoryImage_free(image);
      Error("Line %u in Intel hex record: checksum error (0x%02" PRIX8 " vs. 0x%02" PRIX8 ")", linecount, (uint8_t) chkRead, (uint8_t) chkCalc);
    }

    // debug
    //printf("%ld\n", linecount);
    //fflush(stdout);

    // get next line
    line = strtok(NULL, "\n\r");

  } // while ! end of buffer

  //=====================
  // end data import
  //=====================


  // print message
  if (verbose == SILENT){
    printf("done\n");
  }
  else if (verbose == INFORM) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB)\n", (float) image->numEntries/1024.0/1024.0);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB)\n", (float) image->numEntries/1024.0);
    else if (image->numEntries > 0)
      printf("done (%dB)\n", (int) image->numEntries);
    else
      printf("done, no data\n");
  }
  else if (verbose == CHATTY) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 0)
      printf("done (%dB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (int) image->numEntries, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else
      printf("done, no data\n");
  }
  fflush(stdout);

} // import_buffer_ihx()



/**
  \fn void import_buffer_txt(uint8_t *buf, MemoryImage_s *image, const uint8_t verbose)

  \param[in]  buf         RAM buffer to read (NUL terminated)
  \param      image       pointer to memory image. Must be initialized. Existing content is overwritten
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Read plain table (address / value) file into image memory.
  Address and value may be decimal (plain numbers) or hexadecimal (starting with '0x').
  Lines starting with '#' are ignored. No syntax check is performed.
*/
void import_buffer_txt(uint8_t *buf, MemoryImage_s *image, const uint8_t verbose) {

  // print message
  if (verbose == SILENT)
    printf("  import buffer ... ");    
  else if (verbose == INFORM)
    printf("  import table buffer ... ");
  else if (verbose == CHATTY)
    printf("  import ASCII table buffer ... ");
  fflush(stdout);


  //=====================
  // start data import
  //=====================

  char            *line;
  int             linecount  = 0;
  char            sAddr[STRLEN], sValue[STRLEN];
  uint64_t        address = 0; 
  unsigned int    value = 0;

  // read data line by line
  line = strtok((char*) buf, "\n\r");
  while (line != NULL) {

    // increase line counter
    linecount++;

    // if line starts with '#' ignore as comment
    if (line[0] == '#') {
      line  = strtok(NULL, "\n\r");
      continue;
    }

    // get address and value as string
    sscanf(line, "%s %s", sAddr, sValue);


    //////////
    // extract address
    //////////

    // if string is in hex format, read it
    if (isHexString(sAddr))
      sscanf(sAddr, "%" SCNx64, &address);

    // if string is in decimal format, read it
    else if (isDecString(sAddr))
      sscanf(sAddr, "%" SCNu64, &address);

    // invalid string format
    else {
      MemoryImage_free(image);
      Error("Line %u in table: invalid address '%s'", linecount, sAddr);
    }


    //////////
    // extract value
    //////////

    // if string is in hex format, read it
    if (isHexString(sValue))
      sscanf(sValue, "%x", &value);

    // if string is in decimal format, read it
    else if (isDecString(sValue))
      sscanf(sValue, "%d", &value);

    // invalid string format
    else {
      MemoryImage_free(image);
      Error("Line %u in table: invalid value '%s'", linecount, sValue);
    }

    // store data byte in memory image
    assert(MemoryImage_addData(image, (MEMIMAGE_ADDR_T) address, (uint8_t) value));

    // get next line
    line  = strtok(NULL, "\n\r");

  } // while ! end of buffer

  //=====================
  // end data import
  //=====================


  // print message
  if (verbose == SILENT){
    printf("done\n");
  }
  else if (verbose == INFORM) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB)\n", (float) image->numEntries/1024.0/1024.0);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB)\n", (float) image->numEntries/1024.0);
    else if (image->numEntries > 0)
      printf("done (%dB)\n", (int) image->numEntries);
    else
      printf("done, no data\n");
  }
  else if (verbose == CHATTY) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 0)
      printf("done (%dB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (int) image->numEntries, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else
      printf("done, no data\n");
  }
  fflush(stdout);

} // import_buffer_txt()



/**
  \fn void import_buffer_bin(const uint8_t *buf, const uint64_t lenBuf, const MEMIMAGE_ADDR_T addrStart, MemoryImage_s *image, const uint8_t verbose)

  \param[in]  buf         RAM buffer to read (NUL terminated)
  \param[in]  num         length of RAM buffer
  \param[in]  addrStart   address offset for binary import
  \param      image       pointer to memory image. Must be initialized. Existing content is overwritten
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Read binary file into memory image. Binary data contains no absolute addresses, just data.
  Therefore a starting address must also be provided.
*/
void import_buffer_bin(const uint8_t *buf, const uint64_t lenBuf, const MEMIMAGE_ADDR_T addrStart, MemoryImage_s *image, const uint8_t verbose) {

  // print message
  if (verbose == SILENT)
    printf("  import buffer ... ");    
  else if (verbose == INFORM)
    printf("  import BIN buffer ... ");
  else if (verbose == CHATTY)
    printf("  import binary buffer ... ");
  fflush(stdout);


  //=====================
  // start data import
  //=====================

  // read bytes and store to image
  MEMIMAGE_ADDR_T  address = addrStart;
  uint8_t  value;
  for (uint64_t i=0; i<lenBuf; i++) {
    
    // read next byte
    value = buf[i];

    // store in memory image
    assert(MemoryImage_addData(image, (MEMIMAGE_ADDR_T) address, (uint8_t) value));

    // increment address
    address++;

  } // while !EOF

  //=====================
  // end data import
  //=====================


  // print message
  if (verbose == SILENT) {
    printf("done\n");
  }
  else if (verbose == INFORM) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB)\n", (float) image->numEntries/1024.0/1024.0);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB)\n", (float) image->numEntries/1024.0);
    else if (image->numEntries > 0)
      printf("done (%dB)\n", (int) image->numEntries);
    else
      printf("done, no data\n");
  }
  else if (verbose == CHATTY) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 0)
      printf("done (%dB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (int) image->numEntries, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else
      printf("done, no data\n");
  }
  fflush(stdout);

} // import_buffer_bin()



/**
  \fn void export_file_s19(char *filename, MemoryImage_s *image, const uint8_t verbose)

  \param[in]  filename    name of output file
  \param[in]  image       pointer to memory image
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Export memory image to Motorola s19 hexfile. For description of
  Motorola S19 file format see http://en.wikipedia.org/wiki/SREC_(file_format)
*/
void export_file_s19(char *filename, MemoryImage_s *image, const uint8_t verbose) {

  FILE              *fp;                  // file pointer
  char              *shortname;           // filename w/o path
  const int         maxLine = 32;         // max. length of data line
  MEMIMAGE_ADDR_T   addrBlock, addrLine, addrStart, addrEnd;
  size_t            idxStart, idxEnd;
  uint8_t           value;                // image data value
  uint32_t          chk;                  // checksum

  // strip path from filename for readability
  #if defined(WIN32)
    shortname = strrchr(filename, '\\');
  #else
    shortname = strrchr(filename, '/');
  #endif
  if (!shortname)
    shortname = filename;
  else
    shortname++;

  // print message
  if (verbose == SILENT)
    printf("  export file '%s' ... ", shortname);
  else if (verbose == INFORM)
    printf("  export S19 file '%s' ... ", shortname);
  else if (verbose == CHATTY)
    printf("  export Motorola S19 file '%s' ... ", shortname);
  fflush(stdout);

  // open output file
  fp = fopen(filename,"wb");
  if (!fp) {
    MemoryImage_free(image);
    Error("Failed to create file %s with error [%s]", filename, strerror(errno));
  }

  // start with dummy header line to avoid 'srecord' warning
  fprintf(fp, "S00E000068656C6C6F20776F726C6495\n");

  // loop over consecutive memory blocks in image
  addrBlock = 0x00;
  while (MemoryImage_getMemoryBlock(image, addrBlock, &idxStart, &idxEnd)) {

    addrStart = image->memoryEntries[idxStart].address;
    addrEnd   = image->memoryEntries[idxEnd].address;

    // loop over memory block and store in lines of max. 32B
    addrLine = addrStart;
    while (addrLine <= addrEnd) {
        
      // get length of next line to store (max. 32B)
      int lenLine = 1;
      while ((lenLine < maxLine) && ((addrLine+lenLine) <= addrEnd))
        lenLine++;
      //printf("0x%04" PRIX64 "   0x%04" PRIX64 "   %d\n", (uint64_t) addrLine, (uint64_t) (addrLine+lenLine-1), lenLine);

      // save next line, accound for address width. See http://en.wikipedia.org/wiki/SREC_(file_format)
      if (addrLine+lenLine <= (uint64_t) 0xFFFF) {
        fprintf(fp, "S1%02X%04X", lenLine+3, (int) addrLine);         // 16-bit address: 2B addr + data + 1B chk
        chk = (uint8_t) (lenLine+3) + (uint8_t) addrLine + (uint8_t) (addrLine >> 8);
      }
      else if (addrLine+lenLine <= (uint64_t) 0xFFFFFF) {
        fprintf(fp, "S2%02X%06X", lenLine+4, (int) addrLine);         // 24-bit address: 3B addr + data + 1B chk
        chk = (uint8_t) (lenLine+4) + (uint8_t) addrLine + (uint8_t) (addrLine >> 8) + (uint8_t) (addrLine >> 16);
      }
      else {
        fprintf(fp, "S3%02X%08X", lenLine+5, (int) addrLine);         // 32-bit address: 4B addr + data + 1B chk
        chk = (uint8_t) (lenLine+5) + (uint8_t) addrLine + (uint8_t) (addrLine >> 8) + (uint8_t) (addrLine >> 16) + (uint8_t) (addrLine >> 24);
      }
      for (int j=0; j<lenLine; j++) {
        MemoryImage_getData(image, addrLine+j, &value);
        chk += value;
        fprintf(fp, "%02X", value);
      }
      chk = ((chk & 0xFF) ^ 0xFF);
      fprintf(fp, "%02X\n", (uint8_t) chk);

      // go to next line
      addrLine += lenLine;

    } // loop address over memory block

    // start address for searching next memory block 
    addrBlock = addrEnd + 1;

  } // loop over memory blocks in image

  // attach appropriate termination record, according to type of data records used
  if (addrEnd <= (uint64_t) 0xFFFF)
    fprintf(fp, "S903FFFFFE\n");        // 16-bit addresses
  else if (addrEnd <= (uint64_t) 0xFFFFFF)
    fprintf(fp, "S804FFFFFFFE\n");      // 24-bit addresses
  else
    fprintf(fp, "S705FFFFFFFFFE\n");    // 32-bit addresses

  // close output file
  fflush(fp);
  fclose(fp);

  // print message
  if (verbose == SILENT){
    printf("done\n");
  }
  else if (verbose == INFORM) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB)\n", (float) image->numEntries/1024.0/1024.0);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB)\n", (float) image->numEntries/1024.0);
    else if (image->numEntries > 0)
      printf("done (%dB)\n", (int) image->numEntries);
    else
      printf("done, no data\n");
  }
  else if (verbose == CHATTY) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 0)
      printf("done (%dB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (int) image->numEntries, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else
      printf("done, no data\n");
  }
  fflush(stdout);

} // export_file_s19



/**
  \fn void export_file_ihx(char *filename, MemoryImage_s *image, const uint8_t verbose);

  \param[in]  filename    name of output file
  \param[in]  image       pointer to memory image
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Export memory image to Intel hexfile. For description of
  Intel hex file format see http://en.wikipedia.org/wiki/Intel_HEX
*/

void export_file_ihx(char *filename, MemoryImage_s *image, const uint8_t verbose) {

  FILE              *fp;               // file pointer
  char              *shortname;        // filename w/o path
  const int         maxLine = 32;      // max. length of data line
  MEMIMAGE_ADDR_T   addrBlock, addrLine, addrStart, addrEnd;
  size_t            idxStart, idxEnd;
  uint8_t           value;             // image data value
  uint32_t          chk;               // checksum
  bool              useEla = 0;        // whether ELA records needed
  int64_t           addrEla;           // ELA record address

  // strip path from filename for readability
  #if defined(WIN32)
    shortname = strrchr(filename, '\\');
  #else
    shortname = strrchr(filename, '/');
  #endif
  if (!shortname)
    shortname = filename;
  else
    shortname++;

  // print message
  if (verbose == SILENT)
    printf("  export file '%s' ... ", shortname);
  else if (verbose == INFORM)
    printf("  export IHX file '%s' ... ", shortname);
  else if (verbose == CHATTY)
    printf("  export Intel HEX file '%s' ... ", shortname);
  fflush(stdout);

  // open output file
  fp=fopen(filename,"wb");
  if (!fp) {
    MemoryImage_free(image);
    Error("Failed to create file %s with error [%s]", filename, strerror(errno));
  }

  // use ELA records if address range is greater than 16 bits
  if ((MemoryImage_isEmpty(image) == false) && (image->memoryEntries[image->numEntries-1].address > 0xFFFF)) {
    useEla  = true;
    addrEla = -1;
  }

  // loop over consecutive memory blocks in image
  addrBlock = 0x00;
  while (MemoryImage_getMemoryBlock(image, addrBlock, &idxStart, &idxEnd)) {

    addrStart = image->memoryEntries[idxStart].address;
    addrEnd   = image->memoryEntries[idxEnd].address;

    // loop over memory block and store in lines of max. 32B
    addrLine = addrStart;
    while (addrLine <= addrEnd) {
        
      // get length of next line to store (max. 32B)
      int lenLine = 1;
      while ((lenLine < maxLine) && ((addrLine+lenLine) <= addrEnd))
        lenLine++;
      //printf("0x%04" PRIX64 "   0x%04" PRIX64 "   %d\n", (uint64_t) addrLine, (uint64_t) (addrLine+lenLine-1), lenLine);
        
      // write ELA record if upper 16-bits of line is different than last ELA addr
      if ((useEla == true) && (addrEla != (addrLine >> 16))) {
        addrEla = addrLine >> 16;
        chk = ~(0x02 + 0x04 + (uint8_t) addrEla + (uint8_t) (addrEla >> 8)) + 1;
        fprintf(fp, ":02000004%04X%02X\n", (uint16_t) addrEla, (uint8_t) chk);
      }

      // save next line
      fprintf(fp, ":%02X%04X00", lenLine, (uint16_t) addrLine);
      chk = lenLine + (uint8_t) addrLine + (uint8_t) (addrLine >> 8);
      for (uint8_t j = 0; j < lenLine; j++) {
        MemoryImage_getData(image, addrLine+j, &value);
        chk += value;
        fprintf(fp, "%02X", value);
      }
      chk = ~chk + 1;
      fprintf(fp, "%02X\n", (uint8_t) chk);

      // go to next line
      addrLine += lenLine;

    } // loop address over memory block

    // start address for searching next memory block 
    addrBlock = addrEnd + 1;

  } // loop over memory blocks in image

  // output end-of-file record
  fprintf(fp, ":00000001FF\n");

  // close output file
  fflush(fp);
  fclose(fp);

  // print message
  if (verbose == SILENT){
    printf("done\n");
  }
  else if (verbose == INFORM) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB)\n", (float) image->numEntries/1024.0/1024.0);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB)\n", (float) image->numEntries/1024.0);
    else if (image->numEntries > 0)
      printf("done (%dB)\n", (int) image->numEntries);
    else
      printf("done, no data\n");
  }
  else if (verbose == CHATTY) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 0)
      printf("done (%dB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (int) image->numEntries, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else
      printf("done, no data\n");
  }
  fflush(stdout);

} // export_file_ihx



/**
  void export_file_txt(char *filename, MemoryImage_s *image, const uint8_t verbose)

  \param[in]  filename    name of output file or stdout ('console')
  \param[in]  image       pointer to memory image
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Export memory image to file with plain text table (hex addr / hex data)
*/
void export_file_txt(char *filename, MemoryImage_s *image, const uint8_t verbose) {

  FILE      *fp;               // file pointer
  char      *shortname;        // filename w/o path
  bool      flagFile = true;   // output to file or console?

  // output to stdout
  if (!strcmp(filename, "console")) {
    flagFile = false;
    fp = stdout;
    if (verbose > MUTE)
      printf("  print memory\n");
    fflush(stdout);
  }

  // output to file
  else {
    flagFile = true;

    // strip path from filename for readability
    #if defined(WIN32)
      shortname = strrchr(filename, '\\');
    #else
      shortname = strrchr(filename, '/');
    #endif
    if (!shortname)
      shortname = filename;
    else
      shortname++;

    // print message
    if (verbose == SILENT)
      printf("  export file '%s' ... ", shortname);
    else if (verbose == INFORM)
      printf("  export table file '%s' ... ", shortname);
    else if (verbose == CHATTY)
      printf("  export ASCII table to file '%s' ... ", shortname);
    fflush(stdout);

    // open output file
    fp=fopen(filename,"wb");
    if (!fp) {
      MemoryImage_free(image);
      Error("Failed to create file %s with error [%s]", filename, strerror(errno));
    }

  } // output to file

  // output header
  if (flagFile)
    fprintf(fp, "# address\tvalue\n");
  else
    fprintf(fp, "    address\tvalue\n");

  // loop over image and output address, data in hex format
  for (size_t i = 0; i < image->numEntries; i++) {
    if (flagFile)
      fprintf(fp, "0x%" PRIX64 "\t0x%02" PRIX8 "\n", (uint64_t) image->memoryEntries[i].address, (int) image->memoryEntries[i].data & 0xFF);
    else
      fprintf(fp, "    0x%" PRIX64 "\t0x%02" PRIX8 "\n", (uint64_t) image->memoryEntries[i].address, (int) image->memoryEntries[i].data & 0xFF);      
  }

  // close output file
  fflush(fp);
  if (flagFile)
    fclose(fp);
  else
    fprintf(fp,"  ");

  // print message
  if (verbose == SILENT){
    printf("done\n");
  }
  else if (verbose == INFORM) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB)\n", (float) image->numEntries/1024.0/1024.0);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB)\n", (float) image->numEntries/1024.0);
    else if (image->numEntries > 0)
      printf("done (%dB)\n", (int) image->numEntries);
    else
      printf("done, no data\n");
  }
  else if (verbose == CHATTY) {
    if (image->numEntries > 1024*1024)
      printf("done (%1.1fMB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 1024)
      printf("done (%1.1fkB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) image->numEntries/1024.0, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else if (image->numEntries > 0)
      printf("done (%dB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (int) image->numEntries, 
        (uint64_t) image->memoryEntries[0].address, (uint64_t) image->memoryEntries[image->numEntries-1].address);
    else
      printf("done, no data\n");
  }
  fflush(stdout);

} // export_file_txt



/**
   \fn void export_file_bin(char *filename, MemoryImage_s *image, const uint8_t verbose)

   \param[in]  filename    name of output file
   \param[in]  image       pointer to memory image
   \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   Export memory image to binary file. Note that start address is not stored, and that
   binary format does not allow for "holes" in the file, i.e. undefined data is stored as 0x00.
*/
void export_file_bin(char *filename, MemoryImage_s *image, const uint8_t verbose) {

  FILE      *fp;                  // file pointer
  uint64_t  addrStart, addrStop;  // address range to export
  uint64_t  countByte;            // number of actually exported bytes
  uint8_t   value;

  // strip path from filename for readability
  #if defined(WIN32)
    const char *shortname = strrchr(filename, '\\');
  #else
    const char *shortname = strrchr(filename, '/');
  #endif
  if (!shortname)
    shortname = filename;
  else
    shortname++;

  // print message
  if (verbose == SILENT)
    printf("  export file '%s' ... ", shortname);
  else if (verbose == INFORM)
    printf("  export BIN file '%s' ... ", shortname);
  else if (verbose == CHATTY)
    printf("  export binary file '%s' ... ", shortname);
  fflush(stdout);

  // open output file
  fp=fopen(filename,"wb");
  if (!fp) {
    MemoryImage_free(image);
    Error("Failed to create file %s with error [%s]", filename, strerror(errno));
  }

  // get address range including "holes"
  if (image->numEntries > 0) {
    addrStart = image->memoryEntries[0].address;
    addrStop  = image->memoryEntries[image->numEntries-1].address;
  }
  else {
    addrStart = 0x01;   // make start>stop to skip below for loop
    addrStop  = 0x00;
  }

  // store every value in address range. Undefined values are set to 0x00
  countByte = 0;
  for (uint64_t address=addrStart; address<=addrStop; address++) {
    if (!MemoryImage_getData(image, address, &value))
      value = 0x00;
    fwrite(&value,sizeof(value), 1, fp); // write byte per byte (image is 16-bit)
    countByte++;
  }

  // close output file
  fflush(fp);
  fclose(fp);

  // print message
  if (verbose == SILENT){
    printf("done\n");
  }
  else if (verbose == INFORM) {
    if (countByte > 1024*1024)
      printf("done (%1.1fMB)\n", (float) countByte/1024.0/1024.0);
    else if (countByte > 1024)
      printf("done (%1.1fkB)\n", (float) countByte/1024.0);
    else if (countByte > 0)
      printf("done (%dB)\n", (int) countByte);
    else
      printf("done, no data\n");
  }
  else if (verbose == CHATTY) {
    if (countByte > 1024*1024)
      printf("done (%1.1fMB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) countByte/1024.0/1024.0, 
        (uint64_t) addrStart, (uint64_t) addrStop);
    else if (countByte > 1024)
      printf("done (%1.1fkB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (float) countByte/1024.0, 
        (uint64_t) addrStart, (uint64_t) addrStop);
    else if (countByte > 0)
      printf("done (%dB in [0x%" PRIX64 "; 0x%" PRIX64 "])\n", (int) countByte, 
        (uint64_t) addrStart, (uint64_t) addrStop);
    else
      printf("done, no data\n");
  }
  fflush(stdout);

} // export_file_bin



/**
  \fn void fill_image(MemoryImage_s *image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrStop, const uint8_t value, const uint8_t verbose)

  \param      image       pointer to memory image. Must be initialized. Existing content is overwritten
  \param[in]  addrStart   starting address of filling window
  \param[in]  addrStop    topmost address of filling window
  \param[in]  value       value to write
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Fill memory image in specified window with specified value
*/
void fill_image(MemoryImage_s *image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrStop, const uint8_t value, const uint8_t verbose) {

  uint64_t  numFilled = addrStop-addrStart+1;

  // print message
  if (verbose == INFORM)
    printf("  fill image ... ");
  else if (verbose == CHATTY)
    printf("  fill memory image ... ");
  fflush(stdout);

  // simple checks of address window
  if (addrStart > addrStop) {
    MemoryImage_free(image);
    Error("start address 0x%" PRIX64 " higher than end address 0x%" PRIX64, addrStart, addrStop);
  }

  // loop over memory image and fill all data inside specified range
  MemoryImage_fillValue(image, addrStart, addrStop, value);

  // print message
  if (verbose == INFORM) {
    printf("done\n");
  }
  else if (verbose == CHATTY) {
    if (numFilled>1024*1024)
      printf("done, filled %1.1fMB with 0x%02" PRIX8 " in [0x%" PRIX64 "; 0x%" PRIX64 "]\n", (float) numFilled/1024.0/1024.0, value, 
        (uint64_t) addrStart, (uint64_t) addrStop);
    else if (numFilled>1024)
      printf("done, filled %1.1fkB with 0x%02" PRIX8 " in [0x%" PRIX64 "; 0x%" PRIX64 "]\n", (float) numFilled/1024.0, value, 
        (uint64_t) addrStart, (uint64_t) addrStop);
    else if (numFilled>0)
      printf("done, filled %dB with 0x%02" PRIX8 " in [0x%" PRIX64 "; 0x%" PRIX64 "]\n", (int) numFilled, value, 
        (uint64_t) addrStart, (uint64_t) addrStop);
    else
      printf("done, no data filled\n");
  }
  fflush(stdout);

} // fill_image



/**
  \fn void fill_image_random(MemoryImage_s *image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrStop, const uint8_t verbose)

  \param      image       pointer to memory image. Must be initialized. Existing content is overwritten
  \param[in]  addrStart   starting address of filling window
  \param[in]  addrStop    topmost address of filling window
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Fill memory image in specified window with random values in 0..255
*/
void fill_image_random(MemoryImage_s *image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrStop, const uint8_t verbose) {

  uint64_t  numFilled = addrStop-addrStart+1;

  // print message
  if (verbose == INFORM)
    printf("  random fill image ... ");
  else if (verbose == CHATTY)
    printf("  random fill memory image ... ");
  fflush(stdout);

  // simple checks of address window
  if (addrStart > addrStop) {
    MemoryImage_free(image);
    Error("start address 0x%" PRIX64 " higher than end address 0x%" PRIX64, addrStart, addrStop);
  }

  // loop over memory image and fill all data inside specified range
  MemoryImage_fillRandom(image, addrStart, addrStop);

  // print message
  if (verbose == INFORM) {
    printf("done\n");
  }
  else if (verbose == CHATTY) {
    if (numFilled>1024*1024)
      printf("done, filled %1.1fMB in [0x%" PRIX64 "; 0x%" PRIX64 "]\n", (float) numFilled/1024.0/1024.0, 
        (uint64_t) addrStart, (uint64_t) addrStop);
    else if (numFilled>1024)
      printf("done, filled %1.1fkB in [0x%" PRIX64 "; 0x%" PRIX64 "]\n", (float) numFilled/1024.0, 
        (uint64_t) addrStart, (uint64_t) addrStop);
    else if (numFilled>0)
      printf("done, filled %dB in [0x%" PRIX64 "; 0x%" PRIX64 "]\n", (int) numFilled, 
        (uint64_t) addrStart, (uint64_t) addrStop);
    else
      printf("done, no data filled\n");
  }
  fflush(stdout);

} // fill_image_random



/**
  \fn void clip_image(MemoryImage_s *image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrStop, const uint8_t verbose)

  \param      image       pointer to memory image. Must be initialized. Existing content is overwritten
  \param[in]  addrStart   starting address of clipping window
  \param[in]  addrStop    topmost address of clipping window
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Clip memory image to specified window, i.e. delete all data outside specified window
*/
void clip_image(MemoryImage_s *image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrStop, const uint8_t verbose) {

  uint64_t  numStart = image->numEntries;
  
  // print message
  if (verbose == INFORM)
    printf("  clip image ... ");
  else if (verbose == CHATTY)
    printf("  clip memory image ... ");
  fflush(stdout);

  // simple checks of address window
  if (addrStart > addrStop) {
    MemoryImage_free(image);
    Error("start address 0x%" PRIX64 " higher than end address 0x%" PRIX64, addrStart, addrStop);
  }

  // clear all data outside specified window
  MemoryImage_clip(image, addrStart, addrStop);

  // print message
  if (verbose == INFORM) {
    printf("done\n");
  }
  else if (verbose == CHATTY) {
    uint64_t numCleared = numStart - image->numEntries;
    if (numCleared>1024*1024)
      printf("done, clipped %1.1fMB outside 0x%" PRIX64 " - 0x%" PRIX64 "\n", (float) numCleared/1024.0/1024.0, 
        (uint64_t) addrStart, (uint64_t) addrStop);
    else if (numCleared>1024)
      printf("done, clipped %1.1fkB outside 0x%" PRIX64 " - 0x%" PRIX64 "\n", (float) numCleared/1024.0, 
        (uint64_t) addrStart, (uint64_t) addrStop);
    else if (numCleared>0)
      printf("done, clipped %" PRId64 "B outside 0x%" PRIX64 " - 0x%" PRIX64 "\n", numCleared, 
        (uint64_t) addrStart, (uint64_t) addrStop);
    else
      printf("done, no data cleared\n");
  }
  fflush(stdout);

} // clip_image



/**
  \fn void cut_image(MemoryImage_s *image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrStop, const uint8_t verbose)

  \param      image       pointer to memory image. Must be initialized. Existing content is overwritten
  \param[in]  addrStart   starting address of section to clear
  \param[in]  addrStop    topmost address of section to clear
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Cut data range from memory image, i.e. delete all data inside specified window
*/
void cut_image(MemoryImage_s *image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrStop, const uint8_t verbose) {

  uint64_t  numStart = image->numEntries;

  // print message
  if (verbose == INFORM)
    printf("  cut image ... ");
  else if (verbose == CHATTY)
    printf("  cut memory image ... ");
  fflush(stdout);

  // simple checks of address window
  if (addrStart > addrStop) {
    MemoryImage_free(image);
    Error("start address 0x%" PRIX64 " higher than end address 0x%" PRIX64, addrStart, addrStop);
  }

  // clear all data inside specified window
  MemoryImage_cut(image, addrStart, addrStop);

  // print message
  if (verbose == INFORM) {
    printf("done\n");
  }
  else if (verbose == CHATTY) {
    uint64_t numCleared = numStart - image->numEntries;
    if (numCleared>1024*1024)
      printf("done, cut %1.1fMB within 0x%" PRIX64 " - 0x%" PRIX64 "\n", (float) numCleared/1024.0/1024.0, 
        (uint64_t) addrStart, (uint64_t) addrStop);
    else if (numCleared>1024)
      printf("done, cut %1.1fkB within 0x%" PRIX64 " - 0x%" PRIX64 "\n", (float) numCleared/1024.0, 
        (uint64_t) addrStart, (uint64_t) addrStop);
    else if (numCleared>0)
      printf("done, cut %" PRId64 "B within 0x%" PRIX64 " - 0x%" PRIX64 "\n", numCleared, 
        (uint64_t) addrStart, (uint64_t) addrStop);
    else
      printf("done, no data cut\n");
  }
  fflush(stdout);

} // cut_image



/**
  \fn void copy_image(MemoryImage_s *image, const MEMIMAGE_ADDR_T srcStart, const MEMIMAGE_ADDR_T srcStop, const MEMIMAGE_ADDR_T destStart, const uint8_t verbose)

  \param      image       pointer to memory image. Must be initialized. Existing content is overwritten
  \param[in]  srcStart    starting address to copy from
  \param[in]  srcStop     last address to copy from
  \param[in]  destStart   starting address to copy to
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Copy data section within image to new address. Data at old address is maintained (if sections don't overlap).
*/
void copy_image(MemoryImage_s *image, const MEMIMAGE_ADDR_T srcStart, const MEMIMAGE_ADDR_T srcStop, const MEMIMAGE_ADDR_T destStart, const uint8_t verbose) {

  // print message
  if (verbose == INFORM)
    printf("  copy data ... ");
  else if (verbose == CHATTY)
    printf("  copy image data ... ");
  fflush(stdout);

  // simple checks of address window
  if (srcStart > srcStop) {
    MemoryImage_free(image);
    Error("source start address 0x%" PRIX64 " higher than end address 0x%" PRIX64, srcStart, srcStop);
  }

  // copy data within image
  MemoryImage_copyRange(image, srcStart, srcStop, destStart);

  // print message
  if (verbose == INFORM) {
    printf("done\n");
  }
  else if (verbose == CHATTY) {
    uint64_t  numCopied = srcStop - srcStart + 1;
    if (numCopied>1024*1024)
      printf("done, copied %1.1fMB from 0x%" PRIX64 "-0x%" PRIX64 " to 0x%" PRIX64 "\n", (float) numCopied/1024.0/1024.0,
        (uint64_t) srcStart, (uint64_t) srcStop, (uint64_t) destStart);
    else if (numCopied>1024)
      printf("done, copied %1.1fkB from 0x%" PRIX64 "-0x%" PRIX64 " to 0x%" PRIX64 "\n", (float) numCopied/1024.0,
        (uint64_t) srcStart, (uint64_t) srcStop, (uint64_t) destStart);
    else if (numCopied>0)
      printf("done, copied %" PRId64 "B from 0x%" PRIX64 "-0x%" PRIX64 " to 0x%" PRIX64 "\n", numCopied,
        (uint64_t) srcStart, (uint64_t) srcStop, (uint64_t) destStart);
    else
      printf("done, no data copied\n");
  }
  fflush(stdout);

} // copy_image



/**
  \fn void move_image(MemoryImage_s *image, const MEMIMAGE_ADDR_T srcStart, const MEMIMAGE_ADDR_T srcStop, const MEMIMAGE_ADDR_T destStart, const uint8_t verbose)

  \param      image       pointer to memory image. Must be initialized. Existing content is overwritten
  \param[in]  srcStart    starting address to move from
  \param[in]  srcStop     last address to move from
  \param[in]  destStart   starting address to move to
  \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

  Move data section within image to new address. Data at old address is deleted.
*/
void move_image(MemoryImage_s *image, const MEMIMAGE_ADDR_T srcStart, const MEMIMAGE_ADDR_T srcStop, const MEMIMAGE_ADDR_T destStart, const uint8_t verbose) {

  // print message
  if (verbose == INFORM)
    printf("  move data ... ");
  else if (verbose == CHATTY)
    printf("  move image data ... ");
  fflush(stdout);

  // simple checks of address window
  if (srcStart > srcStop) {
    MemoryImage_free(image);
    Error("source start address 0x%" PRIX64 " higher than end address 0x%" PRIX64, srcStart, srcStop);
  }

  // move data within image
  MemoryImage_moveRange(image, srcStart, srcStop, destStart);

  // print message
  if (verbose == INFORM) {
    printf("done\n");
  }
  else if (verbose == CHATTY) {
    uint64_t  numMoved = srcStop - srcStart + 1;
    if (numMoved>1024*1024)
      printf("done, moved %1.1fkB from 0x%" PRIX64 "-0x%" PRIX64 " to 0x%" PRIX64 "\n", (float) numMoved/1024.0/1024.0,
        (uint64_t) srcStart, (uint64_t) srcStop, (uint64_t) destStart);
    else if (numMoved>1024)
      printf("done, moved %1.1fkB from 0x%" PRIX64 "-0x%" PRIX64 " to 0x%" PRIX64 "\n", (float) numMoved/1024.0,
        (uint64_t) srcStart, (uint64_t) srcStop, (uint64_t) destStart);
    else if (numMoved>0)
      printf("done, moved %" PRId64 "B from 0x%" PRIX64 "-0x%" PRIX64 " to 0x%" PRIX64 "\n", numMoved,
        (uint64_t) srcStart, (uint64_t) srcStop, (uint64_t) destStart);
    else
      printf("done, no data moved\n");
  }
  fflush(stdout);

} // move_image

// end of file
