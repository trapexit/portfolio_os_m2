/* @(#) atomicbits.s 95/06/08 1.7 */

#include <hardware/PPCMacroequ.i>


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group BitArrays -name AtomicClearBits
|||	Clears bit in a word of memory in an atomic manner.
|||
|||	  Synopsis
|||
|||	    uint32 AtomicClearBits(uint32 *addr, uint32 bits);
|||
|||	  Desription
|||
|||	    This function clears the specified bits from the given
|||	    word of memory in an atomic manner. Essentially, you are
|||	    guaranteed that the write operation will be performed
|||	    without interference from other tasks. This is a very
|||	    efficient replacement for many common uses of semaphores.
|||
|||	  Arguments
|||
|||	    addr
|||	        The address of the word of memory to modify.
|||
|||	    bits
|||	        The bits in the word of memory that should be cleared.
|||
|||	  Return Value
|||
|||	    Returns the previous contents of the word of memory.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V27.
|||
|||	  Associated Files
|||
|||	    <kernel/bitarray.h>, libc.a
|||
|||	  See Also
|||
|||	    AtomicSetBits()
**/

	DECFN	AtomicClearBits

	lwarx	r0,0,r3			// read word at specified address
	andc	r4,r0,r4		// r4 = r0 & ~r4;
	stwcx.	r4,0,r3			// store word back
	bne	AtomicClearBits		// if it didn't work, try again
	mr	r3,r0			// return previous contents
	blr


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group BitArrays -name AtomicSetBits
|||	Sets bit in a word of memory in an atomic manner.
|||
|||	  Synopsis
|||
|||	    uint32 AtomicSetBits(uint32 *addr, uint32 bits);
|||
|||	  Desription
|||
|||	    This function sets the specified bits in the given
|||	    word of memory in an atomic manner. Essentially, you are
|||	    guaranteed that the write operation will be performed
|||	    without interference from other tasks. This is a very
|||	    efficient replacement for many common uses of semaphores.
|||
|||	  Arguments
|||
|||	    addr
|||	        The address of the word of memory to modify.
|||
|||	    bits
|||	        The bits in the word of memory that should be set.
|||
|||	  Return Value
|||
|||	    Returns the previous contents of the word of memory.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V27.
|||
|||	  Associated Files
|||
|||	    <kernel/bitarray.h>, libc.a
|||
|||	  See Also
|||
|||	    AtomicClearBits()
**/

	DECFN	AtomicSetBits

	lwarx	r0,0,r3			// read word at specified address
	or	r4,r0,r4		// r4 = r0 | r4;
	stwcx.	r4,0,r3			// store word back
	bne	AtomicSetBits		// if it didn't work, try again
	mr	r3,r0			// return previous contents
	blr
