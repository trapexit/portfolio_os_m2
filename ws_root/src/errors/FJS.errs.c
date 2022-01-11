/* @(#) FJS.errs.c 95/09/13 1.2 */

#include <kernel/types.h>
#include <kernel/operror.h>


static const char * const ErrorMsgs[] =
{
                                  "no error",
    /* JSTR_ERR_BUFFERTOOSMALL */ "The result buffer was not large enough to hold the output"
};

static const TagArg ErrorTags[] =
{
    TAG_ITEM_NAME,	(void *)"JString Folio",
    ERRTEXT_TAG_OBJID,	(void *)((ER_FOLI<<ERR_IDSIZE)|(ER_JSTR)),
    ERRTEXT_TAG_MAXERR,	(void *)(sizeof(ErrorMsgs)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,	(void *)ErrorMsgs,
    TAG_END,		0
};

const TagArg *main(void)
{
    return ErrorTags;
}
