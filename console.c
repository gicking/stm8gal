/**
  \file console.c
   
  \author G. Icking-Konert
  \date 2020-10-10
  \version 0.1
   
  \brief implementation of console related routines
   
  implementation of console related routines
*/


// include files
#include <stdio.h>
#include <stdint.h>
#include "console.h"
#include "main.h"


/**
  \fn void setConsoleTitle(const char *title)
  
  \param[in]  title   title for console window
  
   set console title to application name + version number
     Win32: uses Windows API functions
     POSIX: use console escape sequence
*/
 
// 
#if defined(WIN32) || defined(__APPLE__) || defined(__unix__)
void setConsoleTitle(const char *title) {
  
  // for background operation skip to avoid strange control characters 
  if (g_backgroundOperation)
    return;


#if defined(WIN32)
  SetConsoleTitle(title);

#elif defined(__APPLE__) || defined(__unix__)
  printf("%c]0;%s%c", '\033', title, '\007');

#else
  #error unknown OS type
#endif

} // SetTitle
#endif // WIN32 || __APPLE__ || __unix__


  
/**
  \fn void setConsoleColor(uint8_t color)
  
  \param[in] color  new text color
   
  switch text color in console output to specified value
    Win32: uses Windows API functions
    POSIX: uses VT100 escape codes
*/
void setConsoleColor(uint8_t color) {
  
  // for background operation skip to avoid strange control characters 
  if (g_backgroundOperation)
    return;


#if defined(WIN32)

  static WORD                   oldColor, colorBck;
  static char                   flag=0;
  CONSOLE_SCREEN_BUFFER_INFO    csbiInfo; 
  
  // at first call get and store current text and backgound color
  if (flag==0) {
    flag = 1;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbiInfo);
    oldColor = csbiInfo.wAttributes;
    colorBck = (csbiInfo.wAttributes) & (BACKGROUND_BLUE	| BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY);
  }

  // set to text color
  switch (color) {
    
    // revert color to start value
    case PRM_COLOR_DEFAULT:
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), oldColor);
      break;

    // set color to black; retain background color
    case PRM_COLOR_BLACK:
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FG_BLACK | colorBck);
      break;

    // set color to blue; retain background color
    case PRM_COLOR_BLUE:
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FG_BLUE | colorBck);
      break;

    // set color to green; retain background color
    case PRM_COLOR_GREEN:
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FG_GREEN | colorBck);
      break;

    // set color to red; retain background color
    case PRM_COLOR_RED:
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FG_LIGHTRED | colorBck);
      break;

    // set color to pink; retain background color
    case PRM_COLOR_PINK:
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FG_PINK | colorBck);
      break;
      
    // set color to white; retain background color
    case PRM_COLOR_WHITE:
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FG_WHITE | colorBck);
      break;
      
    // set color to yellow; retain background color
    case PRM_COLOR_YELLOW:
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FG_YELLOW | colorBck);
      break;
      
    // else revert color to default
    default:
      //fprintf(stderr, "\n\ndefault\n\n");
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), oldColor);
      
  } //  switch (color)
  

#elif defined(__APPLE__) || defined(__unix__)

  // set to text color (see http://linuxgazette.net/issue65/padala.html)
  switch (color) {
    
    // revert color to start value
    case PRM_COLOR_DEFAULT:
      printf("\033[0m");
      //printf("\n\nrevert\n\n");
      fflush(stdout);
      break;

    // set color to black; retain background color
    case PRM_COLOR_BLACK:
      printf("\033[30m");
      //printf("\n\nblack\n\n");
      fflush(stdout);
      break;

    // set color to blue; retain background color
    case PRM_COLOR_BLUE:
      printf("\033[1;34m");
      //printf("\n\nblue\n\n");
      fflush(stdout);
      break;

    // set color to green; retain background color
    case PRM_COLOR_GREEN:
      printf("\033[1;32m");
      //printf("\n\ngreen\n\n");
      fflush(stdout);
      break;

    // set color to red; retain background color
    case PRM_COLOR_RED:
      printf("\033[1;31m");
      //printf("\n\nred\n\n");
      fflush(stdout);
      break;
      
    // set color to pink; retain background color
    case PRM_COLOR_PINK:
      printf("\033[1;35m");
      //printf("\n\npink\n\n");
      fflush(stdout);
      break;
      
    // set color to white; retain background color
    case PRM_COLOR_WHITE:
      printf("\033[37m");
      //printf("\n\nwhite\n\n");
      fflush(stdout);
      break;
      
    // set color to yellow; retain background color
    case PRM_COLOR_YELLOW:
      printf("\033[1;33m");
      //printf("\n\nyellow\n\n");
      fflush(stdout);
      break;
      
    // else revert color to default
    default:
      printf("\033[0m");
      //printf("\n\ndefault\n\n");
      fflush(stdout);
      
  } //  switch (color)

#else
  #error unknown OS type
#endif

} // setConsoleColor

// end of file
