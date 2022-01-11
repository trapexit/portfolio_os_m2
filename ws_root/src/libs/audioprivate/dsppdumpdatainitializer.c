/* @(#) dsppdumpdatainitializer.c 96/02/16 1.2 */

#include <audio/dspp_template.h>    /* self */
#include <audio/handy_macros.h>
#include <stdio.h>

/**
|||	AUTODOC -private -class audio -group DSPPTemplate -name dsppDumpDataInitializer
|||	Dumps DSPPDataInitializer record to debugging terminal.
|||
|||	  Synopsis
|||
|||	    void dsppDumpDataInitializer (const DSPPDataInitializer *dini)
|||
|||	  Description
|||
|||	    Dumps DSPPDataInitializer record to debugging terminal.
|||
|||	  Arguments
|||
|||	    dini
|||	        DSPPDataInitializer record to display. Assumed to be valid and non-NULL.
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
|||	    dsppDumpRelocation(), dsppDumpResource(), dsppDumpTemplate()
**/
void dsppDumpDataInitializer (const DSPPDataInitializer *dini)
{
    const int32 * const dataptr = dsppGetDataInitializerImage(dini);
    int idata = 0;

    printf ("%4d: size=0x%04x flags=0x%02x\n", dini->dini_RsrcIndex, dini->dini_Many, dini->dini_Flags);

    while (idata < dini->dini_Many) {
        char b[128], *bptr = b;
        int nlinedata = MIN (dini->dini_Many - idata, 4);

        bptr += sprintf (bptr, "      0x%04x:", idata);

        for (; nlinedata--; idata++) {
            bptr += sprintf (bptr, " 0x%08x", dataptr[idata]);
        }

        printf ("%s\n", b);
    }
}
