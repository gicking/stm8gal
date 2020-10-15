#!/bin/bash 

# change to current working directory
cd `dirname $0`

# replace binaries by dummy files with same name
echo "" > stm8gal_linux32
echo "" > stm8gal_linux64
echo "" > stm8gal_raspbian32
echo "" > stm8gal_windows32.exe
echo "" > stm8gal_windows64.exe
