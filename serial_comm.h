/**
  \file serial_comm.h
   
  \author G. Icking-Konert
  \date 2008-11-02
  \version 0.1
   
  \brief declaration of RS232 comm port routines
   
  declaration of of routines for RS232 communication using the Win32 or Posix API.
  For Win32, see e.g. http://msdn.microsoft.com/en-us/library/default.aspx
  For Posix see http://www.easysw.com/~mike/serial/serial.html
*/

// for including file only once
#ifndef _SERIAL_COMM_H_
#define _SERIAL_COMM_H_

#ifdef __cplusplus
extern "C"
{
#endif

// generic ANSI
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

/// physical bootloader interface 
#if defined(USE_SPIDEV) && defined(USE_SPI_ARDUINO)
  typedef enum {UART=0, SPI_ARDUINO=1, SPI_SPIDEV=2} physInterface_t;
#elif defined(USE_SPI_ARDUINO)
  typedef enum {UART=0, SPI_ARDUINO=1} physInterface_t;
#elif defined(USE_SPIDEV)
  typedef enum {UART=0, SPI_SPIDEV=2} physInterface_t;
#else
  typedef enum {UART=0} physInterface_t;
#endif

/// UART communication timeout
#define  TIMEOUT  1000

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

typedef enum STM8gal_SerialErrors
{
    STM8GAL_SERIALCOMMS_NO_ERROR = 0,
    STM8GAL_SERIALCOMMS_CANNOT_LIST_PORTS,
    STM8GAL_SERIALCOMMS_CANNOT_OPEN_PORT,
    STM8GAL_SERIALCOMMS_CANNOT_CLOSE_PORT,
    STM8GAL_SERIALCOMMS_CANNOT_SET_IO,
    STM8GAL_SERIALCOMMS_CANNOT_GET_PORT_CONFIG,
    STM8GAL_SERIALCOMMS_CANNOT_SET_PORT_CONFIG,
    STM8GAL_SERIALCOMMS_UNSUPPORTED_BAUD_RATE,
    STM8GAL_SERIALCOMMS_UNKNOWN_NUMBER_DATA_BITS,
    STM8GAL_SERIALCOMMS_UNKNOWN_PARITY,
    STM8GAL_SERIALCOMMS_FAILED_ONE_WIRE_ECHO,
    STM8GAL_SERIALCOMMS_SEND_ERROR,
} STM8gal_SerialErrors_t;

/// list all available comm ports
STM8gal_SerialErrors_t        list_ports(void);

/// init comm port
STM8gal_SerialErrors_t        init_port(HANDLE *fpCom, const char *port, uint32_t baudrate, uint32_t timeout, uint8_t numBits, uint8_t parity, uint8_t numStop, uint8_t RTS, uint8_t DTR);

/// close comm port
STM8gal_SerialErrors_t        close_port(HANDLE *fpCom);

/// generate low pulse on DTR in [ms] to reset STM8
STM8gal_SerialErrors_t        pulse_DTR(HANDLE fpCom, uint32_t duration);

/// generate low pulse on RTS in [ms] to reset STM8
STM8gal_SerialErrors_t        pulse_RTS(HANDLE fpCom, uint32_t duration);

/// generate low pulse on Raspberry pin in [ms] to reset STM8
void                          pulse_GPIO(int pin, uint32_t duration);

/// get comm port settings
STM8gal_SerialErrors_t        get_port_attribute(HANDLE fpCom, uint32_t *baudrate, uint32_t *timeout, uint8_t *numBits, uint8_t *parity, uint8_t *numStop, uint8_t *RTS, uint8_t *DTR);

/// modify comm port settings
STM8gal_SerialErrors_t        set_port_attribute(HANDLE fpCom, uint32_t baudrate, uint32_t timeout, uint8_t numBits, uint8_t parity, uint8_t numStop, uint8_t RTS, uint8_t DTR);

/// modify comm port baudrate
STM8gal_SerialErrors_t        set_baudrate(HANDLE fpCom, uint32_t Baudrate);

/// modify comm port timeout
STM8gal_SerialErrors_t        set_timeout(HANDLE fpCom, uint32_t Timeout);

/// modify comm port parity
STM8gal_SerialErrors_t        set_parity(HANDLE fpCom, uint8_t Parity);

/// send data
STM8gal_SerialErrors_t        send_port(HANDLE fpCom, uint8_t uartMode, uint32_t lenTx, char *Tx, uint32_t *numChars);

/// receive data
STM8gal_SerialErrors_t        receive_port(HANDLE fpCom, uint8_t uartMode, uint32_t lenRx, char *Rx, uint32_t *numChars);

/// flush port buffers
STM8gal_SerialErrors_t        flush_port(HANDLE fpCom);

/// return last error in the Serial Comms module
STM8gal_SerialErrors_t        SerialComm_GetLastError(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _SERIAL_COMM_H_

// end of file
