/* @(#) DSe.errs.c 95/12/11 1.1 */

#include <kernel/types.h>
#include <kernel/operror.h>


static const char * const ErrorMsgs[] =
{
                                "no error",
    /* SER_ERR_BADBAUDRATE   */ "Bad baud rate specified",
    /* SER_ERR_BADHANDSHAKE  */ "Bad handshaking mode specified",
    /* SER_ERR_BADWORDLENGTH */ "Bad word length specified",
    /* SER_ERR_BADPARITY     */ "Bad parity specified",
    /* SER_ERR_BADSTOPBITS   */ "Bad stop bits specified",
    /* SER_ERR_BADEVENT      */ "Bad event type specified"
};

static const TagArg ErrorTags[] =
{
    TAG_ITEM_NAME,	(void *)"Serial Device",
    ERRTEXT_TAG_OBJID,	(void *)((ER_DEVC<<ERR_IDSIZE)|(ER_SER)),
    ERRTEXT_TAG_MAXERR,	(void *)(sizeof(ErrorMsgs)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,	(void *)ErrorMsgs,
    TAG_END,		0
};

const TagArg *main(void)
{
    return ErrorTags;
}
