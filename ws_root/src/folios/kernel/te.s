/* @(#) te.s 96/07/31 1.2 */

#include <hardware/PPCMacroequ.i>
#include <hardware/bda.i>


/*****************************************************************************/


/**
|||	AUTODOC -class GraphicsFolio -group TE -name GetTEWritePointer
|||	Gets the current value of the triangle engine instruction write pointer.
|||
|||	  Synopsis
|||
|||	    void *GetTEWritePointer(void);
|||
|||	  Description
|||
|||	    This function returns the current value of the triangle engine
|||	    instruction write pointer. This indicates the stop at which the
|||	    next triangle engine command is to be written by the CPU. The
|||	    triangle engine automatically stop executing when its
|||	    instruction read pointer equals the value of the write pointer.
|||
|||	  Return Values
|||
|||	    Returns the current contents of the triangle engine instruction
|||	    write pointer.
|||
|||	  Associated Files
|||
|||	    <device/te.h>
|||
|||	  See Also
|||
|||	    SetTEWritePointer(), GetTEReadPointer(), SetTEReadPointer()
|||
**/

	DECFN	GetTEWritePointer
	esa
	lis	r4,0x0004
	lwz	r3,0x0018(r4)
	dsa
	blr


/*****************************************************************************/


/**
|||	AUTODOC -class GraphicsFolio -group TE -name SetTEWritePointer
|||	Sets the value of the triangle engine instruction write pointer.
|||
|||	  Synopsis
|||
|||	    void SetTEWritePointer(void *ptr);
|||
|||	  Description
|||
|||	    This function sets the value of the triangle engine
|||	    instruction write pointer. This indicates the stop at which the
|||	    next triangle engine command is to be written by the CPU. The
|||	    triangle engine automatically stop executing when its
|||	    instruction read pointer equals the value of the write pointer.
|||
|||	  Argument
|||
|||	    ptr
|||	        The address to put in the triangle engine instruction write
|||	        pointer, which determines where the triangle engine stops
|||	        executing commands.
|||
|||	  Associated Files
|||
|||	    <device/te.h>
|||
|||	  See Also
|||
|||	    GetTEWritePointer(), GetTEReadPointer(), SetTEReadPointer()
|||
**/

	DECFN	SetTEWritePointer
	esa
	lis	r4,0x0004
	stw	r3,0x0018(r4)
	dsa
	blr


/*****************************************************************************/


/**
|||	AUTODOC -class GraphicsFolio -group TE -name GetTEReadPointer
|||	Gets the current value of the triangle engine instruction read pointer.
|||
|||	  Synopsis
|||
|||	    void *GetTEReadPointer(void);
|||
|||	  Description
|||
|||	    This function returns the current value of the triangle engine
|||	    instruction read pointer. This indicates the next instruction that
|||	    the triangle engine will execute.
|||
|||	  Return Values
|||
|||	    Returns the current contents of the triangle engine instruction
|||	    read pointer.
|||
|||	  Associated Files
|||
|||	    <device/te.h>
|||
|||	  See Also
|||
|||	    GetTEWritePointer(), SetTEWritePointer(), SetTEReadPointer()
|||
**/

	DECFN	GetTEReadPointer
	esa
	lis	r4,0x0004
	lwz	r3,0x001c(r4)
	dsa
	blr


/*****************************************************************************/


/**
|||	AUTODOC -class GraphicsFolio -group TE -name SetTEReadPointer
|||	Sets the value of the triangle engine instruction read pointer.
|||
|||	  Synopsis
|||
|||	    void SetTEReadPointer(void *ptr);
|||
|||	  Description
|||
|||	    This function sets the value of the triangle engine
|||	    instruction read pointer.
|||
|||	  Argument
|||
|||	    ptr
|||	        The address to put in the triangle engine instruction read
|||	        pointer.
|||
|||	  Associated Files
|||
|||	    <device/te.h>
|||
|||	  See Also
|||
|||	    GetTEWritePointer(), GetTEReadPointer(), GetTEReadPointer()
|||
**/

	DECFN	SetTEReadPointer
	esa
	lis	r4,0x0004
	stw	r3,0x001c(r4)
	dsa
	blr
