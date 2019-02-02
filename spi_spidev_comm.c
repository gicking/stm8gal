/**
  \file spi_spidev_comm.c
   
  \author G. Icking-Konert
  \date 2017-12-10
  \version 0.1
   
  \brief implementation of SPI comm routines via spidev library
   
  implementation of routines for SPI communication using the spidev library.
  
*/

// include files
#include "spi_spidev_comm.h"
#include "main.h"
#include "misc.h"



/**
  \fn HANDLE init_spi_spidev(const char *port, uint32_t baudrate)
   
  \param[in] port       name of port as string
  \param[in] baudrate   SPI port speed in Baud

  \return           handle to SPI port
  
  open SPI port for communication, set properties (baudrate, polarity,...). 
*/
HANDLE init_spi_spidev(const char *port, uint32_t baudrate) {

  
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

  HANDLE     fp;
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
  fp = open(port, O_RDWR);
  if (fp < 0)
    Error("in 'init_spi_spidev(%s)': open port failed", port);


  //////
  // change properties
  //////
  
  // set SPI mode
  ret = ioctl(fp, SPI_IOC_WR_MODE32, &mode);
  if (ret == -1)
    Error("in 'init_spi_spidev(%s)': set mode %d failed", port, mode);

  // set SPI speed
  ret = ioctl(fp, SPI_IOC_WR_MAX_SPEED_HZ, &baudrate);
  if (ret == -1)
    Error("in 'init_spi_spidev(%s)': set speed %d failed", port, baudrate);

  // set bits per word
  ret = ioctl(fp, SPI_IOC_WR_BITS_PER_WORD, &bits);
  if (ret == -1)
    Error("in 'init_spi_spidev(%s)': set frame width %d failed", port, bits);

  // wait 10ms
  usleep(10000);
  
  // return comm port handle
  return fp;

#endif // __APPLE__ || __unix__

} // init_spi_spidev



/**
  \fn void close_spi_spidev(HANDLE *fp)

  \param[in] fp    handle to SPI port

  close & release SPI port. 
*/
void close_spi_spidev(HANDLE *fp) {

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

  // if open, close port
  if (*fp != 0) {
    if (close(*fp) != 0)
      Error("in 'close_spi_spidev': close port failed");
  }
  *fp = 0;

#endif // __APPLE__ || __unix__

} // close_spi_spidev



/**
  \fn uint32_t send_spi_spidev(HANDLE fp, uint32_t lenTx, char *Tx)

  \param[in] fp       handle to SPI port
  \param[in] lenTx    number of bytes to send
  \param[in] Tx       array of bytes to send

  \return number of sent bytes
  
  send data via SPI port. Use this function to facilitate SPI communication
  on different platforms, e.g. Win32 and Posix.
*/
uint32_t send_spi_spidev(HANDLE fp, uint32_t lenTx, char *Tx) {

  
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
  numChars = write(fp, Tx, lenTx);
  
  // just to be on the safe side...
  //SLEEP(5);

#endif // __APPLE__ || __unix__

  // return number of sent bytes
  return((uint32_t) numChars);

} // send_spi_spidev



/**
  \fn uint32_t receive_spi_spidev(HANDLE fp, uint32_t lenRx, char *Rx)
  
  \param[in]  fp      handle to SPI port
  \param[in]  lenRx   number of bytes to receive
  \param[out] Rx      array containing bytes received
  
  \return number of received bytes
  
  receive data via SPI port. Use this function to facilitate SPI communication
  on different platforms, e.g. Win32 and Posix
*/
uint32_t receive_spi_spidev(HANDLE fp, uint32_t lenRx, char *Rx) {

  
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
  int len = read(fp, Rx, lenRx);
  
  // just to be on the safe side...
  //SLEEP(5);
  
  // return number of received bytes
  return(len);
  
#endif // __APPLE__ || __unix__

} // receive_spi_spidev



/**
  \fn void spi_transfer(HANDLE fp, int len, uint8_t *Tx, uint8_t *Rx)
   
  \param[in]  fp      handle to SPI port
  \param[in]  len     number of bytes to send & receive
  \param[in]  Tx      array containing bytes to send
  \param[out] Rx      array containing bytes received
  
  \return number of received bytes
  
  send & receive data via SPI port. Use this function to facilitate SPI communication
  on different platforms, e.g. Win32 and Posix
*/
void spi_transfer(HANDLE fp, int len, uint8_t *Tx, uint8_t *Rx) {

  
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

  // fill SPI data structure
  struct spi_ioc_transfer tr = {
    .tx_buf = (unsigned long)Tx,
    .rx_buf = (unsigned long)Rx,
    .len = len,
    .delay_usecs = 4,
    .speed_hz = 0,
    .bits_per_word = 0,
  };

  // send & receive data
  int ret = ioctl(fp, SPI_IOC_MESSAGE(1), &tr);
  if (ret != 0)
    Error("in 'spi_transfer': cannot transfer data (code %d)", ret);
  
#endif // __APPLE__ || __unix__

} // receive_spi_spidev

// end of file
