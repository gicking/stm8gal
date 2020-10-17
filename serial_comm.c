/**
  \file serial_comm.c
   
  \author G. Icking-Konert
  \date 2008-11-02
  \version 0.1
   
  \brief implementation of RS232 comm port routines
   
  implementation of of routines for RS232 communication using the Win32 or Posix API.
  For Win32, see e.g. http://msdn.microsoft.com/en-us/library/default.aspx
  For Posix see http://www.easysw.com/~mike/serial/serial.html
*/

// include files
#include "serial_comm.h"
#include "console.h"
#include "timer.h"
#if defined(__ARMEL__) && defined(USE_WIRING)
  #include <wiringPi.h>       // for reset via GPIO
#endif // __ARMEL__ && USE_WIRING



/**
  \fn void list_ports(void)
   
  print list of all available COM ports. I don't know how to do this in
  a better way than to actually try and open each of them...
*/
void list_ports(void) {
  
/////////
// Windows
/////////
#if defined(WIN32) || defined(WIN64)

  HANDLE        fpCom = NULL;
  uint16_t      i, j;
  char          port_tmp[100];
    
  // loop 1..255 over ports and list each available
  j=1;
  for (i=1; i<=255; i++) {

    // required to allow COM ports >COM9
    sprintf(port_tmp,"\\\\.\\COM%d", i);    
  
    // try to open COM-port
    fpCom = CreateFile(port_tmp,
      GENERIC_READ | GENERIC_WRITE,  // both read & write
      0,    // must be opened with exclusive-access
      NULL, // no security attributes
      OPEN_EXISTING, // must use OPEN_EXISTING
      0,    // not overlapped I/O
      NULL  // hTemplate must be NULL for comm devices
    );
    if (fpCom != INVALID_HANDLE_VALUE) {
      if (j!=1)
        console_print(STDOUT, ", ");
      console_print(STDOUT, "COM%d", i);
      CloseHandle(fpCom);
      j++;
    }
  }
  
#endif // WIN32 || WIN64


/////////
// Posix
/////////
#if defined(__APPLE__) || defined(__unix__) 

  // list all  (see http://bytes.com/groups/net-vc/545618-list-files-current-directory)
  uint16_t        i;
  struct dirent   *ent;
  DIR             *dir = opendir("/dev");
  if(dir) {
    i = 1;
    while((ent = readdir(dir)) != NULL) {
      
      // FTDI FT232 or CH340 based USB-RS232 adapter (MacOS X)
      if (strstr(ent->d_name, "tty.") && strstr(ent->d_name, "usbserial")) {
        if (i!=1)
          console_print(STDOUT, ", ");
        console_print(STDOUT, "/dev/%s", ent->d_name);
        i++;
      }

      // Prolific PL2303 based USB-RS232 adapter
      if (strstr(ent->d_name, "tty.PL2303")) {
        if (i!=1)
          console_print(STDOUT, ", ");
        console_print(STDOUT, "/dev/%s", ent->d_name);
        i++;
      }
      
      // FTDI FT232 based USB-RS232 adapter (Ubuntu)
      if (strstr(ent->d_name, "ttyUSB")) {
        if (i!=1)
          console_print(STDOUT, ", ");
        console_print(STDOUT, "/dev/%s", ent->d_name);
        i++;
      }
      
      // direct UART under Raspberry Pi / Raspbian 1+2
      if (strstr(ent->d_name, "ttyAMA")) {
        if (i!=1)
          console_print(STDOUT, ", ");
        console_print(STDOUT, "/dev/%s", ent->d_name);
        i++;
      }

      // direct UART under Raspberry Pi / Raspbian 3 (see https://raspberrypi.stackexchange.com/questions/45570/how-do-i-make-serial-work-on-the-raspberry-pi3)
      if (strstr(ent->d_name, "serial0")) {
        if (i!=1)
          console_print(STDOUT, ", ");
        console_print(STDOUT, "/dev/%s", ent->d_name);
        i++;
      }

    }
  }
  else
    Error("cannot list, check /dev for port name");

#endif // __APPLE__ || __unix__

} // list_ports



/**
  \fn HANDLE init_port(const char *port, uint32_t baudrate, uint32_t timeout, uint8_t numBits, uint8_t parity, uint8_t numStop, uint8_t RTS, uint8_t DTR)
   
  \param[in] port       name of port as string
  \param[in] baudrate   comm port speed in Baud (must be supported by port)
  \param[in] timeout    timeout between chars in ms
  \param[in] numBits    number of data bits per byte (7 or 8)
  \param[in] parity     parity control by HW (0=none, 1=odd, 2=even)
  \param[in] numStop    number of stop bits (1=1; 2=2; other=1.5)
  \param[in] RTS        Request To Send (required for some multimeter optocouplers)
  \param[in] DTR        Data Terminal Ready (required for some multimeter optocouplers)

  \return           handle to comm port
  
  open comm port for communication, set properties (baudrate, timeout,...). 
*/
HANDLE init_port(const char *port, uint32_t baudrate, uint32_t timeout, uint8_t numBits, uint8_t parity, uint8_t numStop, uint8_t RTS, uint8_t DTR) {

/////////
// Windows
/////////
#if defined(WIN32) || defined(WIN64)

  char          port_tmp[100];
  HANDLE        fpCom = NULL;
    
  // required to allow COM ports >COM9
  sprintf(port_tmp,"\\\\.\\%s", port);    
  
  // create handle to COM-port
  fpCom = CreateFile(port_tmp,
    GENERIC_READ | GENERIC_WRITE,  // both read & write
    0,    // must be opened with exclusive-access
    NULL, // no security attributes
    OPEN_EXISTING, // must use OPEN_EXISTING
    0,    // not overlapped I/O
    NULL  // hTemplate must be NULL for comm devices
  );
  if (fpCom == INVALID_HANDLE_VALUE)
    Error("in 'init_port(%s)': open port failed with code %d", port, (int) GetLastError());

  // reset COM port error buffer
  PurgeComm(fpCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);

#endif // WIN32 || WIN64


/////////
// Posix
/////////
#if defined(__APPLE__) || defined(__unix__) 

  HANDLE          fpCom;

  // open port
  fpCom = open(port, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
  if (fpCom == -1)
    Error("in 'init_port(%s)': open port failed", port);

#endif // __APPLE__ || __unix__
  
  // set port attributes
  set_port_attribute(fpCom, baudrate, timeout, numBits, parity, numStop, RTS, DTR);
  
  // return comm port handle
  return fpCom;

} // init_port



/**
  \fn void close_port(HANDLE *fpCom)
   
  \param[in] fpCom    handle to comm port

  close & release comm port. 
*/
void close_port(HANDLE *fpCom) {

/////////
// Windows
/////////
#if defined(WIN32) || defined(WIN64)

  BOOL      fSuccess;

  if (*fpCom != NULL) {
    fSuccess = CloseHandle(*fpCom);
    if (!fSuccess)
      Error("in 'close_port': close port failed with code %d", (int) GetLastError());
  }
  *fpCom = NULL;

#endif // WIN32 || WIN64


/////////
// Posix
/////////
#if defined(__APPLE__) || defined(__unix__) 

  // if open, close port
  if (*fpCom != 0) {
    if (close(*fpCom) != 0)
      Error("in 'close_port': close port failed");
  }
  *fpCom = 0;

#endif // __APPLE__ || __unix__

} // close_port



/**
  \fn void pulse_DTR(HANDLE *fpCom, uint32_t duration)
   
  \param[in] fpCom      port handle
  \param[in] duration   duration of DTR low pulse in ms

  generate low pulse on DTR in [ms] to reset STM8.
*/
void pulse_DTR(HANDLE fpCom, uint32_t duration) {
  
/////////
// Windows (see https://msdn.microsoft.com/en-us/library/windows/desktop/aa363254(v=vs.85).aspx)
/////////
#if defined(WIN32) || defined(WIN64)

  // set DTR
  EscapeCommFunction(fpCom, SETDTR);
  
  // wait specified duration
  SLEEP(duration);
  
  // clear DTR
  EscapeCommFunction(fpCom, CLRDTR);

#endif // WIN32 || WIN64


/////////
// Posix
/////////
#if defined(__APPLE__) || defined(__unix__) 

  int status;
  
  ioctl(fpCom, TIOCMGET, &status);

  // set DTR
  status |= TIOCM_DTR;
  if (ioctl(fpCom, TIOCMSET, &status))
    Error("in 'pulse_DTR()': cannot set DTS status");

  // wait specified duration
  SLEEP(duration);

  // clear DTR
  status &= ~TIOCM_DTR;
  if (ioctl(fpCom, TIOCMSET, &status))
    Error("in 'pulse_DTR()': cannot reset DTS status");

#endif // __APPLE__ || __unix__

} // pulse_DTR



/**
  \fn void pulse_RTS(HANDLE *fpCom, uint32_t duration)

  \param[in] fpCom      port handle
  \param[in] duration   duration of RTS low pulse in ms

  generate low pulse on RTS in [ms] to reset STM8.
*/
void pulse_RTS(HANDLE fpCom, uint32_t duration)
{

/////////
// Windows (see https://msdn.microsoft.com/en-us/library/windows/desktop/aa363254(v=vs.85).aspx)
/////////
#if defined(WIN32) || defined(WIN64)

    // set RTS
    EscapeCommFunction(fpCom, SETRTS);

    // wait specified duration
    SLEEP(duration);

    // clear RTS
    EscapeCommFunction(fpCom, CLRRTS);

#endif // WIN32 || WIN64

/////////
// Posix
/////////
#if defined(__APPLE__) || defined(__unix__)

    int status;

    ioctl(fpCom, TIOCMGET, &status);

    // set RTS
    status |= TIOCM_RTS;
    if(ioctl(fpCom, TIOCMSET, &status))
        Error("in 'pulse_RTS()': cannot set RTS status");

    // wait specified duration
    SLEEP(duration);

    // clear RTS
    status &= ~TIOCM_RTS;
    if(ioctl(fpCom, TIOCMSET, &status))
        Error("in 'pulse_RTS()': cannot reset RTS status");

#endif // __APPLE__ || __unix__

} // pulse_RTS



/**
  \fn void pulse_GPIO(int pin, uint32_t duration)
   
  \param[in] pin        reset pin (use header numbering)
  \param[in] duration   duration of DTR low pulse in ms

  Generate low pulse on GPIO in [ms] to reset STM8.
  Note: for non-root access add user to group gpio. See https://stackoverflow.com/questions/33831336/wiringpi-non-root-access-to-gpio
*/
#if defined(__ARMEL__) && defined(USE_WIRING)
void pulse_GPIO(int pin, uint32_t duration) {
  
  // initialize wiringPi. Use header numbering scheme
  wiringPiSetupPhys();
  
  // set direction of GPIO to output
  pinMode (pin, OUTPUT);

  // set GPIO low --> reset STM8
  digitalWrite (pin,  LOW);
  
  // wait specified duration [ms]
  SLEEP(duration);

  // set GPIO high --> start STM8
  digitalWrite (pin,  HIGH);
  
} // pulse_GPIO
#endif // __ARMEL__ && USE_WIRING



/**
  \fn void get_port_attribute(HANDLE fpCom, uint32_t *baudrate, uint32_t *timeout, uint8_t *numBits, uint8_t *parity, uint8_t *numStop, uint8_t *RTS, uint8_t *DTR)
   
  \param[in]  fpCom      handle to comm port
  \param[out] baudrate   comm port speed in Baud
  \param[out] timeout    timeout between chars in ms
  \param[out] numBits    number of data bits per byte (7 or 8)
  \param[out] parity     parity control by HW (0=none, 1=odd, 2=even)
  \param[out] numStop    number of stop bits (1=1; 2=2; 3=1.5)
  \param[out] RTS        Request To Send (required for some multimeter optocouplers)
  \param[out] DTR        Data Terminal Ready (required for some multimeter optocouplers)

  get current attributes of an already open comm port.
*/
void get_port_attribute(HANDLE fpCom, uint32_t *baudrate, uint32_t *timeout, uint8_t *numBits, uint8_t *parity, uint8_t *numStop, uint8_t *RTS, uint8_t *DTR) {

/////////
// Windows
/////////
#if defined(WIN32) || defined(WIN64)

  DCB           fDCB;
  COMMTIMEOUTS  fTimeout;
  BOOL          fSuccess;

  // get the current port configuration
  fSuccess = GetCommState(fpCom, &fDCB);
  if (!fSuccess)
    Error("in 'get_port_attribute': GetCommState() failed with code %d", (int) GetLastError());

  // get port settings
  *baudrate = fDCB.BaudRate;        // baud rate (19200, 57600, 115200)
  *numBits  = fDCB.ByteSize;        // number of data bits per byte
  *parity   = fDCB.Parity;          // bug in Win API, see https://stackoverflow.com/questions/36411498/fparity-member-of-dcb-structure-always-false-after-a-getcommstate
  *numStop  = fDCB.StopBits;        // number of stop bits
  if (fDCB.StopBits == ONESTOPBIT) 
    *numStop = 1;                      // 1 stop bit
  else if (fDCB.StopBits == TWOSTOPBITS) 
    *numStop = 2;                      // 2 stop bits
  else
    *numStop = 3;                      // 1.5 stop bits
  *RTS      = fDCB.fRtsControl;     // RTS off(=0=-12V) or on(=1=+12V)
  *DTR      = fDCB.fDtrControl;     // DTR off(=0=-12V) or on(=1=+12V)
  
  // get port timeout
  fSuccess = GetCommTimeouts(fpCom, &fTimeout);
  if (!fSuccess)
    Error("in 'get_port_attribute': GetCommTimeouts() failed with code %d", (int) GetLastError());
  *timeout = fTimeout.ReadTotalTimeoutConstant;       // this parameter fits also for timeout=0
  
#endif // WIN32 || WIN64


/////////
// Posix
/////////
#if defined(__APPLE__) || defined(__unix__) 

  struct termios  toptions;
  int             status;
  
  // get attributes
  if (tcgetattr(fpCom, &toptions) < 0)
    Error("in 'get_port_attribute': get port attributes failed");
  
  // get baudrate
  speed_t brate = cfgetospeed(&toptions);
  switch (brate) {
#ifdef B4800
    case B4800:    *baudrate = 4800;    break;
#endif
#ifdef B9600
    case B9600:    *baudrate = 9600;    break;
#endif
#ifdef B14400
    case B14400:   *baudrate = 14400;   break;
#endif
#ifdef B19200
    case B19200:   *baudrate = 19200;   break;
#endif
#ifdef B28800
    case B28800:   *baudrate = 28800;   break;
#endif
#ifdef B38400
    case B38400:   *baudrate = 38400;   break;
#endif
#ifdef B57600
    case B57600:   *baudrate = 57600;   break;
#endif
#ifdef B115200
    case B115200:  *baudrate = 115200;  break;
#endif
#ifdef B230400
    case B230400:  *baudrate = 230400;  break;
#endif
    default: *baudrate = UINT32_MAX;
  } // switch (brate)
  
  
  // get timeout (see: http://unixwiz.net/techtips/termios-vmin-vtime.html)
  *timeout = toptions.c_cc[VTIME] * 100;   // convert 0.1s to ms
  
  // number of data bits
  if ((toptions.c_cflag & CSIZE) == CS8)
    *numBits = 8;
  else
    *numBits = 7;
  
  
  // get parity bit
  if (!(toptions.c_cflag & PARENB))
    *parity = 0;
  else {
    if (toptions.c_cflag & PARODD)
      *parity = 1;
    else
      *parity = 2;
  }
  
  // get number of stop bits
  if (toptions.c_cflag & CSTOPB)
    *numStop = 2;
  else
    *numStop = 1;
  

  // get static RTS and DTR status (required for some multimeter optocouplers)
  ioctl(fpCom, TIOCMGET, &status);
  if (status & TIOCM_RTS)
    *RTS = 1;
  else
    *RTS = 0;
  if (status & TIOCM_DTR)
    *DTR = 1;
  else
    *DTR = 0;

#endif // __APPLE__ || __unix__
     
} // get_port_attribute



/**
  \fn void set_port_attribute(HANDLE fpCom, uint32_t baudrate, uint32_t timeout, uint8_t numBits, uint8_t parity, uint8_t numStop, uint8_t RTS, uint8_t DTR)
   
  \param[in] fpCom      handle to comm port
  \param[in] baudrate   comm port speed in Baud
  \param[in] timeout    timeout between chars in ms
  \param[in] numBits    number of data bits per byte (7 or 8)
  \param[in] parity     parity control by HW (0=none, 1=odd, 2=even)
  \param[in] numStop    number of stop bits (1=1; 2=2; 3=1.5)
  \param[in] RTS        Request To Send (required for some multimeter optocouplers)
  \param[in] DTR        Data Terminal Ready (required for some multimeter optocouplers)

  change attributes of an already open comm port.
*/
void set_port_attribute(HANDLE fpCom, uint32_t baudrate, uint32_t timeout, uint8_t numBits, uint8_t parity, uint8_t numStop, uint8_t RTS, uint8_t DTR) {
  
/////////
// Windows
/////////
#if defined(WIN32) || defined(WIN64)

  DCB           fDCB;
  BOOL          fSuccess;
  COMMTIMEOUTS  fTimeout;
  
  // reset COM port error buffer
  PurgeComm(fpCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
  
  // get the current port configuration
  fSuccess = GetCommState(fpCom, &fDCB);
  if (!fSuccess)
    Error("in 'set_port_attribute()': get port attributes failed with code %d", (int) GetLastError());

  // change port settings
  fDCB.BaudRate = baudrate;         // set the baud rate (19200, 57600, 115200)
  fDCB.ByteSize = numBits;          // number of data bits per byte
  if (parity) {
    fDCB.fParity = TRUE;            // Enable parity checking
    fDCB.Parity  = parity;          // 0-4=no,odd,even,mark,space
  }
  else {
    fDCB.fParity  = FALSE;          // disable parity checking
    fDCB.Parity   = NOPARITY;       // just to make sure
  }
  if (numStop == 1) 
    fDCB.StopBits = ONESTOPBIT;     // one stop bit
  else if (numStop == 2) 
    fDCB.StopBits = TWOSTOPBITS;    // two stop bit
  else
    fDCB.StopBits = ONE5STOPBITS;   // 1.5 stop bit
  fDCB.fRtsControl = (RTS != 0);    // RTS off(=0=-12V) or on(=1=+12V)
  fDCB.fDtrControl = (DTR != 0);    // DTR off(=0=-12V) or on(=1=+12V)

  // set new COM state
  fSuccess = SetCommState(fpCom, &fDCB);
  if (!fSuccess)
    Error("in 'set_port_attribute()': set port attributes failed with code %d", (int) GetLastError());


  // set timeouts for port to avoid hanging of program. For simplicity set all timeouts to same value.
  // For timeout=0 set values to query for buffer content
  if (timeout == 0)
    fTimeout.ReadIntervalTimeout        = MAXDWORD;    // --> no read timeout
  else
    fTimeout.ReadIntervalTimeout        = 0;           // max. ms between following read bytes (0=not used)
  fTimeout.ReadTotalTimeoutMultiplier   = 0;           // time per read byte (use contant timeout instead)
  fTimeout.ReadTotalTimeoutConstant     = timeout;     // total read timeout in ms
  fTimeout.WriteTotalTimeoutMultiplier  = 0;           // time per write byte (use contant timeout instead) 
  fTimeout.WriteTotalTimeoutConstant    = timeout;
  fSuccess = SetCommTimeouts(fpCom, &fTimeout);
  if (!fSuccess)
    Error("in 'set_port_attribute()': set port timeout failed with code %d", (int) GetLastError());

#endif // WIN32 || WIN64


/////////
// Posix
/////////
#if defined(__APPLE__) || defined(__unix__) 

  struct termios  toptions;
  int             status;
  
  
  // get attributes
  if (tcgetattr(fpCom, &toptions) < 0)
    Error("in 'set_port_attribute()': get port attributes failed");
  
  // set baudrate
  speed_t brate = baudrate; // let you override switch below if needed
  switch(baudrate) {
#ifdef B4800
    case 4800:   brate=B4800;   break;
#endif
#ifdef B9600
    case 9600:   brate=B9600;   break;
#endif
#ifdef B14400
    case 14400:  brate=B14400;  break;
#endif
#ifdef B19200
    case 19200:  brate=B19200;  break;
#endif
#ifdef B28800
    case 28800:  brate=B28800;  break;
#endif
#ifdef B38400
    case 38400:  brate=B38400;  break;
#endif
#ifdef B57600
    case 57600:  brate=B57600;  break;
#endif
#ifdef B115200
    case 115200: brate=B115200; break;
#endif
#ifdef B230400
    case 230400: brate=B230400; break;
#endif
    default: 
      Error("in 'set_port_attribute()': unsupported baudrate %d Baud", (int) baudrate);
  }
  cfmakeraw(&toptions);
  cfsetispeed(&toptions, brate);    // receive
  cfsetospeed(&toptions, brate);    // send
  
  // number of data bits
  toptions.c_cflag &= ~CSIZE;       // clear data bits entry
  if (numBits == 7)
    toptions.c_cflag |=  CS7;       // 7 data bits
  else if (numBits == 8)
    toptions.c_cflag |=  CS8;       // 8 data bits
  else
    Error("in 'set_port_attribute()': unknown number of data bits %d", (int) numBits);

  // parity bit (0=none, 1=odd, 2=even)
  if (parity == 0)
    toptions.c_cflag &= ~PARENB;    // 0=no parity
  else if (parity==1) {             // 1=odd parity
    toptions.c_cflag |=  PARENB;
    toptions.c_cflag |=  PARODD;
  }
  else if (parity==2) {             // even parity
    toptions.c_cflag |=  PARENB;
    toptions.c_cflag &=  ~PARODD;
  }
  else
    Error("in 'set_port_attribute()': unknown parity %d", (int) parity);

  // number of stop bits
  if (numStop == 1)
    toptions.c_cflag &= ~CSTOPB;    // one stop bit
  else
    toptions.c_cflag |= CSTOPB;     // two stop bit

  // disable flow control
  toptions.c_cflag &= ~CRTSCTS;
  toptions.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
  toptions.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl
  
  // make raw
  toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
  toptions.c_oflag &= ~OPOST; // make raw
  
  // set timeout (see: http://unixwiz.net/techtips/termios-vmin-vtime.html)
  toptions.c_cc[VMIN]  = 255;
  toptions.c_cc[VTIME] = timeout/100;   // convert ms to 0.1s

  // set term properties
  if (tcsetattr(fpCom, TCSANOW, &toptions) < 0)
    Error("in 'set_port_attribute()': set port attributes failed");
  
  
  // set static RTS and DTR status (required for some multimeter optocouplers)
  ioctl(fpCom, TIOCMGET, &status);
  if (RTS==1)
    status |= TIOCM_RTS;
  else
    status &= ~TIOCM_RTS;
  if (DTR==1)
    status |= TIOCM_DTR;
  else
    status &= ~TIOCM_DTR;
  if (ioctl(fpCom, TIOCMSET, &status))
    Error("in 'set_port_attribute()': cannot set RTS status");
  
#endif // __APPLE__ || __unix__

     
} // set_port_attribute



/**
  \fn void set_baudrate(HANDLE fpCom, uint32_t Baudrate)
   
  \param[in] fpCom      handle to comm port
  \param[in] Baudrate   new comm port speed in Baud (must be supported by port)

  set new baudrate for an already open comm port.
*/
void set_baudrate(HANDLE fpCom, uint32_t Baudrate) {
  
  uint32_t   baudrate, timeout;
  uint8_t    numBits, parity, numStop, RTS, DTR;

  // read port setting
  get_port_attribute(fpCom, &baudrate, &timeout, &numBits, &parity, &numStop, &RTS, &DTR);
  
  // set new baudrate
  baudrate = Baudrate;

  // change port setting
  set_port_attribute(fpCom, baudrate, timeout, numBits, parity, numStop, RTS, DTR);

} // set_baudrate



/**
  \fn void set_timeout(HANDLE fpCom, uint32_t Timeout)
   
  \param[in] fpCom      handle to comm port
  \param[in] Timeout    new timeout in ms

  set new timeout for an already open comm port
*/
void set_timeout(HANDLE fpCom, uint32_t Timeout) {
  
  uint32_t   baudrate, timeout;
  uint8_t    numBits, parity, numStop, RTS, DTR;

  // read port setting
  get_port_attribute(fpCom, &baudrate, &timeout, &numBits, &parity, &numStop, &RTS, &DTR);
  
  // set new timeout
  timeout = Timeout;

  // change port setting
  set_port_attribute(fpCom, baudrate, timeout, numBits, parity, numStop, RTS, DTR);

} // set_timeout



/**
  \fn void set_parity(HANDLE fpCom, uint8_t Parity)
   
  \param[in] fpCom      handle to comm port
  \param[in] Parity     parity control by HW (0=none, 1=odd, 2=even)

  set new parity for an already open comm port
*/
void set_parity(HANDLE fpCom, uint8_t Parity) {

  uint32_t   baudrate, timeout;
  uint8_t    numBits, parity, numStop, RTS, DTR;

  // read port setting
  get_port_attribute(fpCom, &baudrate, &timeout, &numBits, &parity, &numStop, &RTS, &DTR);

  // set new parity
  parity = Parity;

  // change port setting
  set_port_attribute(fpCom, baudrate, timeout, numBits, parity, numStop, RTS, DTR);

} // set_parity



/**
  \fn uint32_t send_port(HANDLE fpCom, uint8_t uartMode, uint32_t lenTx, char *Tx)
   
  \param[in] fpCom      handle to comm port
  \param[in] uartMode   UART bootloader mode: 0=duplex, 1=1-wire reply, 2=2-wire reply
  \param[in] lenTx      number of bytes to send
  \param[in] Tx         array of bytes to send

  \return number of sent bytes
  
  send data via comm port. Use this function to facilitate serial communication
  on different platforms, e.g. Win32 and Posix.
  If uartMode==1 (1-wire interface), read back LIN echo 
*/
uint32_t send_port(HANDLE fpCom, uint8_t uartMode, uint32_t lenTx, char *Tx) {

  // for reading back LIN echo 
  char      Rx[1000];
  uint32_t  lenRx;
  
  
/////////
// Windows
/////////
#if defined(WIN32) || defined(WIN64)

  DWORD   numChars;
  
  // send data & return number of sent bytes
  PurgeComm(fpCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
  WriteFile(fpCom, Tx, lenTx, &numChars, NULL);

#endif // WIN32 || WIN64


/////////
// Posix
/////////
#if defined(__APPLE__) || defined(__unix__) 

  uint32_t  numChars;
  
  // send data & return number of sent bytes
  numChars = write(fpCom, Tx, lenTx);

#endif // __APPLE__ || __unix__


  // for 1-wire UART interface, read back LIN echo and ignore
  if (uartMode == 1) {
    lenRx = receive_port(fpCom, uartMode, numChars, Rx);
    if (lenRx != numChars)
      Error("in 'send_port()': read 1-wire echo failed");
    //console_print(STDERR,"received echo %dB 0x%02x\n", (int) lenRx, Rx[0]);
  }
  
  // return number of sent bytes
  return((uint32_t) numChars);

} // send_port



/**
  \fn uint32_t receive_port(HANDLE fpCom, uint8_t uartMode, uint32_t lenRx, char *Rx)
   
  \param[in]  fpCom     handle to comm port
  \param[in]  uartMode  UART bootloader mode: 0=duplex, 1=1-wire reply, 2=2-wire reply
  \param[in]  lenRx     number of bytes to receive
  \param[out] Rx        array containing bytes received
  
  \return number of received bytes
  
  receive data via comm port. Use this function to facilitate serial communication
  on different platforms, e.g. Win32 and Posix
  If uartMode==2 (UART reply mode with 2-wire interface), reply each byte from STM8 -> SLOW
*/
uint32_t receive_port(HANDLE fpCom, uint8_t uartMode, uint32_t lenRx, char *Rx) {

  
/////////
// Windows
/////////
#if defined(WIN32) || defined(WIN64)

  DWORD     numChars, numTmp;
  uint32_t  i;
  
  // for UART reply mode with 2-wire interface echo each received bytes -> SLOW
  if (uartMode==2) {
    
    // echo each byte as it is received
    numChars = 0;
    for (i=0; i<lenRx; i++) {
      ReadFile(fpCom, Rx+i, 1, &numTmp, NULL);
      if (numTmp == 1) {
        numChars++;
        send_port(fpCom, uartMode, 1, Rx+i);
      }
      else
        break;
    } // loop i
  
  } // uartMode==2
  
  
  // UART duplex mode or 1-wire interface -> receive all bytes in single block -> fast
  else {
    ReadFile(fpCom, Rx, lenRx, &numChars, NULL);
  }
  
  // return number of bytes received
  return((uint32_t) numChars);

#endif // WIN32 || WIN64


/////////
// Posix
/////////
#if defined(__APPLE__) || defined(__unix__) 

  char            *dest = Rx;
  uint32_t        remaining = lenRx, got = 0, received = 0;
  struct          timeval tv;
  fd_set          fdr;
  struct termios  toptions;
  uint32_t        timeout;
  
  // get terminal attributes
  if (tcgetattr(fpCom, &toptions) < 0)
    Error("in 'receive_port': get port attributes failed");
  
  // get terminal timeout (see: http://unixwiz.net/techtips/termios-vmin-vtime.html)
  timeout = toptions.c_cc[VTIME] * 100;        // convert 0.1s to ms
  
  // while there are bytes left to read...
  while (remaining != 0) {
   
    // reinit timeval structure each time through loop
    tv.tv_sec  = (timeout / 1000L);
    tv.tv_usec = (timeout % 1000L) * 1000L;

    // wait for data to come in using select
    FD_ZERO(&fdr);
    FD_SET(fpCom, &fdr);
    if (select(fpCom + 1, &fdr, NULL, NULL, &tv) != 1) {
      return(received);
    }

    // read a response, we know there's data waiting
    got = read(fpCom, dest, remaining);
    
    // handle errors. retry on EAGAIN, fail on anything else, ignore if no bytes read
    if (got == -1) {
      if (errno == EAGAIN)
        continue;
      else
        return(received);
    } 
    else if (got > 0) {
      
      // for UART reply mode with 2-wire interface echo each byte
      if (uartMode==2) {
        //console_print(STDERR, "\nsent echo 0x%02x\n", *dest);
        send_port(fpCom, uartMode, 1, dest);
      }
      
      // figure out how many bytes are left and increment dest pointer through buffer
      dest += got;
      remaining -= got;
      received += got;
      
    } // received bytes

  } // while (remaining != 0)

  // return number of received bytes
  return(received);
  
#endif // __APPLE__ || __unix__

} // receive_port



/**
  \fn void flush_port(HANDLE fpCom)
   
  \param[in]  fpCom   handle to comm port
  
  flush port input & output buffer. Use this function to facilitate serial communication
  on different platforms, e.g. Win32 and Posix
*/
void flush_port(HANDLE fpCom) {

/////////
// Windows
/////////
#if defined(WIN32) || defined(WIN64)

  // purge all port buffers (see http://msdn.microsoft.com/en-us/library/windows/desktop/aa363428%28v=vs.85%29.aspx)
  PurgeComm(fpCom, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);

#endif // WIN32 || WIN64


/////////
// Posix
/////////
#if defined(__APPLE__) || defined(__unix__) 
  
  // purge all port buffers (see http://linux.die.net/man/3/tcflush)
  tcflush(fpCom, TCIOFLUSH);
  
#endif // __APPLE__ || __unix__

} // flush_port

// end of file
