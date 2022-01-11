/* @(#) simplecompress.c 95/09/22 1.6 */

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_FOLIO_H
#include <kernel/folio.h>
#endif

#ifndef __MISC_COMPRESSION_H
#include <misc/compression.h>
#endif

/*****************************************************************************/


typedef struct Context
{
    uint32 *ctx_Dest;
    uint32 *ctx_Max;
    bool    ctx_Overflow;
} Context;


/*****************************************************************************/


static void PutWord(Context *ctx, uint32 word)
{
    if (ctx->ctx_Dest >= ctx->ctx_Max)
        ctx->ctx_Overflow = TRUE;
    else
        *ctx->ctx_Dest++ = word;
}


/*****************************************************************************/


Err SimpleCompress(const void *source, uint32 sourceWords,
                   void *result, uint32 resultWords)
{
Compressor *comp;
TagArg      tags[2];
Context     ctx;
Err         err;

    ctx.ctx_Dest     = (uint32 *)result;
    ctx.ctx_Max      = (uint32 *)((uint32)result + resultWords * sizeof(uint32));
    ctx.ctx_Overflow = FALSE;

    tags[0].ta_Tag = COMP_TAG_USERDATA;
    tags[0].ta_Arg = (void *)&ctx;
    tags[1].ta_Tag = TAG_END;

    err = CreateCompressor(&comp,(CompFunc)PutWord,tags);
    if (err >= 0)
    {
        FeedCompressor(comp,source,sourceWords);
        err = DeleteCompressor(comp);

        if (err == 0)
        {
            if (ctx.ctx_Overflow)
                err = COMP_ERR_OVERFLOW;
            else
                err = ((uint32)ctx.ctx_Dest - (uint32)result) / sizeof(uint32);
        }
    }

    return err;
}
