/* @(#) LLd.errs.c 96/05/31 1.5 */

#include <kernel/tags.h>
#include <kernel/operror.h>


static const char * const ErrorMsg[] =
{
                                "No error",
    /* LOADER_ERR_BADFILE    */ "Bad Object File",
    /* LOADER_ERR_RELOC      */ "Bad relocation entry in object file",
    /* LOADER_ERR_SYM        */ "Bad symbol reference in object file",
    /* LOADER_ERR_RSA        */ "RSA failure",
    /* LOADER_ERR_BADSECTION */ "Bad section in object file",
    /* LOADER_ERR_NOIMPORT   */ "No import found",
    /* LOADER_ERR_IMPRANGE   */ "Import out of range",
};

static const TagArg ErrorTags[] =
{
    TAG_ITEM_NAME,      (void *)"Loader",
    ERRTEXT_TAG_OBJID,  (void *)(ER_LINKLIB << ERR_IDSIZE | ER_LOADER),
    ERRTEXT_TAG_MAXERR, (void *)(sizeof ErrorMsg / sizeof ErrorMsg[0]),
    ERRTEXT_TAG_TABLE,  (void *)ErrorMsg,
    TAG_END,            0
};

const TagArg *main(void)
{
    return ErrorTags;
}
