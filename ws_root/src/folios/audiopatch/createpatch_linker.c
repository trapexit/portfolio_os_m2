/******************************************************************************
**
**  @(#) createpatch_linker.c 96/07/19 1.55
**
**  DSP patch intstrument template builder - linker.
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
**  950717 WJB  Created.
**  950718 WJB  Added skeleton for dsppLinkPatchTemplate().
**              Implemented LinkPatchCode().
**  950720 WJB  Implemented LinkPatchResources().
**  950721 WJB  Implemented LinkPatchRelocations().
**  950802 WJB  Added placeholder init code for dtmp_Header.
**  950809 WJB  Removed support for old code-absolute relocation lists and old-style branches.
**  950809 WJB  Began data initializer implementation.
**  950810 WJB  Added LinkPatchDataInitializers().
**  950810 WJB  Revised part of LinkPatchRelocations() for PatchDestInfo.
**  950811 WJB  Completed LinkPatchRelocations() conversion to PatchDestInfo.
**  950828 WJB  Supports patches with zero relocations.
**  950828 WJB  Added triviality trap for zero resources.
**  950828 WJB  Added triviality trap for zero code or ticks.
**  950831 WJB  Recovered from addition of drsc_SubType.
**  950914 WJB  Implemented LinkPatchDynamicLinkNames().
**  950919 WJB  Changes resource type of internal Inputs, Knobs, and Outputs to Variable
**              to avoid logical problems such as writing to a knob with a _MOVE.
**  960226 WJB  Removed excessive inter-instrument and MOVE padding.
**              Corrected tick computations.
**  960227 WJB  Added support for optional inter-instrument padding.
**  960716 WJB  Rearranged resource table so that ticks come first, followed by code.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <audio/handy_macros.h>         /* packed string */
#include <dspptouch/dspp_instructions.h>
#include <kernel/mem.h>

#include "createpatch_internal.h"


/* -------------------- Debug */

#define DEBUG_LinkCode              0   /* debug LinkPatchCode() */
#define DEBUG_LinkResources         0   /* debug LinkPatchResources() */
#define DEBUG_LinkDataInitializers  0   /* debug LinkPatchDataInitializers() */
#define DEBUG_LinkRelocations       0   /* debug LinkPatchRelocations() */
#define DEBUG_LinkDynamicLinkNames  0   /* debug LinkPatchDynamicLinkNames() */
#define DEBUG_Summary               0   /* print out patch resource summary */


/* -------------------- Misc defines */

#define PATCH_MAX_NUM_RESOURCES 0xffff          /* (16 bits) max number of resources we can tolerate in a patch */


/* -------------------- Local data structures */

    /* data structure used during linking */
typedef struct PatchLinkerData {
        /* constants */
    PatchData  *pld_PatchData;                  /* input PatchData structure */
    DSPPTemplate *pld_DTmp;                     /* DSPPTemplate under construction */

        /* computed during LinkPatchCode(), and read by LinkPatchResources() */
    uint16      pld_TotalCodeSize;              /* Total code size */
    uint16      pld_TotalTicks;                 /* Total # of ticks */

        /* computed during LinkPatchResources(), and read by LinkPatchRelocations */
    uint16      pld_TargetCodeRsrcIndex;        /* resource index of final DRSC_TYPE_CODE resource */
} PatchLinkerData;


/* -------------------- Patch code construction statics and defines */

    /* some instruction timing (!!! move elsewhere?) */
#define DSPN_TICKS_MOVE 2
#define DSPN_TICKS_NOP  1

    /* MOVE instruction replicated for each move */
static const uint16 PatchMoveCodeImage[] = {
    DSPN_OPCODE_MOVEADDR | 0,       /* relocate dest address */
    DSPN_OPERAND_ADDR | 0,          /* relocate src address */
};
#define PATCH_MOVE_CODE_SIZE        (sizeof PatchMoveCodeImage / sizeof PatchMoveCodeImage[0])
#define PATCH_MOVE_TICKS            DSPN_TICKS_MOVE
#define PATCH_MOVE_NUM_RELOCS       2

    /* Standard tail for all instruments, which gets replaced by JUMP/NOP when
    ** written to DSP code memory. This is trimmed off of each constituent
    ** instrument, and then added to the tail of the completed patch.
    */
static const uint16 PatchInsTailCodeImage[] = {
    DSPN_OPCODE_SLEEP,
    DSPN_OPCODE_NOP,
};
#define PATCH_INS_TAIL_CODE_SIZE    (sizeof PatchInsTailCodeImage / sizeof PatchInsTailCodeImage[0])
#define PATCH_INS_TAIL_TICKS        2

    /* Optional inter-instrument padding */
static const uint16 PatchInsPadCodeImage[] = {
    DSPN_OPCODE_NOP,
    DSPN_OPCODE_NOP,
};
#define PATCH_INS_PAD_CODE_SIZE     (sizeof PatchInsPadCodeImage / sizeof PatchInsPadCodeImage[0])
#define PATCH_INS_PAD_TICKS         (2 * DSPN_TICKS_NOP)


/* -------------------- Local functions */

static Err LinkPatchCode (PatchLinkerData *);
static Err LinkPatchResources (PatchLinkerData *);
static Err LinkPatchDataInitializers (PatchLinkerData *);
static Err LinkPatchRelocations (PatchLinkerData *);
static Err LinkPatchDynamicLinkNames (PatchLinkerData *);


/* -------------------- dsppLinkPatchTemplate() */

/*
    Link constituent DSPPTemplates into a user mode DSPPTemplate using
    PatchData constructed by dsppParsePatchCmds().

    This function fills in certain fields within PatchData and substructures
    as part of its resource usage analysis. It assumes that these fields
    have been initialized to 0 by dsppParsePatchCmds().

    Inputs
        resultDTmp
            Pointer to location to write resulting DSPPTemplate pointer.

        pd
            PatchData structure created by dsppParsePatchCmds().

    Results
        Success
            Returns 0. Writes pointer to user mode DSPPTemplate in *resultDTmp.

        Failure
            Returns Err code. Writes NULL in *resultDTmp.
*/
Err dsppLinkPatchTemplate (DSPPTemplate **resultDTmp, PatchData *pd)
{
    PatchLinkerData pld;
    Err errcode;

        /* init pld */
    memset (&pld, 0, sizeof pld);
    pld.pld_PatchData = pd;

        /* clear result */
    *resultDTmp = NULL;

        /* Create user mode DSPPTemplate */
    if (!(pld.pld_DTmp = dsppCreateUserTemplate())) {
        errcode = PATCH_ERR_NOMEM;
        goto clean;
    }
        /* !!! some of this stuff ought to go inside dsppCreateUserTemplate() as defaults */
    pld.pld_DTmp->dtmp_Header.dhdr_FunctionID     = DFID_PATCH;
    pld.pld_DTmp->dtmp_Header.dhdr_SiliconVersion = DSPP_SILICON_BULLDOG;   /* !!! perhaps get this from somewhere */
    pld.pld_DTmp->dtmp_Header.dhdr_FormatVersion  = DHDR_SUPPORTED_FORMAT_VERSION;
 /* pld.pld_DTmp->dtmp_Header.dhdr_Flags          = 0;  @@@ defaults to 0, no need to write it twice */

        /* build pieces of DSPPTemplate (@@@ this order is significant) */
    if ((errcode = LinkPatchCode (&pld))) goto clean;
    if ((errcode = LinkPatchResources (&pld))) goto clean;
    if ((errcode = LinkPatchDataInitializers (&pld))) goto clean;
    if ((errcode = LinkPatchRelocations (&pld))) goto clean;
    if ((errcode = LinkPatchDynamicLinkNames (&pld))) goto clean;

  #if DEBUG_Summary && defined(BUILD_STRINGS)
    printf ("dsppLinkPatchTemplate: code: %d words, ticks: %d\n", pld.pld_TotalCodeSize, pld.pld_TotalTicks);
  #endif

        /* success */
    *resultDTmp = pld.pld_DTmp;
    errcode = 0;

clean:
    if (errcode < 0) {
        dsppDeleteUserTemplate (pld.pld_DTmp);
    }
    return errcode;
}


/* -------------------- Code */

static void AnalyzePatchTemplateCode (PatchLinkerData *, PatchTemplateInfo *, bool padAtHead);
static uint16 dsppQueryTemplateTicks (const DSPPTemplate *);
static uint16 LinkCodeBase (PatchLinkerData *, uint16 codeSize, uint16 numTicks);

static void WritePatchTemplateCode (PatchLinkerData *, const PatchTemplateInfo *, bool padAtHead);
#define dsppCopyCode(dst,src,numWords) memcpy ((dst), (src), (numWords) * sizeof(uint16))

/*
    Build Code Image in pld_DTmp.

    Results
        Returns 0 on success, Err code on failure.

        pld->pld_DTmp->dtmp_Codes, pld->pld_DTmp->dtmp_CodeSize
            Code Image.

        pld->pld_TotalCodeSize
            Total computed code size of pld_DTmp.

        pld->pld_TotalTicks
            Total computed ticks for pld_DTmp.

        pti->pti_TargetMoveCodeBase, pti->pti_TargetInsCodeBase for each PatchTemplateInfo
            Final code base of each code segment.

    Notes
        . Assumes that pld_TotalCodeSize and pld_TotalTicks are 0 on entry.
        . Expects caller to clean up pld_DTmp after partial successs.
        . Links pd_PortPatchTemplate code after pd_TemplateList code, because
          the only code in pd_PortPatchTemplate are MOVEs for the patch
          outputs.
        . Assumes that it only has to deal w/ single-code-hunk instruments

    Caveats
        !!! ignores code size overflow (> 0xffff words)
*/
static Err LinkPatchCode (PatchLinkerData *pld)
{
    const bool padEnable = (pld->pld_PatchData->pd_Flags & PATCH_DATA_F_INTER_INSTRUMENT_PADDING) != 0;
    uint16 tailCodeBase;
    Err errcode;

    /* @@@ analysis and write phases must remain in sync: order, padding algorithim */

        /* analyze */
    {
        bool padAtHead = FALSE;

            /* constituent templates (MOVEs + code) */
        {
            PatchTemplateInfo *pti;

            SCANLIST (&pld->pld_PatchData->pd_TemplateList, pti, PatchTemplateInfo) {
                AnalyzePatchTemplateCode (pld, pti, padAtHead);
                padAtHead = padEnable;
            }
        }
            /* ports (MOVEs) */
        if (pld->pld_PatchData->pd_PortTemplate) {
            AnalyzePatchTemplateCode (pld, pld->pld_PatchData->pd_PortTemplate, padAtHead);
        }
            /* tail */
        tailCodeBase = LinkCodeBase (pld, PATCH_INS_TAIL_CODE_SIZE, PATCH_INS_TAIL_TICKS);
    }

  #if DEBUG_LinkCode
    printf ("LinkPatchCode: totalcode=0x%04x totalticks=0x%04x\n", pld->pld_TotalCodeSize, pld->pld_TotalTicks);
  #endif

    /* @@@ No zero code or ticks trap here because adding a static tail
    **     prevents this case from occuring. */

        /* allocate */
        /* @@@ assumes its building a single code hunk */
    {
        const uint32 codeSize = sizeof (DSPPCodeHeader) + pld->pld_TotalCodeSize * sizeof (uint16);
        DSPPCodeHeader *dcod;

            /* alloc/init dtmp_Codes */
        pld->pld_DTmp->dtmp_CodeSize = codeSize;
        if (!(dcod = pld->pld_DTmp->dtmp_Codes = AllocMem (codeSize, MEMTYPE_TRACKSIZE | MEMTYPE_NORMAL))) {
            errcode = PATCH_ERR_NOMEM;
            goto clean;
        }
        dcod[0].dcod_Type   = DCOD_RUN_DSPP;
        dcod[0].dcod_Offset = sizeof (DSPPCodeHeader);
        dcod[0].dcod_Size   = pld->pld_TotalCodeSize;
    }

        /* write */
    {
        bool padAtHead = FALSE;

            /* constituent templates */
        {
            const PatchTemplateInfo *pti;

            SCANLIST (&pld->pld_PatchData->pd_TemplateList, pti, PatchTemplateInfo) {
                WritePatchTemplateCode (pld, pti, padAtHead);
                padAtHead = padEnable;
            }
        }
            /* ports */
        if (pld->pld_PatchData->pd_PortTemplate) {
            WritePatchTemplateCode (pld, pld->pld_PatchData->pd_PortTemplate, padAtHead);
        }
            /* tail */
        dsppCopyCode ( (uint16 *)dsppGetCodeHunkImage (pld->pld_DTmp->dtmp_Codes, 0) + tailCodeBase,
                       PatchInsTailCodeImage,
                       PATCH_INS_TAIL_CODE_SIZE );
    }

    return 0;

clean:
    return errcode;
}


/*
    Add PatchTemplateInfo code and tick resources to totals.
    Determine code base for MOVE and instrument code segments.
*/
static void AnalyzePatchTemplateCode (PatchLinkerData *pld, PatchTemplateInfo *pti, bool padAtHead)
{
        /* padding */
    if (padAtHead) {
        pti->pti_TargetPadCodeBase = LinkCodeBase (pld, PATCH_INS_PAD_CODE_SIZE, PATCH_INS_PAD_TICKS);
    }

        /* MOVEs */
    if (pti->pti_NumMoves) {
        pti->pti_TargetMoveCodeBase = LinkCodeBase (pld,
            pti->pti_NumMoves * PATCH_MOVE_CODE_SIZE,
            pti->pti_NumMoves * PATCH_MOVE_TICKS);
    }

        /* Instrument */
        /* @@@ assumes 1 code hunk */
    if (pti->pti_DTmp->dtmp_CodeSize) {
        uint16 codeSegSize = pti->pti_DTmp->dtmp_Codes[0].dcod_Size;
        uint16 ticks = dsppQueryTemplateTicks (pti->pti_DTmp);

            /* remove standard instrument tail (assuming it's there) */
            /* @@@ this _could_ compare instead of simply remove the last couple of words */
        codeSegSize -= MIN (codeSegSize, PATCH_INS_TAIL_CODE_SIZE);
        ticks       -= MIN (ticks, PATCH_INS_TAIL_TICKS);

      #if DEBUG_LinkCode
        dspnDisassemble (dsppGetCodeHunkImage (pti->pti_DTmp->dtmp_Codes, 0), 0, codeSegSize,
            "AnalyzePatchTemplateCode: '%s' (tail removed) ticks=0x%04x", pti->pti.n_Name, ticks);
      #endif

        pti->pti_TargetInsCodeBase = LinkCodeBase (pld, codeSegSize, ticks);
    }
}

/*
    Finds out how many ticks a DSPPTemplate requires.

    Results
        drsc_Many field of DRSC_TYPE_TICKS resource. 0 if no DRSC_TYPE_TICKS resource.

    @@@ assumes there's only one tick resource per instrument
*/
static uint16 dsppQueryTemplateTicks (const DSPPTemplate *dtmp)
{
    int32 i;

    for (i=0; i<dtmp->dtmp_NumResources; i++) {
        const DSPPResource * const drsc = &dtmp->dtmp_Resources[i];

        if (drsc->drsc_Type == DRSC_TYPE_TICKS) return drsc->drsc_Many;
    }

    return 0;
}

/*
    Add code segment size to total code size.
    Add ticks to total ticks.
    Return code base for new code segment.
*/
static uint16 LinkCodeBase (PatchLinkerData *pld, uint16 codeSize, uint16 numTicks)
{
    const uint16 codeBase = pld->pld_TotalCodeSize;

    pld->pld_TotalCodeSize += codeSize;
    pld->pld_TotalTicks    += numTicks;
    return codeBase;
}


/*
    Add PatchTemplateInfo's code to pld_Dtmp under construction
*/
static void WritePatchTemplateCode (PatchLinkerData *pld, const PatchTemplateInfo *pti, bool padAtHead)
{
    DSPPCodeHeader * const dstdcod = pld->pld_DTmp->dtmp_Codes;

        /* padding */
    if (padAtHead) {
        uint16 * const dstCodePtr = (uint16 *)dsppGetCodeHunkImage (dstdcod, 0) + pti->pti_TargetPadCodeBase;

      #if DEBUG_LinkCode
        printf ("WritePatchTemplateCode: base=0x%04x size=0x%04x '%s' NOPs\n", pti->pti_TargetPadCodeBase, PATCH_INS_PAD_CODE_SIZE, pti->pti.n_Name);
      #endif

        dsppCopyCode (dstCodePtr, PatchInsPadCodeImage, PATCH_INS_PAD_CODE_SIZE);
    }

        /* MOVEs */
    if (pti->pti_NumMoves) {
        uint16 *dstCodePtr = (uint16 *)dsppGetCodeHunkImage (dstdcod, 0) + pti->pti_TargetMoveCodeBase;
        int i;

      #if DEBUG_LinkCode
        printf ("WritePatchTemplateCode: base=0x%04x size=0x%04x '%s' MOVEs\n", pti->pti_TargetMoveCodeBase, pti->pti_NumMoves * PATCH_MOVE_CODE_SIZE, pti->pti.n_Name);
      #endif

            /* !!! this code looks pretty inefficient: PATCH_MOVE_CODE_SIZE==2 */
        for (i=0; i<pti->pti_NumMoves; i++) {
            dsppCopyCode (dstCodePtr, PatchMoveCodeImage, PATCH_MOVE_CODE_SIZE);
            dstCodePtr += PATCH_MOVE_CODE_SIZE;
        }
    }

        /* instrument code */
        /* @@@ assumes 1 code hunk */
    if (pti->pti_DTmp->dtmp_CodeSize) {
        uint16 * const dstCodePtr = (uint16 *)dsppGetCodeHunkImage (dstdcod, 0) + pti->pti_TargetInsCodeBase;
        const DSPPCodeHeader * const srcdcod = pti->pti_DTmp->dtmp_Codes;
        uint16 codeSegSize = pti->pti_DTmp->dtmp_Codes[0].dcod_Size;

            /* remove standard instrument tail (assuming it's there) */
        codeSegSize -= MIN (codeSegSize, PATCH_INS_TAIL_CODE_SIZE);

      #if DEBUG_LinkCode
        printf ("WritePatchTemplateCode: base=0x%04x size=0x%04x '%s' instrument\n", pti->pti_TargetInsCodeBase, codeSegSize, pti->pti.n_Name);
      #endif

            /* copy code image */
        dsppCopyCode ( dstCodePtr,
                       (const uint16 *)dsppGetCodeHunkImage (srcdcod, 0),
                       codeSegSize);
    }
}


/* -------------------- Resources */

typedef struct PatchResourceAnalyzerData {
    uint32      prad_TotalResourceNamesSize;    /* total size of ResourceNames packed string array */
    uint16      prad_TotalNumResources;         /* total # of DSPPResources */
} PatchResourceAnalyzerData;

typedef struct PatchResourceWriterData {
    DSPPResource *prwd_RsrcPtr;                 /* pointer to next DSPPResource to write */
    char         *prwd_RsrcNamePtr;             /* pointer to space for next Resource name to write */
#if DEBUG_LinkResources
    const DSPPTemplate *prwd_DTmp;              /* pointer to DTmp under construction (debug only) */
#endif
} PatchResourceWriterData;

static Err AnalyzePatchTemplateResources (PatchResourceAnalyzerData *, PatchTemplateInfo *);
static void AnalyzeDestResource (PatchResourceInfo *);
static int32 CountUniqueDestConstants (int32 *pFirstConstValue, const PatchDestInfo *, uint16 numParts);
static int32 LinkResource (PatchResourceAnalyzerData *, const char *rsrcName);
static void WritePatchTemplateResources (PatchResourceWriterData *, const PatchTemplateInfo *);
static void WriteResource (PatchResourceWriterData *, const DSPPResource *, const char *rsrcName);

/*
    Build Resource table in pld_DTmp.

    Results
        Returns 0 on success, Err code on failure.

        pld->pld_TargetCodeRsrcIndex
        pld->pld_DTmp->dtmp_Resources, pld->pld_DTmp->dtmp_NumResources,
        pld->pld_DTmp->dtmp_ResourceNames,
            Assigned their proper values.

        pri_TargetRsrcIndex in each PatchResourceInfo in each PatchTemplateInfo.
            Assigned final resource index.

    Notes
        . Assumes that pld_ fields that it fills out are 0 on entry.
        . Assumes that pld_TotalCodeSize and pld_TotalTicks are already filled out.
        . Expects caller to clean up pld_DTmp after partial successs.
        . Links pd_PortPatchTemplate resources in this order - chosen to make
          CreateInstrument() fail as quickly as possible for most likely to be
          depleted resources:
            . total ticks
            . total code
            . pd_PortPatchTemplate resources
            . pd_TemplateList resources
        . See AnalyzeDestResource() for list of pri_ and pdi_ fields it modifies.
*/
static Err LinkPatchResources (PatchLinkerData *pld)
{
    Err errcode;

    /* @@@ analysis and write phases must remain in sync: order */

        /* analyze and allocate */
    {
        PatchResourceAnalyzerData prad;

            /* init prad */
        memset (&prad, 0, sizeof prad);

            /* ticks resource */
            /* (just need the space, don't need to remember the index) */
        if ((errcode = LinkResource (&prad, NULL)) < 0) goto clean;
            /* code resource */
        {
            const int32 result = LinkResource (&prad, NULL);

            if ((errcode = result) < 0) goto clean;
            pld->pld_TargetCodeRsrcIndex = result;
        }
            /* port resources */
        if (pld->pld_PatchData->pd_PortTemplate) {
            if ((errcode = AnalyzePatchTemplateResources (&prad, pld->pld_PatchData->pd_PortTemplate)) < 0) goto clean;
        }
            /* constituent template resources */
        {
            PatchTemplateInfo *pti;

            SCANLIST (&pld->pld_PatchData->pd_TemplateList, pti, PatchTemplateInfo) {
                if ((errcode = AnalyzePatchTemplateResources (&prad, pti)) < 0) goto clean;
            }
        }

      #if DEBUG_LinkResources
        printf ("LinkPatchResources: totalresources=%d totalnamesize=%d codersrc=%d\n",
            prad.prad_TotalNumResources, prad.prad_TotalResourceNamesSize,
            pld->pld_TargetCodeRsrcIndex);
      #endif

        /* @@@ Not checking for zero resources because we add a static tail in
        **     LinkPatchCode(), so there are always at least the ticks and code
        **     resources. */
        /* (non-zero prad_TotalNumResources implies non-zero prad_TotalResourceNameSize) */

            /* allocate resource and name arrays */
        pld->pld_DTmp->dtmp_NumResources = prad.prad_TotalNumResources;
        if ( !(pld->pld_DTmp->dtmp_Resources     = AllocMem (prad.prad_TotalNumResources * sizeof (DSPPResource), MEMTYPE_TRACKSIZE | MEMTYPE_NORMAL)) ||
             !(pld->pld_DTmp->dtmp_ResourceNames = AllocMem (prad.prad_TotalResourceNamesSize, MEMTYPE_TRACKSIZE | MEMTYPE_NORMAL)) ) {
            errcode = PATCH_ERR_NOMEM;
            goto clean;
        }
    }

        /* write */
    {
        PatchResourceWriterData prwd;   /* @@@ not init'ing here because all fields are init'd below */

            /* @@@ expects zero sizes to be dealt with above */
        prwd.prwd_RsrcPtr     = pld->pld_DTmp->dtmp_Resources;
        prwd.prwd_RsrcNamePtr = pld->pld_DTmp->dtmp_ResourceNames;
      #if DEBUG_LinkResources
        prwd.prwd_DTmp        = pld->pld_DTmp;
      #endif

            /* ticks resource */
            /* @@@ expects code linker to trap 0 ticks */
        {
            DSPPResource drsc;

            memset (&drsc, 0, sizeof drsc);
            drsc.drsc_Type = DRSC_TYPE_TICKS;
            drsc.drsc_Many = pld->pld_TotalTicks;
            WriteResource (&prwd, &drsc, NULL);
        }
            /* code resource */
            /* @@@ expects code linker to trap 0 code size */
        {
            DSPPResource drsc;

            memset (&drsc, 0, sizeof drsc);
            drsc.drsc_Type = DRSC_TYPE_CODE;
            drsc.drsc_Many = pld->pld_TotalCodeSize;
            WriteResource (&prwd, &drsc, NULL);
        }
            /* port resources */
        if (pld->pld_PatchData->pd_PortTemplate) {
            WritePatchTemplateResources (&prwd, pld->pld_PatchData->pd_PortTemplate);
        }
            /* constituent template resources */
        {
            const PatchTemplateInfo *pti;

            SCANLIST (&pld->pld_PatchData->pd_TemplateList, pti, PatchTemplateInfo) {
                WritePatchTemplateResources (&prwd, pti);
            }
        }
    }

    return 0;

clean:
    return errcode;
}

/*
    Add PatchTemplateInfo resources and names to totals.
    Assign resource index for each resource in PatchTemplateInfo.

    Results
        0 on success, Err code on failure
*/
static Err AnalyzePatchTemplateResources (PatchResourceAnalyzerData *prad, PatchTemplateInfo *pti)
{
    Err errcode;
    int32 rsrcIndex;

  #if DEBUG_LinkResources
    printf ("AnalyzePatchTemplateResources: '%s' (%d)\n", pti->pti.n_Name, pti->pti_NumResources);
  #endif

    for (rsrcIndex=0; rsrcIndex<pti->pti_NumResources; rsrcIndex++) {
        PatchResourceInfo * const pri = &pti->pti_ResourceInfo[rsrcIndex];

        if (pri->pri_Flags & PATCH_RESOURCE_F_KEEP) {
          #if DEBUG_LinkResources
            dsppDumpResource (pti->pti_DTmp, rsrcIndex);
          #endif

                /* analyze dest resources */
                /* (among other things, this can clear PATCH_RESOURCE_F_KEEP) */
            if (pri->pri_Connectability == PATCH_RESOURCE_CONNECTABLE_DEST) {
                AnalyzeDestResource (pri);
            }

                /* if resource still remains, add it to resource list */
            if (pri->pri_Flags & PATCH_RESOURCE_F_KEEP) {
                const int32 result = LinkResource (prad, pri->pri_TargetRsrcName);

                if ((errcode = result) < 0) goto clean;
                pri->pri_TargetRsrcIndex = result;
            }
        }
    }

    /*
        !!! room for other optimizations here:
            . change source's output relocation when going from single absolute to
              a bound resource or a patch output. this eliminates a MOVE and a data
              memory location.
                . only works when fanout == 1, so need to check all connections to
                  this output prior to doing this
    */

    return 0;

clean:
    return errcode;
}

/*
    Analyze dest resources. Determines:
        . whether resource still remains
        . # of parts
        . initializer flavor

    Inputs
        pri
            PatchResourceInfo for resource to analyze. Assumes it is a
            PATCH_RESOURCE_CONNECTABLE_DEST resource.

    Results
        . Sets PATCH_DEST_F_KEEP for all parts that are being kept.
        . Sets pdi_TargetPartNum for all parts that are being kept.
        . Sets pri_DestNumParts.
        . Clears PATCH_RESOURCE_F_KEEP if pri_DestNumParts is 0.
        . Sets pri_DestConstValue, PATCH_RESOURCE_F_DEST_INIT_SINGLE,
          and PATCH_RESOURCE_F_DEST_INIT_MULTI as appropriate for
          the kind of initialization required.

    Notes
        . Assumes that pri_DestNumParts is 0 on entry.
*/
static void AnalyzeDestResource (PatchResourceInfo *pri)
{
    const uint16 origNumParts = pri->pri_DRsc->drsc_Many;

        /* set PATCH_DEST_F_KEEP flags, assign pdi_TargetPartNum, determine pri_DestNumParts */
        /* @@@ assumes that pri->pri_DestNumParts is 0 at beginning. */
    {
        const bool optimize = (pri->pri_Flags & PATCH_RESOURCE_F_DEST_OPTIMIZE) != 0;
        int32 partNum;

        for (partNum = 0; partNum < origNumParts; partNum++) {
            PatchDestInfo * const pdi = &pri->pri_DestInfo[partNum];

                /*
                    Things we can remove a dest part for when PATCH_RESOURCE_F_DEST_OPTIMIZE is set:
                        . it is connected
                        . it is a constant that can fit in a DSPP immediate operand.

                    Otherwise we need to keep the dest part.
                */
            if (!(optimize && (pdi->pdi_State == PATCH_DEST_STATE_CONNECTED ||
                               pdi->pdi_State == PATCH_DEST_STATE_CONSTANT && dspnCanValueBeImmediate (pdi->pdi.ConstInfo.Value)))) {
                pdi->pdi_Flags         |= PATCH_DEST_F_KEEP;
                pdi->pdi_TargetPartNum  = pri->pri_DestNumParts++;
            }
        }
    }

        /* are we keeping this resource? */
    if (!pri->pri_DestNumParts) {
        pri->pri_Flags &= ~PATCH_RESOURCE_F_KEEP;
    }
        /* determine initializer flavor */
    else {
        int32 constValue;

            /* determine order of magnitude of number of unique
               PATCH_DEST_STATE_CONSTANT, PATCH_DEST_F_KEEP dests there are */
        switch (CountUniqueDestConstants (&constValue, pri->pri_DestInfo, origNumParts)) {
            case 0:     /* None: No initializer */
                break;

            case 1:     /* One: Use drsc_Default */
                pri->pri_Flags |= PATCH_RESOURCE_F_DEST_SINGLE_INIT;
                pri->pri_DestConstValue = constValue;
                break;

            default:    /* More than one: Use DataInitializer */
                pri->pri_Flags |= PATCH_RESOURCE_F_DEST_MULTI_INIT;
                break;
        }
    }

  #if DEBUG_LinkResources
          /* xxxx: */
    printf ("      dest: keep %d parts pri_Flags=0x%02x DestConstValue=%d\n", pri->pri_DestNumParts, pri->pri_Flags, pri->pri_DestConstValue);
  #endif
}

/*
    Count number (sort of) of unique initializers required for this dest
    (dests which are PATCH_DEST_STATE_CONSTANT, PATCH_DEST_F_KEEP)
    In fact it only returns 0, 1, >1, which is sufficient for the only caller.

    Results
        0
            Dest has no constants

        1
            Dest has 1 unique constant

        >1
            Dest has more than 1 unique constant.
            The actual number isn't returned, just the fact that there
            are more than 1.

        *pFirstConstValue is filled in with the first constant value, if there is one.
        It is only set if the function result is >= 1.
*/
static int32 CountUniqueDestConstants (int32 *pFirstConstValue, const PatchDestInfo *pdi, uint16 numParts)
{
    int32 numUniqueConsts = 0;
    int32 firstConstValue = 0;

    for (; numParts--; pdi++) {
        if (pdi->pdi_Flags & PATCH_DEST_F_KEEP && pdi->pdi_State == PATCH_DEST_STATE_CONSTANT) {

                /* if first one, get its value for later comparison */
            if (!numUniqueConsts) {
                firstConstValue = pdi->pdi.ConstInfo.Value;
                numUniqueConsts++;
            }
                /* otherwise, compare with first one */
            else if (pdi->pdi.ConstInfo.Value != firstConstValue) {
                numUniqueConsts++;
                goto done;              /* leave the loop; we found a 2nd one */
            }
        }
    }

done:
    *pFirstConstValue = firstConstValue;
    return numUniqueConsts;
}

/*
    Accumulate resource name and presence.
    Return resource index.

    Inputs
        rsrcName
            Name of resource. NULL is treated as an empty name.

    Results
        Resource index on success (>=0). Err code on failure (too many resources).
        Adds rsrcName size to prad_TotalResourceNamesSize.
        Increments prad_TotalNumResources.
*/
static int32 LinkResource (PatchResourceAnalyzerData *prad, const char *rsrcName)
{
    if (prad->prad_TotalNumResources >= PATCH_MAX_NUM_RESOURCES) return PATCH_ERR_TOO_MANY_RESOURCES;

    prad->prad_TotalResourceNamesSize += PackedStringSize (rsrcName ? rsrcName : "");
    return prad->prad_TotalNumResources++;
}

/*
    Add PatchTemplateInfo's resources to dtmp under construction
    Writes to things pointed to by prwd and advances pointers to resources
    beyond those contributed by PatchTemplateInfo.

    @@@ Assumes that Variables have no subtypes. Need to change a few things if this changes.
*/
static void WritePatchTemplateResources (PatchResourceWriterData *prwd, const PatchTemplateInfo *pti)
{
    int32 rsrcIndex;

  #if DEBUG_LinkResources
    printf ("WritePatchTemplateResources: '%s' (%d)\n", pti->pti.n_Name, pti->pti_NumResources);
  #endif

    for (rsrcIndex=0; rsrcIndex<pti->pti_NumResources; rsrcIndex++) {
        const PatchResourceInfo * const pri = &pti->pti_ResourceInfo[rsrcIndex];

        if (pri->pri_Flags & PATCH_RESOURCE_F_KEEP) {
            const DSPPResource * const origdrsc = pri->pri_DRsc;
            DSPPResource newdrsc;

          #if DEBUG_LinkResources
            printf ("orig");
            dsppDumpResource (pti->pti_DTmp, rsrcIndex);
          #endif

                /*
                    Start with original resource, change things as needed below

                    @@@ bind processing code below depends on drsc_Allocated (bind to part)
                        being copied by this.
                */
            newdrsc = *origdrsc;

            switch (pri->pri_Connectability) {
                case PATCH_RESOURCE_CONNECTABLE_DEST:
                  #if DEBUG_LinkResources
                          /* xxx xxxx: */
                    printf ("          dest: keep %d parts pri_Flags=0x%02x DestConstValue=%d\n", pri->pri_DestNumParts, pri->pri_Flags, pri->pri_DestConstValue);
                  #endif

                        /*
                            @@@ Doesn't deal w/ a patch outputs with initializers. This is OK because
                                patch ports aren't currently allowed to have initializers other than
                                the default 0 at allocation. This is set appropriately by the creator
                                of the patch port template's resources (in createpatch_parser.c).
                        */

                        /* Part reduction */
                    newdrsc.drsc_Many = pri->pri_DestNumParts;

                    if (pti->pti.n_Type != PATCH_TEMPLATE_TYPE_PORT) {
                            /*
                                Instrument input / knob.

                                Do special processing:
                                    . type conversion
                                        Since this resource is internal, and might be written to
                                        by a MOVE to load it, it must be a Variable. Otherwise,
                                        the relocator will clobber MOVE opcodes.

                                    . initialization
                                        Use original initializer overridden by constants and connections.

                                        @@@ Assumes that Inputs and Knobs cannot be initialized at start.
                            */
                        newdrsc.drsc_Type    = DRSC_TYPE_VARIABLE;
                        newdrsc.drsc_SubType = 0;       /* @@@ variables don't have subtypes (yet) */
                        newdrsc.drsc_Flags  &= ~DRSC_F_INIT_AT_START;

                            /* If single-value initializer, set init at alloc */
                        if (pri->pri_Flags & PATCH_RESOURCE_F_DEST_SINGLE_INIT) {
                            newdrsc.drsc_Flags  |= DRSC_F_INIT_AT_ALLOC;
                            newdrsc.drsc_Default = pri->pri_DestConstValue;
                        }
                            /* otherwise, clear initializer */
                        else {
                            newdrsc.drsc_Flags  &= ~DRSC_F_INIT_AT_ALLOC;
                            newdrsc.drsc_Default = 0;
                        }
                    }
                    break;

                case PATCH_RESOURCE_CONNECTABLE_SRC:
                    if (pti->pti.n_Type != PATCH_TEMPLATE_TYPE_PORT) {
                            /*
                                Instrument output

                                Do special processing:
                                    . type conversion
                                        Since this resource is internal, and might be written to
                                        by a MOVE to load it, it must be a Variable. Otherwise,
                                        the relocator will clobber MOVE opcodes.
                            */
                        newdrsc.drsc_Type    = DRSC_TYPE_VARIABLE;
                        newdrsc.drsc_SubType = 0;       /* @@@ variables don't have subtypes (yet) */
                    }
                    break;

                /* default: use original resource */
            }

                /* if bound, lookup target resource index of BindToRsrcIndex */
            if (origdrsc->drsc_Flags & DRSC_F_BIND) {
                newdrsc.drsc_BindToRsrcIndex = pti->pti_ResourceInfo [origdrsc->drsc_BindToRsrcIndex].pri_TargetRsrcIndex;
                /* @@@ drsc_Allocated (bind to part) copied when newdrsc is initialized */
            }

                /* write new resource */
            WriteResource (prwd, &newdrsc, pri->pri_TargetRsrcName);

          #if DEBUG_LinkResources
            printf ("new ");
            dsppDumpResource (prwd->prwd_DTmp, pri->pri_TargetRsrcIndex);
          #endif
        }
    }
}

/*
    Write resource and resource name to dtmp under construction.

    Inputs
        rsrcName
            Name of resource. NULL is treated as an empty name.

    Results
        Writes to things pointed to by prwd and advances pointers to next resource.
*/
static void WriteResource (PatchResourceWriterData *prwd, const DSPPResource *drsc, const char *rsrcName)
{
    *prwd->prwd_RsrcPtr++ = *drsc;
    prwd->prwd_RsrcNamePtr = AppendPackedString (prwd->prwd_RsrcNamePtr, rsrcName ? rsrcName : "");
}


/* -------------------- Data Initializers */

static uint32 TotalPatchTemplateDataInitializers (const PatchTemplateInfo *);
static void WritePatchTemplateDataInitializers (DSPPDataInitializer **diniptrbuf, const PatchTemplateInfo *);

/*
    Build Data Initializers in pld_DTmp.

    Substitutes PATCH_RESOURCE_CONNECTABLE_DEST pdi.ConstInfo.Value initializers
    in place of any DINI that the dest resource may initially have had (pdi.ConstInfo.Values
    are set from that DINI if there was one). Other DINIs are preserved intact.

    Results
        Returns 0 on success, Err code on failure.

        Generates pld->pld_DTmp->dtmp_DataInitializers, pld->pld_DTmp->dtmp_DataInitializerSize

    Notes
        . Assumes that target dtmp_Resources has already been constructed.
        . Assumes that pld_ fields that it fills out are 0 on entry.
        . Expects caller to clean up pld_DTmp after partial successs.
        . Constructs relocations in this order:
            . pd_TemplateList relocations
                . MOVEs
                . instrument code
            . pd_PortTemplate MOVE relocations
        . Assumes that only connectable destinations have any form of resource
          optimization.
*/
static Err LinkPatchDataInitializers (PatchLinkerData *pld)
{
    uint32 totalDIniSize = 0;
    Err errcode;

    /* @@@ analysis and write phases must remain in sync: order, assumptions about
    **     the kinds of resources which get part reduction optimization. */
    /* @@@ not scanning port resources because there shouldn't be any DINIs
    **     there. if this needs to be done, it should probably follow the same
    **     order as resource analysis and write */

        /* analyze */
        /* constituent template data initializers */
    {
        PatchTemplateInfo *pti;

        SCANLIST (&pld->pld_PatchData->pd_TemplateList, pti, PatchTemplateInfo) {
            totalDIniSize += TotalPatchTemplateDataInitializers (pti);
        }
    }

  #if DEBUG_LinkDataInitializers
    printf ("LinkPatchDataInitializers: totalDIniSize=%u\n", totalDIniSize);
  #endif

        /* if there are any, allocate and write */
    if (totalDIniSize) {
            /* allocate */
        pld->pld_DTmp->dtmp_DataInitializerSize = totalDIniSize;
        if (!(pld->pld_DTmp->dtmp_DataInitializer = AllocMem (totalDIniSize, MEMTYPE_TRACKSIZE | MEMTYPE_NORMAL | MEMTYPE_FILL))) {
            errcode = PATCH_ERR_NOMEM;
            goto clean;
        }

            /* write */
        {
            DSPPDataInitializer *diniptr = pld->pld_DTmp->dtmp_DataInitializer;

                /* constituent template data initializers */
            {
                const PatchTemplateInfo *pti;

                SCANLIST (&pld->pld_PatchData->pd_TemplateList, pti, PatchTemplateInfo) {
                    WritePatchTemplateDataInitializers (&diniptr, pti);
                }
            }
        }
    }

    return 0;

clean:
    return errcode;
}

/*
    Return the total size of all the required DSPPDataInitializers for PatchTemplateInfo.
*/
static uint32 TotalPatchTemplateDataInitializers (const PatchTemplateInfo *pti)
{
    uint32 totalDIniSize = 0;
    int32 rsrcIndex;

    for (rsrcIndex=0; rsrcIndex<pti->pti_NumResources; rsrcIndex++) {
        const PatchResourceInfo * const pri = &pti->pti_ResourceInfo[rsrcIndex];

        if (pri->pri_Flags & PATCH_RESOURCE_F_KEEP) {

                /* @@@ makes assumptions about which kinds of resources have
                **     part reduction optimizations */
            switch (pri->pri_Connectability) {
                case PATCH_RESOURCE_CONNECTABLE_DEST:
                    if (pri->pri_Flags & PATCH_RESOURCE_F_DEST_MULTI_INIT) {
                            /* add DINI size for remaining parts of multi-initializer dest */
                        totalDIniSize += dsppDataInitializerSize (pri->pri_DestNumParts);
                    }
                    break;

                default:
                    if (pri->pri_DataInitializer) {
                            /* add total size of other DINIs */
                        totalDIniSize += dsppDataInitializerSize (pri->pri_DataInitializer->dini_Many);
                    }
                    break;
            }
        }
    }

    return totalDIniSize;
}

/*
    Write data initializer records in dtmp under construction for a PatchTemplateInfo

    Inputs
        drlcptrbuf
            Pointer to buffer containing next DSPPRelocation in dtmp under construction.

        pti
            PatchTemplateInfo to write

    Results
        *drlcptrbuf is incremented for each DSPPRelocation written to dtmp under construction.

    Notes
        . assumes that dtmp_DataInitializers was cleared when allocated
        . logic must match that of TotalPatchTemplateDataInitializers()
*/
static void WritePatchTemplateDataInitializers (DSPPDataInitializer **diniptrbuf, const PatchTemplateInfo *pti)
{
    DSPPDataInitializer *diniptr = *diniptrbuf; /* pointer to DINI being filled out, incremented after each one is built */
    int32 rsrcIndex;

  #if DEBUG_LinkDataInitializers
    printf ("WritePatchTemplateDataInitializers: '%s'\n", pti->pti.n_Name);
  #endif

    for (rsrcIndex=0; rsrcIndex<pti->pti_NumResources; rsrcIndex++) {
        const PatchResourceInfo * const pri = &pti->pti_ResourceInfo[rsrcIndex];
        const DSPPResource * const drsc = pri->pri_DRsc;

        if (pri->pri_Flags & PATCH_RESOURCE_F_KEEP) {
            switch (pri->pri_Connectability) {
                case PATCH_RESOURCE_CONNECTABLE_DEST:
                    if (pri->pri_Flags & PATCH_RESOURCE_F_DEST_MULTI_INIT) {

                            /* write DINI based on pdi.ConstInfo.Values */
                        diniptr->dini_RsrcIndex = pri->pri_TargetRsrcIndex;
                        diniptr->dini_Many      = pri->pri_DestNumParts;
                        diniptr->dini_Flags     = DINI_F_AT_ALLOC;

                        {
                            const PatchDestInfo *pdi = pri->pri_DestInfo;
                            int32 numParts = drsc->drsc_Many;
                            int32 *diniimage = dsppGetDataInitializerImage (diniptr);

                            for (; numParts--; pdi++) {
                                if (pdi->pdi_Flags & PATCH_DEST_F_KEEP) {
                                    if (pdi->pdi_State == PATCH_DEST_STATE_CONSTANT)
                                        *diniimage++ = pdi->pdi.ConstInfo.Value;
                                    else
                                        diniimage++;        /* already cleared to 0, no sense in writing another 0 there */
                                }
                            }
                        }

                      #if DEBUG_LinkDataInitializers
                        dsppDumpDataInitializer (diniptr);
                      #endif

                            /* advance to next dini */
                        diniptr = dsppNextDataInitializer (diniptr);
                    }
                    break;

                default:
                    if (pri->pri_DataInitializer) {
                            /* copy original DINI, lookup Target RsrcIndex */
                        memcpy (diniptr, pri->pri_DataInitializer, dsppDataInitializerSize(pri->pri_DataInitializer->dini_Many));
                        diniptr->dini_RsrcIndex = pri->pri_TargetRsrcIndex;

                      #if DEBUG_LinkDataInitializers
                        dsppDumpDataInitializer (diniptr);
                      #endif

                            /* advance to next dini */
                        diniptr = dsppNextDataInitializer (diniptr);
                    }
                    break;
            }
        }
    }

    *diniptrbuf = diniptr;
}


/* -------------------- Relocations */

static uint32 CountPatchTemplateRelocations (const PatchTemplateInfo *);
static void WritePatchTemplateRelocations (PatchLinkerData *, DSPPRelocation **drlcptrbuf, const PatchTemplateInfo *);

/*
    Build Relocation table in pld_DTmp.

    Results
        Returns 0 on success, Err code on failure.

        Generates pld->pld_DTmp->dtmp_Relocations, pld->pld_DTmp->dtmp_NumRelocations

    Notes
        . Assumes that target dtmp_Codes has already been constructed.
        . Assumes that pld_ fields that it fills out are 0 on entry.
        . Assumes that pld_TargetCodeRsrcIndex is already filled out.
        . Expects caller to clean up pld_DTmp after partial successs.
        . Constructs relocations in this order:
            . pd_TemplateList relocations
                . MOVEs
                . instrument code
            . pd_PortTemplate MOVE relocations
*/
static Err LinkPatchRelocations (PatchLinkerData *pld)
{
    uint32 totalNumRelocs = 0;
    Err errcode;

    /* @@@ analysis and write phases must remain in sync: order, algorithm to
    **     determine which relocs to keep. */

        /* analyze */
    {
            /* constituent template relocations */
        {
            PatchTemplateInfo *pti;

            SCANLIST (&pld->pld_PatchData->pd_TemplateList, pti, PatchTemplateInfo) {
                totalNumRelocs += CountPatchTemplateRelocations (pti);
            }
        }
            /* port relocations */
        if (pld->pld_PatchData->pd_PortTemplate) {
            totalNumRelocs += CountPatchTemplateRelocations (pld->pld_PatchData->pd_PortTemplate);
        }
    }

  #if DEBUG_LinkRelocations
    printf ("LinkPatchRelocations: totalNumRelocs=%d\n", totalNumRelocs);
  #endif

        /* if there are any, allocate and write */
    if (totalNumRelocs) {
            /* allocate relocation array */
        pld->pld_DTmp->dtmp_NumRelocations = totalNumRelocs;
        if (!(pld->pld_DTmp->dtmp_Relocations = AllocMem (totalNumRelocs * sizeof (DSPPRelocation), MEMTYPE_TRACKSIZE | MEMTYPE_NORMAL | MEMTYPE_FILL))) {
            errcode = PATCH_ERR_NOMEM;
            goto clean;
        }

            /* write */
        {
            DSPPRelocation *drlcptr = pld->pld_DTmp->dtmp_Relocations;

                /* constituent template relocations */
            {
                const PatchTemplateInfo *pti;

                SCANLIST (&pld->pld_PatchData->pd_TemplateList, pti, PatchTemplateInfo) {
                    WritePatchTemplateRelocations (pld, &drlcptr, pti);
                }
            }
                /* port resources */
            if (pld->pld_PatchData->pd_PortTemplate) {
                WritePatchTemplateRelocations (pld, &drlcptr, pld->pld_PatchData->pd_PortTemplate);
            }
        }
    }

    return 0;

clean:
    return errcode;
}

/*
    Return the number of relocations required for PatchTemplateInfo's MOVEs and
    instrument code.
*/
static uint32 CountPatchTemplateRelocations (const PatchTemplateInfo *pti)
{
    uint32 totalNumRelocs = pti->pti_NumMoves * PATCH_MOVE_NUM_RELOCS;

        /* scan original relocation records */
    {
        const DSPPRelocation *drlc = pti->pti_DTmp->dtmp_Relocations;
        int32 nrelocs = pti->pti_DTmp->dtmp_NumRelocations;

        for (; nrelocs--; drlc++) {
            const PatchResourceInfo * const pri = &pti->pti_ResourceInfo [drlc->drlc_RsrcIndex];

            switch (pri->pri_Connectability) {
                case PATCH_RESOURCE_CONNECTABLE_DEST:
                    {
                        const PatchDestInfo * const pdi = &pri->pri_DestInfo [drlc->drlc_Part];

                            /* drop the relocation only if a non-IMMED constant */
                        switch (pdi->pdi_State) {
                            case PATCH_DEST_STATE_CONSTANT:
                                if (pdi->pdi_Flags & PATCH_DEST_F_KEEP) totalNumRelocs++;
                                break;

                            default:
                                totalNumRelocs++;
                                break;
                        }
                    }
                    break;

                default:
                        /* keep the relocation otherwise */
                    totalNumRelocs++;
                    break;
            }
        }
    }

  #if DEBUG_LinkRelocations
    printf ("CountPatchTemplateRelocations: '%s' %d move relocs, %d->%d ins relocs\n",
        pti->pti.n_Name,
        pti->pti_NumMoves * PATCH_MOVE_NUM_RELOCS,
        pti->pti_DTmp->dtmp_NumRelocations, totalNumRelocs - pti->pti_NumMoves * PATCH_MOVE_NUM_RELOCS);
  #endif

    return totalNumRelocs;
}

/*
    Write relocation records in dtmp under construction for a PatchTemplateInfo
        . MOVEs
        . instrument code

    Inputs
        pld
            PatchLinkerData. Reads pld->pld_TargetCodeRsrcIndex.

        drlcptrbuf
            Pointer to buffer containing next DSPPRelocation in dtmp under construction.

        pti
            PatchTemplateInfo to write

    Results
        *drlcptrbuf is incremented for each DSPPRelocation written to dtmp under construction.

    Notes
        . assumes that dtmp_Relocations was cleared when allocated
        . assumes that it is only writing relocations for code hunk 0
        . writes to target code image, so target code image must have already been built:
            . insertion of IMMED constants
            . relocation merging (!!! not implemented yet)
*/
static void WritePatchTemplateRelocations (PatchLinkerData *pld, DSPPRelocation **drlcptrbuf, const PatchTemplateInfo *pti)
{
    DSPPRelocation *drlcptr = *drlcptrbuf;  /* pointer to relocation being filled out, incremented after each one is built */

  #if DEBUG_LinkRelocations
    printf ("WritePatchTemplateRelocations: '%s'\n", pti->pti.n_Name);
  #endif

    /*
        reminder: PATCH_RESOURCE_F_KEEP and PATCH_DEST_F_KEEP are independent of
        connection information because a connection can cause a part or a whole
        resource to be removed. The connection information contained in the
        associated DestInfo for a removed resource is still valid and must
        be processed.
    */

        /* write MOVE relocations */
    if (pti->pti_NumMoves) {
        uint16 codeOffset = pti->pti_TargetMoveCodeBase;
        int32 rsrcIndex;

        for (rsrcIndex=0; rsrcIndex < pti->pti_NumResources; rsrcIndex++) {
            const PatchResourceInfo * const pri = &pti->pti_ResourceInfo[rsrcIndex];
            const DSPPResource * const drsc = pri->pri_DRsc;

                /* generate MOVE relocations for all connected dests that require them */
                /* (not checking PATCH_RESOURCE_F_KEEP for reason stated in above reminder) */
            if (pri->pri_Connectability == PATCH_RESOURCE_CONNECTABLE_DEST &&
                pri->pri_Flags & PATCH_RESOURCE_F_DEST_NEEDS_MOVE) {

                const PatchDestInfo *pdi = pri->pri_DestInfo;
                int32 numParts = drsc->drsc_Many;

                for (; numParts--; pdi++) {
                    if (pdi->pdi_State == PATCH_DEST_STATE_CONNECTED) {
                      #if DEBUG_LinkRelocations
                        printf ("  port '%s', part %u: connect w/ MOVE to block '%s', port '%s', part %u\n",
                            dsppGetTemplateRsrcName (pti->pti_DTmp, rsrcIndex),
                            pdi - pri->pri_DestInfo,    /* (a way to get the PartNum w/i the DestInfo array of current pdi) */
                            pdi->pdi.ConnectInfo.SrcTemplate->pti.n_Name,
                            dsppGetTemplateRsrcName (pdi->pdi.ConnectInfo.SrcTemplate->pti_DTmp, pdi->pdi.ConnectInfo.SrcRsrcIndex),
                            pdi->pdi.ConnectInfo.SrcPartNum);
                      #endif

                            /* MOVE dest reloc (codeoffset = instr+0) */
                            /* use target resource index from ResourceInfo and target part number from DestInfo */
                        drlcptr->drlc_RsrcIndex  = pri->pri_TargetRsrcIndex;
                        drlcptr->drlc_Part       = pdi->pdi_TargetPartNum;
                        drlcptr->drlc_CodeOffset = codeOffset + 0;
                      #if DEBUG_LinkRelocations
                        printf ("    ");
                        dsppDumpRelocation (drlcptr);
                      #endif
                        drlcptr++;

                            /* MOVE src reloc (codeoffset = instr+1) */
                            /* lookup target resource index of SrcRsrcIndex thru SrcTemplate's ResourceInfo[] */
                        drlcptr->drlc_RsrcIndex  = pdi->pdi.ConnectInfo.SrcTemplate->pti_ResourceInfo[pdi->pdi.ConnectInfo.SrcRsrcIndex].pri_TargetRsrcIndex;
                        drlcptr->drlc_Part       = pdi->pdi.ConnectInfo.SrcPartNum;
                        drlcptr->drlc_CodeOffset = codeOffset + 1;
                      #if DEBUG_LinkRelocations
                        printf ("    ");
                        dsppDumpRelocation (drlcptr);
                      #endif
                        drlcptr++;

                        codeOffset += PATCH_MOVE_CODE_SIZE;
                    }
                }
            }
        }
    }

        /* write instrument code relocations */
    {
        const DSPPRelocation *origdrlc = pti->pti_DTmp->dtmp_Relocations;
        int32 nrelocs = pti->pti_DTmp->dtmp_NumRelocations;

        for (; nrelocs--; origdrlc++) {
            const PatchResourceInfo * const pri = &pti->pti_ResourceInfo [origdrlc->drlc_RsrcIndex];
            const DSPPResource * const origdrsc = pri->pri_DRsc;

            switch (pri->pri_Connectability) {
                case PATCH_RESOURCE_CONNECTABLE_DEST:
                    {
                        const PatchDestInfo * const pdi = &pri->pri_DestInfo [origdrlc->drlc_Part];

                            /* drop the relocation only if an IMMED constant */
                        switch (pdi->pdi_State) {
                         /* case PATCH_DEST_STATE_CONNECTED: */
                            default:

                              #if DEBUG_LinkRelocations
                                printf ("  port '%s', part %u: connect to block '%s', port '%s', part %u\n",
                                    dsppGetTemplateRsrcName (pti->pti_DTmp, origdrlc->drlc_RsrcIndex),
                                    origdrlc->drlc_Part,
                                    pdi->pdi.ConnectInfo.SrcTemplate->pti.n_Name,
                                    dsppGetTemplateRsrcName (pdi->pdi.ConnectInfo.SrcTemplate->pti_DTmp, pdi->pdi.ConnectInfo.SrcRsrcIndex),
                                    pdi->pdi.ConnectInfo.SrcPartNum);
                              #endif

                                    /* If connected, create relocation using source (connect from)
                                       resource & part. Look up source target resource index. */
                                drlcptr->drlc_RsrcIndex  = pdi->pdi.ConnectInfo.SrcTemplate->pti_ResourceInfo[pdi->pdi.ConnectInfo.SrcRsrcIndex].pri_TargetRsrcIndex;
                                drlcptr->drlc_Part       = pdi->pdi.ConnectInfo.SrcPartNum;
                                goto finish_drlc;

                            case PATCH_DEST_STATE_CONSTANT:

                              #if DEBUG_LinkRelocations
                                printf ("  port '%s', part %u: constant %d flags=0x%02x\n",
                                    dsppGetTemplateRsrcName (pti->pti_DTmp, origdrlc->drlc_RsrcIndex),
                                    origdrlc->drlc_Part,
                                    pdi->pdi.ConstInfo.Value,
                                    pdi->pdi_Flags);
                              #endif

                                if (pdi->pdi_Flags & PATCH_DEST_F_KEEP) {

                                        /* If a non-IMMED constant, create relocation to resource which
                                           is initialized to constant. Use pdi_TargetPartNum because
                                           we might have reduced parts */
                                    drlcptr->drlc_RsrcIndex  = pri->pri_TargetRsrcIndex;
                                    drlcptr->drlc_Part       = pdi->pdi_TargetPartNum;
                                    goto finish_drlc;
                                }
                                else {
                                  #if DEBUG_LinkRelocations
                                    printf ("    use IMMED operand\n");
                                  #endif

                                        /*
                                            If an IMMED constant, insert constant value directly into
                                            target code image. Create no relocation.

                                            Use the relocator to do this:
                                                . template, reloc: originals because:
                                                    . in sync w/ one another
                                                    . original resource might be eliminated
                                                      in target
                                                . fixup location: code image as found in target dtmp
                                            @@@ assumes single code hunk in both source and target
                                        */
                                    dsppRelocate (pti->pti_DTmp, origdrlc, dspnPackImmediate(pdi->pdi.ConstInfo.Value),
                                                  (DSPPFixupFunction)dsppFixupCodeImage,
                                                  (uint16 *)dsppGetCodeHunkImage (pld->pld_DTmp->dtmp_Codes, 0) + pti->pti_TargetInsCodeBase);
                                }
                                break;
                        }
                    }
                    break;

                default:                /* all other kinds of relocations */
                  #if DEBUG_LinkRelocations
                    printf ("  port '%s', part %u: other reloc\n",
                        dsppGetTemplateRsrcName (pti->pti_DTmp, origdrlc->drlc_RsrcIndex),
                        origdrlc->drlc_Part);
                  #endif

                        /* @@@ shouldn't actually encounter any DRSC_F_BINDs here, could relax this check a bit */
                    if (origdrsc->drsc_Type == DRSC_TYPE_CODE &&
                        !(origdrsc->drsc_Flags & (DRSC_F_BIND | DRSC_F_IMPORT))) {

                            /* Branch relocation: point to target code resource and target base-relative code location */
                        drlcptr->drlc_RsrcIndex = pld->pld_TargetCodeRsrcIndex;
                        drlcptr->drlc_Part      = origdrlc->drlc_Part + pti->pti_TargetInsCodeBase;
                    }
                    else {
                            /* Other relocation: look up resource in target, use original part */
                        drlcptr->drlc_RsrcIndex = pri->pri_TargetRsrcIndex;
                        drlcptr->drlc_Part      = origdrlc->drlc_Part;
                    }
                    goto finish_drlc;

                finish_drlc:            /* write common parts of new drlc and increment to next one */
                        /* offset code pointer using template's code base in target */
                    drlcptr->drlc_CodeOffset = origdrlc->drlc_CodeOffset + pti->pti_TargetInsCodeBase;
                  #if DEBUG_LinkRelocations
                    printf ("    ");
                    dsppDumpRelocation (drlcptr);
                  #endif
                    drlcptr++;
                    break;
            }
        }
    }

    *drlcptrbuf = drlcptr;
}


/* -------------------- Dynamic Link Names */

/*
    Build Dynamic Link Names in pld_DTmp.

    Concatenates all constituent dtmp_DynamicLinkNames.

    !!! could optimize the resulting dtmp_DynamicLinkNames to contain only
        unique names. This might speed up CreateInstrument() a tiny bit, as
        each one results in an OpenItem() call.

    Results
        Returns 0 on success, Err code on failure.

        Generates pld->pld_DTmp->dtmp_DynamicLinkNames, pld->pld_DTmp->dtmp_DynamicLinkNamesSize

    Notes
        . Assumes that pld_ fields that it fills out are 0 on entry.
        . Expects caller to clean up pld_DTmp after partial successs.
        . Assumes that port template has no resources (it shouldn't have, because it has
          no real code)
*/
static Err LinkPatchDynamicLinkNames (PatchLinkerData *pld)
{
    uint32 totalDLnkSize = 0;
    Err errcode;

    /* @@@ analysis and write phases should have the same order, but it isn't critical */
    /* @@@ not scanning port template because it shouldn't have any dynamic links */

        /* analyze */
    {
            /* constituent template dynamic links */
        {
            PatchTemplateInfo *pti;

            SCANLIST (&pld->pld_PatchData->pd_TemplateList, pti, PatchTemplateInfo) {
                totalDLnkSize += pti->pti_DTmp->dtmp_DynamicLinkNamesSize;
            }
        }
    }

  #if DEBUG_LinkDynamicLinkNames
    printf ("LinkPatchDynamicLinkNames: totalDLnkSize=%u\n", totalDLnkSize);
  #endif

        /* if there are any, allocate and write */
    if (totalDLnkSize) {
            /* allocate dynamic links */
        pld->pld_DTmp->dtmp_DynamicLinkNamesSize = totalDLnkSize;
        if (!(pld->pld_DTmp->dtmp_DynamicLinkNames = AllocMem (totalDLnkSize, MEMTYPE_TRACKSIZE | MEMTYPE_NORMAL))) {
            errcode = PATCH_ERR_NOMEM;
            goto clean;
        }

            /* write dynamic links */
        {
            char *dlnkptr = pld->pld_DTmp->dtmp_DynamicLinkNames;

                /* constituent template dynamic links */
            {
                const PatchTemplateInfo *pti;

                SCANLIST (&pld->pld_PatchData->pd_TemplateList, pti, PatchTemplateInfo) {
                    const DSPPTemplate * const srcdtmp = pti->pti_DTmp;

                    if (srcdtmp->dtmp_DynamicLinkNamesSize) {
                        memcpy (dlnkptr, srcdtmp->dtmp_DynamicLinkNames, srcdtmp->dtmp_DynamicLinkNamesSize);
                        dlnkptr += srcdtmp->dtmp_DynamicLinkNamesSize;
                    }
                }
            }
        }
    }

    return 0;

clean:
    return errcode;
}
