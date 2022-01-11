/******************************************************************************
**
**  @(#) createpatch_parser.c 96/05/02 1.79
**
**  DSP patch intstrument template builder - parser.
**
**  By: Bill Barton
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950712 WJB  Created placeholder.
**  950715 WJB  Completed initial parser.
**  950717 WJB  Split into modules along functional lines.
**  950717 WJB  Now using PatchParserData for things that don't need to be returned by ParsePatchCmds().
**  950718 WJB  Moved DumpPatchData() to createpatch.c.
**              Updates pti_NumMoves in AddPatchMoveInfo().
**  950728 WJB  Improved parser diagnostic messages to include much more useful information.
**  950728 WJB  Changed several signed PatchCmd fields to unsigned to cut bounds checking in half.
**  950728 WJB  Added port and block name validation.
**  950728 WJB  Added part count bounds checking.
**  950731 WJB  Improved clearing of KEEP flag a bit and added some documentation about it.
**  950803 WJB  Layed groundwork for converting ppcmdConnect() to ppcmdConnectAndSetConstants().
**  950803 WJB  Replaced PatchRelocInfo and PatchMoveInfo with PatchDestInfo.
**  950804 WJB  Added multi-part dest resource optimizier.
**  950808 WJB  Added support for PATCH_CMD_SET_CONSTANT.
**  950809 WJB  Applied DRSC_F_INIT_AT_ flags to PortDTmp resources.
**  950814 WJB  Initialize pti_NumResources and pri_DRsc. Make some use of these fields.
**  950818 WJB  Moved PatchResourceNameInfo list into PatchData.
**              Using n_Size instead of MEMTYPE_TRACKSIZE in a few places.
**  950821 WJB  Added envelope hook support for PATCH_CMD_EXPOSE.
**  950822 WJB  Now adds envelope hook names to list of exposed things to prevent odd namespace collisions.
**  950822 WJB  Moved name uniqueness testing into AddPatchResourceNameInfo().
**              Added PATCH_RESOURCE_NAME_F_CONST to permit allocating only those names which don't remain constant.
**  950822 WJB  Moved dsppFindEnvHookResources() to audio_envelopes.c
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <audio/audio.h>
#include <audio/handy_macros.h>             /* packed string */
#include <dspptouch/dspp_instructions.h>    /* dspnCanValueBeImmediated() */
#include <kernel/mem.h>

#include "createpatch_internal.h"

#ifdef BUILD_STRINGS
#include <stdarg.h>                         /* va_list */
#include <stdio.h>                          /* printf() */
#endif


/* -------------------- Debug */

#define DEBUG_Parse     0       /* turn on debuggin during parsing */
#define DEBUG_Expose    0       /* turn on debugging in ExposeResource() */

#if DEBUG_Parse || DEBUG_Expose
    #include <stdio.h>
#endif


/* -------------------- Local data structures */

    /* data structure used during parsing */
typedef struct PatchParserData {
    const PatchCmd *ppd_PatchCmdList;           /* list of PatchCmds passed into parser */
    PatchData  *ppd_PatchData;                  /* pointer to allocated PatchData under construction */
} PatchParserData;


/* -------------------- Local functions */

#ifdef BUILD_PARANOIA
    /* preprocessor */
static Err ValidatePatchCmdList (const PatchCmd *patchCmdList);
#endif

    /* parsers */
    /* ("ppcmd" stands for ParsePatchCmds) */
static Err ppcmdApplyCompilerOptions (PatchParserData *);
static Err ppcmdAddTemplates (PatchParserData *);
static Err ppcmdDefinePortsAndKnobs (PatchParserData *);
static Err ppcmdExpose (PatchParserData *);
static Err ppcmdConnectAndSetConstants (PatchParserData *);

    /* PatchData */
static PatchData *CreatePatchData (void);

    /* PatchTemplateInfo */
static PatchTemplateInfo *CreatePatchTemplateInfo (uint8 ptiType, const char *ptiName, const DSPPTemplate *dtmp);
static void DeletePatchTemplateInfo (PatchTemplateInfo *);

    /* PatchResourceInfo/PatchDestInfo */
static bool SetPatchResourceConnectability (PatchResourceInfo *, uint8 ptiType, uint8 connectability);
static void ClearPatchResourceConnectability (PatchResourceInfo *);

    /* PatchResourceNameInfo */
static Err ExposeResource (PatchData *, PatchResourceInfo *, const char *exposedName, uint8 pniFlags);
static Err AddPatchResourceNameInfo (PatchResourceNameInfo **resultpni, PatchData *, const char *name, uint8 flags);
static void DeletePatchResourceNameInfo (PatchResourceNameInfo *);

    /* name checkers */
#define CheckUniqueName(l,n)            (FindNamedNode((l),(n)) ? PATCH_ERR_NAME_NOT_UNIQUE : 0)
#define CheckBlockNameLegality(name)    (!(name) || !*(name) ? PATCH_ERR_BAD_NAME : 0)
    /* port names: don't allow '.' punctuations to avoid collision with envelope resources */
#define CheckPortNameLegality(name)     (!(name) || !*(name) || strchr(name,'.') ? PATCH_ERR_BAD_NAME : 0) /* @@@ might requires some additional syntactical rules */

    /* diagnostics */
#ifdef BUILD_STRINGS
    static void PrintPatchCmdError (const PatchCmd *, Err, const char *msgfmt, ...);
    static void ppcerrDestAlreadyUsed (const PatchCmd *, Err errcode, const PatchTemplateInfo *dstpti, uint16 dstRsrcIndex, uint16 dstPartNum);
    #define PPCERR(x) PrintPatchCmdError x
#else
    #define ppcerrDestAlreadyUsed(pc,errcode,dstpti,dstRsrcIndex,dstPartNum)
    #define PPCERR(x)
#endif


/* -------------------- PatchCmd parser */

/*
    Parse PatchCmds and build PatchData structure.

    Does full parsing of PatchCmds, allocates PatchData and related structures.
    Does not do resource requirement analysis: that is done by linker.

    Call DeletePatchData() to destroy PatchData.

    Inputs
        resultpd
            Buffer to contain allocated PatchData pointer.

        patchCmdList
            list of PatchCmds to parse.

    Results
        0 on success, Err code on failure.
        *resultpd is set to allocated PatchData on success, NULL on failure.
*/
Err dsppParsePatchCmds (PatchData **resultpd, const PatchCmd *patchCmdList)
{
    PatchParserData ppd;
    Err errcode;

        /* init ppd */
    memset (&ppd, 0, sizeof ppd);
    ppd.ppd_PatchCmdList = patchCmdList;

        /* clear result */
    *resultpd = NULL;

        /* alloc/init PatchData */
    if (!(ppd.ppd_PatchData = CreatePatchData())) {
        errcode = PATCH_ERR_NOMEM;
        goto clean;
    }

  #ifdef BUILD_PARANOIA
    if ((errcode = ValidatePatchCmdList (patchCmdList)) < 0) goto clean;
  #endif

        /*
            Process commands:
                compiler options - PATCH_CMD_SET_COHERENCE
                templates - PATCH_CMD_ADD_TEMPLATE
                ports - PATCH_CMD_DEFINE_PORT and PATCH_CMD_DEFINE_KNOB
                exposures - PATCH_CMD_EXPOSE
                connections - PATCH_CMD_CONNECT and PATCH_CMD_SET_CONSTANT
            @@@ order is significant:
                . expose depends on templates being added first
                . connect depends on templates, defined ports, and exposures being done first
                  (exposures can change pri_Connectability, so they must happen before connections
                  are allowed).
        */
    if ((errcode = ppcmdApplyCompilerOptions (&ppd)) < 0) goto clean;
    if ((errcode = ppcmdAddTemplates (&ppd)) < 0) goto clean;
    if ((errcode = ppcmdDefinePortsAndKnobs (&ppd)) < 0) goto clean;
    if ((errcode = ppcmdExpose (&ppd)) < 0) goto clean;
    if ((errcode = ppcmdConnectAndSetConstants (&ppd)) < 0) goto clean;

        /* success */
    *resultpd = ppd.ppd_PatchData;
    errcode = 0;

clean:
        /* on failure dispose of ppd_PatchData */
    if (errcode < 0) {
        dsppDeletePatchData (ppd.ppd_PatchData);
    }
    return errcode;
}


#ifdef BUILD_PARANOIA       /* { */

/* -------------------- PatchCmd preprocessing */

static int32 GetNumPatchCmdArgs (uint32 cmdID);
static bool CheckPadArgs (PatchCmdGeneric *pc, int32 numArgs);

/*
    Preprocess and validate PatchCmds
        - trap undefined PatchCmd IDs
        - check padding for non-zeros

    Arguments
        patchCmdList
            PatchCmd list to validate.

    Results
        0 on success, Err code on failure.
        Prints diagnostic error messages.
*/

static Err ValidatePatchCmdList (const PatchCmd *patchCmdList)
{
    const PatchCmd *pci, *pcstate;
    int32 numArgs;
    Err errcode;

    for (pcstate = patchCmdList; pci = NextPatchCmd (&pcstate); ) {

            /* look up PatchCmd ID; get number of arguments */
        if ((errcode = numArgs = GetNumPatchCmdArgs (pci->pc_Generic.pc_CmdID)) < 0) {
            PPCERR((pci, errcode, "Undefined PatchCmd ID %u", pci->pc_Generic.pc_CmdID));
            goto clean;
        }

            /* check padding */
            /* @@@ this doesn't actually dump the non-zero padding, though */
        if (!CheckPadArgs (&pci->pc_Generic, numArgs)) {
            errcode = PATCH_ERR_BAD_PATCH_CMD;
            PPCERR((pci, errcode, "PatchCmd contains non-zero padding"));
            goto clean;
        }
    }

    return 0;

clean:
    return errcode;
}

/*
    Return number of valid pc_Args[] for each defined PATCH_CMD_

    Arguments
        cmdID
            PATCH_CMD_ ID

    Results
        Number of arguments, or Err code if undefined cmdID.
*/
static int32 GetNumPatchCmdArgs (uint32 cmdID)
{
    static const uint8 NumArgTable[PATCH_CMD_MAX-PATCH_CMD_MIN+1] = {   /* @@@ indexed by PATCH_CMD_ IDs */
        2,  /* PATCH_CMD_ADD_TEMPLATE  */
        4,  /* PATCH_CMD_DEFINE_PORT   */
        4,  /* PATCH_CMD_DEFINE_KNOB   */
        3,  /* PATCH_CMD_EXPOSE        */
        6,  /* PATCH_CMD_CONNECT       */
        4,  /* PATCH_CMD_SET_CONSTANT  */
        1,  /* PATCH_CMD_SET_COHERENCE */
    };

    return (cmdID >= PATCH_CMD_MIN && cmdID <= PATCH_CMD_MAX)
        ? NumArgTable [cmdID - PATCH_CMD_MIN]
        : PATCH_ERR_BAD_PATCH_CMD;
}

/*
    Checks the pad args after the indicated number of arguments to make sure
    they are all set to 0.

    Arguments
        pc
            PatchCmd to check

        numArgs
            Number of arguments PatchCmd has. Arguments after this are checked.

    Results
        TRUE if all pad arguments are zero. FALSE otherwise.
*/
static bool CheckPadArgs (PatchCmdGeneric *pc, int32 numArgs)
{
    int32 i;

    for (i=numArgs; i<sizeof pc->pc_Args / sizeof pc->pc_Args[0]; i++) {
        if (pc->pc_Args[i]) return FALSE;
    }
    return TRUE;
}

#endif  /* } */


/* -------------------- apply compiler options */

/*
    Parse compiler options commands:
        . PATCH_CMD_SET_COHERENCE - controls state of
          PATCH_DATA_F_INTER_INSTRUMENT_PADDING

    Expects DeletePatchData() to clean up after partial success.
*/
static Err ppcmdApplyCompilerOptions (PatchParserData *ppd)
{
    const PatchCmd *pci, *pcstate;

  #if DEBUG_Parse
    printf ("ppcmdApplyCompilerOptions:\n");
  #endif

    for (pcstate = ppd->ppd_PatchCmdList; pci = NextPatchCmd (&pcstate); ) {
        switch (pci->pc_Generic.pc_CmdID) {
            case PATCH_CMD_SET_COHERENCE:
                {
                    const PatchCmdSetCoherence * const pc = &pci->pc_SetCoherence;

                  #if DEBUG_Parse
                    DumpPatchCmd (pci, NULL);
                  #endif

                    if (pc->pc_State)
                        ppd->ppd_PatchData->pd_Flags |= PATCH_DATA_F_INTER_INSTRUMENT_PADDING;
                    else
                        ppd->ppd_PatchData->pd_Flags &= ~PATCH_DATA_F_INTER_INSTRUMENT_PADDING;
                }
                break;
        }
    }

    return 0;
}


/* -------------------- add templates */

/*
    Parse PATCH_CMD_ADD_TEMPLATE commands
    Expects DeletePatchData() to clean up after partial success.
*/
static Err ppcmdAddTemplates (PatchParserData *ppd)
{
    const PatchCmd *pci, *pcstate;
    Err errcode;

  #if DEBUG_Parse
    printf ("ppcmdAddTemplates:\n");
  #endif

    for (pcstate = ppd->ppd_PatchCmdList; pci = NextPatchCmd (&pcstate); ) {
        if (pci->pc_Generic.pc_CmdID == PATCH_CMD_ADD_TEMPLATE) {
            const PatchCmdAddTemplate * const pc = &pci->pc_AddTemplate;
            const DSPPTemplate *dtmp;
            PatchTemplateInfo *pti;

          #if DEBUG_Parse
            DumpPatchCmd (pci, NULL);
          #endif

                /* check name legality */
            if ((errcode = CheckBlockNameLegality (pc->pc_BlockName)) < 0) {
                PPCERR ((pci, errcode, "Block name \"%s\" invalid", pc->pc_BlockName));
                goto clean;
            }

                /* verify that name is unique */
            if ((errcode = CheckUniqueName (&ppd->ppd_PatchData->pd_TemplateList, pc->pc_BlockName)) < 0) {
                PPCERR ((pci, errcode, "Block name \"%s\" already defined in patch", pc->pc_BlockName));
                goto clean;
            }

                /* get DSPPTemplate pointer, which conveniently requires an item lookup */
            {
                    /* resolve pointers */
                if (!(dtmp = dsppLookupTemplate (pc->pc_InsTemplate))) {
                    errcode = PATCH_ERR_BADITEM;
                    PPCERR ((pci, errcode, "Item 0x%x not a valid instrument template item", pc->pc_InsTemplate));
                    goto clean;
                }

                    /* check flags: only permit templates that aren't DHDR_F_SHARED or DHDR_F_PRIVILEGED */
                if (dtmp->dtmp_Header.dhdr_Flags & (DHDR_F_SHARED | DHDR_F_PRIVILEGED)) {
                    errcode = PATCH_ERR_BADITEM;
                  #if BUILD_STRINGS
                    {
                        const ItemNode * const n = LookupItem (pc->pc_InsTemplate);

                        PPCERR ((pci, errcode, "Template 0x%x ('%s') not legal in a patch", pc->pc_InsTemplate, n ? n->n_Name : NULL));
                    }
                  #endif
                    goto clean;
                }
            }

                /* create and add PatchTemplateInfo to PatchData */
            {
                if (!(pti = CreatePatchTemplateInfo (PATCH_TEMPLATE_TYPE_INSTRUMENT, pc->pc_BlockName, dtmp))) {
                    errcode = PATCH_ERR_NOMEM;
                    PPCERR ((pci, errcode, NULL));
                    goto clean;
                }
                AddTail (&ppd->ppd_PatchData->pd_TemplateList, (Node *)pti);
            }

                /* expose imported resources (they need to be matched to the exported resource by name) */
                /* (no clean up of pti on failure here because DeletePatchData() takes care of it nicely) */
                /* @@@ not in CreatePatchTemplateInfo(), because that would mean that its creation would have side effects */
            {
                const char *drscname = dtmp->dtmp_ResourceNames;
                PatchResourceInfo *pri = pti->pti_ResourceInfo;
                uint16 numResources = pti->pti_NumResources;

                for (; numResources--; pri++, drscname = NextPackedString (drscname)) {
                    if (pri->pri_DRsc->drsc_Flags & DRSC_F_IMPORT) {
                            /* imported resources must be allowed to appear multiple times, so relax uniqueness
                               for them (don't set PATCH_RESOURCE_NAME_F_UNIQUE). don't let imported and
                               non-imported resources have the same name, however */
                        if ((errcode = ExposeResource (ppd->ppd_PatchData, pri, drscname, PATCH_RESOURCE_NAME_F_CONST)) < 0) {
                            PPCERR ((pci, errcode, "Unable to expose imported resource \"%s\"", drscname));
                            goto clean;
                        }
                    }
                }
            }

        }
    }

    return 0;

clean:
    return errcode;
}


/* -------------------- port/knob name definitions (PortPatchTemplate builder) */

    /* temp structure used to name validation and speed up multiple passes thru port/knob cmds */
typedef struct PatchPortInfo {
    Node        ppi;                            /* n_Name is port/knob name */
    const PatchCmd *ppi_PatchCmd;               /* PatchCmd that will create this port */
} PatchPortInfo;

static PatchPortInfo *CreatePatchPortInfo (const char *name, const PatchCmd *);
#define DeletePatchPortInfo(ppi) FreeMem ((ppi), sizeof (PatchPortInfo))
static Err CreatePatchPortDTmp (DSPPTemplate **resultPortDTmp, const List *portList);

/*
    Parse PATCH_CMD_DEFINE_PORT and PATCH_CMD_DEFINE_KNOB commands.
    Expects DeletePatchData() to clean up after partial success.
*/
static Err ppcmdDefinePortsAndKnobs (PatchParserData *ppd)
{
    List portlist;
    Err errcode;

  #if DEBUG_Parse
    printf ("ppcmdDefinePortsAndKnobs:\n");
  #endif

        /* init portlist */
    PrepList (&portlist);

        /* build portlist */
    {
        const PatchCmd *pci, *pcstate;

        for (pcstate = ppd->ppd_PatchCmdList; pci = NextPatchCmd (&pcstate); ) {
            const char *name;

            switch (pci->pc_Generic.pc_CmdID) {
                case PATCH_CMD_DEFINE_PORT:
                    name = pci->pc_DefinePort.pc_PortName;
                    goto addport;

                case PATCH_CMD_DEFINE_KNOB:
                    name = pci->pc_DefineKnob.pc_KnobName;
                    goto addport;

                addport:
                  #if DEBUG_Parse
                    DumpPatchCmd (pci, NULL);
                  #endif

                        /* check name legality */
                    if ((errcode = CheckPortNameLegality (name)) < 0) {
                        PPCERR ((pci, errcode, "Port/knob name \"%s\" invalid", name));
                        goto clean;
                    }

                        /* check name uniqueness */
                    if ((errcode = CheckUniqueName (&portlist, name)) < 0) {
                        PPCERR ((pci, errcode, "Port/knob name \"%s\" already defined in patch", name));
                        goto clean;
                    }

                        /* alloc/add PatchPortInfo */
                    {
                        PatchPortInfo * const ppi = CreatePatchPortInfo (name, pci);

                        if (!ppi) {
                            errcode = PATCH_ERR_NOMEM;
                            PPCERR ((pci, errcode, NULL));
                            goto clean;
                        }

                        AddTail (&portlist, (Node *)ppi);
                    }
                    break;
            }
        }
    }

        /* build port template, if there are any ports */
    if (!IsEmptyList (&portlist)) {
        PatchData * const pd = ppd->ppd_PatchData;

            /* create pd_PortDTmp */
            /* (this function calls PPCERR()) */
        if ((errcode = CreatePatchPortDTmp (&pd->pd_PortDTmp, &portlist)) < 0) goto clean;

            /* create pd_PortTemplate */
        if (!(pd->pd_PortTemplate = CreatePatchTemplateInfo (PATCH_TEMPLATE_TYPE_PORT, NULL, pd->pd_PortDTmp))) {
            /* (could call PPCERR() here, but not a lot of point in it really) */
            errcode = PATCH_ERR_NOMEM;
            goto clean;
        }

            /* expose names of all resources in pd_PortTemplate */
            /* @@@ does possibly redundant name uniqueness test, but should be harmless */
        {
            const char *drscname = pd->pd_PortDTmp->dtmp_ResourceNames;
            PatchResourceInfo *pri = pd->pd_PortTemplate->pti_ResourceInfo;
            uint16 numResources = pd->pd_PortTemplate->pti_NumResources;

            for (; numResources--; pri++, drscname = NextPackedString (drscname)) {
                if ((errcode = ExposeResource (pd, pri, drscname, PATCH_RESOURCE_NAME_F_UNIQUE | PATCH_RESOURCE_NAME_F_CONST)) < 0) {
                    PPCERR ((NULL, errcode, "Unable to expose port/knob \"%s\"", drscname));
                    goto clean;
                }
            }
        }
    }

        /* success */
    errcode = 0;

clean:
        /* delete members of portlist */
    {
        PatchPortInfo *ppi, *next;

        PROCESSLIST (&portlist, ppi, next, PatchPortInfo) {
            DeletePatchPortInfo (ppi);
        }
    }
    return errcode;
}

static PatchPortInfo *CreatePatchPortInfo (const char *name, const PatchCmd *pc)
{
    PatchPortInfo *ppi;

    if (ppi = AllocMem (sizeof *ppi, MEMTYPE_ANY | MEMTYPE_FILL)) {
        ppi->ppi.n_Name   = (char *)name;
        ppi->ppi_PatchCmd = pc;
    }

    return ppi;
}

/*
    Create DSPPTemplate for ports and knobs suitable for storing in pd_PortDTmp

    Inputs
        resultPortDTmp
            Pointer to buffer to store allocated DSPPTemplate. Assumed to be valid.

        portList
            Pointer to list of PatchPortInfos. Must not be empty.

    Results
        0 on success, Err code on failure.
        Fills in *resultPortDTmp on success. Writes NULL to *resultPortDTmp on failure.
*/
static Err CreatePatchPortDTmp (DSPPTemplate **resultPortDTmp, const List *portList)
{
    DSPPTemplate *portdtmp;
    int32 numports=0, totalnamesize=0;
    const PatchPortInfo *ppi;
    Err errcode;

        /* init result */
    *resultPortDTmp = NULL;

        /* total up resource space */
    SCANLIST (portList, ppi, PatchPortInfo) {
        totalnamesize += PackedStringSize (ppi->ppi.n_Name);
        numports++;
    }

        /* alloc DSPPTemplate and substructures */
    if ( !(portdtmp = dsppCreateUserTemplate()) ||
         !(portdtmp->dtmp_Resources     = (DSPPResource *)AllocMem (numports * sizeof (DSPPResource), MEMTYPE_TRACKSIZE | MEMTYPE_NORMAL | MEMTYPE_FILL)) ||
         !(portdtmp->dtmp_ResourceNames = (char *)AllocMem (totalnamesize, MEMTYPE_TRACKSIZE | MEMTYPE_NORMAL)) ) {

        errcode = PATCH_ERR_NOMEM;
        goto clean;
    }
    portdtmp->dtmp_NumResources = numports;

        /* fill in dtmp_Resources[] and dtmp_ResourceNames */
    {
        DSPPResource *drscptr = portdtmp->dtmp_Resources;
        char *nameptr = portdtmp->dtmp_ResourceNames;

        SCANLIST (portList, ppi, PatchPortInfo) {
            DSPPResource drsc;

                /* everthing below assumes that this is init'd to 0 */
            memset (&drsc, 0, sizeof drsc);

            switch (ppi->ppi_PatchCmd->pc_Generic.pc_CmdID) {
                case PATCH_CMD_DEFINE_PORT:
                    {
                        const PatchCmdDefinePort * const pc = &ppi->ppi_PatchCmd->pc_DefinePort;

                      #ifdef BUILD_PARANOIA
                            /* validate pc_NumParts */
                        if (pc->pc_NumParts < AF_PART_COUNT_MIN || pc->pc_NumParts > AF_PART_COUNT_MAX) {
                            errcode = PATCH_ERR_OUT_OF_RANGE;
                            PPCERR ((ppi->ppi_PatchCmd, errcode, "Part count %u out of range", pc->pc_NumParts));
                            goto clean;
                        }
                            /* validate pc_SignalType */
                        if (pc->pc_SignalType > AF_SIGNAL_TYPE_MAX) {
                            errcode = PATCH_ERR_BAD_SIGNAL_TYPE;
                            PPCERR ((ppi->ppi_PatchCmd, errcode, "Signal type %u undefined", pc->pc_SignalType));
                            goto clean;
                        }
                            /* pc_PortType valiated below */
                      #endif

                            /* build type-specific parts of DSPPResource (and catch bad pc_PortType) */
                        switch (pc->pc_PortType) {
                            case AF_PORT_TYPE_INPUT:
                                drsc.drsc_Type  = DRSC_TYPE_INPUT;
                                break;

                            case AF_PORT_TYPE_OUTPUT:
                                drsc.drsc_Type  = DRSC_TYPE_OUTPUT;
                                drsc.drsc_Flags = DRSC_F_INIT_AT_ALLOC; /* init to 0 */
                                break;

                            default:
                                errcode = PATCH_ERR_BAD_PORT_TYPE;
                                PPCERR ((ppi->ppi_PatchCmd, errcode, "Port type %u illegal", pc->pc_PortType));
                                goto clean;
                        }

                            /* fill in common DSPPResource parts */
                        drsc.drsc_SubType = pc->pc_SignalType;
                        drsc.drsc_Many    = pc->pc_NumParts;
                    }
                    goto addport;

                case PATCH_CMD_DEFINE_KNOB:
                    {
                        const PatchCmdDefineKnob * const pc = &ppi->ppi_PatchCmd->pc_DefineKnob;

                      #ifdef BUILD_PARANOIA
                            /* validate pc_NumParts */
                        if (pc->pc_NumParts < AF_PART_COUNT_MIN || pc->pc_NumParts > AF_PART_COUNT_MAX) {
                            errcode = PATCH_ERR_OUT_OF_RANGE;
                            PPCERR ((ppi->ppi_PatchCmd, errcode, "Part count %u out of range", pc->pc_NumParts));
                            goto clean;
                        }
                            /* validate pc_KnobType (signal type) */
                        if (pc->pc_KnobType > AF_SIGNAL_TYPE_MAX) {
                            errcode = PATCH_ERR_BAD_SIGNAL_TYPE;
                            PPCERR ((ppi->ppi_PatchCmd, errcode, "Signal type %u undefined", pc->pc_KnobType));
                            goto clean;
                        }
                      #endif

                            /* build DSPPResource */
                        drsc.drsc_Type    = DRSC_TYPE_KNOB;
                        drsc.drsc_SubType = pc->pc_KnobType;
                        drsc.drsc_Flags   = DRSC_F_INIT_AT_ALLOC;
                        drsc.drsc_Many    = pc->pc_NumParts;
                        drsc.drsc_Default = dsppClipRawValue (pc->pc_KnobType, ConvertFP_SF15 (ConvertAudioSignalToGeneric (pc->pc_KnobType, 1, pc->pc_DefaultValue)));
                    }
                    goto addport;

                addport:
                    *drscptr++ = drsc;
                    nameptr = AppendPackedString (nameptr, ppi->ppi.n_Name);
                    break;

                /* @@@ the only way we could get any other PATCH_CMD_ here is if the
                       client's PatchCmd list changed between passes. */
            }
        }
    }

        /* success */
    *resultPortDTmp = portdtmp;
    return 0;

clean:
    dsppDeleteUserTemplate (portdtmp);
    return errcode;
}


/* -------------------- expose ports */

static Err ExposePort (PatchData *, const PatchCmdExpose *, PatchResourceInfo *);
static Err CheckResourceExposability (const DSPPResource *);
static Err ExposeEnvHook (PatchData *, const PatchCmdExpose *, const PatchTemplateInfo *, const DSPPEnvHookRsrcInfo *);
static Err ExposeEnvHookPart (PatchData *, const PatchResourceInfo *, const char *envHookName, const char *suffix);

/*
    Parse PATCH_CMD_EXPOSE commands.
    Expects DeletePatchData() to clean up after partial success.
*/
static Err ppcmdExpose (PatchParserData *ppd)
{
    const PatchCmd *pci, *pcstate;
    Err errcode;

  #if DEBUG_Parse
    printf ("ppcmdExpose:\n");
  #endif

    for (pcstate = ppd->ppd_PatchCmdList; pci = NextPatchCmd (&pcstate); ) {
        if (pci->pc_Generic.pc_CmdID == PATCH_CMD_EXPOSE) {
            const PatchCmdExpose * const pc = &pci->pc_Expose;
            const PatchTemplateInfo *pti;
            DSPPEnvHookRsrcInfo deri;
            int32 rsrcIndex;

          #if DEBUG_Parse
            DumpPatchCmd (pci, NULL);
          #endif

                /* find PatchTemplateInfo containing thing to expose */
            if (!(pti = (PatchTemplateInfo *)FindNamedNode (&ppd->ppd_PatchData->pd_TemplateList, pc->pc_SrcBlockName))) {
                errcode = PATCH_ERR_NAME_NOT_FOUND;
                PPCERR ((pci, errcode, "Block \"%s\" not found in patch", pc->pc_SrcBlockName));
                goto clean;
            }

                /* check exposed name legality. A further check for envelope hook name length
                   limitations is done later if necessary. */
            if ((errcode = CheckPortNameLegality (pc->pc_PortName)) < 0) {
                PPCERR ((pci, errcode, "Port name \"%s\" invalid", pc->pc_PortName));
                goto clean;
            }

                /* does a resource by this name exist? */
            if ((rsrcIndex = dsppFindResourceIndex (pti->pti_DTmp, pc->pc_SrcPortName)) >= 0) {
                if ((errcode = ExposePort (ppd->ppd_PatchData, pc, &pti->pti_ResourceInfo[rsrcIndex])) < 0) goto clean;
            }
                /* if not, see if there's an envelope by that name */
            else if ((errcode = dsppFindEnvHookResources (&deri, sizeof deri, pti->pti_DTmp, pc->pc_SrcPortName)) >= 0) {
                if ((errcode = ExposeEnvHook (ppd->ppd_PatchData, pc, pti, &deri)) < 0) goto clean;
            }
                /* otherwise, handle port not found (or possibly no memory from dsppFindEnvelopeResources) error */
            else {
                    /* !!! could print Template Item's n_Name field instead of block name (be sure to check NODE_NAMEVALID first) */
                PPCERR ((pci, errcode, "Port \"%s\" not member of block \"%s\"", pc->pc_SrcPortName, pc->pc_SrcBlockName));
                goto clean;
            }
        }
    }

    return 0;

clean:
    return errcode;
}

/*
    Expose a port (as opposed to an envelope hook).

    Inputs
        pd

        pc
            PATCH_CMD_EXPOSE command being processed. pc_PortName must
            have already been tested for legality. This function (or rather
            a call to ExposeResource()) checks for uniqueness.

        pri
            PatchResourceInfo for resource named by pc_SrcPortName to expose.
            This function checks the legality of the resource type for exposure.

    Results
        0 on success, error code on failure.
*/
static Err ExposePort (PatchData *pd, const PatchCmdExpose *pc, PatchResourceInfo *pri)
{
    Err errcode;

  #if DEBUG_Expose
    printf ("ExposePort() block '%s', port '%s' -> '%s'\n", pc->pc_SrcBlockName, pc->pc_SrcPortName, pc->pc_PortName);
  #endif

        /* make sure its exposable */
    if ((errcode = CheckResourceExposability (pri->pri_DRsc)) < 0) {
        PPCERR (((PatchCmd *)pc, errcode, "Port \"%s\" not exposable", pc->pc_SrcPortName));
        goto clean;
    }

        /* expose resource name (does uniqueness test, but doesn't check exposability) */
    if ((errcode = ExposeResource (pd, pri, pc->pc_PortName, PATCH_RESOURCE_NAME_F_UNIQUE | PATCH_RESOURCE_NAME_F_CONST)) < 0) {
        PPCERR (((PatchCmd *)pc, errcode, "Unable to expose port \"%s\" as \"%s\"", pc->pc_SrcPortName, pc->pc_PortName));
        goto clean;
    }

  #if 0 /* @@@ superfluous now, but necessary if Knob, Input, and Output arrays become exposable */
        /* success, prevent this port from being connected */
    ClearPatchResourceConnectability (pri);
  #endif

    return 0;

clean:
    return errcode;
}

/*
    Check resource exposability. Note that because this is only called by ExposePort()
    it doesn't have to deal with envelope hooks.

    Inputs
        drsc
            DSPPResource to check

    Results
        0 on success, Err code if not exposable.
*/
static Err CheckResourceExposability (const DSPPResource *drsc)
{
    switch (drsc->drsc_Type) {
        case DRSC_TYPE_TRIGGER:
        case DRSC_TYPE_IN_FIFO:
        case DRSC_TYPE_OUT_FIFO:
            return 0;

        default:
            return PATCH_ERR_BAD_PORT_TYPE;
    }
}


/*
    Expose an envelope hook by exposing the 4 resources that constitute it.

    Inputs
        pd

        pc
            PATCH_CMD_EXPOSE being processed.

            pc_PortName must have already been tested for basic legality.
            It is further checked agains the maximum legal envelope hook
            name length. This function (or rather calls to ExposeResource())
            checks for uniqueness of each component resource. This function
            also adds pc_PortName to the exposed name list to ensure uniqueness
            of the envelope hook name itself.

        pti
            PatchTemplateInfo for pc_SrcBlockName.

        deri
            DSPPEnvHookRsrcInfo containing validated envelope hook resource
            indeces to expose.

    Results
        0 on success, error code on failure.
*/
static Err ExposeEnvHook (PatchData *pd, const PatchCmdExpose *pc, const PatchTemplateInfo *pti, const DSPPEnvHookRsrcInfo *deri)
{
    Err errcode;
    /* !!! cache pc->pc_PortName? */

  #if DEBUG_Expose
    printf ("ExposeEnvHook() block '%s', port '%s' -> '%s'\n", pc->pc_SrcBlockName, pc->pc_SrcPortName, pc->pc_PortName);
  #endif

        /* check name length */
    if (strlen(pc->pc_PortName) > ENV_MAX_NAME_LENGTH) {
        errcode = PATCH_ERR_NAME_TOO_LONG;
        goto clean;
    }

        /* add pc_PortName to exposed name list to prevent anything else from colliding */
    {
        PatchResourceNameInfo *pni;

        if ((errcode = AddPatchResourceNameInfo (&pni, pd, pc->pc_PortName, PATCH_RESOURCE_NAME_F_UNIQUE | PATCH_RESOURCE_NAME_F_CONST)) < 0) goto clean;
    }

        /* expose hook parts */
    if ((errcode = ExposeEnvHookPart (pd, &pti->pti_ResourceInfo[deri->deri_RequestRsrcIndex], pc->pc_PortName, ENV_SUFFIX_REQUEST)) < 0) goto clean;
    if ((errcode = ExposeEnvHookPart (pd, &pti->pti_ResourceInfo[deri->deri_IncrRsrcIndex],    pc->pc_PortName, ENV_SUFFIX_INCR)) < 0) goto clean;
    if ((errcode = ExposeEnvHookPart (pd, &pti->pti_ResourceInfo[deri->deri_TargetRsrcIndex],  pc->pc_PortName, ENV_SUFFIX_TARGET)) < 0) goto clean;
    if ((errcode = ExposeEnvHookPart (pd, &pti->pti_ResourceInfo[deri->deri_CurrentRsrcIndex], pc->pc_PortName, ENV_SUFFIX_CURRENT)) < 0) goto clean;

        /* success */
    return 0;

clean:
    PPCERR (((PatchCmd *)pc, errcode, "Unable to expose envelope hook \"%s\" as \"%s\"", pc->pc_SrcPortName, pc->pc_PortName));
    return errcode;
}

/*
    Expose one of an envelope's resources

    Inputs
        pd
        pri
            PatchResourceInfo of an envelope's resource to expose. Assumed to be valid.

        envHookName
            Exposed envelope hook name (w/o suffix).
            Assumes that strlen(envHookName) <= ENV_MAX_NAME_LENGTH.

        suffix
            Envelope resource suffix to tack onto envHookName

    Results
        0 on success, error code on failure.
*/
static Err ExposeEnvHookPart (PatchData *pd, const PatchResourceInfo *pri, const char *envHookName, const char *suffix)
{
    char envRsrcNameBuf [AF_MAX_NAME_SIZE];
    Err errcode;

        /* get env hook part name */
    strcpy (envRsrcNameBuf, envHookName);
    strcat (envRsrcNameBuf, suffix);

        /* expose resource name (does uniqueness test, but doesn't check exposability) */
    if ((errcode = ExposeResource (pd, pri, envRsrcNameBuf, PATCH_RESOURCE_NAME_F_UNIQUE)) < 0) goto clean;

        /* success, prevent this port from being connected */
    ClearPatchResourceConnectability (pri);

    return 0;

clean:
    return errcode;
}


/* -------------------- connect ports */

static Err ParseCmdConnect (PatchData *, const PatchCmdConnect *);
static Err ParseCmdSetConstant (PatchData *, const PatchCmdSetConstant *);
static Err CheckUnusedPatchTemplatePorts (const PatchTemplateInfo *);
static void ConstantifyUnusedPatchTemplateDests (PatchTemplateInfo *);
static void pdiConnect (PatchDestInfo *, const PatchTemplateInfo *srcpti, uint16 srcRsrcIndex, uint16 srcPartNum);
static void pdiSetConstant (PatchDestInfo *, int32 constValue);

/*
    Parse PATCH_CMD_CONNECT and PATCH_CMD_SET_CONSTANT commands.
    Expects DeletePatchData() to clean up after partial success.
*/
static Err ppcmdConnectAndSetConstants (PatchParserData *ppd)
{
    PatchData * const pd = ppd->ppd_PatchData;
    Err errcode;

  #if DEBUG_Parse
    printf ("ppcmdConnectAndSetConstants:\n");
  #endif

        /* process PatchCmds */
    {
        const PatchCmd *pci, *pcstate;

        for (pcstate = ppd->ppd_PatchCmdList; pci = NextPatchCmd (&pcstate); ) {
            switch (pci->pc_Generic.pc_CmdID) {
                case PATCH_CMD_CONNECT:
                  #if DEBUG_Parse
                    DumpPatchCmd (pci, NULL);
                  #endif
                    if ((errcode = ParseCmdConnect (pd, &pci->pc_Connect)) < 0) goto clean;
                    break;

                case PATCH_CMD_SET_CONSTANT:
                  #if DEBUG_Parse
                    DumpPatchCmd (pci, NULL);
                  #endif
                    if ((errcode = ParseCmdSetConstant (pd, &pci->pc_SetConstant)) < 0) goto clean;
                    break;
            }
        }
    }

        /* make sure all patch inputs and outputs are connected to something */
    if (pd->pd_PortTemplate) {
        if ((errcode = CheckUnusedPatchTemplatePorts (pd->pd_PortTemplate)) < 0) goto clean;
    }

        /* convert all unused instrument inputs to constants */
    {
        PatchTemplateInfo *pti;

        SCANLIST (&pd->pd_TemplateList, pti, PatchTemplateInfo) {
            ConstantifyUnusedPatchTemplateDests (pti);
        }
    }

    return 0;

clean:
    return errcode;
}

/*
    Parse a PATCH_CMD_CONNECT command.
    Expects DeletePatchData() to clean up after partial success.
*/
static Err ParseCmdConnect (PatchData *pd, const PatchCmdConnect *pc)
{
    PatchTemplateInfo *srcpti, *dstpti;
    uint16 srcRsrcIndex, dstRsrcIndex;
    PatchResourceInfo *srcpri, *dstpri;
    Err errcode;

        /* find source and destination blocks (name==NULL means to use pd_PortTemplate, which might also be NULL) */
    if (pc->pc_FromBlockName) {
        if (!(srcpti = (PatchTemplateInfo *)FindNamedNode (&pd->pd_TemplateList, pc->pc_FromBlockName))) {
            errcode = PATCH_ERR_NAME_NOT_FOUND;
            PPCERR (((PatchCmd *)pc, errcode, "Source block \"%s\" not found in patch", pc->pc_FromBlockName));
            goto clean;
        }
    }
    else {
        if (!(srcpti = pd->pd_PortTemplate)) {
            errcode = PATCH_ERR_NAME_NOT_FOUND;
            PPCERR (((PatchCmd *)pc, errcode, "Source port \"%s\" not defined in patch", pc->pc_FromPortName));
            goto clean;
        }
    }
    if (pc->pc_ToBlockName) {
        if (!(dstpti = (PatchTemplateInfo *)FindNamedNode (&pd->pd_TemplateList, pc->pc_ToBlockName))) {
            errcode = PATCH_ERR_NAME_NOT_FOUND;
            PPCERR (((PatchCmd *)pc, errcode, "Destination block \"%s\" not found in patch", pc->pc_ToBlockName));
            goto clean;
        }
    }
    else {
        if (!(dstpti = pd->pd_PortTemplate)) {
            errcode = PATCH_ERR_NAME_NOT_FOUND;
            PPCERR (((PatchCmd *)pc, errcode, "Destination port \"%s\" not defined in patch", pc->pc_ToPortName));
            goto clean;
        }
    }

        /* find source and destination resources */
    {
        int32 t;

        if ((errcode = t = dsppFindResourceIndex (srcpti->pti_DTmp, pc->pc_FromPortName)) < 0) {
            PPCERR (((PatchCmd *)pc, errcode,
                pc->pc_FromBlockName
                    ? "Source port \"%s\" not member of block \"%s\""
                    : "Source port \"%s\" not defined in patch",
                pc->pc_FromPortName, pc->pc_FromBlockName));
            goto clean;
        }
        srcRsrcIndex = t;
        if ((errcode = t = dsppFindResourceIndex (dstpti->pti_DTmp, pc->pc_ToPortName)) < 0) {
            PPCERR (((PatchCmd *)pc, errcode,
                pc->pc_ToBlockName
                    ? "Destination port \"%s\" not member of block \"%s\""
                    : "Destination port \"%s\" not defined in patch",
                pc->pc_ToPortName, pc->pc_ToBlockName));
            goto clean;
        }
        dstRsrcIndex = t;
    }
    srcpri = &srcpti->pti_ResourceInfo[srcRsrcIndex];
    dstpri = &dstpti->pti_ResourceInfo[dstRsrcIndex];

        /* validate resource types */
        /* source port must be PATCH_RESOURCE_CONNECTABLE_SRC */
        /* @@@ dest might not be an input because it got exposed - making this error message a bit confusing */
    if (srcpri->pri_Connectability != PATCH_RESOURCE_CONNECTABLE_SRC) {
        errcode = PATCH_ERR_BAD_PORT_TYPE;
        PPCERR (((PatchCmd *)pc, errcode,
            pc->pc_FromBlockName
                ? "Source port \"%s\" not an output port of block \"%s\""
                : "Source port \"%s\" not a patch input",
            pc->pc_FromPortName, pc->pc_FromBlockName));
        goto clean;
    }
        /* dest port must be PATCH_RESOURCE_CONNECTABLE_DEST */
        /* @@@ dest might not be an input because it got exposed - making this error message a bit confusing */
    if (dstpri->pri_Connectability != PATCH_RESOURCE_CONNECTABLE_DEST) {
        errcode = PATCH_ERR_BAD_PORT_TYPE;
        PPCERR (((PatchCmd *)pc, errcode,
            pc->pc_ToBlockName
                ? "Destination port \"%s\" not an input port of block \"%s\""
                : "Destination port \"%s\" not a patch output",
            pc->pc_ToPortName, pc->pc_ToBlockName));
        goto clean;
    }

        /* bounds check source and destination parts */
    if (pc->pc_FromPartNum >= srcpri->pri_DRsc->drsc_Many) {
        errcode = PATCH_ERR_OUT_OF_RANGE;
        PPCERR (((PatchCmd *)pc, errcode, "Source part %u out of range", pc->pc_FromPartNum));
        goto clean;
    }
    if (pc->pc_ToPartNum >= dstpri->pri_DRsc->drsc_Many) {
        errcode = PATCH_ERR_OUT_OF_RANGE;
        PPCERR (((PatchCmd *)pc, errcode, "Destination part %u out of range", pc->pc_ToPartNum));
        goto clean;
    }

        /* attempt connection */
    {
        PatchDestInfo * const pdi = &dstpri->pri_DestInfo [pc->pc_ToPartNum];

            /* make sure that dst part hasn't already been connected to or been set as a constant */
        if (pdi->pdi_State != PATCH_DEST_STATE_UNUSED) {
            errcode = PATCH_ERR_PORT_IN_USE;
            ppcerrDestAlreadyUsed ((PatchCmd *)pc, errcode, dstpti, dstRsrcIndex, pc->pc_ToPartNum);
            goto clean;
        }

            /* set pdi to PATCH_DEST_STATE_CONNECTED */
        pdiConnect (pdi, srcpti, srcRsrcIndex, pc->pc_FromPartNum);

            /* if a move is required for this connection, increment NumMoves */
        if (dstpri->pri_Flags & PATCH_RESOURCE_F_DEST_NEEDS_MOVE) dstpti->pti_NumMoves++;
    }

    return 0;

clean:
    return errcode;
}


/*
    Parse a PATCH_CMD_SET_CONSTANT command.
    Expects DeletePatchData() to clean up after partial success.

    Note: Unlike Connect, this doesn't support setting constants for the
          patch's outputs.
*/
static Err ParseCmdSetConstant (PatchData *pd, const PatchCmdSetConstant *pc)
{
    PatchTemplateInfo *dstpti;
    uint16 dstRsrcIndex;
    const DSPPResource *dstdrsc;
    PatchResourceInfo *dstpri;
    Err errcode;

        /* find block */
    if ((errcode = CheckBlockNameLegality (pc->pc_BlockName)) < 0) {
        PPCERR (((PatchCmd *)pc, errcode, "Block name \"%s\" invalid", pc->pc_BlockName));
        goto clean;
    }
    if (!(dstpti = (PatchTemplateInfo *)FindNamedNode (&pd->pd_TemplateList, pc->pc_BlockName))) {
        errcode = PATCH_ERR_NAME_NOT_FOUND;
        PPCERR (((PatchCmd *)pc, errcode, "Block \"%s\" not found in patch", pc->pc_BlockName));
        goto clean;
    }

        /* find resource */
    {
        int32 t;

        if ((errcode = t = dsppFindResourceIndex (dstpti->pti_DTmp, pc->pc_PortName)) < 0) {
            PPCERR (((PatchCmd *)pc, errcode, "Port \"%s\" not member of block \"%s\"", pc->pc_PortName, pc->pc_BlockName));
            goto clean;
        }
        dstRsrcIndex = t;
    }
    dstpri  = &dstpti->pti_ResourceInfo[dstRsrcIndex];
    dstdrsc = dstpri->pri_DRsc;

        /* validate resource type */
        /* port must be PATCH_RESOURCE_CONNECTABLE_DEST */
        /* @@@ dest might not be an input because it got exposed - making this error message a bit confusing */
    if (dstpri->pri_Connectability != PATCH_RESOURCE_CONNECTABLE_DEST) {
        errcode = PATCH_ERR_BAD_PORT_TYPE;
        PPCERR (((PatchCmd *)pc, errcode, "Port \"%s\" not an input port of block \"%s\"", pc->pc_PortName, pc->pc_BlockName));
        goto clean;
    }

        /* bounds check part */
    if (pc->pc_PartNum >= dstdrsc->drsc_Many) {
        errcode = PATCH_ERR_OUT_OF_RANGE;
        PPCERR (((PatchCmd *)pc, errcode, "Part %u out of range", pc->pc_PartNum));
        goto clean;
    }

        /* attempt connection */
    {
        PatchDestInfo * const pdi = &dstpri->pri_DestInfo [pc->pc_PartNum];

            /* make sure that dst part hasn't already been connected to or been set as a constant */
        if (pdi->pdi_State != PATCH_DEST_STATE_UNUSED) {
            errcode = PATCH_ERR_PORT_IN_USE;
            ppcerrDestAlreadyUsed ((PatchCmd *)pc, errcode, dstpti, dstRsrcIndex, pc->pc_PartNum);
            goto clean;
        }

            /* set pdi to PATCH_DEST_STATE_CONSTANT */
        pdiSetConstant (pdi, dsppClipRawValue (dstdrsc->drsc_SubType, ConvertFP_SF15 (ConvertAudioSignalToGeneric (dstdrsc->drsc_SubType, 1, pc->pc_ConstantValue))));
    }

    return 0;

clean:
    return errcode;
}


/*
    Check PatchTemplateInfo for unused ports

    !!! add trap for unconnected inputs (currently only finds unused PATCH_RESOURCE_CONNECTABLE_DESTs)
*/
static Err CheckUnusedPatchTemplatePorts (const PatchTemplateInfo *pti)
{
    int32 rsrcIndex;
    Err errcode;

    for (rsrcIndex = 0; rsrcIndex < pti->pti_NumResources; rsrcIndex++) {
        const PatchResourceInfo * const pri = &pti->pti_ResourceInfo[rsrcIndex];

        switch (pri->pri_Connectability) {
            case PATCH_RESOURCE_CONNECTABLE_DEST:
                {
                    int32 partNum;

                    for (partNum = 0; partNum < pri->pri_DRsc->drsc_Many; partNum++) {
                        const PatchDestInfo * const pdi = &pri->pri_DestInfo[partNum];

                        if (pdi->pdi_State == PATCH_DEST_STATE_UNUSED) {
                            errcode = PATCH_ERR_PORT_NOT_USED;
                          #ifdef BUILD_STRINGS
                            {
                                const char *portName = dsppGetTemplateRsrcName (pti->pti_DTmp, rsrcIndex);

                                    /* !!! some of this diagnostic really ought to be common */
                                    /* !!! only used for port template at the moment, so both cases aren't really necessary */
                                if (pti->pti.n_Type == PATCH_TEMPLATE_TYPE_PORT)
                                    PPCERR ((NULL, errcode, "Patch port \"%s\", part %u unused", portName, partNum));
                                else
                                    PPCERR ((NULL, errcode, "Block \"%s\", port \"%s\", part %u unused", pti->pti.n_Name, portName, partNum));
                            }
                          #endif
                            goto clean;
                        }
                    }
                }
                break;

          #if 0
            case PATCH_RESOURCE_CONNECTABLE_SRC:
                /* !!! need to check for unused SRCs as well */
                break;
          #endif
        }

    }
    return 0;

clean:
    return errcode;
}

/*
    Convert all PATCH_DEST_STATE_UNUSED to PATCH_DEST_STATE_CONSANT
    using value from DINI or drsc_Default
*/
static void ConstantifyUnusedPatchTemplateDests (PatchTemplateInfo *pti)
{
    int32 rsrcIndex;

    for (rsrcIndex = 0; rsrcIndex < pti->pti_NumResources; rsrcIndex++) {
        const PatchResourceInfo * const pri = &pti->pti_ResourceInfo[rsrcIndex];

        if (pri->pri_Connectability == PATCH_RESOURCE_CONNECTABLE_DEST) {
            const DSPPResource * const drsc = pri->pri_DRsc;
            const DSPPDataInitializer * const dini = pri->pri_DataInitializer;
            int32 partNum;

            for (partNum = 0; partNum < drsc->drsc_Many; partNum++) {
                PatchDestInfo * const pdi = &pri->pri_DestInfo[partNum];

                if (pdi->pdi_State == PATCH_DEST_STATE_UNUSED) {
                    int32 constval;

                    if (dini && partNum < dini->dini_Many)
                        constval = dsppGetDataInitializerImage(dini)[partNum];
                    else if (drsc->drsc_Flags & (DRSC_F_INIT_AT_ALLOC | DRSC_F_INIT_AT_START))
                        constval = drsc->drsc_Default;
                    else
                        constval = 0;

                    pdiSetConstant (pdi, constval);
                }
            }
        }
    }
}


/*
    Set PatchDestInfo to PATCH_DEST_STATE_CONNECTED.
    Assumes that pdi not been changed since initialized to PATCH_DEST_STATE_UNUSED.
*/
static void pdiConnect (PatchDestInfo *pdi, const PatchTemplateInfo *srcpti, uint16 srcRsrcIndex, uint16 srcPartNum)
{
    pdi->pdi_State                    = PATCH_DEST_STATE_CONNECTED;
    pdi->pdi.ConnectInfo.SrcTemplate  = srcpti;
    pdi->pdi.ConnectInfo.SrcRsrcIndex = srcRsrcIndex;
    pdi->pdi.ConnectInfo.SrcPartNum   = srcPartNum;
}

/*
    Set PatchDestInfo to PATCH_DEST_STATE_CONSTANT.
    Assumes that pdi not been changed since initialized to PATCH_DEST_STATE_UNUSED.
*/
static void pdiSetConstant (PatchDestInfo *pdi, int32 constValue)
{
    pdi->pdi_State           = PATCH_DEST_STATE_CONSTANT;
    pdi->pdi.ConstInfo.Value = constValue;
}


/* -------------------- PatchData management */

/*
    Create empty PatchData structure
*/
static PatchData *CreatePatchData (void)
{
    PatchData *pd;

    if (pd = AllocMem (sizeof *pd, MEMTYPE_ANY | MEMTYPE_FILL)) {
        PrepList (&pd->pd_TemplateList);
        PrepList (&pd->pd_ResourceNameList);
        pd->pd_Flags = PATCH_DATA_F_INTER_INSTRUMENT_PADDING;
    }

    return pd;
}

/*
    Delete PatchData and all associated structures

    Inputs
        pd
            PatchData to delete. Can be NULL.
*/
void dsppDeletePatchData (PatchData *pd)
{
    if (pd) {
            /* empty pd_TemplateList */
        {
            PatchTemplateInfo *pti, *next;

            PROCESSLIST (&pd->pd_TemplateList, pti, next, PatchTemplateInfo) {
                DeletePatchTemplateInfo (pti);
            }
        }

            /* free port template */
        DeletePatchTemplateInfo (pd->pd_PortTemplate);
        dsppDeleteUserTemplate (pd->pd_PortDTmp);

            /* empty pd_ResourceNameList */
            /* @@@ do last because other things above point to it */
        {
            PatchResourceNameInfo *pni, *next;

            PROCESSLIST (&pd->pd_ResourceNameList, pni, next, PatchResourceNameInfo) {
                DeletePatchResourceNameInfo (pni);
            }
        }

        FreeMem (pd, sizeof *pd);
    }
}


/* -------------------- PatchTemplateInfo management */

static uint8 GetConnectability (uint8 ptiType, uint8 rsrcType);

/*
    Creates a PatchTemplateInfo from supplied info.

    Inputs
        type
            PATCH_TEMPLATE_TYPE_ to store in pti.n_Type. Assumed to be valid.
            This affects the way PatchDestInfo pdi_Connectability is set. When
            PATCH_TEMPLATE_TYPE_PORT, flips the normal association between
            resource type and connectability around so that patch outputs
            are PATCH_DEST_CONNECTABLE_DEST and patch inputs are
            PATCH_DEST_CONNECTABLE_SRC.

        name
            Name to place in pti.n_Name. Not copied. Can be NULL.

        dtmp
            Pointer to DSPPTemplate to place in pti_DTmp. This can be either
            the supervisor DSPPTemplate from a aitp_DeviceTemplate from a
            template item or a user mode DSPPTemplate (e.g., port template).

        @@@ might need item or AudioInsTemplate of thing containing dtmp some day

    Results
        Returns pointer to PatchTemplateInfo on success, NULL if out of memory.
*/
static PatchTemplateInfo *CreatePatchTemplateInfo (uint8 ptiType, const char *ptiName, const DSPPTemplate *dtmp)
{
    const int32 ptiSize = sizeof (PatchTemplateInfo) + dtmp->dtmp_NumResources * sizeof (PatchResourceInfo);
    PatchTemplateInfo *pti;

        /* single alloc of of PatchTemplateInfo, followed by pti_ResourceInfo[] */
    if (!(pti = AllocMem (ptiSize, MEMTYPE_ANY | MEMTYPE_FILL))) goto clean;

        /* init simple stuff */
    pti->pti.n_Type      = ptiType;
    pti->pti.n_Size      = ptiSize;
    pti->pti.n_Name      = (char *)ptiName;     /* @@@ potential casting problem */
#if 0   /* @@@ not used yet */
    pti->pti_InsTemplate = insTemplate;         /* @@@ might need item or AudioInsTemplate pointer for dtmp's container */
#endif
    pti->pti_DTmp        = dtmp;

        /* init pti_ResourceInfo[]: PATCH_RESOURCE_F flags, connectability */
    if (dtmp->dtmp_NumResources) {
        pti->pti_ResourceInfo = (PatchResourceInfo *)(pti + 1);
        pti->pti_NumResources = dtmp->dtmp_NumResources;

            /* initialize ResourceInfo array */
        {
            int32 rsrcIndex;

            for (rsrcIndex=0; rsrcIndex<pti->pti_NumResources; rsrcIndex++) {
                const DSPPResource * const drsc = &dtmp->dtmp_Resources[rsrcIndex];
                PatchResourceInfo * const pri = &pti->pti_ResourceInfo[rsrcIndex];

                /* !!! validate a bit?
                    . don't allow DRSC_F_EXPORT
                    . don't allow DRSC_F_BIND code resources (which really only makes sense for DRSC_F_EXPORTED things)
                    --- might add this sort of validation to dsppValidateTemplate() and just assume that things are kosher
                    . might check binding to make sure I'm keeping the thing to which the resource is bound
                    --- no point in doing this here, but probably worth checking when dereferencing resource thru TargetRsrcIndex
                */

                    /* initialize resource pointer */
                pri->pri_DRsc = drsc;

                    /* make initial decision whether to keep this resource */
                if (drsc->drsc_Type == DRSC_TYPE_CODE && !(drsc->drsc_Flags & (DRSC_F_BIND | DRSC_F_IMPORT))) {
                    /* template's code allocation resource. don't keep. */
                }
                else if (drsc->drsc_Type == DRSC_TYPE_TICKS) {
                    /* template's tick allocation resource. don't keep. */
                }
                else {
                    /* keep everything else */
                    pri->pri_Flags |= PATCH_RESOURCE_F_KEEP;
                }

                    /* determine connectability (@@@ requires that pri_DRsc be set) */
                if (!SetPatchResourceConnectability (pri, ptiType, GetConnectability (ptiType, drsc->drsc_Type))) goto clean;
            }
        }

            /* match DINIs with ResourceInfos */
        if (dtmp->dtmp_DataInitializerSize) {
            const DSPPDataInitializer *dini = dtmp->dtmp_DataInitializer;
            const DSPPDataInitializer * const dini_end = (DSPPDataInitializer *)((char *)dtmp->dtmp_DataInitializer + dtmp->dtmp_DataInitializerSize);

            for (; dini < dini_end; dini = dsppNextDataInitializer(dini)) {
                pti->pti_ResourceInfo [dini->dini_RsrcIndex].pri_DataInitializer = dini;
            }
        }
    }

        /* success */
    return pti;

clean:
    DeletePatchTemplateInfo (pti);
    return NULL;
}

/*
    Deletes a PatchTemplateInfo.

    Deletes all contained PatchDestInfos for this PatchTemplateInfo.

    Inputs
        pti
            PatchTemplateInfo to delete. Can be NULL.
*/
static void DeletePatchTemplateInfo (PatchTemplateInfo *pti)
{
    if (pti) {
            /* clear connectability */
        {
            int32 i;

            for (i=0; i<pti->pti_NumResources; i++) {
                ClearPatchResourceConnectability (&pti->pti_ResourceInfo[i]);
            }
        }
            /* free pti and pti_ResourceInfo[] (single allocation) */
        FreeMem (pti, pti->pti.n_Size);
    }
}


/*
    Generate suitable PATCH_RESOURCE_CONNECTABILITY for given PATCH_TEMPLATE_TYPE_
    and DRSC_ type.
*/
static uint8 GetConnectability (uint8 ptiType, uint8 rsrcType)
{
    switch (rsrcType) {
        case DRSC_TYPE_INPUT:
        case DRSC_TYPE_KNOB:
            return (ptiType == PATCH_TEMPLATE_TYPE_PORT)
                ? PATCH_RESOURCE_CONNECTABLE_SRC
                : PATCH_RESOURCE_CONNECTABLE_DEST;

        case DRSC_TYPE_OUTPUT:
            return (ptiType == PATCH_TEMPLATE_TYPE_PORT)
                ? PATCH_RESOURCE_CONNECTABLE_DEST
                : PATCH_RESOURCE_CONNECTABLE_SRC;

        default:
            return PATCH_RESOURCE_NOT_CONNECTABLE;
    }
}


/* -------------------- PatchResourceInfo/PatchDestInfo management */

/*
    Set pri_Connectability and initialize associated fields.

    Inputs
        pri
            Partially initialized pri.
                pri_DRsc must be set correctly.
                pri_Connectability is assumed to be PATCH_RESOURCE_NOT_CONNECTABLE.

        ptiType
            PATCH_TEMPLATE_TYPE_ of pti that owns this pri

        connectability
            One of PATCH_RESOURCE_CONNECTABLE_SRC,
            PATCH_RESOURCE_CONNECTABLE_DEST, PATCH_RESOURCE_NOT_CONNECTABLE.

    Results
        TRUE on success, FALSE if insufficient memory.
        Sets
            pri_Connectability
            pri_DestInfo
                Allocates and initializes DestInfo array
            pri_Flags
                Sets appropriate PATCH_RESOURCE_F_DEST_ flags
*/
static bool SetPatchResourceConnectability (PatchResourceInfo *pri, uint8 ptiType, uint8 connectability)
{
        /* handle special stuff for dest resources */
    if (connectability == PATCH_RESOURCE_CONNECTABLE_DEST) {

            /* set up pri_DestInfo (cleared is as initialized as it needs to be - how convenient) */
        if (!(pri->pri_DestInfo = AllocMem (pri->pri_DRsc->drsc_Many * sizeof (PatchDestInfo), MEMTYPE_ANY | MEMTYPE_FILL | MEMTYPE_TRACKSIZE))) {
            goto clean;
        }

            /* see whether connections to this dest require a MOVE */
            /* !!! it'd be nice if we could just sniff a DRSC_F_ flag instead */
        if (ptiType == PATCH_TEMPLATE_TYPE_PORT || pri->pri_DRsc->drsc_Flags & DRSC_F_BIND) {
            pri->pri_Flags |= PATCH_RESOURCE_F_DEST_NEEDS_MOVE;
        }

            /* determine if we can optimize this resource (i.e. are there things other than rlocs refering to it?) */
            /* !!! might need to know if ins expects arrays to remain arrays */
        if (ptiType != PATCH_TEMPLATE_TYPE_PORT && !(pri->pri_DRsc->drsc_Flags & (DRSC_F_BIND | DRSC_F_BOUND_TO))) {
            pri->pri_Flags |= PATCH_RESOURCE_F_DEST_OPTIMIZE;
        }
    }

        /* set connectability */
    pri->pri_Connectability = connectability;

    return TRUE;

clean:
    return FALSE;
}

/*
    Clear connectability set by SetPatchResourceConnectability()

    Inputs
        pri
            pri to affect

    Results
        Sets
            pri_DestInfo
                Frees DestInfo array
            pri_Flags
                Clears any dest flags
            pri_Connectability
                Sets to PATCH_RESOURCE_NOT_CONNECTABLE
*/
static void ClearPatchResourceConnectability (PatchResourceInfo *pri)
{
    if (pri->pri_Connectability == PATCH_RESOURCE_CONNECTABLE_DEST) {
        FreeMem (pri->pri_DestInfo, TRACKED_SIZE);
        pri->pri_DestInfo = NULL;
        pri->pri_Flags   &= ~PATCH_RESOURCE_F_DEST_FLAGS;
    }

    pri->pri_Connectability = PATCH_RESOURCE_NOT_CONNECTABLE;
}


/* -------------------- PatchResourceNameInfo management */

/*
    Expose a resource name.

    Adds an entry to pd_ResourceNameList and sets pri_TargetRsrcName for resource
    Does name uniqueness test, and double exposure test, but does not test
    exposability of resource type.

    Permits multiple imported resources to have the same name, because in this
    case the name is used to attach the resource to an exported resource; the
    name is special. Imported resources aren't allowed to share names with
    non-imported resources, however.

    Doesn't clean up after itself if it fails. Expects DeletePatchData() to take
    care of that.

    Inputs
        pd
            PatchData to work on

        pri
            PatchResourceInfo for resource to expose

        exposedName
            Name to expose resource as. Assumed to be valid. Uniqueness is tested.

        pniFlags
            PATCH_RESOURCE_NAME_F_ flags for AddPatchResourceNameInfo(). Even though this
            function can inspect the resource to see if it's imported (and therefore
            requires PATCH_RESOURCE_NAME_F_UNIQUE to be cleared), it expects the caller to
            do this (@@@ the only client that requires PATCH_RESOURCE_NAME_F_UNIQUE to
            be cleared is the thing that exposes imported resources, so it's a bit less
            executable code this way).

    Results
        Returns 0 on success, Err code on failure (lack of memory, non-unique name)
        Adds entry to pd_ResourceNameList on success, it not already there (see above).
        Sets associated pri_TargetRsrcName field on success.
*/
static Err ExposeResource (PatchData *pd, PatchResourceInfo *pri, const char *exposedName, uint8 pniFlags)
{
    const PatchResourceNameInfo *pni;
    Err errcode;

  #if DEBUG_Expose
    printf ("ExposeResource() '%s' 0x%02x\n", exposedName, pniFlags);
  #endif

        /* make sure it's not already exposed, perhaps under another name */
    if (pri->pri_TargetRsrcName) {
        errcode = PATCH_ERR_PORT_ALREADY_EXPOSED;
        goto clean;
    }

        /* add entry in exposed name list */
    if ((errcode = AddPatchResourceNameInfo (&pni, pd, exposedName, pniFlags)) < 0) goto clean;

        /* set ResourceInfo using allocated name from PatchNameResourceInfo */
    pri->pri_TargetRsrcName = pni->pni.n_Name;

    return 0;

clean:
    return errcode;
}

/*
    Allocate and add an entry to pd_ResourceNameList.

    Inputs
        resultpni
            Buffer to hold new (or found) pni. Must be valid.

        pd
            Contains pd_ResourceNameList to add to.

        name
            Name to add to list. If PATCH_RESOURCE_NAME_F_CONST is set, this pointer
            must remain valid for the life of the list. Otherwise, it may change after
            this call. Assumed to be valid.

        flags
            PATCH_RESOURCE_NAME_F_UNIQUE
                When set, this name must not already exist in the name list. If
                it does, PATCH_ERR_NAME_NOT_UNIQUE is returned. Also, when a name
                is added this way, no other instance of this name (unique or otherwise)
                is allowed to be added to the list.

                When cleared, the name may be requested to be added more than once.
                If it already exists, and the original record was added with this
                bit cleared, no new record is allocated and a pointer to the original
                record is returned. If the the original record has this flag set,
                PATCH_ERR_NAME_NOT_UNIQUE is returned just as if the order of adding
                were reversed.

            PATCH_RESOURCE_NAME_F_CONST
                When this is set, n_Name is set to point to name. Therefore, name must be
                constant for the life of the list. When this is cleared, n_Name is allocated
                and name is copied to the new buffer.

    Results
        . Returns 0 on success, error code on failure.
        . Writes the allocated or found PatchResourceNameInfo pointer on success,
          or NULL on failure, to *resultpni.
*/
static Err AddPatchResourceNameInfo (PatchResourceNameInfo **resultpni, PatchData *pd, const char *name, uint8 flags)
{
    PatchResourceNameInfo *pni;
    Err errcode;

        /* initialize result */
    *resultpni = NULL;

    if (pni = (PatchResourceNameInfo *)FindNamedNode (&pd->pd_ResourceNameList, name)) {
            /* if either existing or new have PATCH_RESOURCE_NAME_F_UNIQUE set, return error. */
        if ((pni->pni.n_Flags | flags) & PATCH_RESOURCE_NAME_F_UNIQUE) {
            errcode = PATCH_ERR_NAME_NOT_UNIQUE;
            goto clean;
        }
            /* otherwise, return pointer to this pni */
    }
    else {
        int32 pniSize = sizeof (PatchResourceNameInfo);

            /* if string won't remain constant, allocate space for a copy of it */
        if (!(flags & PATCH_RESOURCE_NAME_F_CONST)) {
            pniSize += strlen(name) + 1;
        }

            /* alloc/init PatchResourceNameInfo + conditionally string buffer */
        if (!(pni = (PatchResourceNameInfo *)AllocMem (pniSize, MEMTYPE_ANY | MEMTYPE_FILL))) {
            errcode = PATCH_ERR_NOMEM;
            goto clean;
        }
        pni->pni.n_Flags = flags;
        pni->pni.n_Size  = pniSize;

            /* set n_Name according to whether we allocated a copy, or are going to point to the original */
        if (flags & PATCH_RESOURCE_NAME_F_CONST) {
            pni->pni.n_Name = (char *)name;     /* @@@ const-breaking cast */
        }
        else {
            pni->pni.n_Name = (char *)(pni + 1);
            strcpy (pni->pni.n_Name, name);
        }

            /* add to name list */
        AddTail (&pd->pd_ResourceNameList, (Node *)pni);
    }

    *resultpni = pni;
    return 0;

clean:
    /* @@@ note: no allocation cleanup because there's only one way allocation path can fail: lack of memory */
    return errcode;
}

static void DeletePatchResourceNameInfo (PatchResourceNameInfo *pni)
{
    if (pni) {
        FreeMem (pni, pni->pni.n_Size);
    }
}


#ifdef BUILD_STRINGS    /* { */

/* -------------------- Diagnostics */

/*
    Print a PatchCmd parser error message.

    Inputs
        pc
            Pointer to PatchCmd causing the problem. Can be NULL.

        errcode
            Err code to print.
*/
static void PrintPatchCmdError (const PatchCmd *pc, Err errcode, const char *msgfmt, ...)
{
    #define PPCERR_PREFIX "### "

    if (pc) DumpPatchCmd (pc, PPCERR_PREFIX);

    if (msgfmt) {
        va_list ap;

        va_start (ap, msgfmt);

        printf (PPCERR_PREFIX);
        vprintf (msgfmt, ap);
        printf ("\n");

        va_end (ap);
    }

    printf (PPCERR_PREFIX);
    PrintfSysErr (errcode);
}

/*
    specialized shared error message used by Connect and SetConstant
*/
static void ppcerrDestAlreadyUsed (const PatchCmd *pc, Err errcode, const PatchTemplateInfo *dstpti, uint16 dstRsrcIndex, uint16 dstPartNum)
{
    const char * const dstPortName = dsppGetTemplateRsrcName (dstpti->pti_DTmp, dstRsrcIndex);
    const PatchDestInfo * const pdi = &dstpti->pti_ResourceInfo[dstRsrcIndex].pri_DestInfo[dstPartNum];

    switch (pdi->pdi_State) {
        case PATCH_DEST_STATE_CONNECTED:
            if (pdi->pdi.ConnectInfo.SrcTemplate->pti.n_Type == PATCH_TEMPLATE_TYPE_PORT) {
                PPCERR ((pc, errcode, "Patch port \"%s\", part %u already connected to port \"%s\", part %u",
                    dsppGetTemplateRsrcName (pdi->pdi.ConnectInfo.SrcTemplate->pti_DTmp, pdi->pdi.ConnectInfo.SrcRsrcIndex),
                    pdi->pdi.ConnectInfo.SrcPartNum,
                    dstPortName, dstPartNum));
            }
            else {
                PPCERR ((pc, errcode, "Block \"%s\", port \"%s\", part %u already connected to port \"%s\", part %u",
                    pdi->pdi.ConnectInfo.SrcTemplate->pti.n_Name,
                    dsppGetTemplateRsrcName (pdi->pdi.ConnectInfo.SrcTemplate->pti_DTmp, pdi->pdi.ConnectInfo.SrcRsrcIndex),
                    pdi->pdi.ConnectInfo.SrcPartNum,
                    dstPortName, dstPartNum));
            }
            break;

        case PATCH_DEST_STATE_CONSTANT:
            PPCERR ((pc, errcode, "Port \"%s\", part %u already set to a constant", dstPortName, dstPartNum));
            break;
    }
}

#endif  /* } BUILD_STRINGS */
