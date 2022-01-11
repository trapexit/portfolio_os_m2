
/******************************************************************************
**
**  @(#) hello.S 96/02/21 1.2
**
**  This is the assembly language Hello World program.
**  The only function it calls is printf()
**
******************************************************************************/

	.globl	main

	.text
main:
	# r1 is always the stack pointer.
	# the assembler recognizes "sp" as an alias for "r1"

	# The stack pointer always maintains 16-byte alignment.

	# For this program we need only a minimal stack frame
	# so we can save the SP and LR
	stwu	r1,-16(r1)
	mflr	r0
	stw	r0,20(r1)

	#  put a pointer to the string into r3
	lis     r3,str1@h
        ori     r3,r3,str1@l

	# PPC EABI spec:	The caller of a function that takes
	# a variable argument list shall set condition register bit 6 to 1
	# if it passes one or more arguments in the FP registers.
	crclr	6
	bl	printf

	# restore stack
	lwz	r0,20(r1)
	mtlr	r0
	addi	sp,sp,0x10
	blr

str1:
        .asciz  "\nHello World\n\n"
