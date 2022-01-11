/* @(#) FIF.errs.c 95/09/13 1.2 */

#include <kernel/types.h>
#include <kernel/operror.h>


static const char * const ErrorMsgs[] =
{
                                "no error",
    /* IFF_ERR_NOSCOPE       */ "No valid scope for property",
    /* IFF_ERR_PREMATURE_EOF */ "Hit the end of file too soon",
    /* IFF_ERR_MANGLED       */ "Data in file is corrupt",
    /* IFF_ERR_SYNTAX        */ "IFF syntax error",
    /* IFF_ERR_NOTIFF        */ "Not an IFF file",
    /* IFF_ERR_NOOPENTYPE    */ "No open type specified",
    /* IFF_ERR_BADLOCATION   */ "Bad storage location given",
    /* IFF_ERR_TOOBIG        */ "Too much data for a 32-bit size field",
    /* IFF_ERR_BADMODE       */ "Bad mode parameter given",
    /* IFF_PARSE_EOF         */ "Reached logical end of file",
    /* IFF_PARSE_EOC         */ "About to leave context"
};

static const TagArg ErrorTags[] =
{
    TAG_ITEM_NAME,      (void *)"IFF Folio",
    ERRTEXT_TAG_OBJID,  (void *)((ER_FOLI << ERR_IDSIZE) | ER_IFF),
    ERRTEXT_TAG_MAXERR, (void *)(sizeof(ErrorMsgs) / sizeof(char *)),
    ERRTEXT_TAG_TABLE,  (void *)ErrorMsgs,
    TAG_END,            0
};

const TagArg *main(void)
{
    return ErrorTags;
}
