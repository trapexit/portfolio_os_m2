/***************************************************************************
**
** @(#) blend.c 96/06/07 1.3
**
**  Code to calculate destination blend instructions.
**
****************************************************************************/

#include <kernel/types.h>
#include <graphics/clt/cltmacros.h>
#include <graphics/blitter.h>

Err Blt_BlendBlitObject(BlitObject *bo, gfloat src, gfloat dest)
{
    DBlendSnippet *dbl = bo->bo_dbl;
    uint8 mult;
    
    if (dbl == NULL)
    {
        return(BLITTER_ERR_NODBLEND);
    }

    mult = (uint32)(255.0 * src);
    dbl->dbl_txtCoefConst[0] = dbl->dbl_txtCoefConst[1] =
        ((mult << 16) | (mult << 8) | mult);
    mult = (uint32)(255.0 * dest);
    dbl->dbl_multCoefConst[0] = dbl->dbl_multCoefConst[1] =
        ((mult << 16) | (mult << 8) | mult);

    /* Enable DBlending */
    dbl->dbl_userGenCntl |= (FV_DBUSERCONTROL_BLENDEN_MASK |
                             FV_DBUSERCONTROL_SRCEN_MASK);

    /* Set up the MultCntl parameters */
    dbl->dbl_txtMultCntl =
        CLA_DBAMULTCNTL(RC_DBAMULTCNTL_AINPUTSELECT_TEXCOLOR,
                        RC_DBAMULTCNTL_AMULTCOEFSELECT_CONST,
                        RC_DBAMULTCNTL_AMULTCONSTCONTROL_TEXSSB,
                        0);
    dbl->dbl_srcMultCntl =
        CLA_DBBMULTCNTL(RC_DBBMULTCNTL_BINPUTSELECT_SRCCOLOR,
                        RC_DBBMULTCNTL_BMULTCOEFSELECT_CONST,
                        RC_DBBMULTCNTL_BMULTCONSTCONTROL_TEXSSB,
                        0);

    if (dest == 0.0)
    {
        /* Don't use B */
        dbl->dbl_aluCntl =
            CLA_DBALUCNTL(RC_DBALUCNTL_ALUOPERATION_A, 0);
        dbl->dbl_userGenCntl &= ~(FV_DBUSERCONTROL_BLENDEN_MASK |
                                  FV_DBUSERCONTROL_SRCEN_MASK);

    }
    else if (src == 0.0)
    {
        /* Don't use A */
        dbl->dbl_aluCntl =
            CLA_DBALUCNTL(RC_DBALUCNTL_ALUOPERATION_B, 0);

    }
    else
    {
        dbl->dbl_aluCntl =
            CLA_DBALUCNTL(RC_DBALUCNTL_ALUOPERATION_A_PLUS_BCLAMP, 0);
    }
    
    return(0);
}


   
        
