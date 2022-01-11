/* @(#) circle.c 96/09/16 1.11 */
/* This code generates the vertices of a circle (approximately), maps
 * a section of the screen onto the circle, and then performs a number
 * of operations on the circle under user control.
 */

#include <kernel/types.h>
#include <graphics/blitter.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/frame2d/spriteobj.h>
#include <misc/event.h>
#include <stdio.h>
#include <math.h>
#include "testblit.h"

#define POINTS 16
#define RADIUS_X 60
#define RADIUS_Y RADIUS_X
#define CX (FBWIDTH / 2)
#define CY (FBHEIGHT / 2)
#define BLEND_RATE 0.05
void MatrixRotate(BlitMatrix fm, gfloat angle, Point2*center);
void MatrixScale(BlitMatrix fm, gfloat scale, Point2 *center);
void MatrixTranslate(BlitMatrix fm, gfloat dx, gfloat dy);
void WaitForAnyButton(ControlPadEventData *cped);

/* Set USE_MASK to 1 to blit through a mask */
#define USE_MASK 0
#if USE_MASK
extern uint32 mask[BOXHEIGHT];
#endif

void doCircleTest(GState *gs, Item *bm, ControlPadEventData *cped, BlitObject *bdrop, Item viewI, Item vbl)
{
    gfloat *vertices;
    gfloat *next;
    VerticesSnippet *vtx;
    BlitObject *bo;
    gfloat i;
    uint32 visible = 0;
    uint32 count;
    uint32 dBuff;
    uint32 ugc;
    gfloat blendRatio = 0.0;
    gfloat blendRate = BLEND_RATE;
    bool blend;
    bool clipped;
    gfloat r;
    BlitMatrix tran;
    Err err;
    BlitRect br =
    {
        {
            (CX - RADIUS_X), (CY - RADIUS_Y),
        },
        {
            (CX + RADIUS_X), (CY + RADIUS_Y),
        },
    };
    
#if USE_MASK
    BlitMask bmask;

    bmask.blm_discardType = FV_DBDISCARDCONTROL_SSB0_MASK;
    bmask.blm_width = BOXWIDTH;
    bmask.blm_height = BOXHEIGHT;
    bmask.blm_data = mask;
    bmask.blm_flags =
        (FLAG_BLM_REPEAT | FLAG_BLM_CENTER | FLAG_BLM_INVERT | FLAG_BLM_FORCE_VISIBLE);
#endif

    err = Blt_CreateVertices(&vtx,
                             CLA_TRIANGLE(1, 1, 1, 1, 0, (POINTS + 2)));
    if (err < 0)
    {
        PrintfSysErr(err);
        return;
    }
    vertices = &vtx->vtx_vertex[0];

    /* Build the vertices */
    next = vertices;
    /* Start with the center point */
    *next++ = CX;  /* x */
    *next++ = CY;  /* y */
    *next++ = W_2D;             /* w */
    *next++ = RADIUS_X;  /* u */
    *next++ = RADIUS_Y;  /* v */
    count = 0;
    for (i = 0.0; i <= 360.0; i += (360.0 / POINTS))
    {
        count++;
        *next++ = (CX + (RADIUS_X * cosf(i * (PI / 180.0))));  /* x */
        *next++ = (CY + (RADIUS_Y * sinf(i * (PI / 180.0))));   /* y */
        *next++ = W_2D;             /* w */

        if ((i >= 0) && (i < 45))
        {
            *next++ = (RADIUS_X * 2);
            r = ((gfloat)RADIUS_Y + ((gfloat)RADIUS_Y * ((gfloat)i / 45.0)));
            *next++ = r;
        }
        else if ((i >= 45) && (i < 135))
        {
            r = ((RADIUS_X * 2.0) - ((RADIUS_X * 2.0) * ((i - 45.0) / 90.0)));
            *next++ = r;
            *next++ = (RADIUS_Y * 2);
        }
        else if ((i >= 135) && (i < 225))
        {
            *next++ = 0;
            r = ((RADIUS_Y * 2) - ((RADIUS_Y * 2.0) * ((i - 135.0) / 90.0)));
            *next++ = r;
        }
        else if ((i >= 225) && (i < 315))
        {
            r = ((RADIUS_X * 2.0) * ((i - 225.0) / 90.0));
            *next++ = r;
            *next++ = 0;
        }
        else
        {
            *next++ = (RADIUS_X * 2);
            r = ((RADIUS_Y * 2.0) * ((i - 315.0) / 90.0));
            *next++ = r;
        }
    }
    GS_SetDestBuffer(gs, bm[visible]);
    Blt_BlitObjectToBitmap(gs, bdrop, bm[visible], 0);
    GS_SendList(gs);
    GS_WaitIO(gs);
    ModifyGraphicsItemVA(viewI,
                         VIEWTAG_BITMAP, bm[visible],
                         TAG_END);

    err = Blt_CreateBlitObjectVA(&bo,
                                 BLIT_TAG_VERTICES, vtx,
                                 TAG_END);
    if (err >= 0)
    {
        cped->cped_ButtonBits = 0;
#if USE_MASK
        Blt_RectangleToBlitObject(NULL, bo, bm[visible], &br);
        Blt_MakeMask(bo, &bmask);
        Blt_EnableMask(bo, bmask.blm_discardType);
#else
        /* See if we can avoid the intermediate step of blitting
         * the bitmap area into a buffer.
         */
        err = Blt_RectangleInBitmap(NULL, bo, bm[visible], &br);
        if (err == BLITTER_ERR_TOOBIG)
        {
            /* No can do. The area will not fit in the TRAM, so we have
             * to blit the rectangle into a buffer and let the blitter folio
             * slice the texture and vertices for us.
             */
            Blt_RectangleToBlitObject(NULL, bo, bm[visible], &br);
        }
#endif
        dBuff = 1;
        blend = FALSE;
        clipped = FALSE;
        ugc = bo->bo_dbl->dbl_userGenCntl;

        printf("Move the circle around the screen with the control pad:\n");
        printf("\tDirection pad moves the circle.\n");
        printf("\tShift buttons rotate the circle.\n");
        printf("\tA - Toggle double buffering and screen refreshing.\n");
        printf("\tB - Toggle destination blending on/off\n");
        printf("\tC - Change blending ratio by 5%\n");
        printf("\tPause - toggle clipping. A clip region is defined around the circle.\n");
        printf("\tPause + Left Shift - clip away pixels inside the clip region.\n");
        printf("\tX - exit\n\n");

        while (!(cped->cped_ButtonBits & ControlX))
        {
            bool transform = FALSE;
            
                /* Initialise the matrix to the Identity */
            tran[0][0] = 1.0; tran[0][1] = 0.0;
            tran[1][0] = 0.0; tran[1][1] = 1.0;
            tran[2][0] = 0.0; tran[2][1] = 0.0;

            if (cped->cped_ButtonBits & ControlStart)
            {
                bool clipIn = TRUE;
                
                if (cped->cped_ButtonBits & ControlLeftShift)
                {
                    clipIn = FALSE;
                }
                clipped = !clipped;
                Blt_SetClipBox(bo, clipped, clipIn, &br.min, &br.max);
                WaitForAnyButton(cped);
            }   
            if ((cped->cped_ButtonBits & ControlLeft) && (br.min.x > 0))
            {
                br.min.x--;
                br.max.x--;
                MatrixTranslate(tran, -1.0, 0);
                transform = TRUE;
            }
            if ((cped->cped_ButtonBits & ControlRight) && (br.max.x < FBWIDTH))
            {
                br.min.x++;
                br.max.x++;
                MatrixTranslate(tran, 1.0, 0);
                transform = TRUE;
            }
            if ((cped->cped_ButtonBits & ControlUp) && (br.min.y > 0))
            {
                br.min.y--;
                br.max.y--;
                MatrixTranslate(tran, 0, -1.0);
                transform = TRUE;
            }
            if ((cped->cped_ButtonBits & ControlDown) && (br.max.y < FBHEIGHT))
            {
                br.min.y++;
                br.max.y++;
                MatrixTranslate(tran, 0, 1.0);
                transform = TRUE;
            }
            if (cped->cped_ButtonBits & ControlLeftShift)
            {
                Point2 pt;
                pt.x = (br.min.x + RADIUS_X);
                pt.y = (br.min.y + RADIUS_Y);
                MatrixRotate(tran, -1.0, &pt);
                transform = TRUE;
            }
            if (cped->cped_ButtonBits & ControlRightShift)
            {
                Point2 pt;
                pt.x = (br.min.x + RADIUS_X);
                pt.y = (br.min.y + RADIUS_Y);
                MatrixRotate(tran, 1.0, &pt);
                transform = TRUE;
            }
            if (cped->cped_ButtonBits & ControlA)
            {
                dBuff ^= 1;
                GetControlPad(1, TRUE, cped);
            }
            if (cped->cped_ButtonBits & ControlB)
            {
                if (blend)
                {
                    bo->bo_dbl->dbl_userGenCntl = ugc;
                }
                else
                {
                    bo->bo_dbl->dbl_userGenCntl = CLA_DBUSERCONTROL (0, 0, 0, 0, 1, 1, 0,
                                                                     (CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, RED) |
                                                                      CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, GREEN) |
                                                                      CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, BLUE) |
                                                                      CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, ALPHA)));
                }
                blend = !blend;
                GetControlPad(1, TRUE, cped);
            }
            if (transform)
            {
                Blt_TransformVertices(vtx, tran);
            }
            
            GS_SetDestBuffer(gs, bm[visible]);
            if (dBuff)
            {
                Blt_BlitObjectToBitmap(gs, bdrop, bm[visible], 0);
                GS_SendList(gs);
                GS_WaitIO(gs);
            }
#if USE_MASK
            Blt_RectangleToBlitObject(gs, bo, bm[visible], &br);
            GS_SendList(gs);
            GS_WaitIO(gs);
            GS_SetDestBuffer(gs, bm[visible]);
            Blt_MakeMask(bo, &bmask);
            Blt_EnableMask(bo, bmask.blm_discardType);
#endif
            if (cped->cped_ButtonBits & ControlC)
            {
                blendRatio += blendRate;
                if (blendRatio > 1.0)
                {
                    blendRatio = 1.0;
                    blendRate *= -1;
                }
                else if (blendRatio < 0.0)
                {
                    blendRatio = 0.0;
                    blendRate *= -1;
                }
                err = Blt_BlendBlitObject(bo, blendRatio, (1 - blendRatio));
                if (err < 0) PrintfSysErr(err);
                blend = TRUE;
                GetControlPad(1, TRUE, cped);
            }
            
#if USE_MASK
            Blt_BlitObjectToBitmap(gs, bo, bm[visible], 0);
#else
            err = Blt_RectangleInBitmap(gs, bo, bm[visible], &br);
            if (err == BLITTER_ERR_TOOBIG)
            {
                    /* Blit this part of the bitmap into the BltObject's
                     * buffer.
                     */
                Blt_RectangleToBlitObject(gs, bo, bm[visible], &br); 
                    /* Blt_RectnalgeToBlitObject() used the GState list to
                     * blit the data, and also called GS_SetDestBuffer().
                     *
                     * We must therefore execute the TE instructions and
                     * set the destination buffer back again before blitting
                     * back out.
                     */
                GS_SendList(gs);
                GS_WaitIO(gs);
                GS_SetDestBuffer(gs, bm[visible]);
                Blt_BlitObjectToBitmap(gs, bo, bm[visible], 0);
            }
#endif
            GS_SendList(gs);
            GS_WaitIO(gs);
            ModifyGraphicsItemVA(viewI,
                                 VIEWTAG_BITMAP, bm[visible],
                                 TAG_END);
            WaitTimeVBL(vbl, 1);
            visible ^= dBuff;
            
            GetControlPad(1, FALSE, cped);
        }
        Blt_DeleteBlitObject(bo);
    }

    Blt_DeleteVertices(vtx);
}


