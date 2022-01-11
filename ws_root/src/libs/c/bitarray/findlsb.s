/* @(#) findlsb.s 95/09/05 1.3 */

#include <hardware/PPCMacroequ.i>

/**
|||	AUTODOC -public -class Kernel -group BitArrays -name FindLSB
|||	Finds the least-significant bit.
|||
|||	  Synopsis
|||
|||	    int32 FindLSB(uint32 mask);
|||
|||	  Description
|||
|||	    FindLSB() finds the lowest-numbered bit that is set in the argument.
|||	    The least-significant bit is bit 0, and the most-significant
|||	    bit is bit 31.
|||
|||	  Arguments
|||
|||	    mask
|||	        The 32-bit word to check.
|||
|||	  Return Value
|||
|||	    Returns the number of the lowest-numbered bit that is set
|||	    in the argument, or -1 if no bits are set.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V20.
|||
|||	  Associated Files
|||
|||	    <kernel/bitarray.h>, libc.a
|||
|||	  See Also
|||
|||	    FindMSB()
**/


	DECFN	FindLSB
	addi	r5,r3,-1
	andc	r3,r3,r5	// r3 = A & ~(A-1)  --  get the LSB mask
	li	r5,31
	cntlzw	r4,r3
	sub	r3,r5,r4
	blr
