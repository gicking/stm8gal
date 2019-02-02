echo off
cls

REM set make tool (if not in PATH, set complete path)
set MAKE=mingw32-make

REM use SDCC makefile to delete output
%MAKE% -f Makefile clean

REM delete DevC++ objects. Keep Objects folder
rd /S /Q .\Objects
mkdir .\Objects

REM make application
%MAKE% -f Makefile.win

PAUSE
echo on
