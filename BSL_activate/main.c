/*----------------------------------------------------------
    INCLUDE FILES
----------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include "stm8as.h"
#include "flash.h"
#define _MAIN_
  #include "globals.h"
#undef _MAIN_


///////////////
// toggle LED pin once and wait approx. period milliseconds
// required LED pin to be known and initialized (see below)
///////////////
void toggleLED(uint16_t period_ms) {
  
  uint32_t  count, max;
  
  // convert to approx. ms (empirically determined)
  max = (uint32_t) period_ms * 600L;
  
  // blink LED
#if defined(STM8S105)
  PD.ODR.bit.b0 ^= 1;
#elif defined(STM8S207)
  PH.ODR.bit.b2 ^= 1;
#endif

  // wait a bit
  for (count=0; count<max; count++)
    _NOP_;

} // toggleLED



/////////////////
//	main routine
/////////////////
void main (void) {
  
  uint8_t     flagWD, i;
 
  
  /////////////////
  //	init peripherals
  /////////////////
  
  // disable interrupts
  DISABLE_INTERRUPTS;

  // switch to 16MHz (default is 2MHz)
  CLK_CKDIVR.byte = 0x00;  
  
  // init LED pin (STM8 Discovery and muBoard) 
#if defined(STM8S105)
  PD.ODR.bit.b0 = 0;   // init output
  PD.DDR.bit.b0 = 1;   // input(=0) or output(=1)
  PD.CR1.bit.b0 = 1;   // input: 0=float, 1=pull-up; output: 0=open-drain, 1=push-pull
  PD.CR2.bit.b0 = 1;   // input: 0=no exint, 1=exint; output: 0=2MHz slope, 1=10MHz slope
#elif defined(STM8S207)
  PH.ODR.bit.b2 = 0;   // init output
  PH.DDR.bit.b2 = 1;   // input(=0) or output(=1)
  PH.CR1.bit.b2 = 1;   // input: 0=float, 1=pull-up; output: 0=open-drain, 1=push-pull
  PH.CR2.bit.b2 = 1;   // input: 0=no exint, 1=exint; output: 0=2MHz slope, 1=10MHz slope
#endif
  

  ///////////////////
  // check all option bytes and correct if required
  ///////////////////
  flagWD = 0;
  
  // read-out protection (=OPT0) and user boot code are (=OPT1) only writable via SWIM 
  
  // reset alternate GPIO mapping (=OPT2/NOPT2)
  if ((*((uint8_t*) OPT2) != 0x00) || (*((uint8_t*) NOPT2)  != 0xFF)) {
    flash_write_option_byte(OPT2,  0x00);
    flash_write_option_byte(NOPT2, 0xFF);
    flagWD = 1;
  }
  
  // deactivate watchdog (=OPT3/NOPT3)
  if ((*((uint8_t*) OPT3) != 0x00) || (*((uint8_t*) NOPT3)  != 0xFF)) {
    flash_write_option_byte(OPT3,  0x00);
    flash_write_option_byte(NOPT3, 0xFF);
    flagWD = 1;
  }
  
  // reset clock options to default (=OPT4/NOPT4)
  if ((*((uint8_t*) OPT4) != 0x00) || (*((uint8_t*) NOPT4)  != 0xFF)) {
    flash_write_option_byte(OPT4,  0x00);
    flash_write_option_byte(NOPT4, 0xFF);
    flagWD = 1;
  }
   
  // max. HCE clock startup time (=OPT5/NOPT5)
  if ((*((uint8_t*) OPT5) != 0x00) || (*((uint8_t*) NOPT5)  != 0xFF)) {
    flash_write_option_byte(OPT5,  0x00);
    flash_write_option_byte(NOPT5, 0xFF);
    flagWD = 1;
  }
   
  // OPT6 is reserved/undocumented
   
  // no flash wait state (required for >16MHz) (=OPT7/NOPT7)
  if ((*((uint8_t*) OPT7) != 0x00) || (*((uint8_t*) NOPT7)  != 0xFF)) {
    flash_write_option_byte(OPT7,  0x00);
    flash_write_option_byte(NOPT7, 0xFF);
    flagWD = 1;
  }
   
  // OPT8-16 contain temporary memory unprotection key (TMU) -> rather don't touch 

  // activate ROM-bootloader (=OPT17/NOPT17)  
  if ((*((uint8_t*) OPT17) != 0x55) || (*((uint8_t*) NOPT17)  != 0xAA)) {
    flash_write_option_byte(OPT17,  0x55);
    flash_write_option_byte(NOPT17, 0xAA);
    flagWD = 1;
  }
 
  // if any option byte was changed, indicate via LED and trigger SW reset
  if (flagWD != 0) {
    
    // flash LED fast 10x to indicate change
    for (i=0; i<10; i++)
      toggleLED(100);
    
    // triger SW reset (clear bit B6)
    WWDG_CR.byte = 0x3F;

  } // if option byte was changed
       
    

  /////////////////
  //	main loop
  /////////////////
  while (1) {
    
    // blink LED with 1s period 
    toggleLED(500);
    
  } // main loop

} // main


/*-----------------------------------------------------------------------------
    END OF MODULE
-----------------------------------------------------------------------------*/
