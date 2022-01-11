
problem.o:     file format elf32-powerpc

Disassembly of section .text:
DbgAdd():
00000000 <DbgAdd> cmpwi	r3,0
00000004 <DbgAdd+4> bne	00000010 <DbgAdd+10>
00000008 <DbgAdd+8> lis	r3,0
		RELOC: 0000000a R_PPC_ADDR16_HA head
0000000c <DbgAdd+c> addi	r3,r3,0
		RELOC: 0000000e R_PPC_ADDR16_LO head
00000010 <DbgAdd+10> lwz	r6,0(r3)
00000014 <DbgAdd+14> cmpwi	r6,0
00000018 <DbgAdd+18> bne	0000002c <DbgAdd+2c>
0000001c <DbgAdd+1c> stw	r4,0(r3)
00000020 <DbgAdd+20> stw	r4,16(r4)
00000024 <DbgAdd+24> stw	r4,20(r4)
00000028 <DbgAdd+28> b	00000044 <DbgAdd+44>
0000002c <DbgAdd+2c> lwz	r5,20(r6)
00000030 <DbgAdd+30> stw	r5,20(r4)
00000034 <DbgAdd+34> stw	r4,16(r5)
00000038 <DbgAdd+38> lwz	r11,0(r3)
0000003c <DbgAdd+3c> stw	r11,16(r4)
00000040 <DbgAdd+40> stw	r4,20(r6)
00000044 <DbgAdd+44> blr
DbgRemove():
00000048 <DbgRemove> cmpwi	r3,0
0000004c <DbgRemove+4> bne	00000058 <DbgRemove+10>
00000050 <DbgRemove+8> lis	r3,0
		RELOC: 00000052 R_PPC_ADDR16_HA head
00000054 <DbgRemove+c> addi	r3,r3,0
		RELOC: 00000056 R_PPC_ADDR16_LO head
00000058 <DbgRemove+10> lwz	r5,16(r4)
0000005c <DbgRemove+14> cmplw	r5,r4
00000060 <DbgRemove+18> bne	00000070 <DbgRemove+28>
00000064 <DbgRemove+1c> li	r12,0
00000068 <DbgRemove+20> stw	r12,0(r3)
0000006c <DbgRemove+24> b	00000084 <DbgRemove+3c>
00000070 <DbgRemove+28> lwz	r12,20(r4)
00000074 <DbgRemove+2c> stw	r5,16(r12)
00000078 <DbgRemove+30> lwz	r11,16(r4)
0000007c <DbgRemove+34> lwz	r10,20(r4)
00000080 <DbgRemove+38> stw	r10,20(r11)
00000084 <DbgRemove+3c> lwz	r12,0(r3)
00000088 <DbgRemove+40> cmplw	r12,r4
0000008c <DbgRemove+44> bne	00000098 <DbgRemove+50>
00000090 <DbgRemove+48> lwz	r12,16(r4)
00000094 <DbgRemove+4c> stw	r12,0(r3)
00000098 <DbgRemove+50> blr
DbgIncrement():
0000009c <DbgIncrement> stwu	r1,-24(r1)
000000a0 <DbgIncrement+4> lwz	r4,4(r3)
000000a4 <DbgIncrement+8> cmpwi	r4,1
000000a8 <DbgIncrement+c> bne	000000d4 <DbgIncrement+38>
000000ac <DbgIncrement+10> lwz	r4,0(r3)
000000b0 <DbgIncrement+14> lwz	r11,0(r4)
000000b4 <DbgIncrement+18> lfs	f13,8(r3)
000000b8 <DbgIncrement+1c> fctiwz	f0,f13
000000bc <DbgIncrement+20> li	r0,8
000000c0 <DbgIncrement+24> stfiwx	f0,r1,r0
000000c4 <DbgIncrement+28> lwz	r10,8(r1)
000000c8 <DbgIncrement+2c> add	r11,r11,r10
000000cc <DbgIncrement+30> stw	r11,0(r4)
000000d0 <DbgIncrement+34> b	000000f0 <DbgIncrement+54>
000000d4 <DbgIncrement+38> cmpwi	r4,2
000000d8 <DbgIncrement+3c> bne	000000f0 <DbgIncrement+54>
000000dc <DbgIncrement+40> lwz	r4,0(r3)
000000e0 <DbgIncrement+44> lfs	f13,0(r4)
000000e4 <DbgIncrement+48> lfs	f12,8(r3)
000000e8 <DbgIncrement+4c> fadds	f13,f13,f12
000000ec <DbgIncrement+50> stfs	f13,0(r4)
000000f0 <DbgIncrement+54> addi	r1,r1,24
000000f4 <DbgIncrement+58> blr
DbgDecrement():
000000f8 <DbgDecrement> stwu	r1,-24(r1)
000000fc <DbgDecrement+4> lwz	r4,4(r3)
00000100 <DbgDecrement+8> cmpwi	r4,1
00000104 <DbgDecrement+c> bne	00000130 <DbgDecrement+38>
00000108 <DbgDecrement+10> lwz	r4,0(r3)
0000010c <DbgDecrement+14> lwz	r11,0(r4)
00000110 <DbgDecrement+18> lfs	f13,8(r3)
00000114 <DbgDecrement+1c> fctiwz	f0,f13
00000118 <DbgDecrement+20> li	r0,8
0000011c <DbgDecrement+24> stfiwx	f0,r1,r0
00000120 <DbgDecrement+28> lwz	r10,8(r1)
00000124 <DbgDecrement+2c> subf	r11,r10,r11
00000128 <DbgDecrement+30> stw	r11,0(r4)
0000012c <DbgDecrement+34> b	0000014c <DbgDecrement+54>
00000130 <DbgDecrement+38> cmpwi	r4,2
00000134 <DbgDecrement+3c> bne	0000014c <DbgDecrement+54>
00000138 <DbgDecrement+40> lwz	r4,0(r3)
0000013c <DbgDecrement+44> lfs	f13,0(r4)
00000140 <DbgDecrement+48> lfs	f12,8(r3)
00000144 <DbgDecrement+4c> fsubs	f13,f13,f12
00000148 <DbgDecrement+50> stfs	f13,0(r4)
0000014c <DbgDecrement+54> addi	r1,r1,24
00000150 <DbgDecrement+58> blr
DbgPrintVar():
00000154 <DbgPrintVar> stwu	r1,-72(r1)
00000158 <DbgPrintVar+4> mflr	r0
0000015c <DbgPrintVar+8> stfs	f30,64(r1)
00000160 <DbgPrintVar+c> stfs	f31,68(r1)
00000164 <DbgPrintVar+10> stw	r31,60(r1)
00000168 <DbgPrintVar+14> stw	r0,76(r1)
0000016c <DbgPrintVar+18> addi	r31,r3,0
00000170 <DbgPrintVar+1c> fmr	f31,f1
00000174 <DbgPrintVar+20> fmr	f30,f2
00000178 <DbgPrintVar+24> lwz	r6,4(r31)
0000017c <DbgPrintVar+28> cmpwi	r6,1
00000180 <DbgPrintVar+2c> bne	000001a8 <DbgPrintVar+54>
00000184 <DbgPrintVar+30> lwz	r6,0(r31)
00000188 <DbgPrintVar+34> lwz	r5,12(r31)
0000018c <DbgPrintVar+38> addi	r3,r1,16
00000190 <DbgPrintVar+3c> lis	r4,0
		RELOC: 00000192 R_PPC_ADDR16_HA .text+0x00000414
00000194 <DbgPrintVar+40> addi	r4,r4,0
		RELOC: 00000196 R_PPC_ADDR16_LO .text+0x00000414
00000198 <DbgPrintVar+44> lwz	r6,0(r6)
0000019c <DbgPrintVar+48> crclr	6
000001a0 <DbgPrintVar+4c> bl	000001a0 <DbgPrintVar+4c>
		RELOC: 000001a0 R_PPC_REL24 sprintf
000001a4 <DbgPrintVar+50> b	000001d0 <DbgPrintVar+7c>
000001a8 <DbgPrintVar+54> cmpwi	r6,2
000001ac <DbgPrintVar+58> bne	000001d0 <DbgPrintVar+7c>
000001b0 <DbgPrintVar+5c> lwz	r6,0(r31)
000001b4 <DbgPrintVar+60> lwz	r5,12(r31)
000001b8 <DbgPrintVar+64> addi	r3,r1,16
000001bc <DbgPrintVar+68> lis	r4,0
		RELOC: 000001be R_PPC_ADDR16_HA .text+0x0000041d
000001c0 <DbgPrintVar+6c> addi	r4,r4,0
		RELOC: 000001c2 R_PPC_ADDR16_LO .text+0x0000041d
000001c4 <DbgPrintVar+70> lfs	f1,0(r6)
000001c8 <DbgPrintVar+74> crset	6
000001cc <DbgPrintVar+78> bl	000001cc <DbgPrintVar+78>
		RELOC: 000001cc R_PPC_REL24 sprintf
000001d0 <DbgPrintVar+7c> lis	r31,0
		RELOC: 000001d2 R_PPC_ADDR16_HA gdp
000001d4 <DbgPrintVar+80> lwz	r31,0(r31)
		RELOC: 000001d6 R_PPC_ADDR16_LO gdp
000001d8 <DbgPrintVar+84> lwz	r3,0(r31)
000001dc <DbgPrintVar+88> addi	r4,r31,44
000001e0 <DbgPrintVar+8c> addi	r5,r1,16
000001e4 <DbgPrintVar+90> fmr	f1,f31
000001e8 <DbgPrintVar+94> fmr	f2,f30
000001ec <DbgPrintVar+98> bl	000001ec <DbgPrintVar+98>
		RELOC: 000001ec R_PPC_REL24 TwoD_DrawText
000001f0 <DbgPrintVar+9c> lwz	r31,60(r1)
000001f4 <DbgPrintVar+a0> lfs	f30,64(r1)
000001f8 <DbgPrintVar+a4> lfs	f31,68(r1)
000001fc <DbgPrintVar+a8> lwz	r0,76(r1)
00000200 <DbgPrintVar+ac> mtlr	r0
00000204 <DbgPrintVar+b0> addi	r1,r1,72
00000208 <DbgPrintVar+b4> blr
DbgInitVars():
0000020c <DbgInitVars> stwu	r1,-24(r1)
00000210 <DbgInitVars+4> mflr	r0
00000214 <DbgInitVars+8> stw	r0,28(r1)
00000218 <DbgInitVars+c> lis	r4,0
		RELOC: 0000021a R_PPC_ADDR16_HA di1
0000021c <DbgInitVars+10> addi	r4,r4,0
		RELOC: 0000021e R_PPC_ADDR16_LO di1
00000220 <DbgInitVars+14> lis	r11,0
		RELOC: 00000222 R_PPC_ADDR16_HA vi1
00000224 <DbgInitVars+18> addi	r11,r11,0
		RELOC: 00000226 R_PPC_ADDR16_LO vi1
00000228 <DbgInitVars+1c> stw	r11,0(r4)
0000022c <DbgInitVars+20> li	r10,1
00000230 <DbgInitVars+24> stw	r10,4(r4)
00000234 <DbgInitVars+28> lis	r9,0
		RELOC: 00000236 R_PPC_ADDR16_HA .text+0x00000408
00000238 <DbgInitVars+2c> lfs	f13,0(r9)
		RELOC: 0000023a R_PPC_ADDR16_LO .text+0x00000408
0000023c <DbgInitVars+30> stfs	f13,8(r4)
00000240 <DbgInitVars+34> lis	r12,0
		RELOC: 00000242 R_PPC_ADDR16_HA i1
00000244 <DbgInitVars+38> lwz	r12,0(r12)
		RELOC: 00000246 R_PPC_ADDR16_LO i1
00000248 <DbgInitVars+3c> stw	r12,12(r4)
0000024c <DbgInitVars+40> li	r3,0
00000250 <DbgInitVars+44> bl	00000000 <DbgAdd>
00000254 <DbgInitVars+48> lis	r4,0
		RELOC: 00000256 R_PPC_ADDR16_HA df1
00000258 <DbgInitVars+4c> addi	r4,r4,0
		RELOC: 0000025a R_PPC_ADDR16_LO df1
0000025c <DbgInitVars+50> lis	r9,0
		RELOC: 0000025e R_PPC_ADDR16_HA vf1
00000260 <DbgInitVars+54> addi	r9,r9,0
		RELOC: 00000262 R_PPC_ADDR16_LO vf1
00000264 <DbgInitVars+58> stw	r9,0(r4)
00000268 <DbgInitVars+5c> li	r12,2
0000026c <DbgInitVars+60> stw	r12,4(r4)
00000270 <DbgInitVars+64> lis	r11,0
		RELOC: 00000272 R_PPC_ADDR16_HA .text+0x0000040c
00000274 <DbgInitVars+68> lfs	f12,0(r11)
		RELOC: 00000276 R_PPC_ADDR16_LO .text+0x0000040c
00000278 <DbgInitVars+6c> stfs	f12,8(r4)
0000027c <DbgInitVars+70> lis	r10,0
		RELOC: 0000027e R_PPC_ADDR16_HA f1
00000280 <DbgInitVars+74> lwz	r10,0(r10)
		RELOC: 00000282 R_PPC_ADDR16_LO f1
00000284 <DbgInitVars+78> stw	r10,12(r4)
00000288 <DbgInitVars+7c> li	r3,0
0000028c <DbgInitVars+80> bl	00000000 <DbgAdd>
00000290 <DbgInitVars+84> lwz	r0,28(r1)
00000294 <DbgInitVars+88> mtlr	r0
00000298 <DbgInitVars+8c> addi	r1,r1,24
0000029c <DbgInitVars+90> blr
DbgDisplayVars():
000002a0 <DbgDisplayVars> stwu	r1,-104(r1)
000002a4 <DbgDisplayVars+4> mflr	r0
000002a8 <DbgDisplayVars+8> stw	r28,88(r1)
000002ac <DbgDisplayVars+c> stw	r29,92(r1)
000002b0 <DbgDisplayVars+10> stw	r30,96(r1)
000002b4 <DbgDisplayVars+14> stw	r31,100(r1)
000002b8 <DbgDisplayVars+18> stw	r0,108(r1)
000002bc <DbgDisplayVars+1c> lis	r28,0
		RELOC: 000002be R_PPC_ADDR16_HA head
000002c0 <DbgDisplayVars+20> addi	r28,r28,0
		RELOC: 000002c2 R_PPC_ADDR16_LO head
000002c4 <DbgDisplayVars+24> lwz	r11,0(r28)
000002c8 <DbgDisplayVars+28> cmpwi	r11,0
000002cc <DbgDisplayVars+2c> beq	00000394 <DbgDisplayVars+f4>
000002d0 <DbgDisplayVars+30> lis	r12,0
		RELOC: 000002d2 R_PPC_ADDR16_HA gdp
000002d4 <DbgDisplayVars+34> lwz	r12,0(r12)
		RELOC: 000002d6 R_PPC_ADDR16_LO gdp
000002d8 <DbgDisplayVars+38> lwz	r3,28(r12)
000002dc <DbgDisplayVars+3c> addi	r5,r1,16
000002e0 <DbgDisplayVars+40> li	r4,0
000002e4 <DbgDisplayVars+44> bl	000002e4 <DbgDisplayVars+44>
		RELOC: 000002e4 R_PPC_REL24 GetCPortData
000002e8 <DbgDisplayVars+48> lwz	r31,16(r1)
000002ec <DbgDisplayVars+4c> cmpwi	r31,0
000002f0 <DbgDisplayVars+50> bne	00000304 <DbgDisplayVars+64>
000002f4 <DbgDisplayVars+54> lis	r12,0
		RELOC: 000002f6 R_PPC_ADDR16_HA .data
000002f8 <DbgDisplayVars+58> li	r11,20
000002fc <DbgDisplayVars+5c> stw	r11,0(r12)
		RELOC: 000002fe R_PPC_ADDR16_LO .data
00000300 <DbgDisplayVars+60> b	00000394 <DbgDisplayVars+f4>
00000304 <DbgDisplayVars+64> lis	r29,0
		RELOC: 00000306 R_PPC_ADDR16_HA .data
00000308 <DbgDisplayVars+68> addi	r29,r29,0
		RELOC: 0000030a R_PPC_ADDR16_LO .data
0000030c <DbgDisplayVars+6c> lwz	r11,0(r29)
00000310 <DbgDisplayVars+70> addi	r30,r11,-1
00000314 <DbgDisplayVars+74> stw	r30,0(r29)
00000318 <DbgDisplayVars+78> cmpwi	r30,19
0000031c <DbgDisplayVars+7c> beq	00000330 <DbgDisplayVars+90>
00000320 <DbgDisplayVars+80> cmpwi	r30,0
00000324 <DbgDisplayVars+84> bgt	00000394 <DbgDisplayVars+f4>
00000328 <DbgDisplayVars+88> li	r12,5
0000032c <DbgDisplayVars+8c> stw	r12,0(r29)
00000330 <DbgDisplayVars+90> lis	r12,-32768
00000334 <DbgDisplayVars+94> cmplw	r31,r12
00000338 <DbgDisplayVars+98> bne	0000034c <DbgDisplayVars+ac>
0000033c <DbgDisplayVars+9c> lwz	r12,0(r28)
00000340 <DbgDisplayVars+a0> lwz	r12,16(r12)
00000344 <DbgDisplayVars+a4> stw	r12,0(r28)
00000348 <DbgDisplayVars+a8> b	00000394 <DbgDisplayVars+f4>
0000034c <DbgDisplayVars+ac> lis	r12,16384
00000350 <DbgDisplayVars+b0> cmplw	r31,r12
00000354 <DbgDisplayVars+b4> bne	00000368 <DbgDisplayVars+c8>
00000358 <DbgDisplayVars+b8> lwz	r12,0(r28)
0000035c <DbgDisplayVars+bc> lwz	r12,20(r12)
00000360 <DbgDisplayVars+c0> stw	r12,0(r28)
00000364 <DbgDisplayVars+c4> b	00000394 <DbgDisplayVars+f4>
00000368 <DbgDisplayVars+c8> lis	r12,4096
0000036c <DbgDisplayVars+cc> cmplw	r31,r12
00000370 <DbgDisplayVars+d0> bne	00000380 <DbgDisplayVars+e0>
00000374 <DbgDisplayVars+d4> lwz	r3,0(r28)
00000378 <DbgDisplayVars+d8> bl	000000f8 <DbgDecrement>
0000037c <DbgDisplayVars+dc> b	00000394 <DbgDisplayVars+f4>
00000380 <DbgDisplayVars+e0> lis	r12,8192
00000384 <DbgDisplayVars+e4> cmplw	r31,r12
00000388 <DbgDisplayVars+e8> bne	00000394 <DbgDisplayVars+f4>
0000038c <DbgDisplayVars+ec> lwz	r3,0(r28)
00000390 <DbgDisplayVars+f0> bl	0000009c <DbgIncrement>
00000394 <DbgDisplayVars+f4> lwz	r30,0(r28)
00000398 <DbgDisplayVars+f8> li	r29,200
0000039c <DbgDisplayVars+fc> li	r31,6
000003a0 <DbgDisplayVars+100> lwz	r12,0(r28)
000003a4 <DbgDisplayVars+104> cmplw	r12,r30
000003a8 <DbgDisplayVars+108> bne	000003b8 <DbgDisplayVars+118>
000003ac <DbgDisplayVars+10c> subfic	r3,r31,6
000003b0 <DbgDisplayVars+110> cmpwi	r3,0
000003b4 <DbgDisplayVars+114> bgt	000003e8 <DbgDisplayVars+148>
000003b8 <DbgDisplayVars+118> addi	r3,r29,0
000003bc <DbgDisplayVars+11c> bl	000003bc <DbgDisplayVars+11c>
		RELOC: 000003bc R_PPC_REL24 __itof
000003c0 <DbgDisplayVars+120> fmr	f2,f1
000003c4 <DbgDisplayVars+124> addi	r3,r30,0
000003c8 <DbgDisplayVars+128> lis	r12,0
		RELOC: 000003ca R_PPC_ADDR16_HA .text+0x00000410
000003cc <DbgDisplayVars+12c> lfs	f1,0(r12)
		RELOC: 000003ce R_PPC_ADDR16_LO .text+0x00000410
000003d0 <DbgDisplayVars+130> bl	00000154 <DbgPrintVar>
000003d4 <DbgDisplayVars+134> addi	r29,r29,30
000003d8 <DbgDisplayVars+138> addi	r31,r31,-1
000003dc <DbgDisplayVars+13c> lwz	r30,16(r30)
000003e0 <DbgDisplayVars+140> cmpwi	r31,0
000003e4 <DbgDisplayVars+144> bne	000003a0 <DbgDisplayVars+100>
000003e8 <DbgDisplayVars+148> lwz	r28,88(r1)
000003ec <DbgDisplayVars+14c> lwz	r29,92(r1)
000003f0 <DbgDisplayVars+150> lwz	r30,96(r1)
000003f4 <DbgDisplayVars+154> lwz	r31,100(r1)
000003f8 <DbgDisplayVars+158> lwz	r0,108(r1)
000003fc <DbgDisplayVars+15c> mtlr	r0
00000400 <DbgDisplayVars+160> addi	r1,r1,104
00000404 <DbgDisplayVars+164> blr
00000408 <DbgDisplayVars+168> lis	r28,0
0000040c <DbgDisplayVars+16c> addis	r6,r12,-13107
00000410 <DbgDisplayVars+170> bdnz+	00000410 <DbgDisplayVars+170>
00000414 <DbgDisplayVars+174> dozi	r11,r19,8253
00000418 <DbgDisplayVars+178> subfic	r1,r5,25600
0000041c <DbgDisplayVars+17c> .long 0x257320
00000420 <DbgDisplayVars+180> lis	r9,9574
00000424 <DbgDisplayVars+184> .long 0x5445
00000428 <DbgDisplayVars+188> rlwimi.	r20,r26,4,1,4
0000042c <DbgDisplayVars+18c> .long 0x4e543100
00000430 <DbgDisplayVars+190> rlwinm	r5,r2,10,13,10
00000434 <DbgDisplayVars+194> subfic	r2,r9,20052
00000438 <DbgDisplayVars+198> addic	r16,r0,21573
0000043c <DbgDisplayVars+19c> rlwimi.	r20,r26,4,1,4
00000440 <DbgDisplayVars+1a0> .long 0x4e543300
00000444 <DbgDisplayVars+1a4> rlwinm	r5,r2,10,13,10
00000448 <DbgDisplayVars+1a8> subfic	r2,r9,20052
0000044c <DbgDisplayVars+1ac> addic.	r0,r0,21573
00000450 <DbgDisplayVars+1b0> rlwimi	r20,r26,4,1,3
00000454 <DbgDisplayVars+1b4> .long 0x4c4f4154
00000458 <DbgDisplayVars+1b8> addic	r8,r0,21573
0000045c <DbgDisplayVars+1bc> rlwimi	r20,r26,4,1,3
00000460 <DbgDisplayVars+1c0> .long 0x4c4f4154
00000464 <DbgDisplayVars+1c4> addic	r16,r0,21573
00000468 <DbgDisplayVars+1c8> rlwimi	r20,r26,4,1,3
0000046c <DbgDisplayVars+1cc> .long 0x4c4f4154
00000470 <DbgDisplayVars+1d0> addic	r24,r0,21573
00000474 <DbgDisplayVars+1d4> rlwimi	r20,r26,4,1,3
00000478 <DbgDisplayVars+1d8> .long 0x4c4f4154
0000047c <DbgDisplayVars+1dc> Address 0x47c is out of bounds.
