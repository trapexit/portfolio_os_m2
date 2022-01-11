/* @(#) FRQ.errs.c 96/02/02 1.1 */

#include <kernel/types.h>
#include <kernel/operror.h>


static const char * const ErrorMsgs[] =
{
                                "no error",
    /* REQ_ERR_BADREQ       */  "Invalid or corrupt requester pointer supplied to a function",
    /* REQ_ERR_BADTYPE      */  "Invalid requester type",
    /* REQ_ERR_CORRUPTFILE  */  "Ancilliary data file corrupt",
    /* REQ_ERR_BADOPTION    */  "Bad option supplied",
    /* REQ_ERR_PROMPTTOOBIG */  "Supplied prompt sprite is too large"
};

static const TagArg ErrorTags[] =
{
    TAG_ITEM_NAME,      (void *)"Requester Folio",
    ERRTEXT_TAG_OBJID,  (void *)((ER_FOLI << ERR_IDSIZE) | ER_REQ),
    ERRTEXT_TAG_MAXERR, (void *)(sizeof(ErrorMsgs) / sizeof(char *)),
    ERRTEXT_TAG_TABLE,  (void *)ErrorMsgs,
    TAG_END,            0
};

const TagArg *main(void)
{
    return ErrorTags;
}
