/*******************************************************************************
 *
 * Replacement open-source erase/write RAM routines for STM8 ROM bootloader
 * https://github.com/basilhussain/stm8-bootloader-erase-write
 *
 * Copyright 2021 Basil Hussain
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

unsigned char bin_erase_write_verL_8k_1_0_ihx[] = {
  0x3a, 0x30, 0x31, 0x30, 0x31, 0x45, 0x41, 0x30, 0x30, 0x42, 0x32, 0x36,
  0x32, 0x0a, 0x3a, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x31, 0x46,
  0x46, 0x0a
};
unsigned int bin_erase_write_verL_8k_1_0_ihx_len = 26;
