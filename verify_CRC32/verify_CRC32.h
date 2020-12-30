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


// include files
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


// macro to access STM8 peripheral register
#define _SFR(mem_addr) (*(volatile uint8_t *)(mem_addr))

// CLK prescaler
#define CLK_CKDIVR  _SFR(0x50c6)

// IWDG watchdog
#define IWDG_KR    _SFR(0x50e0)
#define SERVICE_IWDG  IWDG_KR = 0xAA

// WWDG watchdog
#define WWDG_CR    _SFR(0x50d1)
#define SERVICE_WWDG  WWDG_CR = 0x7F

// TIM2 (must be cleared prior to return to ROM-BL)
#define TIM2_SR1  _SFR(0x5302)
#define TIM2_EGR  _SFR(0x5304)

// TIM3 (must be cleared prior to return to ROM-BL)
#define TIM3_SR1  _SFR(0x5322)
#define TIM3_EGR  _SFR(0x5324)

// return value (stored in global variable "status")
#define SUCCESS   0
#define ERROR     1

// memory pointer size
#ifdef __SDCC_MODEL_LARGE
    #define ASM_ARGS_SP_OFFSET 4
    #define ASM_RETURN retf
#else
    #define ASM_ARGS_SP_OFFSET 3
    #define ASM_RETURN ret
#endif

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

/// ROM routine to service IWDG & WWDG watchdogs
extern void watchdog_refresh(void);


// Parameters passed from ROM-BL to RAM-verify and results passed back.
// Locations are specified at link time according to compatibility with ROM
// bootloader version. See *.lk command files.
extern uint32_t     addr_start;    // first address for checksum (@ 0x350 - 0x353)
extern uint32_t     addr_stop;     // last address for checksum (@ 0x354 - 0x357)
extern uint8_t      status;        // return status (0=ok, 1=error) (@ 0x358)
extern uint32_t     crc32;         // calculated CRC32 checksum (@ 0x359 - 0x35C)
extern uint8_t      BL_timeout;    // ROM-BL timeout (0=no timeout; 1=1s timeout)
