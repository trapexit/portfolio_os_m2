
/******************************************************************************
**
**  @(#) discjuggler.c 96/03/18 1.9 (based on showmessage.c)
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
#include <device/cdrom.h>
#include <string.h>
#include <stdio.h>

extern const char *RomAppMediaType(void);
extern Err InitEventStuff (int32 numControlPads, int32 isFocused);
extern Err GetEventStuff(int32 padNumber, bool wait, ControlPadEventData *data, uint32 *deviceChanged);

#define	DEFAULT_FONT	"default_14"

#define CHK4ERR(x,str)	if (x < 0) { printf str; PrintfSysErr(x); return (x); }

/*****************************************************************************/


/* general attributes of the view we're gonna be creating */
#define DISPLAY_TYPE   VIEWTYPE_16
#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 240
#define DISPLAY_DEPTH  16


/*****************************************************************************/

Err
WaitForExit(char *discType)
{
	ControlPadEventData cped;
	uint32		devChanged;
	Err		err;

	err = InitEventStuff (1, LC_ISFOCUSED);
	CHK4ERR(err, ("Error in InitEventUtility\n"));

	for (;;)
	{
		GetEventStuff (1, FALSE, &cped, &devChanged);
		/* if any button event, then break */
		if (cped.cped_ButtonBits) break;
		/* If this is the NoCD app, and a disc is present, then break */
		if (devChanged)
		{
			if (discType != NULL && 
			    strcmp(RomAppMediaType(), discType) != 0)
				break;
		}
	}

	KillEventUtility();
	return (0);
}


int 
main(int argc, char **argv)
{
	Item font;
	Item bitmaps[2];
	GState *gs;
	Err err;
	Item view;
	PenInfo pen;
	uint32 i;

#ifdef BUILD_NOREBOOT
	BeginNoReboot();
#endif
#ifdef BUILD_EXPECTDATADISC
	ExpectDataDisc();
#endif

	err = OpenGraphicsFolio();
	CHK4ERR(err, ("OpenGraphicsFolio() failed: "));

	err = OpenFontFolio();
	CHK4ERR(err, ("OpenFontFolio() failed: "));

	font = OpenFont(DEFAULT_FONT);
	CHK4ERR(font, ("OpenFont(%s) failed: ", DEFAULT_FONT));

	gs = GS_Create();
	if (gs == NULL)
	{
		printf("GS_Create() failed\n");
		return 1;
	}

	/* FIXME: Add error checking */
	GS_AllocLists(gs, 2, 1024);
	GS_AllocBitmaps(bitmaps, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_TYPE, 1, 1);
	GS_SetDestBuffer(gs, bitmaps[0]);
	GS_SetZBuffer(gs, bitmaps[1]);

	view = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
			    VIEWTAG_VIEWTYPE,      DISPLAY_TYPE,
			    VIEWTAG_BITMAP,        GS_GetDestBuffer(gs),
			    TAG_END);
	CHK4ERR(view, ("CreateItem() of the view failed: "));

	err = AddViewToViewList(view, 0);
	CHK4ERR(err, ("AddToViewList() failed: "));

	/* clear the frame buffer */
	CLT_ClearFrameBuffer(gs, .0, .0, .6, 0., TRUE, TRUE);
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

	pen.pen_X = 30;
	pen.pen_Y += 40;
#if defined(BUILD_NOREBOOT) || defined(BUILD_EXPECTDATADISC)
	DrawString(gs, font, &pen, "No reboot on eject", 18);
#else
	DrawString(gs, font, &pen, "Reboots on eject", 16);
#endif

	pen.pen_X = 30;
	pen.pen_Y += 40;
	DrawString(gs, font, &pen, "No reboot on romapp insert", 26);

	pen.pen_X = 30;
	pen.pen_Y += 20;
#ifdef BUILD_EXPECTDATADISC
	DrawString(gs, font, &pen, "No reboot on title/data insert", 30);
#else
	DrawString(gs, font, &pen, "Reboots on title insert", 23);
#endif

	pen.pen_X = 30;
	pen.pen_Y += 40;
	DrawString(gs, font, &pen, "(press any button to exit)", 26);

	WaitForExit(argc > 1 ? argv[1] : NULL);

	GS_FreeBitmaps(bitmaps, 2);
	GS_FreeLists(gs);
	GS_Delete(gs);
	DeleteItem(view);
	CloseFont(font);
	CloseFontFolio();
        CloseGraphicsFolio();

#ifdef BUILD_NOREBOOT
	EndNoReboot();
#endif
#ifdef BUILD_EXPECTDATADISC
	NoExpectDataDisc();
#endif

	return 0;
}
