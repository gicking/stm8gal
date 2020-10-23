/**
  \file main.h
   
  \author G. Icking-Konert
  \date 2014-03-15
  \version 0.1

  \brief main header and declaration of global variables 

  main header with global macros and global variable declaration. 
  All global variables start with "g_" to indicate their scope.

*/

// for including file only once
#ifndef _MAIN_H_
#define _MAIN_H_

#ifdef __cplusplus
extern "C"
{
#endif


/*******
  global includes
*******/

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>


/*******
  global macros
*******/

/// max length of strings, e.g. filenames
#define  STRLEN   1000

/// UART communication timeout
#define  TIMEOUT  1000

// activate debug output
//#define DEBUG


/*******
  global typedefs
*******/

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _MAIN_H_

// end of file
