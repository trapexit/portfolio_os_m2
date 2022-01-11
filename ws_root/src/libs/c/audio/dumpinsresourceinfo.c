/* @(#) dumpinsresourceinfo.c 96/01/13 1.1 */

#include <audio/audio.h>
#include <stdio.h>

/**
|||	AUTODOC -public -class Audio -group Instrument -name DumpInstrumentResourceInfo
|||	Prints out instrument resource usage information.
|||
|||	  Synopsis
|||
|||	    void DumpInstrumentResourceInfo (Item insOrTemplate, const char *banner)
|||
|||	  Description
|||
|||	    Prints all resource usage information for the specified instrument or
|||	    template to the debug console.
|||
|||	  Arguments
|||
|||	    insOrTemplate
|||	        Instrument or Template to print information for.
|||
|||	    banner
|||	        Description of print out. Can be NULL.
|||
|||	  Implementation
|||
|||	    Library call implemented in libc.a V30.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>, libc.a
|||
|||	  See Also
|||
|||	    GetInstrumentResourceInfo()
**/

void DumpInstrumentResourceInfo (Item insOrTemplate, const char *banner)
{
    static const char * const rsrcdesc [AF_RESOURCE_TYPE_MANY] = {  /* @@@ depends on AF_RESOURCE_TYPE_ order */
        "Ticks",
        "Code",
        "Data",
        "FIFOs",
        "Triggers",
    };
    int32 i;
    InstrumentResourceInfo rinfo;
    bool comma = FALSE;

    if (banner) printf ("%s:", banner);
    for (i=0; i<AF_RESOURCE_TYPE_MANY; i++) {
        if (GetInstrumentResourceInfo (&rinfo, sizeof rinfo, insOrTemplate, i) >= 0 && (rinfo.rinfo_PerInstrument || rinfo.rinfo_MaxOverhead)) {
            printf ("%s %s: %d/ins", comma ? "," : "", rsrcdesc[i], rinfo.rinfo_PerInstrument);
            if (rinfo.rinfo_MaxOverhead)
                printf (" + %d", rinfo.rinfo_MaxOverhead);
            comma = TRUE;
        }
    }
    printf ("\n");
}
