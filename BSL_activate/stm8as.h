/**
  \file stm8as
   
  \author G. Icking-Konert
  \date 2013-11-22
  \version 0.1
   
  \brief definition of STM8 peripherals etc.
   
  definition of STM8 peripheral registers, interrupt
  vector table, and some useful macros
*/

/*-----------------------------------------------------------------------------
    MODULE DEFINITION FOR MULTIPLE INCLUSION
-----------------------------------------------------------------------------*/
#ifndef _STM8AS_H
#define _STM8AS_H


/*-----------------------------------------------------------------------------
    set device specific peripherals base addresses from datasheet
    (see "hardware register map")
-----------------------------------------------------------------------------*/

// GPIOs
#define ADDR_PA           0x5000
#define ADDR_PB           0x5005
#define ADDR_PC           0x500A
#define ADDR_PD           0x500F
#define ADDR_PE           0x5014
#define ADDR_PF           0x5019
#define ADDR_PG           0x501E
#define ADDR_PH           0x5023
#define ADDR_PI           0x5028

// flash
#define ADDR_FLASH_CR1    0x505A
#define ADDR_FLASH_CR2    0x505B
#define ADDR_FLASH_NCR2   0x505C
#define ADDR_FLASH_FPR    0x505D
#define ADDR_FLASH_NFPR   0x505E
#define ADDR_FLASH_IAPSR  0x505F



/*-----------------------------------------------------------------------------
    COMPILER SPECIFIC SETTINGS
-----------------------------------------------------------------------------*/

#include <stdint.h>

// Cosmic compiler
#if defined(__CSMC__)
  
  /// syntax for variables at absolute addresses
  #define reg(addr,type,name)    extern volatile type name @addr
  
  /// single line inline assembler
  #define ASM(mnem)    _asm(mnem)
  
  /// start multi-line inline assembler
  #define ASM_START    #asm

  /// end multi-line inline assembler
  #define ASM_END      #endasm
  

// SDCC compiler
#elif defined(__SDCC)
  
  /// syntax for variables at absolute addresses
  #define reg(addr,type,name)    volatile __at(addr) type name

  /// single line inline assembler
  #define ASM(mnem)    __asm__(mnem)
  
  /// start multi-line inline assembler
  #define ASM_START    __asm

  /// end multi-line inline assembler
  #define ASM_END      __endasm;


// compiler unknown
#else
  #error in 'stm8.h': compiler not supported
#endif



/*-----------------------------------------------------------------------------
    DEFINITION OF GLOBAL MACROS/#DEFINES
-----------------------------------------------------------------------------*/

/***
 device information
***/

/// name of STM8 device
#define __DEVICE        "stm8af52aa"

/// RAM size in bytes (see datasheet of device)
#define __RAM_SIZE      (6*1024)

/// flash size in bytes (see datasheet of device)
#define __FLASH_SIZE    (128*1024)


/***
 misc macros
***/

/// perform a nop() operation (=minimum delay)
#define _NOP_                ASM("nop")

/// disable interrupt handling
#define DISABLE_INTERRUPTS   ASM("sim")

/// enable interrupt handling
#define ENABLE_INTERRUPTS    ASM("rim")


/***
 macros for bitwise r/w access
***/

/// read single bit from data
#define read_bit(byte, bit)			      (byte & (1 << bit), >> bit)

/// set single bit in data to '1'
#define set_bit(byte, bit)            byte |= (1 << bit)

/// clear single bit in data to '0'
#define clear_bit(byte, bit)		      byte &= ~(1 << bit)

/// set single bit state in data to specified value
#define write_bit(byte, bit, state)		(state?(byte|=(1<<bit)):(byte&=~(1<<bit)))

/// toggle single bit state in data
#define toggle_bit(byte, bit)		      byte ^= (1 << bit)



/***
 STM8 interrupt vector for SDCC (for Cosmic see 'stm8_interrupt_vector.c')
***/
#if defined(__SDCC)
  
  /// irq0 - External Top Level interrupt (TLI)
  #define __TLI_VECTOR__             0
  
  /// irq1 - Auto Wake Up from Halt interrupt
  #define __AWU_VECTOR__             1

  /// irq2 - Clock Controller interrupt
  #define __CLK_VECTOR__             2
  
  /// irq3 - External interrupt 0 (GPIOA)
  #define __EXTI0_VECTOR__           3
  
  /// irq4 - External interrupt 1 (GPIOB)
  #define __EXTI1_VECTOR__           4
  
  /// irq5 - External interrupt 2 (GPIOC)
  #define __EXTI2_VECTOR__           5
  
  /// irq6 - External interrupt 3 (GPIOD)
  #define __EXTI3_VECTOR__           6
  
  /// irq7 - External interrupt 4 (GPIOE)
  #define __EXTI4_VECTOR__           7
  
  /// irq8 - CAN receive interrupt
  #define __CAN_RX_VECTOR__          8
  
  /// irq9 - CAN transmit interrupt
  #define __CAN_TX_VECTOR__          9
  
  /// irq10 - SPI End of transfer interrupt
  #define __SPI_VECTOR__            10
  
  /// irq11 - TIM1 Update/Overflow/Trigger/Break interrupt
  #define __TIM1UPD_VECTOR__        11
  
  /// irq12 - TIM1 Capture/Compare interrupt
  #define __TIM1CAP_VECTOR__        12
  
  /// irq13 - TIM2 Update/Overflow/Break interrupt
  #define __TIM2UPD_VECTOR__        13
  
  /// irq14 - TIM2 Capture/Compare interrupt
  #define __TIM2CAP_VECTOR__        14
  
  /// irq15 - Reserved
  #define __TIM3UPD_VECTOR__        15
  
  /// irq16 - Reserved
  #define __TIM3CAP_VECTOR__        16
  
  /// irq17 - UART1(=LINUART, Tx complete interrupt
  #define __UART1_TX_CMPL_VECTOR__  17
  
  /// irq18 - UART1(=LINUART, Rx interrupt
  #define __UART1_RX_FULL_VECTOR__  18
  
  /// irq19 - I2C interrupt
  #define __I2C_VECTOR__            19
  
  /// irq20 - UART2 Tx interrupt
  #define __UART3_TX_CMPL_VECTOR__  20
  
  /// irq21 - UART2 Rx interrupt
  #define __UART3_RX_FULL_VECTOR__  21
  
  /// irq22 - ADC1 end of conversion/Analog watchdog interrupts
  #define __ADC2_VECTOR__           22
  
  /// irq23 - Timer 4 interrupt
  #define __TIM4UPD_VECTOR__        23
  
  /// irq24 - FLASH interrupt
  #define __FLASH_VECTOR__          24
  
#endif // __SDCC


/*-----------------------------------------------------------------------------
    DECLARATION OF GLOBAL TYPEDEFS
-----------------------------------------------------------------------------*/

/***
 typedefs for comfortable 8- and 16-bit access 
***/

/// union for bit- or bytewise access to 8-bit data
/** union for bitwise or bytewise read/write access to 8-bit data (=byte) */
typedef union {

  /// for byte access
  uint8_t  byte;
  
  /// for bit access
  struct {
    uint8_t b0 : 1;
    uint8_t b1 : 1;
    uint8_t b2 : 1;
    uint8_t b3 : 1;
    uint8_t b4 : 1;
    uint8_t b5 : 1;
    uint8_t b6 : 1;
    uint8_t b7 : 1;
  } bit;
  
} byte_t;


/// struct for bytewise r/w access to 16bit data
/**
 Note: order of r/w access is important and cannot be guaranteed in C --> don't use union like for byte_t
   - write: HB+LB ok / LB+HB and word fails! \n
   - read: LB+HB and word ok / HB+LB fails! \n
*/
typedef struct {

  /// high byte in 16b word
  uint8_t byteH   : 8;

  /// low byte in 16b word
  uint8_t byteL  : 8;

} word_t;


/*-----------------------------------------------------------------------------
    DEFINITION OF STM8 PERIPHERAL REGISTERS
-----------------------------------------------------------------------------*/

//--------
// GPIOs
//--------

/// port control structure
/** structure for controlling/monitoring pins in GPIO mode */
typedef struct {
  
  /// Port x output data register (Px_ODR)
  byte_t ODR;

  /// Port x pin input register (Px_IDR)
  byte_t IDR;

  /// Port x data direction register (Px_DDR)
  byte_t DDR;

  /// Port x control register 1 (Px_CR1)
  byte_t CR1;

  /// Port x control register 2 (Px_CR2)
  byte_t CR2;

} port_t;

/// registers for GPIO port PA access
reg(ADDR_PA, port_t, PA);

/// registers for GPIO port PB access
reg(ADDR_PB, port_t, PB);

/// registers for GPIO port PC access
reg(ADDR_PC, port_t, PC);

/// registers for GPIO port PD access
reg(ADDR_PD, port_t, PD);

/// registers for GPIO port PE access
reg(ADDR_PE, port_t, PE);

/// registers for GPIO port PF access
reg(ADDR_PF, port_t, PF);

/// registers for GPIO port PG access
reg(ADDR_PG, port_t, PG);

/// registers for GPIO port PH access
reg(ADDR_PH, port_t, PH);

/// registers for GPIO port PI access
reg(ADDR_PI, port_t, PI);


//--------
// Clocks
//--------

/// Internal clock register (CLK_ICKR)
/** union for controlling/monitoring the internal high- and low-speed clocks */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
    
    /// High speed internal RC oscillator enable
    uint8_t HSIEN   : 1;
    
    /// High speed internal oscillator ready flag
    uint8_t HSIRDY  : 1;
    
    /// Fast wakeup from Halt/Active-halt modes enable
    uint8_t FHW     : 1;
    
    /// Low speed internal RC oscillator enable
    uint8_t LSIEN   : 1;
    
    /// Low speed internal oscillator ready flag
    uint8_t LSIRDY  : 1;
    
    /// Regulator power off in Active-halt mode enable
    uint8_t REGAH   : 1;
    
    /// Reserved, must be kept cleared
    uint8_t res     : 2;

  } reg;
  
} CLK_ICKR_t;

/// register for controlling/monitoring the internal high- and low-speed clocks
reg(0x50c0, CLK_ICKR_t, CLK_ICKR);



/// External clock register (CLK_ECKR)
/** union for controlling/monitoring an optional external resonator */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
    
    /// High speed external crystal oscillator enable
    uint8_t HSEEN   : 1;
    
    /// High speed external crystal oscillator ready
    uint8_t HSERDY  : 1;
    
    /// Reserved, must be kept cleared
    uint8_t res     : 6;

  } reg;
  
} CLK_ECKR_t;

/// register for controlling/monitoring an optional external resonator
reg(0x50c1, CLK_ECKR_t, CLK_ECKR);



/// Clock master status register (CLK_CMSR)
/** union to read the currently selected master clock source */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
    
    /// Clock master status bits
    uint8_t CKM     : 8;
    
  } reg;
  
} CLK_CMSR_t;

/// register to read the currently selected master clock source
reg(0x50c3, CLK_CMSR_t, CLK_CMSR);



/// Clock master switch register (CLK_SWR)
/** union to select the master clock source */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Clock master selection bits
    uint8_t SWI     : 8;
    
  } reg;
  
} CLK_SWR_t;

/// register to select the master clock source
reg(0x50c4, CLK_SWR_t, CLK_SWR);



/// Switch control register (CLK_SWCR)
/** union to trigger clock source switching */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
    
    /// Switch busy flag
    uint8_t SWBSY   : 1;
    
    /// Switch start/stop enable
    uint8_t SWEN    : 1;
    
    /// Clock switch interrupt enable
    uint8_t SWIEN   : 1;
    
    /// Clock switch interrupt flag
    uint8_t SWIF    : 1;
    
    /// Reserved
    uint8_t res     : 4;

  } reg;

} CLK_SWCR_t;

/// register to trigger clock source switching
reg(0x50c5, CLK_SWCR_t, CLK_SWCR);



/// Clock divider register (CLK_CKDIVR)
/** union for selecting clock prescalers */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
    
    /// CPU clock prescaler
    uint8_t CPUDIV  : 3;
    
    /// High speed internal clock prescaler
    uint8_t HSIDIV  : 2;
    
    /// Reserved, must be kept cleared.
    uint8_t res     : 3;

  } reg;

} CLK_CKDIVR_t;

/// register for selecting clock prescalers
reg(0x50c6, CLK_CKDIVR_t, CLK_CKDIVR);



/// Peripheral clock gating register 1 (CLK_PCKENR1)
/** union to en-/disable clocking of peripherals 1/2 */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// clock enable I2C
    uint8_t PCKEN_I2C       : 1;
  
    /// clock enable SPI
    uint8_t PCKEN_SPI       : 1;
  
    /// clock enable UART1
    uint8_t PCKEN_UART1     : 1;
  
    /// clock enable UART2
    uint8_t PCKEN_UART2     : 1;
  
    /// clock enable TIM4/TIM6
    uint8_t PCKEN_TIM4_TIM6 : 1;
  
    /// clock enable TIM4/TIM6
    uint8_t PCKEN_TIM2_TIM5 : 1;
  
    /// clock enable TIM3
    uint8_t PCKEN_TIM3      : 1;
  
    /// clock enable TIM1
    uint8_t PCKEN_TIM1      : 1;
    
  } reg;
  
} CLK_PCKENR1_t;

/// register to en-/disable clocking of peripherals 1/2
reg(0x50c7, CLK_PCKENR1_t, CLK_PCKENR1);



/// Clock security system register (CLK_CSSR)
/** union to control/monitor clock supervision */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
    
    /// Clock security system enable
    uint8_t CSSEN   : 1;
    
    /// Auxiliary oscillator connected to master clock
    uint8_t AUX     : 1;
    
    /// Clock security system detection interrupt enable
    uint8_t CSSDIE  : 1;
    
    /// Clock security system detection
    uint8_t CSSD    : 1;
    
    /// Reserved, must be kept cleared.
    uint8_t res     : 4;
    
  } reg;
  
} CLK_CSSR_t;

/// register to control/monitor clock supervision
reg(0x50c8, CLK_CSSR_t, CLK_CSSR);



/// Configurable clock output register (CLK_CCOR)
/** union to output an internal clock via GPIO */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Configurable clock output enable
    uint8_t CCOEN   : 1;
  
    /// Configurable clock output selection.
    uint8_t CCOSEL  : 4;
  
    /// Configurable clock output ready
    uint8_t CCORDY  : 1;
  
    /// Configurable clock output busy
    uint8_t CCOBSY  : 1;
  
    /// Reserved, must be kept cleared.
    uint8_t res     : 1;

  } reg;

} CLK_CCOR_t;

/// register to output an internal clock via GPIO
reg(0x50c9, CLK_CCOR_t, CLK_CCOR);



/// Peripheral clock gating register 2 (CLK_PCKENR2)
/** union for en-/disabling clocking of peripherals 2/2 */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Reserved
    uint8_t res         : 2;

    /// clock enable AWU
    uint8_t PCKEN_AWU   : 1;

    /// clock enable ADC
    uint8_t PCKEN_ADC   : 1;
  
    /// Reserved
    uint8_t res2        : 3;

    /// clock enable CAN
    uint8_t PCKEN_CAN   : 1;

  } reg;

} CLK_PCKENR2_t;

/// register for en-/disabling clocking of peripherals 2/2
reg(0x50ca, CLK_PCKENR2_t, CLK_PCKENR2);



// CAN clock control (obsolete as of STM8 UM rev 7)
/*
typedef union {
  uint8_t  byte;
  struct {
    uint8_t CANDIV  : 3;
    uint8_t res     : 5;
  } reg;
} CLK_CANCCR_t;
reg(0x50cb, CLK_CANCCR_t, CLK_CANCCR);
*/



/// HSI clock calibration trimming register (CLK_HSITRIMR)
/** union for triming the internal HSI clock */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// HSI trimming value
    uint8_t HSITRIM : 4;
    
    /// Reserved, must be kept cleared.
    uint8_t res     : 4;
    
  } reg;
  
} CLK_HSITRIMR_t;

/// register for triming the internal HSI clock
reg(0x50cc, CLK_HSITRIMR_t, CLK_HSITRIMR);



/// SWIM clock control register (CLK_SWIMCCR)
/** union for reducing SWIM debug interface speed for robustness */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// SWIM clock divider
    uint8_t SWIMCLK : 1;
    
    /// Reserved.
    uint8_t res     : 7;
    
  } reg;
  
} CLK_SWIMCCR_t;

/// register for reducing SWIM debug interface speed for robustness
reg(0x50cd, CLK_SWIMCCR_t, CLK_SWIMCCR);



//--------
// Window Watchdog WWDG
//--------

/// WWDG Control register (WWDG_CR)
/** union for activation of the window watchdog */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// 7-bit WWDG counter (MSB to LSB)
    uint8_t T       : 7;
    
    /// WWDG activation bit (not used if WWDG is enabled by option byte)
    uint8_t WDGA    : 1;
    
  } reg;
  
} WWDG_CR_t;

/// register for activation of the window watchdog
reg(0x50d1, WWDG_CR_t, WWDG_CR);



/// WWDR Window register
/** union for setting the window watchdog period */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// 7-bit window value
    uint8_t W       : 7;
    
    /// Reserved
    uint8_t res     : 1;
    
  } reg;
  
} WWDG_WR_t;

/// register for setting the window watchdog period
reg(0x50d2, WWDG_WR_t, WWDG_WR);



//--------
// independent watchdog (IWDG)
//--------

/// IWDG Key register (IWDG_KR)
/** union for configuring and service the independent timeout watchdog */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Key value
    uint8_t KEY     : 8;
    
  } reg;
  
} IWDG_KR_t;

/// register for configuring and service the independent timeout watchdog
reg(0x50e0, IWDG_KR_t, IWDG_KR);



/// IWDG Prescaler register (IWDG_PR)
/** union for setting the prescaler for the independent timeout watchdog */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Prescalerdivider
    uint8_t PR      : 3;
    
    /// Reserved
    uint8_t res     : 5;
    
  } reg;
  
} IWDG_PR_t;

/// register for setting the prescaler for the independent timeout watchdog
reg(0x50e1, IWDG_PR_t, IWDG_PR);



/// IWDG Reload register (IWDG_RLR)
/** union for setting the reload value (=period) of the independent timeout watchdog */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
    
    /// Watchdog counter reload value
    uint8_t RL      : 8;
    
  } reg;
  
} IWDG_RLR_t;

/// register for setting the reload value (=period) of the independent timeout watchdog
reg(0x50e2, IWDG_RLR_t, IWDG_RLR);



//--------
// Flash
//--------

/// Flash control register 1 (FLASH_CR1)
/** union for general flash configuration */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
    
    /// Fixed Byte programming time
    uint8_t FIX     : 1;
    
    /// Flash Interrupt enable
    uint8_t IE      : 1;
    
    /// Power-down in Active-halt mode
    uint8_t AHALT   : 1;
    
    /// Power-down in Halt mode
    uint8_t HALT    : 1;
    
    /// Reserved
    uint8_t res     : 4;

  } reg;
} FLASH_CR1_t;

/// register for general flash configuration
reg(0x505a, FLASH_CR1_t, FLASH_CR1);



/// Flash control register 2 (FLASH_CR2)
/** union for triggering flash program/erase activity */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
    
    /// Standard block programming
    uint8_t PRG     : 1;
    
    /// Reserved
    uint8_t res     : 3;
    
    /// Fast block programming
    uint8_t FPRG    : 1;
    
    /// Block erasing
    uint8_t ERASE   : 1;
    
    /// Word programming
    uint8_t WPRG    : 1;
    
    /// Write option bytes
    uint8_t OPT     : 1;

  } reg;

} FLASH_CR2_t;

/// register for triggering flash program/erase activity (see also FLASH_CR2)
reg(0x505b, FLASH_CR2_t, FLASH_CR2);

/// complementary register for triggering flash program/erase activity. Write with (FLASH_CR2 ^ 0xFF)
reg(0x505c, FLASH_CR2_t, FLASH_NCR2);




/// Flash protection register (FLASH_FPR)
/** union for setting the size of user boot code area which is especially protected */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// User boot code area protection bits
    uint8_t WPB     : 6;
  
    /// Reserved
    uint8_t res     : 2;

  } reg;

} FLASH_FPR_t;

/// register for setting the size of user boot code area which is especially protected (see also FLASH_NFPR)
reg(0x505d, FLASH_FPR_t, FLASH_FPR);

/// complementary register for setting the size of user boot code area which is especially protected. Write with (FLASH_FPR ^ 0xFF)
reg(0x505e, FLASH_FPR_t, FLASH_NFPR);



/// Flash status register (FLASH_IAPSR)
/** union showing the state of the flash module */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
    
    /// Write attempted to protected page flag
    uint8_t WR_PG_DIS : 1;
    
    /// Flash Program memory unlocked flag
    uint8_t PUL       : 1;
    
    /// End of programming (write or erase operation) flag
    uint8_t EOP       : 1;
    
    /// Data EEPROM area unlocked flag
    uint8_t DUL       : 1;
    
    /// Reserved, forced by hardware to 0
    uint8_t res       : 2;
    
    /// End of high voltage flag
    uint8_t HVOFF     : 1;
    
    /// Reserved
    uint8_t res2      : 1;

  } reg;

} FLASH_IAPSR_t;

/// register showing the state of the flash module
reg(0x505f, FLASH_IAPSR_t, FLASH_IAPSR);



/// Flash program memory unprotecting key register (FLASH_PUKR)
/** union for unlocking program flash for write/erase */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
    
    /// Main program memory unlock keys
    uint8_t PUK     : 8;
    
  } reg;
  
} FLASH_PUKR_t;

/// register for unlocking program flash for write/erase
reg(0x5062, FLASH_PUKR_t, FLASH_PUKR);



/// Data EEPROM unprotection key register (FLASH_DUKR)
/** union for unlocking EEPROM area for write/erase */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// DataEEPROMwriteunlockkeys
    uint8_t DUK     : 8;
    
  } reg;
  
} FLASH_DUKR_t;

/// register for unlocking EEPROM area for write/erase
reg(0x5064, FLASH_DUKR_t, FLASH_DUKR);



//--------
// Auto Wake-Up (AWU)
//--------

/// AWU Control/status register (AWU_CSR)
/** union for controlling/monitoring the Auto Wake-Up (AWU) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
    
    /// LSI measurement enable
    uint8_t MSR     : 1;
    
    /// Reserved
    uint8_t res     : 3;
    
    /// Auto-wakeup enable
    uint8_t AWUEN   : 1;
    
    /// Auto-wakeup flag
    uint8_t AWUF    : 1;
    
    /// Reserved
    uint8_t res2    : 2;

  } reg;

} AWU_CSR_t;

/// register for controlling/monitoring the Auto Wake-Up (AWU)
reg(0x50f0, AWU_CSR_t, AWU_CSR);



/// AWU Asynchronous prescaler register (AWU_APR)
/** union for setting the the AWU clock prescaler divider */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Asynchronous prescaler divider
    uint8_t APR     : 6;
    
    /// Reserved
    uint8_t res     : 2;
    
  } reg;
  
} AWU_APR_t;

/// register for setting the the AWU clock prescaler divider
reg(0x50f1, AWU_APR_t, AWU_APR);



/// AWU Timebase selection register (AWU_TBR)
/** union for setting time intervals between AWU interrupts */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// 
    uint8_t AWUTB   : 4;
  
    /// 
    uint8_t res     : 4;

  } reg;

} AWU_TBR_t;

/// register for setting time intervals between AWU interrupts
reg(0x50f2, AWU_TBR_t, AWU_TBR);



//--------
// Misc
//--------

/// Beeper control/status register (BEEP_CSR)
/** union for beeper control */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Beep clock prescaler divider
    uint8_t BEEPDIV : 5;

    /// Beep enable
    uint8_t BEEPEN  : 1;
    
    /// Beeper frequency selection
    uint8_t BEEPSEL : 2;
    
  } reg;
  
} BEEP_CSR_t;

/// register for beeper control
reg(0x50f3, BEEP_CSR_t, BEEP_CSR);



/// Reset status register (RST_SR)
/** union for reading the source of the last reset event */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Window Watchdog reset flag
    uint8_t WWDGF   : 1;

    /// Independent Watchdog reset flag
    uint8_t IWDGF   : 1;

    /// Illegal opcode reset flag
    uint8_t ILLOPF  : 1;

    /// SWIM reset flag
    uint8_t SWIMF   : 1;

    /// EMC reset flag
    uint8_t EMCF    : 1;

    /// Reserved
    uint8_t res     : 3;

  } reg;

} RST_SR_t;

/// register for reading the source of the last reset event
reg(0x50b3, RST_SR_t, RST_SR);



/// Global configuration register (CFG_GCR)
/** union for configuration of SWIM pin as GPIO and behavior on return from interrupt (IRET) command */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// SWIM disable
    uint8_t SWD     : 1;
    
    /// Activation level
    uint8_t AL      : 1;
    
    /// Reserved
    uint8_t res     : 6;
    
  } reg;
  
} CFG_GCR_t;

/// register for configuration of SWIM pin as GPIO and behavior on return from interrupt (IRET) instruction
reg(0x7f60, CFG_GCR_t, CFG_GCR);



//--------
// Interrupts
//--------

/// External interrupt control register 1 (EXTI_CR1)
/** union for selecting edge/level for external interrupts via GPIOs (1/2) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Port A external interrupt sensitivity bits
    uint8_t PAIS    : 2;
  
    /// Port B external interrupt sensitivity bits
    uint8_t PBIS    : 2;
  
    /// Port C external interrupt sensitivity bits
    uint8_t PCIS    : 2;
  
    /// Port D external interrupt sensitivity bits
    uint8_t PDIS    : 2;

  } reg;

} EXTI_CR1_t;

/// register for selecting edge/level for external interrupts via GPIOs (1/2)
reg(0x50a0, EXTI_CR1_t, EXTI_CR1);



/// External interrupt control register 2 (EXTI_CR2)
/** union for selecting edge/level for external interrupts via GPIOs (2/2) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Port E external interrupt sensitivity bits
    uint8_t PEIS    : 2;
  
    /// Top level interrupt sensitivity
    uint8_t TLIS    : 1;
    
    /// Reserved
    uint8_t res     : 5;

  } reg;

} EXTI_CR2_t;

/// register for selecting edge/level for external interrupts via GPIOs (2/2)
reg(0x50a1, EXTI_CR2_t, EXTI_CR2);



/// Software priority register x (ITC_SPRx)
/** union for prioritization of software interrupts */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Vector 0,4,8,...28 software priority bits
    uint8_t VECT0SPR : 2;
  
    /// Vector 1,5,9,...29 software priority bits
    uint8_t VECT1SPR : 2;
  
    /// Vector 2,6,10,...26 software priority bits
    uint8_t VECT2SPR : 2;
  
    /// Vector 3,7,11,...27 software priority bits
    uint8_t VECT3SPR : 2;

  } reg;

} ITC_SPR_t;

/// register for prioritization of software interrupts (1/8)
reg(0x7f70, ITC_SPR_t, ITC_SPR1);

/// register for prioritization of software interrupts (2/8)
reg(0x7f71, ITC_SPR_t, ITC_SPR2);

/// register for prioritization of software interrupts (3/8)
reg(0x7f72, ITC_SPR_t, ITC_SPR3);

/// register for prioritization of software interrupts (4/8)
reg(0x7f73, ITC_SPR_t, ITC_SPR4);

/// register for prioritization of software interrupts (5/8)
reg(0x7f74, ITC_SPR_t, ITC_SPR5);

/// register for prioritization of software interrupts (6/8)
reg(0x7f75, ITC_SPR_t, ITC_SPR6);

/// register for prioritization of software interrupts (7/8)
reg(0x7f76, ITC_SPR_t, ITC_SPR7);

/// register for prioritization of software interrupts (8/8); VECT2SPR and VECT3SPR are reserved
reg(0x7f77, ITC_SPR_t, ITC_SPR8);



//--------
// Serial Peripheral Interface (SPI)
//--------

/// SPI control register 1 (SPI_CR1)
/** union for configuring SPI interface (1/2) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
    
    /// Clock phase
    uint8_t CPHA     : 1;
    
    /// Clock polarity
    uint8_t CPOL     : 1;
    
    /// Master/slave selection
    uint8_t MSTR     : 1;
    
    /// Baudrate control
    uint8_t BR       : 3;
    
    /// SPI enable
    uint8_t SPE      : 1;
    
    /// Frame format
    uint8_t LSBFIRST : 1;
    
  } reg;

} SPI_CR1_t;

/// register for configuring SPI interface (1/2) 
reg(0x5200, SPI_CR1_t, SPI_CR1);



/// SPI control register 2 (SPI_CR2)
/** union for configuring SPI interface (2/2) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
    
    /// Internal slave select
    uint8_t SSI     : 1;
    
    /// Software slave management
    uint8_t SSM     : 1;
    
    /// Receive only
    uint8_t RXOnly  : 1;
    
    /// Reserved
    uint8_t res     : 1;
    
    /// Transmit CRC next
    uint8_t CRCNEXT : 1;
    
    /// Hardware CRC calculation enable
    uint8_t CRCEN   : 1;
    
    /// Input/Output enable in bidirectional mode
    uint8_t BDOE    : 1;
    
    /// Bidirectional data mode enable
    uint8_t BDM     : 1;

  } reg;

} SPI_CR2_t;

/// register for configuring SPI interface (2/2) 
reg(0x5201, SPI_CR2_t, SPI_CR2);



/// SPI interrupt control register (SPI_ICR)
/** union for configuring SPI interrupt sources */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Reserved
    uint8_t res     : 4;
  
    /// Wakeup interrupt enable
    uint8_t WKIE    : 1;
  
    /// Error interrupt enable
    uint8_t ERRIE   : 1;
  
    /// Rx buffer not empty interrupt enable
    uint8_t RXIE    : 1;
  
    /// Tx buffer empty interrupt enable
    uint8_t TXIE    : 1;

  } reg;

} SPI_ICR_t;

/// register for configuring SPI interrupt sources
reg(0x5202, SPI_ICR_t, SPI_ICR);



/// SPI status register (SPI_SR)
/** union for reading the state of the SPI interface */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Receive buffer not empty
    uint8_t RxNE    : 1;

    /// Transmit buffer empty
    uint8_t TXE     : 1;

    /// Reserved
    uint8_t res     : 1;

    /// Wakeup flag
    uint8_t WKUP    : 1;

    /// CRC error flag
    uint8_t CRCERR  : 1;

    /// Mode fault
    uint8_t MODF    : 1;

    /// Overrun flag
    uint8_t OVR     : 1;

    /// Busy flag
    uint8_t BSY     : 1;

  } reg;

} SPI_SR_t;

/// register for reading the state of the SPI interface
reg(0x5203, SPI_SR_t, SPI_SR);



/// SPI data register (SPI_DR)
/** union containing the SPI data both Rx and Tx */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Data register
    uint8_t DR      : 8;
    
  } reg;
  
} SPI_DR_t;

/// register containing the SPI data both Rx and Tx
reg(0x5204, SPI_DR_t, SPI_DR);



/// SPI CRC polynomial register (SPI_CRCPR)
/** union for defining polynomial for CRC check */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// CRC polynomial register
    uint8_t CRCPOLY : 8;
    
  } reg;
  
} SPI_CRCPR_t;

/// register for defining polynomial for CRC check
reg(0x5205, SPI_CRCPR_t, SPI_CRCPR);



/// SPI Rx CRC register (SPI_RXCRCR)
/** union containing the computed CRC value of the Rx bytes */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Rx CRC Register
    uint8_t RxCRC   : 8;
    
  } reg;
  
} SPI_RXCRCR_t;

/// register containing the computed CRC value of the Rx bytes
reg(0x5206, SPI_RXCRCR_t, SPI_RXCRCR);



/// SPI Tx CRC register (SPI_TXCRCR)
/** union containing the computed CRC value of the Tx bytes */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Tx CRC register
    uint8_t TxCRC   : 8;
    
  } reg;
  
} SPI_TXCRCR_t;

/// register containing the computed CRC value of the Tx bytes
reg(0x5207, SPI_TXCRCR_t, SPI_TXCRCR);



//--------
// I2C Bus Interface (I2C)
//--------

/// I2C Control register 1 (I2C_CR1)
/** union for configuring I2C interface (1/2) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Peripheral enable
    uint8_t PE        : 1;
  
    /// Reserved
    uint8_t res       : 5;
  
    /// General call enable
    uint8_t ENGC      : 1;
  
    /// Clock stretching disable (Slave mode)
    uint8_t NOSTRETCH : 1;

  } reg;

} I2C_CR1_t;

/// register for configuring I2C interface (1/2)
reg(0x5210, I2C_CR1_t, I2C_CR1);



/// I2C Control register 2 (I2C_CR2)
/** union for configuring I2C interface (2/2) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Start generation
    uint8_t START     : 1;
  
    /// Stop generation
    uint8_t STOP      : 1;
  
    /// Acknowledge enable
    uint8_t ACK       : 1;
  
    /// Acknowledge position (for data reception)
    uint8_t POS       : 1;
  
    /// Reserved
    uint8_t res       : 3;
  
    /// Software reset
    uint8_t SWRST     : 1;

  } reg;

} I2C_CR2_t;

/// register for configuring I2C interface (2/2)
reg(0x5211, I2C_CR2_t, I2C_CR2);



/// I2C Frequency register (I2C_FREQR)
/** union for setting the I2C speed */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Peripheral clock frequency
    uint8_t FREQ      : 6;
  
    /// Reserved
    uint8_t res       : 2;

  } reg;

} I2C_FREQR_t;

/// register for setting the I2C speed
reg(0x5212, I2C_FREQR_t, I2C_FREQR);



/// I2C Own address register LSB (I2C_OARL)
/** union for setting the I2C address of the STM8 (low byte) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {
  
    /// Interface address [10] (in 10-bit address mode)
    uint8_t ADD0      : 1;
  
    /// Interface address [7:1]
    uint8_t ADD       : 7;

  } reg;

} I2C_OARL_t;

/// register for setting the I2C address of the STM8 (low byte)
reg(0x5213, I2C_OARL_t, I2C_OARL);



/// I2C own address high
/** union for setting the I2C address of the STM8 (high byte) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Reserved
    uint8_t res       : 1;

    /// Interface address [9:8] (in 10-bit address mode)
    uint8_t ADD       : 2;

    /// Reserved
    uint8_t res2      : 3;

    /// Address mode configuration (must always be written as ‘1’)
    uint8_t ADDCONF   : 1;
    
    /// 7-/10-bit addressing mode (Slave mode)
    uint8_t ADDMODE   : 1;
    
  } reg;
  
} I2C_OARH_t;

/// register for setting the I2C address of the STM8 (high byte)
reg(0x5214, I2C_OARH_t, I2C_OARH);



/// I2C Data register (I2C_DR)
/** union containing the I2C data (both Rx and Tx) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Data register
    uint8_t DR        : 8;
    
  } reg;
  
} I2C_DR_t;

/// register containing the I2C data (both Rx and Tx)
reg(0x5216, I2C_DR_t, I2C_DR);



/// I2C Status register 1 (I2C_SR1)
/** union for reading the I2C bus status (1/3) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Start bit (Mastermode)
    uint8_t SB        : 1;

    /// Address sent (master mode) / matched (slave mode)
    uint8_t ADDR      : 1;

    /// Byte transfer finished
    uint8_t BTF       : 1;

    /// 10-bit header sent (Master mode)
    uint8_t ADD10     : 1;

    /// Stop detection (Slave mode)
    uint8_t STOPF     : 1;

    /// Reserved
    uint8_t res       : 1;

    /// Data register not empty (receivers)
    uint8_t RXNE      : 1;

    /// Data register empty (transmitters)
    uint8_t TXE       : 1;
    
  } reg;
  
} I2C_SR1_t;

/// register for reading the I2C bus status (1/3)
reg(0x5217, I2C_SR1_t, I2C_SR1);



/// I2C Status register 2 (I2C_SR2)
/** union for reading the I2C bus status (2/3) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Bus error
    uint8_t BERR      : 1;

    /// Arbitration lost (master mode)
    uint8_t ARLO      : 1;

    /// Acknowledge failure
    uint8_t AF        : 1;

    /// Overrun/underrun
    uint8_t OVR       : 1;

    /// Reserved
    uint8_t res       : 1;

    /// Wakeup from Halt
    uint8_t WUFH      : 1;

    /// Reserved
    uint8_t res2      : 2;
    
  } reg;
  
} I2C_SR2_t;

/// register for reading the I2C bus status (2/3)
reg(0x5218, I2C_SR2_t, I2C_SR2);



/// I2C status 3
/** union for reading the I2C bus status (3/3) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Master/Slave
    uint8_t MSL       : 1;

    /// Bus busy
    uint8_t BUSY      : 1;

    /// Transmitter/Receiver
    uint8_t TRA       : 1;

    /// Reserved
    uint8_t res       : 1;

    /// General call header (Slavemode)
    uint8_t GENCALL   : 1;

    /// Reserved
    uint8_t res2      : 2;

    /// (not documented)
    uint8_t DUALF     : 1;
    
  } reg;
  
} I2C_SR3_t;

/// register for reading the I2C bus status (3/3)
reg(0x5219, I2C_SR3_t, I2C_SR3);



/// I2C Interrupt register (I2C_ITR)
/** union for configuring I2C interrupts */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Error interrupt enable
    uint8_t ITERREN   : 1;

    /// Event interrupt enable
    uint8_t ITEVTEN   : 1;

    /// Buffer interrupt enable
    uint8_t ITBUFEN   : 1;

    /// Reserved
    uint8_t res       : 5;
    
  } reg;
  
} I2C_ITR_t;

/// register for configuring I2C interrupts
reg(0x521a, I2C_ITR_t, I2C_ITR);



/// I2C Clock control register low (I2C_CCRL)
/** union for configuring I2C clock (1/2) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Clock control register (Master mode)
    uint8_t CCR       : 8;
    
  } reg;
  
} I2C_CCRL_t;

/// register for configuring I2C clock (1/2)
reg(0x521b, I2C_CCRL_t, I2C_CCRL);



/// I2C Clock control register high (I2C_CCRH)
/** union for configuring I2C clock (2/2) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Clock control register in Fast/Standard mode (Master mode)
    uint8_t CCR       : 4;

    /// Reserved
    uint8_t res       : 2;

    /// Fast mode duty cycle
    uint8_t DUTY      : 1;

    /// I2C master mode selection
    uint8_t FS        : 1;
    
  } reg;
  
} I2C_CCRH_t;

/// register for configuring I2C clock (2/2)
reg(0x521c, I2C_CCRH_t, I2C_CCRH);



/// I2C TRISE register (I2C_TRISER)
/** union for configuring the rise time of the SCL line */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Maximum rise time in Fast/Standard mode (Master mode)
    uint8_t TRISE     : 6;

    /// Reserved
    uint8_t res       : 2;
    
  } reg;
  
} I2C_TRISER_t;

/// register for configuring the rise time of the SCL line
reg(0x521d, I2C_TRISER_t, I2C_TRISER);



// I2C packet error checking (undocumented in STM8 UM rev 9)
/*
typedef union {
  uint8_t  byte;
  struct {
    uint8_t res       : 8;
  } reg;
} I2C_PECR_t;
reg(0x521e, I2C_PECR_t, I2C_PECR);
*/

//xxx

//--------
// Controller Area Network (CAN)
//--------

// CAN Master Control
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t INRQ      : 1;

    /// 
    uint8_t SLEEP     : 1;

    /// 
    uint8_t TXFP      : 1;

    /// 
    uint8_t RFLM      : 1;

    /// 
    uint8_t NART      : 1;

    /// 
    uint8_t AWUM      : 1;

    /// 
    uint8_t ABOM      : 1;

    /// 
    uint8_t TTCM      : 1;
    
  } reg;
  
} CAN_MCR_t;

/// register 
reg(0x5420, CAN_MCR_t, CAN_MCR);



/// CAN Master Status
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t INAK      : 1;

    /// 
    uint8_t SLAK      : 1;

    /// 
    uint8_t ERRI      : 1;

    /// 
    uint8_t WKUI      : 1;

    /// 
    uint8_t TX        : 1;

    /// 
    uint8_t RX        : 1;

    /// 
    uint8_t res       : 2;
    
  } reg;
  
} CAN_MSR_t;

/// register 
reg(0x5421, CAN_MSR_t, CAN_MSR);



/// CAN Transmit Status
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t RQCP0     : 1;

    /// 
    uint8_t RQCP1     : 1;

    /// 
    uint8_t RQCP2     : 1;

    /// 
    uint8_t res       : 1;

    /// 
    uint8_t TXOK0     : 1;

    /// 
    uint8_t TXOK1     : 1;

    /// 
    uint8_t TXOK2     : 1;

    /// 
    uint8_t res2      : 1;
    
  } reg;
  
} CAN_TSR_t;

/// register 
reg(0x5422, CAN_TSR_t, CAN_TSR);



/// CAN Transmit Priority
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t CODE0     : 1;

    /// 
    uint8_t CODE1     : 1;

    /// 
    uint8_t TME0      : 1;

    /// 
    uint8_t TME1      : 1;

    /// 
    uint8_t TME2      : 1;

    /// 
    uint8_t LOW0      : 1;

    /// 
    uint8_t LOW1      : 1;

    /// 
    uint8_t LOW2      : 1;
    
  } reg;
  
} CAN_TPR_t;

/// register 
reg(0x5423, CAN_TPR_t, CAN_TPR);



/// CAN Receive FIFO
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t FMP       : 2;

    /// 
    uint8_t res       : 1;

    /// 
    uint8_t FULL      : 1;

    /// 
    uint8_t FOVR      : 1;

    /// 
    uint8_t RFOM      : 1;

    /// 
    uint8_t res2      : 2;
    
  } reg;
  
} CAN_RFR_t;

/// register 
reg(0x5424, CAN_RFR_t, CAN_RFR);



/// CAN Interrupt Enable
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t TMEIE     : 1;

    /// 
    uint8_t FMPIE     : 1;

    /// 
    uint8_t FFIE      : 1;

    /// 
    uint8_t FOVIE     : 1;

    /// 
    uint8_t res       : 3;

    /// 
    uint8_t WKUIE     : 1;
    
  } reg;
  
} CAN_IER_t;

/// register 
reg(0x5425, CAN_IER_t, CAN_IER);



/// CAN Diagnosis
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t LBKM      : 1;

    /// 
    uint8_t SILM      : 1;

    /// 
    uint8_t SAMP      : 1;

    /// 
    uint8_t RX        : 1;

    /// 
    uint8_t TXM2E     : 1;

    /// 
    uint8_t res       : 3;
    
  } reg;
  
} CAN_DGR_t;

/// register 
reg(0x5426, CAN_DGR_t, CAN_DGR);



/// CAN Filter Page Select (for below registers)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t PS        : 3;

    /// 
    uint8_t res       : 5;
    
  } reg;
  
} CAN_PSR_t;

/// register 
reg(0x5427, CAN_PSR_t, CAN_PSR);


//---
// CAN register page 0 (Tx Mailbox 0)
//---


/// CAN message control/status register (page 0,1,5)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t TXRQ      : 1;

    /// 
    uint8_t ABRQ      : 1;

    /// 
    uint8_t RQCP      : 1;

    /// 
    uint8_t TXOK      : 1;

    /// 
    uint8_t ALST      : 1;

    /// 
    uint8_t TERR      : 1;

    /// 
    uint8_t res       : 2;
    
  } reg;
  
} CAN_MCSR_t;

/// register 
reg(0x5428, CAN_MCSR_t, CAN_MCSR);



/// CAN mailbox data length control register (page 0,1,5,7)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t DLC       : 4;

    /// 
    uint8_t res       : 3;

    /// 
    uint8_t TGT       : 1;
    
  } reg;
  
} CAN_MDLCR_t;

/// register 
reg(0x5429, CAN_MDLCR_t, CAN_MDLCR);



/// CAN mailbox time stamp register (page 0,1,5,7, 16bit)
reg(0x542A, word_t, CAN_MTSR);



/// CAN mailbox identifier register 1 (page 0,1,5,7)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t ID        : 5;  // STID or EXID
    uint8_t RTR       : 1;

    /// 
    uint8_t IDE       : 1;

    /// 
    uint8_t res       : 1;
    
  } reg;
  
} CAN_MIDR1_t;

/// register 
reg(0x542C, CAN_MIDR1_t, CAN_MIDR1);



/// CAN mailbox identifier register 2 (page 0,1,5,7)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t EXID      : 2;  // EXID[17:16]
    uint8_t ID        : 6;  // STID[5:0] or EXID[23:18]
  } reg;
} CAN_MIDR2_t;

/// register 
reg(0x542D, CAN_MIDR2_t, CAN_MIDR2);



/// CAN mailbox identifier register 3 (page 0,1,5,7)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t EXID      : 8;
    
  } reg;
  
} CAN_MIDR3_t;

/// register 
reg(0x542E, CAN_MIDR3_t, CAN_MIDR3);



/// CAN mailbox identifier register 4 (page 0,1,5,7)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t EXID      : 8;
    
  } reg;
  
} CAN_MIDR4_t;

/// register 
reg(0x542F, CAN_MIDR4_t, CAN_MIDR4);



/// CAN mailbox data register x (x=1..8, (page 0,1,5,7)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t DATA      : 8;
    
  } reg;
  
} CAN_MDAR_t;

/// register 
reg(0x5430, CAN_MDAR_t, CAN_MDAR1);

/// register 
reg(0x5431, CAN_MDAR_t, CAN_MDAR2);

/// register 
reg(0x5432, CAN_MDAR_t, CAN_MDAR3);

/// register 
reg(0x5433, CAN_MDAR_t, CAN_MDAR4);

/// register 
reg(0x5434, CAN_MDAR_t, CAN_MDAR5);

/// register 
reg(0x5435, CAN_MDAR_t, CAN_MDAR6);

/// register 
reg(0x5436, CAN_MDAR_t, CAN_MDAR7);

/// register 
reg(0x5437, CAN_MDAR_t, CAN_MDAR8);


//---
// CAN register page 1 (Tx Mailbox 1)
//---

// CAN_MCSR (0x5428, see page 0
// CAN_MDLCR (0x5429, see page 0
// CAN_MTSRL (0x542A, see page 0
// CAN_MTSRH (0x542B, see page 0
// CAN_MIDR1 (0x542C, see page 0
// CAN_MIDR2 (0x542D, see page 0
// CAN_MIDR3 (0x542E, see page 0
// CAN_MIDR4 (0x542F, see page 0
// CAN_MDAR1 (0x5430, see page 0
// CAN_MDAR2 (0x5431, see page 0
// CAN_MDAR3 (0x5432, see page 0
// CAN_MDAR4 (0x5433, see page 0
// CAN_MDAR5 (0x5434, see page 0
// CAN_MDAR6 (0x5435, see page 0
// CAN_MDAR7 (0x5436, see page 0
// CAN_MDAR8 (0x5437, see page 0


//---
// CAN register page 2 (Acceptance Filter 0:1)
//---


/// CAN acceptance filters FxRn (register page 2)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t MASK    : 8;
    
  } reg;
  
} CAN_FxRn_t;

/// register 
reg(0x5428, CAN_FxRn_t, CAN_F0R1);

/// register 
reg(0x5429, CAN_FxRn_t, CAN_F0R2);

/// register 
reg(0x542A, CAN_FxRn_t, CAN_F0R3);

/// register 
reg(0x542B, CAN_FxRn_t, CAN_F0R4);

/// register 
reg(0x542C, CAN_FxRn_t, CAN_F0R5);

/// register 
reg(0x542D, CAN_FxRn_t, CAN_F0R6);

/// register 
reg(0x542E, CAN_FxRn_t, CAN_F0R7);

/// register 
reg(0x542F, CAN_FxRn_t, CAN_F0R8);

/// register 
reg(0x5430, CAN_FxRn_t, CAN_F1R1);

/// register 
reg(0x5431, CAN_FxRn_t, CAN_F1R2);

/// register 
reg(0x5432, CAN_FxRn_t, CAN_F1R3);

/// register 
reg(0x5433, CAN_FxRn_t, CAN_F1R4);

/// register 
reg(0x5434, CAN_FxRn_t, CAN_F1R5);

/// register 
reg(0x5435, CAN_FxRn_t, CAN_F1R6);

/// register 
reg(0x5436, CAN_FxRn_t, CAN_F1R7);

/// register 
reg(0x5437, CAN_FxRn_t, CAN_F1R8);

	
//---
// CAN register page 3 (Acceptance Filter 2:3)
//---

/// CAN acceptance filters FxRn (register page 3)
reg(0x5428, CAN_FxRn_t, CAN_F2R1);

/// register 
reg(0x5429, CAN_FxRn_t, CAN_F2R2);

/// register 
reg(0x542A, CAN_FxRn_t, CAN_F2R3);

/// register 
reg(0x542B, CAN_FxRn_t, CAN_F2R4);

/// register 
reg(0x542C, CAN_FxRn_t, CAN_F2R5);

/// register 
reg(0x542D, CAN_FxRn_t, CAN_F2R6);

/// register 
reg(0x542E, CAN_FxRn_t, CAN_F2R7);

/// register 
reg(0x542F, CAN_FxRn_t, CAN_F2R8);

/// register 
reg(0x5430, CAN_FxRn_t, CAN_F3R1);

/// register 
reg(0x5431, CAN_FxRn_t, CAN_F3R2);

/// register 
reg(0x5432, CAN_FxRn_t, CAN_F3R3);

/// register 
reg(0x5433, CAN_FxRn_t, CAN_F3R4);

/// register 
reg(0x5434, CAN_FxRn_t, CAN_F3R5);

/// register 
reg(0x5435, CAN_FxRn_t, CAN_F3R6);

/// register 
reg(0x5436, CAN_FxRn_t, CAN_F3R7);

/// register 
reg(0x5437, CAN_FxRn_t, CAN_F3R8);


//---
// CAN register page 4 (Acceptance Filter 4:5)
//---


/// CAN acceptance filters FxRn (register page 4)
reg(0x5428, CAN_FxRn_t, CAN_F4R1);

/// register 
reg(0x5429, CAN_FxRn_t, CAN_F4R2);

/// register 
reg(0x542A, CAN_FxRn_t, CAN_F4R3);

/// register 
reg(0x542B, CAN_FxRn_t, CAN_F4R4);

/// register 
reg(0x542C, CAN_FxRn_t, CAN_F4R5);

/// register 
reg(0x542D, CAN_FxRn_t, CAN_F4R6);

/// register 
reg(0x542E, CAN_FxRn_t, CAN_F4R7);

/// register 
reg(0x542F, CAN_FxRn_t, CAN_F4R8);

/// register 
reg(0x5430, CAN_FxRn_t, CAN_F5R1);

/// register 
reg(0x5431, CAN_FxRn_t, CAN_F5R2);

/// register 
reg(0x5432, CAN_FxRn_t, CAN_F5R3);

/// register 
reg(0x5433, CAN_FxRn_t, CAN_F5R4);

/// register 
reg(0x5434, CAN_FxRn_t, CAN_F5R5);

/// register 
reg(0x5435, CAN_FxRn_t, CAN_F5R6);

/// register 
reg(0x5436, CAN_FxRn_t, CAN_F5R7);

/// register 
reg(0x5437, CAN_FxRn_t, CAN_F5R8);


//---
// CAN register page 5 (Tx Mailbox 2)
//---

// CAN_MCSR (0x5428, see page 0
// CAN_MDLCR (0x5429, see page 0
// CAN_MTSRL (0x542A, see page 0
// CAN_MTSRH (0x542B, see page 0
// CAN_MIDR1 (0x542C, see page 0
// CAN_MIDR2 (0x542D, see page 0
// CAN_MIDR3 (0x542E, see page 0
// CAN_MIDR4 (0x542F, see page 0
// CAN_MDAR1 (0x5430, see page 0
// CAN_MDAR2 (0x5431, see page 0
// CAN_MDAR3 (0x5432, see page 0
// CAN_MDAR4 (0x5433, see page 0
// CAN_MDAR5 (0x5434, see page 0
// CAN_MDAR6 (0x5435, see page 0
// CAN_MDAR7 (0x5436, see page 0
// CAN_MDAR8 (0x5437, see page 0


//---
// CAN register page 6 (Configuration/Diagnostic)
//---


/// CAN error status register (register page 6)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t EWGF      : 1;

    /// 
    uint8_t EPVF      : 1;

    /// 
    uint8_t BOFF      : 1;

    /// 
    uint8_t res       : 1;

    /// 
    uint8_t LEC       : 3;

    /// 
    uint8_t res2      : 1;
    
  } reg;
  
} CAN_ESR_t;

/// register 
reg(0x5428, CAN_ESR_t, CAN_ESR);



/// CAN error interrupt enable register (register page 6)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t EWGIE     : 1;

    /// 
    uint8_t EPVIE     : 1;

    /// 
    uint8_t BOFIE     : 1;

    /// 
    uint8_t res       : 1;

    /// 
    uint8_t LECIE     : 1;

    /// 
    uint8_t res2      : 2;

    /// 
    uint8_t ERRIE     : 1;
    
  } reg;
  
} CAN_EIER_t;

/// register 
reg(0x5429, CAN_EIER_t, CAN_EIER);



/// CAN transmit error counter register (register page 6)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t TEC       : 8;
    
  } reg;
  
} CAN_TECR_t;

/// register 
reg(0x542C, CAN_TECR_t, CAN_TECR);



/// CAN receive error counter register (register page 6)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t REC       : 8;
    
  } reg;
  
} CAN_RECR_t;

/// register 
reg(0x542D, CAN_RECR_t, CAN_RECR);



/// CAN bit timing register 1 (register page 6)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t BRP       : 6;

    /// 
    uint8_t SJW       : 2;
    
  } reg;
  
} CAN_BTR1_t;

/// register 
reg(0x542E, CAN_BTR1_t, CAN_BTR1);



/// CAN bit timing register 2 (register page 6)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t BS1       : 4;

    /// 
    uint8_t BS2       : 3;

    /// 
    uint8_t res       : 1;
    
  } reg;
  
} CAN_BTR2_t;

/// register 
reg(0x542F, CAN_BTR2_t, CAN_BTR2);



/// CAN filter mode register 1 (register page 6)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t FML0      : 1;

    /// 
    uint8_t FMH0      : 1;

    /// 
    uint8_t FML1      : 1;

    /// 
    uint8_t FMH1      : 1;

    /// 
    uint8_t FML2      : 1;

    /// 
    uint8_t FMH2      : 1;

    /// 
    uint8_t FML3      : 1;

    /// 
    uint8_t FMH3      : 1;
    
  } reg;
  
} CAN_FMR1_t;

/// register 
reg(0x5432, CAN_FMR1_t, CAN_FMR1);



/// CAN filter mode register 2 (register page 6)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t FML4      : 1;

    /// 
    uint8_t FMH4      : 1;

    /// 
    uint8_t FML5      : 1;

    /// 
    uint8_t FMH5      : 1;

    /// 
    uint8_t res       : 4;
    
  } reg;
  
} CAN_FMR2_t;

/// register 
reg(0x5433, CAN_FMR2_t, CAN_FMR2);



/// CAN filter configuration register 1 (register page 6)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t FACT0     : 1;

    /// 
    uint8_t FSC00     : 1;

    /// 
    uint8_t FSC01     : 1;

    /// 
    uint8_t res       : 1;

    /// 
    uint8_t FACT1     : 1;

    /// 
    uint8_t FSC10     : 1;

    /// 
    uint8_t FSC11     : 1;

    /// 
    uint8_t res2      : 1;
    
  } reg;
  
} CAN_FCR1_t;

/// register 
reg(0x5434, CAN_FCR1_t, CAN_FCR1);



/// CAN filter configuration register 2 (register page 6)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t FACT2     : 1;

    /// 
    uint8_t FSC20     : 1;

    /// 
    uint8_t FSC21     : 1;

    /// 
    uint8_t res       : 1;

    /// 
    uint8_t FACT3     : 1;

    /// 
    uint8_t FSC30     : 1;

    /// 
    uint8_t FSC31     : 1;

    /// 
    uint8_t res2      : 1;
    
  } reg;
  
} CAN_FCR2_t;

/// register 
reg(0x5435, CAN_FCR2_t, CAN_FCR2);



/// CAN filter configuration register 3 (register page 6)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t FACT4     : 1;

    /// 
    uint8_t FSC40     : 1;

    /// 
    uint8_t FSC41     : 1;

    /// 
    uint8_t res       : 1;

    /// 
    uint8_t FACT5     : 1;

    /// 
    uint8_t FSC50     : 1;

    /// 
    uint8_t FSC51     : 1;

    /// 
    uint8_t res2      : 1;
    
  } reg;
  
} CAN_FCR3_t;

/// register 
reg(0x5436, CAN_FCR3_t, CAN_FCR3);


//---
// CAN register page 7 (Receive FIFO)
//---


/// CAN mailbox filter match index register (register page 7)
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t FMI     : 8;
    
  } reg;
  
} CAN_MFMIR_t;

/// register 
reg(0x5428, CAN_MFMIR_t, CAN_MFMIR);


/// CAN_MDLCR (0x5429, see page 0)
// CAN_MTSRL (0x542A, see page 0)
// CAN_MTSRH (0x542B, see page 0)
// CAN_MIDR1 (0x542C, see page 0)
// CAN_MIDR2 (0x542D, see page 0)
// CAN_MIDR3 (0x542E, see page 0)
// CAN_MIDR4 (0x542F, see page 0)
// CAN_MDAR1 (0x5430, see page 0)
// CAN_MDAR2 (0x5431, see page 0)
// CAN_MDAR3 (0x5432, see page 0)
// CAN_MDAR4 (0x5433, see page 0)
// CAN_MDAR5 (0x5434, see page 0)
// CAN_MDAR6 (0x5435, see page 0)
// CAN_MDAR7 (0x5436, see page 0)
// CAN_MDAR8 (0x5437, see page 0)

//xxx

//--------
// LIN Universal asynch. receiver transmitter (LINUART)
//--------

/// UART Status register (UART_SR)
/** union for reading the state of the UART interface */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Parity error
    uint8_t PE      : 1;

    /// Framing error
    uint8_t FE      : 1;

    /// Noise flag
    uint8_t NF      : 1;

    /// LIN Header Error (LIN slave mode) / Overrun error
    uint8_t OR_LHE  : 1;

    /// IDLE line detected
    uint8_t IDLE    : 1;

    /// Read data register not empty
    uint8_t RXNE    : 1;

    /// Transmission complete
    uint8_t TC      : 1;

    /// Transmit data register empty
    uint8_t TXE     : 1;
    
  } reg;
  
} UART_SR_t;

/// register for reading the state of the LINUART interface
reg(0x5240, UART_SR_t, LINUART_SR);



/// UART Data register (UART_DR)
/** union containing UART receive and transmit data */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Data value
    uint8_t DR      : 8;
    
  } reg;
  
} UART_DR_t;

/// register containing LINUART receive and transmit data
reg(0x5241, UART_DR_t, LINUART_DR);



/// UART Baud rate register 1 (UART_BRR1)
/** union for setting the UART baudrate (1/2) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// UART_DIV bits [11:4]
    uint8_t UART_DIV_4_11 : 8;
    
  } reg;
  
} UART_BRR1_t;

/// register for setting the LINUART baudrate (1/2)
reg(0x5242, UART_BRR1_t, LINUART_BRR1);



/// UART Baud rate register 2 (UART_BRR2)
/** union for setting the UART baudrate (2/2) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// UART_DIV bits [3:0]
    uint8_t UART_DIV_0_3   : 4;

    /// UART_DIV bits [15:12]
    uint8_t UART_DIV_12_15 : 4;
    
  } reg;
  
} UART_BRR2_t;

/// register for setting the LINUART baudrate (2/2)
reg(0x5243, UART_BRR2_t, LINUART_BRR2);



/// UART Control register 1 (UART_CR1)
/** union for configuring the UART interface (1/6) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Parity interrupt enable
    uint8_t PIEN    : 1;

    /// Parity selection
    uint8_t PS      : 1;

    /// Parity control enable
    uint8_t PCEN    : 1;

    /// Wakeup method
    uint8_t WAKE    : 1;

    /// word length
    uint8_t M       : 1;

    /// UART Disable (for low power consumption)
    uint8_t UARTD   : 1;

    /// Transmit Data bit 8 (in 9-bit mode)
    uint8_t T8      : 1;

    /// Receive Data bit 8 (in 9-bit mode)
    uint8_t R8      : 1;
    
  } reg;
  
} UART_CR1_t;

/// register for configuring the LINUART interface (1/6)
reg(0x5244, UART_CR1_t, LINUART_CR1);



/// UART Control register 2 (UART_CR2)
/** union for configuring the UART interface (2/6) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Send break
    uint8_t SBK     : 1;

    /// Receiver wakeup
    uint8_t RWU     : 1;

    /// Receiver enable
    uint8_t REN     : 1;

    /// Transmitter enable
    uint8_t TEN     : 1;

    /// IDLE Line interrupt enable
    uint8_t ILIEN   : 1;

    /// Receiver interrupt enable
    uint8_t RIEN    : 1;

    /// Transmission complete interrupt enable
    uint8_t TCIEN   : 1;

    /// Transmitter interrupt enable
    uint8_t TIEN    : 1;
    
  } reg;
  
} UART_CR2_t;

/// register for configuring the LINUART interface (2/6)
reg(0x5245, UART_CR2_t, LINUART_CR2);



/// UART Control register 3 (UART_CR3)
/** union for configuring the UART interface (3/6) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Last bit clock pulse
    uint8_t LBCL    : 1;

    /// Clock phase
    uint8_t CPHA    : 1;

    /// Clock polarity
    uint8_t CPOL    : 1;

    /// Clock enable
    uint8_t CKEN    : 1;

    /// STOP bits
    uint8_t STOP    : 2;

    /// LIN mode enable
    uint8_t LINEN   : 1;

    /// Reserved, must be kept cleared
    uint8_t res     : 1;
    
  } reg;
  
} UART_CR3_t;

/// register for configuring the LINUART interface (3/6)
reg(0x5246, UART_CR3_t, LINUART_CR3);



/// UART Control register 4 (UART_CR4)
/** union for configuring the UART interface (4/6) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Address of the UART node
    uint8_t ADD     : 4;

    /// LIN Break Detection Flag
    uint8_t LBDF    : 1;

    /// LIN Break Detection Length
    uint8_t LBDL    : 1;

    /// LIN Break Detection Interrupt Enable
    uint8_t LBDIEN  : 1;

    /// Reserved, must be kept cleared
    uint8_t res     : 1;
    
  } reg;
  
} UART_CR4_t;

/// register for configuring the LINUART interface (4/6)
reg(0x5247, UART_CR4_t, LINUART_CR4);



/// UART Control register 5 (UART_CR5)
/** union for configuring the UART interface (5/6) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Reserved, must be kept cleared
    uint8_t res     : 1;

    /// IrDA mode Enable
    uint8_t IREN    : 1;

    /// IrDA Low Power
    uint8_t IRLP    : 1;

    /// Half-Duplex Selection
    uint8_t HDSEL   : 1;

    /// Smartcard NACK enable
    uint8_t NACK    : 1;

    /// Smartcard mode enable
    uint8_t SCEN    : 1;

    /// Reserved, must be kept cleared
    uint8_t res2    : 2;
    
  } reg;
  
} UART_CR5_t;

/// register for configuring the LINUART interface (5/6)
reg(0x5248, UART_CR5_t, LINUART_CR5);



/// LINUART Control register 6 (UART_CR6)
/** union for configuring the LINUART interface (6/6) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// LIN Sync Field
    uint8_t LSF     : 1;

    /// LIN Header Detection Flag
    uint8_t LHDF    : 1;

    /// LIN Header Detection Interrupt Enable
    uint8_t LHDIEN  : 1;

    /// Reserved
    uint8_t res     : 1;

    /// LIN automatic resynchronisation enable
    uint8_t LASE    : 1;

    /// LIN Slave Enable
    uint8_t LSLV    : 1;

    /// Reserved
    uint8_t res2    : 1;

    /// LIN Divider Update Method
    uint8_t LDUM    : 1;
    
  } reg;
  
} UART_CR6_t;

/// register for configuring the LINUART interface (6/6)
reg(0x5249, UART_CR6_t, LINUART_CR6);



/// UART Guard time register (UART_GTR)
/** union for configuring the UART guard time in Smartcard mode */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Guard time value (Smartcard mode)
    uint8_t GT      : 8;
    
  } reg;
  
} UART_GTR_t;

/// register for configuring the LINUART guard time in Smartcard mode
reg(0x524A, UART_GTR_t, LINUART_GTR);



/// UART Prescaler register (UART_PSCR)
/** union containing the clock prescaler for the UART baudrate generator */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Prescaler value
    uint8_t PSR     : 8;
    
  } reg;
  
} UART_PSCR_t;

/// register containing the clock prescaler for the LINUART baudrate generator
reg(0x524B, UART_PSCR_t, LINUART_PSCR);



//--------
// Universal asynch. receiver transmitter (UART)
// use unions of LINUART (are identical except LIN support configuration)
//--------

/// register for reading the state of the UART interface
reg(0x5230, UART_SR_t,   USART_SR);

/// register containing UART receive and transmit data
reg(0x5231, UART_DR_t,   USART_DR);

/// register for setting the UART baudrate (1/2)
reg(0x5232, UART_BRR1_t, USART_BRR1);

/// register for setting the UART baudrate (2/2)
reg(0x5233, UART_BRR2_t, USART_BRR2);

/// register  for configuring the UART interface (1/5)
reg(0x5234, UART_CR1_t,  USART_CR1);

/// register for configuring the UART interface (2/5)
reg(0x5235, UART_CR2_t,  USART_CR2);

/// register for configuring the UART interface (3/5)
reg(0x5236, UART_CR3_t,  USART_CR3);

/// register for configuring the UART interface (4/5)
reg(0x5237, UART_CR4_t,  USART_CR4);

/// register for configuring the UART interface (5/5)
reg(0x5238, UART_CR5_t,  USART_CR5);

/// register for configuring the UART guard time in Smartcard mode
reg(0x5239, UART_GTR_t,  USART_GTR);

/// register containing the clock prescaler for the UART baudrate generator
reg(0x523a, UART_PSCR_t, USART_PSCR);



//--------
// Analog Digital Converter (ADC)
//--------


/// ADC control/status register (ADC_CSR)
/** union for controlling / monitoring the analog-digital-converter */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Channel selection bits
    uint8_t CH      : 4;

    /// Analog watchdog interrupt enable
    uint8_t AWDIE   : 1;

    /// Interrupt enable for EOC
    uint8_t EOCIE   : 1;

    /// Analog Watchdog flag
    uint8_t AWD     : 1;

    /// End of conversion
    uint8_t EOC     : 1;
    
  } reg;
  
} ADC_CSR_t;

/// register for controlling / monitoring the analog-digital-converter
reg(0x5400, ADC_CSR_t, ADC_CSR);



/// ADC Configuration 1
/** union for configuring the analog-digital-converter (1/3) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// A/D Converter on/off
    uint8_t ADON    : 1;

    /// Continuous conversion
    uint8_t CONT    : 1;

    /// Reserved, always read as 0
    uint8_t res     : 2;

    /// Clock prescaler selection
    uint8_t SPSEL   : 3;

    /// Reserved, always read as 0
    uint8_t res2    : 1;
    
  } reg;
  
} ADC_CR1_t;

/// register for configuring the analog-digital-converter (1/3)
reg(0x5401, ADC_CR1_t, ADC_CR1);



/// ADC Configuration 2
/** union for configuring the analog-digital-converter (2/3) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Reserved, must be kept cleared
    uint8_t res     : 1;

    /// Scan mode enable
    uint8_t SCAN    : 1;

    /// Reserved, must be kept cleared
    uint8_t res2    : 1;

    /// Data alignment
    uint8_t ALIGN   : 1;

    /// External event selection
    uint8_t EXTSEL  : 2;

    /// External trigger enable
    uint8_t EXTTRIG : 1;

    /// Reserved, must be kept cleared
    uint8_t res3    : 1;
    
  } reg;
  
} ADC_CR2_t;

/// register for configuring the analog-digital-converter (2/3)
reg(0x5402, ADC_CR2_t, ADC_CR2);



/// ADC Configuration 3
/** union for configuring the analog-digital-converter (3/3) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Reserved, must be kept cleared
    uint8_t res     : 6;

    /// Overrun flag
    uint8_t OVR     : 1;

    /// Data buffer enable
    uint8_t DBUF    : 1;
    
  } reg;
  
} ADC_CR3_t;

/// register for configuring the analog-digital-converter (3/3)
reg(0x5403, ADC_CR3_t, ADC_CR3);



/// register for 10-bit ADC measurement result
reg(0x5404, word_t, ADC_DR);



/// ADC Schmitt Trigger Disable register
reg(0x5406, word_t, ADC_TDR);



//--------
// 16-Bit Timer TIM1
//--------


/// TIM1 Control register 1 (TIM1_CR1)
/** union for configuring timer TIM1 (1/2) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Counter enable
    uint8_t CEN     : 1;

    /// Update disable
    uint8_t UDIS    : 1;

    /// Update request source
    uint8_t URS     : 1;

    /// One-pulse mode
    uint8_t OPM     : 1;

    /// Direction
    uint8_t DIR     : 1;

    /// Center-aligned mode selection
    uint8_t CMS     : 2;

    /// Auto-reload preload enable
    uint8_t ARPE    : 1;
    
  } reg;
  
} TIM1_CR1_t;

/// register for configuring timer TIM1 (1/2)
reg(0x5250, TIM1_CR1_t, TIM1_CR1);



/// TIM1 Control register 2 (TIM1_CR2)
/** union for configuring timer TIM1 (2/2) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Capture/compare preloaded control
    uint8_t CCPC    : 1;

    /// Reserved, forced by hardware to 0
    uint8_t res     : 1;

    /// Capture/compare control update selection
    uint8_t COMS    : 1;

    /// Reserved, must be kept cleared
    uint8_t res2    : 1;

    /// Master mode selection
    uint8_t MMS     : 3;

    /// Reserved
    uint8_t res3    : 1;
    
  } reg;
  
} TIM1_CR2_t;

/// register for configuring timer TIM1 (2/2)
reg(0x5251, TIM1_CR2_t, TIM1_CR2);



/// Slave mode control register (TIM1_SMCR)
/** union for configuring master/slave mode of timer */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Clock/trigger/slave mode selection
    uint8_t SMS     : 3;

    /// Reserved
    uint8_t res     : 1;

    /// Trigger selection
    uint8_t TS      : 3;

    /// Master/slave mode
    uint8_t MSM     : 1;
    
  } reg;
  
} TIM1_SMCR_t;

/// register for configuring master/slave mode of timer
reg(0x5252, TIM1_SMCR_t, TIM1_SMCR);



/// TIM1 External trigger register (TIM1_ETR)
/** union for configuring external trigger for timer 1 */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// External trigger filter
    uint8_t ETF     : 4;

    /// External trigger prescaler
    uint8_t ETPS    : 2;

    /// External clock enable
    uint8_t ECE     : 1;

    /// External trigger polarity
    uint8_t ETP     : 1;
    
  } reg;
  
} TIM1_ETR_t;

/// register for configuring external trigger for timer 1
reg(0x5253, TIM1_ETR_t, TIM1_ETR);



/// TIM1 Interrupt enable register (TIM1_IER)
/** union for en-/disabling timer 1 interrupts */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Update interrupt enable
    uint8_t UIE     : 1;

    /// Capture/compare 1 interrupt enable
    uint8_t CC1IE   : 1;

    /// Capture/compare 2 interrupt enable
    uint8_t CC2IE   : 1;

    /// Capture/compare 3 interrupt enable
    uint8_t CC3IE   : 1;

    /// Capture/compare 4 interrupt enable
    uint8_t CC4IE   : 1;

    /// Commutation interrupt enable
    uint8_t COMIE   : 1;

    /// Trigger interrupt enable
    uint8_t TIE     : 1;

    /// Break interrupt enable
    uint8_t BIE     : 1;
    
  } reg;
  
} TIM1_IER_t;

/// register for en-/disabling timer 1 interrupts
reg(0x5254, TIM1_IER_t, TIM1_IER);



/// TIM1 Status register 1 (TIM1_SR1)
/** union for reading the state of timer 1 (1/2) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Update interrupt flag
    uint8_t UIF     : 1;

    /// Capture/compare 1 interrupt flag
    uint8_t CC1IF   : 1;

    /// Capture/compare 2 interrupt flag
    uint8_t CC2IF   : 1;

    /// Capture/compare 3 interrupt flag
    uint8_t CC3IF   : 1;

    /// Capture/compare 4 interrupt flag
    uint8_t CC4IF   : 1;

    /// Commutation interrupt flag
    uint8_t COMIF   : 1;

    /// Trigger interrupt flag
    uint8_t TIF     : 1;

    /// Break interrupt flag
    uint8_t BIF     : 1;
    
  } reg;
  
} TIM1_SR1_t;

/// register for reading the state of timer 1 (1/2)
reg(0x5255, TIM1_SR1_t, TIM1_SR1);



/// TIM1 Status register 2 (TIM1_SR2)
/** union for reading the state of timer 1 (2/2) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Reserved, must be kept cleared
    uint8_t res     : 1;

    /// Capture/compare 1 overcapture flag
    uint8_t CC1OF   : 1;

    /// Capture/compare 2 overcapture flag
    uint8_t CC2OF   : 1;

    /// Capture/compare 3 overcapture flag
    uint8_t CC3OF   : 1;

    /// Capture/compare 4 overcapture flag
    uint8_t CC4OF   : 1;

    /// Reserved, must be kept cleared
    uint8_t res2    : 3;
    
  } reg;
  
} TIM1_SR2_t;

/// register for reading the state of timer 1 (2/2)
reg(0x5256, TIM1_SR2_t, TIM1_SR2);



/// TIM1 Event generation register (TIM1_EGR)
/** union for generating timer 1 event by SW */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Update generation
    uint8_t UG      : 1;

    /// Capture/compare 1 generation
    uint8_t CC1G    : 1;

    /// Capture/compare 2 generation
    uint8_t CC2G    : 1;

    /// Capture/compare 3 generation
    uint8_t CC3G    : 1;

    /// Capture/compare 4 generation
    uint8_t CC4G    : 1;

    /// Capture/compare control update generation
    uint8_t COMG    : 1;

    /// Trigger generation
    uint8_t TG      : 1;

    /// Break generation
    uint8_t BG      : 1;
    
  } reg;
  
} TIM1_EGR_t;

/// register for generating timer 1 event by SW
reg(0x5257, TIM1_EGR_t, TIM1_EGR);



/// TIM1 Capture/compare mode register 1 (TIM1_CCMR1)
/** union for configuring capture compare for timer 1 channel CC1 */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Capture/compare 1 selection
    uint8_t CC1S    : 2;

    /// Output compare 1 fast enable
    uint8_t OC1FE   : 1;

    /// Output compare 1 preload enable
    uint8_t OC1PE   : 1;

    /// Output compare 1 mode
    uint8_t OC1M    : 3;

    /// Output compare 1 clear enable
    uint8_t OC1CE   : 1;
    
  } reg;
  
} TIM1_CCMR1_t;

/// register for configuring capture compare for timer 1 channel CC1
reg(0x5258, TIM1_CCMR1_t, TIM1_CCMR1);



/// TIM1 Capture/compare mode register 2 (TIM1_CCMR2)
/** union for configuring capture compare for timer 1 channel CC2 */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Capture/compare 2 selection
    uint8_t CC2S    : 2;

    /// Output compare 2 fast enable
    uint8_t OC2FE   : 1;

    /// Output compare 2 preload enable
    uint8_t OC2PE   : 1;

    /// Output compare 2 mode
    uint8_t OC2M    : 3;

    /// Output compare 2 clear enable
    uint8_t OC2CE   : 1;
    
  } reg;
  
} TIM1_CCMR2_t;

/// register for configuring capture compare for timer 1 channel CC2
reg(0x5259, TIM1_CCMR2_t, TIM1_CCMR2);



/// TIM1 Capture/compare mode register 3 (TIM1_CCMR3)
/** union for configuring capture compare for timer 1 channel CC3 */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Capture/compare 3 selection
    uint8_t CC3S    : 2;

    /// Output compare 3 fast enable
    uint8_t OC3FE   : 1;

    /// Output compare 3 preload enable
    uint8_t OC3PE   : 1;

    /// Output compare 3 mode
    uint8_t OC3M    : 3;

    /// Output compare 3 clear enable
    uint8_t OC3CE   : 1;
    
  } reg;
  
} TIM1_CCMR3_t;

/// register for configuring capture compare for timer 1 channel CC3
reg(0x525a, TIM1_CCMR3_t, TIM1_CCMR3);



/// TIM1 Capture/compare mode register 4 (TIM1_CCMR4)
/** union for configuring capture compare for timer 1 channel CC4 */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Capture/compare 4 selection
    uint8_t CC4S    : 2;

    /// Output compare 4 fast enable
    uint8_t OC4FE   : 1;

    /// Output compare 4 preload enable
    uint8_t OC4PE   : 1;

    /// Output compare 4 mode
    uint8_t OC4M    : 3;

    /// Output compare 4 clear enable
    uint8_t OC4CE   : 1;
    
  } reg;
  
} TIM1_CCMR4_t;

/// register for configuring capture compare for timer 1 channel CC4
reg(0x525b, TIM1_CCMR4_t, TIM1_CCMR4);



/// TIM1 Capture/compare enable register 1 (TIM1_CCER1)
/** union for enabling and configuring compare outputs and complementary outputs (1/2)  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Capture/compare 1 output enable
    uint8_t CC1E    : 1;

    /// Capture/compare 1 output polarity
    uint8_t CC1P    : 1;

    /// Capture/compare 1 complementary output enable
    uint8_t CC1NE   : 1;

    /// Capture/compare 1 complementary output polarity
    uint8_t CC1NP   : 1;

    /// Capture/compare 2 output enable
    uint8_t CC2E    : 1;

    /// Capture/compare 2 output polarity
    uint8_t CC2P    : 1;

    /// Capture/compare 2 complementary output enable
    uint8_t CC2NE   : 1;

    /// Capture/compare 2 complementary output polarity
    uint8_t CC2NP   : 1;
    
  } reg;
  
} TIM1_CCER1_t;

/// register for enabling and configuring compare outputs and complementary outputs (1/2)
reg(0x525c, TIM1_CCER1_t, TIM1_CCER1);



/// TIM1 Capture/Compare enable 2
/** union for enabling and configuring compare outputs and complementary outputs (2/2) */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Capture/compare 3 output enable
    uint8_t CC3E    : 1;

    /// Capture/compare 3 output polarity
    uint8_t CC3P    : 1;

    /// Capture/compare 3 complementary output enable
    uint8_t CC3NE   : 1;

    /// Capture/compare 3 complementary output polarity
    uint8_t CC3NP   : 1;

    /// Capture/compare 4 output enable
    uint8_t CC4E    : 1;

    /// Capture/compare 4 output polarity
    uint8_t CC4P    : 1;

    /// Reserved
    uint8_t res     : 2;
    
  } reg;
  
} TIM1_CCER2_t;

/// register for enabling and configuring compare outputs and complementary outputs (2/2)
reg(0x525d, TIM1_CCER2_t, TIM1_CCER2);



/// TIM1 Counter high (16 bit)
reg(0x525e, word_t, TIM1_CNTR);



/// TIM1 Prescaler (16 bit)
reg(0x5260, word_t, TIM1_PSCR);



/// TIM1 Auto-reload register (16 bit)
reg(0x5262, word_t, TIM1_ARR);



/// TIM1 Repetition counter
/** union for reducing update rate of timer 1 re-load register */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Repetition counter value
    uint8_t REP     : 8;
    
  } reg;
  
} TIM1_RCR_t;

/// register union for reducing update rate of timer 1 re-load register
reg(0x5264, TIM1_RCR_t, TIM1_RCR);



/// TIM1 Capture/Compare 1 register (16 bit)
reg(0x5265, word_t, TIM1_CCR1);



/// TIM1 Capture/Compare 2 register (16 bit)
reg(0x5267, word_t, TIM1_CCR2);



/// TIM1 Capture/Compare 3 register (16 bit)
reg(0x5269, word_t, TIM1_CCR3);



/// TIM1 Capture/Compare 4 register (16 bit)
reg(0x526b, word_t, TIM1_CCR4);



/// TIM1 Break register (TIM1_BKR)
/** union for configuring the timer 1 break input pin */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Lock configuration
    uint8_t LOCK    : 2;

    /// Off state selection for idle mode
    uint8_t OSSI    : 1;

    /// Off state selection for Run mode
    uint8_t OSSR    : 1;

    /// Break enable
    uint8_t BKE     : 1;

    /// Break polarity
    uint8_t BKP     : 1;

    /// Automatic output enable
    uint8_t AOE     : 1;

    /// Main output enable
    uint8_t MOE     : 1;
    
  } reg;
  
} TIM1_BKR_t;

/// register for configuring the timer 1 break input pin
reg(0x526d, TIM1_BKR_t, TIM1_BKR);



/// TIM1 Dead-time
/** union for configuring the compare deadtime between channels and complementary channels */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Deadtime generator set-up
    uint8_t DTG     : 8;
    
  } reg;
  
} TIM1_DTR_t;

/// register for configuring the compare deadtime between channels and complementary channels
reg(0x526e, TIM1_DTR_t, TIM1_DTR);



/// TIM1 Output idle state register (TIM1_OISR)
/** union for setting the timer 1 pin levels in idle mode */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Output idle state 1 (OC1 output)
    uint8_t OIS1    : 1;

    /// Output idle state 1 (OC1N output)
    uint8_t OIS1N   : 1;

    /// Output idle state 2 (OC2 output)
    uint8_t OIS2    : 1;

    /// Output idle state 2 (OC2N output)
    uint8_t OIS2N   : 1;

    /// Output idle state 3 (OC3 output)
    uint8_t OIS3    : 1;

    /// Output idle state 3 (OC3N output)
    uint8_t OIS3N   : 1;

    /// Output idle state 4 (OC4 output)
    uint8_t OIS4    : 1;

    /// Reserved, forced by hardware to 0
    uint8_t res     : 1;
    
  } reg;
  
} TIM1_OISR_t;

/// register for setting the timer 1 pin levels in idle mode
reg(0x526f, TIM1_OISR_t, TIM1_OISR);



//--------
// 16-Bit Timer TIM2
//--------

/// TIMx Control register 1 (TIMx_CR1)
/** union for configuring timer TIM2/TIM3/TIM5 */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Counter enable
    uint8_t CEN     : 1;

    /// Update disable
    uint8_t UDIS    : 1;

    /// Update request source
    uint8_t URS     : 1;

    /// One-pulse mode
    uint8_t OPM     : 1;

    /// Reserved
    uint8_t res     : 3;

    /// Auto-reload preload enable
    uint8_t ARPE    : 1;
    
  } reg;
  
} TIMx_CR1_t;

/// register for configuring timer TIM2
reg(0x5300, TIMx_CR1_t, TIM2_CR1);



/// TIM2 Interrupt enable
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t UIE     : 1;

    /// 
    uint8_t CC1IE   : 1;

    /// 
    uint8_t CC2IE   : 1;

    /// 
    uint8_t CC3IE   : 1;

    /// 
    uint8_t res     : 4;
    
  } reg;
  
} TIM2_IER_t;

/// register 
reg(0x5301, TIM2_IER_t, TIM2_IER);



/// TIM2 Status 1
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t UIF     : 1;

    /// 
    uint8_t CC1IF   : 1;

    /// 
    uint8_t CC2IF   : 1;

    /// 
    uint8_t CC3IF   : 1;

    /// 
    uint8_t res     : 4;
    
  } reg;
  
} TIM2_SR1_t;

/// register 
reg(0x5302, TIM2_SR1_t, TIM2_SR1);



/// TIM2 Status 2
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t res     : 1;

    /// 
    uint8_t CC1OF   : 1;

    /// 
    uint8_t CC2OF   : 1;

    /// 
    uint8_t CC3OF   : 1;

    /// 
    uint8_t res2    : 4;
    
  } reg;
  
} TIM2_SR2_t;

/// register 
reg(0x5303, TIM2_SR2_t, TIM2_SR2);



/// TIM2 Event Generation
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t UG      : 1;

    /// 
    uint8_t CC1G    : 1;

    /// 
    uint8_t CC2G    : 1;

    /// 
    uint8_t CC3G    : 1;

    /// 
    uint8_t res     : 4;
    
  } reg;
  
} TIM2_EGR_t;

/// register 
reg(0x5304, TIM2_EGR_t, TIM2_EGR);



/// TIM2 Capture/Compare mode 1
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t CC1S    : 2;

    /// 
    uint8_t res     : 1;

    /// 
    uint8_t OC1PE   : 1;

    /// 
    uint8_t OC1M    : 3;

    /// 
    uint8_t res2    : 1;
    
  } reg;
  
} TIMx_CCMR1_t;

/// register 
reg(0x5305, TIMx_CCMR1_t, TIM2_CCMR1);



/// TIM2 Capture/Compare mode 2
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t CC2S    : 2;

    /// 
    uint8_t res     : 1;

    /// 
    uint8_t OC2PE   : 1;

    /// 
    uint8_t OC2M    : 3;

    /// 
    uint8_t res2    : 1;
    
  } reg;
  
} TIMx_CCMR2_t;

/// register 
reg(0x5306, TIMx_CCMR2_t, TIM2_CCMR2);



/// TIM2 Capture/Compare mode 3
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t CC3S    : 2;

    /// 
    uint8_t res     : 1;

    /// 
    uint8_t OC3PE   : 1;

    /// 
    uint8_t OC3M    : 3;

    /// 
    uint8_t res2    : 1;
    
  } reg;
  
} TIM2_CCMR3_t;

/// register 
reg(0x5307, TIM2_CCMR3_t, TIM2_CCMR3);



/// TIM2 Capture/Compare enable 1
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t CC1E    : 1;

    /// 
    uint8_t CC1P    : 1;

    /// 
    uint8_t res     : 2;

    /// 
    uint8_t CC2E    : 1;

    /// 
    uint8_t CC2P    : 1;

    /// 
    uint8_t res2    : 2;
    
  } reg;
  
} TIMx_CCER1_t;

/// register 
reg(0x5308, TIMx_CCER1_t, TIM2_CCER1);



/// TIM2 Capture/Compare enable 2
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t CC3E    : 1;

    /// 
    uint8_t CC3P    : 1;

    /// 
    uint8_t res     : 6;
    
  } reg;
  
} TIM2_CCER2_t;

/// register 
reg(0x5309, TIM2_CCER2_t, TIM2_CCER2);



/// TIM2 Counter (16 bit)
reg(0x530a, word_t, TIM2_CNTR);



/// TIM2 Prescaler
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t PSC     : 4;

    /// 
    uint8_t res     : 4;
    
  } reg;
  
} TIMx_PSCR_t;

/// register 
reg(0x530c, TIMx_PSCR_t, TIM2_PSCR);



/// TIM2 Auto-reload (16 bit)
reg(0x530d, word_t, TIM2_ARR);



/// TIM2 Capture/Compare 1 (16 bit)
reg(0x530f, word_t, TIM2_CCR1);



/// TIM2 Capture/Compare 2 (16 bit)
reg(0x5311, word_t, TIM2_CCR2);



/// TIM2 Capture/Compare 3 (16 bit)
reg(0x5313, word_t, TIM2_CCR3);



//--------
// 16-Bit Timer TIM3
//--------

/// register for configuring timer TIM2
reg(0x5320, TIMx_CR1_t, TIM3_CR1);





/// TIM3 Interrupt enable
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t UIE     : 1;

    /// 
    uint8_t CC1IE   : 1;

    /// 
    uint8_t CC2IE   : 1;

    /// 
    uint8_t res     : 5;
    
  } reg;
  
} TIM3_IER_t;

/// register 
reg(0x5321, TIM3_IER_t, TIM3_IER);



/// TIM3 Status 1
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t UIF     : 1;

    /// 
    uint8_t CC1IF   : 1;

    /// 
    uint8_t CC2IF   : 1;

    /// 
    uint8_t res     : 5;
    
  } reg;
  
} TIM3_SR1_t;

/// register 
reg(0x5322, TIM3_SR1_t, TIM3_SR1);



/// TIM3 Status 2
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t res     : 1;

    /// 
    uint8_t CC1OF   : 1;

    /// 
    uint8_t CC2OF   : 1;

    /// 
    uint8_t res2    : 5;
    
  } reg;
  
} TIM3_SR2_t;

/// register 
reg(0x5323, TIM3_SR2_t, TIM3_SR2);



/// TIM3 Event Generation
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t UG      : 1;

    /// 
    uint8_t CC1G    : 1;

    /// 
    uint8_t CC2G    : 1;

    /// 
    uint8_t res     : 5;
    
  } reg;
  
} TIM3_EGR_t;

/// register 
reg(0x5324, TIM3_EGR_t, TIM3_EGR);



/// TIM3 Capture/Compare mode 1
reg(0x5325, TIMx_CCMR1_t, TIM3_CCMR1);



/// TIM3 Capture/Compare mode 2
reg(0x5326, TIMx_CCMR2_t, TIM3_CCMR2);



/// TIM3 Capture/Compare enable 1
reg(0x5327, TIMx_CCER1_t, TIM3_CCER1);



/// TIM3 Counter (16 bit)
reg(0x5328, word_t, TIM3_CNTR);



/// TIM3 Prescaler
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t PSC     : 4;

    /// 
    uint8_t res     : 4;
    
  } reg;
  
} TIM3_PSCR_t;

/// register 
reg(0x532a, TIM3_PSCR_t, TIM3_PSCR);



/// TIM3 Auto-reload (16 bit)
reg(0x532b, word_t, TIM3_ARR);



/// TIM3 Capture/Compare 1 (16 bit)
reg(0x532d, word_t, TIM3_CCR1);



/// TIM3 Capture/Compare 2 (16 bit)
reg(0x532f, word_t, TIM3_CCR2);



//--------
// 8-Bit Timer TIM4
//--------


/// TIM4 Control 1
reg(0x5340, TIMx_CR1_t, TIM4_CR1);


/// TIM4 Interrupt enable
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t UIE     : 1;

    /// 
    uint8_t res     : 5;

    /// 
    uint8_t TIE     : 1;

    /// 
    uint8_t res2    : 1;
    
  } reg;
  
} TIM4_IER_t;

/// register 
reg(0x5341, TIM4_IER_t, TIM4_IER);



/// TIM4 Status
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t UIF     : 1;

    /// 
    uint8_t res     : 5;

    /// 
    uint8_t TIF     : 1;

    /// 
    uint8_t res2    : 1;
    
  } reg;
  
} TIM4_SR1_t;

/// register 
reg(0x5342, TIM4_SR1_t, TIM4_SR1);



/// TIM4 Event Generation
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t UIF     : 1;

    /// 
    uint8_t res     : 5;

    /// 
    uint8_t TIF     : 1;

    /// 
    uint8_t res2    : 1;
    
  } reg;
  
} TIM4_EGR_t;

/// register 
reg(0x5343, TIM4_EGR_t, TIM4_EGR);



/// TIM4 Counter
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t CNT     : 8;
    
  } reg;
  
} TIM4_CNTR_t;

/// register 
reg(0x5344, TIM4_CNTR_t, TIM4_CNTR);



/// TIM4 Prescaler
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t PSC     : 3;

    /// 
    uint8_t res     : 5;
    
  } reg;
  
} TIM4_PSCR_t;

/// register 
reg(0x5345, TIM4_PSCR_t, TIM4_PSCR);



/// TIM4 Auto-reload
/** union  */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// 
    uint8_t ARR     : 8;
    
  } reg;
  
} TIM4_ARR_t;

/// register 
reg(0x5346, TIM4_ARR_t, TIM4_ARR);


//xxx

//--------
// 16-Bit Timer TIM5
//--------

/// TIM5 Control register 2 (TIM5_CR2)
/** union for configuring timer 5 master mode */
typedef union {
  
  /// bytewise access to register
  uint8_t  byte;
  
  /// bitwise access to register
  struct {

    /// Reserved, must be kept cleared
    uint8_t res      : 4;

    /// Master mode selection
    uint8_t MM       : 3;

    /// Reserved, must be kept cleared
    uint8_t res2     : 1;
    
  } reg;
  
} TIM5_CR2_t;

/// register for configuring timer 5 master mode (address not found)
//reg(xxxx, TIM5_CR2_t, TIM5_CR2);




/*-----------------------------------------------------------------------------
    END OF MODULE DEFINITION FOR MULTIPLE INLUSION
-----------------------------------------------------------------------------*/
#endif // _STM8AS_H
