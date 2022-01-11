/* @(#) FSG.errs.c 96/02/29 1.1 */

#include <kernel/types.h>
#include <kernel/operror.h>


static const char * const ErrorMsgs[] =
{
                                  "no error",
	/* SG_ERR_MUTUALLYEXCLUSIVE   */ "Mutually-exclusive parameters",
	/* SG_ERR_UNKNOWNFORMAT       */ "Unknown file format",
	/* SG_ERR_INCOMPLETE		  */ "Incomplete parameter arguments",
	/* SG_ERR_OVERFLOW			  */ "More data than buffers will hold"
};

static const TagArg ErrorTags[] =
{
    TAG_ITEM_NAME,  (void *)"SaveGame Folio",
    ERRTEXT_TAG_OBJID,  (void *)((ER_FOLI<<ERR_IDSIZE)|(ER_SAVEGAME)),
    ERRTEXT_TAG_MAXERR, (void *)(sizeof(ErrorMsgs)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,  (void *)ErrorMsgs,
    TAG_END,        0
};

const TagArg *main(void)
{
    return ErrorTags;
}

