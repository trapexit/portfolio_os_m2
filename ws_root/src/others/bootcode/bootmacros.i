/* @(#) bootmacros.i 96/05/03 1.10 */

/********************************************************************************
*
*	bootmacros.i
*
*	This file contains assembly language macros used in the bootcode.
*
********************************************************************************/

#include <hardware/PPCequ.i>


/********************************************************************************
*
*	DECVAR
*
*	This macro declares a one word long global variable and initializes it
*	to zero.
*
*	Input:  varname = Name of global variable
*	Output: None
*	Calls:	None
*
********************************************************************************/

	.macro
	DECVAR	&varname

	.globl	&varname			// Declare varname as a global
&varname:					// Create varname symbol
	.long	0				// Initialize to zero
	.endm


/********************************************************************************
*
*	stack
*
*	This macro stores information about the current context on a stack
*	so that subroutines may be called.  The unstack macro is used to
*	return to the original context.
*
*	Currently this is a hack, since it is used in situations where there
*	isn't really any memory available for a real stack.  All it does is
*	save the link register in r12.
*
*	Input:  None
*	Output: None
*	Calls:	None
*
********************************************************************************/

	.macro
	stack

	mfspr	r12,lr				// Save link register to r12
	.endm


/********************************************************************************
*
*	unstack
*
*	This macro retrieves a context previously stored using the stack macro.
*
*	Currently this is a hack, since it is used in situations where there
*	isn't really any memory available for a real stack.  All it does is
*	restore the link register from r12.
*
*	Input:  None
*	Output: None
*	Calls:	None
*
********************************************************************************/

	.macro
	unstack

	mtspr	lr,r12				// Restore link register from r12
	.endm


/********************************************************************************
*
*	ViaIBR
*
*	This macro is a generic exception handler used to build the ROM's
*	exception vector table.  It simply redirects the CPU to a corresponding
*	vector in RAM.  In doing so it uses r3 and the counter as scratch
*	registers, so it first saves them to special purpose registers 1 and
*	2, then jumps to a point just before the actual RAM handler, where a
*	piece of patch code restores r3 and the counter.
*
*	Input:  offset = Offset of vector from base of table
*	Output: None
*	Calls:	None
*
********************************************************************************/

	.macro
	ViaIBR	&offset

	mtsprg1	r3				// Store r3 to SPRG1 so we can use it
	mfctr	r3				// Get current value of counter
	mtsprg2	r3				// Store to SPRG2 so we can use counter too
	mfspr	r3,IBR				// Get current value of IBR
	addi	r3,r3,&offset-12		// Calculate address of patch code
	mtctr	r3				// Put address in counter
	bctr					// Branch via counter to patch code
	.endm


