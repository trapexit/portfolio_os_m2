/* @(#) FBl.errs.c 96/08/23 1.7 */

#include <kernel/types.h>
#include <kernel/operror.h>


static const char * const ErrorMsgs[] =
{
    "no error",
        /* BLITTER_ERR_DUPLICATETAG */ "Duplicate tag",
        /* BLITTER_ERR_SNIPPETUSED */ "Snippet in use",
        /*BLITTER_ERR_APPSSNIPPET */ "Snippet allocated by app",
        /* BLITTER_ERR_BADTYPE */ "Bad snippet type",
        /* BLITTER_ERR_NOTXDATA */ "No BltTxData in the BlitObject",
        /* BLITTER_ERR_SMALLTEXEL */ "Texel buffer is too small",
        /* BLITTER_ERR_NODBLEND */ "No DBlendSnippet in the BlitObject",
        /* BLITTER_ERR_NOTBLEND */ "No TxBlendSnippet in the BlitObject",    
        /* BLITTER_ERR_NOPIP */ "No PIPLoadSnippet in the BlitObject",    
        /* BLITTER_ERR_NOTXLOAD */ "No TxLoadSnippet in the BlitObject",    
        /* BLITTER_ERR_NOVTX */ "No VerticesSnippet in the BlitObject",    
        /* BLITTER_ERR_TOOBIG */ "Rectangular area is too big for TRAM",    
        /* BLITTER_ERR_BADSRCBOUNDS */ "Illegal rectangle source bounds",
        /* BLITTER_ERR_UNKNOWNIFFFILE */ "Unknown UTF chunk",
        /* BLITTER_ERR_FAULTYTEXTURE */ "Faulty texture data",

};

static const TagArg ErrorTags[] =
{
    TAG_ITEM_NAME,      (void *)"Blitter Folio",
    ERRTEXT_TAG_OBJID,  (void *)((ER_FOLI<<ERR_IDSIZE)|(ER_BLITTER)),
    ERRTEXT_TAG_MAXERR, (void *)(sizeof(ErrorMsgs)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,  (void *)ErrorMsgs,
    TAG_END,            0
};

const TagArg *main(void)
{
    return ErrorTags;
}
