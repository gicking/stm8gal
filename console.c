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
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "console.h"
#include "main.h"


/// optimize for background operation, e.g. skip prompts and console colors
static bool           g_backgroundOperation;

/**
  \fn void setConsoleTitle(const char *title)
  
  \param[in]  title   title for console window
  
   set console title to application name + version number
     Win32: uses Windows API functions
     POSIX: use console escape sequence
*/
 
// 
#if defined(WIN32) || defined(WIN64) || defined(__APPLE__) || defined(__unix__)
void setConsoleTitle(const char *title) {
  
  // for background operation skip to avoid strange control characters 
  if (g_backgroundOperation)
    return;


#if defined(WIN32) || defined(WIN64)
  SetConsoleTitle(title);

#elif defined(__APPLE__) || defined(__unix__)
  printf("%c]0;%s%c", '\033', title, '\007');

#else
  #error unknown OS type
#endif

} // SetTitle
#endif // WIN32 || WIN64 || __APPLE__ || __unix__


  
/**
  \fn void Console_SetBackgroundOperation(bool bgOperation)
  
  \param[in] bgOperation  True: background operation; False: Foreground

  Sets module variable to enable or disable background operation
   
*/
void Console_SetBackgroundOperation(bool bgOperation) {
  
  g_backgroundOperation = bgOperation; 
}

/**
  \fn bool Console_GetBackgroundOperation(void)
  
  Returns module variable to enable or disable background operation

*/
bool Console_GetBackgroundOperation(void) {
  
  return(g_backgroundOperation); 
}

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


#if defined(WIN32) || defined(WIN64)

;  static WORD                   oldColor, colorBck;
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



/**
  \fn int console_print(output_t dest, char *fmt, ...)

  \param dest      destination to print to (STDOUT or STDERR)
  \param fmt       format string as for printf()

  \return number of printed characters (positive) or error (negative)

  message output function. Is separate to facilitate output to GUI window.
  Output format is identical to printf(). 

*/
int console_print(output_t dest, char *fmt, ...) {
  
  int   result;
  char  str[500];
    
  // create string containing message
  va_list args;
  va_start(args, fmt);
  result = vsnprintf(str, 500, fmt, args);
  va_end(args);

  // output to stdout
  if (dest == STDOUT) {
    fprintf(stdout, "%s", str);
    fflush(stdout);
  }

  // output to stderr in red
  else if (dest == STDERR) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "%s", str);
    setConsoleColor(PRM_COLOR_DEFAULT);
  }

  // return vsnprintf status
  return result;

} // console_print()



#ifndef NO_EXIT_ON_ERROR
/**
  \fn int Error(const char *format, ...)
   
  \param[in] code    return code of application to commandline
  \param[in] pause   wait for keyboard input before terminating

  \return number of printed characters (positive) or error (negative)

  Display error message and terminate program. 
  Output format is identical to printf(). 
  Prior to program termination query for \<return\> unless background operation is specified.
*/
int Error(char *fmt, ...)
{
  int   result;
  char  str[500];
    
  // create string containing message
  va_list args;
  va_start(args, fmt);
  result = vsnprintf(str, 500, fmt, args);
  va_end(args);

  // output to stderr in red
  setConsoleColor(PRM_COLOR_RED);
  fprintf(stderr, "\nError: %s\n", str);
  setConsoleColor(PRM_COLOR_DEFAULT);
  
  // terminate program
  Exit(1, 1);
  
  // return vsnprintf status (avoid compiler warning)
  return result;
  
} // Error()



/**
  \fn void Exit(uint8_t code, uint8_t pause)
   
  \param[in] code    return code of application to commandline
  \param[in] pause   wait for keyboard input before terminating

  Terminate program. Replace standard exit() to query for \<return\> 
  before termination, unless background operation is specified.
*/
void Exit(uint8_t code, uint8_t pause) {

  // on error code !=0 ring bell
  if (code) {
    printf("\a");
    fflush(stdout);
  }
  
  // reset text color to default, just to be sure
  setConsoleColor(PRM_COLOR_DEFAULT);

  // optionally prompt for <return>
  if ((pause) && (!g_backgroundOperation)) {
    printf("\npress <return> to exit");
    fflush(stdout);
    fflush(stdin);
    getchar();
  }
  printf("\n");

  // terminate application
  exit(code);

} // Exit

#endif  /* NO_EXIT_ON_ERROR */

// end of file
