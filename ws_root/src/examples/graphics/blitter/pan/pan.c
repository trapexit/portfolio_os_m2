/* @(#) pan.c 96/10/22 1.4 */
/* This code pans a 640x480 bitmap around a 320x240 screen */

#include <kernel/types.h>
#include <kernel/task.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/clt/cltmacros.h>
#include <graphics/blitter.h>
#include <graphics/view.h>
#include <misc/event.h>
#include <kernel/mem.h>
#include <stdio.h>
#include <stdlib.h>

#define CKERR(x, y) {if(x < 0){printf("Error - 0x%lx %s\n", x, y);PrintfSysErr(x); goto cleanup;}}
#define ERR(s) {printf((s)); goto cleanup;}
#define CKNULL(x, y) {if(x == NULL){printf("Error - %s\n", x, y); goto cleanup;}}

#define FBWIDTH 640
#define FBHEIGHT 480
#define VWIDTH 320
#define VHEIGHT 240
#define DEPTH 32
#define NUM_FBUFFS 2
#define NUM_ZBUFFS 0
#define NUM_TBUFFS (NUM_FBUFFS + NUM_ZBUFFS)
#define SCROLL_DELTA 4

void WaitForAnyButton(ControlPadEventData *cped)
{
    GetControlPad(1, TRUE, cped); /* Button down */
    GetControlPad(1, TRUE, cped); /* Button up */
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

int main(int argc, char *argv[])
{
    GState *gs = NULL;
    Item bitmaps[NUM_TBUFFS];
    ControlPadEventData cped;
    Item viewI = 0;
    Item vbl = 0;
    uint32 bmType = ((DEPTH == 16) ? BMTYPE_16 : BMTYPE_32);
    const uint32 viewType = BestViewType(VWIDTH, VHEIGHT, DEPTH);
    BlitObject *bdrop = NULL;
    uint32 bytesPerRow;
    uint32 bytesPerPixel;
    char *currentAddress;
    void *texelData;
    uint32 visible = 0;
    int32 signal = 0;
    Err err;
    gfloat vertices[] =
    {
        0, 0, W_2D, 0, 0,
        VWIDTH, 0, W_2D, VWIDTH, 0,
        VWIDTH, VHEIGHT, W_2D, VWIDTH, VHEIGHT,
        0, VHEIGHT, W_2D, 0, VHEIGHT,
    };
    

    if (argc != 2)
    {
        printf("Usage: pan <640x480 backdrop utf file>\n");
        exit(0);
    }
    
         /* Initialize the EventBroker. */
    if (InitEventUtility(1, 0, LC_ISFOCUSED) < 0 )
    {
        ERR("InitEvenUtility\n");
    }
    vbl = CreateTimerIOReq();
    signal = AllocSignal(0);
    if (signal == ILLEGALSIGNAL)
    {
        printf("Bad signal\n");
        goto cleanup;
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
                         VIEWTAG_BITMAP, bitmaps[visible],
                         VIEWTAG_PIXELWIDTH, VWIDTH,
                         VIEWTAG_PIXELHEIGHT, VHEIGHT,
                         VIEWTAG_DISPLAYSIGNAL, signal,
                         TAG_END );
    CKERR(viewI, "can't create view item");

    err = GS_SetVidSignal(gs, signal);
    CKERR(err, "SetVidSignal");
    
    err = Blt_LoadUTF(&bdrop, argv[1]);
    CKERR(err, "Could not load utf file");
    err = Blt_GetTexture(bdrop, &texelData);
    CKERR(err, "Could not get texture address");
    
    /**********************************************************************************/
    
    GS_SetDestBuffer(gs, bitmaps[visible]);
    AddViewToViewList(viewI, 0);
            
    bytesPerPixel = (bdrop->bo_txdata->btd_txData.bitsPerPixel / 8);
    bytesPerRow = (bdrop->bo_txdata->btd_txData.minX * bytesPerPixel);
    
    err = Blt_SetVertices(bdrop->bo_vertices, vertices);
    CKERR(err, "Blt_SetVertices()\n");
        /* Render the image back out to the framebuffer once.
         * This also sets up all the TE hardware registers, so
         * that we can do a more optimised blit in the main loop.
         */
    err = Blt_BlitObjectToBitmap(gs, bdrop, bitmaps[0], 0);
    CKERR(err, "BlitObjectToBitmap");
    GS_SendList(gs);
    GS_WaitIO(gs);
    err = Blt_GetTexture(bdrop, (void **)&currentAddress);
    CKERR(err, "GetTexture");
    
    cped.cped_ButtonBits = 0;
    GS_BeginFrame(gs);
    while (!(cped.cped_ButtonBits & ControlX))
    {
        GetControlPad(1, FALSE, &cped);

        if (cped.cped_ButtonBits & ControlUp)
        {
            currentAddress -= (bytesPerRow * SCROLL_DELTA);
        }
        if (cped.cped_ButtonBits & ControlDown)
        {
            currentAddress += (bytesPerRow * SCROLL_DELTA);
        }
        if (cped.cped_ButtonBits & ControlLeft)
        {
            currentAddress -= (bytesPerPixel * SCROLL_DELTA);
        }
        if (cped.cped_ButtonBits & ControlRight)
        {
            currentAddress += (bytesPerPixel * SCROLL_DELTA);
        }
        if (cped.cped_ButtonBits & (ControlUp | ControlDown | ControlLeft | ControlRight))
        {
            GS_SetDestBuffer(gs, bitmaps[visible ^ 1]);
            err = Blt_SetTexture(bdrop, (void *)currentAddress);
            CKERR(err, "SetTexture");
                /* Note a slightly optimised blit: we don't need to set the
                 * DBLend, TBlend or PIP registers again.
                 */
            err = Blt_BlitObjectToBitmap(gs, bdrop, bitmaps[visible ^ 1],
                                         SKIP_SNIPPET_DBLEND | SKIP_SNIPPET_TBLEND | SKIP_SNIPPET_PIP);
            CKERR(err, "BlitObjectToBitmap");
            GS_SendList(gs);

            visible ^= 1;
            ModifyGraphicsItemVA(viewI,
                                 VIEWTAG_BITMAP, bitmaps[visible],
                                 TAG_END);
            GS_BeginFrame(gs);
        }
        else
        {
            WaitTimeVBL(vbl, 0);
        }
    }
    

  cleanup:
        /* We must reset the texture address before freeing the UTF. */
    Blt_SetTexture(bdrop, texelData);
    Blt_FreeUTF(bdrop);
    DeleteItem(viewI);
    GS_FreeBitmaps(bitmaps, NUM_TBUFFS);
    GS_FreeLists(gs);
    GS_Delete(gs);
    KillEventUtility();
    DeleteTimerIOReq(vbl);
    CloseBlitterFolio();
    FreeSignal(signal);
    
    printf("pan done\n");
    
    return(0);
}
 

