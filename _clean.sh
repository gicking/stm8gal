#!/bin/bash 

# change to current working directory
cd `dirname $0`

# just for output
echo off
clear

# make application
make -f Makefile clean

# delete DevC++ objects. Keep Objects folder
rm -fr .\Objects
mkdir .\Objects

# delete doxygen output
rm -fr .\doxygen\html
rm -fr .\doxygen\latex
rm -fr .\doxygen\man
rm -fr .\doxygen\rtf
rm -fr .\doxygen\xml

# delete other output
rm -fr .\dump.txt
rm -fr .\.DS_Store
rm -fr .\doxygen\.DS_Store
rm -fr .\doxygen\images\.DS_Store
rm -fr .\STM8_Routines\.DS_Store

echo " "
read -p "press key to close window..."
echo on

