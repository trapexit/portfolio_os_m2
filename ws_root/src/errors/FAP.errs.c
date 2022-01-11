
/******************************************************************************
**
**  @(#) FAP.errs.c 96/07/17 1.3
**
**  AudioPatch Folio error text
**
******************************************************************************/

#include <audio/patch.h>

static const char * const ErrorMsg[] = {    /* @@@ must remain in sync w/ PATCH_ERR_ codes in audio/patch.h */
                                        "no error",
    /* PATCH_ERR_BAD_NAME            */ "Invalid name",
    /* PATCH_ERR_BAD_PATCH_CMD       */ "Invalid PatchCmd",
    /* PATCH_ERR_BAD_PORT_TYPE       */ "Invalid port type",
    /* PATCH_ERR_BAD_SIGNAL_TYPE     */ "Invalid signal type",
    /* PATCH_ERR_INCOMPATIBLE_VERSION*/ "Version mismatch between AudioPatch Folio and Audio Folio",
    /* PATCH_ERR_NAME_NOT_FOUND      */ "Named thing not found",
    /* PATCH_ERR_NAME_NOT_UNIQUE     */ "Name is not unique",
    /* PATCH_ERR_NAME_TOO_LONG       */ "Name too long",
    /* PATCH_ERR_OUT_OF_RANGE        */ "Value out of range",
    /* PATCH_ERR_PORT_ALREADY_EXPOSED*/ "Port already exposed",
    /* PATCH_ERR_PORT_IN_USE         */ "Port already connected",
    /* PATCH_ERR_PORT_NOT_USED       */ "Port not used",
    /* PATCH_ERR_TOO_MANY_RESOURCES  */ "Patch requires too many resources",
};

static const TagArg ErrorTags[] = {
    { TAG_ITEM_NAME,      (TagData)"AudioPatch Folio" },
    { ERRTEXT_TAG_OBJID,  (TagData)(ER_FOLI << ERR_IDSIZE | ER_AUDIOPATCH) },
    { ERRTEXT_TAG_MAXERR, (TagData)(sizeof ErrorMsg / sizeof ErrorMsg[0]) },
    { ERRTEXT_TAG_TABLE,  (TagData)ErrorMsg },
    { TAG_END }
};

const TagArg *main (void)
{
    return ErrorTags;
}
