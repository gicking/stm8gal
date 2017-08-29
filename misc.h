/**
  \file misc.h
   
  \author G. Icking-Konert
  \date 2014-03-14
  \version 0.1
   
  \brief declaration of misc routines
   
  declaration of routines not really fitting anywhere else
*/

// for including file only once
#ifndef _MISC_H_
#define _MISC_H_


// include files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

// color codes 
#define PRM_COLOR_DEFAULT       0
#define PRM_COLOR_BLACK         1
#define PRM_COLOR_BLUE          2
#define PRM_COLOR_GREEN         3
#define PRM_COLOR_RED           4
#define PRM_COLOR_PINK          5
#define PRM_COLOR_WHITE         6
#define PRM_COLOR_YELLOW        7


/// for sleep(ms) use system specific routines
#if defined(WIN32)
  #define SLEEP(a)    Sleep(a)
#elif defined(__APPLE__) || defined(__unix__)
  #define SLEEP(a)    usleep((int32_t) a*1000L)
#else
  #error OS not supported
#endif

/// Display error message and terminate
void Error(const char *format, ...);

/// terminate program after cleaning up
void        Exit(uint8_t code, uint8_t pause);

/// strip path from application name
void        stripPath(char *in, char *out);

/// extract major / minor / build revision number from 16b identifier
void        get_version(uint16_t vers, uint8_t *major, uint8_t *minor, uint8_t *build, uint8_t *status);

/// print application name and version
void        get_app_name(char *in, uint16_t vers, char *out);

/// set title of console window
#if defined(WIN32) || defined(__APPLE__) || defined(__unix__)
void        setConsoleTitle(const char *title);
#endif // WIN32 || __APPLE__ || __unix__

/// set console text color
void        setConsoleColor(uint8_t color);

#endif // _MISC_H_

// end of file
