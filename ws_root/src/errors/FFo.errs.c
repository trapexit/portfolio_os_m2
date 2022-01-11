/* @(#) FFo.errs.c 96/06/21 1.5 */

#include <kernel/types.h>
#include <kernel/operror.h>


static const char * const ErrorMsgs[] =
{
                               "no error",
    /* FONT_ERR_BADFONTFILE */ "The named file wasn't a valid font file",
    /* FONT_ERR_BADVERSION  */ "The font file isn't of a supported version",
    /* FONT_ERR_BADBPP      */ "The font has an unsupported bits-per-pixel value",
    /* FONT_ERR_BADTS        */ "Bad TextState",
    /* FONT_ERR_BADCHAR      */ "Character is out of range",
    /* FONT_ERR_BADSTENCILRANGE */ "Bad range of characters for a TextStencil",
};

static const TagArg ErrorTags[] =
{
    TAG_ITEM_NAME,	(void *)"Font Folio",
    ERRTEXT_TAG_OBJID,	(void *)((ER_FOLI<<ERR_IDSIZE)|(ER_FONT)),
    ERRTEXT_TAG_MAXERR,	(void *)(sizeof(ErrorMsgs)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,	(void *)ErrorMsgs,
    TAG_END,		0
};

const TagArg *main(void)
{
    return ErrorTags;
}
