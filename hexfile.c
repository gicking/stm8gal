/**
   \file hexfile.c
   
   \author G. Icking-Konert
   \date 2014-03-16
   \version 0.1
   
   \brief implementation of routines to import HEX and S19 files
   
   implementation of routines for importing and interpreting hexfiles in
   Motorola s19 and Intel hex format 
   (description under http://en.wikipedia.org/wiki/SREC_(file_format) or
   http://www.keil.com/support/docs/1584.htm). 
*/

// include files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <ctype.h>
#include "hexfile.h"
#include "misc.h"
#include "globals.h"



/**
   \fn char *get_line(char **buf, char *line)
   
   \brief read next line from RAM buffer
   
   \param[in]  buf        pointer to read from (is updated)
   \param[out] line       pointer to line read (has to be large anough)
   
   read line (until LF, CR, or EOF) from RAM buffer and advance buffer pointer.
   memory for line has to be allocated externally
*/
char *get_line(char **buf, char *line) {
  
  char  *p = line;
  
  // copy line
  while ((**buf!=10) && (**buf!=13) && (**buf!=0)) {
    *line = **buf;
    line++;
    (*buf)++;
  }
  
  // skip CR + LF in buffer
  while ((**buf==10) || (**buf==13))
    (*buf)++;
    
  // terminate line
  *line = '\0';
  
  // check if data was copied
  if (p == line)
    return(NULL);
  else
    return(p);
  
} // get_line

  

/**
   \fn load_hexfile(char *filename, char *buf)
   
   \brief read hexfile into memory buffer
   
   \param[in]  filename   name of hexfile to read
   \param[out] buf        memory buffer containing file content (0-terminated)
   \param[in]  bufsize    max size of memory buffer
   
   read hexfile from file to memory buffer. Don't interpret (is done
   in separate routine)
*/
void load_hexfile(const char *filename, char *buf, uint32_t bufsize) {

  FILE      *fp;
  uint32_t  len;
  
  // open file to read
  if (!(fp = fopen(filename, "rb")))
    Error("Failed to open file");
     
  // get filesize
  fseek(fp, 0, SEEK_END);
  len = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  // read file to buffer
  fread(buf, len, 1, fp);
  
  // close file again
  fclose(fp);

  // attach 0 to buffer to detect EOF
  buf[len++] = 0;
  
  // print message
  if (g_verbose) {
    if (len>2048)
      printf("ok (%1.1fkB)\n", (float) len/1024.0);
    else if (len>0)
      printf("ok (%dB)\n", len);
    else
      printf("ok, no data read\n");
    fflush(stdout);
  }

} // load_hexfile

  

/**
   \fn load_binfile(const char *filename, char *buf, uint32_t *addrStart, uint32_t *numBytes, uint32_t bufsize)
   
   \brief read binary into memory buffer
   
   \param[in]  filename   name of hexfile to read
   \param[out] buf        memory buffer containing file content (0-terminated)
   \param[out] addrStart  starting address (fixed @ PFLASH_START)
   \param[out] numBytes   number of read bytes
   \param[in]  bufsize    max size of memory buffer
   
   read hexfile from file to memory buffer. Don't interpret (is done
   in separate routine)
*/
void load_binfile(const char *filename, char *buf, uint32_t *addrStart, uint32_t *numBytes, uint32_t bufsize)
{
  FILE *fp;
  long len;

  if(!(fp = fopen(filename, "rb")))
    Error("Failed to open file");

  // Get filesize
  fseek(fp, 0, SEEK_END);
  len = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if(len > bufsize)
    Error("Binary file too large (%ld bytes)", len);

  fread(buf, len, 1, fp);
  fclose(fp);

  *addrStart = 0x8000;    // fixed for all STM8 devices
  *numBytes  = len;
  
  // print message
  if (g_verbose) {
    if (len>2048)
      printf("ok (%1.1fkB)\n", (float) len/1024.0);
    else if (len>0)
      printf("ok (%ldB)\n", len);
    else
      printf("ok, no data read\n");
    fflush(stdout);
  }

} // load_binfile



/**
   \fn void convert_s19(char *buf, uint32_t *addrStart, uint32_t *numBytes, char *image)
   
   \brief convert s19 format in memory buffer to memory image
   
   \param[in]  buf        memory buffer to read from (0-terminated)
   \param[out] addrStart  start address of image (=lowest address in hexfile)
   \param[out] numBytes   number of bytes in image
   \param[out] image      RAM image of hexfile
   
   convert memory buffer containing s19 hexfile to memory buffer. For description of 
   Motorola S19 file format see http://en.wikipedia.org/wiki/SREC_(file_format)
*/
void convert_s19(char *buf, uint32_t *addrStart, uint32_t *numBytes, char *image) {
  
  char      line[1000], tmp[1000], *p;
  int       linecount, idx, i;
  uint8_t   type, len, chkRead, chkCalc;
  uint32_t  addr, addrMin, addrMax, val;
  
  // print message (if present, strip path)
  /*
  printf("  convert s19 ... ");
  fflush(stdout);
  */
  
  // 1st run: check syntax and extract min/max addresses
  linecount = 0;
  addrMin = 0xFFFFFFFF;
  addrMax = 0x00000000;
  p = buf;
  while (get_line(&p, line)) {
  
    // increase line counter
    linecount++;
    chkCalc = 0x00;
    
    // check 1st char (must be 'S')
    if (line[0] != 'S')
      Error("Line %d of Motorola S-record file not start with 'S'", linecount);
    
    // record type
    type = line[1]-48;
    
    // skip if line contains no data
    if ((type==0) || (type==8) || (type==9))
      continue; 
    
    // record length (address + data + checksum)
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+2, 2);
    sscanf(tmp, "%x", &val);
    len = val;
    chkCalc += val;              // increase checksum
    
    // address (S1=16bit, S2=24bit, S3=32bit)
    addr = 0;
    for (i=0; i<type+1; i++) {
      sprintf(tmp,"0x00");
      tmp[2] = line[4+(i*2)];
      tmp[3] = line[5+(i*2)];
      sscanf(tmp, "%x", &val);
      addr *= 256;
      addr += val;    
      chkCalc += val;
    } 

    // read record data
    idx=6+(type*2);                // start at position 8, 10, or 12, depending on record type
    len=len-1-(1+type);            // substract chk and address length
    for (i=0; i<len; i++) {
      sprintf(tmp,"0x00");
      strncpy(tmp+2, line+idx, 2);    // get next 2 chars as string
      sscanf(tmp, "%x", &val);        // interpret as hex data
      chkCalc += val;                 // increase checksum
      idx+=2;                         // advance 2 chars in line
    }

    // checksum
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+idx, 2);
    sscanf(tmp, "%x", &val);
    chkRead = val;
    
    // assert checksum (0xFF xor (sum over all except record type)
    chkCalc ^= 0xFF;                 // invert checksum
    if (chkCalc != chkRead)
      Error("Checksum error in line %d of Motorola S-record file (0x%02x vs. 0x%02x)", linecount, chkRead, chkCalc);
    
    // store min/max address
    if (addr < addrMin)
      addrMin = addr;
    if (addr+len-1 > addrMax)
      addrMax = addr+len-1;
    
  } // while !EOF
      
  // store base address and image size
  *addrStart = addrMin;
  if ((addrMin != 0xFFFFFFFF) || (addrMax != 0x00000000))
    *numBytes  = addrMax-addrMin+1;
  else
    *numBytes  = 0;
       
  
  // 2nd run: store data to image
  if (*numBytes != 0) {
    p = buf;
    while (get_line(&p, line)) {
    
      // record type
      type = line[1]-48;
    
      // skip if line contains no data
      if ((type==0) || (type==8) || (type==9))
        continue; 
    
      // record length (address + data + checksum)
      sprintf(tmp,"0x00");
      strncpy(tmp+2, line+2, 2);
      sscanf(tmp, "%x", &val);
      len = val;
    
      // address (S1=16bit, S2=24bit, S3=32bit)
      addr = 0;
      for (i=0; i<type+1; i++) {
        sprintf(tmp,"0x00");
        strncpy(tmp+2, line+4+(i*2), 2);
        sscanf(tmp, "%x", &val);
        addr *= 256;
        addr += val;    
      }
    
      // read record data
      idx=6+(type*2);                // start at position 8, 10, or 12, depending on record type
      len=len-1-(1+type);            // substract chk and address length
      for (i=0; i<len; i++) {
        sprintf(tmp,"0x00");
        strncpy(tmp+2, line+idx, 2);    // get next 2 chars as string
        sscanf(tmp, "%x", &val);        // interpret as hex data
        image[addr-addrMin+i] = val;     // store data byte in buffer
        idx+=2;                         // advance 2 chars in line
      }
    
    } // while !EOF
    
  } // if numBytes!=0
  
  /*
  printf("\n");
  for (i=0; i<(*numBytes); i++)
    printf("%3d   0x%04x   0x%02x\n", i, addrMin+i, (int) (image[i]) & 0xFF);
  printf("\n");
  Exit(1,1);      
  */
  
  // print message
  /*
  if ((*numBytes)>2048)
    printf("ok (%1.1fkB @ 0x%04x)\n", (float) (*numBytes)/1024.0, addrMin);
  else if ((*numBytes)>0)
    printf("ok (%dB @ 0x%04x)\n", *numBytes, addrMin);
  else
    printf("ok, no data read\n");
  fflush(stdout);
  */
  
} // convert_s19

  

/**
   \fn void convert_hex(char *buf, uint32_t *addrStart, uint32_t *numBytes, char *image)
   
   \brief convert intel hex format in memory buffer to memory image
   
   \param[in]  buf        memory buffer to read from (0-terminated)
   \param[out] addrStart  start address of image (=lowest address in hexfile)
   \param[out] numBytes   number of bytes in image
   \param[out] image      RAM image of hexfile
   
   convert memory buffer containing intel hexfile to memory buffer. For description of 
   Intel hex file format see http://en.wikipedia.org/wiki/Intel_HEX
*/
void convert_hex(char *buf, uint32_t *addrStart, uint32_t *numBytes, char *image) {
  
  char      line[1000], tmp[1000], *p;
  int       linecount, idx, i;
  uint8_t   type, len, chkRead, chkCalc;
  uint32_t  addr, addrMin, addrMax, addrOff, addrJumpStart, val;
  
  // print message (if present, strip path)
  /*
  printf("  convert hex ... ");
  fflush(stdout);
  */
  
  // 1st run: check syntax and extract min/max addresses
  linecount = 0;
  addrMin = 0xFFFFFFFF;
  addrMax = 0x00000000;
  addrOff = 0x00000000;
  p = buf;
  while (get_line(&p, line)) {
  
    // increase line counter
    linecount++;
    chkCalc = 0x00;
    
    // check 1st char (must be ':')
    if (line[0] != ':')
      Error("Line %d of Intel hex file not start with ':'", linecount);
    
    // record length (address + data + checksum)
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+1, 2);
    sscanf(tmp, "%x", &val);
    len = val;
    chkCalc += len;              // increase checksum
    
    // 16b address
    addr = 0;
    sprintf(tmp,"0x0000");
    strncpy(tmp+2, line+3, 4);
    sscanf(tmp, "%x", &val);
    chkCalc += (uint8_t) (val >> 8);
    chkCalc += (uint8_t)  val;
    addr = val + addrOff;         // add offset for >64kB addresses

    // record type
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+7, 2);
    sscanf(tmp, "%x", &val);
    type = val;
    chkCalc += type;              // increase checksum
    
    // record contains data
    if (type==0) {
      idx = 9;                          // start at index 9
      for (i=0; i<len; i++) {
        sprintf(tmp,"0x00");
        strncpy(tmp+2, line+idx, 2);    // get next 2 chars as string
        sscanf(tmp, "%x", &val);        // interpret as hex data
        chkCalc += val;                 // increase checksum
        idx+=2;                         // advance 2 chars in line
      }
    } // type==0

    // EOF indicator
    else if (type==1)
      continue; 
    
    // extended address (=upper 16b of address for following data records)
    else if (type==4) {
      sprintf(tmp,"0x0000");
      strncpy(tmp+2, line+9, 4);        // get next 4 chars as string
      sscanf(tmp, "%x", &val);        // interpret as hex data
      chkCalc += (uint8_t) (val >> 8);
      chkCalc += (uint8_t)  val;
      addrOff = val << 16;
    } // type==4
    
    // start linear address records. Can be ignored, see http://www.keil.com/support/docs/1584/
    else if (type==5) {
      sprintf(tmp,"0x00000000");
      strncpy(tmp+2, line+9, 8);      // get next 8 chars as string
      sscanf(tmp, "%x", &val);        // interpret as hex data
      chkCalc += (uint8_t) (val >> 24);
      chkCalc += (uint8_t) (val >> 16);
      chkCalc += (uint8_t) (val >> 8);
      chkCalc += (uint8_t)  val;
      addrJumpStart = val;            // not used yet
    } // type==5
    
    // unsupported record type -> error
    else
      Error("Line %d of Intel hex file has unsupported type %d", linecount, type);
    
    
    // checksum
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+idx, 2);
    sscanf(tmp, "%x", &val);
    chkRead = val;
    
    // assert checksum (0xFF xor (sum over all except record type)
    chkCalc = 255 - chkCalc + 1;                 // calculate 2-complement
    if (chkCalc != chkRead)
      Error("Line %d of Intel hex file has wrong checksum (0x%02x vs. 0x%02x)", linecount, chkRead, chkCalc);
    
    // store min/max address
    if (addr < addrMin)
      addrMin = addr;
    if (addr+len-1 > addrMax)
      addrMax = addr+len-1;
    
  } // while !EOF
    
  // store base address and image size
  *addrStart = addrMin;
  if ((addrMin != 0xFFFFFFFF) || (addrMax != 0x00000000))
    *numBytes  = addrMax-addrMin+1;
  else
    *numBytes  = 0;
       
  
  // 2nd run: store data to image
  addrOff = 0x00000000;
  if (*numBytes != 0) {
    p = buf;
    while (get_line(&p, line)) {
    
      // record length (address + data + checksum)
      sprintf(tmp,"0x00");
      strncpy(tmp+2, line+1, 2);
      sscanf(tmp, "%x", &val);
      len = val;
      
      // 16b address
      addr = 0;
      sprintf(tmp,"0x0000");
      strncpy(tmp+2, line+3, 4);
      sscanf(tmp, "%x", &val);
      addr = val;         // add offset for >64kB addresses

      // record type
      sprintf(tmp,"0x00");
      strncpy(tmp+2, line+7, 2);
      sscanf(tmp, "%x", &val);
      type = val;
      
      // record contains data
      if (type==0) {
        idx = 9;                          // start at index 9
        for (i=0; i<len; i++) {
          sprintf(tmp,"0x00");
          strncpy(tmp+2, line+idx, 2);          // get next 2 chars as string
          sscanf(tmp, "%x", &val);              // interpret as hex data
          image[addr+addrOff-addrMin+i] = val;  // store data byte in buffer
          idx+=2;                               // advance 2 chars in line
        }
      } // type==0

      // EOF indicator
      else if (type==1)
        continue; 
    
      // extended address (=upper 16b of address for following data records)
      else if (type==4) {
        sprintf(tmp,"0x0000");
        strncpy(tmp+2, line+9, 4);        // get next 4 chars as string
        sscanf(tmp, "%x", &val);        // interpret as hex data
        addrOff = val << 16;
      } // type==4
    
      else
        Error("Line %d of Intel hex file has unsupported type %d", linecount, type);
    
    } // while !EOF
    
  } // if numBytes!=0
  
  /*
  printf("\n");
  for (i=0; i<(*numBytes); i++)
    printf("%3d   0x%04x   0x%02x\n", i, addrMin+i, (int) (image[i]) & 0xFF);
  printf("\n");
  Exit(1,1);      
  */
  
  // print message
  /*
  if ((*numBytes)>2048)
    printf("ok (%1.1fkB @ 0x%04x)\n", (float) (*numBytes)/1024.0, addrMin);
  else if ((*numBytes)>0)
    printf("ok (%dB @ 0x%04x)\n", *numBytes, addrMin);
  else
    printf("ok, no data read\n");
  fflush(stdout);
  */
  
} // convert_hex

  

/**
   \fn void export_s19(char *outfile, char *buf, uint32_t addrStart, uint32_t numBytes)
   
   \brief export RAM image to file in Motorola s19 format
   
   \param[in]  outfile    filename to output to
   \param[in]  buf        RAM image to save
   \param[in]  addrStart  first address to export
   \param[in]  numBytes   number of bytes to export
   
   export RAM image to file in s19 hexfile format. For description of 
   Motorola S19 file format see http://en.wikipedia.org/wiki/SREC_(file_format)
*/
void export_s19(char *outfile, char *buf, uint32_t addrStart, uint32_t numBytes) {

  int       i, j;                 // loop variables
  int       lenLine;
  uint8_t   data;
  uint32_t  chk;                  // checksum
  FILE      *fp;                  // file pointer

  // open output file
  fp=fopen(outfile,"w");
  if (!fp) {
    fprintf(stderr, "\n\nerror in 'export_s19()': cannot create file '%s', exit!\n\n", outfile);
    Exit(1, g_pauseOnExit);
  }

  // store in lines of 32B
  lenLine = 32;
  for (i=0; i<numBytes; i+=lenLine) {

    // save line (see http://en.wikipedia.org/wiki/SREC_(file_format) )
    fprintf(fp, "S1%02X%04X", lenLine+3, addrStart+i); // 2B addr + data + 1B chk
    chk = (uint8_t) (lenLine+3) + (uint8_t) (addrStart+i) + (uint8_t) ((addrStart+i) >> 8);
    for (j=0; (j<lenLine) && (j<numBytes); j++) {
      data = (uint8_t) (buf[i+j]);
      chk += data;
      fprintf(fp, "%02X", data);
    }
    chk = ((chk & 0xFF) ^ 0xFF);
    fprintf(fp, "%02X\n", chk);

  } // loop over lines

  // attach generic EOF line
  fprintf(fp, "S903FFFFFE\n");
  
  // close output file
  fflush(fp);
  fclose(fp);

} // export_s19

  

/**
   \fn void export_hex(char *outfile, char *buf, uint32_t addrStart, uint32_t numBytes)
   
   \brief export RAM image to file in Intel hex format
   
   \param[in]  outfile    filename to output to
   \param[in]  buf        RAM image to save
   \param[in]  addrStart  first address to export
   \param[in]  numBytes   number of bytes to export
   
   export RAM image to file in intel hexfile format. For description of 
   Intel hex file format see http://en.wikipedia.org/wiki/Intel_HEX
*/
void export_hex(char *outfile, char *buf, uint32_t addrStart, uint32_t numBytes) {

  // to be done
  fprintf(stderr, "\n\nerror in 'export_hex()': Intel Hex output not yet supported, exit!\n\n");
  Exit(1, g_pauseOnExit);

} // export_hex

  

/**
   \fn void export_txt(char *outfile, char *buf, uint32_t addrStart, uint32_t numBytes)
   
   \brief export RAM image to file with plain text table (hex addr / data)
   
   \param[in]  outfile    filename to output to
   \param[in]  buf        RAM image to save
   \param[in]  addrStart  first address to export
   \param[in]  numBytes   number of bytes to export
   
   export RAM image to file with plain text table (hex addr / data)
*/
void export_txt(char *outfile, char *buf, uint32_t addrStart, uint32_t numBytes) {

  int       i;                 // index
  FILE      *fp;               // file pointer

  // open output file
  fp=fopen(outfile,"w");
  if (!fp) {
    fprintf(stderr, "\n\nerror in 'export_s19()': cannot create file '%s', exit!\n\n", outfile);
    Exit(1, g_pauseOnExit);
  }

  // store each value in separate line (addr value)
  for (i=0; i<numBytes; i++) {
    fprintf(fp, "0x%04x	0x%02x\n", (int) (addrStart+i), (uint8_t) (buf[i]));
  }
  
  // close output file
  fflush(fp);
  fclose(fp);

} // export_txt

// end of file
