/* @(#) testblit.c 96/08/23 1.3 */
/* This code runs through a series of  blit tests. */

#include <kernel/types.h>
#include <graphics/clt/cltmacros.h>
#include <graphics/blitter.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <misc/event.h>
#include <kernel/mem.h>
#include <stdio.h>
#include "testblit.h"

void WaitForAnyButton(ControlPadEventData *cped);
void doCircleTest(GState *gs, Item *bm, ControlPadEventData *cped, BlitObject  *bdrop, Item viewI, Item vbl);
void doMaskTest(GState *gs, Item *bm, ControlPadEventData *cped, BlitObject  *bdrop, Item viewI);
void doSimpleTest(GState *gs, Item *bm, ControlPadEventData *cped, BlitObject  *bdrop, Item viewI, Item vbl);
void doSnippetTests(void);

#define CKERR(x, y) {if(x < 0){printf("Error - 0x%lx %s\n", x, y);PrintfSysErr(x); goto cleanup;}}
#define ERR(s) {printf((s)); goto cleanup;}
#define CKNULL(x, y) {if(x == NULL){printf("Error - %s\n", x, y); goto cleanup;}}

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

int main(int argc, char **argv)
{
    GState *gs = NULL;
    Item bitmaps[NUM_TBUFFS];
    ControlPadEventData cped;
    Item viewI = 0;
    Item vbl = 0;
    uint32 bmType = ((DEPTH == 16) ? BMTYPE_16 : BMTYPE_32);
    const uint32 viewType = BestViewType(FBWIDTH, FBHEIGHT, DEPTH);
    BlitObject  *bdrop = NULL;
    Err err;

    TOUCH(argc);
    
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

    printf("Load file %s\n", argv[1]);
    err = Blt_LoadUTF(&bdrop, argv[1]);
    CKERR(err, "Could not load utf file");

    GS_SetDestBuffer(gs, bitmaps[0]);
    err = Blt_BlitObjectToBitmap(gs, bdrop, bitmaps[0], 0);    
    CKERR(err, "Could not blit the backdrop");
    GS_SendList(gs);
    GS_WaitIO(gs);
    AddViewToViewList(viewI, 0);
    
    doSnippetTests();
    doCircleTest(gs, bitmaps, &cped, bdrop, viewI, vbl);
    doMaskTest(gs, bitmaps, &cped, bdrop, viewI); 
    doSimpleTest(gs, bitmaps, &cped, bdrop, viewI, vbl);

  cleanup:
    Blt_FreeUTF(bdrop);
    DeleteItem(viewI);
    GS_FreeBitmaps(bitmaps, NUM_TBUFFS);
    GS_FreeLists(gs);
    GS_Delete(gs);
    KillEventUtility();
    DeleteTimerIOReq(vbl);
    CloseBlitterFolio();
    
    printf("testblit done\n");
    
    return(0);
}
 
