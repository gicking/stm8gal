echo off
cls

REM download STM8 serial bootloader tool from https://github.com/gicking/stm8gal

REM set path to flash loader
set LOADER=".\stm8gal.exe"

REM set other parameters
set ADDRSTART="0x8000"
set ADDRSTOP="0x80FF"
set OUTFILE=".\memory_dump.txt"

REM set serial port
set PORT=COM11

REM use flash loader to read out memory
%LOADER% -p COM%PORT% -r %ADDRSTART% %ADDRSTOP% %OUTFILE% -v 2

echo.
PAUSE 
