
/******************************************************************************
**
**  @(#) newfont.c 96/07/09 1.4
**
**  Tests font rendering
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
#define LOOPCOUNT loopcount

#define ERR(s) {printf((s)); goto cleanup;}
#define CKERR(x,y) {if(x < 0) {printf("Error - 0x%lx %s\n",  x, y);PrintfSysErr(x); goto cleanup;}}

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

void DrawClipBox(GState *gs, FontTextArray *fta)
{
    Color4 col[2];
    Point2 p1, p2;
    int32 i;

    col[0].r = col[0].g = col[0].b = col[0].a = 1.0;
    col[1].r = col[1].a = 1.0; col[1].g = col[1].b = 0.0;

    for (i = 0; i < 2; i++)
    {
        p1 = fta[i].fta_Clip.min;
        p2.x = fta[i].fta_Clip.max.x;
        p2.y = p1.y;
        F2_DrawLine(gs, &p1, &p2, &col[i], &col[i]);
        p1 = fta[i].fta_Clip.max;
        F2_DrawLine(gs, &p1, &p2, &col[i], &col[i]);
        p2.x = fta[i].fta_Clip.min.x;
        p2.y = p1.y;
        F2_DrawLine(gs, &p1, &p2, &col[i], &col[i]);
        p1 = fta[i].fta_Clip.min;
        F2_DrawLine(gs, &p1, &p2, &col[i], &col[i]);

        GS_SendList(gs);
        GS_WaitIO(gs);
    }
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
    int32 i, j;
    TextState *ts = NULL;
    PenInfo pen;
    StringExtent se;
    CharacterData cd;
    TextStencilInfo tsi;
    TextStencil *tsl;
        /* Timing stuff */
    TimeVal tv;
    TimerTicks in, out, difference, total;
    int32 loopcount = 1000;
    Err err;
    FontTextArray fta[3];
    char *fontName = "default_14";
    gfloat sx = 1.0, sy = 1.0;

    if (((argc > 1) && (*argv[1] == '?')) ||
        (argc == 1))
    {
        printf("Usage:\n");
        printf("-n Number of iterations\n");
        printf("-f Font\n");
        printf("-sx x scale factor\n");
        printf("-sy y scale factor\n");
        exit(0);
    }

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-n") == 0)
        {
            loopcount = atoi(argv[++i]);
        }

        if (strcmp(argv[i], "-f") == 0)
        {
            fontName = argv[++i];
        }
        if (strcmp(argv[i], "-sx") == 0)
        {
            sx = strtof(argv[++i], NULL);
        }
        if (strcmp(argv[i], "-sy") == 0)
        {
            sy = strtof(argv[++i], NULL);
        }
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
    if (fontOpened < 0)
    {
        ERR("Could not open font folio\n");
    }

    fontI = OpenFont(fontName);
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

        /* Get the StringExtent of a string */
    memset(&pen, 0, sizeof(PenInfo));
    pen.pen_X = 70.0;
    pen.pen_Y = 120.0;
    pen.pen_XScale = sx;
    pen.pen_YScale = sy;
    err = GetStringExtent(&se, fontI, &pen, "Hello!", 6);
    CKERR(err, "GetStringExtent() ");
    printf("Extent for Hello! -\n");
    printf("(%g, %g) - (%g, %g) - (%g, %g) - (%g, %g)\n",
           se.se_TopLeft.x, se.se_TopLeft.y, se.se_TopRight.x, se.se_TopRight.y,
           se.se_BottomLeft.x, se.se_BottomLeft.y, se.se_BottomRight.x, se.se_BottomRight.y);
    printf("Baseline from (%g, %g) - (%g, %g)\n",
           se.se_BaselineLeft.x, se.se_BaselineLeft.y, se.se_BaselineRight.x, se.se_BaselineRight.y);

        /* Get the character data of a character */
    err = GetCharacterData(&cd, fontI, 'H');
    CKERR(err, "GetCharacterData");
    printf("For character H\n");
    printf("texel @ 0x%lx, height = %ld, width = %ld\n", cd.cd_Texel, cd.cd_CharHeight,
           cd.cd_CharWidth);
    printf("bpp = %ld, ascent = %ld, descent = %ld, leading = %ld, bpr = %ld\n", cd.cd_BitsPerPixel,
           cd.cd_Ascent, cd.cd_Descent, cd.cd_Leading, cd.cd_BytesPerRow);

        /* Test the simple case of drawing a string */
    ClearScreen(gs, 0.0, 0.0, 0.0);
    memset(&total, 0, sizeof(TimerTicks));

    for (i = 0; i < LOOPCOUNT; i++)
    {
        pen.pen_X = 70.0;
        pen.pen_Y = 120.0;
        pen.pen_FgColor = 0x0000ff00;
        pen.pen_BgColor = 0;
        SampleSystemTimeTT(&in);
        err = DrawString(gs, fontI, &pen, "Hello!", 6);
        SampleSystemTimeTT(&out);
        CKERR(err, "DrawString() ");
        SubTimerTicks(&in, &out, &difference); /* difference = out - in */
        AddTimerTicks(&difference, &total, &total);
    }
    ConvertTimerTicksToTimeVal(&total, &tv);
    printf("DrawString() Hello! %d times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
    WaitForAnyButton(&cped);


        /* Now try it with a /n character */
    ClearScreen(gs, 0.0, 0.0, 0.0);
    memset(&total, 0, sizeof(TimerTicks));
    for (i = 0; i < LOOPCOUNT; i++)
    {
        pen.pen_X = 70.0;
        pen.pen_Y = 70.0;
        pen.pen_FgColor = 0x0000ff00;
        pen.pen_BgColor = 0x00ffffff;
        SampleSystemTimeTT(&in);
        err = DrawString(gs, fontI, &pen, "Hello!\nWorld", 12);
        SampleSystemTimeTT(&out);
        CKERR(err, "DrawString() ");
        SubTimerTicks(&in, &out, &difference); /* difference = out - in */
        AddTimerTicks(&difference, &total, &total);
    }
    ConvertTimerTicksToTimeVal(&total, &tv);
    printf("DrawString() Hello\\nWorld %d times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
    WaitForAnyButton(&cped);

        /* Now try it with an unprintable character */
    ClearScreen(gs, 0.0, 0.0, 0.0);
    memset(&total, 0, sizeof(TimerTicks));
    for (i = 0; i < LOOPCOUNT; i++)
    {
        pen.pen_X = 70.0;
        pen.pen_Y = 70.0;
        pen.pen_FgColor = 0x0000ff00;
        pen.pen_BgColor = 0;
        SampleSystemTimeTT(&in);
        err = DrawString(gs, fontI, &pen, "No\tGaps\tHere\t", 13);
        SampleSystemTimeTT(&out);
        CKERR(err, "DrawString() ");
        SubTimerTicks(&in, &out, &difference); /* difference = out - in */
        AddTimerTicks(&difference, &total, &total);
    }
    ConvertTimerTicksToTimeVal(&total, &tv);
    printf("DrawString() unprintable string %d times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
    WaitForAnyButton(&cped);

        /* Try a FontTextArray of 1, but with a \n in the text */
    fta[0].fta_StructSize = sizeof(FontTextArray);
    fta[0].fta_Clip.min.x = 0;
    fta[0].fta_Clip.min.y = 0;
    fta[0].fta_Clip.max.x = (FBWIDTH - 1);
    fta[0].fta_Clip.max.y = (FBHEIGHT - 1);
    memset(&fta[0].fta_Pen, 0, sizeof(PenInfo));
    fta[0].fta_Pen.pen_X = 60.0;
    fta[0].fta_Pen.pen_Y = 40.0;
    fta[0].fta_Pen.pen_BgColor = 0x0000ff;
    fta[0].fta_Pen.pen_FgColor = 0xffffff;
    fta[0].fta_Pen.pen_XScale = sx;
    fta[0].fta_Pen.pen_YScale = sy;
    fta[0].fta_String = "hello\nfriends";
    fta[0].fta_NumChars = 13;
    ClearScreen(gs, 0.0, 0.0, 0.0);
    memset(&total, 0, sizeof(TimerTicks));
    err = CreateTextState(&ts, fontI, fta, 1);
    CKERR(err, "DrawFontTextArray()\n");
    for (i = 0; i < LOOPCOUNT; i++)
    {
        SampleSystemTimeTT(&in);
        err = DrawText(gs, ts);
        CKERR(err, "DrawText");
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        SubTimerTicks(&in, &out, &difference); /* difference = out - in */
        AddTimerTicks(&difference, &total, &total);
    }
    DrawClipBox(gs, fta);
    ConvertTimerTicksToTimeVal(&total, &tv);
    printf("DrawText() white hello\\nfriends %d times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
    WaitForAnyButton(&cped);
    DeleteTextState(ts);
    ts = NULL;

        /* Do the same, but clip off the bottom after the \n */
    fta[0].fta_Clip.max.y = (fta[0].fta_Pen.pen_Y + 20);
    ClearScreen(gs, 0.0, 0.0, 0.0);
    memset(&total, 0, sizeof(TimerTicks));
    err = CreateTextState(&ts, fontI, fta, 1);
    CKERR(err, "DrawFontTextArray()\n");
    for (i = 0; i < LOOPCOUNT; i++)
    {
        SampleSystemTimeTT(&in);
        err = DrawText(gs, ts);
        CKERR(err, "DrawText");
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        SubTimerTicks(&in, &out, &difference); /* difference = out - in */
        AddTimerTicks(&difference, &total, &total);
    }
    DrawClipBox(gs, fta);
    ConvertTimerTicksToTimeVal(&total, &tv);
    printf("DrawText() clipped white hello\\nfriends %d times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
    WaitForAnyButton(&cped);
    DeleteTextState(ts);
    ts = NULL;

    /* Here comes a slew of more tests */
    fta[0].fta_Clip.min.x = 0;
    fta[0].fta_Clip.min.y = 0;
    fta[0].fta_Clip.max.x = (FBWIDTH - 1);
    fta[0].fta_Clip.max.y = (FBHEIGHT - 1);
    fta[1].fta_Clip.min.x = 0;
    fta[1].fta_Clip.min.y = 0;
    fta[1].fta_Clip.max.x = (FBWIDTH - 1);
    fta[1].fta_Clip.max.y = (FBHEIGHT - 1);
    for (j = 0; j < 2; j++)
    {
        fta[0].fta_StructSize = sizeof(FontTextArray);
        memset(&fta[0].fta_Pen, 0, sizeof(PenInfo));
        fta[0].fta_Pen.pen_X = 60.0;
        fta[0].fta_Pen.pen_Y = 40.0;
        fta[0].fta_Pen.pen_BgColor = 0;
        fta[0].fta_Pen.pen_FgColor = 0xffffff;
        fta[0].fta_Pen.pen_XScale = sx;
        fta[0].fta_Pen.pen_YScale = sy;
        fta[0].fta_String = "hello";
        fta[0].fta_NumChars = 5;
        fta[1].fta_StructSize = sizeof(FontTextArray);
        memset(&fta[1].fta_Pen, 0, sizeof(PenInfo));
        fta[1].fta_Pen.pen_X = 60.0;
        fta[1].fta_Pen.pen_Y = 60.0;
        fta[1].fta_Pen.pen_BgColor = 0x0;
        fta[1].fta_Pen.pen_FgColor = 0xffffff;
        fta[1].fta_Pen.pen_XScale = sx;
        fta[1].fta_Pen.pen_YScale = sy;
        fta[1].fta_String = "world";
        fta[1].fta_NumChars = 5;


            /* Set up the text state */
        ClearScreen(gs, 0.0, 0.0, 0.0);
        err = CreateTextState(&ts, fontI, fta, 2);
        CKERR(err, "DrawFontTextArray()\n");

        memset(&total, 0, sizeof(TimerTicks));
        for (i = 0; i < LOOPCOUNT; i++)
        {
            SampleSystemTimeTT(&in);
            err = DrawText(gs, ts);
            CKERR(err, "DrawText");
            GS_SendList(gs);
            GS_WaitIO(gs);
            SampleSystemTimeTT(&out);
            SubTimerTicks(&in, &out, &difference); /* difference = out - in */
            AddTimerTicks(&difference, &total, &total);
        }
        DrawClipBox(gs, fta);
        ConvertTimerTicksToTimeVal(&total, &tv);
        printf("DrawText() white hello world %d times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
        WaitForAnyButton(&cped);
        DeleteTextState(ts);
        ts = NULL;

        ClearScreen(gs, 0.0, 0.0, 0.0);
        /* Draw with a blue background */
        fta[0].fta_Pen.pen_BgColor = fta[1].fta_Pen.pen_BgColor = 0x0000ff;
        err = CreateTextState(&ts, fontI, fta, 2);
        CKERR(err, "DrawFontTextArray()\n");
        memset(&total, 0, sizeof(TimerTicks));
        for (i = 0; i < LOOPCOUNT; i++)
        {
            SampleSystemTimeTT(&in);
            err = DrawText(gs, ts);
            CKERR(err, "DrawText");
            GS_SendList(gs);
            GS_WaitIO(gs);
            SampleSystemTimeTT(&out);
            SubTimerTicks(&in, &out, &difference); /* difference = out - in */
            AddTimerTicks(&difference, &total, &total);
        }
        DrawClipBox(gs, fta);
        ConvertTimerTicksToTimeVal(&total, &tv);
        printf("DrawText() white on blue hello world %d times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
        WaitForAnyButton(&cped);
        DeleteTextState(ts);
        ts = NULL;

        /* Try again, with capital H and W in Hello World */
        ClearScreen(gs, 0.0, 0.0, 0.0);
        fta[0].fta_Pen.pen_BgColor = fta[1].fta_Pen.pen_BgColor = 0x0;
        fta[0].fta_String = "Hello";
        fta[1].fta_String = "World";
        err = CreateTextState(&ts, fontI, fta, 2);
        CKERR(err, "DrawFontTextArray()\n");
        memset(&total, 0, sizeof(TimerTicks));
        for (i = 0; i < LOOPCOUNT; i++)
        {
            SampleSystemTimeTT(&in);
            err = DrawText(gs, ts);
            CKERR(err, "DrawText");
            GS_SendList(gs);
            GS_WaitIO(gs);
            SampleSystemTimeTT(&out);
            SubTimerTicks(&in, &out, &difference); /* difference = out - in */
            AddTimerTicks(&difference, &total, &total);
        }
        DrawClipBox(gs, fta);
        ConvertTimerTicksToTimeVal(&total, &tv);
        printf("DrawText() white Hello World %d times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
        WaitForAnyButton(&cped);
        DeleteTextState(ts);
        ts = NULL;

        ClearScreen(gs, 0.0, 0.0, 0.0);
        fta[0].fta_Pen.pen_BgColor = fta[1].fta_Pen.pen_BgColor = 0x0000ff;
        fta[0].fta_String = "Hello";
        fta[1].fta_String = "World";
        err = CreateTextState(&ts, fontI, fta, 2);
        CKERR(err, "DrawFontTextArray()\n");
        memset(&total, 0, sizeof(TimerTicks));
        for (i = 0; i < LOOPCOUNT; i++)
        {
            SampleSystemTimeTT(&in);
            err = DrawText(gs, ts);
            CKERR(err, "DrawText");
            GS_SendList(gs);
            GS_WaitIO(gs);
            SampleSystemTimeTT(&out);
            SubTimerTicks(&in, &out, &difference); /* difference = out - in */
            AddTimerTicks(&difference, &total, &total);
        }
        DrawClipBox(gs, fta);
        ConvertTimerTicksToTimeVal(&total, &tv);
        printf("DrawText() white on blue Hello World %d times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
        WaitForAnyButton(&cped);
        DeleteTextState(ts);
        ts = NULL;

        /* Print the whole alphabet */
        ClearScreen(gs, 0.0, 0.0, 0.0);
        fta[0].fta_Pen.pen_BgColor = fta[1].fta_Pen.pen_BgColor = 0x0;
        fta[0].fta_String = "abcdefghijklmnopqrstuvwxyz";
        fta[1].fta_String = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        fta[0].fta_NumChars = fta[1].fta_NumChars = 26;
        err = CreateTextState(&ts, fontI, fta, 2);
        CKERR(err, "DrawFontTextArray()\n");
        memset(&total, 0, sizeof(TimerTicks));
        for (i = 0; i < LOOPCOUNT; i++)
        {
            SampleSystemTimeTT(&in);
            err = DrawText(gs, ts);
            CKERR(err, "DrawText");
            GS_SendList(gs);
            GS_WaitIO(gs);
            SampleSystemTimeTT(&out);
            SubTimerTicks(&in, &out, &difference); /* difference = out - in */
            AddTimerTicks(&difference, &total, &total);
        }
        DrawClipBox(gs, fta);
        ConvertTimerTicksToTimeVal(&total, &tv);
        printf("DrawText() white alphabet %d times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
        WaitForAnyButton(&cped);
        DeleteTextState(ts);
        ts = NULL;

        /* and again, with a blue background */
        ClearScreen(gs, 0.0, 0.0, 0.0);
        fta[0].fta_Pen.pen_BgColor = fta[1].fta_Pen.pen_BgColor = 0x0000ff;
        fta[0].fta_String = "abcdefghijklmnopqrstuvwxyz";
        fta[1].fta_String = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        fta[0].fta_NumChars = fta[1].fta_NumChars = 26;
        err = CreateTextState(&ts, fontI, fta, 2);
        CKERR(err, "DrawFontTextArray()\n");
        memset(&total, 0, sizeof(TimerTicks));
        for (i = 0; i < LOOPCOUNT; i++)
        {
            SampleSystemTimeTT(&in);
            err = DrawText(gs, ts);
            CKERR(err, "DrawText");
            GS_SendList(gs);
            GS_WaitIO(gs);
            SampleSystemTimeTT(&out);
            SubTimerTicks(&in, &out, &difference); /* difference = out - in */
            AddTimerTicks(&difference, &total, &total);
        }
        DrawClipBox(gs, fta);
        ConvertTimerTicksToTimeVal(&total, &tv);
        printf("DrawText() white on blue alphabet %d times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
        WaitForAnyButton(&cped);
        DeleteTextState(ts);
        ts = NULL;

        if (j == 0)
        {
            /* Really clip some of the text out in the next loop */
            printf("Clipping\n");
            fta[0].fta_Clip.min.x = 100;
            fta[0].fta_Clip.min.y = 20;
            fta[0].fta_Clip.max.x = 160;
            fta[0].fta_Clip.max.y = (FBHEIGHT - 15);
            fta[1].fta_Clip.min.x = 40;
            fta[1].fta_Clip.min.y = 25;
            fta[1].fta_Clip.max.x = 100;
            fta[1].fta_Clip.max.y = (FBHEIGHT - 20);
        }
    }

        /* Measure time difference between DrawString() and DrawTextStencil() */
    tsi.tsi_Font = fontI;
    tsi.tsi_MinChar = '0';
    tsi.tsi_MaxChar = '9';
    tsi.tsi_NumChars = 20;
    tsi.tsi_FgColor = 0x00ff00;
    tsi.tsi_BgColor = 0x00cccccc;
    tsi.tsi_reserved = 0;
    err = CreateTextStencil(&tsl, &tsi);
    CKERR(err, "CreateTextStencil()");
    ClearScreen(gs, 0.0, 0.0, 0.0);
    memset(&total, 0, sizeof(TimerTicks));
    for (i = 0; i < LOOPCOUNT; i++)
    {
        pen.pen_X = 70.0;
        pen.pen_Y = 70.0;
        pen.pen_FgColor = 0x0000ff00;
        pen.pen_BgColor = 0x00cccccc;
        SampleSystemTimeTT(&in);
        err = DrawString(gs, fontI, &pen, "01234567890123456789", 20);
        SampleSystemTimeTT(&out);
        CKERR(err, "DrawString() ");
        SubTimerTicks(&in, &out, &difference); /* difference = out - in */
        AddTimerTicks(&difference, &total, &total);
    }
    ConvertTimerTicksToTimeVal(&total, &tv);
    printf("DrawString() with 10 characters %d times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
    memset(&total, 0, sizeof(TimerTicks));
    for (i = 0; i < LOOPCOUNT; i++)
    {
        pen.pen_X = 70.0;
        pen.pen_Y = 100.0;
        SampleSystemTimeTT(&in);
        err = DrawTextStencil(gs, tsl, &pen, "01234567890123456789", 20);
        GS_SendList(gs);
        GS_WaitIO(gs);
        SampleSystemTimeTT(&out);
        CKERR(err, "DrawTextStencil() ");
        SubTimerTicks(&in, &out, &difference); /* difference = out - in */
        AddTimerTicks(&difference, &total, &total);
    }
    ConvertTimerTicksToTimeVal(&total, &tv);
    printf("DrawTextStencil() with 10 characters %d times took %d.%06d seconds\n", LOOPCOUNT, tv.tv_Seconds,tv.tv_Microseconds);
    WaitForAnyButton(&cped);
    DeleteTextStencil(tsl);

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

    printf("newfont done\n");

    return(0);
}
