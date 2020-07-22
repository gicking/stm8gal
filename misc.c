/**
  \file misc.c
  
  \author G. Icking-Konert
  \date 2014-03-14
  \version 0.1
   
  \brief implementation of misc routines
   
  implementation of routines not really fitting anywhere else
*/


#include <string.h>
#include <stdlib.h>

#if !defined(_MSC_VER)
  #include <unistd.h>
  #include <sys/time.h>
#endif


#include "misc.h"
#include "main.h"
#include "version.h"


// WIN32 specific
#if defined(WIN32)

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



/**
  \fn void Error(const char *format, ...)
   
  \param[in] code    return code of application to commandline
  \param[in] pause   wait for keyboard input before terminating

  Display error message and terminate program. Output format is identical to
  printf(). Prior to program termination query for \<return\> unless
  background operation is specified.
*/
void Error(const char *format, ...)
{
  va_list vargs;
  va_start(vargs, format);
  setConsoleColor(PRM_COLOR_RED);
  fprintf(stderr, "Error: ");
  vfprintf(stderr, format, vargs);
  va_end(vargs);
  fprintf(stderr, "\n");
  Exit(1, 1);
}



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
  
  // reset text color to default
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



/**
  \fn void stripPath(char *in, char *out)
   
  \param[in] in      name of application incl. path
  \param[in] out     name of application excl. path

  strip pathname from application name 
*/
void stripPath(char *in, char *out) {

  int32_t   i;
  char      *tmp;

  // find position of last path delimiter '/' (Posix) or '\' (Win)
  tmp = in;
  for (i=0; i<strlen(in); i++) {
    if ((in[i] == '/') || (in[i] == '\\'))
      tmp = in+i+1;
  }

  // copy name without path
  sprintf(out, "%s", tmp);

} // stripPath



/**
  \fn void get_version(uint16_t vers, uint8_t *major, uint8_t *minor, uint8_t *build, uint8_t *status)
   
  \param[in]  vers      16b revision number in format xx.xxxxxxxx.xxxxx.x
  \param[out] major     major revision number [15:14] -> 0..3
  \param[out] minor     minor revision number [13:6] -> 0..255
  \param[out] build     build number [5:1] -> 0..31
  \param[out] status    status [0] -> 0=beta; 1=released

  extract major / minor / build revision number from 16b identifier
  in format xx.xxxxxxxx.xxxxx.x
*/
void get_version(uint16_t vers, uint8_t *major, uint8_t *minor, uint8_t *build, uint8_t *status) {

  // major version ([15:14] -> 0..7)
  *major = (uint8_t) ((vers & 0xC000) >> 14);
  
  // minor version ([13:6] -> 0..255)
  *minor = (uint8_t) ((vers & 0x3FC0) >> 6);
  
  // build number ([5:1] -> 0..31)
  *build = (uint8_t) ((vers&0x003E) >> 1);
  
  // release status ([0] -> 0=beta; 1=released)
  *status = (uint8_t) (vers&0x0001);
  
} // get_version



/**
  \fn void get_app_name(char *appFull, uint16_t versID, char *appName, char *versStr)
   
  \param[in]  appFull   name of application incl. path
  \param[in]  versID    16b version identifier
  \param[out] appName   name of application w/o path
  \param[out] versStr   version number as string

  print application name and major / minor / build revision numbers. Remove path from app name
*/
void get_app_name(char *appFull, uint16_t versID, char *appName, char *versStr) {

  int32_t   i;
  char      *tmp;
  uint8_t   major, minor, build, status;

  // find position of last path delimiter '/' (Posix) or '\' (Win)
  tmp = appFull;
  for (i=0; i<strlen(appFull); i++) {
    if ((appFull[i] == '/') || (appFull[i] == '\\'))
      tmp = appFull+i+1;
  }

  // copy app name w/o path
  sprintf(appName, "%s", tmp);


  // extract major / minor / build revision number
  get_version(versID, &major, &minor, &build, &status);
  
  // copy version data to string
  if (status==0)
    sprintf(versStr, "v%d.%d.%d beta", major, minor, build);
  else
    sprintf(versStr, "%d.%d.%d", major, minor, build);

} // get_app_name



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

  
  
/**
  \fn uint64_t millis(void)
  
  \return time [ms] since start of program
  
  Return the number of milliseconds since the current program was launched. 
  Due to used uint64_t format this number will not overflow in any realistic
  time (in contrast to Arduino).
*/
uint64_t millis() {

  // use below micros() for simplicity. On a PC overhead is negligible
  return(micros()/1000LL);

} // millis

  
  
/**
  \fn uint64_t micros(void)
  
  \return time [us] since start of program
  
  Return the number of microseconds since the current program was launched. 
  Due to used uint64_t format this number will not overflow in any realistic
  time (in contrast to Arduino).
*/
uint64_t micros() {

  static bool      s_firstCall = true;
  static uint64_t  s_microsStart = 0;
  uint64_t         microsCurr;

#if defined(WIN32)

  static double    s_ticksPerMicros = 0;     // resolution of Windows fast core timer
  LARGE_INTEGER    tick;

  // on first call get resolution of fast core timer
  if (s_firstCall) {
    QueryPerformanceFrequency(&tick);
    s_ticksPerMicros = (double) (tick.QuadPart) / 1e6;
  }

  // get time in us
  QueryPerformanceCounter(&tick);
  microsCurr = (uint64_t) (tick.QuadPart / s_ticksPerMicros);

#endif // WIN32


#if defined(__APPLE__) || defined(__unix__)
  
  // get current time
  struct timeval  te; 
  gettimeofday(&te, NULL);

  // calculate milliseconds
  microsCurr = te.tv_sec*1000000LL + te.tv_usec;

#endif // WIN32

  // on 1st call also store starting time and set flag
  if (s_firstCall) {
    s_firstCall = false;
    s_microsStart = microsCurr;
  }

  // return micros since 1st call
  return(microsCurr - s_microsStart);

} // micros

// end of file
