/**
  \file spi_comm.h
   
  \author G. Icking-Konert
  \date 2017-12-10
  \version 0.1
   
  \brief declaration of SPI comm routines
   
  declaration of routines for SPI communication using the spidev library.
  
*/

// for including file only once
#ifndef _SPI_COMM_H_
#define _SPI_COMM_H_


// generic ANSI
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

// OS specific: Win32
#if defined(WIN32)
  #error Windows not yet supported (which API?)

// OS specific: Posix
#elif defined(__APPLE__) || defined(__unix__)
  #define HANDLE  int     // comm port handler is int
  #include <fcntl.h>      // File control definitions
  #include <termios.h>    // Posix terminal control definitions
  #include <errno.h>      // error number definitions
  #include <sys/types.h>
  #include <dirent.h>
  #include <string.h>
  #include <sys/ioctl.h>
  #include <unistd.h>
  #include <sys/ioctl.h>
  #include <linux/types.h>
  #include <linux/spi/spidev.h>

#else
  #error OS not supported
#endif


/// init SPI port
HANDLE      init_spi(const char *port, uint32_t baudrate);

/// close SPI port
void        close_spi(HANDLE *fpCom);

/// send data via SPI (no parallel receive)
uint32_t    send_spi(HANDLE fpCom, uint32_t lenTx, char *Tx);

/// receive data via SPI (no parallel send)
uint32_t    receive_spi(HANDLE fpCom, uint32_t lenRx, char *Rx);

#endif // _SPI_COMM_H_

// end of file
