/******************************************************************************
**
**  @(#) loadpatch.c 96/06/19 1.43
**
**  Patch file (FORM 3PCH) loader
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
**  951023 WJB  Created.
**  951031 WJB  Added FORM 3PCH support (both primary and nested).
**  951101 WJB  Added some PCMD chunk parsing diagnostics.
**              Fixed up a couple of small failure path problems.
**              Improved debug code.
**  951102 WJB  Now using StoredItems and taking advantage of IFF_CIL_PROP for
**              FORM entry and exit handlers.
**  951103 WJB  Added placeholder for FORM PATT handler.
**  951103 WJB  Refined StoredItem support a bit.
**  951107 WJB  Added placeholder for FORM PTUN stuff.
**  951110 WJB  Removed DeleteStoredItem().
**              Added DataChunk handler (publish later).
**  951110 WJB  Added FORM PATT handler.
**  951127 WJB  Added no attachments trap in FORM PATT handler.
**  951128 WJB  Fixed ExitFormPATT() failure path.
**  951128 WJB  Improved no attachments trap in FORM PATT handler.
**  951129 WJB  Now including audio/handy_macros.h.
**              Enhanced PCMD and PNAM diagnostic handling.
**  951129 WJB  Disabled incomplete PTUN handling.
**              Downgraded some triple bangs.
**  951130 WJB  Moved stored item and data chunk support to libs/music/iff.
**  960301 WJB  Merged contents of libs/music/patch in preparation for DLL-ification.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
**-----------------------------------------------------------------------------
**
**  Notes:
**
**  1. @@@ FORM PTUN (tuning property for patch) is disabled until there is some
**     resolution about how to delete a tuning attached to a Template Item
**     (maybe using AF_TAG_AUTO_DELETE_SLAVE). See '@@@ PTUN' for all the areas
**     affected.
**
******************************************************************************/

#include <audio/atag.h>
#include <audio/audio.h>
#include <audio/handy_macros.h>     /* PackedString stuff, PROCESSLIST() */
#include <audio/music_iff.h>        /* StoredItem, DataChunk */
#include <audio/patch.h>
#include <audio/patchfile.h>        /* self */
#include <kernel/mem.h>
#include <misc/iff.h>
#include <string.h>


/* -------------------- Debug control */

#define DEBUG_Load  0   /* turns on debugging while loading */


/* -------------------- Diagnostics */

#ifdef BUILD_STRINGS
#include <stdio.h>
#define ERR(x) printf x
#else
#define ERR(x)
#endif


/* -------------------- Misc autodocs */

/**
|||	AUTODOC -public -class AudioPatchFile -name --Patch-File-Overview--
|||	Overview of the FORM 3PCH binary patch file format.
|||
|||	  File Format -preformatted
|||
|||	    FORM 3PCH {
|||
|||
|||	        Patch Compiler Input - This FORM represents the PatchCmd list to be
|||	        given to the patch compiler. There must be exactly 1 FORM PCMD in the
|||	        FORM 3PCH (not counting nesting).
|||
|||	        FORM PCMD {
|||
|||	            Constituent Template list for Patch. Any number and order of these
|||	            chunks/FORMs. The resulting templates are referenced by index by the
|||	            PCMD chunk.
|||
|||	            PTMP {
|||	                PackedStringArray   // packed string array of .dsp template names
|||	            }
|||	            PMIX {
|||	                MixerSpec[]         // array of MixerSpecs
|||	            }
|||	            FORM 3PCH {             // nested patch
|||	                .
|||	                .
|||	                .
|||	            }
|||
|||
|||	            Packed string array of all strings required by PatchCmd list.
|||	            Indexed by keys stored in PCMD chunk. There must be one of these in
|||	            the FORM PCMD.
|||
|||	            PNAM {
|||	                PackedStrings
|||	            }
|||
|||
|||	            Array of file-conditioned PatchCmds terminated by a PATCH_CMD_END:
|||
|||	            - char * are stored as uint32 byte offset into PNAM chunk + 1, NULL is 0.
|||
|||	            - InsTemplate items are stored as index into List of templates defined
|||	              stored in PTMP, PMIX, and nested FORM 3PCHs in order of appearance
|||	              in the FORM PCMD.
|||
|||	            PCMD {
|||	                PatchCmd[]
|||	            }
|||	        }
|||
|||
|||	        Tuning - Defines a custom tuning for the the Patch Template. Either 0
|||	        or 1 of these FORMs (@@@ not yet supported)
|||
|||	        FORM PTUN {
|||	            ATAG {
|||	                AudioTagHeader AUDIO_TUNING_NODE
|||	            }
|||	            BODY {
|||	                float32 TuningData[]
|||	            }
|||	        }
|||
|||
|||	        Attachments - Each FORM PATT defines a slave Item and attachments for
|||	        that Item. There may be any number of these FORMs in a FORM 3PCH. If
|||	        present, they must appear after the FORM PCMD.
|||
|||	        Each PATT chunk within a FORM PATT defines one attachment between the
|||	        slave defined by the FORM PATT and the patch. There must be at least
|||	        one PATT chunk in each FORM PATT.
|||
|||	        FORM PATT {
|||	            PATT {
|||	                PatchAttachment
|||	            }
|||	            PATT {
|||	                PatchAttachment
|||	            }
|||	            .
|||	            .
|||	            .
|||	            ATAG {
|||	                AudioTagHeader
|||	            }
|||	            BODY {
|||	                data
|||	            }
|||	        }
|||
|||	  Description
|||
|||	    This is an IFF FORM which is capable of storing a PatchCmd(@) list and list
|||	    of attachments for a Patch Template. The function LoadPatchTemplate() loads
|||	    this file format. The shell program makepatch(@) and the graphical patch
|||	    creation program ARIA write patch files in this format.
|||
|||	    The AudioPatchFile folio offers two ways to parse a FORM 3PCH:
|||
|||	    LoadPatchTemplate()
|||	        Loads a FORM 3PCH out of the named file. This is the simplest way to
|||	        load a patch written by makepatch(@) or ARIA.
|||
|||	    EnterForm3PCH(), ExitForm3PCH()
|||	        These two functions are IFF Folio entry and exit handlers for FORM 3PCH.
|||	        You may install these handlers, your own FORM 3PCH handlers which then
|||	        call these, or a combination. Access to the handlers permits you to
|||	        parse one or more FORM 3PCHs embedded in a larger IFF context (e.g., a
|||	        score file).
|||
|||	        You don't need to bother with these functions if all you want to do is
|||	        load a patch made by makepatch(@) or ARIA. Just use
|||	        LoadPatchTemplate(), which is a client of these handlers.
|||
|||	    In order to facilitate simple and complete deletion of a loaded patch
|||	    template, the parser creates slave items (for Sample(@)s and Envelope(@)s
|||	    loaded from the patch file) with { AF_TAG_AUTO_FREE_DATA, TRUE }, and
|||	    Attachment(@)s to these slave items with { AF_TAG_AUTO_DELETE_SLAVE, TRUE }.
|||
|||	  Caveats
|||
|||	    Delay lines, like other attached items, belong to the patch template, so all
|||	    instruments created from such a patch share the same delay line. If you want
|||	    a delay line per instrument, use a delay line template in the patch instead
|||	    of a delay line. See Sample(@) for more on this.
|||
|||	    Doesn't yet support custom tuning information stored in patches (FORM PTUN).
|||
|||	    Attachments in nested patches aren't propagated to the enclosing patch (a
|||	    limitation of the patch compiler).
|||
|||	  Associated Files
|||
|||	    <audio/patchfile.h>, <audio/patch.h>, <audio/atag.h>,
|||	    System.m2/Modules/audiopatch, System.m2/Modules/audiopatchfile
|||
|||	  See Also
|||
|||	    LoadPatchTemplate(), EnterForm3PCH(), ExitForm3PCH(), PatchCmd(@),
|||	    makepatch(@), Template(@), Sample(@), Envelope(@), Tuning(@), Attachment(@)
**/


/* -------------------- Load/UnloadPatchTemplate() */

/**
|||	AUTODOC -public -class AudioPatchFile -name LoadPatchTemplate
|||	Loads a binary patch file (FORM 3PCH).
|||
|||	  Synopsis
|||
|||	    Item LoadPatchTemplate (const char *fileName)
|||
|||	  Description
|||
|||	    This function loads a binary Patch Template(@) file prepared by audio tools
|||	    such as makepatch(@) and ARIA, and returns a Patch Template(@) Item. The
|||	    Patch Template Item is given the same name as the patch file.
|||
|||	    Unload it with UnloadPatchTemplate().
|||
|||	    This function assumes that the patch you want to load is in a discrete file.
|||	    To parse a FORM 3PCH in a larger IFF context, use EnterForm3PCH() and
|||	    ExitForm3PCH(). See --Patch-File-Overview--(@) for more information on this.
|||
|||	  Arguments
|||
|||	    fileName
|||	        Name of patch template file to load. This name is also given to the
|||	        Patch Template Item.
|||
|||	  Return Value
|||
|||	    The procedure returns an item number of the Template if successful (a
|||	    non-negative value) or an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in AudioPatchFile Folio V30.
|||
|||	  Notes -preformatted
|||
|||	    This function is equivalent to:
|||
|||	    Item LoadPatchTemplate (const char *fileName)
|||	    {
|||	        IFFParser *iff;
|||	        Item result;
|||
|||	            // Open IFF file (sets iff to NULL on failure).
|||	        if ((result = CreateIFFParserVA (&iff, FALSE,
|||	            IFF_TAG_FILE, fileName,
|||	            TAG_END)) < 0) goto clean;
|||
|||	            // Install FORM 3PCH entry handler.
|||	            // Pass the fileName to it so that the resulting Item is named the
|||	            // same as the patch file.
|||	        if ((result = InstallEntryHandler (iff, ID_3PCH, ID_FORM, IFF_CIL_TOP,
|||	            (IFFCallBack)EnterForm3PCH, fileName)) < 0) goto clean;
|||
|||	            // Install FORM 3PCH exit handler.
|||	            // Take advantage of the fact that ExitForm3PCH() returns the
|||	            // completed Item number, which causes ParseIFF() to stop parsing
|||	            // and return it.
|||	        if ((result = InstallExitHandler  (iff, ID_3PCH, ID_FORM, IFF_CIL_TOP,
|||	            (IFFCallBack)ExitForm3PCH, NULL)) < 0) goto clean;
|||
|||	            // Parse file: returns completed Patch Template Item on success
|||	            // (because of what ExitForm3PCH() returns).
|||	            // Translate otherwise meaningless IFF completion codes into suitable
|||	            // errors.
|||	        switch (result = ParseIFF (iff, IFF_PARSE_SCAN)) {
|||	            case IFF_PARSE_EOF:
|||	                result = PATCHFILE_ERR_MANGLED;
|||	                break;
|||	        }
|||
|||	    clean:
|||	        DeleteIFFParser (iff);
|||	        return result;
|||	    }
|||
|||	  Caveats
|||
|||	    Doesn't yet support custom tuning information stored in patches (FORM PTUN).
|||
|||	    Attachments in nested patches aren't propagated to the enclosing patch (a
|||	    limitation of the patch compiler).
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio(), OpenAudioPatchFileFolio()
|||
|||	  Associated Files
|||
|||	    <audio/patchfile.h>, System.m2/Modules/audio,
|||	    System.m2/Modules/audiopatchfile
|||
|||	  See Also
|||
|||	    --Patch-File-Overview--(@), UnloadPatchTemplate(), LoadScoreTemplate(),
|||	    EnterForm3PCH(), ExitForm3PCH(), CreatePatchTemplate()
**/
Item LoadPatchTemplate (const char *fileName)
{
    IFFParser *iff;
    Item result;

        /* Open IFF file (sets iff to NULL on failure). */
    if ((result = CreateIFFParserVA (&iff, FALSE, IFF_TAG_FILE, fileName, TAG_END)) < 0) goto clean;

        /* Install FORM 3PCH entry handler.
        ** Pass the fileName to it so that the resulting Item is named the
        ** same as the patch file.
        */
    if ((result = InstallEntryHandler (iff, ID_3PCH, ID_FORM, IFF_CIL_TOP, (IFFCallBack)EnterForm3PCH, fileName)) < 0) goto clean;

        /* Install FORM 3PCH exit handler.
        ** Take advantage of the fact that ExitForm3PCH() returns the
        ** completed Item number, which causes ParseIFF() to stop parsing
        ** and return it.
        */
        /* @@@ assumes that the Item number returned by ExitForm3PCH() will never be equal to IFF_CB_CONTINUE (1) */
    if ((result = InstallExitHandler  (iff, ID_3PCH, ID_FORM, IFF_CIL_TOP, (IFFCallBack)ExitForm3PCH,  NULL)) < 0) goto clean;

        /* Parse file: returns completed Patch Template Item on success
        ** (because of what ExitForm3PCH() returns).
        ** Translate otherwise meaningless IFF completion codes into suitable
        ** errors.
        */
    switch (result = ParseIFF (iff, IFF_PARSE_SCAN)) {
        case IFF_PARSE_EOF:
            result = PATCHFILE_ERR_BAD_TYPE;
            break;
    }

clean:
    DeleteIFFParser (iff);
    return result;
}


/**
|||	AUTODOC -public -class AudioPatchFile -name UnloadPatchTemplate
|||	Unloads a Patch Template(@) loaded by LoadPatchTemplate().
|||
|||	  Synopsis
|||
|||	    Err UnloadPatchTemplate (Item patchTemlate)
|||
|||	  Description
|||
|||	    Unloads a patch template loaded by LoadPatchTemplate(). This has the same
|||	    side effects as UnloadInsTemplate() (e.g., deleting instruments,
|||	    attachments, slaves of AF_TAG_AUTO_DELETE_SLAVE attachments, etc.)
|||
|||	  Arguments
|||
|||	    patchTemplate
|||	        Patch Template Item to unload.
|||
|||	  Return Value
|||
|||	    The procedure returns a non-negative value on success or an error code (a
|||	    negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in <audio/patchfile.h> V29.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/patchfile.h>, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    LoadPatchTemplate()
**/


/* -------------------- FORM 3PCH Parser */

    /* FORM 3PCH context data */
typedef struct PatchData {
        /* constants */
    const char *pd_PatchName;       /* Name to give Patch Template created by FORM PCMD. NULL for nested patches */
} PatchData;

    /* FORM PCMD context data */
typedef struct PCMDData {
    List        pcmd_TemplateList;  /* list of constituent PCMDTemplates for FORM PCMD */
} PCMDData;

typedef struct PCMDTemplate {       /* one of these per constituent template */
    MinNode     ptmp;
    Item        ptmp_Item;          /* Template Item */
} PCMDTemplate;

    /* FORM PCMD */
static Err RegisterFormPCMD (IFFParser *);

    /* FORM PATT */
static Err RegisterFormPATT (IFFParser *);

    /* FORM PTUN */
/* static Err RegisterFormPTUN (IFFParser *); @@@ PTUN not implemented yet */

static Err BufferChunk (void **resultBuf, IFFParser *, int32 bufSize);


/* -------------------- Enter/ExitForm3PCH() */

/**
|||	AUTODOC -public -class AudioPatchFile -name EnterForm3PCH
|||	Standard FORM 3PCH entry handler.
|||
|||	  Synopsis
|||
|||	    Err EnterForm3PCH (IFFParser *iff, const char *patchName)
|||
|||	  Description
|||
|||	    This function creates context data and registers additional chunks and
|||	    handlers for parsing a FORM 3PCH patch. Either install this function as the
|||	    FORM 3PCH entry handler, or call it from your own FORM 3PCH entry handler.
|||
|||	    This function must be used in conjunction with ExitForm3PCH().
|||
|||	  Arguments
|||
|||	    iff
|||	        IFFParser positioned at the entry of a FORM 3PCH.
|||
|||	    patchName
|||	        Name to give the Patch Template Item created and returned by
|||	        ExitForm3PCH(), or NULL to leave the Item unnamed. This string is
|||	        copied when the Item is created, but not until then. So the string must
|||	        remain valid until the FORM 3PCH context is exited.
|||
|||	  Return Value
|||
|||	    The procedure returns IFF_CB_CONTINUE if successful, or an error code (a
|||	    negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in AudioPatchFile Folio V30.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio(), OpenAudioPatchFileFolio(), OpenIFFFolio()
|||
|||	  Associated Files
|||
|||	    <audio/patchfile.h>, System.m2/Modules/audio,
|||	    System.m2/Modules/audiopatchfile, System.m2/Modules/iff
|||
|||	  See Also
|||
|||	    --Patch-File-Overview--(@), ExitForm3PCH(), InstallEntryHandler(),
|||	    LoadPatchTemplate()
**/

Err EnterForm3PCH (IFFParser *iff, const char *patchName)
{
    ContextInfo *ci;
    PatchData *pd;
    Err errcode;

  #if DEBUG_Load
    IFFPrintf (iff, "EnterForm3PCH name='%s'\n", patchName);
  #endif

        /* create/init context for FORM 3PCH data */
    if (!(ci = AllocContextInfo (ID_3PCH, ID_3PCH, ID_3PCH, sizeof (PatchData), NULL))) {
        errcode = PATCHFILE_ERR_NOMEM;
        goto clean;
    }
    pd = (PatchData *)ci->ci_Data;
    pd->pd_PatchName = patchName;

        /* register "property" FORMs */
    if ((errcode = RegisterFormPCMD (iff)) < 0) goto clean;
    if ((errcode = RegisterFormPATT (iff)) < 0) goto clean;
/*  if ((errcode = RegisterFormPTUN (iff)) < 0) goto clean; @@@ PTUN not implemented yet */

        /* store in FORM context */
        /* @@@ do this last to avoid having to RemoveContextInfo() on failure */
    if ((errcode = StoreContextInfo (iff, ci, IFF_CIL_TOP)) < 0) goto clean;

        /* success: keep parsing */
    return IFF_CB_CONTINUE;

clean:
    FreeContextInfo (ci);
    return errcode;
}


/**
|||	AUTODOC -public -class AudioPatchFile -name ExitForm3PCH
|||	Standard FORM 3PCH exit handler.
|||
|||	  Synopsis
|||
|||	    Item ExitForm3PCH (IFFParser *iff, void *dummy)
|||
|||	  Description
|||
|||	    This function completes the FORM 3PCH parsing started by EnterForm3PCH(),
|||	    and returns the Item number of the completed Patch Template. Either install
|||	    this function as the FORM 3PCH exit handler, or call it from your own FORM
|||	    3PCH exit handler.
|||
|||	    Because this function returns an Item number instead of IFF_CB_CONTINUE, when
|||	    used as the exit handler (instead of being called by your own) it causes
|||	    ParseIFF() to stop parsing the IFF stream and to return this item number.
|||	    LoadPatchTemplate() takes advantage of this behavior.
|||
|||	    This function must be used in conjunction with EnterForm3PCH().
|||
|||	  Arguments
|||
|||	    iff
|||	        IFFParser positioned at the exit of a FORM 3PCH.
|||
|||	    dummy
|||	        This argument is isn't used, and is here only to fit the IFFCallBack
|||	        prototype. Set it to NULL.
|||
|||	  Return Value
|||
|||	    The procedure returns a Patch Template Item number (a non-negative value) if
|||	    successful, or an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in AudioPatchFile Folio V30.
|||
|||	  Examples
|||
|||	    Assume that we are collecting a list of patches from an IFF file (error
|||	    handling omitted). Use a custom FORM 3PCH handler to put the results of
|||	    ExitForm3PCH() into our list.
|||
|||	    {
|||	        .
|||	        .
|||	        if ((errcode = InstallEntryHandler (iff, ID_3PCH, ID_FORM, IFF_CIL_TOP,
|||	            (IFFCallBack)EnterForm3PCH, NULL)) < 0) goto clean;
|||	        if ((errcode = InstallExitHandler (iff, ID_3PCH, ID_FORM, IFF_CIL_TOP,
|||	            (IFFCallBack)MyExitForm3PCH, &patchList)) < 0) goto clean;
|||	        .
|||	        .
|||	        if ((errcode = ParseIFF (iff, IFF_PARSE_SCAN)) < 0) goto clean;
|||	        .
|||	        .
|||	    }
|||
|||	    // custom exit handler - gets Patch Template Item from FORM 3PCH parser
|||	    // and adds to to patchList, calling our
|||	    Err MyExitForm3PCH (IFFParser *iff, List *patchList)
|||	    {
|||	        Err errcode;
|||	        Item patchTemplate;
|||
|||	            // get patch template from FORM 3PCH handler by calling ExitForm3PCH()
|||	        if ((errcode = patchTemplate = ExitForm3PCH (iff, NULL))) < 0) goto clean;
|||
|||	            // add patch to list
|||	        if ((errcode = AddPatchToList (patchList, patchTemplate)) < 0) goto clean;
|||
|||	            // success: keep parsing
|||	        return IFF_CB_CONTINUE;
|||
|||	    clean:
|||	            // failure: delete patch (because it didn't get added to list)
|||	            // and return error code.
|||	        DeleteItem (patchTemplate);
|||	        return errcode;
|||	    }
|||
|||	    Err AddPatchToList (List *patchList, Item patchTemplate)
|||	    {
|||	        .
|||	        .
|||	        .
|||	    }
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio(), OpenAudioPatchFileFolio(), OpenIFFFolio()
|||
|||	  Associated Files
|||
|||	    <audio/patchfile.h>, System.m2/Modules/audio,
|||	    System.m2/Modules/audiopatchfile, System.m2/Modules/iff
|||
|||	  See Also
|||
|||	    --Patch-File-Overview--(@), EnterForm3PCH(), InstallExitHandler(),
|||	    LoadPatchTemplate()
**/

/*
    @@@ Clients of this function assume that the resulting Item number will
        never be equal to IFF_CB_CONTINUE (1), which would cause ParseIFF()
        to continue parsing rather than returning the completed Item.
*/
Item ExitForm3PCH (IFFParser *iff, void *dummy)
{
        /* extract items */
    const Item patchTemplate = ExtractStoredItem (iff, ID_3PCH, ID_PCMD);
/*  const Item tuning = ExtractStoredItem (iff, ID_3PCH, ID_PTUN);  @@@ PTUN not implemented yet */
    Err errcode;

    TOUCH (dummy);

        /* find template */
    if ((errcode = patchTemplate) <= 0) {
        if (!errcode) errcode = PATCHFILE_ERR_MANGLED;
        goto clean;
    }

  #if DEBUG_Load
    IFFPrintf (iff, "ExitForm3PCH\t0x%x\n", patchTemplate);
  #endif

#if 0   /* @@@ PTUN not implemented yet */
        /* find tuning. if there is one, attach it to template */
    if ((errcode = tuning) < 0) goto clean;
    if (tuning) {
        if ((errcode = TuneInsTemplate (patchTemplate, tuning)) < 0) goto clean;
    }
#endif

        /* success: return patch template */
    return patchTemplate;

clean:
        /* DeleteItem() safely ignores items <= 0 */
/*  DeleteItem (tuning);    @@@ PTUN not implemented yet */
    DeleteItem (patchTemplate);
    return errcode;
}


/* -------------------- FORM PCMD */

static Err EnterFormPCMD (IFFParser *, void *);
static Err PurgePCMDData (IFFParser *, ContextInfo *);
static Err HandlePTMPChunk (IFFParser *, PCMDData *);
static Err HandlePMIXChunk (IFFParser *, PCMDData *);
static Err ExitNestedForm3PCH (IFFParser *, PCMDData *);
static Err AddPCMDTemplate (PCMDData *, Item ptmpItem);
static Err ExitFormPCMD (IFFParser *, PCMDData *);


/*
    Installs FORM PCMD entry handler and stores dummy item.

    Results
        0 on success, Err code on failure.
*/
static Err RegisterFormPCMD (IFFParser *iff)
{
    Err errcode;

        /* store dummy template to avoid picking one up from lower on the context stack */
    if ((errcode = StoreItemInContext (iff, ID_3PCH, ID_PCMD, IFF_CIL_TOP, 0)) < 0) goto clean;

        /* register entry handler */
    if ((errcode = InstallEntryHandler (iff, ID_PCMD, ID_FORM, IFF_CIL_TOP, (IFFCallBack)EnterFormPCMD, NULL)) < 0) goto clean;

    return 0;

clean:
    return errcode;
}


/*
    FORM PCMD entry handler. Sets up FORM PCMD context data, and adds remaining
    FORM PCMD handlers.

    Results
        IFF_CB_CONTINUE on success, Err code on failure.
*/
static Err EnterFormPCMD (IFFParser *iff, void *dummy)
{
    static const IFFTypeID propChunks[] = {
        { ID_PCMD, ID_PNAM },
        { ID_PCMD, ID_PCMD },
        0
    };
    ContextInfo *ci;
    PCMDData *pcmddata;
    Err errcode;

    TOUCH(dummy);

  #if DEBUG_Load
    IFFPrintf (iff, "EnterFormPCMD\n");
  #endif

        /* create/init context for FORM PCMD data */
    if (!(ci = AllocContextInfo (ID_PCMD, ID_PCMD, ID_PCMD, sizeof (PCMDData), (IFFCallBack)PurgePCMDData))) {
        errcode = PATCHFILE_ERR_NOMEM;
        goto clean;
    }
    pcmddata = (PCMDData *)ci->ci_Data;
    PrepList (&pcmddata->pcmd_TemplateList);

        /* Handlers for things that add to pcmd_TemplateList */
    if ((errcode = InstallEntryHandler (iff, ID_PCMD, ID_PTMP, IFF_CIL_TOP, (IFFCallBack)HandlePTMPChunk,    pcmddata)) < 0) goto clean;
    if ((errcode = InstallEntryHandler (iff, ID_PCMD, ID_PMIX, IFF_CIL_TOP, (IFFCallBack)HandlePMIXChunk,    pcmddata)) < 0) goto clean;
    if ((errcode = InstallEntryHandler (iff, ID_3PCH, ID_FORM, IFF_CIL_TOP, (IFFCallBack)EnterForm3PCH,      NULL)) < 0) goto clean;
    if ((errcode = InstallExitHandler  (iff, ID_3PCH, ID_FORM, IFF_CIL_TOP, (IFFCallBack)ExitNestedForm3PCH, pcmddata)) < 0) goto clean;

        /* FORM PCMD property and exit handler */
    if ((errcode = RegisterPropChunks (iff, propChunks)) < 0) goto clean;
    if ((errcode = InstallExitHandler (iff, ID_PCMD, ID_FORM, IFF_CIL_TOP, (IFFCallBack)ExitFormPCMD, pcmddata)) < 0) goto clean;

        /* store in FORM context */
        /* @@@ do this last to avoid having to RemoveContextInfo() on failure */
    if ((errcode = StoreContextInfo (iff, ci, IFF_CIL_TOP)) < 0) goto clean;

        /* success: keep parsing */
    return IFF_CB_CONTINUE;

clean:
    PurgePCMDData (iff, ci);
    return errcode;
}

/*
    PCMDData ContextInfo purge method

    Arguments
        ci
            ContextInfo containing pointer to PCMDData structure to purge.
            Can be NULL. If non-NULL, ci->ci_Data must be valid.
*/
static Err PurgePCMDData (IFFParser *iff, ContextInfo *ci)
{
    TOUCH(iff);

  #if DEBUG_Load
    IFFPrintf (iff, "PurgePCMDData\n");
  #endif

        /* @@@ tolerates NULL because HandleDataChunk() expects it to */
    if (ci) {
        PCMDData * const pcmddata = ci->ci_Data;

            /* delete Template Items in pcmd_TemplateList */
        {
            PCMDTemplate *ptmp, *succ;

            PROCESSLIST (&pcmddata->pcmd_TemplateList, ptmp, succ, PCMDTemplate) {
                DeleteItem (ptmp->ptmp_Item);
                FreeMem (ptmp, sizeof *ptmp);
            }
        }

            /* free context info */
        FreeContextInfo (ci);
    }

    return 0;
}


/*
    Handle PCMD.PTMP chunk. Loads and adds each contained .dsp template
    to pcmd_TemplateList.

    Arguments
        pcmddata
            pcmd_TemplateList
                Adds each loaded template to this list.

    Results
        IFF_CB_CONTINUE on success, Err code on error.

    @@@ Since this causes templates to be loaded, it might be a bit
        inefficient wrt seek times. At least it stores multiple template
        names, which can help to reduce disc thrashing.
*/
static Err HandlePTMPChunk (IFFParser *iff, PCMDData *pcmddata)
{
    ContextNode * const cn = GetCurrentContext (iff);
    const int32 ptmpBufSize = cn->cn_Size;  /* @@@ truncates 64-bit size */
    char *ptmpBuf;      /* this gets set by call to BufferChunk() before
                           anything else happens, so no need to init to NULL */
    Err errcode;

  #if DEBUG_Load
    IFFPrintf (iff, "HandlePTMPChunk: %d bytes\n", ptmpBufSize);
  #endif

        /* buffer chunk and validate (BufferChunk() traps invalid chunk size) */
    if ((errcode = BufferChunk (&ptmpBuf, iff, ptmpBufSize)) < 0) goto clean;
    if (!IsValidPackedStringArray(ptmpBuf,ptmpBufSize)) {
        errcode = PATCHFILE_ERR_MANGLED;
        goto clean;
    }

        /* LoadInsTemplate() for each template named in chunk. store in pcmp_TemplateList */
        /* @@@ could use LoadScoreTemplate() to support nest-by-reference patches */
    {
        const char *ptmpBufEnd = ptmpBuf + ptmpBufSize;
        const char *ptmpName;

        for (ptmpName = ptmpBuf; ptmpName < ptmpBufEnd; ptmpName = NextPackedString(ptmpName)) {
          #if DEBUG_Load
            printf ("  '%s'", ptmpName);
          #endif
            if ((errcode = AddPCMDTemplate (pcmddata, LoadInsTemplate (ptmpName, NULL))) < 0) goto clean;
        }
    }

        /* success: keep parsing */
    errcode = IFF_CB_CONTINUE;

clean:
    FreeMem (ptmpBuf, ptmpBufSize);
    return errcode;
}

/*
    Handle PCMD.PMIX chunk. Creates and adds each mixer template to
    pcmd_TemplateList.

    Arguments
        pcmddata
            pcmd_TemplateList
                Adds each loaded template to this list.

    Results
        IFF_CB_CONTINUE on success, Err code on error.
*/
static Err HandlePMIXChunk (IFFParser *iff, PCMDData *pcmddata)
{
    ContextNode * const cn = GetCurrentContext (iff);
    const int32 numMixerSpecs = cn->cn_Size / sizeof (MixerSpec);   /* @@@ truncates 64-bit size */
    MixerSpec *pmixBuf; /* this gets set by call to BufferChunk() before
                           anything else happens, so no need to init to NULL */
    Err errcode;

  #if DEBUG_Load
    IFFPrintf (iff, "HandlePMIXChunk: %d mixers\n", numMixerSpecs);
  #endif

        /* buffer chunk */
    if ((errcode = BufferChunk (&pmixBuf, iff, numMixerSpecs * sizeof (MixerSpec))) < 0) goto clean;

        /* CreateMixerTemplate() for each MixerSpec in chunk. store in pcmp_TemplateList */
    {
        int32 i;

        for (i=0; i<numMixerSpecs; i++) {
          #if DEBUG_Load
            printf ("  %d x %d, flags=0x%04x", MixerSpecToNumIn(pmixBuf[i]), MixerSpecToNumOut(pmixBuf[i]), MixerSpecToFlags(pmixBuf[i]));
          #endif
            if ((errcode = AddPCMDTemplate (pcmddata, CreateMixerTemplate (pmixBuf[i], NULL))) < 0) goto clean;
        }
    }

        /* success: keep parsing */
    errcode = IFF_CB_CONTINUE;

clean:
    FreeMem (pmixBuf, numMixerSpecs * sizeof (MixerSpec));
    return errcode;
}

/*
    Exit handler for FORM 3PCH nested inside FORM PCMD. Adds completed Patch
    Template to containing FORM PCMD's pcmd_TemplateList.

    Arguments
        iff
            Extracts stored Patch Template Item for ID_3PCH, ID_PCMD.

        pcmddata
            pcmd_TemplateList
                Adds each loaded template to this list.

    Results
        IFF_CB_CONTINUE on success, Err code on failure.
        Adds patch template to parent PCMDData.pcmd_TemplateList.
*/
static Err ExitNestedForm3PCH (IFFParser *iff, PCMDData *pcmddata)
{
  #if DEBUG_Load
    IFFPrintf (iff, "ExitNestedForm3PCH\n");
  #endif

    return AddPCMDTemplate (pcmddata, ExitForm3PCH (iff, NULL));
}

/*
    Add Template Item to PCMDData. Allocates a new PCMDTemplate structure
    and adds it to pcmp_TemplateList.

    If this fails, it also deletes ptmpItem.

    Arguments
        pcmddata
            pcmd_TemplateList
                Adds PCMDTemplate to this list.

        ptmpItem
            >=0
                Template Item to add to list.

            <0
                Error code to return (e.g., failure code from
                LoadInsTemplate()). No PCMDTemplate is added.

    Results
        IFF_CB_CONTINUE on success, error code on failure.

        pcmddata
            pcmd_TemplateList
                New PCMDTemplate is added to this list on success. Not affected
                on failure.
*/
static Err AddPCMDTemplate (PCMDData *pcmddata, Item ptmpItem)
{
    PCMDTemplate *ptmp;
    Err errcode;

        /* did we get passed an error code? */
    if (ptmpItem < 0) return ptmpItem;

        /* Allocate PCMPTemplate */
        /* (last thing, so no cleanup) */
    if (!(ptmp = AllocMem (sizeof *ptmp, MEMTYPE_NORMAL | MEMTYPE_FILL))) {
        errcode = PATCHFILE_ERR_NOMEM;
        goto clean;
    }
    ptmp->ptmp_Item = ptmpItem;

  #if DEBUG_Load
    printf ("\t0x%x\n", ptmpItem);
  #endif

        /* success: add to list */
    AddTail (&pcmddata->pcmd_TemplateList, (Node *)ptmp);
    return IFF_CB_CONTINUE;

clean:
    DeleteItem (ptmpItem);
    return errcode;
}


/*
    Creates Patch Template from PCMD and PTMP chunks, and pcmd_TemplateList.
    Stores resulting Patch Template Item into the IFF context stack with
    ID_3PCH, ID_PCMD (Locate with FindStoredItem (iff, ID_3PCH, ID_PCMD))

    Arguments
        iff
            Looks for PCMD and PNAM property chunks.

            parent PatchData.pd_PatchName
                Name to give Patch Template Item. Can be NULL, which results
                in an unnamed Item (for nested FORM 3PCHs).

        pcmddata
            pcmd_TemplateList
                List of Template Items to which PCMD chunk refers

    Results
        Returns IFF_CB_CONTINUE on success, Err code on failure.

        Stores ID_3PCH, ID_PCMD Item in property context (parent FORM),
        deleting any previously stored Item with this type and ID in the
        property context.
*/
static Err ExitFormPCMD (IFFParser *iff, PCMDData *pcmddata)
{
    PatchData *pd;
    const char *pnamBuf = NULL;
    int32 pnamSize = 0;
    PatchCmd *pcmdBuf;
    int32 numPatchCmds;
    Item patchTemplate = -1;
    Err errcode;

  #if DEBUG_Load
    IFFPrintf (iff, "ExitFormPCMD");
  #endif

        /* locate parent PatchData */
    {
        const ContextInfo * const ci = FindContextInfo (iff, ID_3PCH, ID_3PCH, ID_3PCH);

        if (!ci) {
            errcode = PATCHFILE_ERR_MANGLED;
            goto clean;
        }
        pd = ci->ci_Data;
    }

        /* find and validate PNAM chunk. Ignores missing or 0-length chunk,
           which is trapped later when indexing into it. */
    {
        const PropChunk * const pc = FindPropChunk (iff, ID_PCMD, ID_PNAM);

        if (pc) {
            pnamBuf  = pc->pc_Data;
            pnamSize = pc->pc_DataSize;
        }
    }
  #if DEBUG_Load
    printf (" PNAM=0x%x, %d bytes", pnamBuf, pnamSize);
  #endif
        /* let 0-length chunk slide, trapped later */
    if (pnamSize && !IsValidPackedStringArray(pnamBuf,pnamSize)) {
        errcode = PATCHFILE_ERR_MANGLED;
        goto clean;
    }

        /* find PCMD chunk (required) */
    {
        const PropChunk * const pc = FindPropChunk (iff, ID_PCMD, ID_PCMD);

        if (!pc) {
            errcode = PATCHFILE_ERR_MANGLED;
            goto clean;
        }
        pcmdBuf = pc->pc_Data;
        numPatchCmds = pc->pc_DataSize / sizeof (PatchCmd);
    }
  #if DEBUG_Load
    printf (" PCMD=0x%x, %d cmds\n", pcmdBuf, numPatchCmds);
  #endif

        /*
            Do in-place fixups on PatchCmd array loaded from file.

            Also do limited validation of PatchCmd array;
                . name and template indeces are in range (part of the
                  fixup process).
                . there are > 0 PatchCmds (counting PATCH_CMD_END).
                . doesn't contain any PATCH_CMD_JUMPs, premature
                  PATCH_CMD_ENDs, or any IDs that we don't know how to fixup.
                . has trailing PATCH_CMD_END
        */
    if (numPatchCmds <= 0) {
        errcode = PATCHFILE_ERR_MANGLED;
        goto clean;
    }
    {
        const PatchCmd * const pcmdBufEnd = pcmdBuf + numPatchCmds;
        PatchCmd *pc;

            /*
                Fixup name:
                    . if (fld) > 0, treat (fld)-1 as index into pnamBuf.
                    . if (fld) == 0, it becomes (and already is) NULL.

                Note: missing or empty PNAM chunk results in pnamBuf==NULL, pnamSize==0
            */
        #define FIXUP_NAME(fld) \
        { \
            if ((uint32)(fld) && (uint32)(fld)-1 >= pnamSize) { \
                ERR(("### ExitFormPCMD: Name key %d out of range of 0..%d\n", (fld), pnamSize)); \
                errcode = PATCHFILE_ERR_MANGLED; \
                goto clean; \
            } \
            if ((uint32)(fld)) (fld) = &pnamBuf [ (uint32)(fld)-1 ]; \
        }

            /*
                Fixup template:
                    . (fld) is index in pcmd_TemplateList. replace with ptmp_Item.
            */
        #define FIXUP_TEMPLATE(fld) \
        { \
            const PCMDTemplate * const ptmp = (PCMDTemplate *)FindNodeFromHead (&pcmddata->pcmd_TemplateList, (fld)); \
            \
            if (!ptmp) { \
                ERR(("### ExitFormPCMD: Template index %d out of range of 0..%d\n", (fld), GetNodeCount(&pcmddata->pcmd_TemplateList)-1)); \
                errcode = PATCHFILE_ERR_MANGLED; \
                goto clean; \
            } \
            (fld) = ptmp->ptmp_Item; \
        }


            /* @@@ Because of validation that we need to do, we can't use NextPatchCmd(). */
        for (pc = pcmdBuf; pc < pcmdBufEnd-1; pc++) {
            switch (pc->pc_Generic.pc_CmdID) {
                case PATCH_CMD_ADD_TEMPLATE:
                    FIXUP_NAME     (pc->pc_AddTemplate.pc_BlockName);
                    FIXUP_TEMPLATE (pc->pc_AddTemplate.pc_InsTemplate);
                    break;

                case PATCH_CMD_DEFINE_PORT:
                    FIXUP_NAME (pc->pc_DefinePort.pc_PortName);
                    break;

                case PATCH_CMD_DEFINE_KNOB:
                    FIXUP_NAME (pc->pc_DefineKnob.pc_KnobName);
                    break;

                case PATCH_CMD_EXPOSE:
                    FIXUP_NAME (pc->pc_Expose.pc_PortName);
                    FIXUP_NAME (pc->pc_Expose.pc_SrcBlockName);
                    FIXUP_NAME (pc->pc_Expose.pc_SrcPortName);
                    break;

                case PATCH_CMD_CONNECT:
                    FIXUP_NAME (pc->pc_Connect.pc_FromBlockName);
                    FIXUP_NAME (pc->pc_Connect.pc_FromPortName);
                    FIXUP_NAME (pc->pc_Connect.pc_ToBlockName);
                    FIXUP_NAME (pc->pc_Connect.pc_ToPortName);
                    break;

                case PATCH_CMD_SET_CONSTANT:
                    FIXUP_NAME (pc->pc_SetConstant.pc_BlockName);
                    FIXUP_NAME (pc->pc_SetConstant.pc_PortName);
                    break;

                case PATCH_CMD_SET_COHERENCE:
                case PATCH_CMD_NOP:
                    break;

                default:
                  #ifdef BUILD_STRINGS
                    DumpPatchCmd (pc, "### ");
                    printf ("### ExitFormPCMD: Invalid PatchCmd\n");
                  #endif
                    errcode = PATCHFILE_ERR_MANGLED;
                    goto clean;
            }

          #if DEBUG_Load
            DumpPatchCmd (pc, NULL);
          #endif
        }

            /* make sure last command is PATCH_CMD_END */
        if (pc->pc_Generic.pc_CmdID != PATCH_CMD_END) {
            errcode = PATCHFILE_ERR_MANGLED;
            goto clean;
        }
      #if DEBUG_Load
        DumpPatchCmd (pc, NULL);
      #endif

        #undef FIXUP_NAME
        #undef FIXUP_TEMPLATE
    }

        /* Create patch template. Name it if pd_PatchName is non-NULL. */
    if ((errcode = patchTemplate = CreatePatchTemplateVA (pcmdBuf,
        pd->pd_PatchName ? TAG_ITEM_NAME : TAG_NOP, pd->pd_PatchName,
    /*  AF_TAG_AUTO_DELETE_SLAVE, TRUE,  cause tuning attached to this template to be deleted. @@@ PTUN not implemented yet. @@@ this tag not implemented for templates yet. */
        TAG_END)) < 0) goto clean;

        /* store in property context (which is conveniently the parent FORM of this one) */
        /* @@@ if anything is added after this which might fail, might want to take care
               not to delete patchTemplate and let it get deleted automatically when the
               stack is popped. */
    if ((errcode = StoreItemInContext (iff, ID_3PCH, ID_PCMD, IFF_CIL_PROP, patchTemplate)) < 0) goto clean;

        /* success: keep parsing */
    return IFF_CB_CONTINUE;

clean:
    DeleteItem (patchTemplate);
    return errcode;
}


/* -------------------- FORM PATT stuff */

static Err ExitFormPATT (IFFParser *, void *dummy);
static Item CreatePATTItem (IFFParser *);
static Err AttachPATT (IFFParser *, Item patchTemplate, Item atagItem);

/*
    Register prop chunks and handlers for FORM PATT.

    Returns
        0 on success, Err code on failure.
*/
static Err RegisterFormPATT (IFFParser *iff)
{
    static const IFFTypeID collectionChunks[] = {
        { ID_PATT, ID_PATT },
        0
    };
    static const IFFTypeID propChunks[] = {
        { ID_PATT, ID_ATAG },
        0
    };
    static const IFFTypeID dataChunks[] = {
        { ID_PATT, ID_BODY },
        0
    };
    Err errcode;

    if ((errcode = RegisterCollectionChunks (iff, collectionChunks)) < 0) goto clean;
    if ((errcode = RegisterPropChunks (iff, propChunks)) < 0) goto clean;
    if ((errcode = RegisterDataChunks (iff, dataChunks)) < 0) goto clean;
    if ((errcode = InstallExitHandler (iff, ID_PATT, ID_FORM, IFF_CIL_TOP, (IFFCallBack)ExitFormPATT, NULL)) < 0) goto clean;

    return 0;

clean:
    return errcode;
}

/*
    Creates Item from ATAG and creates Attachments to patch template (item stored
    under ID_3PCH, ID_PCMD).

    Finds prop, collection, and data chunks for FORM PATT:
        ID_PATT (collection) - attachment descriptions for this item
        ID_ATAG (prop) - AudioTagHeader
        ID_BODY (data) - data (optional)

    Arguments
        iff

    Returns
        IFF_CB_CONTINUE on success, Err code on failure.

    Caveats
        A FORM PATT w/o an PATT chunks is not very useful. This is trapped with
        a suitable error message for BUILD_PARANOIA mode. When not in
        BUILD_PARANOIA mode, an unattached slave item remains allocated (i.e.,
        it is lost to the application). The moral: don't create patches with
        unattached slave items.
*/
static Err ExitFormPATT (IFFParser *iff, void *dummy)
{
    Item patchTemplate;
    Item atagItem = -1;
    Err errcode;

    TOUCH(dummy);

        /* find patch template to attach to */
    if ((errcode = patchTemplate = FindStoredItem (iff, ID_3PCH, ID_PCMD)) <= 0) {
        if (!errcode) errcode = PATCHFILE_ERR_MANGLED;
        goto clean;
    }

  #if DEBUG_Load
    IFFPrintf (iff, "ExitFormPATT: patch=0x%x ", patchTemplate);
  #endif

        /* create Item from ATAG and BODY chunks */
    if ((errcode = atagItem = CreatePATTItem (iff)) < 0) goto clean;

  #if DEBUG_Load
    printf ("slave=0x%x\n", atagItem);
  #endif

        /* create attachments */
    if ((errcode = AttachPATT (iff, patchTemplate, atagItem)) < 0) goto clean;

        /* success */
    return IFF_CB_CONTINUE;

clean:
        /* delete atag item. mostly a safety net in case no attachments
           (BUILD_PARANOIA case) or first attachment fails */
    DeleteItem (atagItem);
    return errcode;
}

/*
    Create slave Item from ATAG and BODY chunks.

    Results
        Returns slave Item number, Err code on failure.
*/
static Item CreatePATTItem (IFFParser *iff)
{
        /* extract BODY data, if present */
    void * const body = ExtractDataChunk (iff, ID_PATT, ID_BODY);
    const AudioTagHeader *atag;
    Item atagItem;
    Err errcode;

        /* find and validate AudioTagHeader */
    {
        const PropChunk * const atagProp = FindPropChunk (iff, ID_PATT, ID_ATAG);

        if (!atagProp) {
            errcode = PATCHFILE_ERR_MANGLED;
            goto clean;
        }
        atag = atagProp->pc_Data;

      #ifdef BUILD_PARANOIA
        if ((errcode = ValidateAudioTagHeader (atag, atagProp->pc_DataSize)) < 0) goto clean;
      #endif
    }

        /* create item */
    if ((errcode = atagItem = CreateItemVA ( MKNODEID (AUDIONODE, atag->athd_NodeType),
        body ? AF_TAG_ADDRESS : TAG_NOP,        body,
        body ? AF_TAG_AUTO_FREE_DATA : TAG_NOP, TRUE,
        TAG_JUMP,                               atag->athd_Tags )) < 0) goto clean;

        /* success */
    return atagItem;

clean:
        /* delete atagItem if created, which also frees body; free body if atagItem not yet created */
#if 0   /* @@@ atagItem is last thing created; don't need all this at the moment */
    if (atagItem > 0)
        DeleteItem (atagItem);
    else
#endif
        FreeMem (body, TRACKED_SIZE);

    return errcode;
}

/*
    Create attachments to PATT's Item

    Arguments
        iff
            Context containing PATT.PATT collection chunks.

        patchTemplate
            Template Item to attach slave to.

        atagItem
            Slave Item to attach.

    Results
        0 on success, Err code on failure.

    Caveats
        Creates attachments in reverse order in which they are found in FORM PATT.
        Fortunately, this only matters in the unlikely event that there are two
        attachments from this FORM PATT to the same hook.
*/
static Err AttachPATT (IFFParser *iff, Item patchTemplate, Item atagItem)
{
    const CollectionChunk *pattColl = FindCollection (iff, ID_PATT, ID_PATT);
    Err errcode;

  #if DEBUG_Load
    IFFPrintf (iff, "ExitFormPATT\n");
  #endif

    if (pattColl) {
            /* create each attachment (one per Collection) */
        for (; pattColl; pattColl = pattColl->cc_Next) {
            const PatchAttachment *patt = pattColl->cc_Data;

          #if DEBUG_Load
            printf ("  hook='%s' %d tags\n", patt->patt_HookName, patt->patt_NumTags);
          #endif

          #ifdef BUILD_PARANOIA
                /* make sure chunk is at least large enough to hold tags */
            if ( pattColl->cc_DataSize < PatchAttachmentSize (1) ||
                 pattColl->cc_DataSize < PatchAttachmentSize (patt->patt_NumTags) ) {

                ERR(("### ExitFormPATT: Short PATT chunk (%d bytes)\n", (int32)pattColl->cc_DataSize));
                errcode = PATCHFILE_ERR_MANGLED;
                goto clean;
            }

                /* validate tags */
            {
                static const uint32 invalidTagIDs[] = {
                    AF_TAG_NAME,                /* contains pointer */
                    AF_TAG_MASTER,              /* conflicts with loader */
                    AF_TAG_SLAVE,               /* conflicts with loader */
                    AF_TAG_AUTO_DELETE_SLAVE,   /* conflicts with loader */
                    TAG_END
                };

                if ((errcode = ValidateFileConditionedTags (patt->patt_Tags, patt->patt_NumTags, invalidTagIDs)) < 0) goto clean;
            }
          #endif

                /* create item */
            if ((errcode = CreateAttachmentVA (patchTemplate, atagItem,
                AF_TAG_AUTO_DELETE_SLAVE, TRUE,
                AF_TAG_NAME, patt->patt_HookName,
                TAG_JUMP,    patt->patt_Tags)) < 0) goto clean;
        }
    }
  #ifdef BUILD_PARANOIA
        /* If no attachments, the slave item is lost and won't get deleted.
           Trap this case if BUILD_PARANOIA is on */
    else {
      #ifdef BUILD_STRINGS
        {
            const ItemNode * const n = LookupItem (atagItem);

            printf ("### ExitFormPATT: Attached Item 0x%x (type %d) has no attachments.\n", atagItem, n->n_Type);
        }
      #endif
        errcode = PATCHFILE_ERR_MANGLED;
        goto clean;
    }
  #endif

    return 0;

clean:
    return errcode;
}


#if 0   /* @@@ PTUN not implemented yet */
/* -------------------- FORM PTUN stuff */

static Err EnterFormPTUN (IFFParser *, void *);

/* @@@ prelim */
static Err RegisterFormPTUN (IFFParser *iff)
{
    Err errcode;

        /* store dummy tuning to avoid picking one up from lower on the context stack */
    if ((errcode = StoreItemInContext (iff, ID_3PCH, ID_PTUN, IFF_CIL_TOP, 0)) < 0) goto clean;

        /* register entry handler */
    if ((errcode = InstallEntryHandler (iff, ID_PTUN, ID_FORM, IFF_CIL_TOP, (IFFCallBack)EnterFormPTUN, NULL)) < 0) goto clean;

    return 0;

clean:
    return errcode;
}

/* @@@ prelim */
static Err EnterFormPTUN (IFFParser *iff, void *dummy)
{
    TOUCH(dummy);

  #if DEBUG_Load
    IFFPrintf (iff, "EnterFormPTUN\n");
  #endif

    return IFF_CB_CONTINUE;
}
#endif  /* @@@ PTUN not implemented yet */


/* -------------------- Handy goodies */

/*
    Allocate requested buffer and fill it from current chunk. If fewer than
    the requested number of bytes are read, returns an error.

    Arguments
        resultBuf
            Pointer to location to store allocated buffer pointer.
            Set to NULL on failure.

        iff
            IFF handle to read current chunk from

        bufSize
            Number of bytes to allocate and read. Fails if <= 0.

    Results
        0 on success, Err code on failure.
*/
static Err BufferChunk (void **resultBuf, IFFParser *iff, int32 bufSize)
{
    void *buf = NULL;
    Err errcode;

        /* init result */
    *resultBuf = NULL;

        /* make sure size is legal */
    if (bufSize <= 0) {
        errcode = PATCHFILE_ERR_MANGLED;
        goto clean;
    }

        /* alloc buffer and read chunk contents */
    if (!(buf = AllocMem (bufSize, MEMTYPE_NORMAL))) {
        errcode = PATCHFILE_ERR_NOMEM;
        goto clean;
    }
    {
        const int32 result = ReadChunk (iff, buf, bufSize);

        if (result != bufSize) {
            errcode = result < 0 ? result : PATCHFILE_ERR_MANGLED;
            goto clean;
        }
    }

        /* success: return buffer pointer */
    *resultBuf = buf;
    return 0;

clean:
    FreeMem (buf, bufSize);
    return errcode;
}
