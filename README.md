stm8gal
-------------------

A tool for uploading hexfiles to the STM8 microcontroller flash and memory dump to file via COM port using the built-in ROM bootloader (BSL) of the STM8. I wrote this tool, because the similar "Flash Loader Demonstrator" tool by STMicroelectronics supports Windows only.

Content:
  - stm8gal        -> program or read out STM8 via UART bootloader and USB, RS232 or UART interface
  - BSL_activate   -> STM8 project for activating bootloader. For details check 'README'

For more details and instructions on building and using the tool see the Wiki under https://github.com/gicking/stm8gal/wiki

If you find any bugs or for feature requests, please send me a note.

Have fun!
Georg

====================================

Revision History
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
