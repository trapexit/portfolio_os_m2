/* @(#) dsppdumptemplate.c 96/02/16 1.2 */

#include <audio/dspp_template.h>            /* self */
#include <audio/handy_macros.h>             /* NextPackedString() */
#include <dspptouch/dspp_instructions.h>    /* dspnDisassemble() */
#include <stdio.h>

/**
|||	AUTODOC -private -class audio -group DSPPTemplate -name dsppDumpTemplate
|||	Dumps DSPPTemplate contents to debugging terminal.
|||
|||	  Synopsis
|||
|||	    void dsppDumpTemplate (const DSPPTemplate *dtmp, const char *banner)
|||
|||	  Description
|||
|||	    Dumps DSPPTemplate contents to debugging terminal. This includes:
|||
|||	    - header
|||
|||	    - resources
|||
|||	    - relocations
|||
|||	    - dynamic link names
|||
|||	    - code disassembly
|||
|||	    - data initializers
|||
|||	  Arguments
|||
|||	    dtmp
|||	        DSPPTemplate to display. Assumed to be valid and non-NULL.
|||
|||	    banner
|||	        Optional description. Can be NULL.
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
|||	    dsppDumpDataInitializer(), dsppDumpRelocation(), dsppDumpResource(),
|||	    dspnDisassemble()
**/
void dsppDumpTemplate (const DSPPTemplate *dtmp, const char *banner)
{
    printf ("\n");
    if (banner) printf ("%s: ", banner);
    printf ("DSPP Template: FunctionID=%d Silicon=%d Format=%d Flags=0x%02x\n", dtmp->dtmp_Header.dhdr_FunctionID, dtmp->dtmp_Header.dhdr_SiliconVersion, dtmp->dtmp_Header.dhdr_FormatVersion, dtmp->dtmp_Header.dhdr_Flags);

    if (dtmp->dtmp_NumResources) {
        int32 i;

        printf ("Resources: (%d)\n", dtmp->dtmp_NumResources);
        for (i=0; i<dtmp->dtmp_NumResources; i++) dsppDumpResource (dtmp, i);
    }
    if (dtmp->dtmp_NumRelocations) {
        int32 i;

        printf ("Relocations: (%d)\n", dtmp->dtmp_NumRelocations);
        for (i=0; i<dtmp->dtmp_NumRelocations; i++) {
            printf ("%4d: ", i);
            dsppDumpRelocation (&dtmp->dtmp_Relocations[i]);
        }
    }
    if (dtmp->dtmp_DynamicLinkNamesSize) {
        const char *dlnkname = dtmp->dtmp_DynamicLinkNames;
        const char *const dlnkname_end = dtmp->dtmp_DynamicLinkNames + dtmp->dtmp_DynamicLinkNamesSize;

        printf ("Dynamic Links:\n");
        for (; dlnkname < dlnkname_end; dlnkname = NextPackedString(dlnkname)) {
            printf ("  '%s'\n", dlnkname);
        }
    }
    if (dtmp->dtmp_CodeSize) {
        dspnDisassemble (dsppGetCodeHunkImage (dtmp->dtmp_Codes, 0), 0, dtmp->dtmp_Codes[0].dcod_Size, "Hunk 0");
    }
    if (dtmp->dtmp_DataInitializerSize) {
        const DSPPDataInitializer *dini = dtmp->dtmp_DataInitializer;
        const DSPPDataInitializer *const dini_end = (DSPPDataInitializer *)((char *)dtmp->dtmp_DataInitializer + dtmp->dtmp_DataInitializerSize);

        printf ("Data Initializers: (%d)\n", dtmp->dtmp_DataInitializerSize);
        for (; dini < dini_end; dini = dsppNextDataInitializer(dini)) {
            dsppDumpDataInitializer (dini);
        }
    }
}
