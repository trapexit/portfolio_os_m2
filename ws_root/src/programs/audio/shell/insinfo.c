/******************************************************************************
**
**  @(#) insinfo.c 96/06/18 1.13
**
******************************************************************************/

/**
|||	AUTODOC -public -class Shell_Commands -group Audio -name insinfo
|||	Displays information about DSP Instruments.
|||
|||	  Format
|||
|||	    insinfo [<instrument name> | <item number>]...
|||
|||	  Description
|||
|||	    This command displays information about the requested instruments. The
|||	    information is sent out to the debugging terminal.
|||
|||	  Arguments
|||
|||	    <instrument name>
|||	        Name of a DSP instrument or patch file to display.
|||
|||	    <item number>
|||	        The item number of an existing Instrument or Template item to display.
|||	        The item number can be in decimal, or in hexadecimal starting with 0x.
|||
|||	  Implementation
|||
|||	    Command implemented in V30.
|||
|||	  Location
|||
|||	    System.m2/Programs/insinfo
|||
|||	  See Also
|||
|||	    GetInstrumentResourceInfo(), GetInstrumentPortInfoByName(), GetAttachments()
**/

#include <audio/audio.h>
#include <audio/handy_macros.h>
#include <audio/score.h>
#include <kernel/mem.h>
#include <stdio.h>
#include <stdlib.h>

static bool LookupAndDumpIns (const char *itemNum);
static bool LoadAndDumpIns (const char *name);
static void DumpInsInfo (Item insOrTemplate);
static Err DumpAttachments (Item insOrTemplate, const char *hookName, uint8 portType);
static const char *GetPortTypeDesc (char *porttypedescbuf, uint8 portType);
static const char *GetSignalTypeDesc (char *sigtypedescbuf, uint8 signalType);

int main (int argc, char *argv[])
{
    Err errcode;
    int i;

#ifdef MEMDEBUG
    if (CreateMemDebug ( NULL ) < 0) return 0;

    if (ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
    /**/                  MEMDEBUGF_FREE_PATTERNS |
    /**/                  MEMDEBUGF_PAD_COOKIES |
    /**/                  MEMDEBUGF_CHECK_ALLOC_FAILURES |
    /**/                  MEMDEBUGF_KEEP_TASK_DATA ) < 0) return 0;
#endif

    if ((errcode = OpenAudioFolio()) < 0) {
        PrintError (NULL, "open audio folio", NULL, errcode);
        goto clean;
    }

    if (argc < 2) {
        printf ("usage: %s [<instrument name> | <item number>]...\n", argv[0]);
        errcode = -1;
        goto clean;
    }

    for (i=1; i<argc; i++) {
            /* do one of the following */
        LookupAndDumpIns (argv[i]) ||
        LoadAndDumpIns (argv[i]);       /* must be last */

            /* blank line between instruments */
        printf ("\n");
    }

clean:
    CloseAudioFolio();
#ifdef MEMDEBUG
    DumpMemDebug(NULL);
    DeleteMemDebug();
#endif
    return errcode;
}

/*
    Dump functions. These functions are supposed to return TRUE if the arg
    given them is identified as being for that function, regardless of
    whether there's an error, and FALSE if the arg is not for that function.
    This way multiple dump functions can be called in a sequence until
    one of them claims the arg. LoadAndDumpIns() always returns TRUE, so
    it should be last in the list.
*/

/*
    Lookup Item number and print information.

    Arguments
        itemNum
            String containing number of item to lookup in hex or decimal.
            If this isn't actually a number (i.e., there are non-numeric
            characters in it), then FALSE is returned. If this is a bogus
            Item, an error message is printed.

    Results
        TRUE if itemNum is really a number, FALSE otherwise.
*/
static bool LookupAndDumpIns (const char *itemNum)
{
    char *endp;
    const Item item = strtol (itemNum, &endp, 0);

        /* if the arg isn't a number (i.e., it contains non-numeric characters), it's not an item number */
    if (*endp) return FALSE;

        /* lookup item and display it */
    {
        const ItemNode * const n = LookupItem (item);

        if (n && n->n_SubsysType == AUDIONODE && (n->n_Type == AUDIO_TEMPLATE_NODE || n->n_Type == AUDIO_INSTRUMENT_NODE)) {
            printf ("Item 0x%x (", item);
            if (n->n_Name) {
                printf ("'%s', ", n->n_Name);
            }
            switch (n->n_Type) {
                case AUDIO_TEMPLATE_NODE:
                    printf ("Template");
                    break;

                case AUDIO_INSTRUMENT_NODE:
                    printf ("Instrument");
                    break;

                default:
                    printf ("type %d", n->n_Type);
                    break;
            }
            printf ("):\n");
            DumpInsInfo (item);
        }
        else {
            PrintError (NULL, "get information for", itemNum, AF_ERR_BADITEM);
        }
    }
    return TRUE;
}

/*
    Load named instrument or patch template and print information.

    Arguments
        name
            Name of dsp instrument or patch file to pass to LoadScoreTemplate().

    Results
        Always returns TRUE.
*/
static bool LoadAndDumpIns (const char *name)
{
    Item insTemplate;
    Err errcode;

    if ((errcode = insTemplate = LoadScoreTemplate (name)) >= 0) {
        printf ("%s:\n", name);
        DumpInsInfo (insTemplate);
        UnloadScoreTemplate (insTemplate);
    }
    else {
        PrintError (NULL, "load instrument", name, errcode);
    }

    TOUCH(errcode);     /* silence warning for not using errcode */
    return TRUE;
}

static void DumpInsInfo (Item insOrTemplate)
{
    Err errcode;

        /* display resources */
    {
        static const char * const rsrcDesc[AF_RESOURCE_TYPE_MANY] = {   /* @@@ depends on AF_RESOURCE_TYPE_ order */
            "Ticks / Frame",
            "Code Memory Words",
            "Data Memory Words",
            "FIFOs",
            "Triggers",
        };
        InstrumentResourceInfo info;
        int32 i;

        printf ("Resource Consumption\n");
        printf ("  Resource             Per  Ovrhd\n");
        /*         xxxxxxxxxxxxxxxxx  xxxxx  xxxxx  */

        for (i=0; i<AF_RESOURCE_TYPE_MANY; i++) {
            if ((errcode = GetInstrumentResourceInfo (&info, sizeof info, insOrTemplate, i)) < 0) goto clean;
            printf ("  %-17s%7d%7d\n", rsrcDesc[i], info.rinfo_PerInstrument, info.rinfo_MaxOverhead);
        }
    }

        /* display ports */
    {
        const int32 numPorts = GetNumInstrumentPorts (insOrTemplate);

        if (numPorts > 0) {
            char porttypebuf[32];
            char sigtypebuf[32];
            InstrumentPortInfo info;
            int32 i;

            printf ("Ports\n");
            printf ("  Port Type  Signal Type  Parts  Name\n");
            /*         xxxxxxxxx  xxxxxxxxxxx  xxxxx  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx */

            for (i=0; i<numPorts; i++) {
                if ((errcode = GetInstrumentPortInfoByIndex (&info, sizeof info, insOrTemplate, i)) < 0) goto clean;

                switch (info.pinfo_Type) {
                    case AF_PORT_TYPE_INPUT:
                    case AF_PORT_TYPE_OUTPUT:
                    case AF_PORT_TYPE_KNOB:
                        printf ("  %-9s  %-11s  %5d  %s\n", GetPortTypeDesc (porttypebuf, info.pinfo_Type), GetSignalTypeDesc (sigtypebuf, info.pinfo_SignalType), info.pinfo_NumParts, info.pinfo_Name);
                        break;

                    case AF_PORT_TYPE_ENVELOPE:
                        printf ("  %-9s  %-11s  %5s  %s\n", GetPortTypeDesc (porttypebuf, info.pinfo_Type), GetSignalTypeDesc (sigtypebuf, info.pinfo_SignalType), "-", info.pinfo_Name);
                        break;

                    default:
                        printf ("  %-9s  %-11s  %5s  %s\n", GetPortTypeDesc (porttypebuf, info.pinfo_Type), "-", "-", info.pinfo_Name);
                        break;
                }
            }
        }
    }

        /* display attachments */
    {
        const int32 numPorts = GetNumInstrumentPorts (insOrTemplate);
        InstrumentPortInfo info;
        int32 i;

        for (i=0; i<numPorts; i++) {
            if ((errcode = GetInstrumentPortInfoByIndex (&info, sizeof info, insOrTemplate, i)) < 0) goto clean;

            if (info.pinfo_Type == AF_PORT_TYPE_IN_FIFO ||
                info.pinfo_Type == AF_PORT_TYPE_OUT_FIFO ||
                info.pinfo_Type == AF_PORT_TYPE_ENVELOPE) {

                if ((errcode = DumpAttachments (insOrTemplate, info.pinfo_Name, info.pinfo_Type)) < 0) goto clean;
            }
        }
    }

    return;

clean:
    PrintError (NULL, "get information", NULL, errcode);
    TOUCH(errcode);     /* silence warning for not using errcode */
}

static Err DumpAttachments (Item insOrTemplate, const char *hookName, uint8 portType)
{
    Item *attachments = NULL;
    int32 allocNumAttachments;
    Err errcode;

    if ((errcode = allocNumAttachments = GetNumAttachments (insOrTemplate, hookName)) < 0) goto clean;

    if (allocNumAttachments > 0) {
        int32 i;
        int32 readNumAttachments;
        char porttypebuf[32];

        if (!(attachments = AllocMem (allocNumAttachments * sizeof (Item), MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE | MEMTYPE_FILL))) {
            errcode = MAKEERR (ER_USER, 0, ER_SEVERE, ER_E_USER, ER_C_STND, ER_NoMem);
            goto clean;
        }
        if ((errcode = readNumAttachments = GetAttachments (attachments, allocNumAttachments, insOrTemplate, hookName)) < 0) goto clean;

        printf ("%s '%s' Attachments\n", GetPortTypeDesc (porttypebuf, portType), hookName);

        for (i=0; i<MIN(allocNumAttachments,readNumAttachments); i++) {
            Item slaveItem;

            {
                TagArg tags[] = {
                    { AF_TAG_SLAVE },
                    { AF_TAG_START_AT },
                    { AF_TAG_SET_FLAGS },
                    TAG_END
                };
                const Item attItem = attachments[i];

                if ((errcode = GetAudioItemInfo (attItem, tags)) < 0) goto clean;
                slaveItem = (Item)tags[0].ta_Arg;

                printf ("  Attachment 0x%08x, Start at %d, Flags 0x%02x\n", attItem, (int32)tags[1].ta_Arg, (uint32)tags[2].ta_Arg);
            }

            {
                const ItemNode *slave;

                if (!(slave = LookupItem (slaveItem))) {
                    errcode = AF_ERR_BADITEM;
                    goto clean;
                }

                switch (slave->n_Type) {
                    case AUDIO_SAMPLE_NODE:
                        {
                            TagArg tags[] = {
                                { AF_TAG_NUMBITS },
                                { AF_TAG_CHANNELS },
                                { AF_TAG_FRAMES },
                                { AF_TAG_SAMPLE_RATE_FP },
                                { AF_TAG_COMPRESSIONTYPE },
                                { AF_TAG_ADDRESS },
                                TAG_END
                            };

                            if ((errcode = GetAudioItemInfo (slaveItem, tags)) < 0) goto clean;

                            printf ("  %-10s 0x%08x, %d bits, %d channel(s), %d frames, %g Hz",
                                slave->n_ItemFlags & ITEMNODE_PRIVILEGED
                                    ? (tags[5].ta_Arg ? "Delay Line" : "Delay Line Template")
                                    : "Sample",
                                slaveItem,
                                (int32)tags[0].ta_Arg,
                                (int32)tags[1].ta_Arg,
                                (int32)tags[2].ta_Arg,
                                ConvertTagData_FP(tags[3].ta_Arg));
                            if ((PackedID)tags[4].ta_Arg) printf (", %.4s", &(PackedID)tags[4].ta_Arg);
                        }
                        break;

                    case AUDIO_ENVELOPE_NODE:
                        {
                            TagArg tags[] = {
                                { AF_TAG_TYPE },
                                { AF_TAG_FRAMES },
                                TAG_END
                            };
                            char sigtypebuf[32];

                            if ((errcode = GetAudioItemInfo (slaveItem, tags)) < 0) goto clean;

                            printf ("  Envelope   0x%08x, %s, %d frames",
                                slaveItem,
                                GetSignalTypeDesc (sigtypebuf, (uint32)tags[0].ta_Arg),
                                (int32)tags[1].ta_Arg);
                        }
                        break;

                    default:
                        printf ("  Slave Item 0x%08x", slaveItem);
                        break;
                }
                if (slave->n_Name) printf (", name '%s'", slave->n_Name);
                printf ("\n");
            }
        }
    }

        /* success */
    errcode = 0;

clean:
    FreeMem (attachments, TRACKED_SIZE);
    return errcode;
}

static const char *GetPortTypeDesc (char *porttypedescbuf, uint8 portType)
{
    static const char * const portDesc[AF_PORT_TYPE_MANY] = {   /* @@@ depends on AF_PORT_TYPE_ order */
        "Input",
        "Output",
        "Knob",
        "In FIFO",
        "Out FIFO",
        "Trigger",
        "Envelope",
    };
    const char *porttypedesc;

    if (portType > AF_PORT_TYPE_MAX || !(porttypedesc = portDesc[portType])) {
        sprintf (porttypedescbuf, "%d", portType);
        porttypedesc = porttypedescbuf;
    }

    return porttypedesc;
}

static const char *GetSignalTypeDesc (char *sigtypedescbuf, uint8 signalType)
{
    static const char * const sigDesc[AF_SIGNAL_TYPE_MANY] = {  /* @@@ depends on AF_SIGNAL_TYPE_ order */
        "Signed",
        "Unsigned",
        "Osc Freq",
        "LFO Freq",
        "Sample Rate",
        "Whole",
    };
    const char *sigtypedesc;

    if (signalType > AF_SIGNAL_TYPE_MAX || !(sigtypedesc = sigDesc[signalType])) {
        sprintf (sigtypedescbuf, "%d", signalType);
        sigtypedesc = sigtypedescbuf;
    }

    return sigtypedesc;
}
