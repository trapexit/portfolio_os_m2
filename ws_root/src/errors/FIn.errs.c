/* @(#) FIn.errs.c 95/09/13 1.2 */

#include <kernel/types.h>
#include <kernel/operror.h>


static const char * const ErrorMsgs[] =
{
                                    "no error",
    /* INTL_ERR_BADRESULTBUFFER  */ "The supplied result buffer pointer is invalid",
    /* INTL_ERR_BUFFERTOOSMALL   */ "The result buffer was not large enough to hold the output",
    /* INTL_ERR_BADNUMERICSPEC   */ "The supplied NumericSpec structure pointer is invalid",
    /* INTL_ERR_FRACTOOLARGE     */ "The frac parameter is greater than 999999999",
    /* INTL_ERR_IMPOSSIBLEDATE   */ "The supplied GregorianDate structure contained an impossible date",
    /* INTL_ERR_BADGREGORIANDATE */ "The supplied GregorianDate structure pointer is invalid",
    /* INTL_ERR_BADDATESPEC      */ "The supplied DateSpec pointer is invalid",
    /* INTL_ERR_CANTFINDCOUNTRY  */ "Unable to access the country database",
    /* INTL_ERR_BADCHARACTERSET  */ "An unknown character set was specified for intlTransliterateString()",
    /* INTL_ERR_BADSOURCEBUFFER  */ "The supplied source buffer pointer is invalid",
    /* INTL_ERR_BADFLAGS         */ "The supplied flags parameters has some undefined bits that are set"
};

static TagArg const ErrorTags[] =
{
    TAG_ITEM_NAME,	(void *)"International Folio",
    ERRTEXT_TAG_OBJID,	(void *)((ER_FOLI<<ERR_IDSIZE)|(ER_INTL)),
    ERRTEXT_TAG_MAXERR,	(void *)(sizeof(ErrorMsgs)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,	(void *)ErrorMsgs,
    TAG_END,		0
};

const TagArg *main(void)
{
    return ErrorTags;
}
