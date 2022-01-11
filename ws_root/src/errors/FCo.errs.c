/* @(#) FCo.errs.c 95/09/13 1.2 */

#include <kernel/types.h>
#include <kernel/operror.h>


static const char * const ErrorMsgs[] =
{
                               "no error",
    /* COMP_ERR_DATAREMAINS */ "There is more source data than the decompressor expects",
    /* COMP_ERR_DATAMISSING */ "There is not enough data for the decompressor",
    /* COMP_ERR_OVERFLOW    */ "Result buffer is too small for output data"
};

static const TagArg ErrorTags[] =
{
    TAG_ITEM_NAME,	(void *)"Compression Folio",
    ERRTEXT_TAG_OBJID,	(void *)((ER_FOLI<<ERR_IDSIZE)|(ER_COMP)),
    ERRTEXT_TAG_MAXERR,	(void *)(sizeof(ErrorMsgs)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,	(void *)ErrorMsgs,
    TAG_END,		0
};

const TagArg *main (void)
{
    return ErrorTags;
}
