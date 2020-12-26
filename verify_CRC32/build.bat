@echo off
setlocal

REM set path to make tool
set MAKE="C:\Program Files\Mingw-W64\x86_64-8.1.0-posix-seh-rt_v6-rev0\mingw64\bin\mingw32-make.exe"

REM run makefile
%MAKE% -f Makefile
