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
#include "serial_comm.h"
#include "misc.h"
#include "globals.h"



/**
  \fn uint8_t checksum_Arduino(uint8_t *buf)
   
  \brief calculate checksum over frame (w/o checksum)
  
  \param[in] buf   buffer containing frame. buf[0] contains total frame length
  
  Calculate checksum over frame (inverted XOR over bytes w/o checksum).
*/
uint8_t checksum_Arduino(uint8_t *buf) {
  
  uint8_t chk = 0xFF, i;
  uint8_t lim = buf[0]-1;   // assert only one minus
  for (i=0; i<lim; i++)
    chk ^= buf[i];

  return(chk);
  
} // checksum_Arduino()



/**
  \fn void configSPI_Arduino(HANDLE fp, uint32_t baudrateSPI)
   
  \brief configure Arduino SPI for bridge
  
  \param[in] fp            handle to Arduino port
  \param[in] baudrateSPI   SPI baudrate
  \param[in] bitOrder      SPI bit order (LSB/MSB)
  \param[in] mode          SPI polarity & clock phase (see Arduino SPI reference)
  
  configure SPI interface of Arduino SPI bridge
  For serial protocol see https://github.com/gicking/Arduino_SPI_bridge/protocol.ods 
*/
void configSPI_Arduino(HANDLE fp, uint32_t baudrateSPI, uint8_t bitOrder, uint8_t mode) {

  uint8_t  Tx[150], Rx[150];
  uint8_t  lenRx, num;
  
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
  send_port(fp, 0, Tx[0], (char*) Tx);
  
  // get response from Arduino
  lenRx = 3;
  num = receive_port(fp, 0, lenRx, (char*) Rx);
  
  // check for timeout
  if (num != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'configSPI_Arduino()': response timeout, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  // check length
  if (Rx[0] != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'configSPI_Arduino()': wrong frame length (expect %d, read %d), exit!\n\n", (int) lenRx, (int) (Rx[0]));
    Exit(1, g_pauseOnExit);
  }
      
  // check checksum
  if (Rx[lenRx-1] != checksum_Arduino(Rx)) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'configSPI_Arduino()': checksum error (expect 0x%02x, read 0x%02x), exit!\n\n", Rx[lenRx-1], checksum_Arduino(Rx));
    Exit(1, g_pauseOnExit);
  }
      
  // check for ACK
  if (Rx[1] != ARDUINO_SUCCESS) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'configSPI_Arduino()': acknowledge error (expect 0x%02x, read 0x%02x), exit!\n\n", ARDUINO_SUCCESS, Rx[1]);
    Exit(1, g_pauseOnExit);
  }

} // configSPI_Arduino



/**
  \fn void setPin_Arduino(HANDLE fp, uint8_t pin, bool state)
   
  \brief set pin on Arduino SPI bridge
  
  \param[in] fp     handle to Arduino port
  \param[in] pin    Arduino pin to set
  \param[in] state  state for pin
  
  set pin on Arduino SPI bridge using serial command. 
  For serial protocol see https://github.com/gicking/Arduino_SPI_bridge/protocol.ods  
*/
void setPin_Arduino(HANDLE fp, uint8_t pin, uint8_t state) {

  uint8_t  Tx[150], Rx[150];
  uint8_t  lenRx, num;
  
  // assemble frame
  Tx[0] = 5;                    // frame length
  Tx[1] = ARDUINO_CMD_SET_PIN;  // command code
  Tx[2] = pin;                  // pin number
  Tx[3] = state;                // new state
  Tx[4] = checksum_Arduino(Tx); // frame checksum
  
  // send command to Arduino
  send_port(fp, 0, Tx[0], (char*) Tx);
  
  // get response from Arduino
  lenRx = 3;
  num = receive_port(fp, 0, lenRx, (char*) Rx);
  
  // check for timeout
  if (num != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'setPin_Arduino()': response timeout, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  // check length
  if (Rx[0] != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'setPin_Arduino()': wrong frame length (expect %d, read %d), exit!\n\n", (int) lenRx, (int) (Rx[0]));
    Exit(1, g_pauseOnExit);
  }
      
  // check checksum
  if (Rx[lenRx-1] != checksum_Arduino(Rx)) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'setPin_Arduino()': checksum error (expect 0x%02x, read 0x%02x), exit!\n\n", Rx[lenRx-1], checksum_Arduino(Rx));
    Exit(1, g_pauseOnExit);
  }
      
  // check for ACK
  if (Rx[1] != ARDUINO_SUCCESS) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'setPin_Arduino()': acknowledge error (expect 0x%02x, read 0x%02x), exit!\n\n", ARDUINO_SUCCESS, Rx[1]);
    Exit(1, g_pauseOnExit);
  }

} // setPin_Arduino



/**
  \fn uint32_t sendReceiveSPI_Arduino(HANDLE fpSPI, uint8_t CSN, uint32_t lenFrame, char *Tx, char *Rx)
   
  \brief send/receive SPI frames via Arduino USB<->SPI bridge
  
  \param[in]  fp        handle to Arduino port
  \param[in]  CSN       Arduino pin used as chip-select
  \param[in]  lenFrame  number of SPI bytes to send
  \param[in]  bufTx     array of SPI bytes to send
  \param[out] bufRx     array of received SPI bytes
  
  \return number of received SPI bytes

  send/receive SPI frames via Arduino USB<->SPI bridge
  For serial protocol see https://github.com/gicking/Arduino_SPI_bridge/protocol.ods  
*/
uint32_t sendReceiveSPI_Arduino(HANDLE fp, uint8_t CSN, uint32_t lenFrame, char *bufTx, char *bufRx) {

  uint8_t  Tx[150], Rx[150];
  uint8_t  lenRx, num;
  int      i;
  
  // assemble frame
  Tx[0] = 4+lenFrame;                 // frame length
  Tx[1] = ARDUINO_CMD_SEND_RECEIVE;   // command code
  Tx[2] = CSN;                        // chip select pin
  for (i=0; i<lenFrame; i++) {    // copy MOSI bytes 
    if (bufTx != NULL)
      Tx[3+i] = bufTx[i];
    else
      Tx[3+i] = 0x00;
  }
  Tx[Tx[0]-1] = checksum_Arduino(Tx); // frame checksum
  
  // send command to Arduino
  send_port(fp, 0, Tx[0], (char*) Tx);
  
  // get response from Arduino
  lenRx = 3+lenFrame;
  num = receive_port(fp, 0, lenRx, (char*) Rx);
  
  // check for timeout
  if (num != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'sendReceiveSPI_Arduino()': response timeout, exit!\n\n");
    Exit(1, g_pauseOnExit);
  }

  // check length
  if (Rx[0] != lenRx) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'sendReceiveSPI_Arduino()': wrong frame length (expect %d, read %d), exit!\n\n", (int) lenRx, (int) (Rx[0]));
    Exit(1, g_pauseOnExit);
  }
      
  // check checksum
  if (Rx[lenRx-1] != checksum_Arduino(Rx)) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'sendReceiveSPI_Arduino()': checksum error (expect 0x%02x, read 0x%02x), exit!\n\n", Rx[lenRx-1], checksum_Arduino(Rx));
    Exit(1, g_pauseOnExit);
  }
      
  // check for ACK
  if (Rx[1] != ARDUINO_SUCCESS) {
    setConsoleColor(PRM_COLOR_RED);
    fprintf(stderr, "\n\nerror in 'sendReceiveSPI_Arduino()': acknowledge error (expect 0x%02x, read 0x%02x), exit!\n\n", ARDUINO_SUCCESS, Rx[1]);
    Exit(1, g_pauseOnExit);
  }
  
  // copy MISO bytes
  if (bufRx != NULL) {
    for (i=0; i<lenFrame; i++)
      bufRx[i] = Rx[2+i];
  }
  
  // debug SPI data
  #if defined(DEBUG)
    fprintf(stderr, "n  MOSI  MISO\n");
    for (i=0; i<lenFrame; i++)
      fprintf(stderr, "%d  0x%02x  0x%02x\n", i, Tx[3+i], Rx[2+i]);
    fprintf(stderr, "\n");  
  #endif

  // return number of SPI bytes
  return(lenFrame);
  
} // sendReceiveSPI_Arduino

// end of file
