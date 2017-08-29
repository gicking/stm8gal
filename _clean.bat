REM just for output
echo off
cls

REM set make tool (if not in PATH, set complete path)
set MAKE=mingw32-make

REM use SDCC makefile to delete output
%MAKE% -f Makefile clean

REM delete DevC++ objects. Keep Objects folder
rd /S /Q .\Objects
mkdir .\Objects

REM delete doxygen output
rd /S /Q  .\doxygen\html
rd /S /Q  .\doxygen\latex
rd /S /Q  .\doxygen\man
rd /S /Q  .\doxygen\rtf
rd /S /Q  .\doxygen\xml

REM delete other output
DEL .\.DS_Store
DEL .\doxygen\.DS_Store
DEL .\doxygen\images\.DS_Store
DEL .\STM8_Routines\.DS_Store

echo on
