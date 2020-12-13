#!/bin/bash 

# open terminal on double-click, skip else. See https://www.linuxquestions.org/questions/programming-9/executing-shell-script-in-terminal-directly-with-a-double-click-370091/
tty -s; if [ $? -ne 0 ]; then /etc/alternatives/x-terminal-emulator -e "$0"; exit; fi


############
# download STM8 serial bootloader tool from https://github.com/gicking/stm8gal
############

# change to current working directory
cd `dirname $0`

# set path to flash loader
LOADER=./stm8gal

# set other parameters
FIRMWARE=./dummy_32kB.s19
#FIRMWARE=./dummy_32kB.txt

# set serial port
PORT=/dev/ttyUSB0

# use flash loader to upload new SW
$LOADER -p $PORT -w $FIRMWARE -v 2

echo
read -p "press return to close window"
echo
