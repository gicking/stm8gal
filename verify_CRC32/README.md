_verify\_CRC32_ is a RAM routine for STM8 which calculates a CRC32 checksum over a specified address range.

The motivation for this is to significantly accelerate the verification of uploaded code, especially when using the "UART reply-mode" via a 2-wire interface.
In this mode the PC has to echo each received byte before the STM8 ROM bootloader (ROM-BL) sends the next byte, which leads to **extremely** slow memory read-out due to USB latency. For background see [https://github.com/gicking/stm8gal/issues/20](https://github.com/gicking/stm8gal/issues/20).

Implementation details:
  - the code is partially written in [SDCC](http://sdcc.sourceforge.net/) for small code size. Therefore the SDCC toolchain is required for optional compilation
  - the actual CRC32 calculation is copied from [https://github.com/basilhussain/stm8-crc](https://github.com/basilhussain/stm8-crc)
  - the _verify\_CRC32_ RAM routine is restricted to address range 0x210..0x0x2F3 to avoid conflict with the also required write/erase RAM routines
  - the verify start address (4B) is read from 0x2F4..0x2F7, the end address (4B) from 0x2F8..0x2FB. They are written by _stm8gal_ prior to jumping to 0x210
  - the calculated CRC32 checksum (4B) is stored at 0x2FC..0x2FF and is read by _stm8gal_ after returning to ROM-BL
  - to avoid conflict with _verify\_CRC32_ and the write/erase RAM routines, optional user RAM code & parameters should start at 0x400

Notes:
  - ROM-BL manual ([UM0560](https://www.st.com/resource/en/user_manual/cd00201192-stm8-bootloader-stmicroelectronics.pdf)) states that RAM usage of write/erase routines is restricted to 0x00..0x1FF. However, the STM8L/AL low-density (8kB) bootloader v1.0 apparently uses 0x00..0x206
  - STM8 is big endian, while almost all PCs / OS'es are litte endian. This must be taken into account when storing addresses, reading the CRC32 checksum and calculating the CRC32 checksum on PC side
  - the STM8S/AF medium density (32kB) ROM-BL v1.0, and high density (>=64kB) v2.0 do not have an entry point after ROM-BL and ROP check (see [UM0560](https://www.st.com/resource/en/user_manual/cd00201192-stm8-bootloader-stmicroelectronics.pdf), Appendix B). This would preventing returning from _verify\_CRC32_ to the ROM-BL after the respective option bytes have been written. To avoid this issue, _stm8gal_ does not allow a CRC32 verify for these (old) devices. Instead use verify via read-back  
