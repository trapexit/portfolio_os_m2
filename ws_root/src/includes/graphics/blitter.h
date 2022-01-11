#ifndef __GRAPHICS_BLITTER_H
#define __GRAPHICS_BLITTER_H

/***************************************************************************
**
**  @(#) blitter.h 96/10/15 1.17
**
**  Definitions for the Blitter folio.
**
****************************************************************************/

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __GRAPHICS_CLT_GSTATE_H
#include <graphics/clt/gstate.h>
#endif

#ifndef __GRAPHICS_CLT_CLTMACROS_H
#include <graphics/clt/cltmacros.h>
#endif

#ifndef __GRAPHICS_CLT_CLTTXDBLEND_H
#include <graphics/clt/clttxdblend.h>
#endif

#ifndef __GRAPHICS_FRAME2D_FRAME2D_H
#include <graphics/frame2d/frame2d.h>
#endif

#ifndef __GRAPHICS_FRAME2D_LOADTXTR_H
#include <graphics/frame2d/loadtxtr.h>
#endif

#ifndef __GRAPHICS_PIPE_BOX_H
#include <graphics/pipe/box.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

/* kernel interface definitions */
#define BLITTER_FOLIONAME  "blitter"

typedef struct BlitterSnippetHeader
{
    uint32 bsh_usageCount;
    uint32 bsh_flags;      /* Private */
    uint32 bsh_type;
} BlitterSnippetHeader;

typedef struct TxBlendSnippet
{
    BlitterSnippetHeader txb_header;
    uint32 txb_instruction1;	/* WriteRegister instruction*/
    uint32 txb_addrCntl;	/* [0x00046408] TXTADDRCNTL */
    uint32 txb_instruction2;	/* WriteRegister instruction*/
    uint32 txb_tabCntl;	/* [0x00046410] TXTTABCNTL */
    uint32 txb_instruction3;	/* WriteRegister instruction*/
    uint32 txb_srcType[2];	/* [0x00046434/8] TXTSRCTYPE01/23 */
    uint32 txb_expType;	/* [0x0004643c] TXTEXPTYPE */
    uint32 txb_constant[2];	/* [0x00046440/4] TXTCONST0/1 */
    uint32 txb_tabConst[2];	/* [0x00046448/c] TXTTABCONST0, TXTTABCONT1
                                   * and TXTCONST2/3 */
} TxBlendSnippet;

typedef struct TxLoadSnippet
{
    BlitterSnippetHeader txl_header;
    uint32 txl_instruction1;	/* WriteRegister instruction */
    uint32 txl_cntl;		/* [0x00046404] TXTLDCNTL */
    uint32 txl_instruction2;	/* WriteRegister instruction */
    uint32 txl_base;		/* [0x00046414] TXTLDDSTBASE */
    uint32 txl_instruction3;	/* WriteRegister instruction */
    uint32 txl_src;		/* [0x00046424] TXTLDSRCADDR */
    uint32 txl_count;		/* [0x00046428] TXTCOUNT */
    uint32 txl_width;		/* [0x0004642c] TXTLDWIDTH */
    uint32 txl_instruction4;	/* WriteRegister instruction */
    uint32 txl_tLoad;		/* TextureLoad instruction */
    uint32 txl_instruction5;	/* WriteRegister instruction */
    uint32 txl_uvMax;	/* [0x0004642c] TXTUVMAX */
    uint32 txl_uvMask;	/* [0x00046430] TXTUVMASK */
} TxLoadSnippet;

typedef struct PIPLoadSnippet
{
    BlitterSnippetHeader ppl_header;
    uint32 ppl_instruction1;	/* WriteRegister instruction */
    uint32 ppl_cntl;		/* [0x0004640c] TXTPIPCNTL */
    uint32 ppl_instruction2;	/* WriteRegister instruction */
    uint32 ppl_constant[2];	/* [0x00046440/4] TXTCONST0, TXTCONST1 */
    uint32 ppl_instruction3;	/* WriteRegister instruction */
    uint32 ppl_ldCntl;	/* [0x00046404] TXTLDCNTL */
    uint32 ppl_instruction4;	/* WriteRegister instruction */
    uint32 ppl_base;		/* [0x00046414] TXTLODBASE0 */
    uint32 ppl_instruction5;	/* WriteRegister instruction */
    uint32 ppl_src;		/* [0x00046424] TXTLDSRCADDR */
    uint32 ppl_count;	/* [0x00046428] TXTCOUNT */
    uint32 ppl_instruction6;	/* WriteRegister instruction */
    uint32 ppl_pLoad;	/* PIPLoad instruction */
} PIPLoadSnippet;
#ifndef TEPIPRAM
#define TEPIPRAM 0x00046000
#endif

typedef struct DBlendSnippet
{
    BlitterSnippetHeader dbl_header;
    uint32 dbl_instruction1;	/* WriteRegister instruction */
    uint32 dbl_userGenCntl;	/* [0x00048008] DBUSERCONTROL */
    uint32 dbl_discardCntl;	/* [0x0004800c] DBDISCARDCONTROL */
    uint32 dbl_instruction2;	/* WriteRegister instruction */
    uint32 dbl_srcCntl;	/* [0x00048030] DBSRCCNTL */
    uint32 dbl_srcBaseAddr;	/* [0x00048034] DBSRCBASEADDR */
    uint32 dbl_srcXStride;	/* [0x00048038] DBSRCXSTRIDE */
    uint32 dbl_srcOffset;	/* [0x0004803c] DBSRCOFFSET */
    uint32 dbl_instruction3;	/* WriteRegister instruction */
    uint32 dbl_dsbCntl;	/* [0x00048050] DBSSBDSBCNTL */
    uint32 dbl_constIn;	/* [0x00048054] DBCONSTIN */
    uint32 dbl_txtMultCntl;	/* [0x00048058] DBAMULTCNTL */
    uint32 dbl_txtCoefConst[2];/* [0x0004805c/60] DBAMULTCONSTSSB0/1 */
    uint32 dbl_srcMultCntl;	/* [0x00048064] DBBMULTCNTL */
    uint32 dbl_multCoefConst[2];/* [0x00048068/6c] DBBMULTCONSTSSB0/1 */
    uint32 dbl_aluCntl;	/* [0x00048070] DBALUCNTL] */
    uint32 dbl_instruction4;	/* WriteRegister instruction */
    uint32 dbl_srcAlphaCntl;	/* [0x00048074] DBSRCALPHACNTL] */
    uint32 dbl_dstAlphaCntl;	/* [0x00048078] DBDESTALPHACNTL] */
    uint32 dbl_dstAlphaConst;/* [0x0004807c] DBDESTALPHACONST] */
} DBlendSnippet;

#define TRAM_SIZE 16384  /* Number of bytes in TRAM */
typedef struct BltTxData
{
    BlitterSnippetHeader btd_header;
    CltTxData btd_txData;	/* Store the texture data in here */
    CltTxLOD btd_txLOD;	/* btd_txData.texelData will point here */
    void *btd_slice;		/* System Private */
} BltTxData;

typedef struct VerticesSnippet
{
    BlitterSnippetHeader vtx_header;
    uint32 vtx_vertices;	/* The number of vertices in the list */
    uint32 vtx_wordsPerVertex;	/* The number of gfloats per vertex */
    uint32 vtx_uOffset;	/* Offset from the start of each vertex to the U value */
    uint32 vtx_rOffset;	/* Offset from the start of each vertex to the R value */
    uint32 vtx_wOffset;	/* Offset from the start of each vertex to the W value */
    void *vtx_private;             /* System private */
    uint32 vtx_instruction;	/* The CLT_TRIANGLE instruction */
    gfloat vtx_vertex[1];	/* The first vertex in the list */
} VerticesSnippet;
/* This macro finds the nth vertex in a VerticesSnippet structure */
#define BLITVERTEX(v, n) (&(v)->vtx_vertex[0] + ((v)->vtx_wordsPerVertex * (n)))

/* For the (x,y) values of a vertex: */
#define BLITVERTEX_X(v, n) BLITVERTEX(v, n)
#define BLITVERTEX_Y(v, n) (BLITVERTEX(v, n) + 1)

/* (u, v) are a little more complicated. */
#define BLITVERTEX_U(v, n) (BLITVERTEX(v, n) + (v)->vtx_uOffset)
#define BLITVERTEX_V(v, n) (BLITVERTEX_U(v, n) + 1)

/* (r, g, b, a) values: */
#define BLITVERTEX_R(v, n) (BLITVERTEX(v, n) + (v)->vtx_rOffset)
#define BLITVERTEX_G(v, n) (BLITVERTEX_R(v, n) + 1)
#define BLITVERTEX_B(v, n) (BLITVERTEX_R(v, n) + 2)
#define BLITVERTEX_A(v, n) (BLITVERTEX_R(v, n) + 3)

/* And the w value: */
#define BLITVERTEX_W(v, n) (BLITVERTEX(v, n) + (v)->vtx_wOffset)

typedef struct BlitObject
{
    PIPLoadSnippet *bo_pip;
    TxBlendSnippet *bo_tbl;
    TxLoadSnippet *bo_txl;
    DBlendSnippet *bo_dbl;
    BltTxData *bo_txdata;
    VerticesSnippet *bo_vertices;
    void *bo_blitStuff;    /* Private */
} BlitObject;

enum BlitObject_tags
{
    BLIT_TAG_DBLEND = (TAG_ITEM_LAST + 1),
    BLIT_TAG_TBLEND,
    BLIT_TAG_TXLOAD,
    BLIT_TAG_PIP,
    BLIT_TAG_TXDATA,
    BLIT_TAG_VERTICES
};

enum skipSnippet_flags /* pass these to Blt_BlitObjectToBitmap() */
{
    SKIP_SNIPPET_DBLEND = 0x00000001,
    SKIP_SNIPPET_TBLEND = 0x00000002,
    SKIP_SNIPPET_TXLOAD = 0x00000004,
    SKIP_SNIPPET_PIP = 0x00000008,
    SKIP_SNIPPET_TXDATA = 0x00000010,
    SKIP_SNIPPET_VERTICES = 0x00000020,
};

typedef struct BlitMask
{
    uint32 blm_discardType;	/* SSB, Alpha, or RGB */
    uint32 blm_width;
    uint32 blm_height;
    uint32 *blm_data;
    uint32 blm_flags;
} BlitMask;
#define FLAG_BLM_INVERT 1
#define FLAG_BLM_REPEAT 2
#define FLAG_BLM_CENTER 4
#define FLAG_BLM_FORCE_VISIBLE 8

typedef Box2 BlitRect;

/* Use the BlitMatrix for Blt_TransformVertices() */
typedef gfloat BlitMatrix[3][2];

/* This is passed to the Blt_Scroll() utility function */
typedef struct BlitScroll
{
    BlitRect bsc_region;        /* Rectangular region to scroll */
    gfloat bsc_dx;
    gfloat bsc_dy;              /* Distance to scroll by */
    uint32 bsc_replaceColor;    /* Color to replace scrolled region with */
} BlitScroll;
#define BLT_SCROLL_VERTICES CLA_TRIANGLE(1, RC_FAN, 1, 1, 0, 4)

/* Use this for the W value */
#define W_2D 0.999998

/* Use the for the blitter_utils function Blt_LoadTexture() */
typedef struct BlitObjectNode
{
    MinNode bon_Node;
    BlitObject *bon_BlitObject;
} BlitObjectNode;


Err OpenBlitterFolio(void);
void CloseBlitterFolio(void);

Err Blt_CreateBlitObject(BlitObject **bo, const TagArg *tagList);
Err Blt_CreateBlitObjectVA(BlitObject **bo, uint32 tag, ...);
void Blt_DeleteBlitObject(const BlitObject *bo);

Err Blt_RectangleToBlitObject(GState *gs, const BlitObject *bo, Item srcBitmap, const BlitRect *rect);
Err Blt_RectangleToBuffer(GState *gs, Item srcBitmap, const BlitRect *rect, void *buffer);
Err Blt_BlitObjectToBitmap(GState *gs, const BlitObject *bo, Item dstBitmap, uint32 flags);

Err Blt_CreateVertices(VerticesSnippet **vtx, uint32 triangles);
#define Blt_DeleteVertices(v) Blt_DeleteSnippet((void *)v)
Err Blt_SetVertices(VerticesSnippet *vtx, const gfloat *vertices);
Err Blt_MoveVertices(VerticesSnippet*vtx, gfloat dx, gfloat dy);
Err Blt_RotateVertices(VerticesSnippet *vtx, gfloat angle, gfloat x, gfloat y);
Err Blt_TransformVertices(VerticesSnippet *vtx, BlitMatrix bm);
Err Blt_SetClipBox(BlitObject *bo, bool set, bool clipOut, Point2 *tl, Point2 *br);
Err Blt_SetTexture(BlitObject *bo, void *texelData);
Err Blt_GetTexture(BlitObject *bo, void **texelData);
Err Blt_MoveUV(VerticesSnippet *vtx, gfloat du, gfloat dv);

void *Blt_CreateSnippet(uint32 type);
Err Blt_DeleteSnippet(const void *snippet);
void *Blt_ReuseSnippet(const BlitObject *src, BlitObject *dest, uint32 type);
Err Blt_CopySnippet(const BlitObject *src, BlitObject *dest, void **original, uint32 type);
void Blt_SwapSnippet(BlitObject *bo1, BlitObject *bo2, uint32 type);
void *Blt_RemoveSnippet(BlitObject *bo, uint32 type);
void *Blt_SetSnippet(BlitObject *bo, void *snippet);

/* Mask functions */
Err Blt_MakeMask(const BlitObject *bo, const BlitMask *mask);

/* Query functions */
bool Blt_BlitObjectSliced(BlitObject *bo);
bool Blt_BlitObjectClipped(BlitObject *bo);

/* These functions are part of the blitter_utils lib */
Err Blt_BlendBlitObject(BlitObject *bo, gfloat src, gfloat dest);
Err Blt_EnableMask(BlitObject *bo, uint32 discardType);
Err Blt_DisableMask(BlitObject *bo);
Err Blt_RectangleInBitmap(GState *gs, BlitObject *bo, Item bmI, BlitRect *br);
Err Blt_Scroll(GState *gs, BlitObject *bo, Item bitmap, BlitScroll *bs);
Err Blt_LoadUTF(BlitObject **bo, const char *fname);
Err Blt_FreeUTF(BlitObject *bo);
Err Blt_LoadTextureVA(List *blitObjectList, uint32 tag, ...);
Err Blt_LoadTexture(List *blitObjectList, const TagArg *tags);
void Blt_FreeTexture(List *blitObjectList);
void Blt_InitDimensions(BlitObject *bo, uint32 width, uint32 height, uint32 bitsPerPixel);

/* Error codes */
#define MakeBlitterErr(svr, class, err) MakeErr(ER_FOLI, ER_BLITTER, svr, ER_E_SSTM, class, err)
/* No memory */
#define BLITTER_ERR_NOMEM MakeBlitterErr(ER_SEVERE, ER_C_STND, ER_NoMem)
#define BLITTER_ERR_BADPTR MakeBlitterErr(ER_SEVERE, ER_C_STND, ER_BadPtr)
#define BLITTER_ERR_BADTAGARG MakeBlitterErr(ER_SEVERE, ER_C_STND, ER_BadTagArg)
#define BLITTER_ERR_BADITEM MakeBlitterErr(ER_SEVERE, ER_C_STND, ER_BadItem)

#define BLITTER_ERR_DUPLICATETAG MakeBlitterErr(ER_SEVERE, ER_C_NSTND, 1)
#define BLITTER_ERR_SNIPPETUSED MakeBlitterErr(ER_SEVERE, ER_C_NSTND, 2)
#define BLITTER_ERR_APPSSNIPPET MakeBlitterErr(ER_SEVERE, ER_C_NSTND, 3)
#define BLITTER_ERR_BADTYPE MakeBlitterErr(ER_SEVERE, ER_C_NSTND, 4)
#define BLITTER_ERR_NOTXDATA MakeBlitterErr(ER_SEVERE, ER_C_NSTND, 5)
#define BLITTER_ERR_SMALLTEXEL MakeBlitterErr(ER_SEVERE, ER_C_NSTND, 6)
#define BLITTER_ERR_NODBLEND MakeBlitterErr(ER_SEVERE, ER_C_NSTND, 7)
#define BLITTER_ERR_NOTBLEND MakeBlitterErr(ER_SEVERE, ER_C_NSTND, 8)
#define BLITTER_ERR_NOPIP MakeBlitterErr(ER_SEVERE, ER_C_NSTND, 9)
#define BLITTER_ERR_NOTXLOAD MakeBlitterErr(ER_SEVERE, ER_C_NSTND, 10)
#define BLITTER_ERR_NOVTX MakeBlitterErr(ER_SEVERE, ER_C_NSTND, 11)
#define BLITTER_ERR_TOOBIG MakeBlitterErr(ER_SEVERE, ER_C_NSTND, 12)
#define BLITTER_ERR_BADSRCBOUNDS MakeBlitterErr(ER_SEVERE, ER_C_NSTND, 13)
#define BLITTER_ERR_UNKNOWNIFFFILE MakeBlitterErr(ER_SEVERE, ER_C_NSTND, 14)
#define BLITTER_ERR_FAULTYTEXTURE MakeBlitterErr(ER_SEVERE, ER_C_NSTND, 15)

#endif
