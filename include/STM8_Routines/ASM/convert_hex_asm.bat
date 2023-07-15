echo off
cls

REM .\srec_cat.exe %1.s19 -o %1.hex -intel
REM ..\naken_util.exe -disasm -stm8 %1.hex > %1.asm

set FILE="E_W_ROUTINEs_128K_ver_2.0"
.\srec_cat.exe %FILE%.s19 -o %FILE%.hex -intel
..\naken_util.exe -disasm -stm8 %FILE%.hex > %FILE%.asm

set FILE="E_W_ROUTINEs_128K_ver_2.1"
.\srec_cat.exe %FILE%.s19 -o %FILE%.hex -intel
..\naken_util.exe -disasm -stm8 %FILE%.hex > %FILE%.asm

set FILE="E_W_ROUTINEs_128K_ver_2.2"
.\srec_cat.exe %FILE%.s19 -o %FILE%.hex -intel
..\naken_util.exe -disasm -stm8 %FILE%.hex > %FILE%.asm

set FILE="E_W_ROUTINEs_128K_ver_2.4"
.\srec_cat.exe %FILE%.s19 -o %FILE%.hex -intel
..\naken_util.exe -disasm -stm8 %FILE%.hex > %FILE%.asm

set FILE="E_W_ROUTINEs_256K_ver_1.0"
.\srec_cat.exe %FILE%.s19 -o %FILE%.hex -intel
..\naken_util.exe -disasm -stm8 %FILE%.hex > %FILE%.asm

set FILE="E_W_ROUTINEs_32K_verL_1.0"
.\srec_cat.exe %FILE%.s19 -o %FILE%.hex -intel
..\naken_util.exe -disasm -stm8 %FILE%.hex > %FILE%.asm

set FILE="E_W_ROUTINEs_32K_ver_1.0"
.\srec_cat.exe %FILE%.s19 -o %FILE%.hex -intel
..\naken_util.exe -disasm -stm8 %FILE%.hex > %FILE%.asm

set FILE="E_W_ROUTINEs_32K_ver_1.2"
.\srec_cat.exe %FILE%.s19 -o %FILE%.hex -intel
..\naken_util.exe -disasm -stm8 %FILE%.hex > %FILE%.asm

set FILE="E_W_ROUTINEs_32K_ver_1.3"
.\srec_cat.exe %FILE%.s19 -o %FILE%.hex -intel
..\naken_util.exe -disasm -stm8 %FILE%.hex > %FILE%.asm

set FILE="E_W_ROUTINEs_32K_ver_1.4"
.\srec_cat.exe %FILE%.s19 -o %FILE%.hex -intel
..\naken_util.exe -disasm -stm8 %FILE%.hex > %FILE%.asm

set FILE="E_W_ROUTINEs_8K_verL_1.0"
.\srec_cat.exe %FILE%.s19 -o %FILE%.hex -intel
..\naken_util.exe -disasm -stm8 %FILE%.hex > %FILE%.asm
 