echo off
cls

REM set make tool (if not in PATH, set complete path)
set MAKE=mingw32-make

REM make application
%MAKE% -f Makefile.win

PAUSE
echo on
