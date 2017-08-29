/**
  \file stdint.h
   
  \author G. Icking-Konert
  \date 2013-11-22
  \version 0.1
   
  \brief substitute for <stdint.h>, if missing
   
  if <stdint.h> is missing in compiler, e.g. Cosmic, define data types
  with corresponding min/max values here
*/

/*-----------------------------------------------------------------------------
    MODULE DEFINITION FOR MULTIPLE INCLUSION
-----------------------------------------------------------------------------*/
#ifndef _STDINT_H_
#define _STDINT_H_

#define TRUE  1
#define FALSE 0

// compiler specific --> If possible, use <stdint.h> from compiler
typedef signed long     int32_t;
typedef signed short    int16_t;
typedef signed char     int8_t; 

typedef unsigned long   uint32_t;
typedef unsigned short  uint16_t;
typedef unsigned char   uint8_t; 


// define min/max values
#define 	INT8_MAX   0x7f
#define 	INT8_MIN   (-INT8_MAX - 1)

#define 	UINT8_MAX   0xFF
#define 	UINT8_MIN   0

#define 	INT16_MAX   0x7fff
#define 	INT16_MIN   (-INT16_MAX - 1)

#define 	UINT16_MAX  0xFFFF
#define 	UINT16_MIN  0

#define 	INT32_MAX   0x7fffffffL
#define 	INT32_MIN   (-INT32_MAX - 1L)

#define 	UINT32_MAX  0xFFFFFFFF
#define 	UINT32_MIN  0


/*-----------------------------------------------------------------------------
    END OF MODULE DEFINITION FOR MULTIPLE INLUSION
-----------------------------------------------------------------------------*/
#endif // _STDINT_H_
