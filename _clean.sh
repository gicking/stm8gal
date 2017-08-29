#!/bin/bash 

# change to current working directory
cd `dirname $0`

# just for output
echo off
clear

# make application
make -f Makefile clean

echo " "
read -p "press key to close window..."
echo on

