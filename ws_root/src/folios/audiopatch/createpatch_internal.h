#ifndef __CREATEPATCH_INTERNAL_H
#define __CREATEPATCH_INTERNAL_H


/******************************************************************************
**
**  @(#) createpatch_internal.h 96/02/27 1.27
**
**  DSP patch intstrument template builder - module internal include.
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
**  950717 WJB  Removed resource name list stuff.
**  950718 WJB  Added dsppLinkPatchTemplate().
**              Removed some unused stuff from PatchData.
**  950721 WJB  Moved errors codes to audio.h.
**  950731 WJB  Renamed pri_ExposedName as pri_TargetRsrcName.
**  950803 WJB  Expanded resource indeces to 16 bits.
**  950803 WJB  Replaced PatchRelocInfo and PatchMoveInfo with PatchDestInfo.
**  950808 WJB  Revised for changes in resource / initializer strategy.
**  950814 WJB  Added pti_NumResources and pri_DRsc to help optimize code a bit.
**  950818 WJB  Moved PatchResourceNameInfo list into PatchData.
**  950822 WJB  Replaced PATCH_RESOURCE_NAME_F_IMPORT with PATCH_RESOURCE_NAME_F_UNIQUE.
**              Added PATCH_RESOURCE_NAME_F_CONST.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#ifndef __AUDIO_DSPP_TEMPLATE_H
#include <audio/dspp_template.h>
#endif

#ifndef __AUDIO_PATCH_H
#include <audio/patch.h>
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif


/* -------------------- Data structures used to construct patch */

    /* forward typedefs */
typedef struct PatchTemplateInfo PatchTemplateInfo;


    /* Connection destination part information. One per part of each
       PATCH_RESOURCE_CONNECTABLE_DEST resource. This exists regardless
       of PATCH_RESOURCE_F_KEEP. */
    /* !!! might think about reorganizing this structure to have less memory padding */
typedef struct PatchDestInfo {
        /* set during parsing */
    uint8       pdi_State;                      /* PATCH_DEST_STATE_ value.
                                                   Indicates which part of union is valid. */
    union {
        struct {                            /* PATCH_DEST_STATE_CONNECTED */
            const PatchTemplateInfo *SrcTemplate; /* PatchTemplateInfo to connect from. */
            uint16  SrcRsrcIndex;               /* resource in pri_SrcTemplate to connect from. */
            uint16  SrcPartNum;                 /* part in pri_SrcRsrcIndex to connect from. */
        } ConnectInfo;
        struct {                            /* PATCH_DEST_STATE_CONSTANT */
            int32   Value;                      /* F15 constant value */
        } ConstInfo;
    } pdi;

        /* set during resource analysis */
    uint8       pdi_Flags;                      /* PATCH_DEST_F_ flags */
    uint16      pdi_TargetPartNum;              /* part number in target resource. Valid only if
                                                   PATCH_DEST_F_KEEP is set. */
} PatchDestInfo;

    /* PatchDestInfo pdi_State values */
enum {
    PATCH_DEST_STATE_UNUSED,                    /* Destination part hasn't been used yet. Internal to parser. All unused
                                                   destination are set to constants before the parser finishes.
                                                   (@@@ assumed to be 0) */
    PATCH_DEST_STATE_CONNECTED,                 /* Destination part has been connected. Info in pdi.ConnectInfo */
    PATCH_DEST_STATE_CONSTANT                   /* Destination part has been set to a constant value. Info in pdi.ConstInfo */
};

    /* PatchDestInfo pdi_Flags (managed by resource analyzer) */
#define PATCH_DEST_F_KEEP           0x01        /* keep this part */


    /* Resource information. One per resource in template */
typedef struct PatchResourceInfo {
        /* filled out during parsing */
    const DSPPResource *pri_DRsc;               /* Pointer to DSPPResource in pti->pti_DTmp */
    const DSPPDataInitializer *pri_DataInitializer; /* Pointer to original DINI from pti->pti_DTmp, if there is one */
    const char *pri_TargetRsrcName;             /* Pointer to non-empty name to give resource in
                                                   target dtmp_Names array (points to a
                                                   PatchResourceNameInfo n_Name). NULL if unnamed. */
    PatchDestInfo *pri_DestInfo;                /* Array allocated as pri_DestInfo[drsc_Many] for
                                                   PATCH_RESOURCE_CONNECTABLE_DEST, NULL otherwise */
    uint8       pri_Connectability;             /* PATCH_RESOURCE_CONNECTABLE_ */
    uint8       pri_Flags;                      /* PATCH_RESOURCE_F_ flags. (note: some are set during
                                                   resource anaylisis) */

        /* filled out during resource analysis */
    uint16      pri_DestNumParts;               /* Number of dest parts left after optimizing */
    int32       pri_DestConstValue;             /* Resource initializer when PATCH_RESOURCE_F_DEST_SINGLE_INIT
                                                   is set. Only used by PATCH_RESOURCE_CONNECTABLE_DEST
                                                   resources. */
    uint16      pri_TargetRsrcIndex;            /* index of resource in final resource array */
} PatchResourceInfo;

    /* PatchResourceInfo pri_Connectability */
enum {
    PATCH_RESOURCE_NOT_CONNECTABLE,             /* Resource can't be connected or set to a constant (@@@ assumed to be 0) */
    PATCH_RESOURCE_CONNECTABLE_SRC,             /* Resource can be the source of a connection. */
    PATCH_RESOURCE_CONNECTABLE_DEST             /* Resource can be the destination of a connection or be set to a constant. */
};

    /* PatchResourceInfo pri_Flags */
    /* (some only apply to PATCH_RESOURCE_CONNECTABLE_DEST resources, some are set during
       resource analysis) */
#define PATCH_RESOURCE_F_KEEP               0x01    /* When set, include resource in completed patch.
                                                       Otherwise eliminate it. Set initialially by
                                                       parser, conditionally cleared during resource
                                                       analysis. */

#define PATCH_RESOURCE_F_DEST_OPTIMIZE      0x10    /* When set, indicates that dest resource may be
                                                       optimized: number of parts may be reduced or
                                                       completely eliminated, depending on applied
                                                       connections and constants. Otherwise, resource
                                                       allocation must remain as it appears in associated
                                                       DSPPResource. Only applies to
                                                       PATCH_RESOURCE_CONNECTABLE_DEST resources. */

#define PATCH_RESOURCE_F_DEST_NEEDS_MOVE    0x20    /* When set, indicates that dest resource requires a
                                                       MOVE instruction to achieve connection. Otherwise
                                                       twiddle relocations to do connection. Only applies
                                                       to PATCH_RESOURCE_CONNECTABLE_DEST resources. */

#define PATCH_RESOURCE_F_DEST_SINGLE_INIT   0x40    /* When set, indicates that dest resource requires a
                                                       constant to be stored in target drsc_Default and
                                                       that DRSC_F_INIT_AT_ALLOC must be set in target
                                                       resource. Constant value is stored in
                                                       pri_DestConstValue. Only applies to
                                                       PATCH_RESOURCE_CONNECTABLE_DEST resources. */

#define PATCH_RESOURCE_F_DEST_MULTI_INIT    0x80    /* When set, indicates that dest resource requires a
                                                       DINI chunk comprised of values from
                                                       pdi.ConstInfo.Value. Only applies to
                                                       PATCH_RESOURCE_CONNECTABLE_DEST resources. */

#define PATCH_RESOURCE_F_DEST_FLAGS         0xf0    /* composite of PATCH_RESOURCE_F_DEST_ flags */


    /* Template information. One per template added to patch */
struct PatchTemplateInfo {
    Node        pti;                            /* n_Name is block name.
                                                   n_Type is block type.
                                                   n_Size is allocation size. */

        /* links to Template Item */
    const DSPPTemplate *pti_DTmp;               /* DSPPTemplate */
#if 0   /* @@@ not used yet */
    Item        pti_InsTemplate;                /* Audio template item # or perhaps AudioInsTemplate * of
                                                   template containing pti_DTmp. Only valid for
                                                   PATCH_TEMPLATE_TYPE_INSTRUMENT. */
#endif

        /* running analysis of resources */
    PatchResourceInfo *pti_ResourceInfo;        /* Array allocated as pti_ResourceInfo [pti_NumResources] */
    uint16      pti_NumResources;               /* Number of resources (from pti_DTmp->dtmp_NumResources) */
    uint16      pti_NumMoves;                   /* Number of MOVE connections required */

        /* filled out during code analysis */
    uint16      pti_TargetPadCodeBase;          /* NOP padding to write at head of instrument (valid only if padding
                                                ** enabled for this instrument (internal state of LinkPatchCode()) */
    uint16      pti_TargetMoveCodeBase;         /* target code address of start of MOVE block (valid only if pti_NumMoves != 0) */
    uint16      pti_TargetInsCodeBase;          /* base address of code hunk (valid only if pti_DTmp->dtmp_CodeSize != 0) */
};

    /* PatchTemplateInfo pti.n_Type */
enum {
    PATCH_TEMPLATE_TYPE_PORT,                   /* This PatchTemplateInfo is Patch's port template */
    PATCH_TEMPLATE_TYPE_INSTRUMENT              /* This PatchTemplateInfo is one of the constituent instrument templates */
};


    /* Target Resource Name info. One for each unique name that will appear in the
       target template (defined ports, exposed ports, imported resources). Used
       as a memory tracker for allocated resource names and to ensure target
       resource name uniqueness. */
typedef struct PatchResourceNameInfo {
    Node        pni;                            /* n_Name is resource name.
                                                   n_Flags are PATCH_RESOURCE_NAME_F_ flags.
                                                   n_Size is allocated size of structure (including name
                                                   if PATCH_RESOURCE_NAME_F_CONST isn't set) */
} PatchResourceNameInfo;

    /* pni.n_Flags */
#define PATCH_RESOURCE_NAME_F_UNIQUE    0x01    /* Name must be unique in list. */
#define PATCH_RESOURCE_NAME_F_CONST     0x02    /* Name remains constant for life of list. If not set
                                                   name buffer is allocated for name. */

    /* overall patch construction stuff */
typedef struct PatchData {
    List        pd_TemplateList;                /* list of PatchTemplateInfos. names must be unique here. */
    List        pd_ResourceNameList;            /* List of PatchResourceNameInfos. Most names need to be unique.
                                                   The exception is that IMPORTed resources do not need
                                                   to have unique names. Names in this list are pointed
                                                   to by things in pd_TemplateList (e.g. pri_TargetRsrcName) */
    uint8       pd_Flags;                       /* PATCH_DATA_F_ flags (set by createpatch_parser.c) */

        /* place to store ports and knobs
           (allocated if there are any defined ports) */
    DSPPTemplate *pd_PortDTmp;                  /* DSPPTemplate built from defined ports and knobs */
    PatchTemplateInfo *pd_PortTemplate;         /* PatchTemplateInfo for pd_PortDTmp */
} PatchData;

    /* pd_Flags */
#define PATCH_DATA_F_INTER_INSTRUMENT_PADDING   0x01    /* Add NOPs between instrument code hunks when linking
                                                        ** them together. */


/* -------------------- Functions */

    /* PatchCmd parser / PatchData management */
Err dsppParsePatchCmds (PatchData **resultpd, const PatchCmd *patchCmdList);
void dsppDeletePatchData (PatchData *);

    /* Patch linker */
Err dsppLinkPatchTemplate (DSPPTemplate **resultDTmp, PatchData *);


/*****************************************************************************/

#endif /* __CREATEPATCH_INTERNAL_H */
