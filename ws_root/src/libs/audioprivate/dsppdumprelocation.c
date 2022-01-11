/* @(#) dsppdumprelocation.c 96/02/16 1.2 */

#include <audio/dspp_template.h>    /* self */
#include <stdio.h>

/**
|||	AUTODOC -private -class audio -group DSPPTemplate -name dsppDumpRelocation
|||	Dumps DSPPRelocation to debugging terminal.
|||
|||	  Synopsis
|||
|||	    void dsppDumpRelocation (const DSPPRelocation *drlc)
|||
|||	  Description
|||
|||	    Dumps DSPPRelocation to debugging terminal.
|||
|||	  Arguments
|||
|||	    drlc
|||	        DSPPRelocation to display.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libaudioprivate.a V30.
|||
|||	  Associated Files
|||
|||	    <audio/dspp_template>, libaudioprivate.a
|||
|||	  See Also
|||
|||	    dsppDumpDataInitializer(), dsppDumpResource(), dsppDumpTemplate()
**/
void dsppDumpRelocation (const DSPPRelocation *drlc)
{
    printf ("rsrc[%2d]+0x%04x hunk[%d]+0x%04x\n",
        drlc->drlc_RsrcIndex, drlc->drlc_Part,
        drlc->drlc_CodeHunk, drlc->drlc_CodeOffset);
}
