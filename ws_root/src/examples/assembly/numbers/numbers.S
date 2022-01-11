
/******************************************************************************
**
**  @(#) numbers.S 96/02/21 1.3
**
**  Example ppcas assembly code.  This code demonstrates
**  loops, macros, and simple FP math.
**
******************************************************************************/

	.include	"macros.i"

	.globl	main

	# lets create a symbol name for r31
	.equ	mycounter, 31

	.equ	NUMBER_OF_LOOPS,10

	.text
main:
	stwu	r1,-16(r1)
	mflr	r0
	# r30 amd r31 are nonvolatile registers so we must save them
	# Note that we cannot save anything at 0(sp) or 4(sp) because
	# those locations will be overwritten by the new SP and LR when
	# a function is called from by function.
	stw	r30,8(r1)
	stw	r31,12(r1)
	stw	r0,20(r1)

	#  put a pointer to the string into r30
	lea     r30,str1

	# we want to print "Hello World" NUMBER_OF_LOOPS
	# times, numbering each one.
	li	mycounter,NUMBER_OF_LOOPS

.Loop:	# local labels start with ".L"
	mr	r3,r30
	subfic	r4,mycounter,NUMBER_OF_LOOPS
        bl	printf
        addic.	mycounter,mycounter,-1
	bne	.Loop

	/*  now load pi into f1 */
	lis	r4,pi@ha
	lfs	f1,pi@l(r4)

	# print out pi
	lea	r3,pistr
	# PPC EABI spec:	The caller of a function that takes
	# a variable argument list shall set condition register bit 6 to 1
	# if it passes one or more arguments in the FP registers
	crset	6
	bl	printf


	# test the li32 macro
	lea	r3,str2
	li32	r4, 0x40001234
	li32	r5,0
	li32	r6,0xffff9000
	bl	printf

	lea	r3,str2
	li32	r4,0xdeadbeef
	li32	r5,0xF000
	li32	r6,-2
	bl	printf

	# finally some simple math
	# print the areas of circles with diameters from 1 to 10

	lea	r3,circstr1
	bl	printf

	li	mycounter,10
	lea	r30,circstr2

.Loop2:
	subfic	r3,mycounter,11
	bl	__itof		# c library function
	fmuls	f1,f1,f1	# r^2
	lis	r4,pi@ha
	lfs	f3,pi@l(r4)
	fmuls	f2,f1,f3	# pi * r^2
	mr	r3,r30		# string to print
	subfic	r4,mycounter,11	# radius
	crset	6
        bl	printf
        addic.	mycounter,mycounter,-1
	bne	.Loop2

	# restore stack
	lwz	r0,20(r1)
	mtlr	r0
	addi	sp,sp,0x10
	blr

	.data
str1:
        .asciz  "Hello World: %d\n"
str2:
	.asciz	"Values are 0x%x 0x%x 0x%x\n"
pistr:
	.asciz	"\npi = %f\n"
circstr1:
	.asciz	"\nr     r^2     pi*r^2\n"
circstr2:
	.asciz	"%2d  %f  %f\n"

	.align	2
pi:
	.float 	3.14159265358979







