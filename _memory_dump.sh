#!/bin/bash 

# download STM8 serial bootloader tool from https://github.com/gicking/stm8gal

# change to current working directory
cd `dirname $0`

# set path to STM flash loader, serial port name, etc.
LOADER=./stm8gal
PORT=/dev/ttyUSB0
ADDRSTART=0x8000
ADDRSTOP=0x80FF
OUTFILE=./dump.txt

# acccording to STM8 bootloader manual section 2.1, the minimum baudrate is 4800Baud
#BAUD=4800
#BAUD=9600
#BAUD=19200
BAUD=38400
#BAUD=57600
#BAUD=115200
#BAUD=230400

# use flash loader to read out memory
$LOADER -p $PORT -b $BAUD -r $ADDRSTART $ADDRSTOP $OUTFILE -V 2

echo
read -p "press return to close window"
echo
