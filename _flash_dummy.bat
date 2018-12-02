echo off
cls

REM download STM8 serial bootloader tool from https://github.com/gicking/stm8gal

REM set path to flash loader, COM port number, etc.
set LOADER=".\stm8gal.exe"
set PORT=6
set FIRMWARE=".\dummy.s19"
REM set FIRMWARE=".\dummy.txt"

REM acccording to STM8 bootloader manual section 2.1, the minimum baudrate is 4800Baud
REM set BAUD=4800
REM set BAUD=9600
REM set BAUD=19200
set BAUD=38400
REM set BAUD=57600 
REM set BAUD=115200 
REM set BAUD=230400 

REM use flash loader to upload new SW
%LOADER% -p COM%PORT% -b %BAUD% -w %FIRMWARE% -v -V 2

echo.
PAUSE 
