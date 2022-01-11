/* @(#) debug.c 96/06/12 1.45 */

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/io.h>
#include <kernel/internalf.h>
#include <kernel/kernel.h>


/*****************************************************************************/


static bool suspendKprintf = FALSE;


/*****************************************************************************/


int externalMayGetChar(void) {

	return (*KB_FIELD(kb_MayGetChar))();
}


/*****************************************************************************/


static void OutputChar(char ch)
{
    if (ch < 32)
    {
        if ((ch < 0x07) && (ch > 0x0d))
        {
            /* if not bell, backspace, tab, newline, vtab, ff, or cr... */

            if (ch == KPRINTF_STOP)
                suspendKprintf = TRUE;
            else if (ch == KPRINTF_START)
                suspendKprintf = FALSE;

            return;
        }
    }

    if (suspendKprintf)
        return;

    (*KB_FIELD(kb_PutC))(ch);
}


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Debugging -name DebugPutChar
|||	Output a character to the debugging terminal.
|||
|||	  Synopsis
|||
|||	    void DebugPutChar(char *ch);
|||
|||	  Description
|||
|||	    This function outputs a character to the debugging terminal.
|||	    If there is no debugging terminal, this function has no effect.
|||
|||	  Arguments
|||
|||	    ch
|||	        The character to output to the debugging terminal.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/debug.h>, libc.a
|||
|||	  See Also
|||
|||	    DebugPutStr(), DebugBreakpoint()
|||
**/

void internalDebugPutChar(char ch) {

#ifndef	BUILD_DEBUGGER
	if (KB_FIELD(kb_CPUFlags) & KB_SERIALPORT) {
#endif	/* ifndef BUILD_DEBUGGER */

	OutputChar(ch);
	(*KB_FIELD(kb_PutC))(0); /* flush it out */

#ifndef	BUILD_DEBUGGER
   	}
#endif	/* ifndef BUILD_DEBUGGER */

}


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Debugging -name DebugPutStr
|||	Output a string to the debugging terminal.
|||
|||	  Synopsis
|||
|||	    void DebugPutStr(const char *str);
|||
|||	  Description
|||
|||	    This function outputs a string to the debugging terminal.
|||	    If there is no debugging terminal, this function has no effect.
|||
|||	  Arguments
|||
|||	    str
|||	        The string to output.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/debug.h>, libc.a
|||
|||	  See Also
|||
|||	    DebugPutChar(), DebugBreakpoint()
|||
**/

void internalDebugPutStr(const char *str) {

	char ch;

#ifndef	BUILD_DEBUGGER
	if (KB_FIELD(kb_CPUFlags) & KB_SERIALPORT) {
#endif	/* ifndef BUILD_DEBUGGER */

	while (ch = *str++) OutputChar(ch);
	(*KB_FIELD(kb_PutC))(0); /* flush it out */

#ifndef	BUILD_DEBUGGER
	}
#endif	/* ifndef BUILD_DEBUGGER */

}


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Debugging -name DebugBreakpoint
|||	Trigger a breakpoint-like event in the debugger.
|||
|||	  Synopsis
|||
|||	    void DebugBreakpoint(void);
|||
|||	  Description
|||
|||	    This function lets you tell the debugger to interrupt
|||	    execution of your task just like if a breakpoint was in
|||	    place. The debugger will then be in control, and normal
|||	    debugging activities can be performed, just like when a
|||	    breakpoint is encountered.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/debug.h> V27.
|||
|||	  Associated Files
|||
|||	    <kernel/debug.h>, libc.a
|||
|||	  See Also
|||
|||	    DebugPutStr(), DebugPutChar()
|||
**/
