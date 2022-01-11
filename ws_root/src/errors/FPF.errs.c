/******************************************************************************
**
**  @(#) FPF.errs.c 96/03/01 1.1
**
**  AudioPatchFile Folio error text
**
******************************************************************************/

#include <audio/patchfile.h>

static const char * const ErrorMsg[] = {    /* @@@ must remain in sync w/ PATCHFILE_ERR_ codes in audio/patchfile.h */
                                        "no error",
    /* PATCHFILE_ERR_BAD_TYPE        */ "Invalid file type",
    /* PATCHFILE_ERR_MANGLED         */ "Data in file is corrupt",
};

static const TagArg ErrorTags[] = {
    { TAG_ITEM_NAME,      (TagData)"AudioPatchFile Folio" },
    { ERRTEXT_TAG_OBJID,  (TagData)(ER_FOLI << ERR_IDSIZE | ER_AUDIOPATCHFILE) },
    { ERRTEXT_TAG_MAXERR, (TagData)(sizeof ErrorMsg / sizeof ErrorMsg[0]) },
    { ERRTEXT_TAG_TABLE,  (TagData)ErrorMsg },
    { TAG_END }
};

const TagArg *main (void)
{
    return ErrorTags;
}
