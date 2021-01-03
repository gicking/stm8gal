/*******************************************************************************
 *
 * crc32.c - CRC32 implementation
 *
 * Copyright (c) 2020 Basil Hussain
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ******************************************************************************/

 // SDCC pragmas
 #pragma codeseg VERIFY_SEG

#include <stddef.h>
#include <stdint.h>
#include "./crc.h"

#ifdef __SDCC_MODEL_LARGE
#define ASM_ARGS_SP_OFFSET 4
#define ASM_RETURN retf
#else
#define ASM_ARGS_SP_OFFSET 3
#define ASM_RETURN ret
#endif

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
