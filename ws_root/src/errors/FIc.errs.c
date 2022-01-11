/* @(#) FIc.errs.c 96/02/27 1.6 */

#include <kernel/types.h>
#include <kernel/operror.h>


static const char * const ErrorMsgs[] =
{
                                  "no error",
    /* ICON_ERR_INCOMPLETE    */  "Incomplete icon located",
    /* ICON_ERR_MUTUALLYEXCLUSIVE */ \
                                  "Mutually-exclusive parameters",
    /* ICON_ERR_ARGUMENTS     */  "Incomplete parameter arguments",
    /* ICON_ERR_UNKNOWNFORMAT */  "Encountered unknown format",
    /* ICON_ERR_OVERSIZEDICON */  "Icon is too large"
};

static const TagArg ErrorTags[] =
{
    TAG_ITEM_NAME,  (void *)"Icon Folio",
    ERRTEXT_TAG_OBJID,  (void *)((ER_FOLI<<ERR_IDSIZE)|(ER_ICON)),
    ERRTEXT_TAG_MAXERR, (void *)(sizeof(ErrorMsgs)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,  (void *)ErrorMsgs,
    TAG_END,        0
};

const TagArg *main(void)
{
    return ErrorTags;
}
