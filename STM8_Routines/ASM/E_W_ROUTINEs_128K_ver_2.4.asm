
naken_util - by Michael Kohn
                Joe Davisson
    Web: http://www.mikekohn.net/
  Email: mike@mikekohn.net

Version: October 30, 2017

Loaded hexfile E_W_ROUTINEs_128K_ver_2.4.hex from 0x00a0 to 0x01ff
Type help for a list of commands.

Addr    Opcode Instruction                              Cycles
------- ------ ----------------------------------       ------
0x00a0:  5f             clrw X                                   cycles=1
0x00a1:  3f 90          clr $90                                  cycles=1
0x00a3:  3f 9b          clr $9b                                  cycles=1
0x00a5:  72 09 00 8e 16 btjf $8e, #4, $c0  (offset=22)           cycles=2-3
0x00aa:  cd 60 8a       call $608a                               cycles=4
0x00ad:  b6 90          ld A, $90                                cycles=1
0x00af:  e7 00          ld ($00,X),A                             cycles=1
0x00b1:  5c             incw X                                   cycles=1
0x00b2:  4c             inc A                                    cycles=1
0x00b3:  b7 90          ld $90,A                                 cycles=1
0x00b5:  a1 82          cp A, #$82                               cycles=1
0x00b7:  26 f1          jrne $aa  (offset=-15)                   cycles=1-2
0x00b9:  a6 81          ld A, #$81                               cycles=1
0x00bb:  b7 88          ld $88,A                                 cycles=1
0x00bd:  5f             clrw X                                   cycles=1
0x00be:  3f 90          clr $90                                  cycles=1
0x00c0:  e6 00          ld A, ($00,X)                            cycles=1
0x00c2:  a1 80          cp A, #$80                               cycles=1
0x00c4:  26 07          jrne $cd  (offset=7)                     cycles=1-2
0x00c6:  3f 8a          clr $8a                                  cycles=1
0x00c8:  ae 40 00       ldw X, #$4000                            cycles=2
0x00cb:  20 3d          jra $10a  (offset=61)                    cycles=2
0x00cd:  a1 81          cp A, #$81                               cycles=1
0x00cf:  26 07          jrne $d8  (offset=7)                     cycles=1-2
0x00d1:  3f 8a          clr $8a                                  cycles=1
0x00d3:  ae 44 00       ldw X, #$4400                            cycles=2
0x00d6:  20 32          jra $10a  (offset=50)                    cycles=2
0x00d8:  a1 20          cp A, #$20                               cycles=1
0x00da:  24 0e          jrnc $ea  (offset=14)                    cycles=1-2
0x00dc:  3f 8a          clr $8a                                  cycles=1
0x00de:  ae 00 80       ldw X, #$80                              cycles=2
0x00e1:  42             mul X, A                                 cycles=4
0x00e2:  58             sllw X                                   cycles=2
0x00e3:  58             sllw X                                   cycles=2
0x00e4:  58             sllw X                                   cycles=2
0x00e5:  1c 80 00       addw X, #$8000                           cycles=2
0x00e8:  20 20          jra $10a  (offset=32)                    cycles=2
0x00ea:  a1 60          cp A, #$60                               cycles=1
0x00ec:  24 0f          jrnc $fd  (offset=15)                    cycles=1-2
0x00ee:  a0 20          sub A, #$20                              cycles=1
0x00f0:  ae 00 80       ldw X, #$80                              cycles=2
0x00f3:  42             mul X, A                                 cycles=4
0x00f4:  58             sllw X                                   cycles=2
0x00f5:  58             sllw X                                   cycles=2
0x00f6:  58             sllw X                                   cycles=2
0x00f7:  a6 01          ld A, #$01                               cycles=1
0x00f9:  b7 8a          ld $8a,A                                 cycles=1
0x00fb:  20 0d          jra $10a  (offset=13)                    cycles=2
0x00fd:  a0 60          sub A, #$60                              cycles=1
0x00ff:  ae 00 80       ldw X, #$80                              cycles=2
0x0102:  42             mul X, A                                 cycles=4
0x0103:  58             sllw X                                   cycles=2
0x0104:  58             sllw X                                   cycles=2
0x0105:  58             sllw X                                   cycles=2
0x0106:  a6 02          ld A, #$02                               cycles=1
0x0108:  b7 8a          ld $8a,A                                 cycles=1
0x010a:  90 5f          clrw Y                                   cycles=1
0x010c:  cd 60 8a       call $608a                               cycles=4
0x010f:  9e             ld A, XH                                 cycles=1
0x0110:  b7 8b          ld $8b,A                                 cycles=1
0x0112:  9f             ld A, XL                                 cycles=1
0x0113:  b7 8c          ld $8c,A                                 cycles=1
0x0115:  a6 20          ld A, #$20                               cycles=1
0x0117:  c7 50 5b       ld $505b,A                               cycles=1
0x011a:  43             cpl A                                    cycles=1
0x011b:  c7 50 5c       ld $505c,A                               cycles=1
0x011e:  4f             clr A                                    cycles=1
0x011f:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x0123:  5c             incw X                                   cycles=1
0x0124:  9f             ld A, XL                                 cycles=1
0x0125:  b7 8c          ld $8c,A                                 cycles=1
0x0127:  4f             clr A                                    cycles=1
0x0128:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x012c:  5c             incw X                                   cycles=1
0x012d:  9f             ld A, XL                                 cycles=1
0x012e:  b7 8c          ld $8c,A                                 cycles=1
0x0130:  4f             clr A                                    cycles=1
0x0131:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x0135:  5c             incw X                                   cycles=1
0x0136:  9f             ld A, XL                                 cycles=1
0x0137:  b7 8c          ld $8c,A                                 cycles=1
0x0139:  4f             clr A                                    cycles=1
0x013a:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x013e:  72 00 50 5f 07 btjt $505f, #0, $14a  (offset=7)         cycles=2-3
0x0143:  72 05 50 5f fb btjf $505f, #2, $143  (offset=-5)        cycles=2-3
0x0148:  20 04          jra $14e  (offset=4)                     cycles=2
0x014a:  72 10 00 9b    bset $9b, #0                             cycles=1
0x014e:  90 a3 00 07    cpw Y, #$7                               cycles=2
0x0152:  27 0a          jreq $15e  (offset=10)                   cycles=1-2
0x0154:  90 5c          incw Y                                   cycles=1
0x0156:  1d 00 03       subw X, #$3                              cycles=2
0x0159:  1c 00 80       addw X, #$80                             cycles=2
0x015c:  20 ae          jra $10c  (offset=-82)                   cycles=2
0x015e:  b6 90          ld A, $90                                cycles=1
0x0160:  b1 88          cp A, $88                                cycles=1
0x0162:  27 1b          jreq $17f  (offset=27)                   cycles=1-2
0x0164:  5f             clrw X                                   cycles=1
0x0165:  3c 90          inc $90                                  cycles=1
0x0167:  b6 90          ld A, $90                                cycles=1
0x0169:  97             ld XL, A                                 cycles=1
0x016a:  cc 00 c0       jp $c0                                   cycles=1
0x016d:  9d             nop                                      cycles=1
0x016e:  9d             nop                                      cycles=1
0x016f:  9d             nop                                      cycles=1
0x0170:  9d             nop                                      cycles=1
0x0171:  9d             nop                                      cycles=1
0x0172:  9d             nop                                      cycles=1
0x0173:  9d             nop                                      cycles=1
0x0174:  9d             nop                                      cycles=1
0x0175:  9d             nop                                      cycles=1
0x0176:  9d             nop                                      cycles=1
0x0177:  9d             nop                                      cycles=1
0x0178:  9d             nop                                      cycles=1
0x0179:  9d             nop                                      cycles=1
0x017a:  9d             nop                                      cycles=1
0x017b:  9d             nop                                      cycles=1
0x017c:  9d             nop                                      cycles=1
0x017d:  9d             nop                                      cycles=1
0x017e:  9d             nop                                      cycles=1
0x017f:  81             ret                                      cycles=4
0x0180:  cd 60 8a       call $608a                               cycles=4
0x0183:  5f             clrw X                                   cycles=1
0x0184:  3f 9c          clr $9c                                  cycles=1
0x0186:  72 0d 00 8e 18 btjf $8e, #6, $1a3  (offset=24)          cycles=2-3
0x018b:  72 00 00 98 0b btjt $98, #0, $19b  (offset=11)          cycles=2-3
0x0190:  a6 01          ld A, #$01                               cycles=1
0x0192:  c7 50 5b       ld $505b,A                               cycles=1
0x0195:  43             cpl A                                    cycles=1
0x0196:  c7 50 5c       ld $505c,A                               cycles=1
0x0199:  20 08          jra $1a3  (offset=8)                     cycles=2
0x019b:  35 81 50 5b    mov $505b, #$81                          cycles=1
0x019f:  35 7e 50 5c    mov $505c, #$7e                          cycles=1
0x01a3:  3f 98          clr $98                                  cycles=1
0x01a5:  f6             ld A, (X)                                cycles=1
0x01a6:  92 a7 00 8a    ldf ([$8a.e],X),A                        cycles=4
0x01aa:  72 0c 00 8e 13 btjt $8e, #6, $1c2  (offset=19)          cycles=2-3
0x01af:  72 00 50 5f 07 btjt $505f, #0, $1bb  (offset=7)         cycles=2-3
0x01b4:  72 05 50 5f fb btjf $505f, #2, $1b4  (offset=-5)        cycles=2-3
0x01b9:  20 04          jra $1bf  (offset=4)                     cycles=2
0x01bb:  72 10 00 9c    bset $9c, #0                             cycles=1
0x01bf:  cd 60 8a       call $608a                               cycles=4
0x01c2:  9f             ld A, XL                                 cycles=1
0x01c3:  b1 88          cp A, $88                                cycles=1
0x01c5:  27 03          jreq $1ca  (offset=3)                    cycles=1-2
0x01c7:  5c             incw X                                   cycles=1
0x01c8:  20 db          jra $1a5  (offset=-37)                   cycles=2
0x01ca:  72 0d 00 8e 10 btjf $8e, #6, $1df  (offset=16)          cycles=2-3
0x01cf:  72 00 50 5f 07 btjt $505f, #0, $1db  (offset=7)         cycles=2-3
0x01d4:  72 05 50 5f fb btjf $505f, #2, $1d4  (offset=-5)        cycles=2-3
0x01d9:  20 24          jra $1ff  (offset=36)                    cycles=2
0x01db:  72 10 00 9c    bset $9c, #0                             cycles=1
0x01df:  20 1e          jra $1ff  (offset=30)                    cycles=2
0x01e1:  9d             nop                                      cycles=1
0x01e2:  9d             nop                                      cycles=1
0x01e3:  9d             nop                                      cycles=1
0x01e4:  9d             nop                                      cycles=1
0x01e5:  9d             nop                                      cycles=1
0x01e6:  9d             nop                                      cycles=1
0x01e7:  9d             nop                                      cycles=1
0x01e8:  9d             nop                                      cycles=1
0x01e9:  9d             nop                                      cycles=1
0x01ea:  9d             nop                                      cycles=1
0x01eb:  9d             nop                                      cycles=1
0x01ec:  9d             nop                                      cycles=1
0x01ed:  9d             nop                                      cycles=1
0x01ee:  9d             nop                                      cycles=1
0x01ef:  9d             nop                                      cycles=1
0x01f0:  9d             nop                                      cycles=1
0x01f1:  9d             nop                                      cycles=1
0x01f2:  9d             nop                                      cycles=1
0x01f3:  9d             nop                                      cycles=1
0x01f4:  9d             nop                                      cycles=1
0x01f5:  9d             nop                                      cycles=1
0x01f6:  9d             nop                                      cycles=1
0x01f7:  9d             nop                                      cycles=1
0x01f8:  9d             nop                                      cycles=1
0x01f9:  9d             nop                                      cycles=1
0x01fa:  9d             nop                                      cycles=1
0x01fb:  9d             nop                                      cycles=1
0x01fc:  9d             nop                                      cycles=1
0x01fd:  9d             nop                                      cycles=1
0x01fe:  9d             nop                                      cycles=1
0x01ff:  81             ret                                      cycles=4
