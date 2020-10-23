/**
  \file console.h
   
  \author G. Icking-Konert
  \date 2020-10-10
  \version 0.1
   
  \brief declaration of console related routines
   
  declaration of console related routines
*/

// for including file only once
#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>

// color codes 
#define PRM_COLOR_DEFAULT       0
#define PRM_COLOR_BLACK         1
#define PRM_COLOR_BLUE          2
#define PRM_COLOR_GREEN         3
#define PRM_COLOR_RED           4
#define PRM_COLOR_PINK          5
#define PRM_COLOR_WHITE         6
#define PRM_COLOR_YELLOW        7

// Windows specific
#if defined(WIN32) || defined(WIN64)

  #include "windows.h"

  // forground colours
  #define FG_BLACK      0
  #define FG_BLUE       ( FOREGROUND_BLUE | FOREGROUND_INTENSITY )
  #define FG_BROWN      ( FOREGROUND_RED | FOREGROUND_GREEN )
  #define FG_DARKBLUE   ( FOREGROUND_BLUE )
  #define FG_DARKGREY   ( FOREGROUND_INTENSITY )
  #define FG_GREEN      ( FOREGROUND_GREEN )
  #define FG_GREY       ( FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE )
  #define FG_LIGHTBLUE  ( FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY )
  #define FG_LIGHTGREEN ( FOREGROUND_GREEN | FOREGROUND_INTENSITY )
  #define FG_PINK       ( FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY )
  #define FG_RED        ( FOREGROUND_RED )
  #define FG_LIGHTRED   ( FOREGROUND_RED | FOREGROUND_INTENSITY )
  #define FG_TURQUOISE  ( FOREGROUND_BLUE | FOREGROUND_GREEN )
  #define FG_VIOLET     ( FOREGROUND_RED | FOREGROUND_BLUE )
  #define FG_WHITE      ( FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY )
  #define FG_YELLOW     ( FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY )
  
  // background colours 
  #define BG_BLACK      ( 0 )
  #define BG_BLUE       ( BACKGROUND_BLUE | BACKGROUND_INTENSITY )
  #define BG_BROWN      ( BACKGROUND_RED | BACKGROUND_GREEN )
  #define BG_DARKBLUE   ( BACKGROUND_BLUE )
  #define BG_DARKGREY   ( BACKGROUND_INTENSITY )
  #define BG_GREEN      ( BACKGROUND_GREEN )
  #define BG_GREY       ( BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE )
  #define BG_LIGHTBLUE  ( BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY )
  #define BG_LIGHTGREEN ( BACKGROUND_GREEN | BACKGROUND_INTENSITY )
  #define BG_PINK       ( BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY )
  #define BG_RED        ( BACKGROUND_RED )
  #define BG_LIGHTRED   ( BACKGROUND_RED | BACKGROUND_INTENSITY )
  #define BG_TURQUOISE  ( BACKGROUND_BLUE | BACKGROUND_GREEN )
  #define BG_VIOLET     ( BACKGROUND_RED | BACKGROUND_BLUE )
  #define BG_WHITE      ( BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY )
  #define BG_YELLOW     ( BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY )

#elif defined(__APPLE__) || defined(__unix__)

#endif // OS


/// destination stream for console_print()
typedef enum {STDOUT=0, STDERR=1} output_t;


/// set title of console window
#if defined(WIN32) || defined(WIN64) || defined(__APPLE__) || defined(__unix__)
  void setConsoleTitle(const char *title);
#endif // WIN32 || WIN64 || __APPLE__ || __unix__

/// Sets module variable to enable or disable background operation
void Console_SetBackgroundOperation(bool bgOperation);

/// Returns module variable to enable or disable background operation
bool Console_GetBackgroundOperation(void);

/// set console text color
void setConsoleColor(uint8_t color);

/// message output function. Is separate to facilitate output to GUI window
int console_print(output_t dest, char *fmt, ...);

/// display error message and terminate
int Error(char *format, ...);

/// terminate program after cleaning up
void Exit(uint8_t code, uint8_t pause);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _CONSOLE_H_

// end of file
