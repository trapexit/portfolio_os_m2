/******************************************************************************
**
**  @(#) patchcmdbuilder.c 96/02/28 1.20
**
**  PatchCmdBuilder - convenience layer for building PatchCmd lists.
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
**  950711 WJB  Created.
**  951222 WJB  Tweaked debug. Removed a bunch of triple bangs.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <audio/handy_macros.h> /* PROCESSLIST() */
#include <kernel/mem.h>

#include "patchcmdbuilder_internal.h"


/* -------------------- Debug */

#define DEBUG_Add   0           /* print info in AddPatchCmdToBuilder() */

#if DEBUG_Add
#include <stdio.h>
#endif


/* -------------------- Local functions */

static Err AddPatchCmdBlock (PatchCmdBuilder *);
#define FreePatchCmdBlock(block) FreeMem ((block), sizeof (PatchCmdBlock))


/* -------------------- create/delete */

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmdBuilder -name CreatePatchCmdBuilder
|||	Creates a PatchCmdBuilder (convenience environment for constructing a
|||	PatchCmd(@) List).
|||
|||	  Synopsis
|||
|||	    Err CreatePatchCmdBuilder (PatchCmdBuilder **resultBuilder)
|||
|||	  Description
|||
|||	    Creates an empty PatchCmdBuilder for constructing a PatchCmd list using the
|||	    PatchCmd constructors (e.g. AddTemplateToPatch(), DefinePatchPort(), etc.).
|||	    When the PatchCmd list is complete, pass the resulting PatchCmd list
|||	    (returned by GetPatchCmdList()) to CreatePatchTemplate() to compile a Patch
|||	    Template Item.
|||
|||	    Because one typically calls a large number of PatchCmd constructors, it is
|||	    inconvenient to check the success of each call. So when one of the
|||	    constructors fails, the error code it returns is stored in the
|||	    PatchCmdBuilder to prevent further constructors calls from succeeding: the
|||	    original error code is returned by each subsequent constructor call.
|||
|||	    GetPatchCmdBuilderError() also returns this error code. It may be called
|||	    to check the validity of the entire PatchCmd list building process prior
|||	    to using the PatchCmd list (see example below).
|||
|||	    All pointers and Items added to the PatchCmdBuilder must remain valid for the
|||	    life of the PatchCmdBuilder. The PatchCmdBuilder only needs to remain in
|||	    existence until a patch is compiled from its PatchCmd list.
|||
|||	  Arguments
|||
|||	    resultBuilder
|||	        Pointer to a client-supplied buffer to receive a pointer to the
|||	        allocated PatchCmdBuilder.
|||
|||	  Return Value
|||
|||	    Non-negative value on success, negative error code on failure.
|||
|||	    When successful a pointer to the allocated PatchCmdBuilder is written
|||	    to *resultBuilder. Nothing is written to this buffer on failure.
|||
|||	  Implementation
|||
|||	    Library call implemented in libc.a V29.
|||
|||	  Example
|||
|||	    Item MakePatchTemplate (void)
|||	    {
|||	        Item tmpl_sawtooth = -1;
|||	        PatchCmdBuilder *pb = NULL;
|||	        Item result;
|||
|||	            // Load constituent templates
|||	        if ((result = tmpl_sawtooth = LoadInsTemplate ("sawtooth.dsp", NULL)) < 0) goto clean;
|||
|||	            // Create PatchCmdBuilder
|||	        if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;
|||
|||	            // Call PatchCmd constructors
|||	        AddTemplateToPatch (pb, "saw", tmpl_sawtooth);
|||	        DefinePatchPort (pb, "Output", 1, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);
|||	        ConnectPatchPorts (pb, "saw", "Output", 0, NULL, "Output", 0);
|||
|||	            // See if an error occured in one of the constructors
|||	        if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;
|||
|||	            // Create patch template
|||	        result = CreatePatchTemplate (GetPatchCmdList(pb), NULL);
|||
|||	    clean:
|||	        DeletePatchCmdBuilder (pb);
|||	        UnloadInsTemplate (tmpl_sawtooth);
|||	        return result;
|||	    }
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>, libc.a
|||
|||	  See Also
|||
|||	    PatchCmd(@), DeletePatchCmdBuilder(), GetPatchCmdList(),
|||	    GetPatchCmdBuilderError(), CreatePatchTemplate()
**/

Err CreatePatchCmdBuilder (PatchCmdBuilder **resultBuilder)
{
    PatchCmdBuilder *builder;
    Err errcode;

    *resultBuilder = NULL;

    if (!(builder = AllocMem (sizeof *builder, MEMTYPE_ANY | MEMTYPE_FILL))) {
        errcode = PATCH_ERR_NOMEM;
        goto clean;
    }
    PrepList (&builder->pcb_Blocks);
    if ((errcode = AddPatchCmdBlock (builder)) < 0) goto clean;

    *resultBuilder = builder;
    return 0;

clean:
    DeletePatchCmdBuilder (builder);
    return errcode;
}


/**
|||	AUTODOC -public -class AudioPatch -group PatchCmdBuilder -name DeletePatchCmdBuilder
|||	Deletes a PatchCmdBuilder created by CreatePatchCmdBuilder().
|||
|||	  Synopsis
|||
|||	    void DeletePatchCmdBuilder (PatchCmdBuilder *builder)
|||
|||	  Description
|||
|||	    Deletes PatchCmdBuilder built by CreatePatchCmdBuilder() and all memory
|||	    allocated by PatchCmd constructors. Does not delete any of the strings or
|||	    Items placed in PatchCmds built in the PatchCmdBuilder.
|||
|||	  Arguments
|||
|||	    builder
|||	        Pointer to PatchCmdBuilder to delete. Can be NULL.
|||
|||	  Implementation
|||
|||	    Library call implemented in libc.a V29.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>, libc.a
|||
|||	  See Also
|||
|||	    PatchCmd(@), CreatePatchCmdBuilder()
**/

void DeletePatchCmdBuilder (PatchCmdBuilder *builder)
{
    if (builder) {
        PatchCmdBlock *block, *next;

        PROCESSLIST (&builder->pcb_Blocks, block, next, PatchCmdBlock) {
            FreePatchCmdBlock (block);
        }
        FreeMem (builder, sizeof *builder);
    }
}


/**
|||	AUTODOC -public -class AudioPatch -group PatchCmdBuilder -name GetPatchCmdList
|||	Returns PatchCmd list from a PatchCmdBuilder.
|||
|||	  Synopsis
|||
|||	    const PatchCmd *GetPatchCmdList (const PatchCmdBuilder *builder)
|||
|||	  Description
|||
|||	    Returns a pointer to the first PatchCmd in the list being constructed
|||	    by the PatchCmdBuilder. Call this in order to get a PatchCmd list suitable
|||	    for CreatePatchTemplate()
|||
|||	  Arguments
|||
|||	    builder
|||	        Pointer to PatchCmdBuilder to interrogate.
|||
|||	  Return Value
|||
|||	    Pointer to a PatchCmd list. Always returns a legal PatchCmd list for a
|||	    valid PatchCmdBuilder, even if an empty one if called before any
|||	    constructors are called.
|||
|||	    Returns NULL if one of the PatchCmd constructors failed (when
|||	    GetPatchCmdBuilderError() returns a negative value).
|||
|||	    Apart from the NULL returned in case of a constructor failure, the address
|||	    returned by this function is constant for the life of the PatchCmdBuilder.
|||
|||	  Example
|||
|||	    templateItem = CreatePatchTemplateVA (
|||	        GetPatchCmdList(builder),
|||	        TAG_END);
|||
|||	  Implementation
|||
|||	    Library call implemented in libc.a V29.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>, libc.a
|||
|||	  See Also
|||
|||	    PatchCmd(@), CreatePatchCmdBuilder(), CreatePatchTemplate(), GetPatchCmdBuilderError()
**/

const PatchCmd *GetPatchCmdList (const PatchCmdBuilder *builder)
{
    return builder->pcb_LastError >= 0
        ? ((PatchCmdBlock *)FIRSTNODE(&builder->pcb_Blocks))->pcbk_Cmds
        : NULL;
}


/**
|||	AUTODOC -public -class AudioPatch -group PatchCmdBuilder -name GetPatchCmdBuilderError
|||	Returns error code from a failed PatchCmd constructor.
|||
|||	  Synopsis
|||
|||	    Err GetPatchCmdBuilderError (const PatchCmdBuilder *builder)
|||
|||	  Description
|||
|||	    When one of the PatchCmd constructor functions (e.g., AddTemplateToPatch(),
|||	    DefinePatchPort(), etc.) fails, all subsequent constructors also fail
|||	    with the same error code. This function also returns that error code. If no
|||	    PatchCmd constructors have failed, this function returns 0. When this
|||	    function returns an error code, GetPatchCmdList() returns NULL.
|||
|||	    You may use this function instead of checking the success of the constructor
|||	    calls to simplify your code. See CreatePatchCmdBuilder() for an example.
|||
|||	  Arguments
|||
|||	    builder
|||	        Pointer to PatchCmdBuilder to test.
|||
|||	  Return Value
|||
|||	    The error code (a negative value) returned by the first failed PatchCmd
|||	    contructor call in this PatchCmdBuilder, or 0 if no PatchCmd constructors
|||	    have failed so far.
|||
|||	  Implementation
|||
|||	    Library call implemented in libc.a V30.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>, libc.a
|||
|||	  See Also
|||
|||	    CreatePatchCmdBuilder(), GetPatchCmdList()
**/

Err GetPatchCmdBuilderError (const PatchCmdBuilder *builder)
{
    return builder->pcb_LastError;
}


/* -------------------- PatchCmd constructor helper */

/*
    Add a PatchCmd to the builder. Adds a new PatchCmdBlock if necessary.
    Called by PatchCmd constructors.

    Reads and sets pcb_LastError.
*/
Err AddPatchCmdToBuilder (PatchCmdBuilder *builder, const PatchCmd *newCmd)
{
    Err errcode;

  #if DEBUG_Add
    DumpPatchCmd (newCmd, "AddPatchCmdToBuilder: ");
  #endif

        /* Have we encountered an error? If so, return immediately. */
    if (builder->pcb_LastError < 0) return builder->pcb_LastError;

        /* see if we need a new block, leave room at end for PATCH_CMD_JUMP or PATCH_CMD_END */
    if (builder->pcb_CurCmdIndex >= PCBK_NUM_CMDS-1) {
        PatchCmdJump * const linkcmd = (PatchCmdJump *)&builder->pcb_CurBlock->pcbk_Cmds[PCBK_NUM_CMDS-1];

        if ((errcode = AddPatchCmdBlock (builder)) < 0) goto clean;

        linkcmd->pc_CmdID        = PATCH_CMD_JUMP;
        linkcmd->pc_NextPatchCmd = &builder->pcb_CurBlock->pcbk_Cmds[0];
    }

        /* install new cmd into current block */
    {
        PatchCmd *destcmd = &builder->pcb_CurBlock->pcbk_Cmds [builder->pcb_CurCmdIndex];

        *destcmd++ = *newCmd;
        destcmd->pc_Generic.pc_CmdID = PATCH_CMD_END;   /* mark next cmd as end */
    }

        /* increment CurCmdIndex */
    builder->pcb_CurCmdIndex++;

    return 0;

clean:
        /* Set pcb_LastError to prevent further construction */
    builder->pcb_LastError = errcode;
    return errcode;
}


/* -------------------- PatchCmdBlock manager */

/*
    Allocate and add a PatchCmdBlock to pcb_Blocks.
    Free blocks w/ FreePatchCmdBlock() macro.
*/
static Err AddPatchCmdBlock (PatchCmdBuilder *builder)
{
    PatchCmdBlock *block;

  #if DEBUG_Add
    printf ("AddPatchCmdBlock\n");
  #endif

        /* allocate/init */
    if (!(block = AllocMem (sizeof *block, MEMTYPE_ANY | MEMTYPE_FILL))) return PATCH_ERR_NOMEM;
    block->pcbk_Cmds[0].pc_Generic.pc_CmdID = PATCH_CMD_END;   /* mark first cmd as end */

        /* add to list */
    AddTail (&builder->pcb_Blocks, (Node *)block);
    builder->pcb_CurBlock    = block;
    builder->pcb_CurCmdIndex = 0;

    return 0;
}
