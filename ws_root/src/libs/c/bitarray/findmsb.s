/* @(#) findmsb.s 95/09/03 1.11 */

#include <hardware/PPCMacroequ.i>

/**
|||	AUTODOC -public -class Kernel -group BitArrays -name FindMSB
|||	Finds the highest-numbered bit.
|||
|||	  Synopsis
|||
|||	    int32 FindMSB(uint32 mask);
|||
|||	  Description
|||
|||	    This function finds the highest-numbered bit that is set in the
|||	    argument. The least-significant bit is bit 0, and the
|||	    most-significant bit is bit 31.
|||
|||	  Arguments
|||
|||	    mask
|||	        The 32-bit word to check.
|||
|||	  Return Value
|||
|||	    Returns the number of the highest-numbered bit that is set
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
|||	    FindLSB()
**/


	DECFN	FindMSB
	li	r5,31
	cntlzw	r4,r3
	sub	r3,r5,r4
	blr
