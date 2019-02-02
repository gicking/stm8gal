echo off
cls

REM download STM8 serial bootloader tool from https://github.com/gicking/stm8gal

REM set path to flash loader
set LOADER=".\stm8gal.exe"

REM set other parameters
set ADDRSTART="0x4800"
set ADDRSTOP="0x487F"
set OUTFILE=".\OPT_dump.txt"

REM set serial port
set PORT=COM11

REM use flash loader to read out memory
%LOADER% -p %PORT% -r %ADDRSTART% %ADDRSTOP% %OUTFILE% -V 2

echo.
PAUSE 
