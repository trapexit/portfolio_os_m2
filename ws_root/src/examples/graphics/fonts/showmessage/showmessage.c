
/******************************************************************************
**
**  @(#) showmessage.c 96/08/20 1.15
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
#include <misc/event.h>
#include <string.h>
#include <stdio.h>


/*****************************************************************************/


/* general attributes of the view we're gonna be creating */
#define DISPLAY_TYPE   VIEWTYPE_16
#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 240
#define DISPLAY_DEPTH  16

#define	DEFAULT_FONT	"default_14"


/*****************************************************************************/

void
WaitForExit(void)
{
	ControlPadEventData cped;

	if (InitEventUtility (1, 0, LC_ISFOCUSED) < 0)
	{
		printf("Error in InitEventUtility\n");
		return;
	}

	for (;;)
	{
		GetControlPad (1, FALSE, &cped);
		/* if any button event, then break */
		if (cped.cped_ButtonBits) break;
	}
	KillEventUtility();
}


int
main(int argc, char **argv)
{
	Item font;
	Item bitmaps[1];
	GState *gs;
	Err err;
	Item view;
	PenInfo pen;
	uint32 i;

	err = OpenGraphicsFolio();
	if (err < 0)
	{
		printf("OpenGraphicsFolio() failed: ");
		PrintfSysErr(err);
		return 1;
	}
	err = OpenFontFolio();
	if (err < 0)
	{
		printf("OpenFontFolio() failed: ");
		PrintfSysErr(err);
		return 1;
	}
	font = OpenFont(DEFAULT_FONT);
	if (font < 0)
	{
		printf("OpenFont(%s) failed: ", DEFAULT_FONT);
		PrintfSysErr(font);
		return 1;
	}

	gs = GS_Create();
	if (gs == NULL)
	{
		printf("GS_Create() failed\n");
		return 1;
	}

	err = GS_AllocLists(gs, 2, 1024);
	if (err < 0)
	{
		printf("GS_AllocLists() failed: ");
		PrintfSysErr(err);
		return 1;
	}

	err = GS_AllocBitmaps(bitmaps, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_TYPE, 1, FALSE);
	if (err < 0)
	{
		printf("GS_AllocBitmaps() failed: ");
		PrintfSysErr(err);
		return 1;
	}

	GS_SetDestBuffer(gs, bitmaps[0]);

	view = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
			    VIEWTAG_VIEWTYPE,      DISPLAY_TYPE,
			    VIEWTAG_BITMAP,        GS_GetDestBuffer(gs),
			    TAG_END);

	if (view < 0)
	{
		printf("CreateItem() of the view failed: ");
		PrintfSysErr(view);
		return 1;
	}
	err = AddViewToViewList(view, 0);
	if (err < 0)
	{
		printf("AddToViewList() failed: ");
		PrintfSysErr(err);
		return 1;
	}

	/* clear the frame buffer */
	CLT_ClearFrameBuffer(gs, .0, .0, .6, 0., TRUE, FALSE);
	GS_SendList(gs);
	GS_WaitIO(gs);

	/* make the view visible */
	ModifyGraphicsItemVA(view,
		VIEWTAG_BITMAP, GS_GetDestBuffer(gs),
		TAG_END);

	CLT_SetSrcToCurrentDest(gs);

        memset(&pen, 0, sizeof(PenInfo));
        pen.pen_XScale = pen.pen_YScale = 1.0;
	for (i = (argv[0][0] != '-') ? 0 : 1;  i < argc;  i++)
	{
		pen.pen_X = 30;
		pen.pen_Y = 30 + (i * 20);
		pen.pen_BgColor = 0;
		pen.pen_FgColor = 0x00FC0000;
		DrawString(gs, font, &pen, argv[i], strlen(argv[i]));
	}

	WaitForExit();

	GS_FreeBitmaps(bitmaps, 1);
	GS_FreeLists(gs);
	GS_Delete(gs);
	DeleteItem(view);
	CloseFont(font);
	CloseFontFolio();
        CloseGraphicsFolio();
	return 0;
}
