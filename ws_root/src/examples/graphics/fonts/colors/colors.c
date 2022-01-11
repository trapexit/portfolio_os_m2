
/******************************************************************************
**
**  @(#) colors.c 96/07/09 1.3
**
**  Tests text color changes
**
******************************************************************************/

#include <kernel/types.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/frame2d/frame2d.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/font.h>
#include <misc/event.h>
#include <kernel/time.h>
#include <kernel/mem.h>
#include <kernel/random.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FBWIDTH 320
#define FBHEIGHT 240
#define DEPTH 16
#define NUM_FBUFFS 2
#define NUM_ZBUFFS 0
#define NUM_TBUFFS (NUM_FBUFFS + NUM_ZBUFFS)
#define ERR(s) {printf((s)); goto cleanup;}
#define CKERR(x, y) {if(x < 0){printf("Error - 0x%lx %s\n", x, y);PrintfSysErr(x); goto cleanup;}}

#define STRINGS 2

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

void ClearScreen(GState *gs, gfloat r, gfloat g, gfloat b)
{
    CLT_ClearFrameBuffer(gs, r, g, b, 0.0, TRUE, FALSE);
}

int main(int argc, char **argv)
{
    GState *gs = NULL;
    Item bitmaps[NUM_TBUFFS];
    ControlPadEventData cped;
    Item viewI = 0;
    Item fontI = 0;
    uint32 viewType = BestViewType(FBWIDTH, FBHEIGHT, DEPTH);
    uint32 visible = 0;
    Err fontOpened = FALSE;
    int32 i;
    uint32 fg, bg;
    TextState *ts = NULL;
    Err err;
    FontTextArray fta[STRINGS];
    char *fontName = "default_14";
    char *strings[STRINGS] =
    {
        "Champagne",
        "Supernova"
    };
    Item vblio;

    if ((argc > 1) && (*argv[1] == '?'))
    {
        printf("Usage:\n");
        printf("-f Font\n");
        exit(0);
    }

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-f") == 0)
        {
            fontName = argv[++i];
        }
    }

    vblio = CreateTimerIOReq();

        /* Initialize the EventBroker. */
    if (InitEventUtility(1, 0, LC_ISFOCUSED) < 0 )
    {
        ERR("InitEvenUtility\n");
    }

    printf("Open the Graphics folio\n");

    OpenGraphicsFolio();
    gs = GS_Create();
    if (gs == NULL)
    {
        ERR("Could not create GState\n");
    }

    printf("Open the font folio\n");

    fontOpened = OpenFontFolio();
    if (fontOpened < 0)
    {
        ERR("Could not open font folio\n");
    }

    fontI = OpenFont(fontName);
    CKERR(fontI, "Opening font ");

    GS_AllocLists(gs, NUM_FBUFFS, 2048);
    err = GS_AllocBitmaps(bitmaps, FBWIDTH, FBHEIGHT, BMTYPE_16, NUM_FBUFFS, NUM_ZBUFFS);
    CKERR(err, "Can't GS_AllocBitmaps()!\n");
    GS_SetDestBuffer(gs, bitmaps[visible]);
#if (NUM_ZBUFFS != 0)
    GS_SetZBuffer(gs, bitmaps[NUM_FBUFFS]); /* Use the last buffer as the Z buffer */
#endif
    viewI = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
                         VIEWTAG_VIEWTYPE, viewType,
                         VIEWTAG_BITMAP, bitmaps[visible],
                         TAG_END );
    CKERR(viewI, "can't create view item");
    AddViewToViewList(viewI, 0);

    ModifyGraphicsItemVA(viewI,
                         VIEWTAG_BITMAP, bitmaps[visible],
                         TAG_END);
    visible ^= 1;
    GS_SetDestBuffer(gs, bitmaps[visible]);

        /************************************************************************/

    bg = 0xff;
    fg = 0xffffff;
    for (i = 0; i < STRINGS; i++)
    {
        memset(&fta[i].fta_Pen, 0, sizeof(PenInfo));
        fta[i].fta_StructSize = sizeof(FontTextArray);
        fta[i].fta_Pen.pen_X = 100;
        fta[i].fta_Pen.pen_Y = (100 + (20 * i));
        fta[i].fta_Pen.pen_BgColor = bg;
        fta[i].fta_Pen.pen_FgColor = fg;
        fta[i].fta_Pen.pen_YScale = 1.0;
        fta[i].fta_Pen.pen_XScale = 1.0;
        fta[i].fta_Clip.min.x = 0;
        fta[i].fta_Clip.min.y = 0;
        fta[i].fta_Clip.max.x = (FBWIDTH - 1);
        fta[i].fta_Clip.max.y = (FBHEIGHT - 1);
        fta[i].fta_String = strings[i];
        fta[i].fta_NumChars = strlen(strings[i]);
    }

    err = CreateTextState(&ts, fontI, &fta[0], STRINGS);
    CKERR(err, "Could not create text state");

        /* Draw the text */
    ClearScreen(gs, 0.0, 0.0, 0.0);
    DrawText(gs, ts);
    GS_SendList(gs);
    GS_WaitIO(gs);
    ModifyGraphicsItemVA(viewI,
                         VIEWTAG_BITMAP, bitmaps[visible],
                         TAG_END);
    visible ^= 1;
    GS_SetDestBuffer(gs, bitmaps[visible]);
    WaitForAnyButton(&cped);

    /* Change colours */
    do
    {
        bg += 0x01020304;
        fg -= 0x01020304;

        ClearScreen(gs, 0.0, 0.0, 0.0);
        err = SetTextColor(ts, fg, bg);
        CKERR(err, "Changing color\n");
        DrawText(gs, ts);
        GS_SendList(gs);
        GS_WaitIO(gs);
        WaitTimeVBL(vblio, 0);
        ModifyGraphicsItemVA(viewI,
                             VIEWTAG_BITMAP, bitmaps[visible],
                             TAG_END);
        visible ^= 1;
        GS_SetDestBuffer(gs, bitmaps[visible]);
    }
    while (GetControlPad(1, FALSE, &cped) == 0);

  cleanup:
    DeleteItem(vblio);
    DeleteItem(viewI);
    GS_FreeBitmaps(bitmaps, NUM_TBUFFS);
    GS_FreeLists(gs);
    GS_Delete(gs);
    KillEventUtility();
    if (fontI > 0)
    {
        CloseFont(fontI);
    }
    if (fontOpened >= 0)
    {
        CloseFontFolio();
    }

    printf("colors done\n");

    return(0);
}
