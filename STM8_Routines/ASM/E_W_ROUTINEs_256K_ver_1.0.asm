
naken_util - by Michael Kohn
                Joe Davisson
    Web: http://www.mikekohn.net/
  Email: mike@mikekohn.net

Version: October 30, 2017

Loaded hexfile E_W_ROUTINEs_256K_ver_1.0.hex from 0x00a0 to 0x01d0
Type help for a list of commands.

Addr    Opcode Instruction                              Cycles
------- ------ ----------------------------------       ------
0x00a0:  5f             clrw X                                   cycles=1
0x00a1:  3f 90          clr $90                                  cycles=1
0x00a3:  72 09 00 8e 16 btjf $8e, #4, $be  (offset=22)           cycles=2-3
0x00a8:  cd 60 97       call $6097                               cycles=4
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
0x00c6:  ae 38 00       ldw X, #$3800                            cycles=2
0x00c9:  20 68          jra $133  (offset=104)                   cycles=2
0x00cb:  a1 81          cp A, #$81                               cycles=1
0x00cd:  26 07          jrne $d6  (offset=7)                     cycles=1-2
0x00cf:  3f 8a          clr $8a                                  cycles=1
0x00d1:  ae 40 00       ldw X, #$4000                            cycles=2
0x00d4:  20 5d          jra $133  (offset=93)                    cycles=2
0x00d6:  a1 10          cp A, #$10                               cycles=1
0x00d8:  24 0f          jrnc $e9  (offset=15)                    cycles=1-2
0x00da:  3f 8a          clr $8a                                  cycles=1
0x00dc:  ae 00 80       ldw X, #$80                              cycles=2
0x00df:  42             mul X, A                                 cycles=4
0x00e0:  58             sllw X                                   cycles=2
0x00e1:  58             sllw X                                   cycles=2
0x00e2:  58             sllw X                                   cycles=2
0x00e3:  58             sllw X                                   cycles=2
0x00e4:  1c 80 00       addw X, #$8000                           cycles=2
0x00e7:  20 4a          jra $133  (offset=74)                    cycles=2
0x00e9:  a1 30          cp A, #$30                               cycles=1
0x00eb:  24 10          jrnc $fd  (offset=16)                    cycles=1-2
0x00ed:  a0 10          sub A, #$10                              cycles=1
0x00ef:  ae 00 80       ldw X, #$80                              cycles=2
0x00f2:  42             mul X, A                                 cycles=4
0x00f3:  58             sllw X                                   cycles=2
0x00f4:  58             sllw X                                   cycles=2
0x00f5:  58             sllw X                                   cycles=2
0x00f6:  58             sllw X                                   cycles=2
0x00f7:  a6 01          ld A, #$01                               cycles=1
0x00f9:  b7 8a          ld $8a,A                                 cycles=1
0x00fb:  20 36          jra $133  (offset=54)                    cycles=2
0x00fd:  a1 50          cp A, #$50                               cycles=1
0x00ff:  24 10          jrnc $111  (offset=16)                   cycles=1-2
0x0101:  a0 30          sub A, #$30                              cycles=1
0x0103:  ae 00 80       ldw X, #$80                              cycles=2
0x0106:  42             mul X, A                                 cycles=4
0x0107:  58             sllw X                                   cycles=2
0x0108:  58             sllw X                                   cycles=2
0x0109:  58             sllw X                                   cycles=2
0x010a:  58             sllw X                                   cycles=2
0x010b:  a6 02          ld A, #$02                               cycles=1
0x010d:  b7 8a          ld $8a,A                                 cycles=1
0x010f:  20 22          jra $133  (offset=34)                    cycles=2
0x0111:  a1 70          cp A, #$70                               cycles=1
0x0113:  24 10          jrnc $125  (offset=16)                   cycles=1-2
0x0115:  a0 50          sub A, #$50                              cycles=1
0x0117:  ae 00 80       ldw X, #$80                              cycles=2
0x011a:  42             mul X, A                                 cycles=4
0x011b:  58             sllw X                                   cycles=2
0x011c:  58             sllw X                                   cycles=2
0x011d:  58             sllw X                                   cycles=2
0x011e:  58             sllw X                                   cycles=2
0x011f:  a6 03          ld A, #$03                               cycles=1
0x0121:  b7 8a          ld $8a,A                                 cycles=1
0x0123:  20 0e          jra $133  (offset=14)                    cycles=2
0x0125:  a0 70          sub A, #$70                              cycles=1
0x0127:  ae 00 80       ldw X, #$80                              cycles=2
0x012a:  42             mul X, A                                 cycles=4
0x012b:  58             sllw X                                   cycles=2
0x012c:  58             sllw X                                   cycles=2
0x012d:  58             sllw X                                   cycles=2
0x012e:  58             sllw X                                   cycles=2
0x012f:  a6 04          ld A, #$04                               cycles=1
0x0131:  b7 8a          ld $8a,A                                 cycles=1
0x0133:  90 5f          clrw Y                                   cycles=1
0x0135:  cd 60 97       call $6097                               cycles=4
0x0138:  9e             ld A, XH                                 cycles=1
0x0139:  b7 8b          ld $8b,A                                 cycles=1
0x013b:  9f             ld A, XL                                 cycles=1
0x013c:  b7 8c          ld $8c,A                                 cycles=1
0x013e:  a6 20          ld A, #$20                               cycles=1
0x0140:  c7 50 5b       ld $505b,A                               cycles=1
0x0143:  43             cpl A                                    cycles=1
0x0144:  c7 50 5c       ld $505c,A                               cycles=1
0x0147:  4f             clr A                                    cycles=1
0x0148:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x014c:  5c             incw X                                   cycles=1
0x014d:  9f             ld A, XL                                 cycles=1
0x014e:  b7 8c          ld $8c,A                                 cycles=1
0x0150:  4f             clr A                                    cycles=1
0x0151:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x0155:  5c             incw X                                   cycles=1
0x0156:  9f             ld A, XL                                 cycles=1
0x0157:  b7 8c          ld $8c,A                                 cycles=1
0x0159:  4f             clr A                                    cycles=1
0x015a:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x015e:  5c             incw X                                   cycles=1
0x015f:  9f             ld A, XL                                 cycles=1
0x0160:  b7 8c          ld $8c,A                                 cycles=1
0x0162:  4f             clr A                                    cycles=1
0x0163:  92 bd 00 8a    ldf [$8a.e],A                            cycles=4
0x0167:  72 05 50 5f fb btjf $505f, #2, $167  (offset=-5)        cycles=2-3
0x016c:  90 a3 00 0f    cpw Y, #$f                               cycles=2
0x0170:  27 0a          jreq $17c  (offset=10)                   cycles=1-2
0x0172:  90 5c          incw Y                                   cycles=1
0x0174:  1d 00 03       subw X, #$3                              cycles=2
0x0177:  1c 00 80       addw X, #$80                             cycles=2
0x017a:  20 b9          jra $135  (offset=-71)                   cycles=2
0x017c:  b6 90          ld A, $90                                cycles=1
0x017e:  b1 88          cp A, $88                                cycles=1
0x0180:  27 09          jreq $18b  (offset=9)                    cycles=1-2
0x0182:  5f             clrw X                                   cycles=1
0x0183:  3c 90          inc $90                                  cycles=1
0x0185:  b6 90          ld A, $90                                cycles=1
0x0187:  97             ld XL, A                                 cycles=1
0x0188:  cc 00 be       jp $be                                   cycles=1
0x018b:  81             ret                                      cycles=4
0x018c:  5f             clrw X                                   cycles=1
0x018d:  72 0d 00 8e 18 btjf $8e, #6, $1aa  (offset=24)          cycles=2-3
0x0192:  72 00 00 98 0b btjt $98, #0, $1a2  (offset=11)          cycles=2-3
0x0197:  a6 01          ld A, #$01                               cycles=1
0x0199:  c7 50 5b       ld $505b,A                               cycles=1
0x019c:  43             cpl A                                    cycles=1
0x019d:  c7 50 5c       ld $505c,A                               cycles=1
0x01a0:  20 08          jra $1aa  (offset=8)                     cycles=2
0x01a2:  35 81 50 5b    mov $505b, #$81                          cycles=1
0x01a6:  35 7e 50 5c    mov $505c, #$7e                          cycles=1
0x01aa:  3f 98          clr $98                                  cycles=1
0x01ac:  cd 60 97       call $6097                               cycles=4
0x01af:  f6             ld A, (X)                                cycles=1
0x01b0:  92 a7 00 8a    ldf ([$8a.e],X),A                        cycles=4
0x01b4:  72 0c 00 8e 05 btjt $8e, #6, $1be  (offset=5)           cycles=2-3
0x01b9:  72 05 50 5f fb btjf $505f, #2, $1b9  (offset=-5)        cycles=2-3
0x01be:  9f             ld A, XL                                 cycles=1
0x01bf:  b1 88          cp A, $88                                cycles=1
0x01c1:  27 03          jreq $1c6  (offset=3)                    cycles=1-2
0x01c3:  5c             incw X                                   cycles=1
0x01c4:  20 e6          jra $1ac  (offset=-26)                   cycles=2
0x01c6:  72 0d 00 8e 05 btjf $8e, #6, $1d0  (offset=5)           cycles=2-3
0x01cb:  72 05 50 5f fb btjf $505f, #2, $1cb  (offset=-5)        cycles=2-3
0x01d0:  81             ret                                      cycles=4
