/*******************************************************************************
 *
 * crc.h - Header file for STM8 CRC library functions
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

#ifndef CRC_H_
#define CRC_H_

#include <stdint.h>

// Initial values for the various CRC implementations.
//#define CRC8_1WIRE_INIT ((uint8_t)0x0)
//#define CRC8_J1850_INIT ((uint8_t)0xFF)
//#define CRC16_ANSI_INIT ((uint16_t)0xFFFF)
//#define CRC16_CCITT_INIT ((uint16_t)0xFFFF)
//#define CRC16_XMODEM_INIT ((uint16_t)0x0)
#define CRC32_INIT ((uint32_t)0xFFFFFFFF)
//#define CRC32_POSIX_INIT ((uint32_t)0x0)

// Function-like macros to return the initial value.
//#define crc8_1wire_init() CRC8_1WIRE_INIT
//#define crc8_j1850_init() CRC8_J1850_INIT
//#define crc16_ansi_init() CRC16_ANSI_INIT
//#define crc16_ccitt_init() CRC16_CCITT_INIT
//#define crc16_xmodem_init() CRC16_XMODEM_INIT
#define crc32_init() CRC32_INIT
//#define crc32_posix_init() CRC32_POSIX_INIT

// Values used to finalise the CRC, being XOR-ed with the CRC.
//#define CRC8_1WIRE_XOROUT ((uint8_t)0x0)
//#define CRC8_J1850_XOROUT ((uint8_t)0xFF)
//#define CRC16_ANSI_XOROUT ((uint16_t)0x0)
//#define CRC16_CCITT_XOROUT ((uint16_t)0x0)
//#define CRC16_XMODEM_XOROUT ((uint16_t)0x0)
#define CRC32_XOROUT ((uint32_t)0xFFFFFFFF)
//#define CRC32_POSIX_XOROUT ((uint32_t)0xFFFFFFFF)

// Function-like macros to do the final XOR of the CRC value.
// Macros are used because in the majority of cases, the XOR-ing value is zero,
// so the compiler will have the opportunity to optimise the operation away
// (because it has no effect).
//#define crc8_1wire_final(c) ((c) ^ CRC8_1WIRE_XOROUT)
//#define crc8_j1850_final(c) ((c) ^ CRC8_J1850_XOROUT)
//#define crc16_ansi_final(c) ((c) ^ CRC16_ANSI_XOROUT)
//#define crc16_ccitt_final(c) ((c) ^ CRC16_CCITT_XOROUT)
//#define crc16_xmodem_final(c) ((c) ^ CRC16_XMODEM_XOROUT)
#define crc32_final(c) ((c) ^ CRC32_XOROUT)
//#define crc32_posix_final(c) ((c) ^ CRC32_POSIX_XOROUT)

// These have the same implementations, just with different initial values, so
// just alias them to the latter functions.
//#define crc16_xmodem_update crc16_ccitt_update

//extern uint8_t crc8_1wire_update(uint8_t crc, uint8_t data);
//extern uint8_t crc8_j1850_update(uint8_t crc, uint8_t data);
//extern uint16_t crc16_ansi_update(uint16_t crc, uint8_t data);
//extern uint16_t crc16_ccitt_update(uint16_t crc, uint8_t data);
extern uint32_t crc32_update(uint32_t crc, uint8_t data);
//extern uint32_t crc32_posix_update(uint32_t crc, uint8_t data);

#endif // CRC_H_
