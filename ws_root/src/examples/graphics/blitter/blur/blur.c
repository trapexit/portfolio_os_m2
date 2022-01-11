/* @(#) blur.c 96/08/23 1.3
/* This code blurs an area of an image in a bitmap */

#include <kernel/types.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/blitter.h>
#include <graphics/view.h>
#include <misc/event.h>
#include <kernel/mem.h>
#include <stdio.h>
#include <stdlib.h>

#define CKERR(x, y) {if(x < 0){printf("Error - 0x%lx %s\n", x, y);PrintfSysErr(x); goto cleanup;}}
#define ERR(s) {printf((s)); goto cleanup;}
#define CKNULL(x, y) {if(x == NULL){printf("Error - %s\n", x, y); goto cleanup;}}

#define FBWIDTH 320
#define FBHEIGHT 240
#define DEPTH 32
#define NUM_FBUFFS 1
#define NUM_ZBUFFS 0
#define NUM_TBUFFS (NUM_FBUFFS + NUM_ZBUFFS)
#define BOXWIDTH (FBWIDTH - 2)
#define BOXHEIGHT (FBHEIGHT - 2)
#define LEFTEDGE ((FBWIDTH - BOXWIDTH) / 2)
#define TOPEDGE ((FBHEIGHT - BOXHEIGHT) / 2)
#define RESLICE  \
{                                               \
    if (Blt_BlitObjectSliced(bo))               \
    {                                           \
        GS_SendList(gs);                        \
        GS_WaitIO(gs);                          \
    }                                               \
}

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

void WaitForAnyButton(ControlPadEventData *cped)
{
    GetControlPad(1, TRUE, cped); /* Button down */
    GetControlPad(1, TRUE, cped); /* Button up */
}

int main(int argc, char *argv[])
{
    GState *gs = NULL;
    Item bitmaps[NUM_TBUFFS];
    ControlPadEventData cped;
    Item viewI = 0;
    uint32 bmType = ((DEPTH == 16) ? BMTYPE_16 : BMTYPE_32);
    const uint32 viewType = BestViewType(FBWIDTH, FBHEIGHT, DEPTH);
    BlitObject *bdrop = NULL;
    BlitObject *bo = NULL;
    BlitScroll bsc;
    Err err;

    if (argc != 2)
    {
        printf("Usage: blur <backdrop utf file>\n");
        exit(0);
    }
    
         /* Initialize the EventBroker. */
    if (InitEventUtility(1, 0, LC_ISFOCUSED) < 0 )
    {
        ERR("InitEvenUtility\n");
    }
    
    OpenGraphicsFolio();
    OpenBlitterFolio();
    gs = GS_Create();
    if (gs == NULL)
    {
        ERR("Could not create GState\n");
    }
    
    GS_AllocLists(gs, 2, 2048 * 4);
    err = GS_AllocBitmaps(bitmaps, FBWIDTH, FBHEIGHT, bmType, NUM_FBUFFS, NUM_ZBUFFS);
    CKERR(err, "Can't GS_AllocBitmaps()!\n"); 
#if (NUM_ZBUFFS != 0)
    GS_SetZBuffer(gs, bitmaps[NUM_FBUFFS]); /* Use the last buffer as the Z buffer */
#endif
    viewI = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
                         VIEWTAG_VIEWTYPE, viewType,
                         VIEWTAG_BITMAP, bitmaps[0],
                         TAG_END );
    CKERR(viewI, "can't create view item");
    
    err = Blt_LoadUTF(&bdrop, argv[1]);
    CKERR(err, "Could not load utf file");
    
    /**********************************************************************************/

    err = Blt_CreateBlitObject(&bo, NULL);
    CKERR(err, "CreateBlitObject()");
    
        /* Initialise the BlitScroll structure with the area to
         * blur. We need this to be one pixel smaller around the
         * edges.
         */
    bsc.bsc_region.min.x = (LEFTEDGE + 1);
    bsc.bsc_region.min.y = (TOPEDGE + 1);
    bsc.bsc_region.max.x = (LEFTEDGE + BOXWIDTH - 1);
    bsc.bsc_region.max.y = (TOPEDGE + BOXHEIGHT - 1);
    bsc.bsc_replaceColor = 0;

    GS_SetDestBuffer(gs, bitmaps[0]);
    Blt_BlitObjectToBitmap(gs, bdrop, bitmaps[0], 0);
    GS_SendList(gs);
    GS_WaitIO(gs);
    AddViewToViewList(viewI, 0);
    WaitForAnyButton(&cped);

        /* Now the actual blur operation.
         *
         * Take the region -1 pixel around the edge, and blend it with itself
         * offset by 1 pixel in all eight directions.
         */
    err = Blt_BlendBlitObject(bo, 0.125, 0.875);
    CKERR(err, "Blt_BlendBlitObject()");
    
    bsc.bsc_dx = -1;
    bsc.bsc_dy = 0;
    err = Blt_Scroll(gs, bo, bitmaps[0], &bsc);
    CKERR(err, "Blt_Scroll() 1");
    RESLICE;
    bsc.bsc_dx = 0;
    bsc.bsc_dy = -1;
    err = Blt_Scroll(gs, bo, bitmaps[0], &bsc);
    CKERR(err, "Blt_Scroll() 2");
    RESLICE;
    bsc.bsc_dx = 1;
    bsc.bsc_dy = 0;
    err = Blt_Scroll(gs, bo, bitmaps[0], &bsc);
    CKERR(err, "Blt_Scroll() 3");
    bsc.bsc_dx = 1;
    bsc.bsc_dy = 0;
    err = Blt_Scroll(gs, bo, bitmaps[0], &bsc);
    CKERR(err, "Blt_Scroll() 4");
    RESLICE;
    bsc.bsc_dx = 0;
    bsc.bsc_dy = 1;
    err = Blt_Scroll(gs, bo, bitmaps[0], &bsc);
    CKERR(err, "Blt_Scroll() 5");
    bsc.bsc_dx = 0;
    bsc.bsc_dy = 1;
    err = Blt_Scroll(gs, bo, bitmaps[0], &bsc);
    CKERR(err, "Blt_Scroll() 6");
    RESLICE;
    bsc.bsc_dx = -1;
    bsc.bsc_dy = 0;
    err = Blt_Scroll(gs, bo, bitmaps[0], &bsc);
    CKERR(err, "Blt_Scroll() 7");
    bsc.bsc_dx = -1;
    bsc.bsc_dy = 0;
    err = Blt_Scroll(gs, bo, bitmaps[0], &bsc);
    CKERR(err, "Blt_Scroll() 8");
    GS_SendList(gs);
    GS_WaitIO(gs);
    WaitForAnyButton(&cped);    
    
  cleanup:
    Blt_DeleteBlitObject(bo);
    Blt_FreeUTF(bdrop);
    DeleteItem(viewI);
    GS_FreeBitmaps(bitmaps, NUM_TBUFFS);
    GS_FreeLists(gs);
    GS_Delete(gs);
    KillEventUtility();
    CloseBlitterFolio();
    
    printf("blur done\n");
    
    return(0);
}
 
