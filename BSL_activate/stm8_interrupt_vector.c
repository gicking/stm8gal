// avoid sdcc compiler warnings for empty file...
#include <stdlib.h>

// only for COSMIC compiler, else skip
#if defined(__CSMC__)

// declaration of below ISR handler
typedef void @far (*interrupt_handler_t)(void);

struct interrupt_vector {
	unsigned char interrupt_instruction;
	interrupt_handler_t interrupt_handler;
};

@far @interrupt void NonHandledInterrupt (void)
{
	return;
}

extern void _stext();           /* startup routine */
extern void TRAP_IRQHandler();  /* trap routine */

struct interrupt_vector const _vectab[] = {
	{0x82, (interrupt_handler_t) _stext},           /* reset */
	{0x82, NonHandledInterrupt},                    /* trap - Software interrupt  */
	{0x82, NonHandledInterrupt},                    /* irq0 - External Top Level interrupt (TLI)  */
	{0x82, NonHandledInterrupt},                    /* irq1 - Auto Wake Up from Halt interrupt  */
	{0x82, NonHandledInterrupt},                    /* irq2 - Clock Controller interrupt  */
  {0x82, NonHandledInterrupt},                    /* irq3 - External interrupt 0 (GPIOA) */
  {0x82, NonHandledInterrupt},                    /* irq4 - External interrupt 1 (GPIOB) */
  {0x82, NonHandledInterrupt},                    /* irq5 - External interrupt 2 (GPIOC) */
  {0x82, NonHandledInterrupt},                    /* irq6 - External interrupt 3 (GPIOD) */
  {0x82, NonHandledInterrupt},                    /* irq7 - External interrupt 4 (GPIOE) */
  {0x82, NonHandledInterrupt},                    /* irq8 - External interrupt 5 (GPIOF) */
	{0x82, NonHandledInterrupt},                    /* irq9 - Reserved */
	{0x82, NonHandledInterrupt},                    /* irq10 - SPI End of transfer interrupt */
	{0x82, NonHandledInterrupt},                    /* irq11 - TIM1 Update/Overflow/Trigger/Break interrupt */
	{0x82, NonHandledInterrupt},                    /* irq12 - TIM1 Capture/Compare interrupt */
	{0x82, NonHandledInterrupt},                    /* irq13 - TIM2 Update/Overflow/Break interrupt */
	{0x82, NonHandledInterrupt},                    /* irq14 - TIM2 Capture/Compare interrupt */
	{0x82, NonHandledInterrupt},                    /* irq15 - Reserved */
	{0x82, NonHandledInterrupt},                    /* irq16 - Reserved */
	{0x82, NonHandledInterrupt},                    /* irq17 - UART1(=LINUART) Tx complete interrupt */
  {0x82, NonHandledInterrupt},                    /* irq18 - UART1(=LINUART) Rx interrupt */
	{0x82, NonHandledInterrupt},                    /* irq19 - I2C interrupt */
 	{0x82, NonHandledInterrupt},                    /* irq20 - UART2 Tx interrupt */
	{0x82, NonHandledInterrupt},                    /* irq21 - UART2 Rx interrupt */
  {0x82, NonHandledInterrupt},                    /* irq22 - ADC1 end of conversion/Analog watchdog interrupts */
	{0x82, NonHandledInterrupt},                    /* irq23 - Timer 4 interrupt */
	{0x82, NonHandledInterrupt},                    /* irq24 - FLASH interrupt */
	{0x82, NonHandledInterrupt},                    /* irq25 - Reserved */
	{0x82, NonHandledInterrupt},                    /* irq26 - Reserved */
	{0x82, NonHandledInterrupt},                    /* irq27 - Reserved */
	{0x82, NonHandledInterrupt},                    /* irq28 - Reserved */
	{0x82, NonHandledInterrupt},                    /* irq29 - Reserved */
};

// end only for Cosmic
#endif // __CSMC__
