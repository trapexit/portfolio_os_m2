
/******************************************************************************
**
**  @(#) FBp.errs.c 96/06/10 1.5
**
**  Beep Folio error text
**
******************************************************************************/

#include <beep/beep.h>

static const char * const ErrorMsg[] =
{    /* @@@ must remain in sync w/ BEEP_ERR_ codes in audio/beep.h */
	"no error",
	"illegal security violation", /* BEEP_ERR_SECURITY */
	"invalid audio signal type",  /* BEEP_ERR_BAD_SIGNAL_TYPE */
	"invalid parameter",  /* BEEP_ERR_INVALID_PARAM */
	"invalid channel configuration",  /* BEEP_ERR_INVALID_CONFIGURATION */
	"no machine loaded",  /* BEEP_ERR_NO_MACHINE */
	"task has not opened Beep folio",  /* BEEP_ERR_FOLIO_NOT_OPEN */
	"channelNum out of range",  /* BEEP_ERR_CHANNEL_RANGE */
	"voiceNum out of range",  /* BEEP_ERR_VOICE_RANGE */
	"can't open file",  /* BEEP_ERR_OPEN_FILE */
	"badly formatted file",  /* BEEP_ERR_BAD_FILE */
	"file could not be read",  /* BEEP_ERR_READ_FAILED */
	"beep machine is already loaded",  /* BEEP_ERR_ALREADY_LOADED */
	"flag parameter is illegal for this routine",  /* BEEP_ERR_ILLEGAL_FLAG */
	"name passed to Beep folio is too long",  /* BEEP_ERR_NAME_TOO_LONG */
	"illegal Beep Machine file information",  /* BEEP_ERR_ILLEGAL_MACHINE */
	"DSP in use by another folio"  /* BEEP_ERR_DSP_BUSY */
};

static const TagArg ErrorTags[] = {
    { TAG_ITEM_NAME,      (TagData)"Beep Folio" },
    { ERRTEXT_TAG_OBJID,  (TagData)(ER_FOLI << ERR_IDSIZE | ER_BEEP) },
    { ERRTEXT_TAG_MAXERR, (TagData)(sizeof ErrorMsg / sizeof ErrorMsg[0]) },
    { ERRTEXT_TAG_TABLE,  (TagData)ErrorMsg },
    { TAG_END }
};

const TagArg *main (void)
{
    return ErrorTags;
}
