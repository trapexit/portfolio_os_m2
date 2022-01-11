#$$m2		- PowerPC mnemonics
#$$pPPC		- PowerPC instructions
#$$oPPC602	- Optimized for PowerPC 602
#$$ko 0		- Reorder info
	.file		"debug.c"
# -X options:
#$$x X7=2 X8=1 X9=3 X12=1 X15=2 X16=20 X18=1 X19=5
#$$x X20=1000 X23=1 X25=2 X26=1 X29=1 X32=1 X36=1 X39=602
#$$x X57=2 X66=1 X70=2 X74=1 X75=1 X80=3 X83=2 X84=-17
#$$x X86=-1 X93=1 X95=29425664 X96=3 X99=6 X201=256
	.text

#$$bf
	.align		2
	.globl		DbgAdd
DbgAdd:
#$$ee
	cmpi		0,0,r3,0
	bc		4,2,.L4	# ne
	addis		r3,r0,head@ha
	addi		r3,r3,head@l
.L4:
	lwz		r6,0(r3)
	cmpi		0,0,r6,0
	bc		4,2,.L5	# ne
	stw		r4,0(r3)
	stw		r4,16(r4)
	stw		r4,20(r4)
	b		.L3
.L5:
	lwz		r5,20(r6)
	stw		r5,20(r4)
	stw		r4,16(r5)
	lwz		r11,0(r3)
	stw		r11,16(r4)
	stw		r4,20(r6)
.L3:
#$$be
	blr
#$$ef
	.type		DbgAdd,@function
	.size		DbgAdd,.-DbgAdd

# Allocations for DbgAdd
#	r3		top
#	r4		node
#	r5		$$1
#	r6		$$2
#	not allocated	tmp

#$$bf
	.align		2
	.globl		DbgRemove
DbgRemove:
#$$ee
	cmpi		0,0,r3,0
	bc		4,2,.L8	# ne
	addis		r3,r0,head@ha
	addi		r3,r3,head@l
.L8:
	lwz		r5,16(r4)
	cmpl		0,0,r5,r4
	bc		4,2,.L9	# ne
	addi		r12,r0,0
	stw		r12,0(r3)
	b		.L10
.L9:
	lwz		r12,20(r4)
	stw		r5,16(r12)
	lwz		r11,16(r4)
	lwz		r10,20(r4)
	stw		r10,20(r11)
.L10:
	lwz		r12,0(r3)
	cmpl		0,0,r12,r4
	bc		4,2,.L7	# ne
	lwz		r12,16(r4)
	stw		r12,0(r3)
.L7:
#$$be
	blr
#$$ef
	.type		DbgRemove,@function
	.size		DbgRemove,.-DbgRemove

# Allocations for DbgRemove
#	r3		top
#	r4		node
#	r5		$$3

#$$bf
	.align		2
	.globl		DbgIncrement
DbgIncrement:
	stwu		r1,-24(r1)
#$$ee
	lwz		r4,4(r3)
	cmpi		0,0,r4,1
	bc		4,2,.L14	# ne
	lwz		r4,0(r3)
	lwz		r11,0(r4)
	lfs		f13,8(r3)
	fctiwz		f0,f13
	addi		r0,r0,8
	stfiwx		f0,r1,r0
	lwz		r10,8(r1)
	add		r11,r11,r10
	stw		r11,0(r4)
	b		.L12
.L14:
	cmpi		0,0,r4,2
	bc		4,2,.L12	# ne
	lwz		r4,0(r3)
	lfs		f13,0(r4)
	lfs		f12,8(r3)
	fadds		f13,f13,f12
	stfs		f13,0(r4)
.L12:
#$$be
	addi		r1,r1,24
	blr
#$$ef
	.type		DbgIncrement,@function
	.size		DbgIncrement,.-DbgIncrement

# Allocations for DbgIncrement
#	r3		node
#	r4		$$4
#	r4		vari
#	r4		varf

#$$bf
	.align		2
	.globl		DbgDecrement
DbgDecrement:
	stwu		r1,-24(r1)
#$$ee
	lwz		r4,4(r3)
	cmpi		0,0,r4,1
	bc		4,2,.L18	# ne
	lwz		r4,0(r3)
	lwz		r11,0(r4)
	lfs		f13,8(r3)
	fctiwz		f0,f13
	addi		r0,r0,8
	stfiwx		f0,r1,r0
	lwz		r10,8(r1)
	subf		r11,r10,r11
	stw		r11,0(r4)
	b		.L16
.L18:
	cmpi		0,0,r4,2
	bc		4,2,.L16	# ne
	lwz		r4,0(r3)
	lfs		f13,0(r4)
	lfs		f12,8(r3)
	fsubs		f13,f13,f12
	stfs		f13,0(r4)
.L16:
#$$be
	addi		r1,r1,24
	blr
#$$ef
	.type		DbgDecrement,@function
	.size		DbgDecrement,.-DbgDecrement

# Allocations for DbgDecrement
#	r3		node
#	r4		$$5
#	r4		vari
#	r4		varf

#$$bf
	.align		2
	.globl		DbgPrintVar
DbgPrintVar:
	stwu		r1,-72(r1)
	mfspr		r0,8
#$$br
	stfs		f30,64(r1)
	stfs		f31,68(r1)
#$$er
#$$br
	stw		r31,60(r1)
#$$er
	stw		r0,76(r1)
#$$ee
	addi		r31,r3,0
	fmr		f31,f1
	fmr		f30,f2
	lwz		r6,4(r31)
	cmpi		0,0,r6,1
	bc		4,2,.L22	# ne
	lwz		r6,0(r31)
	lwz		r5,12(r31)
	addi		r3,r1,16
	addis		r4,r0,.L64@ha
	addi		r4,r4,.L64@l
	lwz		r6,0(r6)
	crxor		6,6,6
#$$fn 0x1ffa 0x3ffe
	bl		sprintf
	b		.L23
.L22:
	cmpi		0,0,r6,2
	bc		4,2,.L23	# ne
	lwz		r6,0(r31)
	lwz		r5,12(r31)
	addi		r3,r1,16
	addis		r4,r0,.L65@ha
	addi		r4,r4,.L65@l
	lfs		f1,0(r6)
	creqv		6,6,6
#$$fn 0x1ffa 0x3ffe
	bl		sprintf
.L23:
	addis		r31,r0,gdp@ha
	lwz		r31,gdp@l(r31)
	lwz		r3,0(r31)
	addi		r4,r31,44
	addi		r5,r1,16
	fmr		f1,f31
	fmr		f2,f30
#$$fn 0x1ffa 0x3ffe
	bl		TwoD_DrawText
#$$be
#$$br
	lwz		r31,60(r1)
#$$er
#$$br
	lfs		f30,64(r1)
	lfs		f31,68(r1)
#$$er
	lwz		r0,76(r1)
	mtspr		8,r0
	addi		r1,r1,72
	blr
#$$ef
	.type		DbgPrintVar,@function
	.size		DbgPrintVar,.-DbgPrintVar

# Allocations for DbgPrintVar
#	r31		node
#	f31		x
#	f30		y
#	SP,16		str
#	r6		$$6
#	r31		$$7
#	r6		vari
#	r6		varf

#$$bf
	.align		2
	.globl		DbgInitVars
DbgInitVars:
	stwu		r1,-24(r1)
	mfspr		r0,8
	stw		r0,28(r1)
#$$ee
	addis		r4,r0,di1@ha
	addi		r4,r4,di1@l
	addis		r11,r0,vi1@ha
	addi		r11,r11,vi1@l
	stw		r11,0(r4)
	addi		r10,r0,1
	stw		r10,4(r4)
	addis		r9,r0,.L67@ha
	lfs		f13,.L67@l(r9)
	stfs		f13,8(r4)
	addis		r12,r0,i1@ha
	lwz		r12,i1@l(r12)
	stw		r12,12(r4)
	addi		r3,r0,0
#$$fn 0x878 0x0
	bl		DbgAdd
	addis		r4,r0,df1@ha
	addi		r4,r4,df1@l
	addis		r9,r0,vf1@ha
	addi		r9,r9,vf1@l
	stw		r9,0(r4)
	addi		r12,r0,2
	stw		r12,4(r4)
	addis		r11,r0,.L68@ha
	lfs		f12,.L68@l(r11)
	stfs		f12,8(r4)
	addis		r10,r0,f1@ha
	lwz		r10,f1@l(r10)
	stw		r10,12(r4)
	addi		r3,r0,0
#$$fn 0x878 0x0
	bl		DbgAdd
#$$be
	lwz		r0,28(r1)
	mtspr		8,r0
	addi		r1,r1,24
	blr
#$$ef
	.type		DbgInitVars,@function
	.size		DbgInitVars,.-DbgInitVars

# Allocations for DbgInitVars
#	r4		$$8
#	r4		$$9

#$$bf
	.align		2
	.globl		DbgDisplayVars
DbgDisplayVars:
	stwu		r1,-104(r1)
	mfspr		r0,8
#$$br
	stw		r28,88(r1)
	stw		r29,92(r1)
	stw		r30,96(r1)
	stw		r31,100(r1)
#$$er
	stw		r0,108(r1)
#$$ee
	addis		r28,r0,head@ha
	addi		r28,r28,head@l
	lwz		r11,0(r28)
	cmpi		0,0,r11,0
	bc		12,2,.L27	# eq
	addis		r12,r0,gdp@ha
	lwz		r12,gdp@l(r12)
	lwz		r3,28(r12)
	addi		r5,r1,16
	addi		r4,r0,0
#$$fn 0x1ffa 0x3ffe
	bl		GetCPortData
	lwz		r31,16(r1)
	cmpi		0,0,r31,0
	bc		4,2,.L29	# ne
	addis		r12,r0,.L26@ha
	addi		r11,r0,20
	stw		r11,.L26@l(r12)
	b		.L27
.L29:
	addis		r29,r0,.L26@ha
	addi		r29,r29,.L26@l
	lwz		r11,0(r29)
	addi		r30,r11,-1
	stw		r30,0(r29)
	cmpi		0,0,r30,19
	bc		12,2,.L31	# eq
	cmpi		0,0,r30,0
	bc		12,1,.L27	# gt
	addi		r12,r0,5
	stw		r12,0(r29)
.L31:
	addis		r12,r0,32768
	cmpl		0,0,r31,r12
	bc		4,2,.L37	# ne
	lwz		r12,0(r28)
	lwz		r12,16(r12)
	stw		r12,0(r28)
	b		.L27
.L37:
	addis		r12,r0,16384
	cmpl		0,0,r31,r12
	bc		4,2,.L35	# ne
	lwz		r12,0(r28)
	lwz		r12,20(r12)
	stw		r12,0(r28)
	b		.L27
.L35:
	addis		r12,r0,4096
	cmpl		0,0,r31,r12
	bc		4,2,.L33	# ne
	lwz		r3,0(r28)
#$$fn 0xc1a 0x3000
	bl		DbgDecrement
	b		.L27
.L33:
	addis		r12,r0,8192
	cmpl		0,0,r31,r12
	bc		4,2,.L27	# ne
	lwz		r3,0(r28)
#$$fn 0xc1a 0x3000
	bl		DbgIncrement
.L27:
	lwz		r30,0(r28)
	addi		r29,r0,200
	addi		r31,r0,6
.L42:
	lwz		r12,0(r28)
	cmpl		0,0,r12,r30
	bc		4,2,.L41	# ne
	subfic		r3,r31,6
	cmpi		0,0,r3,0
	bc		12,1,.L86	# gt
.L41:
	addi		r3,r29,0
#$$fn 0x1ffa 0x3ffe
	bl		__itof
	fmr		f2,f1
	addi		r3,r30,0
	addis		r12,r0,.L87@ha
	lfs		f1,.L87@l(r12)
#$$fn 0x1ffa 0x3ffe
	bl		DbgPrintVar
	addi		r29,r29,30
	addi		r31,r31,-1
	lwz		r30,16(r30)
	cmpi		0,0,r31,0
	bc		4,2,.L42	# ne
.L86:
#$$be
#$$br
	lwz		r28,88(r1)
	lwz		r29,92(r1)
	lwz		r30,96(r1)
	lwz		r31,100(r1)
#$$er
	lwz		r0,108(r1)
	mtspr		8,r0
	addi		r1,r1,104
	blr
#$$ef
	.type		DbgDisplayVars,@function
	.size		DbgDisplayVars,.-DbgDisplayVars

# Allocations for DbgDisplayVars
#	SP,16		input
	.data
	.align		2
#	static		cnt
.L26:
	.long		20
#	r31		i
#	not allocated	start
#	r30		tmp
#	r29		$$10
#	r28		$$11
#	r31		$$12
#	r30		$$13
#	r3		$$14
#	r28		$$15
#	r29		$$16

# Allocations for module
	.type		head,@object
	.size		head,4
	.align		2
	.globl		head
head:
	.long		0
	.globl		di1
	.lcomm		di1,24
	.globl		di2
	.lcomm		di2,24
	.globl		di3
	.lcomm		di3,24
	.globl		di4
	.lcomm		di4,24
	.globl		df1
	.lcomm		df1,24
	.globl		df2
	.lcomm		df2,24
	.globl		df3
	.lcomm		df3,24
	.globl		df4
	.lcomm		df4,24
	.type		vi1,@object
	.size		vi1,4
	.align		2
	.globl		vi1
vi1:
	.long		10
	.type		vi2,@object
	.size		vi2,4
	.align		2
	.globl		vi2
vi2:
	.long		100
	.type		vi3,@object
	.size		vi3,4
	.align		2
	.globl		vi3
vi3:
	.long		1000
	.type		vi4,@object
	.size		vi4,4
	.align		2
	.globl		vi4
vi4:
	.long		10000
	.type		vf1,@object
	.size		vf1,4
	.align		2
	.globl		vf1
vf1:
	.long		0x3f000000	# +5.0000000000000000000e-1
	.type		vf2,@object
	.size		vf2,4
	.align		2
	.globl		vf2
vf2:
	.long		0x40a00000	# +5.0000000000000000000
	.type		vf3,@object
	.size		vf3,4
	.align		2
	.globl		vf3
vf3:
	.long		0x42480000	# +5.0000000000000000000e1
	.type		vf4,@object
	.size		vf4,4
	.align		2
	.globl		vf4
vf4:
	.long		0x43fa0000	# +5.0000000000000000000e2
	.type		i1,@object
	.size		i1,4
	.align		2
	.globl		i1
i1:
	.long		.L88
	.type		i2,@object
	.size		i2,4
	.align		2
	.globl		i2
i2:
	.long		.L89
	.type		i3,@object
	.size		i3,4
	.align		2
	.globl		i3
i3:
	.long		.L90
	.type		i4,@object
	.size		i4,4
	.align		2
	.globl		i4
i4:
	.long		.L91
	.type		f1,@object
	.size		f1,4
	.align		2
	.globl		f1
f1:
	.long		.L92
	.type		f2,@object
	.size		f2,4
	.align		2
	.globl		f2
f2:
	.long		.L93
	.type		f3,@object
	.size		f3,4
	.align		2
	.globl		f3
f3:
	.long		.L94
	.type		f4,@object
	.size		f4,4
	.align		2
	.globl		f4
f4:
	.long		.L95
	.text
	.type		.L67,@object
	.size		.L67,4
	.align		2
#	static		FLOAT_TEMP
.L67:
	.long		0x3f800000	# +1.0000000000000000000
	.type		.L68,@object
	.size		.L68,4
	.align		2
#	static		FLOAT_TEMP
.L68:
	.long		0x3ccccccd	# +2.5000000372529029846e-2
	.type		.L87,@object
	.size		.L87,4
	.align		2
#	static		FLOAT_TEMP
.L87:
	.long		0x42200000	# +4.0000000000000000000e1
	.align		0
.L64:
	.byte		37,115,32,61,32,37,100,0
	.byte		0
	.align		0
.L65:
	.byte		37,115,32,61,32,37,102,0
	.byte		0
	.align		0
.L88:
	.byte		84,69,83,84,32,73,78,84,49
	.byte		0
	.align		0
.L89:
	.byte		84,69,83,84,32,73,78,84,50
	.byte		0
	.align		0
.L90:
	.byte		84,69,83,84,32,73,78,84,51
	.byte		0
	.align		0
.L91:
	.byte		84,69,83,84,32,73,78,84,52
	.byte		0
	.align		0
.L92:
	.byte		84,69,83,84,32,70,76,79,65,84,49
	.byte		0
	.align		0
.L93:
	.byte		84,69,83,84,32,70,76,79,65,84,50
	.byte		0
	.align		0
.L94:
	.byte		84,69,83,84,32,70,76,79,65,84,51
	.byte		0
	.align		0
.L95:
	.byte		84,69,83,84,32,70,76,79,65,84,52
	.byte		0
