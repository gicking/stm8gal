#pragma codeseg VERIFY_SEG
#pragma callee_saves erase

#include "verify_CRC32.h"


void verify_CRC32(void)
{
    uint8_t tmp=0;

    // parameter error
    if ((addr_stop < addr_start) || (addr_stop > 0x27FFF))
    {
        // set return status
        crc32  = 0x00000000;
        status = ERROR;
    }

    // calculate checksum over memory content
    else
    {
        // switch to 16MHz, store old setting
        uint8_t oldCLK = CLK_CKDIVR;
        CLK_CKDIVR = 0x00;
       
 	// initialize CRC32 checksum
        crc32 = crc32_init();

        // calculate CRC32 checksum
        while (addr_start<=addr_stop)
        {            
            // read from memory. Use inline assembler due to lack of far pointers in SDCC
            // store data in variable "status"
            __asm
                push a 
                ldf  a,[_addr_start+1].e
                ld  _status, a
                pop a
            __endasm;
            
            // update CRC32 checksum
            crc32 = crc32_update(crc32, status);

            // increment address
            addr_start++;
            
            // service watchdogs
            SERVICE_IWDG;
            SERVICE_WWDG;

        } // loop over memory
        
        // finalize CRC32 checksum
        crc32 = crc32_final(crc32);

        // set return status
        status = SUCCESS;

        // restore old clock setting;
        CLK_CKDIVR = oldCLK;

    } // if calculate checksum

    // return to bootloader (see UM0560, appendix B)
    TIM2_EGR = 0x01;            // refresh TIM2 prescaler shadow registers to 1
    TIM2_SR1;
    TIM2_SR1 = 0x00;
    TIM3_EGR = 0x01;            // refresh TIM3 prescaler shadow registers to 1
    TIM3_SR1;
    TIM3_SR1 = 0x00;
    BL_timeout = 0x00;          // no ROM-BL timeout
    //__asm__("jp 0x6000");     // jump back to start of ROM-BL
    //__asm__("jp 0x601E");     // jump back to ROM-BL after checking BL activation
    __asm__("jp 0x602E");       // jump back to ROM-BL after checking ROP

} // verify()


// Copied from https://github.com/basilhussain/stm8-crc
// CRC32 (aka GZIP, PKZIP, PNG, ZMODEM)
// Polynomial: x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1 (0xEDB88320, reversed)
// Initial value: 0xFFFFFFFF
// XOR out: 0xFFFFFFFF
uint32_t crc32_update(uint32_t crc, uint8_t data) __naked {
    
    // Avoid compiler warnings for unreferenced args.
    (void)crc;
    (void)data;

    // For return value/arg: 0xAABBCCDD
    // x = 0xCCDD (xh = 0xCC, xl = 0xDD)
    // y = 0xAABB (yh = 0xAA, yl = 0xBB)

    __asm
        ; XOR the LSB of the CRC with data byte, and put it back in the CRC.
        ld a, (ASM_ARGS_SP_OFFSET+4, sp)
        xor a, (ASM_ARGS_SP_OFFSET+3, sp)
        ld (ASM_ARGS_SP_OFFSET+3, sp), a

        ; Load CRC variable from stack into X & Y regs for further work.
        ldw x, (ASM_ARGS_SP_OFFSET+2, sp)
        ldw y, (ASM_ARGS_SP_OFFSET+0, sp)

    .macro crc32_update_shift_xor skip_lbl
            ; Shift CRC value right by one bit.
            srlw y
            rrcw x

            ; Jump if least-significant bit of CRC is now zero.
            jrnc skip_lbl

            ; XOR the CRC value with the polynomial value.
            rrwa x
            xor a, #0x20
            rrwa x
            xor a, #0x83
            rrwa x
            rrwa y
            xor a, #0xB8
            rrwa y
            xor a, #0xED
            rrwa y

        skip_lbl:
    .endm

#ifdef ASM_UNROLL_LOOP

        crc32_update_shift_xor 0001$
        crc32_update_shift_xor 0002$
        crc32_update_shift_xor 0003$
        crc32_update_shift_xor 0004$
        crc32_update_shift_xor 0005$
        crc32_update_shift_xor 0006$
        crc32_update_shift_xor 0007$
        crc32_update_shift_xor 0008$

#else

        ; Initialise counter to loop 8 times, once for each bit of data byte.
        ld a, #8

    0001$:

        crc32_update_shift_xor 0002$

        ; Decrement counter and loop around if it is not zero.
        dec a
        jrne 0001$

#endif

        ; The X and Y registers now contain updated CRC value, so leave them
        ; there as function return value.
        ASM_RETURN
    __endasm;
}
