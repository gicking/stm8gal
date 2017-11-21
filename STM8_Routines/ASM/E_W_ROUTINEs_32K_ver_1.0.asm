
naken_util - by Michael Kohn
                Joe Davisson
    Web: http://www.mikekohn.net/
  Email: mike@mikekohn.net

Version: October 30, 2017

Loaded hexfile E_W_ROUTINEs_32K_ver_1.0.hex from 0x00a0 to 0x0173
Type help for a list of commands.

Addr    Opcode Instruction                              Cycles
------- ------ ----------------------------------       ------
0x00a0:  5f             clrw X                                   cycles=1
0x00a1:  3f 90          clr $90                                  cycles=1
0x00a3:  72 09 00 8e 16 btjf $8e, #4, $be  (offset=22)           cycles=2-3
0x00a8:  cd 60 5f       call $605f                               cycles=4
0x00ab:  b6 90          ld A, $90                                cycles=1
0x00ad:  e7 00          ld ($00,X),A                             cycles=1
0x00af:  5c             incw X                                   cycles=1
0x00b0:  4c             inc A                                    cycles=1
0x00b1:  b7 90          ld $90,A                                 cycles=1
0x00b3:  a1 21          cp A, #$21                               cycles=1
0x00b5:  26 f1          jrne $a8  (offset=-15)                   cycles=1-2
0x00b7:  a6 20          ld A, #$20                               cycles=1
0x00b9:  b7 88          ld $88,A                                 cycles=1
0x00bb:  5f             clrw X                                   cycles=1
0x00bc:  3f 90          clr $90                                  cycles=1
0x00be:  e6 00          ld A, ($00,X)                            cycles=1
0x00c0:  a1 20          cp A, #$20                               cycles=1
0x00c2:  26 07          jrne $cb  (offset=7)                     cycles=1-2
0x00c4:  3f 8a          clr $8a                                  cycles=1
0x00c6:  ae 40 00       ldw X, #$4000                            cycles=2
0x00c9:  20 0c          jra $d7  (offset=12)                     cycles=2
0x00cb:  3f 8a          clr $8a                                  cycles=1
0x00cd:  ae 00 80       ldw X, #$80                              cycles=2
0x00d0:  42             mul X, A                                 cycles=4
0x00d1:  58             sllw X                                   cycles=2
0x00d2:  58             sllw X                                   cycles=2
0x00d3:  58             sllw X                                   cycles=2
0x00d4:  1c 80 00       addw X, #$8000                           cycles=2
0x00d7:  90 5f          clrw Y                                   cycles=1
0x00d9:  cd 60 5f       call $605f                               cycles=4
0x00dc:  9e             ld A, XH                                 cycles=1
0x00dd:  b7 8b          ld $8b,A                                 cycles=1
0x00df:  9f             ld A, XL                                 cycles=1
0x00e0:  b7 8c          ld $8c,A                                 cycles=1
0x00e2:  a6 20          ld A, #$20                               cycles=1
0x00e4:  c7 50 5b       ld $505b,A                               cycles=1
0x00e7:  43             cpl A                                    cycles=1
0x00e8:  c7 50 5c       ld $505c,A                               cycles=1
0x00eb:  4f             clr A                                    cycles=1
0x00ec:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x00f0:  5c             incw X                                   cycles=1
0x00f1:  9f             ld A, XL                                 cycles=1
0x00f2:  b7 8c          ld $8c,A                                 cycles=1
0x00f4:  4f             clr A                                    cycles=1
0x00f5:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x00f9:  5c             incw X                                   cycles=1
0x00fa:  9f             ld A, XL                                 cycles=1
0x00fb:  b7 8c          ld $8c,A                                 cycles=1
0x00fd:  4f             clr A                                    cycles=1
0x00fe:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x0102:  5c             incw X                                   cycles=1
0x0103:  9f             ld A, XL                                 cycles=1
0x0104:  b7 8c          ld $8c,A                                 cycles=1
0x0106:  4f             clr A                                    cycles=1
0x0107:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x010b:  72 05 50 5f fb btjf $505f, #2, $10b  (offset=-5)        cycles=2-3
0x0110:  90 a3 00 07    cpw Y, #$7                               cycles=2
0x0114:  27 0a          jreq $120  (offset=10)                   cycles=1-2
0x0116:  90 5c          incw Y                                   cycles=1
0x0118:  1d 00 03       subw X, #$3                              cycles=2
0x011b:  1c 00 80       addw X, #$80                             cycles=2
0x011e:  20 b9          jra $d9  (offset=-71)                    cycles=2
0x0120:  b6 90          ld A, $90                                cycles=1
0x0122:  b1 88          cp A, $88                                cycles=1
0x0124:  27 08          jreq $12e  (offset=8)                    cycles=1-2
0x0126:  5f             clrw X                                   cycles=1
0x0127:  3c 90          inc $90                                  cycles=1
0x0129:  b6 90          ld A, $90                                cycles=1
0x012b:  97             ld XL, A                                 cycles=1
0x012c:  20 90          jra $be  (offset=-112)                   cycles=2
0x012e:  81             ret                                      cycles=4
0x012f:  5f             clrw X                                   cycles=1
0x0130:  72 0d 00 8e 1a btjf $8e, #6, $14f  (offset=26)          cycles=2-3
0x0135:  72 00 00 94 0b btjt $94, #0, $145  (offset=11)          cycles=2-3
0x013a:  a6 01          ld A, #$01                               cycles=1
0x013c:  c7 50 5b       ld $505b,A                               cycles=1
0x013f:  43             cpl A                                    cycles=1
0x0140:  c7 50 5c       ld $505c,A                               cycles=1
0x0143:  20 0a          jra $14f  (offset=10)                    cycles=2
0x0145:  35 81 50 5b    mov $505b, #$81                          cycles=1
0x0149:  35 7e 50 5c    mov $505c, #$7e                          cycles=1
0x014d:  3f 94          clr $94                                  cycles=1
0x014f:  cd 60 5f       call $605f                               cycles=4
0x0152:  f6             ld A, (X)                                cycles=1
0x0153:  92 a7 00 8a    ldf ([$8a.e],X),A                        cycles=4
0x0157:  72 0c 00 8e 05 btjt $8e, #6, $161  (offset=5)           cycles=2-3
0x015c:  72 05 50 5f fb btjf $505f, #2, $15c  (offset=-5)        cycles=2-3
0x0161:  9f             ld A, XL                                 cycles=1
0x0162:  b1 88          cp A, $88                                cycles=1
0x0164:  27 03          jreq $169  (offset=3)                    cycles=1-2
0x0166:  5c             incw X                                   cycles=1
0x0167:  20 e6          jra $14f  (offset=-26)                   cycles=2
0x0169:  72 0d 00 8e 05 btjf $8e, #6, $173  (offset=5)           cycles=2-3
0x016e:  72 05 50 5f fb btjf $505f, #2, $16e  (offset=-5)        cycles=2-3
0x0173:  81             ret                                      cycles=4
