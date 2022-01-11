/* @(#) scroll.c 96/08/23 1.4 */
/* This code scrolls a region of a bitmap. */

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
#define BOXWIDTH 70
#define BOXHEIGHT 70
#define LEFTEDGE ((FBWIDTH - BOXWIDTH) / 2)
#define TOPEDGE ((FBHEIGHT - BOXHEIGHT) / 2)
#define SCROLL_DELTA 1

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
    Item vbl = 0;
    uint32 bmType = ((DEPTH == 16) ? BMTYPE_16 : BMTYPE_32);
    const uint32 viewType = BestViewType(FBWIDTH, FBHEIGHT, DEPTH);
    BlitObject *bdrop = NULL;
    BlitObject *bo = NULL;
    BlitScroll bsc;
    Box2 box;
    Err err;

    if (argc != 2)
    {
        printf("Usage: scroll <backdrop utf file>\n");
        exit(0);
    }
    
         /* Initialize the EventBroker. */
    if (InitEventUtility(1, 0, LC_ISFOCUSED) < 0 )
    {
        ERR("InitEvenUtility\n");
    }
    vbl = CreateTimerIOReq();
    
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
    CKERR(err, "can't create bdrop sprite");

    /**********************************************************************************/

    err = Blt_CreateBlitObject(&bo, NULL);
    CKERR(err, "CreateBlitObject()");
    
        /* Initialise the BlitScroll structure with the area to
         * scroll. We are going to replace the area we expose
         * with green.
         */
    bsc.bsc_region.min.x = LEFTEDGE;
    bsc.bsc_region.min.y = TOPEDGE;
    bsc.bsc_region.max.x = (bsc.bsc_region.min.x + BOXWIDTH - 1);
    bsc.bsc_region.max.y = (bsc.bsc_region.min.y + BOXHEIGHT - 1);
    bsc.bsc_replaceColor = 0x0000ff00;

        /* Draw a box around the scrolled area for reference */
    box = bsc.bsc_region;
    box.min.x--;
    box.min.y--;
    box.max.x++;
    box.max.y++;
    GS_SetDestBuffer(gs, bitmaps[0]);
    Blt_BlitObjectToBitmap(gs, bdrop, bitmaps[0], 0);
    DrawBox(gs, &box);
    GS_SendList(gs);
    GS_WaitIO(gs);
    AddViewToViewList(viewI, 0);
    
        /* Now the main loop. Scroll the region based on the control
         * pad's arrow buttons. Repeat until the Stop button is pressed.
         */
    cped.cped_ButtonBits = 0;
    while (!(cped.cped_ButtonBits & ControlX))
    {
        bsc.bsc_dx = bsc.bsc_dy = 0;
        
        WaitTimeVBL(vbl, 1);
        GetControlPad(1, FALSE, &cped);
        
        if (cped.cped_ButtonBits & ControlUp)
        {
            bsc.bsc_dy = -SCROLL_DELTA;
        }
        if (cped.cped_ButtonBits & ControlDown)
        {
            bsc.bsc_dy = SCROLL_DELTA;
        }
        if (cped.cped_ButtonBits & ControlLeft)
        {
            bsc.bsc_dx = -SCROLL_DELTA;
        }
        if (cped.cped_ButtonBits & ControlRight)
        {
            bsc.bsc_dx = SCROLL_DELTA;
        }

            /* Do we want to scroll? */
        if (cped.cped_ButtonBits & (ControlUp | ControlDown | ControlLeft | ControlRight))
        {            
            err = Blt_Scroll(gs, bo, bitmaps[0], &bsc);
            CKERR(err, "Blt_Scroll()");
            GS_SendList(gs);
            GS_WaitIO(gs);
        }
    }
    
  cleanup:
    Blt_DeleteBlitObject(bo);
    Blt_FreeUTF(bdrop);
    DeleteItem(viewI);
    GS_FreeBitmaps(bitmaps, NUM_TBUFFS);
    GS_FreeLists(gs);
    GS_Delete(gs);
    KillEventUtility();
    DeleteTimerIOReq(vbl);
    CloseBlitterFolio();
    
    printf("scroll done\n");
    
    return(0);
}
 
