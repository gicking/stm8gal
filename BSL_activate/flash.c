/*----------------------------------------------------------
    INCLUDE FILES
----------------------------------------------------------*/
// include files
#include <stdint.h>
#include <stdio.h>
#include "globals.h"
#include "stm8as.h"
#include "flash.h"



/**
  \fn void flash_write_option_byte(uint16_t addr, uint8_t byte)
  
  \brief write option byte to flash
  
  \param[in] addr   address to write to
  \param[in] byte   byte to program

  write single option byte to given address
*/
void flash_write_option_byte(uint16_t addr, uint8_t byte) {

  uint8_t        *addr_near;    // option bytes are in 2B range

  // check address
  if ((addr < 0x4800) || (addr > 0x48FF))
    return;
  
  // disable interrupts
  DISABLE_INTERRUPTS;

  // unlock w/e access to EEPROM & option bytes
  FLASH_DUKR.byte = 0xAE;
  FLASH_DUKR.byte = 0x56;
  
  // additionally required
  FLASH_CR2.byte  |= 0x80;
  FLASH_NCR2.byte &= 0x7F;
  
  // wait until access granted
  while(!FLASH_IAPSR.reg.DUL);

  // set address
  addr_near = (uint8_t*) addr;
  
  // write option byte to p-flash
  *(addr_near++) = byte;
  
  // wait until done
  while (!FLASH_IAPSR.reg.EOP);
  
  // lock EEPROM again against accidental erase/write
  FLASH_IAPSR.reg.DUL = 0;
  
  // additional lock
  FLASH_CR2.byte  &= 0x7F;
  FLASH_NCR2.byte |= 0x80;
  
  
  // enable interrupts
  ENABLE_INTERRUPTS;

} // flash_write_option_byte



/**
  \fn void flash_write_byte(uint32_t addr, uint8_t ch)
  
  \brief write single byte to flash
  
  \param[in] addr   address to write to
  \param[in] ch     byte to program

  write single byte to address in P-flash or EEPROM
*/
void flash_write_byte(uint32_t addr, uint8_t ch) {
    
  // disable interrupts
  DISABLE_INTERRUPTS;

  // unlock w/e access to P-flash
  FLASH_PUKR.byte = 0x56;
  FLASH_PUKR.byte = 0xAE;
  
  // unlock w/e access to EEPROM
  FLASH_DUKR.byte = 0xAE;
  FLASH_DUKR.byte = 0x56;
  
  // wait until access granted
  while(!FLASH_IAPSR.reg.PUL);
  while(!FLASH_IAPSR.reg.DUL);

// Cosmic compiler (supports far pointer)
#if defined(__CSMC__)

  // write byte to p-flash (use 3b address for near&far range)
  *((@far uint8_t*) addr) = ch;

// SDCC compiler (doesn't support far pointer --> use inline assembler)
#elif defined(__SDCC)
  
  // store address & data globally for assember 
  g_addr = addr;
  g_val  = ch;

  // use inline assembler
ASM_START
  ld	a,_g_val
  ldf	[_g_addr+1].e,a
ASM_END

#endif // SDCC

  // wait until done
  while (!FLASH_IAPSR.reg.EOP);
  
  // lock P-flash again against accidental erase/write
  FLASH_IAPSR.reg.PUL = 0;
  
  // lock EEPROM again against accidental erase/write
  FLASH_IAPSR.reg.DUL = 0;
  
  // enable interrupts
  ENABLE_INTERRUPTS;

} // flash_write_byte



/**
  \fn uint8_t read_byte(uint32_t addr)
  
  \brief read single byte from memory
  
  \param[in] addr  memory address to read from

  \return byte read from memory

  read single byte from address in memory
*/
uint8_t read_byte(uint32_t addr) {
    
// Cosmic compiler (supports far pointer)
#if defined(__CSMC__)

  // return read byte from memory (use 3b address for near&far range)
  return(*((@far uint8_t*) addr));

// SDCC compiler (doesn't support far pointer --> use inline assembler)
#elif defined(__SDCC)
  
  // store address & data globally for assember 
  g_addr = addr;

  // use inline assembler
ASM_START
  ldf	a,[_g_addr+1].e
  ld	_g_val,a
ASM_END

  // return read byte from memory
  return(g_val);

#endif // SDCC

} // read_byte

/*-----------------------------------------------------------------------------
    END OF MODULE
-----------------------------------------------------------------------------*/


