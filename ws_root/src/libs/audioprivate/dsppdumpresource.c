/* @(#) dsppdumpresource.c 96/02/16 1.2 */

#include <audio/audio.h>
#include <audio/dspp_template.h>    /* self */
#include <stdio.h>
#include <string.h>

/**
|||	AUTODOC -private -class audio -group DSPPTemplate -name dsppDumpResource
|||	Dumps DSPPResource to debugging terminal.
|||
|||	  Synopsis
|||
|||	    void dsppDumpResource (const DSPPTemplate *dtmp, int32 rsrcIndex)
|||
|||	  Description
|||
|||	    Dumps DSPPResource to debugging terminal.
|||
|||	  Arguments
|||
|||	    dtmp
|||	        DSPPTemplate containing resource to display.
|||
|||	    rsrcIndex
|||	        Index of the resource to display. Assumed to be in the range of
|||	        0..dtmp->dtmp_NumResources-1.
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
|||	    dsppDumpDataInitializer(), dsppDumpRelocation(), dsppDumpTemplate()
**/
/* !!! this could use a reformatting + more text (e.g. flags, binding) */
void dsppDumpResource (const DSPPTemplate *dtmp, int32 rsrcIndex)
{
    static const char * const RsrcTypeDesc[DSPP_NUM_RSRC_TYPES] = { /* @@@ depends on DRSC_TYPE_ order */
        "Code",     "Knob",     "Var",      "Input",
        "Output",   "In FIFO",  "Out FIFO", "Ticks",
        "RBase",    "Trigger",
        "DAC",      "DAC Stat", "DAC Ctrl",
        "ADC",      "ADC Stat", "ADC Ctrl",
        "Noise",    "IRQ",      "HW Clock",
    };
    static const char * const SigTypeDesc[AF_SIGNAL_TYPE_MANY] = {  /* @@@ depends on AF_SIGNAL_TYPE_ order */
        "Signed",   "Unsigned", "Osc Freq", "LFO Freq",
        "SampRate", "Whole",
    };
    static const char * const InFIFOTypeDesc[] = {                  /* @@@ depends on DRSC_INFIFO_SUBTYPE_ order */
        "16-bit",   "8-bit",    "SQS2",
    };
    const DSPPResource * const drsc = &dtmp->dtmp_Resources[rsrcIndex];
    char rsrctypebuf[32];
    const char *rsrctypedesc;
    char subtypebuf[32];
    const char *subtypedesc;
    char namebuf[1+AF_MAX_NAME_SIZE+1];

    if (drsc->drsc_Type >= DSPP_NUM_RSRC_TYPES || !(rsrctypedesc = RsrcTypeDesc[drsc->drsc_Type])) {
        sprintf (rsrctypebuf, "type=%d", drsc->drsc_Type);
        rsrctypedesc = rsrctypebuf;
    }

    switch (drsc->drsc_Type) {
        case DRSC_TYPE_KNOB:
        case DRSC_TYPE_INPUT:
        case DRSC_TYPE_OUTPUT:
            if ( drsc->drsc_SubType >= AF_SIGNAL_TYPE_MANY ||
                 !(subtypedesc = SigTypeDesc[drsc->drsc_SubType]) ) goto badsubtype;
            break;

        case DRSC_TYPE_IN_FIFO:
            if ( drsc->drsc_SubType >= sizeof InFIFOTypeDesc / sizeof sizeof InFIFOTypeDesc[0] ||
                 !(subtypedesc = InFIFOTypeDesc[drsc->drsc_SubType]) ) goto badsubtype;
            break;

        default:
            if (drsc->drsc_SubType) goto badsubtype;
            subtypedesc = "-";
            break;

        badsubtype:
            sprintf (subtypebuf, "sub=%d", drsc->drsc_SubType);
            subtypedesc = subtypebuf;
            break;
    }
    {
        const char * const rsrcname = dsppGetTemplateRsrcName(dtmp,rsrcIndex);

        if (*rsrcname) sprintf (namebuf, "'%s'", rsrcname);
        else strcpy (namebuf, "");
    }

    printf ("%4d: %-8s %-8s many=%-4d flags=0x%02x bind-to=%2d,%-4d init=%-6d %s\n",
        rsrcIndex,
        rsrctypedesc, subtypedesc, drsc->drsc_Many, drsc->drsc_Flags,
        drsc->drsc_BindToRsrcIndex, drsc->drsc_Allocated,
        drsc->drsc_Default, namebuf);
}
