/**
  \file spi_comm.c
   
  \author G. Icking-Konert
  \date 2017-12-10
  \version 0.1
   
  \brief implementation of SPI comm routines
   
  implementation of routines for SPI communication using the spidev library.
  
*/

// include files
#include "spi_comm.h"
#include "misc.h"
#include "globals.h"



/**
  \fn HANDLE init_spi(const char *port, uint32_t baudrate)
   
  \brief open SPI port
   
  \param[in] port       name of port as string
  \param[in] baudrate   SPI port speed in Baud

  \return           handle to SPI port
  
  open SPI port for communication, set properties (baudrate, polarity,...). 
*/
HANDLE init_spi(const char *port, uint32_t baudrate) {

  
/////////
// Win32
/////////
#if defined(WIN32)
  #error Windows not yet supported (which API?)

#endif // WIN32


/////////
// Posix
/////////
#if defined(__APPLE__) || defined(__unix__) 

  HANDLE     fpSPI;
  int        ret;
  uint32_t   mode = 0;
  uint8_t    bits = 8;


  // latch in on falling edge. Default is latch on rising edge (CPHA=0)
  //mode |= SPI_CPHA;

  // clock idle high. Default is idle clock low (CPOL=0)
  //mode |= SPI_CPOL;

  // LSB first. Default is MSB first
  //mode |= SPI_LSB_FIRST;

  // chip-select high active. Default is low active
  //mode |= SPI_CS_HIGH;

  // 3-wire / half-duplex SPI. Default is 4-wire / full-duplex SPI
  //mode |= SPI_3WIRE;

  // chip-select control in software. Default is hardware CS control
  //mode |= SPI_NO_CS;


  //////
  // open device
  //////
  fpSPI = open(port, O_RDWR);
  if (fpSPI < 0) {
    setConsoleColor(PRM_COLOR_RED);
    fpSPI = 0;
    fprintf(stderr, "\n\nerror in 'init_spi(%s)': open port failed, exit!\n\n", port);
    Exit(1, g_pauseOnExit);
  }


  //////
  // change properties
  //////
  
  // set SPI mode
  ret = ioctl(fpSPI, SPI_IOC_WR_MODE32, &mode);
  if (ret == -1) {
    setConsoleColor(PRM_COLOR_RED);
    fpSPI = 0;
    fprintf(stderr, "\n\nerror in 'init_spi(%s)': set mode %d failed, exit!\n\n", port, mode);
    Exit(1, g_pauseOnExit);
  }

  // set SPI speed
  ret = ioctl(fpSPI, SPI_IOC_WR_MAX_SPEED_HZ, &baudrate);
  if (ret == -1) {
    setConsoleColor(PRM_COLOR_RED);
    fpSPI = 0;
    fprintf(stderr, "\n\nerror in 'init_spi(%s)': set speed %d failed, exit!\n\n", port, baudrate);
    Exit(1, g_pauseOnExit);
  }

  // set bits per word
  ret = ioctl(fpSPI, SPI_IOC_WR_BITS_PER_WORD, &bits);
  if (ret == -1) {
    setConsoleColor(PRM_COLOR_RED);
    fpSPI = 0;
    fprintf(stderr, "\n\nerror in 'init_spi(%s)': set frame width %d failed, exit!\n\n", port, bits);
    Exit(1, g_pauseOnExit);
  }

  // wait 10ms
  usleep(10000);
  
  // return comm port handle
  return fpSPI;

#endif // __APPLE__ || __unix__

} // init_spi



/**
  \fn void close_spi(HANDLE *fpSPI)
   
  \brief close SPI port
  
  \param[in] fpSPI    handle to SPI port

  close & release SPI port. 
*/
void close_spi(HANDLE *fpSPI) {

/////////
// Win32
/////////
#if defined(WIN32)
  #error Windows not yet supported (which API?)

#endif // WIN32


/////////
// Posix
/////////
#if defined(__APPLE__) || defined(__unix__) 

  if (*fpSPI != 0) {
    if (close(*fpSPI) != 0) {
      setConsoleColor(PRM_COLOR_RED);
      *fpSPI = 0;
      fprintf(stderr, "\n\nerror in 'close_spi': close port failed, exit!\n\n");
      Exit(1, g_pauseOnExit);
    }
  }
  *fpSPI = 0;

#endif // __APPLE__ || __unix__

} // close_spi



/**
  \fn uint32_t send_spi(HANDLE fpSPI, uint32_t lenTx, char *Tx)
   
  \brief send data via SPI port
  
  \param[in] fpSPI    handle to SPI port
  \param[in] lenTx    number of bytes to send
  \param[in] Tx       array of bytes to send

  \return number of sent bytes
  
  send data via SPI port. Use this function to facilitate SPI communication
  on different platforms, e.g. Win32 and Posix.
*/
uint32_t send_spi(HANDLE fpSPI, uint32_t lenTx, char *Tx) {

  
/////////
// Win32
/////////
#ifdef WIN32
  #error Windows not yet supported (which API?)

#endif // WIN32


/////////
// Posix
/////////
#if defined(__APPLE__) || defined(__unix__) 

  uint32_t  numChars;
  
  // send data & return number of sent bytes
  numChars = write(fpSPI, Tx, lenTx);
  
  // just to be on the safe side...
  //SLEEP(5);

#endif // __APPLE__ || __unix__

  // return number of sent bytes
  return((uint32_t) numChars);

} // send_spi



/**
  \fn uint32_t receive_spi(HANDLE fpSPI, uint32_t lenRx, char *Rx)
   
  \brief receive data via SPI port
  
  \param[in]  fpSPI   handle to SPI port
  \param[in]  lenRx   number of bytes to receive
  \param[out] Rx      array containing bytes received
  
  \return number of received bytes
  
  receive data via SPI port. Use this function to facilitate SPI communication
  on different platforms, e.g. Win32 and Posix
*/
uint32_t receive_spi(HANDLE fpSPI, uint32_t lenRx, char *Rx) {

  
/////////
// Win32
/////////
#ifdef WIN32
  #error Windows not yet supported (which API?)

#endif // WIN32


/////////
// Posix
/////////
#if defined(__APPLE__) || defined(__unix__) 

  // send dummy bytes and read MISO back
  int len = read(fpSPI, Rx, lenRx);
  
  // just to be on the safe side...
  //SLEEP(5);
  
  // return number of received bytes
  return(len);
  
#endif // __APPLE__ || __unix__

} // receive_spi

// end of file
