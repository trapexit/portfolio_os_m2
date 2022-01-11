
/******************************************************************************
**
**  @(#) showfont.c 96/08/20 1.18
**
******************************************************************************/


#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/time.h>
#include <graphics/font.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/frame2d/frame2d.h>
#include <graphics/frame2d/spriteobj.h>
#include <string.h>
#include <stdio.h>


/*****************************************************************************/


/* general attributes of the view we're gonna be creating */
#define DISPLAY_TYPE   VIEWTYPE_16
#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 240
#define DISPLAY_DEPTH  16


/*****************************************************************************/


void DoExample(GState *gs, Item font, Item view)
{
    PenInfo pen;
    uint32  i;
    StringExtent se;
    Err err;
    char single[2];

    /* clear the frame buffer */
    CLT_ClearFrameBuffer(gs, .0, .0, .6, 0., TRUE, FALSE);
    GS_SendList(gs);
    GS_WaitIO(gs);

    printf("Drawing font 0x%x\n", font);

    /* make the view visible */
    ModifyGraphicsItemVA(view, VIEWTAG_BITMAP, GS_GetDestBuffer(gs),
		               TAG_END);
    CLT_SetSrcToCurrentDest(gs);

    memset(&pen, 0, sizeof(PenInfo));
    pen.pen_XScale = pen.pen_YScale = 1.0;
    pen.pen_X       = 30;
    pen.pen_Y       = 30;
    pen.pen_BgColor = 0x00ffff00;

    pen.pen_FgColor = 0x00ffffff;
    DrawString(gs, font, &pen, "Hello world!", 12);

    pen.pen_FgColor = 0x00ff0000;
    DrawString(gs, font, &pen, "This ", 5);

    pen.pen_FgColor = 0x0000ff00;
    err = GetStringExtent(&se, font, &pen, "is a", 4);
    if (err < 0)
    {
        return;
    }
    DrawString(gs, font, &pen, "is a", 4);

    pen.pen_X       = 30;
    pen.pen_Y       += TEXTHEIGHT(&se);
    pen.pen_BgColor = 0x0000ffff;
    pen.pen_FgColor = 0x00ffff00;
    DrawString(gs, font, &pen, "great example!", 14);

    pen.pen_FgColor = 0x00ff00ff;
    pen.pen_X       = 30;
    pen.pen_Y       += TEXTHEIGHT(&se);
    DrawString(gs, font, &pen, "1234567890", 10);

    pen.pen_X       = 30;
    pen.pen_BgColor = 0;
    pen.pen_Y       += TEXTHEIGHT(&se);
    for (i = 0; i < 16; i++)
    {
        pen.pen_FgColor = ((256 / 16) * (i + 1)) << 16;
        sprintf(single, "%c", ('A' + i));
        DrawString(gs, font, &pen, single, 1);
    }
}


/*****************************************************************************/


int main(int argc, char **argv)
{
Item    font;
Item    bitmaps[1];
GState *gs;
Err     err;
Item    view;
Item    timer;

    if (argc != 2)
    {
	printf("USAGE: showfont <fontName>\n");
	return 0;
    }

    err = OpenGraphicsFolio();
    if (err >= 0)
    {
        err = OpenFontFolio();
        if (err >= 0)
        {
            font = OpenFont(argv[1]);
            if (font >= 0)
            {
                timer = CreateTimerIOReq();
                if (timer >= 0)
                {
                    gs = GS_Create();
                    if (gs)
                    {
                        err = GS_AllocLists(gs, 2, 1024);
                        if (err >= 0)
                        {
                            err = GS_AllocBitmaps(bitmaps, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_TYPE, 1, FALSE);
                            if (err >= 0)
                            {
                                GS_SetDestBuffer(gs, bitmaps[0]);

                                view = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
                                                    VIEWTAG_VIEWTYPE,      DISPLAY_TYPE,
                                                    VIEWTAG_BITMAP,        bitmaps[0],
                                                    TAG_END);
                                if (view >= 0)
                                {
                                    err = AddViewToViewList(view, 0);
                                    if (err >= 0)
                                    {
                                        DoExample(gs,font,view);
                                        WaitTime(timer,5,0);
                                    }
                                    else
                                    {
                                        printf("AddToViewList() failed: ");
                                        PrintfSysErr(err);
                                    }
                                    DeleteItem(view);
                                }
                                else
                                {
                                    printf("CreateItem() of the view failed: ");
                                    PrintfSysErr(view);
                                }
                                GS_FreeBitmaps(bitmaps, 1);
                            }
                            else
                            {
                                printf("GS_AllocBitmap() failed: ");
                                PrintfSysErr(err);
                            }
                            GS_FreeLists(gs);
                        }
                        else
                        {
                            printf("GS_AllocLists() failed: ");
                            PrintfSysErr(err);
                        }
                    }
                    else
                    {
                        printf("GS_Create() failed\n");
                    }
                    DeleteTimerIOReq(timer);
                }
                else
                {
                    printf("CreateTimerIOReq() failed: ");
                    PrintfSysErr(timer);
                }
                CloseFont(font);
            }
            else
            {
                printf("OpenFont(%s) failed: ",argv[1]);
                PrintfSysErr(font);
            }
            CloseFontFolio();
        }
        else
        {
            printf("OpenFontFolio() failed: ");
            PrintfSysErr(err);
        }
        CloseGraphicsFolio();
    }
    else
    {
        printf("OpenGraphicsFolio() failed: ");
        PrintfSysErr(err);
    }

    printf("Showfont done.\n");
    return 0;
}
