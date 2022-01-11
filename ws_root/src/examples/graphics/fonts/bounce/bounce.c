
/******************************************************************************
**
**  @(#) bounce.c 96/09/10 1.4
**
**  Tests text moving and scaling
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

#define STRINGS 2
#define X_START 100
#define Y_START 100
#define BOUNCESPEED 8
#define BOUNCESPEEDMASK (BOUNCESPEED - 1)
#define SCALE_MAX 16

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
    gfloat origHeight, origWidth;
    Err fontOpened = FALSE;
    int32 i;
    TextState *ts = NULL;
    StringExtent se[STRINGS];
    Point2 point;
    int32 directionX, directionY;
    Err err;
    FontTextArray fta[STRINGS];
    char *fontName = "default_14";
    char *strings[STRINGS] =
    {
        "Bounce",
        "around"
    };
    Item vblio;
    gfloat sx, sy;
    uint32 scaleCnt;
    gfloat scaleVal = 1.1;

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
    GS_AllocBitmaps(bitmaps, FBWIDTH, FBHEIGHT, BMTYPE_16, NUM_FBUFFS, NUM_ZBUFFS);
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
        /* Scale the text in the x direction */
        fta[i].fta_Pen.pen_XScale = (1.0 + (gfloat)i);
        GetStringExtent(&se[i], fontI, &fta[i].fta_Pen, strings[i], strlen(strings[i]));
        thisWidth = TEXTWIDTH(&se[i]);
        thisHeight = TEXTHEIGHT(&se[i]);
        if (thisWidth > width)
        {
            width = thisWidth;
        }
        height += thisHeight;
    }
    origWidth = width;
    origHeight = height;

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
            fta[i].fta_Pen.pen_Y = (Y_START + TEXTHEIGHT(&se[i]));
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

    sx = sy = 1.0;

        /* Move text across the screen */
    for (i = X_START; i < (FBWIDTH - width); i++)
    {
        ClearScreen(gs, 0.0, 0.0, 0.0);
        MoveText(ts, 1.0, 0);
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
    for (i = (FBWIDTH - width); i >= 0; i--)
    {
        ClearScreen(gs, 0.0, 0.0, 0.0);
        MoveText(ts, -1.0, 0);
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

    /* Move text up and down the screen */
    GetTextPosition(ts, &point);
    for (i = point.y; i >= TO_BASELINE(&se[0]); i--)
    {
        ClearScreen(gs, 0.0, 0.0, 0.0);
        MoveText(ts, 0, -1.0);
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
    for (i = 0; i < (FBHEIGHT - height); i++)
    {
        ClearScreen(gs, 0.0, 0.0, 0.0);
        MoveText(ts, 0, 1.0);
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

    /* Bounce randomly around the screen */
    GetTextPosition(ts, &point);
    directionX = ((ReadHardwareRandomNumber() & BOUNCESPEEDMASK) + 1);
    /* We know we are at the bottom of the screen, so move up the screen */
    directionY = (((ReadHardwareRandomNumber() & BOUNCESPEEDMASK) + 1) * -1);
    cped.cped_ButtonBits = 0;

    /* We are going to expand and shrink all the text at the same time as we move it */
    scaleCnt = (SCALE_MAX / 2);
    do
    {
        /* Where is the text now? */
        GetTextPosition(ts, &point);

        /* If we move it, where will the text be? */
        point.x += (gfloat)directionX;
        point.y += (gfloat)directionY;

        /* Have we reached the max scale size? */
        if (scaleCnt++ == SCALE_MAX)
        {
            /* If so, contract again */
            scaleVal = (1 / scaleVal);
            scaleCnt = 0;
        }

        /* Scale the text. This will be scaled about the current text's position */
        ScaleText(ts, scaleVal, scaleVal);
        /* Just how much larger or smaller is the text? */
        GetTextScale(ts, &sx, &sy);

        /* Calculate how large the text is now... */
        width = (origWidth * sx);
        height = (origHeight * sy);

            /* ...to see if we over the edge?
             * If the text is over the edge, then bring it back on screen and
             * reverse the direction.
             */
        if (point.x < 0.0)
        {
            point.x = 0.0;
            directionX = ((ReadHardwareRandomNumber() & BOUNCESPEEDMASK) + 1);
        }
        if (point.x > (gfloat)(FBWIDTH - width))
        {
            point.x = (gfloat)(FBWIDTH - width);
            directionX = (((ReadHardwareRandomNumber() & BOUNCESPEEDMASK) + 1) * -1);
        }
        if (point.y < (gfloat)TO_BASELINE(&se[0]))
        {
            point.y = (gfloat)TO_BASELINE(&se[0]);
            directionY = ((ReadHardwareRandomNumber() & BOUNCESPEEDMASK) + 1);
        }
        if (point.y > (gfloat)(FBHEIGHT - height + TO_BASELINE(&se[1])))
        {
            point.y = (gfloat)(FBHEIGHT - height + TO_BASELINE(&se[1]));
            directionY = (((ReadHardwareRandomNumber() & BOUNCESPEEDMASK) + 1) * -1);
        }

        ClearScreen(gs, 0.0, 0.0, 0.0);
        /* Move the text by the desired amount */
        MoveText(ts, (gfloat)directionX, (gfloat)directionY);
        /* Link the text instructions into the GState */
        DrawText(gs, ts);
        /* Render /*/
        GS_SendList(gs);
        GS_WaitIO(gs);
        WaitTimeVBL(vblio, 0);
        ModifyGraphicsItemVA(viewI,
                             VIEWTAG_BITMAP, bitmaps[visible],
                             TAG_END);
        visible ^= 1;
        GS_SetDestBuffer(gs, bitmaps[visible]);
    } while (GetControlPad(1, FALSE, &cped) == 0);

        /* Finished with the text state */
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

    printf("bounce done\n");

    return(0);
}
