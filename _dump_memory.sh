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
ADDRSTART=0x8000
ADDRSTOP=0x80FF
OUTFILE=./memory_dump.txt

# set serial port
PORT=/dev/ttyUSB0

# set optional UART mode (0=duplex, 1=1-wire reply, 2=2-wire reply, else auto-detect)
#MODE=2

# acccording to STM8 bootloader manual section 2.1, the minimum baudrate is 4800Baud. Default is 115.2kBaud
# Typical baudrates are: 4800 9600 19200 38400 57600 115200 230400
BAUD=230400

# if not explicitely set above, configure auto-detect mode
if [ -z "$MODE" ]; then MODE=255; fi

# use flash loader to read out memory
$LOADER -p $PORT -b $BAUD -u $MODE -r $ADDRSTART $ADDRSTOP $OUTFILE -V 2

echo
read -p "press return to close window"
echo
