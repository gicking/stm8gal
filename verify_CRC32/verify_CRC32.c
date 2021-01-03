/**
  \file verify_CRC32.c

  \author G. Icking-Konert
  \date 2020-12-27
  \version 0.1

  \brief implementation of STM8 memory check via CRC32.

  implementation of STM8 memory check via CRC32. Routine is called by ROM bootloader.
  CRC32 routine is copied from https://github.com/basilhussain/stm8-crc.
  Code is compatible with SDCC, parameters via variables at fixed addresses.
  After completion ROM-BL is restarted (for details see AppNote UM0560).
*/

// SDCC pragmas
#pragma codeseg VERIFY_SEG

// include files
#include "verify_CRC32.h"
#include "stm8-crc/crc.h"

// declare jump back to ROM bootloader
void bootloader_jump(void);



/**
  \fn void verify_CRC32(void)

  Calculate CRC32 checksum over address range addr_start...addr_stop (at fixed addresses).
  Store error check in variable status and calculated CRC32 result in variable (also at fixed addresses).
  After completion restart STM8 ROM bootloader (see UM0560).
  Can be __naked as no return to ROM-BL but jump back
*/
void verify_CRC32(void)
{
  // switch to 16MHz, store old setting
  old_CKDIVR = CLK_CKDIVR;
  CLK_CKDIVR = 0x00;

  // initialize CRC32 checksum
  crc32 = crc32_init();

  // calculate CRC32 checksum
  while (addr_start<=addr_stop)
  {
    // read from memory. Use inline assembler due to lack of far pointers in SDCC
    // store value in global variable "data"
    __asm
      push a
      ldf  a,[_addr_start+1].e
      ld  _data, a
      pop a
    __endasm;

    // update CRC32 checksum
    crc32 = crc32_update(crc32, data);

    // increment address
    addr_start++;

    // service watchdogs
    SERVICE_IWDG;
    SERVICE_WWDG;

  } // loop over memory

  // finalize CRC32 checksum
  crc32 = crc32_final(crc32);

  // restore old clock setting
  CLK_CKDIVR = old_CKDIVR;


  // return to bootloader (see UM0560, appendix B)
  TIM2_EGR = 0x01;          // refresh TIM2 prescaler shadow registers to 1
  TIM2_SR1;
  TIM2_SR1 = 0x00;
  TIM3_EGR = 0x01;          // refresh TIM3 prescaler shadow registers to 1
  TIM3_SR1;
  TIM3_SR1 = 0x00;
  //BL_timeout = 0x00;        // ROM-BL timeout (0=no timeout; 1=1s timeout). Address unknown for STM8L/AL -> skip and accept 1s timeout
  bootloader_jump();        // jump back to ROM-BL

} // verify()


/**
  \fn void bootloader_jump_reset(void)

  Jump back to bootloader. Address is given in linker file.
  Ideally skip BL and ROP check to avoid issues after changing respective OPT bytes.
*/
void bootloader_jump(void) __naked {
    __asm__("jp _bootloader_entry");
}


// end of file
