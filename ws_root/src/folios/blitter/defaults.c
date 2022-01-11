/* @(#) defaults.c 96/08/23 1.3
 *
 *  Default snippets
 */

#include <graphics/blitter.h>
#include <graphics/clt/cltmacros.h>

const TxBlendSnippet defaultTxBlend =
{
    {
        0, 0, BLIT_TAG_TBLEND,    /* BlitterSnippetHeader */
    },
    CLT_WriteRegistersHeader(TXTADDRCNTL, 1),
    CLA_TXTADDRCNTL(1, 0, 0, 0, 0),
    CLT_WriteRegistersHeader(TXTTABCNTL, 1),
    CLA_TXTTABCNTL(0, 0, 0, 0, 0,
                   RC_TXTTABCNTL_COLOROUT_TEXCOLOR,
                   RC_TXTTABCNTL_ALPHAOUT_TEXALPHA,
                   0),
    CLT_WriteRegistersHeader(TXTSRCTYPE01, 7),
    0, 0,
    CLA_TXTEXPTYPE(5, 4, 0, 1, 1, 0, 1),   /* Assume 5bits RGB, 1 bit SSB = 16bit frame buffer */
    0, 0, 0, 0,
};

const TxLoadSnippet defaultTxLoad =
{
    {
        0, 0, BLIT_TAG_TXLOAD,    /* BlitterSnippetHeader */
    },
    CLT_WriteRegistersHeader(TXTLDCNTL, 1),
    CLA_TXTLDCNTL(0, RC_TXTLDCNTL_LOADMODE_TEXTURE, 0),
    CLT_WriteRegistersHeader(TXTLDDSTBASE, 1),
    0,
    CLT_WriteRegistersHeader(TXTLDSRCADDR, 3),
    0,    /* Put bitmap address here */
    0,    /* Put number of rows here */
    0,    /* put dstrowbits, srcrowbits here */
    CLT_WriteRegistersHeader(DCNTL, 1),
    CLT_Bits(DCNTL, TLD, 1),
    CLT_WriteRegistersHeader(TXTUVMAX, 2),
    (CLT_Bits(TXTUVMAX, UMAX, 0x3f) |      /* Put true texture size here */
     CLT_Bits(TXTUVMAX, VMAX, 0x3f)),
    (CLT_Bits(TXTUVMASK, UMASK, 0x3ff) |
     CLT_Bits(TXTUVMASK, VMASK, 0x3ff)),
};

const PIPLoadSnippet defaultPIPLoad =
{
    {
        0, 0, BLIT_TAG_PIP,    /* BlitterSnippetHeader */
    },
    CLT_WriteRegistersHeader(TXTPIPCNTL, 1),
    CLA_TXTPIPCNTL(RC_TXTPIPCNTL_PIPSSBSELECT_TEXTURE,
                   RC_TXTPIPCNTL_PIPALPHASELECT_TEXTURE,
                   RC_TXTPIPCNTL_PIPCOLORSELECT_TEXTURE,
                   0),
    CLT_WriteRegistersHeader(TXTCONST0, 2),
    0,
    0,
    CLT_WriteRegistersHeader(TXTLDCNTL, 1),
    CLA_TXTLDCNTL(0, RC_TXTLDCNTL_LOADMODE_PIP, 0),
    CLT_WriteRegistersHeader(TXTLODBASE0, 1),
    TEPIPRAM,
    CLT_WriteRegistersHeader(TXTLDSRCADDR, 2),
    0,
    0,
    CLT_WriteRegistersHeader(DCNTL, 1),
    CLT_Bits(DCNTL, TLD, 1),
};

const DBlendSnippet defaultDBlend =
{
    {
        0, 0, BLIT_TAG_DBLEND,    /* BlitterSnippetHeader */
    },
    CLT_WriteRegistersHeader(DBUSERCONTROL, 2),
    CLA_DBUSERCONTROL (0, 0, 0, 0, 0, 0, 0,   /* enable dblend in here */
                       (CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, RED) | 
                        CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, GREEN) | 
                        CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, BLUE) | 
                        CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, ALPHA))),
    CLA_DBDISCARDCONTROL(0, 0, 0, 0),
    /* default dblend is for mixinf foreground and background, but is
     * disabled in DBUSERCONTROL.
     */
    CLT_WriteRegistersHeader(DBSRCCNTL, 4),
    CLA_DBSRCCNTL(1, 0), /* Put 16/32bpp info here -- default 16bpp */
    0,                                 /* DBSRCBASEADDR Put bitmap address here */
    320,                             /* DBSRCXSTRIDE Put bitmap width here (in pixels) -- default 320 */
    CLA_DBSRCOFFSET(0, 0), /* DBSRCOFFSET Put offset of blend rect here */
    CLT_WriteRegistersHeader(DBSSBDSBCNTL, 9),
    CLA_DBSSBDSBCNTL(0, RC_DBSSBDSBCNTL_DSBSELECT_OBJSSB),
    0x0fff,
    CLA_DBAMULTCNTL(RC_DBAMULTCNTL_AINPUTSELECT_TEXCOLOR,
                    RC_DBAMULTCNTL_AMULTCOEFSELECT_CONST,
                    RC_DBAMULTCNTL_AMULTCONSTCONTROL_TEXSSB,
                    0),
    0x444444,     /* RA_DBAMULTCONSTSSB0 */
    0x444444,     /* RA_DBAMULTCONSTSSB1 */
    CLA_DBBMULTCNTL(RC_DBBMULTCNTL_BINPUTSELECT_SRCCOLOR,
                    RC_DBBMULTCNTL_BMULTCOEFSELECT_CONST,
                    RC_DBBMULTCNTL_BMULTCONSTCONTROL_TEXSSB,
                    3),
    0xcccccc,    /* RA_DBBMULTCONSTSSB0 */
    0xcccccc,    /* RA_DBBMULTCONSTSSB1 */
    CLA_DBALUCNTL(RC_DBALUCNTL_ALUOPERATION_A_PLUS_BCLAMP, 0),
    CLT_WriteRegistersHeader(DBSRCALPHACNTL, 3),
    0,
    0,
    0,
};

const BltTxData defaultTxData =
{
    {
        0, 0, BLIT_TAG_TXDATA,    /* BlitterSnippetHeader */
    },
    {
        0,    /* uncompressed */
        0,
        0,
        1,    /* 1 LOD */
        16,  /* Assume 16bpp */
        0,
        NULL,
        NULL,
    },
    {
        0,
        NULL,
    },
};

    
