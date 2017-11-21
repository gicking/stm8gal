
naken_util - by Michael Kohn
                Joe Davisson
    Web: http://www.mikekohn.net/
  Email: mike@mikekohn.net

Version: October 30, 2017

Loaded hexfile E_W_ROUTINEs_128K_ver_2.0.hex from 0x00a0 to 0x01a6
Type help for a list of commands.

Addr    Opcode Instruction                              Cycles
------- ------ ----------------------------------       ------
0x00a0:  5f             clrw X                                   cycles=1
0x00a1:  3f 90          clr $90                                  cycles=1
0x00a3:  72 09 00 8e 16 btjf $8e, #4, $be  (offset=22)           cycles=2-3
0x00a8:  cd 60 87       call $6087                               cycles=4
0x00ab:  b6 90          ld A, $90                                cycles=1
0x00ad:  e7 00          ld ($00,X),A                             cycles=1
0x00af:  5c             incw X                                   cycles=1
0x00b0:  4c             inc A                                    cycles=1
0x00b1:  b7 90          ld $90,A                                 cycles=1
0x00b3:  a1 82          cp A, #$82                               cycles=1
0x00b5:  26 f1          jrne $a8  (offset=-15)                   cycles=1-2
0x00b7:  a6 81          ld A, #$81                               cycles=1
0x00b9:  b7 88          ld $88,A                                 cycles=1
0x00bb:  5f             clrw X                                   cycles=1
0x00bc:  3f 90          clr $90                                  cycles=1
0x00be:  e6 00          ld A, ($00,X)                            cycles=1
0x00c0:  a1 80          cp A, #$80                               cycles=1
0x00c2:  26 07          jrne $cb  (offset=7)                     cycles=1-2
0x00c4:  3f 8a          clr $8a                                  cycles=1
0x00c6:  ae 40 00       ldw X, #$4000                            cycles=2
0x00c9:  20 3f          jra $10a  (offset=63)                    cycles=2
0x00cb:  a1 81          cp A, #$81                               cycles=1
0x00cd:  26 07          jrne $d6  (offset=7)                     cycles=1-2
0x00cf:  3f 8a          clr $8a                                  cycles=1
0x00d1:  ae 44 00       ldw X, #$4400                            cycles=2
0x00d4:  20 34          jra $10a  (offset=52)                    cycles=2
0x00d6:  a1 20          cp A, #$20                               cycles=1
0x00d8:  24 0e          jrnc $e8  (offset=14)                    cycles=1-2
0x00da:  3f 8a          clr $8a                                  cycles=1
0x00dc:  ae 00 80       ldw X, #$80                              cycles=2
0x00df:  42             mul X, A                                 cycles=4
0x00e0:  58             sllw X                                   cycles=2
0x00e1:  58             sllw X                                   cycles=2
0x00e2:  58             sllw X                                   cycles=2
0x00e3:  1c 80 00       addw X, #$8000                           cycles=2
0x00e6:  20 22          jra $10a  (offset=34)                    cycles=2
0x00e8:  a1 60          cp A, #$60                               cycles=1
0x00ea:  24 11          jrnc $fd  (offset=17)                    cycles=1-2
0x00ec:  a0 20          sub A, #$20                              cycles=1
0x00ee:  ae 00 80       ldw X, #$80                              cycles=2
0x00f1:  42             mul X, A                                 cycles=4
0x00f2:  58             sllw X                                   cycles=2
0x00f3:  58             sllw X                                   cycles=2
0x00f4:  58             sllw X                                   cycles=2
0x00f5:  a6 01          ld A, #$01                               cycles=1
0x00f7:  b7 8a          ld $8a,A                                 cycles=1
0x00f9:  20 0f          jra $10a  (offset=15)                    cycles=2
0x00fb:  20 c1          jra $be  (offset=-63)                    cycles=2
0x00fd:  a0 60          sub A, #$60                              cycles=1
0x00ff:  ae 00 80       ldw X, #$80                              cycles=2
0x0102:  42             mul X, A                                 cycles=4
0x0103:  58             sllw X                                   cycles=2
0x0104:  58             sllw X                                   cycles=2
0x0105:  58             sllw X                                   cycles=2
0x0106:  a6 02          ld A, #$02                               cycles=1
0x0108:  b7 8a          ld $8a,A                                 cycles=1
0x010a:  90 5f          clrw Y                                   cycles=1
0x010c:  cd 60 87       call $6087                               cycles=4
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
0x013e:  72 05 50 5f fb btjf $505f, #2, $13e  (offset=-5)        cycles=2-3
0x0143:  90 a3 00 07    cpw Y, #$7                               cycles=2
0x0147:  27 0a          jreq $153  (offset=10)                   cycles=1-2
0x0149:  90 5c          incw Y                                   cycles=1
0x014b:  1d 00 03       subw X, #$3                              cycles=2
0x014e:  1c 00 80       addw X, #$80                             cycles=2
0x0151:  20 b9          jra $10c  (offset=-71)                   cycles=2
0x0153:  b6 90          ld A, $90                                cycles=1
0x0155:  b1 88          cp A, $88                                cycles=1
0x0157:  27 08          jreq $161  (offset=8)                    cycles=1-2
0x0159:  5f             clrw X                                   cycles=1
0x015a:  3c 90          inc $90                                  cycles=1
0x015c:  b6 90          ld A, $90                                cycles=1
0x015e:  97             ld XL, A                                 cycles=1
0x015f:  20 9a          jra $fb  (offset=-102)                   cycles=2
0x0161:  81             ret                                      cycles=4
0x0162:  5f             clrw X                                   cycles=1
0x0163:  72 0d 00 8e 1a btjf $8e, #6, $182  (offset=26)          cycles=2-3
0x0168:  72 00 00 98 0b btjt $98, #0, $178  (offset=11)          cycles=2-3
0x016d:  a6 01          ld A, #$01                               cycles=1
0x016f:  c7 50 5b       ld $505b,A                               cycles=1
0x0172:  43             cpl A                                    cycles=1
0x0173:  c7 50 5c       ld $505c,A                               cycles=1
0x0176:  20 0a          jra $182  (offset=10)                    cycles=2
0x0178:  35 81 50 5b    mov $505b, #$81                          cycles=1
0x017c:  35 7e 50 5c    mov $505c, #$7e                          cycles=1
0x0180:  3f 98          clr $98                                  cycles=1
0x0182:  cd 60 87       call $6087                               cycles=4
0x0185:  f6             ld A, (X)                                cycles=1
0x0186:  92 a7 00 8a    ldf ([$8a.e],X),A                        cycles=4
0x018a:  72 0c 00 8e 05 btjt $8e, #6, $194  (offset=5)           cycles=2-3
0x018f:  72 05 50 5f fb btjf $505f, #2, $18f  (offset=-5)        cycles=2-3
0x0194:  9f             ld A, XL                                 cycles=1
0x0195:  b1 88          cp A, $88                                cycles=1
0x0197:  27 03          jreq $19c  (offset=3)                    cycles=1-2
0x0199:  5c             incw X                                   cycles=1
0x019a:  20 e6          jra $182  (offset=-26)                   cycles=2
0x019c:  72 0d 00 8e 05 btjf $8e, #6, $1a6  (offset=5)           cycles=2-3
0x01a1:  72 05 50 5f fb btjf $505f, #2, $1a1  (offset=-5)        cycles=2-3
0x01a6:  81             ret                                      cycles=4
