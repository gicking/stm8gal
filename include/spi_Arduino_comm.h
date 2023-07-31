/**
  \file spi_Arduino_comm.h

  \author G. Icking-Konert

  \brief declaration of SPI routines via Arduino USB<->SPI bridge

  declaration of routines for SPI communication using the Arduino
  USB<->SPI bridge available from https://github.com/gicking/Arduino_SPI_bridge
*/

// for including file only once
#ifndef _SPI_ARDUINO_COMM_H_
#define _SPI_ARDUINO_COMM_H_

// includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "serial_comm.h"

// defines
#define ARDUINO_BAUDRATE             115200 //< USB speed to Arduino
#define ARDUINO_RESET_PIN            8      //< Arduino pin used for STM8 reset
#define ARDUINO_CSN_PIN              10     //< Arduino pin used for chip select

#define ARDUINO_CMD_CONFIG_SPI       0x00   //< configure SPI, e.g. baudrate and poratity
#define ARDUINO_CMD_SET_PIN          0x01   //< set state of pin, e.g chip select or reset
#define ARDUINO_CMD_SEND_RECEIVE     0x02   //< send/receive message via SPI

#define ARDUINO_SUCCESS              0x01   //< command ok
#define ARDUINO_ERROR_FRAME_LENGTH   0x02   //< zero or loo long frame length
#define ARDUINO_ERROR_CHECKSUM       0x03   //< received and calculated checksums don't match
#define ARDUINO_ERROR_ILLEGAL_CMD    0x04   //< command unknown
#define ARDUINO_ERROR_ILLEGAL_PARAM  0x05   //< error with command parameters

//#define ARDUINO_LSBFIRST             0      //< bit order LSB first
#define ARDUINO_MSBFIRST             1      //< bit order MSB first
#define ARDUINO_SPI_MODE0            0x00   //< SPI mode (see Arduino SPI reference)
//#define ARDUINO_SPI_MODE1            0x04
//#define ARDUINO_SPI_MODE2            0x08
//#define ARDUINO_SPI_MODE3            0x0C

#define send_spi_Arduino(port,len,buf)     sendReceiveSPI_Arduino(port,ARDUINO_CSN_PIN,len,buf,NULL)
#define receive_spi_Arduino(port,len,buf)  sendReceiveSPI_Arduino(port,ARDUINO_CSN_PIN,len,NULL,buf)

/// configure Arduino SPI for bridge
void      configSPI_Arduino(HANDLE fp, uint32_t baudrateSPI, uint8_t bitOrder, uint8_t mode);

/// set pin on Arduino SPI bridge
void      setPin_Arduino(HANDLE fp, uint8_t pin, uint8_t state);

/// send/receive SPI frames via Arduino USB<->SPI bridge
uint32_t  sendReceiveSPI_Arduino(HANDLE fp, uint8_t pin, uint32_t lenFrame, char *bufTx, char *bufRx);

#endif // _SPI_ARDUINO_COMM_H_

// end of file
