#!/bin/bash 

# download STM8 serial bootloader tool from https://github.com/gicking/stm8gal

# change to current working directory
cd `dirname $0`

# set path to flash loader
LOADER=./stm8gal

# set other parameters
ADDRSTART=0x8000
ADDRSTOP=0x80FF
OUTFILE=./dump.txt

# set serial port
PORT=/dev/ttyUSB0

# set optional UART mode (0=duplex, 1=1-wire reply, 2=2-wire reply)
#MODE=2

# acccording to STM8 bootloader manual section 2.1, the minimum baudrate is 4800Baud. Default is 19200Baud
# Typical baudrates are: 4800 9600 19200 38400 57600 115200 230400
BAUD=230400

# if not explicitely set, configure auto-detect mode
if [ -z "$MODE" ]; then MODE=255; fi

# for UART reply mode (MODE=1|2) limit baudrate. Found >19.2Baud is unreliable
if [[ $MODE != 0 ]]; then BAUD=19200; fi

# use flash loader to read out memory
$LOADER -p $PORT -b $BAUD -u $MODE -r $ADDRSTART $ADDRSTOP $OUTFILE -V 2

echo
read -p "press return to close window"
echo
