echo off
cls

REM download STM8 serial bootloader tool from https://github.com/gicking/stm8gal

REM set path to flash loader
set LOADER=".\stm8gal.exe"

REM set other parameters
set FIRMWARE=".\dummy.s19"
REM set FIRMWARE=".\dummy.txt"

REM set serial port
REM set PORT=10
REM set PORT=15
set PORT=10

REM set optional UART mode (0=duplex, 1=1-wire reply, 2=2-wire reply)
REM set MODE=2

REM acccording to STM8 bootloader manual section 2.1, the minimum baudrate is 4800Baud. Default is 19200Baud
REM Typical baudrates are: 4800 9600 19200 38400 57600 115200 230400
set BAUD=230400 

REM if not explicitely set, configure auto-detect mode
IF NOT DEFINED MODE set MODE=255

REM for UART reply mode (MODE=1|2) limit baudrate. Found >19.2Baud is unreliable
IF NOT %MODE% == 0  set BAUD=19200

REM use flash loader to upload new SW
%LOADER% -p COM%PORT% -b %BAUD% -u %MODE% -w %FIRMWARE% -v -V 2

echo.
PAUSE 
