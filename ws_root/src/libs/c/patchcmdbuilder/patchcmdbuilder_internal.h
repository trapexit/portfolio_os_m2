#ifndef __PATCHCMDBUILDER_INTERNAL_H
#define __PATCHCMDBUILDER_INTERNAL_H

/******************************************************************************
**
**  @(#) patchcmdbuilder_internal.h 96/02/16 1.4
**
**  PatchCmdBuilder internal include
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
**  960129 WJB  Created.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <audio/patch.h>
#include <kernel/list.h>


/* -------------------- Internal structures */

#define PCBK_NUM_CMDS   32          /* # of cmds per PatchCmdBlock */

typedef struct PatchCmdBlock {
    MinNode     pcbk;
    PatchCmd    pcbk_Cmds[PCBK_NUM_CMDS];
} PatchCmdBlock;

struct PatchCmdBuilder {
    List        pcb_Blocks;         /* list of PatchCmdBlocks */
    PatchCmdBlock *pcb_CurBlock;    /* pointer to PatchCmdBlock being constructed */
    int32       pcb_CurCmdIndex;    /* index of next PatchCmd in *pcb_CurBlocks */
    Err         pcb_LastError;      /* Last error encountered */
};


/* -------------------- Internal functions */

Err AddPatchCmdToBuilder (PatchCmdBuilder *builder, const PatchCmd *newCmd);


/*****************************************************************************/

#endif  /* __PATCHCMDBUILDER_INTERNAL_H */
