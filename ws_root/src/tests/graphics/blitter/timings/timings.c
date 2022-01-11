/******************************************************************************
**
**  @(#) timings.c 96/09/18 1.1
**
**  Measures the time difference between similar operations in the
**  Frame2d folio and the Blitter folio.
**
******************************************************************************/

#include <kernel/types.h>
#include <graphics/frame2d/frame2d.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/clt/cltmacros.h>
#include <graphics/blitter.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <kernel/mem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CKERR(x, y) {if(x < 0){printf("Error - 0x%lx %s\n", x, y);PrintfSysErr(x); goto cleanup;}}
#define ERR(s) {printf((s)); goto cleanup;}
#define CKNULL(x, y) {if(x == NULL){printf("Error - %s\n", x, y); goto cleanup;}}

#define LOOPCOUNT 1000
#define FBWIDTH 320
#define FBHEIGHT 240
#define DEPTH 32
#define NUM_FBUFFS 1
#define NUM_ZBUFFS 0
#define NUM_TBUFFS (NUM_FBUFFS + NUM_ZBUFFS)

uint32 BestViewType(uint32 xres, uint32 yres, uint32 depth)
{
    uint32 viewType;

    viewType = VIEWTYPE_INVALID;

    if ((xres==320) && (yres==240) && (depth==16)) viewType = VIEWTYPE_16;
    if ((xres==320) && (yres==480) && (depth==16)) viewType = VIEWTYPE_16_LACE;
    if ((xres==320) && (yres==240) && (depth==32)) viewType = VIEWTYPE_32;
    if ((xres==320) && (yres==480) && (depth==32)) viewType = VIEWTYPE_32_LACE;
    if ((xres==640) && (yres==240) && (depth==16)) viewType = VIEWTYPE_16_640;
    if ((xres==640) && (yres==480) && (depth==16)) viewType = VIEWTYPE_16_640_LACE;
    if ((xres==640) && (yres==240) && (depth==32)) viewType = VIEWTYPE_32_640;
    if ((xres==640) && (yres==480) && (depth==32)) viewType = VIEWTYPE_32_640_LACE;

    return viewType;
}

void SetDBUserControl(GState *gs)
{
    static const uint32 dbucList[] =
    {
        CLT_SetRegistersHeader(DBUSERCONTROL,1),
        CLA_DBUSERCONTROL(0, 0, 0, 0, 0, 0, 0, (DBL_RGBDestOut | DBL_AlphaDestOut)),
    };
    static const CltSnippet dbucSnippet =
    {
        &dbucList[0],
        (sizeof(dbucList) / sizeof(uint32)),
    };

    GS_Reserve(gs, dbucSnippet.size);
    CLT_CopySnippetData(GS_Ptr(gs), &dbucSnippet);
}

int main(int argc, char **argv)
{
    GState *gs;
    Item bitmaps[NUM_TBUFFS];
    Item viewI = 0;
    Item vbl = 0;
    uint32 bmType = ((DEPTH == 16) ? BMTYPE_16 : BMTYPE_32);
    const uint32 viewType = BestViewType(FBWIDTH, FBHEIGHT, DEPTH);
    BlitObject  *bdropBO = NULL;
    SpriteObj *bdropSO = NULL;
    Err err;
    uint32 i;
    Point2 pt, tl, br;
    uint32 width, height;
        /* Timing stuff */
    TimeVal tv;
    TimerTicks in[2], out, difference[2], total[2];

    TOUCH(argc);
    
    OpenGraphicsFolio();
    OpenBlitterFolio();
    gs = GS_Create();
    if (gs == NULL)
    {
        ERR("Could not create GState\n");
    }
    
    GS_AllocLists(gs, NUM_FBUFFS, 2048 * 4);
    err = GS_AllocBitmaps(bitmaps, FBWIDTH, FBHEIGHT, bmType, NUM_FBUFFS, NUM_ZBUFFS);
    CKERR(err, "Can't GS_AllocBitmaps()!\n"); 
    GS_SetDestBuffer(gs, bitmaps[0]);
#if (NUM_ZBUFFS != 0)
    GS_SetZBuffer(gs, bitmaps[NUM_FBUFFS]); /* Use the last buffer as the Z buffer */
#endif
    viewI = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
                         VIEWTAG_VIEWTYPE, viewType,
                         VIEWTAG_BITMAP, bitmaps[0],
                         TAG_END );
    CKERR(viewI, "can't create view item");
    AddViewToViewList(viewI, 0);

    ModifyGraphicsItemVA(viewI,
                         VIEWTAG_BITMAP, bitmaps[0],
                         TAG_END);
    
    printf("Load sprite %s\n", argv[1]);
    bdropSO = Spr_Create(NULL);
    CKNULL(bdropSO, "can't create bdrop sprite");
    err = Spr_LoadUTF(bdropSO, argv[1]);
    CKERR(err, "Could not load utf file");
    Spr_SetTextureAttr(bdropSO, TXA_TextureEnable, 1);
    Spr_SetDBlendAttr(bdropSO, DBLA_SrcInputEnable, 0);
    Spr_SetDBlendAttr(bdropSO, DBLA_BlendEnable, 0);
    Spr_SetDBlendAttr(bdropSO, DBLA_Discard, 0);
    Spr_SetTextureAttr(bdropSO, TXA_ColorOut, TX_BlendOutSelectTex);
    Spr_ResetCorners(bdropSO, SPR_TOPLEFT);

    printf("Load BlitObject %s\n", argv[1]);
    err = Blt_LoadUTF(&bdropBO, argv[1]);
    CKERR(err, "Could not load utf file");

    GS_SetDestBuffer(gs, bitmaps[0]);

        /* Now time the sprite */
    memset(&total[0], 0, sizeof(TimerTicks));
    memset(&total[1], 0, sizeof(TimerTicks));
    CLT_ClearFrameBuffer(gs, 0, 0, 0, 0, TRUE, FALSE);
    GS_SendList(gs);
    GS_WaitIO(gs);
    SetDBUserControl(gs);
    for (i = 0; i < LOOPCOUNT; i++)
    {
        SampleSystemTimeTT(&in[0]);
        F2_Draw(gs, bdropSO);
        SampleSystemTimeTT(&in[1]);
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        SubTimerTicks(&in[0], &out, &difference[0]); /* difference = out - in */
        AddTimerTicks(&difference[0], &total[0], &total[0]);
        SubTimerTicks(&in[1], &out, &difference[1]); /* difference = out - in */
        AddTimerTicks(&difference[1], &total[1], &total[1]);
    }
    ConvertTimerTicksToTimeVal(&total[0], &tv);
    printf("F2_Draw image %ld times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
    ConvertTimerTicksToTimeVal(&total[1], &tv);
    printf("(Render time took %d.%06d seconds)\n", tv.tv_Seconds,tv.tv_Microseconds);
    SubTimerTicks(&total[1], &total[0], &difference[0]);
    ConvertTimerTicksToTimeVal(&difference[0], &tv);
    printf("Setup time took %d.%06d seconds\n\n", tv.tv_Seconds,tv.tv_Microseconds);

        /* Now time the BlitObject */
    memset(&total, 0, sizeof(TimerTicks));
    memset(&total[1], 0, sizeof(TimerTicks));
    CLT_ClearFrameBuffer(gs, 0, 0, 0, 0, TRUE, FALSE);
    GS_SendList(gs);
    GS_WaitIO(gs);
    for (i = 0; i < LOOPCOUNT; i++)
    {
        SampleSystemTimeTT(&in[0]);
        Blt_BlitObjectToBitmap(gs, bdropBO, bitmaps[0], 0);
        SampleSystemTimeTT(&in[1]);
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        SubTimerTicks(&in[0], &out, &difference[0]); /* difference = out - in */
        AddTimerTicks(&difference[0], &total[0], &total[0]);
        SubTimerTicks(&in[1], &out, &difference[1]); /* difference = out - in */
        AddTimerTicks(&difference[1], &total[1], &total[1]);
    }
    ConvertTimerTicksToTimeVal(&total[0], &tv);
    printf("Blt image %ld times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
    ConvertTimerTicksToTimeVal(&total[1], &tv);
    printf("(Render time took %d.%06d seconds)\n", tv.tv_Seconds,tv.tv_Microseconds);
    SubTimerTicks(&total[1], &total[0], &difference[0]);
    ConvertTimerTicksToTimeVal(&difference[0], &tv);
    printf("Setup time took %d.%06d seconds\n\n", tv.tv_Seconds,tv.tv_Microseconds);

        /* Now time the BlitObject, optimisied case */
    memset(&total, 0, sizeof(TimerTicks));
    memset(&total[1], 0, sizeof(TimerTicks));
    CLT_ClearFrameBuffer(gs, 0, 0, 0, 0, TRUE, FALSE);
    Blt_BlitObjectToBitmap(gs, bdropBO, bitmaps[0], 0);
    GS_SendList(gs);
    GS_WaitIO(gs);
    for (i = 0; i < LOOPCOUNT; i++)
    {
        SampleSystemTimeTT(&in[0]);
        Blt_BlitObjectToBitmap(gs, bdropBO, bitmaps[0],
                           SKIP_SNIPPET_DBLEND | SKIP_SNIPPET_TBLEND | SKIP_SNIPPET_TXLOAD | SKIP_SNIPPET_PIP);
        SampleSystemTimeTT(&in[1]);
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        SubTimerTicks(&in[0], &out, &difference[0]); /* difference = out - in */
        AddTimerTicks(&difference[0], &total[0], &total[0]);
        SubTimerTicks(&in[1], &out, &difference[1]); /* difference = out - in */
        AddTimerTicks(&difference[1], &total[1], &total[1]);
    }
    ConvertTimerTicksToTimeVal(&total[0], &tv);
    printf("optimised Blt image %ld times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
    ConvertTimerTicksToTimeVal(&total[1], &tv);
    printf("Render time took %d.%06d seconds\n", tv.tv_Seconds,tv.tv_Microseconds);
    SubTimerTicks(&total[1], &total[0], &difference[0]);
    ConvertTimerTicksToTimeVal(&difference[0], &tv);
    printf("Setup time took %d.%06d seconds\n\n", tv.tv_Seconds,tv.tv_Microseconds);
    
    printf("-------------------------------------------------\n");
    
        /* Measure time to move the sprite accross the screen */
        /* First, unclipped */
        /* Sprite: */
    memset(&total[0], 0, sizeof(TimerTicks));
    memset(&total[1], 0, sizeof(TimerTicks));
    CLT_ClearFrameBuffer(gs, 0, 0, 0, 0, TRUE, FALSE);
    GS_SendList(gs);
    GS_WaitIO(gs);
    SetDBUserControl(gs);
    width = Spr_GetWidth(bdropSO);
    for (i = 0; i < (FBWIDTH - width); i++)
    {
        pt.x = i;
        pt.y = 0;
        SampleSystemTimeTT(&in[0]);
        Spr_SetPosition(bdropSO, &pt);
        F2_Draw(gs, bdropSO);
        SampleSystemTimeTT(&in[1]);
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        SubTimerTicks(&in[0], &out, &difference[0]); /* difference = out - in */
        AddTimerTicks(&difference[0], &total[0], &total[0]);
        SubTimerTicks(&in[1], &out, &difference[1]); /* difference = out - in */
        AddTimerTicks(&difference[1], &total[1], &total[1]);
    }
    ConvertTimerTicksToTimeVal(&total[0], &tv);
    printf("Unclipped move sprite image %ld times took %d.%06d seconds\n", (FBWIDTH - width), tv.tv_Seconds,tv.tv_Microseconds);
    ConvertTimerTicksToTimeVal(&total[1], &tv);
    printf("(Render time took %d.%06d seconds)\n", tv.tv_Seconds,tv.tv_Microseconds);
    SubTimerTicks(&total[1], &total[0], &difference[0]);
    ConvertTimerTicksToTimeVal(&difference[0], &tv);
    printf("Setup time took %d.%06d seconds\n\n", tv.tv_Seconds,tv.tv_Microseconds);
        
        /* BlitObject unoptimised: */
    memset(&total[0], 0, sizeof(TimerTicks));
    memset(&total[1], 0, sizeof(TimerTicks));
    CLT_ClearFrameBuffer(gs, 0, 0, 0, 0, TRUE, FALSE);
    GS_SendList(gs);
    GS_WaitIO(gs);
    for (i = 0; i < (FBWIDTH - width); i++)
    {
        SampleSystemTimeTT(&in[0]);
        Blt_MoveVertices(bdropBO->bo_vertices, 1.0, 0);
        Blt_BlitObjectToBitmap(gs, bdropBO, bitmaps[0], 0);
        SampleSystemTimeTT(&in[1]);
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        SubTimerTicks(&in[0], &out, &difference[0]); /* difference = out - in */
        AddTimerTicks(&difference[0], &total[0], &total[0]);
        SubTimerTicks(&in[1], &out, &difference[1]); /* difference = out - in */
        AddTimerTicks(&difference[1], &total[1], &total[1]);
    }
    ConvertTimerTicksToTimeVal(&total[0], &tv);
    printf("Unclipped move BlitObject %ld times took %d.%06d seconds\n", (FBWIDTH - width), tv.tv_Seconds,tv.tv_Microseconds);
    ConvertTimerTicksToTimeVal(&total[1], &tv);
    printf("(Render time took %d.%06d seconds)\n", tv.tv_Seconds,tv.tv_Microseconds);
    SubTimerTicks(&total[1], &total[0], &difference[0]);
    ConvertTimerTicksToTimeVal(&difference[0], &tv);
    printf("Setup time took %d.%06d seconds\n\n", tv.tv_Seconds,tv.tv_Microseconds);

        /* Now enable clipping, but dont clip  */
        /* Sprite: */
    memset(&total[0], 0, sizeof(TimerTicks));
    memset(&total[1], 0, sizeof(TimerTicks));
    CLT_ClearFrameBuffer(gs, 0, 0, 0, 0, TRUE, FALSE);
    GS_SendList(gs);
    GS_WaitIO(gs);
    SetDBUserControl(gs);
    Spr_EnableClipping(bdropSO);
    for (i = 0; i < (FBWIDTH - width); i++)
    {
        pt.x = i;
        pt.y = 0;
        SampleSystemTimeTT(&in[0]);
        Spr_SetPosition(bdropSO, &pt);
        F2_Draw(gs, bdropSO);
        SampleSystemTimeTT(&in[1]);
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        SubTimerTicks(&in[0], &out, &difference[0]); /* difference = out - in */
        AddTimerTicks(&difference[0], &total[0], &total[0]);
        SubTimerTicks(&in[1], &out, &difference[1]); /* difference = out - in */
        AddTimerTicks(&difference[1], &total[1], &total[1]);
    }
    ConvertTimerTicksToTimeVal(&total[0], &tv);
    printf("Clip enabled move sprite image %ld times took %d.%06d seconds\n", (FBWIDTH - width), tv.tv_Seconds,tv.tv_Microseconds);
    ConvertTimerTicksToTimeVal(&total[1], &tv);
    printf("(Render time took %d.%06d seconds)\n", tv.tv_Seconds,tv.tv_Microseconds);
    SubTimerTicks(&total[1], &total[0], &difference[0]);
    ConvertTimerTicksToTimeVal(&difference[0], &tv);
    printf("Setup time took %d.%06d seconds\n\n", tv.tv_Seconds,tv.tv_Microseconds);
        
        /* Now clip */
        /* Sprite: */
    memset(&total[0], 0, sizeof(TimerTicks));
    memset(&total[1], 0, sizeof(TimerTicks));
    CLT_ClearFrameBuffer(gs, 0, 0, 0, 0, TRUE, FALSE);
    GS_SendList(gs);
    GS_WaitIO(gs);
    SetDBUserControl(gs);
    Spr_EnableClipping(bdropSO);
    for (; i < FBWIDTH; i++)
    {
        pt.x = i;
        pt.y = 0;
        SampleSystemTimeTT(&in[0]);
        Spr_SetPosition(bdropSO, &pt);
        F2_Draw(gs, bdropSO);
        SampleSystemTimeTT(&in[1]);
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        SubTimerTicks(&in[0], &out, &difference[0]); /* difference = out - in */
        AddTimerTicks(&difference[0], &total[0], &total[0]);
        SubTimerTicks(&in[1], &out, &difference[1]); /* difference = out - in */
        AddTimerTicks(&difference[1], &total[1], &total[1]);
    }
    ConvertTimerTicksToTimeVal(&total[0], &tv);
    printf("Clipped move sprite image %ld times took %d.%06d seconds\n", width, tv.tv_Seconds,tv.tv_Microseconds);
    ConvertTimerTicksToTimeVal(&total[1], &tv);
    printf("(Render time took %d.%06d seconds)\n", tv.tv_Seconds,tv.tv_Microseconds);
    SubTimerTicks(&total[1], &total[0], &difference[0]);
    ConvertTimerTicksToTimeVal(&difference[0], &tv);
    printf("Setup time took %d.%06d seconds\n\n", tv.tv_Seconds,tv.tv_Microseconds);
        
        /* Now enable clipping, but dont clip  */
        /* BlitObject unoptimised: */
    memset(&total[0], 0, sizeof(TimerTicks));
    memset(&total[1], 0, sizeof(TimerTicks));
    CLT_ClearFrameBuffer(gs, 0, 0, 0, 0, TRUE, FALSE);
    GS_SendList(gs);
    GS_WaitIO(gs);
    Blt_MoveVertices(bdropBO->bo_vertices, -(gfloat)(FBWIDTH - width), 0);
    tl.x = 0;
    tl.y = 0;
    br.x = (FBWIDTH - 1);
    br.y = (FBHEIGHT - 1);
    Blt_SetClipBox(bdropBO, TRUE, TRUE, &tl, &br);
    for (i = 0; i < (FBWIDTH - width); i++)
    {
        SampleSystemTimeTT(&in[0]);
        Blt_MoveVertices(bdropBO->bo_vertices, 1.0, 0);
        Blt_BlitObjectToBitmap(gs, bdropBO, bitmaps[0], 0);
        SampleSystemTimeTT(&in[1]);
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        SubTimerTicks(&in[0], &out, &difference[0]); /* difference = out - in */
        AddTimerTicks(&difference[0], &total[0], &total[0]);
        SubTimerTicks(&in[1], &out, &difference[1]); /* difference = out - in */
        AddTimerTicks(&difference[1], &total[1], &total[1]);
    }
    ConvertTimerTicksToTimeVal(&total[0], &tv);
    printf("Clip enabled move BlitObject %ld times took %d.%06d seconds\n", (FBWIDTH - width), tv.tv_Seconds,tv.tv_Microseconds);
    ConvertTimerTicksToTimeVal(&total[1], &tv);
    printf("(Render time took %d.%06d seconds)\n", tv.tv_Seconds,tv.tv_Microseconds);
    SubTimerTicks(&total[1], &total[0], &difference[0]);
    ConvertTimerTicksToTimeVal(&difference[0], &tv);
    printf("Setup time took %d.%06d seconds\n\n", tv.tv_Seconds,tv.tv_Microseconds);

        /* Now clip */
        /* BlitObject unoptimised: */
    memset(&total[0], 0, sizeof(TimerTicks));
    memset(&total[1], 0, sizeof(TimerTicks));
    CLT_ClearFrameBuffer(gs, 0, 0, 0, 0, TRUE, FALSE);
    GS_SendList(gs);
    GS_WaitIO(gs);
    Blt_SetClipBox(bdropBO, TRUE, TRUE, &tl, &br);
    for (; i < FBWIDTH; i++)
    {
        SampleSystemTimeTT(&in[0]);
        Blt_MoveVertices(bdropBO->bo_vertices, 1.0, 0);
        Blt_BlitObjectToBitmap(gs, bdropBO, bitmaps[0], 0);
        SampleSystemTimeTT(&in[1]);
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        SubTimerTicks(&in[0], &out, &difference[0]); /* difference = out - in */
        AddTimerTicks(&difference[0], &total[0], &total[0]);
        SubTimerTicks(&in[1], &out, &difference[1]); /* difference = out - in */
        AddTimerTicks(&difference[1], &total[1], &total[1]);
    }
    ConvertTimerTicksToTimeVal(&total[0], &tv);
    printf("Clipped move BlitObject %ld times took %d.%06d seconds\n", width, tv.tv_Seconds,tv.tv_Microseconds);
    ConvertTimerTicksToTimeVal(&total[1], &tv);
    printf("(Render time took %d.%06d seconds)\n", tv.tv_Seconds,tv.tv_Microseconds);
    SubTimerTicks(&total[1], &total[0], &difference[0]);
    ConvertTimerTicksToTimeVal(&difference[0], &tv);
    printf("Setup time took %d.%06d seconds\n\n", tv.tv_Seconds,tv.tv_Microseconds);

        /* Clipped move up screen. If the texture is sliced, then measures the time difference
         * to discard unused slices.
         */
        /* Sprite: */
    memset(&total[0], 0, sizeof(TimerTicks));
    memset(&total[1], 0, sizeof(TimerTicks));
    CLT_ClearFrameBuffer(gs, 0, 0, 0, 0, TRUE, FALSE);
    GS_SendList(gs);
    GS_WaitIO(gs);
    SetDBUserControl(gs);
    pt.x = 0;
    pt.y = 0;
    Spr_SetPosition(bdropSO, &pt);
    height = Spr_GetHeight(bdropSO);
    for (i = 0; i < height; i++)
    {
        pt.x = 0;
        pt.y = -(gfloat)i;
        SampleSystemTimeTT(&in[0]);
        Spr_SetPosition(bdropSO, &pt);
        F2_Draw(gs, bdropSO);
        SampleSystemTimeTT(&in[1]);
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        SubTimerTicks(&in[0], &out, &difference[0]); /* difference = out - in */
        AddTimerTicks(&difference[0], &total[0], &total[0]);
        SubTimerTicks(&in[1], &out, &difference[1]); /* difference = out - in */
        AddTimerTicks(&difference[1], &total[1], &total[1]);
    }
    ConvertTimerTicksToTimeVal(&total[0], &tv);
    printf("clipped move sprite image up %ld times took %d.%06d seconds\n", height, tv.tv_Seconds,tv.tv_Microseconds);
    ConvertTimerTicksToTimeVal(&total[1], &tv);
    printf("(Render time took %d.%06d seconds)\n", tv.tv_Seconds,tv.tv_Microseconds);
    SubTimerTicks(&total[1], &total[0], &difference[0]);
    ConvertTimerTicksToTimeVal(&difference[0], &tv);
    printf("Setup time took %d.%06d seconds\n\n", tv.tv_Seconds,tv.tv_Microseconds);
    
    memset(&total[0], 0, sizeof(TimerTicks));
    memset(&total[1], 0, sizeof(TimerTicks));
    CLT_ClearFrameBuffer(gs, 0, 0, 0, 0, TRUE, FALSE);
    GS_SendList(gs);
    GS_WaitIO(gs);
    Blt_MoveVertices(bdropBO->bo_vertices, -*BLITVERTEX_X(bdropBO->bo_vertices, 0), 0);
    height = bdropBO->bo_txdata->btd_txData.minY;
    for (i = 0; i < height; i++)
    {
        SampleSystemTimeTT(&in[0]);
        Blt_MoveVertices(bdropBO->bo_vertices, 0, -1.0);
        Blt_BlitObjectToBitmap(gs, bdropBO, bitmaps[0], 0);
        SampleSystemTimeTT(&in[1]);
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        SubTimerTicks(&in[0], &out, &difference[0]); /* difference = out - in */
        AddTimerTicks(&difference[0], &total[0], &total[0]);
        SubTimerTicks(&in[1], &out, &difference[1]); /* difference = out - in */
        AddTimerTicks(&difference[1], &total[1], &total[1]);
    }
    ConvertTimerTicksToTimeVal(&total[0], &tv);
    printf("Clipped move BlitObject up %ld times took %d.%06d seconds\n", height, tv.tv_Seconds,tv.tv_Microseconds);
    ConvertTimerTicksToTimeVal(&total[1], &tv);
    printf("(Render time took %d.%06d seconds)\n", tv.tv_Seconds,tv.tv_Microseconds);
    SubTimerTicks(&total[1], &total[0], &difference[0]);
    ConvertTimerTicksToTimeVal(&difference[0], &tv);
    printf("Setup time took %d.%06d seconds\n\n", tv.tv_Seconds,tv.tv_Microseconds);


  cleanup:
    Blt_FreeUTF(bdropBO);
    Spr_Delete(bdropSO);
    DeleteItem(viewI);
    GS_FreeBitmaps(bitmaps, NUM_TBUFFS);
    GS_FreeLists(gs);
    GS_Delete(gs);
    DeleteTimerIOReq(vbl);
    CloseBlitterFolio();
    
    printf("timings done\n");
    
    return(0);
}
 
