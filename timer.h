/**
  \file timer.h
   
  \author G. Icking-Konert
  \date 2020-10-10
  \version 0.1
   
  \brief declaration of time measurement routines
   
  declaration of time measurement routines
*/

// for including file only once
#ifndef _TIMER_H_
#define _TIMER_H_

#ifdef __cplusplus
extern "C"
{
#endif

// include files
#include <stdint.h>
#if !defined(_MSC_VER)
  #include <unistd.h>
  #include <sys/time.h>
#endif


// system specific delay routines [ms]
#if defined(WIN32) || defined(WIN64)
  #include <windows.h>
  #define SLEEP(a)    Sleep(a)                     //< for sleep(ms) use system specific routines
#elif defined(__APPLE__) || defined(__unix__)
  #define SLEEP(a)    usleep((int32_t) a*1000L)    //< for sleep(ms) use system specific routines
#else
  #error OS not supported
#endif


/// get milliseconds since start of program (as Arduino)
uint64_t millis(void);

/// get microseconds since start of program (as Arduino)
uint64_t micros(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _TIMER_H_

// end of file
