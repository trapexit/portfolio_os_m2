	.globl  printf
	.globl	main
	
	.set	myreg, 3
	.equ	areg, 6
	.equ	areg, 7

# this macro loads a 32-bit constant into a register.
# It is smart enough to do the operation in a single instruction
# when possible.			

.macro li32    reg, const32bit
        .if     (\const32bit & 0xFFFF0000)
		.if     ((\const32bit & 0xFFFF8000) - 0xFFFF8000)
			lis     \reg,(((\const32bit)>>16) & 0xFFFF)
                        .if     (\const32bit & 0xFFFF)
                                ori     \reg,\reg,((\const32bit) & 0xFFFF)
                        .endif
                .else
	                li      \reg,(\const32bit & -1)
                .endif
        .else
                .if     (\const32bit & 0x8000)
                        li      \reg,0
                        ori     \reg,\reg,(\const32bit)
                .else
                        li      \reg,((\const32bit) & 0xFFFF)
                .endif
        .endif
.endm
			
	.text
main:
	STWU	R1,-16(sp)
	mflr	r0
	stw	r0,20(sp)

		
	#  put a pointer to the string into myreg
	lis     myreg,str1@h
        ori     myreg,myreg,str1@l

	bne	stop
	#  now load value into f1
.ifdef DO_PI
	lis	r4,pi@ha
	lfs	f1,pi@l(r4)
.else
	lis	r4,value@ha
	lfs	f1,value@l(r4)
.endif
	
stop:	
	bne	.Lstop
	# test the li32 macro
	li32	r7, 0x40001234
	li32	r8,0
	li32	r9,0xffff9000
	li32	areg,0xdeadbeef
	li32	r5,0xF000
	li32	r5,0x4000	

.Lstop:			
	# PPC EABI spec:	The caller of a function that takes
	# a variable argument list shall set condition register bit 6 to 1
	# if it passes one or more arguments in the FP registers
	crset		6
	
	bl	printf

	# restore stack
	lwz	r0,20(r1)
	mtlr	r0
	addi	sp,sp,0x10
	blr

str1:
        .asciz  "\nThe Value is %f\n\n"
	.align 2
	
bad:		
	.long	0xc0dedbad
value:	
	.float 	3.1415901184082031250 #	.long   0x40490fd0      

pi:	
	.float 	3.1415901184082031250 #	.long   0x40490fd0      







