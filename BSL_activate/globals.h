/**
  \file globals_uC.h
   
  \author G. Icking-Konert
  \date 2013-11-22
  \version 0.1
   
  \brief declaration of global variables
   
  central declaration of global variables. If '_MAIN_' is defined in
  pre-processor, variables are declared, else just referenced.
  Note that globals are not initialized here.
*/

/*----------------------------------------------------------
    MODULE DEFINITION FOR MULTIPLE INCLUSION
----------------------------------------------------------*/
#ifndef _GLOBALS_UC_H_
#define _GLOBALS_UC_H_

#include <stdint.h>
#include "stm8as.h"



// declare or reference to global variables, depending on '_MAIN_' 
#ifdef _MAIN_
  #define global
#else // _MAIN_
  #define global extern
#endif // _MAIN_


/*-----------------------------------------------------------------------------
    DECLARATION OF GLOBAL VARIABLES
-----------------------------------------------------------------------------*/


/////////////
// for interfacing C to inline assembler
/////////////
global uint32_t           g_addr;              // 24b address for sdcc (doesn't support far pointers yet)
global uint8_t	          g_val;               // 1B data for r/w far data


// undefine keyword 'global'
#undef global

/*-----------------------------------------------------------------------------
    END OF MODULE DEFINITION FOR MULTIPLE INLUSION
-----------------------------------------------------------------------------*/
#endif // _GLOBALS_UC_H_

