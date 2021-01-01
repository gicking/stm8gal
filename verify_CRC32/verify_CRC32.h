/**
  \file verify_CRC32.h

  \author G. Icking-Konert
  \date 2020-12-27
  \version 0.1

  \brief declaration of STM8 memory check via CRC32.

  declaration of STM8 memory check via CRC32. Routine is called by ROM bootloader.
  CRC32 routine is copied from https://github.com/basilhussain/stm8-crc.
  Code is compatible with SDCC, parameters via variables at fixed addresses.
  After completion ROM-BL is restarted (for details see AppNote UM0560).
*/

// for including file only once
#ifndef _VERIFY_CRC32_H_
#define _VERIFY_CRC32_H_


// include files
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


////////
// Peripheral registers (SFR). Addresses are specified in linker file
////////

// CLK prescaler
extern volatile uint8_t CLK_CKDIVR;

// IWDG watchdog
extern volatile uint8_t IWDG_KR;

// WWDG watchdog
extern volatile uint8_t WWDG_CR;

// TIM2 (must be cleared prior to return to ROM-BL)
extern volatile uint8_t TIM2_SR1;
extern volatile uint8_t TIM2_EGR;

// TIM3 (must be cleared prior to return to ROM-BL)
extern volatile uint8_t TIM3_SR1;
extern volatile uint8_t TIM3_EGR;


////////
// ROM bootloader parameters. Addresses are specified in linker file
////////
extern volatile uint8_t      BL_timeout;    // ROM-BL timeout (0=no timeout; 1=1s timeout)


////////
// Parameters passed between ROM-BL and RAM-verify. Addresses are specified in linker file
////////
extern volatile uint32_t     addr_start;    // first address for checksum (@ 0x350 - 0x353)
extern volatile uint32_t     addr_stop;     // last address for checksum (@ 0x354 - 0x357)
extern volatile uint32_t     crc32;         // calculated CRC32 checksum (@ 0x358 - 0x35B)
extern volatile uint8_t      data;          // value read from memory (use BROM data array)
extern volatile uint8_t      old_CKDIVR;    // old CLK prescaler value (use BROM data array)


////////
// Preprocessor macros
////////

// memory pointer size
#ifdef __SDCC_MODEL_LARGE
    #define ASM_ARGS_SP_OFFSET 4
    #define ASM_RETURN retf
#else
    #define ASM_ARGS_SP_OFFSET 3
    #define ASM_RETURN ret
#endif

// service IWDG watchdog
#define SERVICE_IWDG    IWDG_KR = 0xAA

// service WWDG watchdog
#define SERVICE_WWDG    WWDG_CR = 0x7F

// Initial value for the CRC32 implementation.
#define CRC32_INIT ((uint32_t)0xFFFFFFFF)

// Function-like macro to return the initial value.
#define crc32_init() CRC32_INIT

// Value used to finalise the CRC32, being XOR-ed with the CRC.
#define CRC32_XOROUT ((uint32_t)0xFFFFFFFF)

// Function-like macro to do the final XOR of the CRC value.
#define crc32_final(c) ((c) ^ CRC32_XOROUT)

/// update CRC32 checksum
uint32_t crc32_update(uint32_t crc, uint8_t data);

#endif // _VERIFY_CRC32_H_

// end of file
