
naken_util - by Michael Kohn
                Joe Davisson
    Web: http://www.mikekohn.net/
  Email: mike@mikekohn.net

Version: October 30, 2017

Loaded hexfile E_W_ROUTINEs_32K_ver_1.2.hex from 0x00a0 to 0x01cf
Type help for a list of commands.

Addr    Opcode Instruction                              Cycles
------- ------ ----------------------------------       ------
0x00a0:  5f             clrw X                                   cycles=1
0x00a1:  3f 90          clr $90                                  cycles=1
0x00a3:  3f 97          clr $97                                  cycles=1
0x00a5:  72 09 00 8e 16 btjf $8e, #4, $c0  (offset=22)           cycles=2-3
0x00aa:  cd 60 6d       call $606d                               cycles=4
0x00ad:  b6 90          ld A, $90                                cycles=1
0x00af:  e7 00          ld ($00,X),A                             cycles=1
0x00b1:  5c             incw X                                   cycles=1
0x00b2:  4c             inc A                                    cycles=1
0x00b3:  b7 90          ld $90,A                                 cycles=1
0x00b5:  a1 21          cp A, #$21                               cycles=1
0x00b7:  26 f1          jrne $aa  (offset=-15)                   cycles=1-2
0x00b9:  a6 20          ld A, #$20                               cycles=1
0x00bb:  b7 88          ld $88,A                                 cycles=1
0x00bd:  5f             clrw X                                   cycles=1
0x00be:  3f 90          clr $90                                  cycles=1
0x00c0:  e6 00          ld A, ($00,X)                            cycles=1
0x00c2:  a1 20          cp A, #$20                               cycles=1
0x00c4:  26 07          jrne $cd  (offset=7)                     cycles=1-2
0x00c6:  3f 8a          clr $8a                                  cycles=1
0x00c8:  ae 40 00       ldw X, #$4000                            cycles=2
0x00cb:  20 0c          jra $d9  (offset=12)                     cycles=2
0x00cd:  3f 8a          clr $8a                                  cycles=1
0x00cf:  ae 00 80       ldw X, #$80                              cycles=2
0x00d2:  42             mul X, A                                 cycles=4
0x00d3:  58             sllw X                                   cycles=2
0x00d4:  58             sllw X                                   cycles=2
0x00d5:  58             sllw X                                   cycles=2
0x00d6:  1c 80 00       addw X, #$8000                           cycles=2
0x00d9:  90 5f          clrw Y                                   cycles=1
0x00db:  cd 60 6d       call $606d                               cycles=4
0x00de:  9e             ld A, XH                                 cycles=1
0x00df:  b7 8b          ld $8b,A                                 cycles=1
0x00e1:  9f             ld A, XL                                 cycles=1
0x00e2:  b7 8c          ld $8c,A                                 cycles=1
0x00e4:  a6 20          ld A, #$20                               cycles=1
0x00e6:  c7 50 5b       ld $505b,A                               cycles=1
0x00e9:  43             cpl A                                    cycles=1
0x00ea:  c7 50 5c       ld $505c,A                               cycles=1
0x00ed:  4f             clr A                                    cycles=1
0x00ee:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x00f2:  5c             incw X                                   cycles=1
0x00f3:  9f             ld A, XL                                 cycles=1
0x00f4:  b7 8c          ld $8c,A                                 cycles=1
0x00f6:  4f             clr A                                    cycles=1
0x00f7:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x00fb:  5c             incw X                                   cycles=1
0x00fc:  9f             ld A, XL                                 cycles=1
0x00fd:  b7 8c          ld $8c,A                                 cycles=1
0x00ff:  4f             clr A                                    cycles=1
0x0100:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x0104:  5c             incw X                                   cycles=1
0x0105:  9f             ld A, XL                                 cycles=1
0x0106:  b7 8c          ld $8c,A                                 cycles=1
0x0108:  4f             clr A                                    cycles=1
0x0109:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x010d:  72 00 50 5f 07 btjt $505f, #0, $119  (offset=7)         cycles=2-3
0x0112:  72 05 50 5f fb btjf $505f, #2, $112  (offset=-5)        cycles=2-3
0x0117:  20 04          jra $11d  (offset=4)                     cycles=2
0x0119:  72 10 00 97    bset $97, #0                             cycles=1
0x011d:  90 a3 00 07    cpw Y, #$7                               cycles=2
0x0121:  27 0a          jreq $12d  (offset=10)                   cycles=1-2
0x0123:  90 5c          incw Y                                   cycles=1
0x0125:  1d 00 03       subw X, #$3                              cycles=2
0x0128:  1c 00 80       addw X, #$80                             cycles=2
0x012b:  20 ae          jra $db  (offset=-82)                    cycles=2
0x012d:  b6 90          ld A, $90                                cycles=1
0x012f:  b1 88          cp A, $88                                cycles=1
0x0131:  27 1c          jreq $14f  (offset=28)                   cycles=1-2
0x0133:  5f             clrw X                                   cycles=1
0x0134:  3c 90          inc $90                                  cycles=1
0x0136:  b6 90          ld A, $90                                cycles=1
0x0138:  97             ld XL, A                                 cycles=1
0x0139:  cc 00 c0       jp $c0                                   cycles=1
0x013c:  9d             nop                                      cycles=1
0x013d:  9d             nop                                      cycles=1
0x013e:  9d             nop                                      cycles=1
0x013f:  9d             nop                                      cycles=1
0x0140:  9d             nop                                      cycles=1
0x0141:  9d             nop                                      cycles=1
0x0142:  9d             nop                                      cycles=1
0x0143:  9d             nop                                      cycles=1
0x0144:  9d             nop                                      cycles=1
0x0145:  9d             nop                                      cycles=1
0x0146:  9d             nop                                      cycles=1
0x0147:  9d             nop                                      cycles=1
0x0148:  9d             nop                                      cycles=1
0x0149:  9d             nop                                      cycles=1
0x014a:  9d             nop                                      cycles=1
0x014b:  9d             nop                                      cycles=1
0x014c:  9d             nop                                      cycles=1
0x014d:  9d             nop                                      cycles=1
0x014e:  9d             nop                                      cycles=1
0x014f:  81             ret                                      cycles=4
0x0150:  cd 60 6d       call $606d                               cycles=4
0x0153:  5f             clrw X                                   cycles=1
0x0154:  3f 98          clr $98                                  cycles=1
0x0156:  72 0d 00 8e 18 btjf $8e, #6, $173  (offset=24)          cycles=2-3
0x015b:  72 00 00 94 0b btjt $94, #0, $16b  (offset=11)          cycles=2-3
0x0160:  a6 01          ld A, #$01                               cycles=1
0x0162:  c7 50 5b       ld $505b,A                               cycles=1
0x0165:  43             cpl A                                    cycles=1
0x0166:  c7 50 5c       ld $505c,A                               cycles=1
0x0169:  20 08          jra $173  (offset=8)                     cycles=2
0x016b:  35 81 50 5b    mov $505b, #$81                          cycles=1
0x016f:  35 7e 50 5c    mov $505c, #$7e                          cycles=1
0x0173:  3f 94          clr $94                                  cycles=1
0x0175:  cd 60 6d       call $606d                               cycles=4
0x0178:  f6             ld A, (X)                                cycles=1
0x0179:  92 a7 00 8a    ldf ([$8a.e],X),A                        cycles=4
0x017d:  72 0c 00 8e 13 btjt $8e, #6, $195  (offset=19)          cycles=2-3
0x0182:  72 00 50 5f 07 btjt $505f, #0, $18e  (offset=7)         cycles=2-3
0x0187:  72 05 50 5f fb btjf $505f, #2, $187  (offset=-5)        cycles=2-3
0x018c:  20 04          jra $192  (offset=4)                     cycles=2
0x018e:  72 10 00 98    bset $98, #0                             cycles=1
0x0192:  cd 60 6d       call $606d                               cycles=4
0x0195:  9f             ld A, XL                                 cycles=1
0x0196:  b1 88          cp A, $88                                cycles=1
0x0198:  27 03          jreq $19d  (offset=3)                    cycles=1-2
0x019a:  5c             incw X                                   cycles=1
0x019b:  20 d8          jra $175  (offset=-40)                   cycles=2
0x019d:  72 0d 00 8e 10 btjf $8e, #6, $1b2  (offset=16)          cycles=2-3
0x01a2:  72 00 50 5f 07 btjt $505f, #0, $1ae  (offset=7)         cycles=2-3
0x01a7:  72 05 50 5f fb btjf $505f, #2, $1a7  (offset=-5)        cycles=2-3
0x01ac:  20 21          jra $1cf  (offset=33)                    cycles=2
0x01ae:  72 10 00 98    bset $98, #0                             cycles=1
0x01b2:  20 1b          jra $1cf  (offset=27)                    cycles=2
0x01b4:  9d             nop                                      cycles=1
0x01b5:  9d             nop                                      cycles=1
0x01b6:  9d             nop                                      cycles=1
0x01b7:  9d             nop                                      cycles=1
0x01b8:  9d             nop                                      cycles=1
0x01b9:  9d             nop                                      cycles=1
0x01ba:  9d             nop                                      cycles=1
0x01bb:  9d             nop                                      cycles=1
0x01bc:  9d             nop                                      cycles=1
0x01bd:  9d             nop                                      cycles=1
0x01be:  9d             nop                                      cycles=1
0x01bf:  9d             nop                                      cycles=1
0x01c0:  9d             nop                                      cycles=1
0x01c1:  9d             nop                                      cycles=1
0x01c2:  9d             nop                                      cycles=1
0x01c3:  9d             nop                                      cycles=1
0x01c4:  9d             nop                                      cycles=1
0x01c5:  9d             nop                                      cycles=1
0x01c6:  9d             nop                                      cycles=1
0x01c7:  9d             nop                                      cycles=1
0x01c8:  9d             nop                                      cycles=1
0x01c9:  9d             nop                                      cycles=1
0x01ca:  9d             nop                                      cycles=1
0x01cb:  9d             nop                                      cycles=1
0x01cc:  9d             nop                                      cycles=1
0x01cd:  9d             nop                                      cycles=1
0x01ce:  9d             nop                                      cycles=1
0x01cf:  81             ret                                      cycles=4
