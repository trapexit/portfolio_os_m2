#ifndef __HARDWARE_PPCMACROEQU_I
#define __HARDWARE_PPCMACROEQU_I


/******************************************************************************
**
**  @(#) PPCMacroequ.i 96/02/20 1.24
**
**  Macros of general interest
**
******************************************************************************/


// This macro is used to load a 32bit constant into a register.
// It will use only a single instruction where possible.
//
// inputs - &reg:       register to load
//       - &const32bit: constant to load

        .macro
        li32    &reg,&const32bit
        .if     (&const32bit & 0xFFFF0000)
                .if     ((&const32bit & 0xFFFF8000) == 0xFFFF8000)
                        li      &reg,((&const32bit) & 0xFFFF)
                .else
                        lis     &reg,(((&const32bit)>>16) & 0xFFFF)
                        .if     (&const32bit & 0xFFFF)
                                ori     &reg,&reg,((&const32bit) & 0xFFFF)
                        .endc
                .endc
        .else
                .if     (&const32bit & 0x8000)
                        li      &reg,0
                        ori     &reg,&reg,(&const32bit)
                .else
                        li      &reg,((&const32bit) & 0xFFFF)
                .endc
        .endc
        .endm

	/* the esa instruction */
	.macro
	esa
	.long	0x7c0004a8
	.endm

	/* the dsa instruction */
	.macro
	dsa
	.long	0x7c0004e8
	.endm

	/* mfsprg shortcut */
	.macro
	mfsprg	&reg,&spreg
	mfspr	&reg,sprg&spreg
	.endm

	/* shortcut for mtcrf */
	.macro
	mtcr	&reg
	mtcrf	0xff,&reg
	.endm

	/* load the effective address of a global symbol */
	.macro
	lea	&reg,&addr
	lis	&reg,&addr@h
	ori	&reg,&reg,&addr@l
	.endm


/*****************************************************************************/


/* This macro is the prefered way of declaring a public function in asm.
 * Use this for public functions.
 */

	.macro
	DECFN	&fnname
	.type	&fnname,@function
	.globl	&fnname
	.text
&fnname:
	/* Code starts here */
	.endm


/*****************************************************************************/


/* These macros are used to generate varargs stubs for TagArg functions.
 * The tricky part is in converting the varargs-style stack frame into an
 * array of TagArg structures suitable for the tag parsing functions.
 *
 * To generate a stub, you must first chose which macro is right for you.
 * This is a simple process. Count the number of non-varargs arguments that
 * the function being stubbed has. You then use the macro that has that number
 * as a suffix on its macro name. For example, CreateItemVA() is defined
 * as:
 *	Item CreateItemVA(int32 cntype, uint32 tags, ...);
 *
 * This function only has a single non-varargs parameter. Therefore, the right
 * macro to use if DECVASTUB_1. The macro would be used like:
 *
 *	DECVASTUB_1 CreateItem
 */

	.macro
	DECVASTUB_0	&fnname
	.globl	&fnname%%VA
	.text
&fnname%%VA:
	lea	r0,&fnname
	b	__tagCall_0
	.endm

	.macro
	DECVASTUB_1	&fnname
	.globl	&fnname%%VA
	.text
&fnname%%VA:
	lea	r0,&fnname
	b	__tagCall_1
	.endm

	.macro
	DECVASTUB_2	&fnname
	.globl	&fnname%%VA
	.text
&fnname%%VA:
	lea	r0,&fnname
	b	__tagCall_2
	.endm

	.macro
	DECVASTUB_3	&fnname
	.globl	&fnname%%VA
	.text
&fnname%%VA:
	lea	r0,&fnname
	b	__tagCall_3
	.endm

	.macro
	DECVASTUB_4	&fnname
	.globl	&fnname%%VA
	.text
&fnname%%VA:
	lea	r0,&fnname
	b	__tagCall_4
	.endm



/*****************************************************************************/


/* This macro is used to stop the processor.  it will cause the cpu
 * to spin in an infinite loop, essentially stopping the processor
 */

	.macro
	stop
	b	$
	.endm


/*****************************************************************************/


#endif /* __HARDWARE_PPCMACROEQU_I */
