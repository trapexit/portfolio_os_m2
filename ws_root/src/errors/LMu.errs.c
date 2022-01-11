
/******************************************************************************
**
**  @(#) LMu.errs.c 96/06/10 1.15
**
**  Music library error text
**
******************************************************************************/

#include <audio/musicerror.h>

static const char * const ErrorMsg[] = {    /* @@@ must remain in sync w/ ML_ERR_ codes in audio/musicerrors.h */
    /* no error                      */ "No error",
    /* ML_ERR_NOT_IFF_FORM           */ "Illegal IFF format",
    /* ML_ERR_BAD_FILE_NAME          */ "Could not open file",
    /* ML_ERR_NOT_OPEN               */ "Stream or file not open",
    /* ML_ERR_BAD_SEEK               */ "Bad seek mode or offset",
    /* ML_ERR_END_OF_FILE            */ "Unexpected end of file",
    /* ML_ERR_UNSUPPORTED_SAMPLE     */ "Sample format has no instrument to play it",
    /* ML_ERR_BAD_FORMAT             */ "File format is incorrect",
    /* ML_ERR_BAD_USERDATA           */ "UserData in MIDI Parser bad",
    /* ML_ERR_OUT_OF_RANGE           */ "Input parameter out of range",
    /* ML_ERR_NO_TEMPLATE            */ "No Template mapped to that Program number",
    /* ML_ERR_NO_NOTES               */ "No voices could be allocated",
    /* ML_ERR_BAD_ARG                */ "Invalid or conflicting argument(s)",
    /* ML_ERR_BAD_SAMPLE_ALIGNMENT   */ "Sample not properly DMA aligned",
    /* ML_ERR_BAD_SAMPLE_ADDRESS     */ "Invalid sample address or length",
    /* ML_ERR_INCOMPATIBLE_SOUND     */ "Sound is incompatible with player",
    /* ML_ERR_CORRUPT_DATA           */ "Some data has become corrupt",
    /* ML_ERR_INVALID_MARKER         */ "Inappropriate marker for operation",
    /* ML_ERR_INVALID_FILE_TYPE      */ "Inappropriate file type",
    /* ML_ERR_DUPLICATE_NAME         */ "Named thing already exists",
    /* ML_ERR_NAME_NOT_FOUND         */ "Named thing not found",
    /* ML_ERR_BUFFER_TOO_SMALL       */ "A (sound) buffer is too small",
    /* ML_ERR_LINE_TOO_LONG          */ "Line too long",
    /* ML_ERR_BAD_COMMAND            */ "Unknown command",
    /* ML_ERR_MISSING_ARG            */ "Missing required argument(s)",
    /* ML_ERR_MANGLED_FILE           */ "Data in file is corrupt",
    /* ML_ERR_BAD_KEYWORD            */ "Unknown keyword",
    /* ML_ERR_TOO_MANY_ARGS          */ "Too many arguments",
    /* ML_ERR_BAD_NUMBER             */ "Bad number",
    /* ML_ERR_HEAD_EXPLOSION         */ "Interaural delay > ear-to-ear distance",
};

static const TagArg ErrorTags[] = {
    { TAG_ITEM_NAME,      (TagData)"Music Library" },
    { ERRTEXT_TAG_OBJID,  (TagData)(ER_LINKLIB << ERR_IDSIZE | ER_MUSICLIB) },
    { ERRTEXT_TAG_MAXERR, (TagData)(sizeof ErrorMsg / sizeof ErrorMsg[0]) },
    { ERRTEXT_TAG_TABLE,  (TagData)ErrorMsg },
    { TAG_END }
};

const TagArg *main (void)
{
    return ErrorTags;
}
