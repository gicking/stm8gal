/**
  \file spi_Arduino_comm.c

  \author G. Icking-Konert
  \date 2017-12-19
  \version 0.1

  \brief implementation of SPI routines voa Arduino USB<->SPI bridge

  implementation of routines for SPI communication using the Arduino
  USB<->SPI bridge available from https://github.com/gicking/Arduino_SPI_bridge
*/

// include files
#include "spi_Arduino_comm.h"
#include "console.h"
#include "main.h"
#include "serial_comm.h"
#include "timer.h"


STM8gal_SpiArduinoErrors_t g_SpiArduinoCommsLastError = STM8GAL_SPI_ARDUINO_NO_ERROR;

char * g_SpiArduinoCommsErrorStrings[STM8GAL_SPI_ARDUINO_NO_ERROR+1] =
{
    "No Error",                             //  STM8GAL_SPI_ARDUINO_NO_ERROR = 0,
    "Response Timeout",                     //  STM8GAL_SPI_ARDUINO_RESPONSE_TIMEOUT,
    "Wrong Framelength",                    //  STM8GAL_SPI_ARDUINO_FRAMELENGTH,
    "Checksum Error",                       //  STM8GAL_SPI_ARDUINO_CHECKSUM,
    "Acknowledge Error",                    //  STM8GAL_SPI_ARDUINO_ACKNOWLEDGE,
};


/**
  \fn uint8_t checksum_Arduino(uint8_t *buf)

  \param[in] buf   buffer containing frame. buf[0] contains total frame length

  Calculate checksum over frame (inverted XOR over bytes w/o checksum).
*/
uint8_t checksum_Arduino(uint8_t *buf)
{
  uint8_t chk = 0xFF, i;
  uint8_t lim = buf[0]-1;   // assert only one minus
  for (i=0; i<lim; i++)
    chk ^= buf[i];

  return(chk);

} // checksum_Arduino()



/**
  \fn STM8gal_SpiArduinoErrors_t configSPI_Arduino(HANDLE fp, uint32_t baudrateSPI, uint8_t bitOrder, uint8_t mode)

  \param[in] fp            handle to Arduino port
  \param[in] baudrateSPI   SPI baudrate
  \param[in] bitOrder      SPI bit order (LSB/MSB)
  \param[in] mode          SPI polarity & clock phase (see Arduino SPI reference)

  \return operation status (STM8gal_SpiArduinoErrors_t)

  configure SPI interface of Arduino SPI bridge
  For serial protocol see https://github.com/gicking/Arduino_SPI_bridge/protocol.ods
*/
STM8gal_SpiArduinoErrors_t configSPI_Arduino(HANDLE fp, uint32_t baudrateSPI, uint8_t bitOrder, uint8_t mode)
{
  uint8_t   Tx[150], Rx[150];
  uint8_t   lenRx;
  uint32_t  numChars;

  // assemble frame
  Tx[0] = 9;                              // frame length
  Tx[1] = ARDUINO_CMD_CONFIG_SPI;         // command code
  Tx[2] = (uint8_t) (baudrateSPI >> 24);  // baudrate MSB
  Tx[3] = (uint8_t) (baudrateSPI >> 16);
  Tx[4] = (uint8_t) (baudrateSPI >>  8);
  Tx[5] = (uint8_t) (baudrateSPI >>  0);  // ...LSB
  Tx[6] = bitOrder;                       // SPI bit order
  Tx[7] = mode;                           // polarity & clock phase
  Tx[8] = checksum_Arduino(Tx);           // frame checksum

  // send command to Arduino
  send_port(fp, 0, Tx[0], (char*) Tx, &numChars);

  // get response from Arduino
  lenRx = 3;
  receive_port(fp, 0, lenRx, (char*) Rx, &numChars);

  // check for timeout
  if (numChars != lenRx)
  {
    g_SpiArduinoCommsLastError = STM8GAL_SPI_ARDUINO_RESPONSE_TIMEOUT;
    console_print(STDERR, "in 'configSPI_Arduino()': response timeout");
    return(g_SpiArduinoCommsLastError);
  }

  // check length
  if (Rx[0] != lenRx)
  {
    g_SpiArduinoCommsLastError = STM8GAL_SPI_ARDUINO_FRAMELENGTH;
    console_print(STDERR, "in 'configSPI_Arduino()': wrong frame length (expect %d, read %d)", (int) lenRx, (int) (Rx[0]));
    return(g_SpiArduinoCommsLastError);
  }

  // check checksum
  if (Rx[lenRx-1] != checksum_Arduino(Rx))
  {
    g_SpiArduinoCommsLastError = STM8GAL_SPI_ARDUINO_CHECKSUM;
    console_print(STDERR, "in 'configSPI_Arduino()': checksum error (expect 0x%02x, read 0x%02x)", Rx[lenRx-1], checksum_Arduino(Rx));
    return(g_SpiArduinoCommsLastError);
  }

  // check for ACK
  if (Rx[1] != ARDUINO_SUCCESS)
  {
    g_SpiArduinoCommsLastError = STM8GAL_SPI_ARDUINO_CHECKSUM;
    console_print(STDERR, "in 'configSPI_Arduino()': acknowledge error (expect 0x%02x, read 0x%02x)", ARDUINO_SUCCESS, Rx[1]);
    return(g_SpiArduinoCommsLastError);
  }

  return(g_SpiArduinoCommsLastError);

} // configSPI_Arduino()



/**
  \fn STM8gal_SpiArduinoErrors_t setPin_Arduino(HANDLE fp, uint8_t pin, bool state)

  \param[in] fp     handle to Arduino port
  \param[in] pin    Arduino pin to set
  \param[in] state  state for pin

  \return operation status (STM8gal_SpiArduinoErrors_t)

  set pin on Arduino SPI bridge using serial command.
  For serial protocol see https://github.com/gicking/Arduino_SPI_bridge/protocol.ods
*/
STM8gal_SpiArduinoErrors_t setPin_Arduino(HANDLE fp, uint8_t pin, uint8_t state)
{
  uint8_t   Tx[150], Rx[150];
  uint8_t   lenRx;
  uint32_t  numChars;


  // assemble frame
  Tx[0] = 5;                    // frame length
  Tx[1] = ARDUINO_CMD_SET_PIN;  // command code
  Tx[2] = pin;                  // pin number
  Tx[3] = state;                // new state
  Tx[4] = checksum_Arduino(Tx); // frame checksum

  // send command to Arduino
  send_port(fp, 0, Tx[0], (char*) Tx, &numChars);

  // get response from Arduino
  lenRx = 3;
  receive_port(fp, 0, lenRx, (char*) Rx, &numChars);

  // check for timeout
  if (numChars != lenRx)
  {
    g_SpiArduinoCommsLastError = STM8GAL_SPI_ARDUINO_RESPONSE_TIMEOUT;
    console_print(STDERR, "in 'setPin_Arduino()': response timeout");
    return(g_SpiArduinoCommsLastError);
  }

  // check length
  if (Rx[0] != lenRx)
  {
    g_SpiArduinoCommsLastError = STM8GAL_SPI_ARDUINO_FRAMELENGTH;
    console_print(STDERR, "in 'setPin_Arduino()': wrong frame length (expect %d, read %d)", (int) lenRx, (int) (Rx[0]));
    return(g_SpiArduinoCommsLastError);
  }

  // check checksum
  if (Rx[lenRx-1] != checksum_Arduino(Rx))
  {
    g_SpiArduinoCommsLastError = STM8GAL_SPI_ARDUINO_CHECKSUM;
    console_print(STDERR, "in 'setPin_Arduino()': checksum error (expect 0x%02x, read 0x%02x)", Rx[lenRx-1], checksum_Arduino(Rx));
    return(g_SpiArduinoCommsLastError);
  }

  // check for ACK
  if (Rx[1] != ARDUINO_SUCCESS)
  {
    g_SpiArduinoCommsLastError = STM8GAL_SPI_ARDUINO_CHECKSUM;
    console_print(STDERR, "in 'setPin_Arduino()': acknowledge error (expect 0x%02x, read 0x%02x)", ARDUINO_SUCCESS, Rx[1]);
    return(g_SpiArduinoCommsLastError);
  }

  return(g_SpiArduinoCommsLastError);

} // setPin_Arduino



/**
  \fn uint32_t sendReceiveSPI_Arduino(HANDLE fpSPI, uint8_t CSN, uint32_t lenFrame, char *Tx, char *Rx)

  \param[in]  fp        handle to Arduino port
  \param[in]  CSN       Arduino pin used as chip-select
  \param[in]  lenFrame  number of SPI bytes to send
  \param[in]  bufTx     array of SPI bytes to send
  \param[out] bufRx     array of received SPI bytes
  \param[out] numRx     number of received SPI bytes

  \return operation status (STM8gal_SpiArduinoErrors_t)

  send/receive SPI frames via Arduino USB<->SPI bridge
  For serial protocol see https://github.com/gicking/Arduino_SPI_bridge/protocol.ods
*/
STM8gal_SpiArduinoErrors_t sendReceiveSPI_Arduino(HANDLE fp, uint8_t CSN, uint32_t lenFrame, char *bufTx, char *bufRx, uint32_t *numRx)
{
  uint8_t   Tx[150], Rx[150];
  uint8_t   lenRx;
  uint32_t  numChars;
  int       i;

  // assemble frame
  Tx[0] = 4+lenFrame;                 // frame length
  Tx[1] = ARDUINO_CMD_SEND_RECEIVE;   // command code
  Tx[2] = CSN;                        // chip select pin
  for (i=0; i<lenFrame; i++)          // copy MOSI bytes
  {
    if (bufTx != NULL)
      Tx[3+i] = bufTx[i];
    else
      Tx[3+i] = 0x00;
  }
  Tx[Tx[0]-1] = checksum_Arduino(Tx); // frame checksum

  // send command to Arduino
  send_port(fp, 0, Tx[0], (char*) Tx, &numChars);

  // get response from Arduino
  lenRx = 3+lenFrame;
  receive_port(fp, 0, lenRx, (char*) Rx, &numChars);

  // check for timeout
  if (numChars != lenRx)
    Error("in 'sendReceiveSPI_Arduino()': response timeout");

  // check length
  if (Rx[0] != lenRx)
    Error("in 'sendReceiveSPI_Arduino()': wrong frame length (expect %d, read %d)", (int) lenRx, (int) (Rx[0]));

  // check checksum
  if (Rx[lenRx-1] != checksum_Arduino(Rx))
    Error("in 'sendReceiveSPI_Arduino()': checksum error (expect 0x%02x, read 0x%02x)", Rx[lenRx-1], checksum_Arduino(Rx));

  // check for ACK
  if (Rx[1] != ARDUINO_SUCCESS)
    Error("in 'sendReceiveSPI_Arduino()': acknowledge error (expect 0x%02x, read 0x%02x)", ARDUINO_SUCCESS, Rx[1]);

  // copy MISO bytes
  if (bufRx != NULL)
  {
    for (i=0; i<lenFrame; i++)
      bufRx[i] = Rx[2+i];
  }

  // debug SPI data
  #if defined(DEBUG)
    console_print(STDERR, "n  MOSI  MISO\n");
    for (i=0; i<lenFrame; i++)
      console_print(STDERR, "%d  0x%02x  0x%02x\n", i, Tx[3+i], Rx[2+i]);
    console_print(STDERR, "\n");
  #endif

  // return number of SPI bytes
  return(lenFrame);

} // sendReceiveSPI_Arduino



/**
  \fn STM8gal_SpiArduinoErrors_t SPI_Arduino_GetLastError(void)

  return last error in the SPI_Arduino module
*/
STM8gal_SpiArduinoErrors_t SPI_Arduino_GetLastError(void)
{
  return(g_serialCommsLastError);
}


/**
  \fn const char * SPI_Arduino_GetLastErrorString(void)

  return last error string in the SPI_Arduino module
*/
const char * SPI_Arduino_GetLastErrorString(void)
{
  return(g_SpiArduinoCommsErrorStrings[SPI_Arduino_GetLastError()]);
}

// end of file
