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
#define VERSION     ((1<<14) | (1<<6) | (3<<1) | 1)     // -> v1.1.3

#endif // _SW_VERSION_H_


/********************
 *  add description of changes below

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
