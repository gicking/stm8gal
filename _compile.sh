#!/bin/bash 

# open terminal on double-click, skip else. See https://www.linuxquestions.org/questions/programming-9/executing-shell-script-in-terminal-directly-with-a-double-click-370091/
tty -s; if [ $? -ne 0 ]; then /etc/alternatives/x-terminal-emulator -e "$0"; exit; fi

# change to current working directory
cd `dirname $0`

# just for output
echo off
clear

# use make to delete output
make -f Makefile clean

# delete DevC++ objects. Keep Objects folder
rm -fr ./Objects
mkdir ./Objects

# make application
make -f Makefile

echo " "
read -p "press key to close window..."
echo on

