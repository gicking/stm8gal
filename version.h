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
#define VERSION     ((1<<14) | (5<<6) | (0<<1) | 1)     // -> v1.5.0

#endif // _SW_VERSION_H_


/********************

Revision History
----------------

v1.5.0 (2021-01-23)
  - replaced write/erase RAM routines by STM with OSS routines (see https://github.com/gicking/stm8gal/issues/4)
  - removed STM8S 32kB v1.4 and 128kB v2.4. According to STM support were never released and development is stopped
  - removed STM8S 256kB v1.0. According to STM support variant was never released and development is stopped
  - fixed bug in v1.4.3 which prevented flash write/erase after CRC32-verify 

----------------

v1.4.3 (2021-01-09)
  - fixed bug in SPI read-back after write
  - fixed bug in CRC32-verify (see https://github.com/gicking/stm8gal/issues/20)

----------------

v1.4.2 (2020-12-26)
  - support re-synchronization w/o STM8 reset
  - add option verify via CRC32 checksum (see https://github.com/gicking/stm8gal/issues/20)
  - add parameter to verify option (-V/-verify). Is required due to new CRC32 check 

----------------

v1.4.1 (2020-12-13)
  - minor bugfix

----------------

v1.3.0 (2019-01-02)
  - add multiple up- and downloads in single run
  - added option to print memory map and sector erase
  - fixed S19 export bugs for >16bit addresses and small images
  - fixed IHX import bug for record type 5
  - fixed mass erase timeout bug
  - fixed bug for files with "holes" -> only write specified data
  - harmonized files with https://github.com/gicking/hexfile_merger

----------------

v1.2.0 (2018-12-02)
  - add automatic UART mode detection (duplex, 1-wire, 2-wire reply). See [UART mode issue](https://github.com/gicking/stm8gal/issues/7)
  - changed default UART baudrate to 115.2kBaud for robustness

----------------

v1.1.8 (2018-10-07)
  - add option for background operation for IDE usage. Skip prompts and setting console color & title
  - add different verbosity levels (0..2) for IDE usage

----------------

v1.1.7 (2018-01-04)
  - added option to skip reset (-R 0) via commandline
  - reset STM8 before opening port to avoid [flushing issue under Linux](https://stackoverflow.com/questions/13013387/clearing-the-serial-ports-buffer)

----------------

v1.1.6 (2017-12-22)
  - fixed bug in SPI using [Arduino bridge](https://github.com/gicking/Arduino_SPI_bridge)

----------------

v1.1.5 (2017-12-20)
  - added SPI support via [Arduino bridge](https://github.com/gicking/Arduino_SPI_bridge)
  - replace manual reset parameter "-Q" with "-R 0"
  - fixed bug in "2-wire reply mode" (wrong echo)

----------------

v1.1.4 (2017-12-14)
  - added SPI support via spidev (currently only POSIX)
  - add user-space access to RasPi GPIOs via wiringPi for automatic reset. Use header numbering scheme (="physical")
  - removed verbose commandline option (-V). Always print verbose
  - added listing of /dev/serial0 (new in Pi3, see https://raspberrypi.stackexchange.com/questions/45570/how-do-i-make-serial-work-on-the-raspberry-pi3)

----------------

v1.1.3 (2017-08-29)
  - renamed from "STM8_serial_flasher" to "stm8gal", following a proposal by Philipp Krause (see https://github.com/gicking/STM8_serial_flasher/issues/10)

----------------

v1.1.2 (2016-05-25)
  - add optional flash mass erase prior to upload

----------------

v1.1.1 (2016-02-03):
  - add support for STM8L family (skip RAM code upload)
  - add memory dump to file

----------------

v1.1.0 (2015-06-22):
  - add support for STM8 bootloader “reply mode“
  - add optional reset of STM8 via DTR (RS232/USB) or GPIO18 (Raspberry)

----------------

v1.0.0 (2014-12-21):
  - initial release by Georg Icking-Konert under the Apache License 2.0

********************/

// end of file
