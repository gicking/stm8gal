;--------------------------------------------------------
; File Created by SDCC : free open source ANSI-C Compiler
; Version 4.0.0 #11528 (Linux)
;--------------------------------------------------------
	.module verify_CRC32
	.optsdcc -mstm8
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _verify_CRC32
	.globl _crc32_update
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area DATA
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area INITIALIZED
;--------------------------------------------------------
; absolute external ram data
;--------------------------------------------------------
	.area DABS (ABS)

; default segment ordering for linker
	.area HOME
	.area GSINIT
	.area GSFINAL
	.area CONST
	.area INITIALIZER
	.area CODE

;--------------------------------------------------------
; global & static initialisations
;--------------------------------------------------------
	.area HOME
	.area GSINIT
	.area GSFINAL
	.area GSINIT
;--------------------------------------------------------
; Home
;--------------------------------------------------------
	.area HOME
	.area HOME
;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area VERIFY_SEG
;	verify_CRC32.c: 7: void verify_CRC32(void)
;	-----------------------------------------
;	 function verify_CRC32
;	-----------------------------------------
_verify_CRC32:
	push	a
;	verify_CRC32.c: 12: if ((addr_stop < addr_start) || (addr_stop > 0x27FFF))
	ldw	x, _addr_stop+2
	cpw	x, _addr_start+2
	ld	a, _addr_stop+1
	sbc	a, _addr_start+1
	ld	a, _addr_stop+0
	sbc	a, _addr_start+0
	jrc	00104$
	ldw	x, #0x7fff
	cpw	x, _addr_stop+2
	ld	a, #0x02
	sbc	a, _addr_stop+1
	clr	a
	sbc	a, _addr_stop+0
	jrnc	00105$
00104$:
;	verify_CRC32.c: 15: crc32  = 0x00000000;
	clrw	x
	ldw	_crc32+2, x
	ldw	_crc32+0, x
;	verify_CRC32.c: 16: status = ERROR;
	mov	_status+0, #0x01
	jp	00106$
00105$:
;	verify_CRC32.c: 23: uint8_t oldCLK = CLK_CKDIVR;
	ld	a, 0x50c6
	ld	(0x01, sp), a
;	verify_CRC32.c: 24: CLK_CKDIVR = 0x00;
	mov	0x50c6+0, #0x00
;	verify_CRC32.c: 27: crc32 = crc32_init();
	ldw	x, #0xffff
	ldw	_crc32+2, x
	ldw	x, #0xffff
	ldw	_crc32+0, x
;	verify_CRC32.c: 30: while (addr_start<=addr_stop)
00101$:
	ldw	x, _addr_stop+2
	cpw	x, _addr_start+2
	ld	a, _addr_stop+1
	sbc	a, _addr_start+1
	ld	a, _addr_stop+0
	sbc	a, _addr_start+0
	jrc	00103$
;	verify_CRC32.c: 39: __endasm;
	push	a
	ldf	a,[_addr_start+1].e
	ld	_status, a
	pop	a
;	verify_CRC32.c: 42: crc32 = crc32_update(crc32, status);
	push	_status+0
	ldw	x, _crc32+2
	pushw	x
	ldw	x, _crc32+0
	pushw	x
	call	_crc32_update
	addw	sp, #5
	ldw	_crc32+2, x
	ldw	_crc32+0, y
;	verify_CRC32.c: 45: addr_start++;
	ldw	x, _addr_start+2
	addw	x, #0x0001
	ldw	y, _addr_start+0
	jrnc	00128$
	incw	y
00128$:
	ldw	_addr_start+2, x
	ldw	_addr_start+0, y
;	verify_CRC32.c: 48: SERVICE_IWDG;
	mov	0x50e0+0, #0xaa
;	verify_CRC32.c: 49: SERVICE_WWDG;
	mov	0x50d1+0, #0x7f
	jra	00101$
00103$:
;	verify_CRC32.c: 54: crc32 = crc32_final(crc32);
	ld	a, _crc32+3
	cpl	a
	ld	xl, a
	ld	a, _crc32+2
	cpl	a
	ld	xh, a
	ld	a, _crc32+1
	cpl	a
	ld	yl, a
	ld	a, _crc32+0
	cpl	a
	ld	yh, a
	ldw	_crc32+2, x
	ldw	_crc32+0, y
;	verify_CRC32.c: 57: status = SUCCESS;
	clr	_status+0
;	verify_CRC32.c: 60: CLK_CKDIVR = oldCLK;
	ldw	x, #0x50c6
	ld	a, (0x01, sp)
	ld	(x), a
00106$:
;	verify_CRC32.c: 65: TIM2_EGR = 0x01;            // refresh TIM2 prescaler shadow registers to 1
	mov	0x5304+0, #0x01
;	verify_CRC32.c: 66: TIM2_SR1;
	ld	a, 0x5302
;	verify_CRC32.c: 67: TIM2_SR1 = 0x00;
	mov	0x5302+0, #0x00
;	verify_CRC32.c: 68: TIM3_EGR = 0x01;            // refresh TIM3 prescaler shadow registers to 1
	mov	0x5324+0, #0x01
;	verify_CRC32.c: 69: TIM3_SR1;
	ld	a, 0x5322
;	verify_CRC32.c: 70: TIM3_SR1 = 0x00;
	mov	0x5322+0, #0x00
;	verify_CRC32.c: 71: BL_timeout = 0x00;          // no ROM-BL timeout
	clr	_BL_timeout+0
;	verify_CRC32.c: 74: __asm__("jp 0x602E");       // jump back to ROM-BL after checking ROP
	jp	0x602E
;	verify_CRC32.c: 76: } // verify()
	pop	a
	ret
;	verify_CRC32.c: 84: uint32_t crc32_update(uint32_t crc, uint8_t data) __naked {
;	-----------------------------------------
;	 function crc32_update
;	-----------------------------------------
_crc32_update:
;	naked function: no prologue.
;	verify_CRC32.c: 156: __endasm;
;	XOR the LSB of the CRC with data byte, and put it back in the CRC.
	ld	a, (3 +4, sp)
	xor	a, (3 +3, sp)
	ld	(3 +3, sp), a
;	Load CRC variable from stack into X & Y regs for further work.
	ldw	x, (3 +2, sp)
	ldw	y, (3 +0, sp)
	.macro	crc32_update_shift_xor skip_lbl
;	Shift CRC value right by one bit.
	srlw	y
	rrcw	x
;	Jump if least-significant bit of CRC is now zero.
	jrnc	skip_lbl
;	XOR the CRC value with the polynomial value.
	rrwa	x
	xor	a, #0x20
	rrwa	x
	xor	a, #0x83
	rrwa	x
	rrwa	y
	xor	a, #0xB8
	rrwa	y
	xor	a, #0xED
	rrwa	y
	        skip_lbl:
	.endm
;	Initialise counter to loop 8 times, once for each bit of data byte.
	ld	a, #8
	    0001$:
	crc32_update_shift_xor	0002$
;	Decrement counter and loop around if it is not zero.
	dec	a
	jrne	0001$
;	The X and Y registers now contain updated CRC value, so leave them
;	there as function return value.
	ret
;	verify_CRC32.c: 157: }
	.area CODE
	.area CONST
	.area INITIALIZER
	.area CABS (ABS)
