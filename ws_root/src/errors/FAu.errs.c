
/******************************************************************************
**
**  @(#) FAu.errs.c 96/06/19 1.35
**
**  Audio Folio error text
**
******************************************************************************/

#include <audio/audio.h>

static const char undefinedNoError[] = "undefined error";

static const char * const ErrorMsg[] = {    /* @@@ must remain in sync w/ AF_ERR_ codes in audio/audio.h */
                                        "no error",
    /* @@@ undefined                 */ undefinedNoError,
    /* AF_ERR_DSP_BUSY               */ "DSP in use by another folio",
    /* AF_ERR_MISMATCH               */ "Mismatched Items or Types",
    /* AF_ERR_PATCHES_ONLY           */ "Template only for Patches",
    /* AF_ERR_NAME_NOT_FOUND         */ "Named thing not found",
    /* AF_ERR_NOINSTRUMENT           */ "Item has no instrument",
    /* @@@ undefined                 */ undefinedNoError,
    /* @@@ undefined                 */ undefinedNoError,
    /* @@@ undefined                 */ undefinedNoError,
    /* AF_ERR_EXTERNALREF            */ "External reference not satisfied",
    /* AF_ERR_BADRSRCTYPE            */ "Illegal DSP resource type",
    /* @@@ undefined                 */ undefinedNoError,
    /* AF_ERR_RELOCATION             */ "DSP code relocation error",
    /* AF_ERR_RSRCATTR               */ "Illegal DSP resource attribute",
    /* AF_ERR_INUSE                  */ "Thing is in use",
    /* @@@ undefined                 */ undefinedNoError,
    /* @@@ undefined                 */ undefinedNoError,
    /* AF_ERR_BADOFX                 */ "Invalid DSP instrument file",
    /* AF_ERR_NORSRC                 */ "Insufficient DSP resource",
    /* AF_ERR_BADFILETYPE            */ "Invalid file type",
    /* @@@ undefined                 */ undefinedNoError,
    /* AF_ERR_BAD_NAME               */ "Invalid name",
    /* AF_ERR_OUTOFRANGE             */ "Value out of range",
    /* AF_ERR_NAME_TOO_LONG          */ "Name too long",
    /* AF_ERR_NULLADDRESS            */ "NULL address",
    /* @@@ undefined                 */ undefinedNoError,
    /* AF_ERR_SECURITY               */ "Security violation",
    /* AF_ERR_BAD_SIGNAL_TYPE        */ "Invalid signal type",
    /* AF_ERR_AUDIOCLOSED            */ "OpenAudioFolio() not called by thread",
    /* AF_ERR_SPECIAL                */ "Internal DSP instrument error",
    /* AF_ERR_TAGCONFLICT            */ "Contradictory tags",
    /* @@@ undefined                 */ undefinedNoError,
    /* @@@ undefined                 */ undefinedNoError,
    /* AF_ERR_BAD_RSRC_BINDING       */ "Invalid DSP resource binding (bad template)",
    /* AF_ERR_BAD_MIXER              */ "Invalid mixer specification",
    /* AF_ERR_NAME_NOT_UNIQUE        */ "Name not unique",
    /* AF_ERR_BAD_PORT_TYPE          */ "Invalid port type",
    /* @@@ undefined                 */ undefinedNoError,
    /* @@@ undefined                 */ undefinedNoError,
    /* @@@ undefined                 */ undefinedNoError,
    /* @@@ undefined                 */ undefinedNoError,
    /* AF_ERR_INTERNAL_NOT_SHAREABLE */ "Template not shareable",
};

static const TagArg ErrorTags[] = {
    { TAG_ITEM_NAME,      (TagData)"Audio Folio" },
    { ERRTEXT_TAG_OBJID,  (TagData)(ER_FOLI << ERR_IDSIZE | ER_ADIO) },
    { ERRTEXT_TAG_MAXERR, (TagData)(sizeof ErrorMsg / sizeof ErrorMsg[0]) },
    { ERRTEXT_TAG_TABLE,  (TagData)ErrorMsg },
    { TAG_END }
};

const TagArg *main (void)
{
    return ErrorTags;
}
