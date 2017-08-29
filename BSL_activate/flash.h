/*-----------------------------------------------------------------------------
    MODULE DEFINITION FOR MULTIPLE INCLUSION
-----------------------------------------------------------------------------*/
#ifndef _FLASH_H_
#define _FLASH_H_

/// base address for option bytes
#define BASE_ADDR_OPT         0x4800

//////
// addresses of option bytes
//////
#define OPT0   (BASE_ADDR_OPT+0x00)  //!< Option byte 0: Read-out protection (not accessible in IAP mode)
#define OPT1   (BASE_ADDR_OPT+0x01)  //!< Option byte 1: User boot code */
#define NOPT1  (BASE_ADDR_OPT+0x02)  //!< Complementary Option byte 1 */
#define OPT2   (BASE_ADDR_OPT+0x03)  //!< Option byte 2: Alternate function remapping */
#define NOPT2  (BASE_ADDR_OPT+0x04)  //!< Complementary Option byte 2 */
#define OPT3   (BASE_ADDR_OPT+0x05)  //!< Option byte 3: Watchdog option */
#define NOPT3  (BASE_ADDR_OPT+0x06)  //!< Complementary Option byte 3 */
#define OPT4   (BASE_ADDR_OPT+0x07)  //!< Option byte 4: Clock option */
#define NOPT4  (BASE_ADDR_OPT+0x08)  //!< Complementary Option byte 4 */
#define OPT5   (BASE_ADDR_OPT+0x09)  //!< Option byte 5: HSE clock startup */
#define NOPT5  (BASE_ADDR_OPT+0x0A)  //!< Complementary Option byte 5 */
#define RES1   (BASE_ADDR_OPT+0x0B)  //!< Reserved Option byte*/
#define RES2   (BASE_ADDR_OPT+0x0C)  //!< Reserved Option byte*/
#define OPT7   (BASE_ADDR_OPT+0x0D)  //!< Option byte 7: flash wait states */
#define NOPT7  (BASE_ADDR_OPT+0x0E)  //!< Complementary Option byte 7 */

#define OPT8   (BASE_ADDR_OPT+0x10)  //!< Option byte 8:  TMU key 1 */
#define OPT9   (BASE_ADDR_OPT+0x11)  //!< Option byte 9:  TMU key 2 */
#define OPT10  (BASE_ADDR_OPT+0x12)  //!< Option byte 10: TMU key 3 */
#define OPT11  (BASE_ADDR_OPT+0x13)  //!< Option byte 11: TMU key 4 */
#define OPT12  (BASE_ADDR_OPT+0x14)  //!< Option byte 12: TMU key 5 */
#define OPT13  (BASE_ADDR_OPT+0x15)  //!< Option byte 13: TMU key 6 */
#define OPT14  (BASE_ADDR_OPT+0x16)  //!< Option byte 14: TMU key 7 */
#define OPT15  (BASE_ADDR_OPT+0x17)  //!< Option byte 15: TMU key 8 */
#define OPT16  (BASE_ADDR_OPT+0x18)  //!< Option byte 16: TMU access failure counter */

#define OPT17  (BASE_ADDR_OPT+0x7E)  //!< Option byte 17: BSL activation */
#define NOPT17 (BASE_ADDR_OPT+0x7F)  //!< Complementary Option byte 17 */


/// write option byte
void    flash_write_option_byte(uint16_t addr, uint8_t ch);

/// write single byte to P-flash or EEPROM
void    flash_write_byte(uint32_t addr, uint8_t ch);

/// read single byte from memory
uint8_t read_byte(uint32_t addr);


/*-----------------------------------------------------------------------------
    END OF MODULE DEFINITION FOR MULTIPLE INLUSION
-----------------------------------------------------------------------------*/
#endif // _FLASH_H_
