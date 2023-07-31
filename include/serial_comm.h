/**
  \file serial_comm.h
   
  \author G. Icking-Konert
   
  \brief declaration of RS232 comport routines
   
  declaration of of routines for RS232 communication using the Windows or Posix API.
  For Windows, see e.g. http://msdn.microsoft.com/en-us/library/default.aspx
  For Posix see http://www.easysw.com/~mike/serial/serial.html
*/

// for including file only once
#ifndef _SERIAL_COMM_H_
#define _SERIAL_COMM_H_


// generic ANSI
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

// OS specific: Windows
#if defined(WIN32) || defined(WIN64)
  #include <windows.h>
  #include <conio.h>

// OS specific: Posix
#elif defined(__APPLE__) || defined(__unix__)
  #define HANDLE  int     // comm port handler is int
  #include <fcntl.h>      // File control definitions
  #include <termios.h>    // Posix terminal control definitions
  #include <getopt.h>
  #include <errno.h>      // error number definitions
  #include <sys/types.h>
  #include <dirent.h>
  #include <string.h>
  #include <sys/ioctl.h>
  #include <unistd.h>

#else
  #error OS not supported
#endif


/// list all available comm ports
void        list_ports(void);

/// init comm port
HANDLE      init_port(const char *port, uint32_t baudrate, uint32_t timeout, uint8_t numBits, uint8_t parity, uint8_t numStop, uint8_t RTS, uint8_t DTR);

/// close comm port
void        close_port(HANDLE *fpCom);

/// generate low pulse on DTR in [ms] to reset STM8
void        pulse_DTR(HANDLE fpCom, uint32_t duration);

/// generate low pulse on RTS in [ms] to reset STM8
void        pulse_RTS(HANDLE fpCom, uint32_t duration);

/// generate low pulse on Raspberry pin in [ms] to reset STM8
void        pulse_GPIO(int pin, uint32_t duration);

/// get comm port settings
void        get_port_attribute(HANDLE fpCom, uint32_t *baudrate, uint32_t *timeout, uint8_t *numBits, uint8_t *parity, uint8_t *numStop, uint8_t *RTS, uint8_t *DTR);

/// modify comm port settings
void        set_port_attribute(HANDLE fpCom, uint32_t baudrate, uint32_t timeout, uint8_t numBits, uint8_t parity, uint8_t numStop, uint8_t RTS, uint8_t DTR);

/// modify comm port baudrate
void        set_baudrate(HANDLE fpCom, uint32_t Baudrate);

/// modify comm port timeout
void        set_timeout(HANDLE fpCom, uint32_t Timeout);

/// modify comm port parity
void        set_parity(HANDLE fpCom, uint8_t Parity);

/// send data
uint32_t    send_port(HANDLE fpCom, uint8_t uartMode, uint32_t lenTx, char *Tx);

/// receive data
uint32_t    receive_port(HANDLE fpCom, uint8_t uartMode, uint32_t lenRx, char *Rx);

/// flush port buffers
void        flush_port(HANDLE fpCom);

#endif // _SERIAL_COMM_H_

// end of file
