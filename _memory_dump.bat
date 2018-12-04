echo off
cls

REM download STM8 serial bootloader tool from https://github.com/gicking/stm8gal

REM set path to flash loader
set LOADER=".\stm8gal.exe"

REM set other parameters
set ADDRSTART="0x8000"
set ADDRSTOP="0x8100"
set OUTFILE=".\dump.txt"

REM set COM port and UART mode (0=duplex, 1=1-wire reply, 2=2-wire reply)
REM set PORT=10 & set MODE=0
REM set PORT=15 & set MODE=1
set PORT=10 & set MODE=2

REM acccording to STM8 bootloader manual section 2.1, the minimum baudrate is 4800Baud.
REM Typical baudrates are: 4800 9600 19200 38400 57600 115200 230400
set BAUD=230400 

REM for UART reply mode (MODE=1|2) limit baudrate. Found >19.2Baud is unreliable
IF NOT %MODE% == 0  set BAUD=19200

REM use flash loader to read out memory
%LOADER% -p COM%PORT% -b %BAUD% -u %MODE% -r %ADDRSTART% %ADDRSTOP% %OUTFILE% -V 2

echo.
PAUSE 
