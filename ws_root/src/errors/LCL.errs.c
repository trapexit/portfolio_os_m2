/* @(#) LCL.errs.c 96/06/10 1.2 */

#include <errno.h>

static const char * const ErrorMsg[] =
{
    /* no error                      */ "No error",
    /* C_ERR_NOTSUPPORTED            */ "Not supported on descriptor's medium",
    /* C_ERR_SYSTEMDESCRIPT          */ "Descriptor owned by system"
};

static const TagArg ErrorTags[] = {
    { TAG_ITEM_NAME,      (TagData)"C Library" },
    { ERRTEXT_TAG_OBJID,  (TagData)(ER_LINKLIB << ERR_IDSIZE | ER_CLIB) },
    { ERRTEXT_TAG_MAXERR, (TagData)(sizeof ErrorMsg / sizeof ErrorMsg[0]) },
    { ERRTEXT_TAG_TABLE,  (TagData)ErrorMsg },
    { TAG_END }
};

const TagArg *main (void)
{
    return ErrorTags;
}
