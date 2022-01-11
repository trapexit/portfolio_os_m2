/* @(#) mask.c 96/08/23 1.5 */
/* This code tests the blit-thru-mask features. */

#include <kernel/types.h>
#include <graphics/blitter.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/frame2d/spriteobj.h>
#include <misc/event.h>
#include <stdio.h>
#include "testblit.h"

void WaitForAnyButton(ControlPadEventData *cped);

uint32 mask[BOXHEIGHT] =
{
    0, 0, 0,
    0xf000000f,
    0x7800001e,
    0x3c00003c,
    0x1e000078,
    0x0f0000f0,
    0x078001e0,
    0x03c003c0,
    0x01e00780,
    0x00f00f00,
    0x00781e00,
    0x003c3c00,
    0x001e7800,
    0x000ff000,
    0x000ff000,
    0x001e7800,
    0x003c3c00,
    0x00781e00,
    0x00f00f00,
    0x01e00780,
    0x03c003c0,
    0x078001e0,
    0x0f0000f0,
    0x1e000078,
    0x3c00003c,
    0x7800001e,
    0xf000000f,
    0, 0, 0,
};

#define W 90
#define H 90
#define X 60
#define Y 60

typedef struct discardType
{
    uint32 discard;
    char *string;
} discardType;
#define DISCARD_COUNT 3
discardType discardTypes[DISCARD_COUNT] =
{
    {
        FV_DBDISCARDCONTROL_SSB0_MASK,
        "DISCARD SSB == 0",
    },
    {
        FV_DBDISCARDCONTROL_ALPHA0_MASK,
        "DISCARD ALPHA == 0",
    },
    {
        FV_DBDISCARDCONTROL_RGB0_MASK,
        "DISCARD RGB == 0",
    },
};

#define TEST_COUNT 16

        
void doMaskTest(GState *gs, Item *bm, ControlPadEventData *cped, BlitObject  *bdrop, Item viewI)
{
    BlitMask bmask;
    gfloat vertices[] =
    {
        X, Y, W_2D, 0, 0,
        X + W - 1, Y, W_2D, W - 1, 0,
        X + W - 1, Y + H - 1, W_2D, W - 1, H - 1,
        X, Y + H - 1, W_2D, 0, H - 1,
    };
    BlitRect br =
    {
        {
            (X + W + 20), Y,
        },
        {
            (X + W + 20 + W - 1), Y + H - 1,
        }
    };
    BlitObject *bo;
    Err err;
    uint32 visible = 0;
    uint32 i, j;

    err = Blt_CreateBlitObject(&bo, NULL);
    if (err < 0)
    {
        printf("No blit object\n");
        PrintfSysErr(err);
        return;
    }
    
    bmask.blm_width = BOXWIDTH;
    bmask.blm_height = BOXHEIGHT;
    bmask.blm_data = mask;

    printf("Run through all combinations of mask type\n\n");
    
    for (i = 0; i < DISCARD_COUNT; i++)
    {
        printf("%s\n", discardTypes[i].string);
        bmask.blm_discardType = discardTypes[i].discard;

        for (j = 0; j < TEST_COUNT; j++)
        {
            if (j & FLAG_BLM_INVERT)
            {
                printf("Invert ");
            }
            if (j & FLAG_BLM_REPEAT)
            {
                printf("Repeat ");
            }
            if (j & FLAG_BLM_CENTER)
            {
                printf("Center ");
            }
            if (j & FLAG_BLM_FORCE_VISIBLE)
            {
                printf("ForceVisible ");
            }
            if (j == 0)
            {
                printf("Normal ");
            }
            printf("\n");
            
            bmask.blm_flags = j;
            
            Blt_SetVertices(bo->bo_vertices, vertices);
            GS_SetDestBuffer(gs, bm[visible]);
            Blt_BlitObjectToBitmap(gs, bdrop, bm[visible], 0);
            GS_SendList(gs);
            GS_WaitIO(gs);
            Blt_RectangleToBlitObject(NULL, bo, bm[visible], &br);
            err = Blt_MakeMask(bo, &bmask);
            if (err >= 0)
            {
                err = Blt_EnableMask(bo, bmask.blm_discardType);
                if (err >= 0)
                {
                    Blt_BlitObjectToBitmap(gs, bo, bm[visible], 0);
                    GS_SendList(gs);
                    GS_WaitIO(gs);
                    ModifyGraphicsItemVA(viewI,
                                         VIEWTAG_BITMAP, bm[visible],
                                         TAG_END);
                    WaitForAnyButton(cped);
                }
                else
                {
                    printf("Could not enable mask\n");
                    PrintfSysErr(err);
                }
            }
            else
            {
                printf("Could not make mask\n");
                PrintfSysErr(err);
            }
        }
    }
    
    Blt_DeleteBlitObject(bo);
    return;
}

