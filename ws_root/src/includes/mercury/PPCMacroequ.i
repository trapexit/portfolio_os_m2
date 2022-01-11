	.ifndef __HARDWARE_PPCMACROEQU_I
	define __HARDWARE_PPCMACROEQU_I,1

/* @(#) PPCMacroequ.i 95/07/12 1.23 */

/* Macros of general interest */

/*****************************************************************************/

	/* shortcut for mtcrf */
	.macro	mtcr	reg
	mtcrf	0xff,\reg
	.endm

	/* load the effective address of a global symbol */
	.macro	lea	reg,addr
	lis	\reg,\addr@h
	ori	\reg,\reg,\addr@l
	.endm


/*****************************************************************************/


/* This macro is the prefered way of declaring a public function in asm.
 * Use this for public functions.
 */

	.macro	DECFN	fnname
	.type	\fnname,@function 
	.globl	\fnname
	.text
\fnname:
	/* Code starts here */
	.endm
/*****************************************************************************/

	.endif  /*__HARDWARE_PPCMACROEQU_I */
