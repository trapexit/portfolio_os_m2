/* @(#) simple.c 96/09/16 1.6 */
/* This code runs through a series of simple blit tests. */

#include <kernel/types.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/blitter.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <misc/event.h>
#include <kernel/time.h>
#include <stdio.h>
#include <string.h>
#include "testblit.h"

void WaitForAnyButton(ControlPadEventData *cped);

#define CKERR(x, y) {if(x < 0){printf("Error - 0x%lx %s\n", x, y);PrintfSysErr(x); goto cleanup;}}

void DrawBox(GState *gs, Box2 *box)
{
    Color4 col;
    Point2 pt[2];

    col.r = col.a = 1.0; col.g = col.b = 0.0;

    pt[0].x = box->min.x;
    pt[0].y = box->min.y;
    pt[1].x = box->max.x;
    pt[1].y = box->min.y;
    F2_DrawLine(gs, &pt[0], &pt[1], &col, &col);
    pt[0] = pt[1];
    pt[1].x = box->max.x;
    pt[1].y = box->max.y;
    F2_DrawLine(gs, &pt[0], &pt[1], &col, &col);
    pt[0] = pt[1];
    pt[1].x = box->min.x;
    pt[1].y = box->max.y;
    F2_DrawLine(gs, &pt[0], &pt[1], &col, &col);
    pt[0] = pt[1];
    pt[1].x = box->min.x;
    pt[1].y = box->min.y;
    F2_DrawLine(gs, &pt[0], &pt[1], &col, &col);
}

#define W 60
#define H 60
#define X 60
#define Y 60
#define LOOPCOUNT loopCount

void doSimpleTest(GState *gs, Item *bm, ControlPadEventData *cped, BlitObject  *bdrop, Item viewI, Item vbl)
{
    BlitObject *bo = NULL;
    BlitObject *bo1 = NULL;
    BlitRect br =
    {
        {
            X, Y,
        },
        {
            (X + W - 1), (Y + H - 1),
        },
    };
    BlitRect bbr;
    gfloat vertices[] =
    {
        0, (Y + H), W_2D, 0, 0,
        (W - 1), (Y + H), W_2D, (W - 1), 0,
        (W - 1), ((Y + H) + (H - 1)), W_2D, (W - 1), (H - 1),
        0, ((Y + H) + (H - 1)), W_2D, 0, (H - 1),
    };
    gfloat verticesC[] =
    {
        130, 90, W_2D, 0, 0,
        190, 90, W_2D, W, 0,
        190, 150, W_2D, W, H,
        130, 150, W_2D, 0, H,
    };
    int32 i;
    uint32 visible = 0;
    Err err;
        /* Timing stuff */
    TimeVal tv;
    TimerTicks in, out, difference, total;
    uint32 loopCount;

    printf("A couple of simple tests...\n\n");

    bbr = br;
    bbr.min.x--;
    bbr.min.y--;
    bbr.max.x++;
    bbr.max.y++;

    GS_SetDestBuffer(gs, bm[0]);
    Blt_BlitObjectToBitmap(gs, bdrop, bm[0], 0);
    DrawBox(gs, &bbr);
    GS_SendList(gs);
    GS_WaitIO(gs);

    err = Blt_CreateBlitObject(&bo, NULL);
    CKERR(err, "Could not create BlitObject");
    err = Blt_RectangleToBlitObject(NULL, bo, bm[0], &br);
    CKERR(err, "Blt_RectangleToBlitObject");
    err = Blt_SetVertices(bo->bo_vertices, vertices);
    CKERR(err, "Blt_SetVertices()\n");
    err = Blt_BlitObjectToBitmap(gs, bo, bm[0], SKIP_SNIPPET_PIP);
    CKERR(err, "Blt_BlitObjectToBitmap");
    GS_SendList(gs);
    GS_WaitIO(gs);
    WaitForAnyButton(cped);

    {
        /* Make a copy of this BlitObject, but this time render the info
         * smaller, somewhere else, and enable dblending.
         */
        gfloat vertices1[] =
        {
            210, 90, W_2D, 0, 0,
            230, 90, W_2D, 61, 0,
            230, 150, W_2D, 61, 61,
            210, 150, W_2D, 0, 61,
        };

        err = Blt_CreateBlitObjectVA(&bo1,
                                     BLIT_TAG_TBLEND, bo->bo_tbl,
                                     BLIT_TAG_TXLOAD, bo->bo_txl,
                                     BLIT_TAG_PIP, bo->bo_pip,
                                     BLIT_TAG_TXDATA, bo->bo_txdata,
                                     TAG_END);
        CKERR(err, "Could not create BlitObject");
        err = Blt_CopySnippet(bo, bo1, NULL, BLIT_TAG_DBLEND);
        CKERR(err, "CopySnippet:");
        err = Blt_SetVertices(bo1->bo_vertices, vertices1);
        CKERR(err, "Blt_SetVertices()\n");
        bo1->bo_dbl->dbl_userGenCntl = CLA_DBUSERCONTROL (0, 0, 0, 0, 1, 1, 0,
                                                          (CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, RED) |
                                                           CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, GREEN) |
                                                           CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, BLUE) |
                                                           CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, ALPHA)));
        err = Blt_BlitObjectToBitmap(gs, bo1, bm[0], SKIP_SNIPPET_PIP);
        CKERR(err, "Blt_BlitObjectToBitmap");
        GS_SendList(gs);
        GS_WaitIO(gs);
        WaitForAnyButton(cped);
        Blt_DeleteBlitObject(bo1);
        bo1 = NULL;
    }

    Blt_MoveVertices(bo->bo_vertices, 0, 1.0);
    err = Blt_BlitObjectToBitmap(gs, bo, bm[0], SKIP_SNIPPET_PIP);
    CKERR(err, "Blt_BlitObjectToBitmap");
    GS_SendList(gs);
    GS_WaitIO(gs);
    WaitForAnyButton(cped);

    memset(&total, 0, sizeof(TimerTicks));
    LOOPCOUNT = 0;
    for (i = 1; i < (FBWIDTH - W); i++)
    {
        SampleSystemTimeTT(&in);
        Blt_MoveVertices(bo->bo_vertices, 1.0, 0);
        err = Blt_BlitObjectToBitmap(gs, bo, bm[0],
                                     (SKIP_SNIPPET_PIP | SKIP_SNIPPET_TXLOAD | SKIP_SNIPPET_TBLEND | SKIP_SNIPPET_DBLEND));

        CKERR(err, "Blt_BlitObjectToBitmap");
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        SubTimerTicks(&in, &out, &difference); /* difference = out - in */
        AddTimerTicks(&difference, &total, &total);
        LOOPCOUNT++;
        WaitTimeVBL(vbl, 0);
    }
    ConvertTimerTicksToTimeVal(&total, &tv);
    printf("Move and draw %d times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);

    /* Do same doubleBuffered */
    memset(&total, 0, sizeof(TimerTicks));
    LOOPCOUNT = 0;
    for (i = 0; i < (FBWIDTH - W); i++)
    {
        visible ^= 1;
        GS_SetDestBuffer(gs, bm[visible]);
        Blt_BlitObjectToBitmap(gs, bdrop, bm[0], 0);
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&in);
        Blt_MoveVertices(bo->bo_vertices, -1.0, 0);
        err = Blt_BlitObjectToBitmap(gs, bo, bm[visible], SKIP_SNIPPET_PIP);
        CKERR(err, "Blt_BlitObjectToBitmap");
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        SubTimerTicks(&in, &out, &difference); /* difference = out - in */
        AddTimerTicks(&difference, &total, &total);
        LOOPCOUNT++;
        ModifyGraphicsItemVA(viewI,
                             VIEWTAG_BITMAP, bm[visible],
                             TAG_END);
        WaitTimeVBL(vbl, 0);
    }
    ConvertTimerTicksToTimeVal(&total, &tv);
    printf("Move and draw all %d times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);

        /* Put back in the middle of the screen, and rotate it */
    Blt_SetVertices(bo->bo_vertices, verticesC);
    CKERR(err, "Blt_SetVertices()\n");
    for (i = 0; i < 360; i++)
    {
        visible ^= 1;
        GS_SetDestBuffer(gs, bm[visible]);
        Blt_BlitObjectToBitmap(gs, bdrop, bm[0], 0);
        Blt_RotateVertices(bo->bo_vertices, 1.0, (FBWIDTH / 2), (FBHEIGHT / 2));
        err = Blt_BlitObjectToBitmap(gs, bo, bm[visible], SKIP_SNIPPET_PIP);
        CKERR(err, "Blt_BlitObjectToBitmap");
        GS_SendList(gs);
        GS_WaitIO(gs);
        ModifyGraphicsItemVA(viewI,
                             VIEWTAG_BITMAP, bm[visible],
                             TAG_END);
        WaitTimeVBL(vbl, 0);
    }
    WaitForAnyButton(cped);

  cleanup:
        Blt_DeleteBlitObject(bo);
        Blt_DeleteBlitObject(bo1);

    return;
}
