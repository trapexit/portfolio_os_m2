/* @(#) FF2.errs.c 96/06/10 1.2 */

#include <kernel/types.h>
#include <kernel/operror.h>


static const char * const ErrorMsgs[] =
{
                                       "no error",
    /* LT_ERR_INCOMPLETEPARAMETERS */  "Incomplete parameter arguments",
    /* LT_ERR_MUTUALLYEXCLUSIVE    */  "Mutually-exclusive parameters",
    /* LT_ERR_UNKNOWNIFFFILE       */  "Incomplete parameter arguments",
    /* LT_ERR_FAULTYTEXTURE        */  "Faulty texture data"
};

static const TagArg ErrorTags[] =
{
    TAG_ITEM_NAME,  	(void *)"Frame2d Folio",
    ERRTEXT_TAG_OBJID,  (void *)((ER_FOLI<<ERR_IDSIZE)|(ER_FRAME2D)),
    ERRTEXT_TAG_MAXERR, (void *)(sizeof(ErrorMsgs)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,  (void *)ErrorMsgs,
    TAG_END,                    0
};

const TagArg *main(void)
{
    return ErrorTags;
}


