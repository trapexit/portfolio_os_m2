/* @(#) TEB.errs.c 96/06/27 1.1 */

#include <kernel/tags.h>
#include <kernel/operror.h>


static const char * const ErrorMsg[] =
{
    "No error",
};

static const TagArg ErrorTags[] =
{
    TAG_ITEM_NAME,      (void *)"EventBroker",
    ERRTEXT_TAG_OBJID,  (void *)(ER_TASK << ERR_IDSIZE | ER_EVBR),
    ERRTEXT_TAG_MAXERR, (void *)(sizeof ErrorMsg / sizeof ErrorMsg[0]),
    ERRTEXT_TAG_TABLE,  (void *)ErrorMsg,
    TAG_END,            0
};

const TagArg *main(void)
{
    return ErrorTags;
}
