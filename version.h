/**
  \file version.h
   
  \author G. Icking-Konert
  \date 2008-11-02
  \version 0.1
   
  \brief declaration of SW version number
   
  declaration of 2B SW version number. Format is xx.xxxxxxxx.xxxxx.x
  A change major version ([15:14] -> 0..3) indicates e.g. change in the SW architecture. 
  A change in the minor version ([13:6] -> 0..255) indicates e.g. critical bugfixes. 
  A change in build number ([5:1] -> 0..31) indicates cosmetic changes.
  The release status is indicated by bit [0] (0=beta; 1=released)
*/

// for including file only once
#ifndef _SW_VERSION_H_
#define _SW_VERSION_H_

/// 16b SW version identifier 
#define VERSION     ((1<<14) | (1<<6) | (8<<1) | 1)     // -> v1.1.8

#endif // _SW_VERSION_H_


/********************
 *  add description of changes below

  v1.1.8 (2018-10-07)
    - add option for background operation for IDE usage. Skip prompts and setting console color & title
    - add different verbosity levels (0..2) for IDE usage

  v1.1.7 (2018-01-04)
    - added option to skip reset (-R 0) via commandline
    - reset STM8 before opening port to avoid flushing issue under Linux (see https://stackoverflow.com/questions/13013387/clearing-the-serial-ports-buffer)

  v1.1.6 (2017-12-22)
    - fixed bug in SPI using Arduino bridge (see https://github.com/gicking/Arduino_SPI_bridge)

  v1.1.5 (2017-12-20)
    - added SPI support via Arduino bridge (see https://github.com/gicking/Arduino_SPI_bridge)
    - replace manual reset parameter "-Q" with "-R 0"
    - fixed bug in "2-wire reply mode" (wrong echo)

  v1.1.4 (2017-12-14)
    - added SPI support via spidev (currently only POSIX)
    - add user-space access to RasPi GPIOs via wiringPi for automatic reset. Use header numbering scheme (="physical")
    - removed verbose commandline option (-V). Always print verbose 
    - added listing of /dev/serial0 (new in Pi3, see https://raspberrypi.stackexchange.com/questions/45570/how-do-i-make-serial-work-on-the-raspberry-pi3)

  v1.1.3 (2017-08-29)
    - renamed from "STM8_serial_flasher" to "stm8gal", following a proposal by Philipp Krause (see https://github.com/gicking/STM8_serial_flasher/issues/10)

  v1.1.2 (2016-05-25)
    - add optional flash mass erase prior to upload

  v1.1.1 (2016-02-03)
    - add support for STM8L family (skip RAM code upload)
    - add memory dump to file

  v1.1.0 (2015-06-13)
    - add support for flashing via Raspberry UART
    - add optional reset of STM8 via DTR pin (USB/RS232) / GPIO (Raspberry)

  v1.0.0 (2014-12-21)
    - first release. Start of revision history
      
********************/

// end of file
