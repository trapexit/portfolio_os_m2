/******************************************************************************
**
**  @(#) testnl.c 96/07/09 1.3
**
**  Tests correct handling of \n characters
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FBWIDTH 320
#define FBHEIGHT 240
#define DEPTH 16
#define NUM_FBUFFS 1
#define NUM_ZBUFFS 0
#define NUM_TBUFFS (NUM_FBUFFS + NUM_ZBUFFS)
#define CKERR(x,y) {if(x < 0) {printf("Error - 0x%lx %s\n",  x, y);PrintfSysErr(x); goto cleanup;}}
#define ERR(s) {printf((s)); goto cleanup;}

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
    GS_SendList(gs);
    GS_WaitIO(gs);
}

void DrawExtentBox(GState *gs, TextExtent *te)
{
    Color4 col;

    col.r = 1.0; col.a = 1.0; col.g = col.b = 0.0;

    F2_DrawLine(gs, &te->se_TopLeft, &te->se_TopRight, &col, &col);
    F2_DrawLine(gs, &te->se_TopRight, &te->se_BottomRight, &col, &col);
    F2_DrawLine(gs, &te->se_BottomRight, &te->se_BottomLeft, &col, &col);
    F2_DrawLine(gs, &te->se_BottomLeft, &te->se_TopLeft, &col, &col);
    F2_DrawLine(gs, &te->se_BaselineLeft, &te->se_BaselineRight, &col, &col);
}

#define STRINGS (sizeof(teststrings) / sizeof(char *))

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
    Err err;
    int32 i;
    uint32 bgColor = 0xffffff;
    TextState *ts = NULL;
    FontTextArray fta;
    PenInfo pen;
    TextExtent te;
    char *teststrings[] =
    {
        "Large\nLargest\nsmall",
        "\n",
        "\n\n",
        "Hello",
        "Hello\nWorld",
        "Hello\nWorld\n",
        "Hello\n\nWorld",
        "Hello\n\nWorld\n",
        "Hello\n\nWorld\n\n",
        "Hello\n\nWorld\nGoodbye",
        "Hello\n\nWorld\nGoodbye\n",
        "Hello\n\nWorld\n\nGoodbye\n\n",
        "\nHello",
        "\nHello\nWorld",
        "\nHello\nWorld\n",
        "\nHello\n\nWorld",
        "\nHello\n\nWorld\n",
        "\nHello\n\nWorld\n\n",
        "\nHello\n\nWorld\nGoodbye",
        "\nHello\n\nWorld\nGoodbye\n",
        "\nHello\n\nWorld\n\nGoodbye\n\n",
    };

    if (argc == 2)
    {
        bgColor = strtol(argv[1], NULL, 0);
    }
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
    CKERR(fontOpened, "Opening Font Folio ");

    fontI = OpenFont("default_14");
    CKERR(fontI, "Opening font ");

    GS_AllocLists(gs, NUM_FBUFFS, 2048);
    GS_AllocBitmaps(&bitmaps[0], FBWIDTH, FBHEIGHT, BMTYPE_16, NUM_FBUFFS, NUM_ZBUFFS);
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
    CLT_SetSrcToCurrentDest(gs);

    /****************************************************************************/

    memset(&pen, 0, sizeof(PenInfo));
    pen.pen_XScale = 1.0;
    pen.pen_YScale = 1.0;
    pen.pen_FgColor = 0x0000ff00;
    pen.pen_BgColor = bgColor;
    for(i = 0; i < STRINGS; i++)
    {
        ClearScreen(gs, 0.0, 0.0, 0.0);
        pen.pen_X = 70.0;
        pen.pen_Y = 80.0;
        err = GetStringExtent((StringExtent *)&te, fontI, &pen, teststrings[i], strlen(teststrings[i]));
        CKERR(err, "GetStringExtent() ");
        printf("\n>>>>>%s", teststrings[i]);
        printf("<<<<<\n");
        err = DrawString(gs, fontI, &pen, teststrings[i], strlen(teststrings[i]));
        CKERR(err, "DrawString() ");
        DrawExtentBox(gs, &te);
        GS_SendList(gs);
        GS_WaitIO(gs);
        WaitForAnyButton(&cped);
    }

    memset(&fta, 0, sizeof(FontTextArray));
    printf("Do same with TextStates\n");
    fta.fta_StructSize = sizeof(FontTextArray);
    fta.fta_Pen.pen_XScale = 1.0;
    fta.fta_Pen.pen_YScale = 1.0;
    fta.fta_Pen.pen_FgColor = 0x0000ff00;
    fta.fta_Pen.pen_BgColor = bgColor;
    fta.fta_Clip.min.x = 0;
    fta.fta_Clip.min.y = 0;
    fta.fta_Clip.max.x = (FBWIDTH - 1);
    fta.fta_Clip.max.y = (FBHEIGHT - 1);
    for(i = 0; i < STRINGS; i++)
    {
        ClearScreen(gs, 0.0, 0.0, 0.0);
        fta.fta_Pen.pen_X = 70.0;
        fta.fta_Pen.pen_Y = 80.0;
        fta.fta_String = teststrings[i];
        fta.fta_NumChars = strlen(teststrings[i]);
        printf("\n>>>>>%s", teststrings[i]);
        printf("<<<<<\n");
        err = CreateTextState(&ts, fontI, &fta, 1);
        CKERR(err, "CreateText() ");
        TrackTextBounds(ts, TRUE);
        err = DrawText(gs, ts);
        CKERR(err, "DrawText");
        err = GetTextExtent(ts, &te);
        CKERR(err, "GetTextExtent() ");
        DrawExtentBox(gs, &te);
        GS_SendList(gs);
        GS_WaitIO(gs);
        WaitForAnyButton(&cped);
    }

  cleanup:
    DeleteItem(viewI);
    GS_FreeBitmaps(&bitmaps[0], NUM_TBUFFS);
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

    printf("testnl done\n");

    return(0);
}
