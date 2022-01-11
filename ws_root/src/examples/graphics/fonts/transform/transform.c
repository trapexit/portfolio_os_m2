
/******************************************************************************
**
**  @(#) transform.c 96/09/18 1.5
**
**  Tests text transformations
**
******************************************************************************/

#include <kernel/types.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/frame2d/frame2d.h>
#include <graphics/font.h>
#include <misc/event.h>
#include <kernel/mem.h>
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

void MatrixRotate(FontMatrix fm, gfloat angle, Point2*center);
void MatrixScale(FontMatrix fm, gfloat scale, Point2 *center);
void MatrixTranslate(FontMatrix fm, gfloat dx, gfloat dy);

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
    Err fontOpened = FALSE;
    int32 i;
    TextState *ts = NULL;
    StringExtent se[STRINGS];
    Err err;
    FontTextArray fta[STRINGS];
    char *fontName = "default_14";
    char *strings[STRINGS] =
    {
        "Move me",
        "around",
    };
    Item vblio;
    TextExtent te;
    FontMatrix tran;
    Point2 clipTL, clipBR;
    Point2 center;
    Color4 color = {1.0, 1.0, 0, 1.0};
    gfloat angle;
    gfloat sx, sy;
    PenInfo pen;
    char statusString[64];
    bool clip = TRUE;

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

    OpenGraphicsFolio();
    gs = GS_Create();
    if (gs == NULL)
    {
        ERR("Could not create GState\n");
    }

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
        /* Get the StringExtents of the strings */
    for (i = 0; i < STRINGS; i++)
    {
        memset(&fta[i].fta_Pen, 0, sizeof(PenInfo));
        fta[i].fta_Pen.pen_X = X_START;
        fta[i].fta_Pen.pen_Y = Y_START;
        fta[i].fta_Pen.pen_YScale = 1.0;
        fta[i].fta_Pen.pen_XScale = 1.0;
        GetStringExtent(&se[i], fontI, &fta[i].fta_Pen, strings[i], strlen(strings[i]));
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
    err = TrackTextBounds(ts, TRUE);
    CKERR(err, "Cannot track text bounds");

        /* Set the clip box to the edge of the screen */
    clipTL.x = 0;
    clipTL.y = 0;
    clipBR.x = FBWIDTH;
    clipBR.y = FBHEIGHT;
    err = SetClipBox(ts, clip, &clipTL, &clipBR);
    CKERR(err, "Could not set the clip box");

        /* Draw the text */
    ClearScreen(gs, 0.0, 0.0, 0.0);
    DrawText(gs, ts);
    GetTextExtent(ts, &te);
    DrawExtentBox(gs, &te);
    GS_SendList(gs);
    GS_WaitIO(gs);
    ModifyGraphicsItemVA(viewI,
                         VIEWTAG_BITMAP, bitmaps[visible],
                         TAG_END);
    visible ^= 1;
    GS_SetDestBuffer(gs, bitmaps[visible]);

    cped.cped_ButtonBits = 0;
    center.x = (FBWIDTH / 2);
    center.y = (FBHEIGHT / 2);
    memset(&pen, 0, sizeof(PenInfo));
    pen.pen_FgColor = 0x00888888;
    pen.pen_BgColor = 0;
    pen.pen_XScale = 1.0;
    pen.pen_YScale = 1.0;

    printf("Move the text and center point:\n");
    printf("\tDirection pad moves the text.\n");
    printf("\tShift buttons rotate the text.\n");
    printf("\tA - Scales the text up\n");
    printf("\tB - Scales the text down\n");
    printf("\tP - toggles clipping\n");
    printf("\tC + Direction pad - move the center point.\n");
    printf("\tX - exit\n\n");
        /* This is the main loop. We keep looping until the X button is pressed. */
    sx = 1.0;
    sy = 1.0;
    angle = 0.0;
    while (!(cped.cped_ButtonBits & ControlX))
    {
        bool moveCenter = FALSE;

        /* Initialise the matrix to the Identity */
        tran[0][0] = 1.0; tran[0][1] = 0.0;
        tran[1][0] = 0.0; tran[1][1] = 1.0;
        tran[2][0] = 0.0; tran[2][1] = 0.0;

        if (cped.cped_ButtonBits & ControlStart)
        {
            GetControlPad(1, TRUE, &cped); /* Wait for button up */
            clip = !clip;
            printf("Clipping %s\n", clip ? "on" : "off");
            SetClipBox(ts, clip, &clipTL, &clipBR);
        }
        if (cped.cped_ButtonBits & ControlLeftShift)
        {
            MatrixRotate(tran, -1.0, &center);
        }
        if (cped.cped_ButtonBits & ControlRightShift)
        {
            MatrixRotate(tran, 1.0, &center);
        }
        if ((cped.cped_ButtonBits & ControlA) && (sx <= 10.0) && (sy <= 10.0))
        {
            MatrixScale(tran, 1.1, &center);
        }
        if ((cped.cped_ButtonBits & ControlB) && (sx >= 0.1) && (sy >= 0.1))
        {
            MatrixScale(tran, 0.9, &center);
        }
        if (cped.cped_ButtonBits & ControlC)
        {
            moveCenter = TRUE;
        }
        if (cped.cped_ButtonBits & ControlUp)
        {
            if (moveCenter)
            {
                center.y -= 1.0;
                if (center.y < 0)
                {
                    center.y = 0;
                }
            }
            else
            {
                MatrixTranslate(tran, 0, -1.0);
            }
        }
        if (cped.cped_ButtonBits & ControlDown)
        {
            if (moveCenter)
            {
                center.y += 1.0;
                if (center.y > FBHEIGHT)
                {
                    center.y = FBHEIGHT;
                }
            }
            else
            {
                MatrixTranslate(tran, 0, 1.0);
            }
        }
        if (cped.cped_ButtonBits & ControlLeft)
        {
            if (moveCenter)
            {
                center.x -= 1.0;
                if (center.x < 0)
                {
                    center.x = 0;
                }
            }
            else
            {
                MatrixTranslate(tran, -1.0, 0);
            }
        }
        if (cped.cped_ButtonBits & ControlRight)
        {
            if (moveCenter)
            {
                center.x += 1.0;
                if (center.x > FBWIDTH)
                {
                    center.x = FBWIDTH;
                }
            }
            else
            {
                MatrixTranslate(tran, 1.0, 0);
            }
        }

        ClearScreen(gs, 0.0, 0.0, 0.0);
            /* Draw the center point */
        F2_FillRect(gs, (center.x - 1), (center.y - 1), (center.x + 1), (center.y + 1), &color);

            /* Apply the transformation matrix */
        err = TransformText(ts, tran);
        CKERR(err, "Transformation matrix failed");
            /* Get the text's bounding box */
        GetTextExtent(ts, &te);
            /* Link the text into the GState */
        DrawText(gs, ts);
            /* Draw the bounding box around the text */
        DrawExtentBox(gs, &te);
            /* Render */
        GS_SendList(gs);
        GS_WaitIO(gs);

        GetTextAngle(ts, &angle);
        GetTextScale(ts, &sx, &sy);
        sprintf(statusString, "%3.4g\n%3.4g %3.4g", angle, sx, sy);
        pen.pen_X = 160;
        pen.pen_Y = 40;
        DrawString(gs, fontI, &pen, statusString, strlen(statusString));

        ModifyGraphicsItemVA(viewI,
                             VIEWTAG_BITMAP, bitmaps[visible],
                             TAG_END);
        WaitTimeVBL(vblio, 0);
        visible ^= 1;
        GS_SetDestBuffer(gs, bitmaps[visible]);

        GetControlPad(1, FALSE, &cped);
        while (cped.cped_ButtonBits == 0)
        {
            /* Wait for a button */
            WaitTimeVBL(vblio, 1);
            GetControlPad(1, FALSE, &cped);
        }
    }

  cleanup:
    DeleteTextState(ts);
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
