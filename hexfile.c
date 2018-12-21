/**
   \file hexfile.c
   
   \author G. Icking-Konert
   \date 2018-12-14
   \version 0.2
   
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
#include <ctype.h>
#include "hexfile.h"
#include "main.h"
#include "misc.h"


/**
   \fn char *get_line(char **buf, char *line)
   
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
   \fn void load_file(const char *filename, char *fileBuf, uint32_t *lenFileBuf, uint8_t verbose)
   
   \param[in]  filename     name of file to read
   \param[out] fileBuf      memory buffer containing file content
   \param[out] lenFileBuf   size of data [B] read from file
   \param[in]  verbose      verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)
   
   read file from file to memory buffer. Don't interpret (is done in separate routine)
*/
void load_file(const char *filename, char *fileBuf, uint32_t *lenFileBuf, uint8_t verbose) {

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
  if (verbose >= SILENT)
    printf("  load '%s' ... ", shortname);
  fflush(stdout);

  // open file to read
  if (!(fp = fopen(filename, "rb")))
    Error("Failed to open file %s", filename);
     
  // get filesize
  fseek(fp, 0, SEEK_END);
  (*lenFileBuf) = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  // check file size vs. buffer
  if ((*lenFileBuf) > LENFILEBUF)
    Error("File %s exceeded buffer size (%d vs %d)", (*lenFileBuf), LENFILEBUF);

  // init memory image to zero
  memset(fileBuf, 0, LENFILEBUF * sizeof(*fileBuf));

  // read file to buffer
  fread(fileBuf, (*lenFileBuf), 1, fp);
  
  // close file again
  fclose(fp);

  // print message
  if ((verbose == SILENT) || (verbose == INFORM)){
    printf("done\n");
  }
  else if (verbose == CHATTY) {
    if ((*lenFileBuf)>2048)
      printf("done (%1.1fkB)\n", (float) (*lenFileBuf)/1024.0);
    else if ((*lenFileBuf)>0)
      printf("done (%dB)\n", (*lenFileBuf));
    else
      printf("done, no data read\n");
  }
  fflush(stdout);

} // load_file



/**
   \fn void convert_s19(char *fileBuf, uint32_t lenFileBuf, uint16_t *imageBuf, uint32_t *addrStart, uint32_t *addrStop, uint8_t verbose)
   
   \param[in]  fileBuf      memory buffer to read from (0-terminated)
   \param[in]  lenFileBuf   length of memory buffer
   \param[out] imageBuf     RAM image of file. HB!=0 indicates content. Index 0 corresponds to addrStart
   \param[out] addrStart    start address of image (= lowest address in file)
   \param[out] addrStop     last address of image (= highest address in file)
   \param[in]  verbose      verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)
   
   convert memory buffer containing s19 hexfile to memory buffer. For description of 
   Motorola S19 file format see http://en.wikipedia.org/wiki/SREC_(file_format)
*/
void convert_s19(char *fileBuf, uint32_t lenFileBuf, uint16_t *imageBuf, uint32_t *addrStart, uint32_t *addrStop, uint8_t verbose) {
  
  char      line[1000], tmp[1000], *p;
  int       linecount, idx;
  uint8_t   type, len, chkRead, chkCalc;
  uint32_t  addr, val, numData;
  
  // print message
  if (verbose == INFORM)
    printf("  convert S19 ... ");
  else if (verbose == CHATTY)
    printf("  convert Motorola S19 file ... ");
  fflush(stdout);
  
  // init memory image to zero
  memset(imageBuf, 0, LENIMAGEBUF * sizeof(*imageBuf));


  //////
  // 1st run: check syntax and extract min/max addresses
  //////
  linecount = 0;
  *addrStart = 0xFFFFFFFF;
  *addrStop  = 0x00000000;
  p = fileBuf;
  while ((uint32_t) (p-fileBuf) < lenFileBuf) {
  
    // get next line. On EOF terminate
    if (!get_line(&p, line))
      break;

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
    for (int i=0; i<type+1; i++) {
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
    for (int i=0; i<len; i++) {
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
    if (addr < *addrStart)
      *addrStart = addr;
    if (addr+len-1 > *addrStop)
      *addrStop = addr+len-1;
    
  } // while !EOF
    


  //////
  // file contains data --> 2nd run: store data to image
  //////
  if (*addrStart <= *addrStop) {

    // check for buffer overflow
    if ((*addrStop)-(*addrStart)+1  > LENIMAGEBUF)
      Error("Buffer size exceeded (%d vs %d)", (*addrStop)-(*addrStart)+1, LENIMAGEBUF);
       
    p = fileBuf;
    numData = 0;
    while ((uint32_t) (p-fileBuf) < lenFileBuf) {
    
      // get next line. On EOF terminate
      if (!get_line(&p, line))
        break;

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
      for (int i=0; i<type+1; i++) {
        sprintf(tmp,"0x00");
        strncpy(tmp+2, line+4+(i*2), 2);
        sscanf(tmp, "%x", &val);
        addr *= 256;
        addr += val;    
      }
    
      // read record data
      idx=6+(type*2);                // start at position 8, 10, or 12, depending on record type
      len=len-1-(1+type);            // substract chk and address length
      for (int i=0; i<len; i++) {
        sprintf(tmp,"0x00");
        strncpy(tmp+2, line+idx, 2);    // get next 2 chars as string
        sscanf(tmp, "%x", &val);        // interpret as hex data
        imageBuf[addr-(*addrStart)+i] = val | 0xFF00;    // store data byte in buffer and set high byte
        idx+=2;                         // advance 2 chars in line
        numData++;                      // increade byte counter
      }
    
    } // while !EOF
    
  } // if numBytes!=0
  
  // debug: print memory image
  /*
  printf("\n\n");
  printf("addr: 0x%04X..0x%04X   %d\n", *addrStart, *addrStop, numData);
  for (int i=0; i<(*addrStop)-(*addrStart)+1; i++) {
    if (imageBuf[i])
      printf(" %3d   0x%04x   0x%02x\n", i, (*addrStart)+i, (int) (imageBuf[i]) & 0xFF);
  }
  printf("\n");
  //Exit(1,0);
  */

  // print message
  if (verbose == INFORM) {
    printf("done\n");
  }
  else if (verbose == CHATTY) {
    if (numData>2048)
      printf("done (%1.1fkB @ 0x%04x - 0x%04x)\n", (float) numData/1024.0, (*addrStart), (*addrStop));
    else if (numData>0)
      printf("done (%dB @ 0x%04x - 0x%04x)\n", numData, (*addrStart), (*addrStop));
    else
      printf("done, no data\n");
  }
  fflush(stdout);
  
} // convert_s19

  

/**
   \fn void convert_ihx(char *fileBuf, uint32_t lenFileBuf, uint16_t *imageBuf, uint32_t *addrStart, uint32_t *addrStop, uint8_t verbose)
   
   \param[in]  fileBuf      memory buffer to read from (0-terminated)
   \param[in]  lenFileBuf   length of memory buffer
   \param[out] imageBuf     RAM image of file. HB!=0 indicates content. Index 0 corresponds to addrStart
   \param[out] addrStart    start address of image (= lowest address in file)
   \param[out] addrStop     last address of image (= highest address in file)
   \param[in]  verbose      verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   convert memory buffer containing intel hexfile to memory buffer. For description of 
   Intel hex file format see http://en.wikipedia.org/wiki/Intel_HEX
*/
void convert_ihx(char *fileBuf, uint32_t lenFileBuf, uint16_t *imageBuf, uint32_t *addrStart, uint32_t *addrStop, uint8_t verbose) {
  
  char      line[1000], tmp[1000], *p;
  int       linecount, idx;
  uint8_t   type, len, chkRead, chkCalc;
  uint32_t  addr, val, numData;
  uint32_t  addrOff, addrJumpStart;

  // avoid compiler warning (variable not yet used). See https://stackoverflow.com/questions/3599160/unused-parameter-warnings-in-c
  (void) (addrJumpStart);

  // print message
  if (verbose == INFORM)
    printf("  convert IHX ... ");
  else if (verbose == CHATTY)
    printf("  convert Intel HEX file ... ");
  fflush(stdout);
  
  // init memory image to zero
  memset(imageBuf, 0, LENIMAGEBUF * sizeof(*imageBuf));


  //////
  // 1st run: check syntax and extract min/max addresses
  //////
  linecount = 0;
  *addrStart = 0xFFFFFFFF;
  *addrStop  = 0x00000000;
  addrOff = 0x00000000;
  p = fileBuf;
  while ((uint32_t) (p-fileBuf) < lenFileBuf) {
  
    // get next line. On EOF terminate
    if (!get_line(&p, line))
      break;

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
      for (int i=0; i<len; i++) {
        sprintf(tmp,"0x00");
        strncpy(tmp+2, line+idx, 2);    // get next 2 chars as string
        sscanf(tmp, "%x", &val);        // interpret as hex data
        chkCalc += val;                 // increase checksum
        idx+=2;                         // advance 2 chars in line
      }
    
      // store min/max address
      if (addr < *addrStart)
        *addrStart = addr;
      if (addr+len-1 > *addrStop)
        *addrStop = addr+len-1;

    } // type==0

    // EOF indicator
    else if (type==1)
      continue; 

    // extended segment addresses not yet supported
    else if (type==2) 
      Error("Line %d of Intel hex file: extended segment address type 2 not supported", linecount);
    
    // start segment address (only relevant for 80x86 processors, ignore here) 
    else if (type==3)
      continue;
    
    // extended address (=upper 16b of address for following data records)
    else if (type==4) {
      idx = 13;                       // start at index 13
      sprintf(tmp,"0x0000");
      strncpy(tmp+2, line+9, 4);      // get next 4 chars as string
      sscanf(tmp, "%x", &val);        // interpret as hex data
      chkCalc += (uint8_t) (val >> 8);
      chkCalc += (uint8_t)  val;
      addrOff = val << 16;
    } // type==4
    
    // start linear address records. Can be ignored, see http://www.keil.com/support/docs/1584/
    else if (type==5)
      continue; 
    
    // unsupported record type -> error
    else
      Error("Line %d of Intel hex file has unsupported type %d", linecount, type);
    
    
    // checksum
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+idx, 2);
    sscanf(tmp, "%x", &val);
    chkRead = val;
    
    // assert checksum (0xFF xor (sum over all except record type))
    chkCalc = 255 - chkCalc + 1;                 // calculate 2-complement
    if (chkCalc != chkRead)
      Error("Line %d of Intel hex file has wrong checksum (0x%02x vs. 0x%02x)", linecount, chkRead, chkCalc);
    
  } // while !EOF
    


  //////
  // file contains data --> 2nd run: store data to image
  //////
  if (*addrStart <= *addrStop) {

    // check for buffer overflow
    if ((*addrStop)-(*addrStart)+1  > LENIMAGEBUF)
      Error("Buffer size exceeded (%d vs %d)", (*addrStop)-(*addrStart)+1, LENIMAGEBUF);
       
    p = fileBuf;
    numData = 0;
    while ((uint32_t) (p-fileBuf) < lenFileBuf) {
    
      // get next line. On EOF terminate
      if (!get_line(&p, line))
        break;

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
        for (int i=0; i<len; i++) {
          sprintf(tmp,"0x00");
          strncpy(tmp+2, line+idx, 2);          // get next 2 chars as string
          sscanf(tmp, "%x", &val);              // interpret as hex data
          imageBuf[addr+addrOff-(*addrStart)+i] = val | 0xFF00;    // store data byte in buffer and set high byte
          idx+=2;                               // advance 2 chars in line
        }
      } // type==0

      // EOF indicator
      else if (type==1)
        continue; 
    
      // extended segment addresses not yet supported
      else if (type==2) 
        Error("Line %d of Intel hex file: extended segment address type 2 not supported", linecount);
    
      // start segment address (only relevant for 80x86 processors, ignore here) 
      else if (type==3)
        continue;

      // extended address (=upper 16b of address for following data records)
      else if (type==4) {
        sprintf(tmp,"0x0000");
        strncpy(tmp+2, line+9, 4);        // get next 4 chars as string
        sscanf(tmp, "%x", &val);        // interpret as hex data
        addrOff = val << 16;
      } // type==4
    
      // start address  -> ignore
      else if (type==5)
        continue; 
    
      else
        Error("Line %d of Intel hex file has unsupported type %d", linecount, type);
    
    } // while !EOF
    
  } // if numBytes!=0
  
  // debug: print memory image
  /*
  printf("\n\n");
  printf("addr: 0x%04X..0x%04X   %d\n", *addrStart, *addrStop, numData);
  for (int i=0; i<(*addrStop)-(*addrStart)+1; i++) {
    if (imageBuf[i])
      printf(" %3d   0x%04x   0x%02x\n", i, (*addrStart)+i, (int) (imageBuf[i]) & 0xFF);
  }
  printf("\n");
  //Exit(1,0);
  */

  // print message
  if (verbose == INFORM) {
    printf("done\n");
  }
  else if (verbose == CHATTY) {
    if (numData>2048)
      printf("done (%1.1fkB @ 0x%04x - 0x%04x)\n", (float) numData/1024.0, (*addrStart), (*addrStop));
    else if (numData>0)
      printf("done (%dB @ 0x%04x - 0x%04x)\n", numData, (*addrStart), (*addrStop));
    else
      printf("done, no data\n");
  }
  fflush(stdout);
  
} // convert_ihx

  

/**
   \fn void convert_txt(char *fileBuf, uint32_t lenFileBuf, uint16_t *imageBuf, uint32_t *addrStart, uint32_t *addrStop, uint8_t verbose)
   
   \param[in]  fileBuf      memory buffer to read from (0-terminated)
   \param[in]  lenFileBuf   length of memory buffer
   \param[out] imageBuf     RAM image of file. HB!=0 indicates content. Index 0 corresponds to addrStart
   \param[out] addrStart    start address of image (= lowest address in file)
   \param[out] addrStop     last address of image (= highest address in file)
   \param[in]  verbose      verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   convert memory buffer containing plain table (hex address / value) to memory buffer
*/
void convert_txt(char *fileBuf, uint32_t lenFileBuf, uint16_t *imageBuf, uint32_t *addrStart, uint32_t *addrStop, uint8_t verbose) {
  
  char      line[1000], *p;
  char      *p2, s[1000];
  int       linecount, val;
  uint32_t  addr, numData;
  
  // print message
  if (verbose == INFORM)
    printf("  convert table ... ");
  else if (verbose == CHATTY)
    printf("  convert ASCII table file ... ");
  fflush(stdout);
  
  // init memory image to zero
  memset(imageBuf, 0, LENIMAGEBUF * sizeof(*imageBuf));


  //////
  // 1st run: check syntax and extract min/max addresses
  //////
  linecount = 0;
  *addrStart = 0xFFFFFFFF;
  *addrStop  = 0x00000000;
  p = fileBuf;
  while ((uint32_t) (p-fileBuf) < lenFileBuf) {
  
    // get next line. On EOF terminate
    if (!get_line(&p, line))
      break;

    // increase line counter
    linecount++;
    
    // peek for comment and whether data is hex or dec
    p2 = line;
    if (p2[0] == '#')
      continue;
    sscanf(p2, "%x %s", &addr, s);
    
    // read address & data
    if ((s[0] == '0') && ((s[1] == 'x') || (s[1] == 'X'))) {
      sscanf(line, "%x %x", &addr, &val);
    }
    else {
      sscanf(line, "%x %d", &addr, &val);
    }

    // store min/max address
    if (addr < *addrStart)
      *addrStart = addr;
    if (addr > *addrStop)
      *addrStop = addr;
    
  } // while !EOF
    


  //////
  // file contains data --> 2nd run: store data to image
  //////
  if (*addrStart <= *addrStop) {

    // check for buffer overflow
    if ((*addrStop)-(*addrStart)+1  > LENIMAGEBUF)
      Error("Buffer size exceeded (%d vs %d)", (*addrStop)-(*addrStart)+1, LENIMAGEBUF);
       
    p = fileBuf;
    numData = 0;
    while ((uint32_t) (p-fileBuf) < lenFileBuf) {
    
      // get next line. On EOF terminate
      if (!get_line(&p, line))
        break;

      // peek whether data is hex or dec
      p2 = line;
      if (p2[0] == '#')
        continue;
      sscanf(p2, "%x %s", &addr, s);
    
      // read address & data
      if ((s[0] == '0') && ((s[1] == 'x') || (s[1] == 'X'))) {
        sscanf(line, "%x %x", &addr, &val);
      }
      else {
        sscanf(line, "%x %d", &addr, &val);
      }

      // store data byte in buffer and set high byte
      imageBuf[addr-(*addrStart)] = (uint16_t) val | 0xFF00;   
      numData++; 
    
    } // while !EOF
    
  } // if numBytes!=0

  // debug: print memory image
  /*
  printf("\n\n");
  printf("addr: 0x%04X..0x%04X   %d\n", *addrStart, *addrStop, numData);
  for (int i=0; i<(*addrStop)-(*addrStart)+1; i++) {
    if (imageBuf[i])
      printf(" %3d   0x%04x   0x%02x\n", i, (*addrStart)+i, (int) (imageBuf[i]) & 0xFF);
  }
  printf("\n");
  //Exit(1,0);
  */

  // print message
  if (verbose == INFORM) {
    printf("done\n");
  }
  else if (verbose == CHATTY) {
    if (numData>2048)
      printf("done (%1.1fkB @ 0x%04x - 0x%04x)\n", (float) numData/1024.0, (*addrStart), (*addrStop));
    else if (numData>0)
      printf("done (%dB @ 0x%04x - 0x%04x)\n", numData, (*addrStart), (*addrStop));
    else
      printf("done, no data\n");
  }
  fflush(stdout);
 
} // convert_txt

  

/**
   \fn void convert_bin(char *fileBuf, uint32_t lenFileBuf, uint16_t *imageBuf, uint32_t addrStart, uint32_t *addrStop, uint8_t verbose)
   
   \param[in]  fileBuf      memory buffer to read from (0-terminated)
   \param[in]  lenFileBuf   length of memory buffer
   \param[out] imageBuf     RAM image of file. HB!=0 indicates content. Index 0 corresponds to addrStart
   \param[in]  addrStart    start address of image (= lowest address)
   \param[out] addrStop     last address of image (= highest address)
   \param[in]  verbose      verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   convert memory buffer containing plain table (hex address / value) to memory buffer
*/
void convert_bin(char *fileBuf, uint32_t lenFileBuf, uint16_t *imageBuf, uint32_t addrStart, uint32_t *addrStop, uint8_t verbose) {
  
  uint32_t  numData;
  
  // print message
  if (verbose == INFORM)
    printf("  convert binary ... ");
  else if (verbose == CHATTY)
    printf("  convert binary data ... ");
  fflush(stdout);
  
  // init memory image to zero
  memset(imageBuf, 0, LENIMAGEBUF * sizeof(*imageBuf));

  // set last address. Note starting address is input parameter!
  numData = lenFileBuf;
  *addrStop = addrStart + numData;

  // copy data and mark as set (HB=0xFF)
  for (int i=0; i<numData; i++) {
    imageBuf[i] = ((uint16_t) fileBuf[i]) | 0xFF00;
  }

  // debug: print memory image
  /*
  printf("\n\n");
  printf("addr: 0x%04X..0x%04X   %d\n", addrStart, *addrStop, numData);
  for (int i=0; i<(*addrStop)-addrStart+1; i++) {
    if (imageBuf[i])
      printf(" %3d   0x%04x   0x%02x\n", i, addrStart+i, (int) (imageBuf[i]) & 0xFF);
  }
  printf("\n");
  //Exit(1,0);
  */

  // print message
  if (verbose == INFORM) {
    printf("done\n");
  }
  else if (verbose == CHATTY) {
    if (numData>2048)
      printf("done (%1.1fkB @ 0x%04x - 0x%04x)\n", (float) numData/1024.0, addrStart, (*addrStop));
    else if (numData>0)
      printf("done (%dB @ 0x%04x - 0x%04x)\n", numData, addrStart, (*addrStop));
    else
      printf("done, no data\n");
  }
  fflush(stdout);
 
} // convert_bin

  

/**
   \fn clip_image(uint16_t *imageBuf, uint32_t imageStart, uint32_t clipStart, uint32_t clipStop, uint8_t verbose)
   
   \param      imageBuf     memory image containing data. HB!=0 indicates content. Index 0 corresponds to imageStart
   \param[in]  imageStart   start address of image (= lowest address)
   \param[in]  clipStart    starting address of clipping window
   \param[in]  clipStop     topmost address of clipping window
   \param[in]  verbose      verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   Clip memory image to specified window, i.e. reset all data outside specified window to "undefined" (HB=0x00)
*/
void clip_image(uint16_t *imageBuf, uint32_t imageStart, uint32_t clipStart, uint32_t clipStop, uint8_t verbose) {
  
  uint32_t  numCleared;

  // print message
  if (verbose == INFORM)
    printf("  clip image ... ");
  else if (verbose == CHATTY)
    printf("  clip memory image to 0x%04x - 0x%04x ... ", clipStart, clipStop);
  fflush(stdout);
  
  // loop over memory image and clear all data outside specified clipping window
  numCleared = 0;
  for (int i=0; i<LENIMAGEBUF; i++) {
    if ((i+imageStart < clipStart) || (i+imageStart > clipStop)) {
      if (imageBuf[i] & 0xFF00)
         numCleared++;               // count deleted bytes for output below
      imageBuf[i] = 0x0000;          // HB=0x00 indicates data undefined, LB contains data
    }
  }

  // debug: print memory image
  /*
  printf("\n\n");
  printf("addr: 0x%04X..0x%04X   %d\n", addrStart, *addrStop, numData);
  for (int i=0; i<(*addrStop)-addrStart+1; i++) {
    if (imageBuf[i])
      printf(" %3d   0x%04x   0x%02x\n", i, addrStart+i, (int) (imageBuf[i]) & 0xFF);
  }
  printf("\n");
  //Exit(1,0);
  */

  // print message
  if (verbose == INFORM) {
    printf("done\n");
  }
  else if (verbose == CHATTY) {
    if (numCleared>2048)
      printf("done, cleared %1.1fkB\n", (float) numCleared/1024.0);
    else if (numCleared>0)
      printf("done, cleared %dB\n", numCleared);
    else
      printf("done, no data cleared\n");
  }
  fflush(stdout);
 
} // clip_image

  

/**
   \fn void export_s19(char *filename, uint16_t *imageBuf, uint32_t addrStart, uint32_t addrStop, uint8_t verbose)
   
   \param[in]  filename    name of output file
   \param[in]  imageBuf    RAM image. HB!=0 indicates content. Index 0 corresponds to addrStart
   \param[in]  addrStart   start address of image (= lowest address in file)
   \param[in]  addrStop    last address of image (= highest address in file)
   \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   export RAM image to file in s19 hexfile format. For description of 
   Motorola S19 file format see http://en.wikipedia.org/wiki/SREC_(file_format)
*/
void export_s19(char *filename, uint16_t *imageBuf, uint32_t addrStart, uint32_t addrStop, uint8_t verbose) {

  int       i, j;              // loop variables
  uint32_t  addrRange;         // address range to consider
  int       lenLine;           // actual length of data line
  bool      flagData;          // line contains data (skip empty lines)
  uint8_t   data;              // value to store
  uint32_t  chk;               // checksum
  FILE      *fp;               // file pointer
  uint32_t  countByte;         // number of actually exported bytes

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
    printf("  export '%s' ... ", shortname);
  else if (verbose == INFORM)
    printf("  export S19 file '%s' ... ", shortname);
  else if (verbose == CHATTY)
    printf("  export Motorola S19 file '%s' ... ", shortname);
  fflush(stdout);

  // open output file
  fp=fopen(filename,"wb");
  if (!fp)
    Error("Failed to create file %s", filename);

  // address range [B] to consider
  addrRange = addrStop-addrStart+1;

  // start with dummy header line to avoid 'srecord' warning
  fprintf(fp, "S00F000068656C6C6F202020202000003C\n");
  
  // store in lines of 32B
  lenLine = 32;
  countByte = 0;
  for (i=0; i<addrRange; i+=lenLine) {

    // near end of buffer reduce line length
    if (i+lenLine > addrRange)
      lenLine = addrRange - i;

    // check if line contains set data (HB!=0)
    flagData = false;
    for (j=0; (j<lenLine) && (j<addrRange); j++) {
      if (imageBuf[i+j] && 0xFF00)
        flagData = true;
    }

    // if line contains set data, save line (see http://en.wikipedia.org/wiki/SREC_(file_format) )
    if (flagData) {

      // save data, accound for address width
      if ((addrStart+i) <= 0xFFFF) {
        fprintf(fp, "S1%02X%04X", lenLine+3, addrStart+i); // 16-bit address: 2B addr + data + 1B chk
        chk = (uint8_t) (lenLine+3) + (uint8_t) (addrStart+i) + (uint8_t) ((addrStart+i) >> 8);
      }
      else if ((addrStart+i) <= 0xFFFFFF) {
        fprintf(fp, "S2%02X%06X", lenLine+4, addrStart+i); // 24-bit address: 3B addr + data + 1B chk
        chk = (uint8_t) (lenLine+4) + (uint8_t) (addrStart+i) + (uint8_t) ((addrStart+i) >> 8) + (uint8_t) ((addrStart+i) >> 16);
      }
      else {
        fprintf(fp, "S3%02X%08X", lenLine+5, addrStart+i); // 32-bit address: 4B addr + data + 1B chk
        chk = (uint8_t) (lenLine+5) + (uint8_t) (addrStart+i) + (uint8_t) ((addrStart+i) >> 8) + (uint8_t) ((addrStart+i) >> 16) + (uint8_t) ((addrStart+i) >> 24);
      }
      for (j=0; (j<lenLine) && (j<addrRange); j++) {
        data = (uint8_t) (imageBuf[i+j] & 0x00FF);
        chk += data;
        fprintf(fp, "%02X", data);
        countByte++;
      }
      chk = ((chk & 0xFF) ^ 0xFF);
      fprintf(fp, "%02X\n", chk);

    } // line contains data

  } // loop over lines

  // attach generic EOF line
  fprintf(fp, "S903FFFFFE\n");

  // close output file
  fflush(fp);
  fclose(fp);

  // print message
  if ((verbose == SILENT) || (verbose == INFORM)) {
    printf("done\n");
  }
  else if (verbose == CHATTY) {
    if (countByte>2048)
      printf("done (%1.1fkB @ 0x%04x - 0x%04x)\n", (float) countByte/1024.0, addrStart, addrStop);
    else if (countByte>0)
      printf("done (%dB @ 0x%04x - 0x%04x)\n", countByte, addrStart, addrStop);
    else
      printf("done, no data\n");
  }
  fflush(stdout);

} // export_s19
  


/**
   \fn void export_txt(char *filename, uint16_t *imageBuf, uint32_t addrStart, uint32_t addrStop, uint8_t verbose)
   
   \param[in]  filename    name of output file
   \param[in]  imageBuf    RAM image. HB!=0 indicates content. Index 0 corresponds to addrStart
   \param[in]  addrStart   start address of image (= lowest address in file)
   \param[in]  addrStop    last address of image (= highest address in file)
   \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   export RAM image to file with plain text table (hex addr / data)
*/
void export_txt(char *filename, uint16_t *imageBuf, uint32_t addrStart, uint32_t addrStop, uint8_t verbose) {

  int       i;                 // index
  uint32_t  addrRange;         // address range to consider
  FILE      *fp;               // file pointer
  uint32_t  countByte;         // number of actually exported bytes
  
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
    printf("  export '%s' ... ", shortname);
  else if (verbose == INFORM)
    printf("  export table '%s' ... ", shortname);
  else if (verbose == CHATTY)
    printf("  export ASCII table to file '%s' ... ", shortname);
  fflush(stdout);

  // open output file
  fp=fopen(filename,"wb");
  if (!fp)
    Error("Failed to create file %s", filename);

  // address range [B] to consider
  addrRange = addrStop-addrStart+1;
  
  // save header
  fprintf(fp, "# address	value\n");

  // store each defined value (HB!=0x00) in a separate line (addr \t value)
  countByte = 0;
  for (i=0; i<addrRange; i++) {
    if (imageBuf[i]) {
      countByte++;
      fprintf(fp, "0x%04x	0x%02x\n", addrStart+i, (imageBuf[i] & 0xFF));
      //printf("0x%04x   0x%04x   0x%02x\n", addrStart+i, imageBuf[i], (imageBuf[i] & 0xFF));
    }
  }

  // close output file
  fflush(fp);
  fclose(fp);

  // print message
  if ((verbose == SILENT) || (verbose == INFORM)) {
    printf("done\n");
  }
  else if (verbose == CHATTY) {
    if (countByte>2048)
      printf("done (%1.1fkB @ 0x%04x - 0x%04x)\n", (float) countByte/1024.0, addrStart, addrStop);
    else if (countByte>0)
      printf("done (%dB @ 0x%04x - 0x%04x)\n", countByte, addrStart, addrStop);
    else
      printf("done, no data\n");
  }
  fflush(stdout);

} // export_txt
  


/**
   \fn void export_bin(char *filename, uint16_t *imageBuf, uint32_t addrStart, uint32_t addrStop, uint8_t verbose)
   
   \param[in]  filename    name of output file
   \param[in]  imageBuf    RAM image. HB!=0 indicates content. Index 0 corresponds to addrStart
   \param[in]  addrStart   start address of image (= lowest address in file)
   \param[in]  addrStop    last address of image (= highest address in file)
   \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   export RAM image to binary file. Note that start address is not stored!
*/
void export_bin(char *filename, uint16_t *imageBuf, uint32_t addrStart, uint32_t addrStop, uint8_t verbose) {

  int       i;                 // index
  FILE      *fp;               // file pointer
  uint32_t  countByte;         // number of actually exported bytes
  uint8_t   val;
  
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
    printf("  export '%s' ... ", shortname);
  else if (verbose == INFORM)
    printf("  export binary '%s' ... ", shortname);
  else if (verbose == CHATTY)
    printf("  export binary to file '%s' ... ", shortname);
  fflush(stdout);

  // open output file
  fp=fopen(filename,"wb");
  if (!fp)
    Error("Failed to create file %s", filename);

  // number of bytes to export
  countByte = addrStop-addrStart+1;
  
  // store every value in address range
  for (i=0; i<countByte; i++) {
    val = imageBuf[i] & 0xFF;
    fwrite(&val,sizeof(val), 1, fp); // write byte per byte (image is 16-bit)
    //printf("0x%04x   0x%04x   0x%02x\n", addrStart+i, imageBuf[i], (imageBuf[i] & 0xFF));
  }

  // close output file
  fflush(fp);
  fclose(fp);

  // print message
  if ((verbose == SILENT) || (verbose == INFORM)) {
    printf("done\n");
  }
  else if (verbose == CHATTY) {
    if (countByte>2048)
      printf("done (%1.1fkB @ 0x%04x - 0x%04x)\n", (float) countByte/1024.0, addrStart, addrStop);
    else if (countByte>0)
      printf("done (%dB @ 0x%04x - 0x%04x)\n", countByte, addrStart, addrStop);
    else
      printf("done, no data\n");
  }
  fflush(stdout);

} // export_bin
  


/**
   \fn void print_console(uint16_t *imageBuf, uint32_t addrStart, uint32_t addrStop, uint8_t verbose)
   
   \param[in]  imageBuf    RAM image. HB!=0 indicates content. Index 0 corresponds to addrStart
   \param[in]  addrStart   start address of image (= lowest address in file)
   \param[in]  addrStop    last address of image (= highest address in file)
   \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   print RAM image to console as "hexAddr  data"
*/
void print_console(uint16_t *imageBuf, uint32_t addrStart, uint32_t addrStop, uint8_t verbose) {

  int       i;                 // index
  uint32_t  addrRange;         // address range to consider
  uint32_t  countByte;         // number of actually exported bytes
  
  // print message
  if (verbose > MUTE)
    printf("\n  print memory ... \n");
  fflush(stdout);

  // address range [B] to consider
  addrRange = addrStop-addrStart+1;
  
  // store each defined value (HB!=0x00) in a separate line (addr \t value)
  countByte = 0;
  printf("    address	value\n");
  for (i=0; i<addrRange; i++) {
    if (imageBuf[i]) {
      countByte++;
      printf("    0x%04x	0x%02x\n", addrStart+i, (imageBuf[i] & 0xFF));
      //printf("0x%04x   0x%04x   0x%02x\n", addrStart+i, imageBuf[i], (imageBuf[i] & 0xFF));
    }
  }

  // print message
  if ((verbose == SILENT) || (verbose == INFORM)) {
    printf("  done\n");
  }
  else if (verbose == CHATTY) {
    if (countByte>2048)
      printf("  done (%1.1fkB @ 0x%04x - 0x%04x)\n", (float) countByte/1024.0, addrStart, addrStop);
    else if (countByte>0)
      printf("  done (%dB @ 0x%04x - 0x%04x)\n", countByte, addrStart, addrStop);
    else
      printf("  done, no data\n");
  }
  fflush(stdout);

} // print_console

// end of file
