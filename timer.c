/**
  \file timer.c
   
  \author G. Icking-Konert
  \date 2020-10-10
  \version 0.1
   
  \brief implementation of time measurement routines
   
  implementation of time measurement routines
*/

// include files
#include <stdbool.h>
#include "timer.h"


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

#if defined(WIN32) || defined(WIN64)

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

#endif // WIN32 || WIN64


#if defined(__APPLE__) || defined(__unix__)
  
  // get current time
  struct timeval  te; 
  gettimeofday(&te, NULL);

  // calculate milliseconds
  microsCurr = te.tv_sec*1000000LL + te.tv_usec;

#endif // __APPLE__ || __unix__

  // on 1st call also store starting time and set flag
  if (s_firstCall) {
    s_firstCall = false;
    s_microsStart = microsCurr;
  }

  // return micros since 1st call
  return(microsCurr - s_microsStart);

} // micros

// end of file
