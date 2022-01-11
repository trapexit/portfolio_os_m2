
/******************************************************************************
**
**  @(#) rotate.c 96/09/10 1.4
**
**  Tests text rotation
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
#define X_START 100
#define Y_START 100

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

void DrawExtentBox(GState *gs, TextExtent *te)
{
    Color4 col;

    col.r = col.a = 1.0; col.g = col.b = 0.0;

    F2_DrawLine(gs, &te->se_TopLeft, &te->se_TopRight, &col, &col);
    F2_DrawLine(gs, &te->se_TopRight, &te->se_BottomRight, &col, &col);
    F2_DrawLine(gs, &te->se_BottomRight, &te->se_BottomLeft, &col, &col);
    F2_DrawLine(gs, &te->se_BottomLeft, &te->se_TopLeft, &col, &col);
    F2_DrawLine(gs, &te->se_BaselineLeft, &te->se_BaselineRight, &col, &col);
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
    gfloat height, width;
    Err fontOpened = FALSE;
    int32 i;
    TextState *ts = NULL;
    StringExtent se[STRINGS];
    Err err;
    FontTextArray fta[STRINGS];
    char *fontName = "default_14";
    char *strings[STRINGS] =
    {
        "Spin",
        "around"
    };
    Item vblio;
    gfloat centerx, centery;
    TextExtent te;

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
        /* Calculate the total width and height of our combined strings */
    width = 0;
    height = 0;
    for (i = 0; i < STRINGS; i++)
    {
        uint32 thisWidth, thisHeight;
        memset(&fta[i].fta_Pen, 0, sizeof(PenInfo));
        fta[i].fta_Pen.pen_X = X_START;
        fta[i].fta_Pen.pen_Y = Y_START;
        fta[i].fta_Pen.pen_YScale = 1.0;
        fta[i].fta_Pen.pen_XScale = 1.0;
        GetStringExtent(&se[i], fontI, &fta[i].fta_Pen, strings[i], strlen(strings[i]));
        thisWidth = TEXTWIDTH(&se[i]);
        thisHeight = TEXTHEIGHT(&se[i]);
        if (thisWidth > width)
        {
            width = thisWidth;
        }
        height += thisHeight;
    }

    /* Build the FontTextArray */
    for (i = 0; i < STRINGS; i++)
    {
        fta[i].fta_StructSize = sizeof(FontTextArray);
        fta[i].fta_Pen.pen_X = X_START;
        if (i == 0)
        {
            fta[i].fta_Pen.pen_Y = Y_START;
        }
        else
        {
            fta[i].fta_Pen.pen_Y = (Y_START + TEXTHEIGHT(&se[i-1]));
        }
        fta[i].fta_Pen.pen_BgColor = 0xff;
        fta[i].fta_Pen.pen_FgColor = 0xffffff;
        fta[i].fta_Clip.min.x = 0;
        fta[i].fta_Clip.min.y = 0;
        fta[i].fta_Clip.max.x = (FBWIDTH - 1);
        fta[i].fta_Clip.max.y = (FBHEIGHT - 1);
        fta[i].fta_String = strings[i];
        fta[i].fta_NumChars = strlen(strings[i]);
    }

    /* Create a TextState */
    err = CreateTextState(&ts, fontI, &fta[0], STRINGS);
    CKERR(err, "Could not create text state");
    /* We want to keep track of the Text's bounding box */
    TrackTextBounds(ts, TRUE);

        /* Draw the text */
    ClearScreen(gs, 0.0, 0.0, 0.0);
    DrawText(gs, ts);
    GetTextExtent(ts, &te);
    printf("Extent  -\n");
    printf("(%g, %g) - (%g, %g) - (%g, %g) - (%g, %g)\n",
           te.se_TopLeft.x, te.se_TopLeft.y, te.se_TopRight.x, te.se_TopRight.y,
           te.se_BottomLeft.x, te.se_BottomLeft.y, te.se_BottomRight.x, te.se_BottomRight.y);
    printf("Baseline from (%g, %g) - (%g, %g)\n",
           te.se_BaselineLeft.x, te.se_BaselineLeft.y, te.se_BaselineRight.x, te.se_BaselineRight.y);
    DrawExtentBox(gs, &te);
    GS_SendList(gs);
    GS_WaitIO(gs);
    ModifyGraphicsItemVA(viewI,
                         VIEWTAG_BITMAP, bitmaps[visible],
                         TAG_END);
    visible ^= 1;
    GS_SetDestBuffer(gs, bitmaps[visible]);
    WaitForAnyButton(&cped);

        /* Rotate anti-clockwise around the baseline, scale and move */
    for (i = 0; i < 360; i++)
    {
        ClearScreen(gs, 0.0, 0.0, 0.0);
            /* Find where the text's current position */
        GetTextPosition(ts, (Point2 *)&fta[0].fta_Pen.pen_X);
            /* Rotate about the baseline */
        RotateText(ts, 1.0, fta[0].fta_Pen.pen_X, fta[0].fta_Pen.pen_Y);
            /* Scale it */
        ScaleText(ts, 1.001, 1.001);
            /* Move it */
        MoveText(ts, 0.1, 0);
            /* Get the text's bounding box */
        GetTextExtent(ts, &te);
            /* Link the text into the GState */
        DrawText(gs, ts);
            /* Draw the bounding box around the text */
        DrawExtentBox(gs, &te);
            /* Render */
        GS_SendList(gs);
        GS_WaitIO(gs);
        WaitTimeVBL(vblio, 0);
        ModifyGraphicsItemVA(viewI,
                             VIEWTAG_BITMAP, bitmaps[visible],
                             TAG_END);
        visible ^= 1;
        GS_SetDestBuffer(gs, bitmaps[visible]);
    }

        /* Rotate clockwise around the center */
    centerx = (se[0].se_TopLeft.y + (width / 2));
    centery = (se[0].se_TopLeft.y + (height / 2));
    for (i = 0; i < 360; i++)
    {
        ClearScreen(gs, 0.0, 0.0, 0.0);
        RotateText(ts, -1.0, centerx, centery);
        GetTextExtent(ts, &te);
        DrawText(gs, ts);
        DrawExtentBox(gs, &te);
        GS_SendList(gs);
        GS_WaitIO(gs);
        WaitTimeVBL(vblio, 0);
        ModifyGraphicsItemVA(viewI,
                             VIEWTAG_BITMAP, bitmaps[visible],
                             TAG_END);
        visible ^= 1;
        GS_SetDestBuffer(gs, bitmaps[visible]);
    }

        /* rotate anti-clockwise around somewhere off to the right */
    centerx = ((se[0].se_TopLeft.y + (width / 2)) + 50);
    centery = ((se[0].se_TopLeft.y + (height / 2)) + 50);
    for (i = 0; i < 360; i++)
    {
        ClearScreen(gs, 0.0, 0.0, 0.0);
        RotateText(ts, 1.0, centerx, centery);
        GetTextExtent(ts, &te);
        DrawText(gs, ts);
        DrawExtentBox(gs, &te);
        GS_SendList(gs);
        GS_WaitIO(gs);
        WaitTimeVBL(vblio, 0);
        ModifyGraphicsItemVA(viewI,
                             VIEWTAG_BITMAP, bitmaps[visible],
                             TAG_END);
        visible ^= 1;
        GS_SetDestBuffer(gs, bitmaps[visible]);
    }
    WaitForAnyButton(&cped);
    DeleteTextState(ts);

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

    printf("rotatetest done\n");

    return(0);
}
