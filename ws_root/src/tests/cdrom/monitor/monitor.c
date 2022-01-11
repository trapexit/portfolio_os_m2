/* @(#) monitor.c 96/04/03 1.2 */

/*
	File:		monitor.c

	Contains:	Used to test the M2 cd-rom driver/device.

	Copyright:	©1995 by The 3DO Company, all rights reserved.
*/


#include <kernel/driver.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <kernel/debug.h>
#include <kernel/random.h>
#include <kernel/operror.h>
#include <device/m2cd.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/devicecmd.h>
#include <kernel/msgport.h>
#include <graphics/font.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/frame2d/frame2d.h>
#include <graphics/frame2d/spriteobj.h>

#define	DEBUG	1

#if DEBUG
	#define DBUG(x) kprintf x
#else
	#define DBUG(x)
#endif

#define	CHK4ERR(x,str)	if ((x)<0) { DBUG(str); PrintfSysErr(x); return (x); }

#define BCD2BIN(bcd)	((((bcd) >> 4) * 10) + ((bcd) & 0x0F))
#define BIN2BCD(bin)	((((bin) / 10) << 4) | ((bin) % 10))

extern uint32 SectorECC(uint8 *);

#define kStrCmd			"Cmd:                  "
#define kStrNull		"                   "

#define DISPLAY_TYPE    VIEWTYPE_16
#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240
#define DISPLAY_DEPTH   16
#define DEFAULT_FONT    "default_14"
#define SALTPILE_FONT   "saltpile_12"

static char		fontTable[][20] = { DEFAULT_FONT, SALTPILE_FONT };
static GState	*gGS = NULL;
static Item		gFont, gView, gPort;
static PenInfo	gPen;

void MyDrawString(const char str[], uint32 x, uint32 y)
{
	gPen.pen_X = x;
	gPen.pen_Y = y;

	DrawString(gGS, gFont, &gPen, str, strlen(str));
}

uint8 *cmd2str(uint8 cmd, uint8 str[])
{
	switch (cmd)
	{
		case CDE_DIPIR_BYTE:    sprintf(str, "DipirByte     ");      break;
		case CD_LED:            sprintf(str, "LED           ");      break;
		case CD_SETSPEED:       sprintf(str, "SetSpeed      ");      break;
		case CD_SPINDOWNTIME:   sprintf(str, "SpinDown      ");      break;
		case CD_SECTORFORMAT:   sprintf(str, "Format        ");      break;
		case CD_CIPREPORTEN:    sprintf(str, "CIPRptEn      ");      break;
		case CD_QREPORTEN:      sprintf(str, "QRptEn        ");      break;
		case CD_SWREPORTEN:     sprintf(str, "SwRptEn       ");      break;
		case CD_SENDBYTES:      sprintf(str, "SendBytes     ");      break;
		case CD_PPSO:           sprintf(str, "PPSO          ");      break;
		case CD_SEEK:           sprintf(str, "Seek          ");      break;
		case CD_CHECKWO:        sprintf(str, "CheckWO       ");      break;
		case CD_MECHTYPE:       sprintf(str, "MechType      ");      break;
		default:                sprintf(str, "<UNKNOWN>     ");      break;
	}   

	return (str);
}

uint8 *byte2str(uint8 byte, uint8 str[])
{
	sprintf(str, "%02X", byte);
	return (str);
}

void HandleMsg(Item theMsg)
{
	Message *msg;
	uint32 mode, data, x, y;
	char str[15];

	const char cip2str[][15] =
	{
		"Open          ",
		"Stop          ",
		"Pause         ",
		"Play          ",
		"Opening       ",
		"Stuck         ",
		"Closing       ",
		"Stop & Focused",
		"Stopping      ",
		"Focusing      ",
		"FocusError    ",
		"SpinningUp    ",
		"Unreadable    ",
		"Seeking       ",
		"SeekFailure   ",
		"Latency       " 
	};

	const char sw2str[][4] =
	{
		"uco",
		"ucO",
		"uCo",
		"uCO",
		"Uco",
		"UcO",
		"UCo",
		"UCO"
	};
	
	msg = (Message *)LookupItem(theMsg);

	mode = (((msg->msg_Val1) >> 16) & 0xFFFF);
	data = ((msg->msg_Val1) & 0xFFFF);
	x = (((msg->msg_Val2) >> 16) & 0xFFFF);
	y = ((msg->msg_Val2) & 0xFFFF);
	
	switch (mode)
	{
		case 0:		MyDrawString(byte2str(data,str), x, y);		break;
		case 1:		MyDrawString(cip2str[data], x, y);			break;
		case 2:		MyDrawString(sw2str[data], x, y);			break;
		case 3:		MyDrawString(cmd2str(data,str), x, y);		break;
		case 4:		MyDrawString(kStrCmd, x, y);				break;
		case 5:		MyDrawString(kStrNull, x, y);				break;
		default:	kprintf("Unknown mode!\n");					break;
	}

	ReplySmallMsg(theMsg, 0, 0, 0);
}

Err WaitForExit(void)
{
	ControlPadEventData	cped;
	Err					err;
	Item				theMsg;
					 
	err = InitEventUtility (1, 0, LC_ISFOCUSED);
	CHK4ERR(err, ("Error in InitEventUtility\n"));
							  
	for (;;)
	{
		theMsg = GetMsg(gPort);
		if (theMsg > 0)
			HandleMsg(theMsg);

		/* if any button event, then break */
		GetControlPad (1, FALSE, &cped);
		if (cped.cped_ButtonBits) break;
	}

	KillEventUtility();

	return (0);
}

Err InitGraphics(uint32 fontID, uint32 rgbColor)
{
	Item bitmaps[2];
	Err err;

	err = OpenGraphicsFolio();
	CHK4ERR(err, ("OpenGraphicsFolio() failed\n"));

	err = OpenFontFolio();
	CHK4ERR(err, ("OpenFontFolio() failed\n"));
							 
	gFont = OpenFont(fontTable[fontID]);
	CHK4ERR(err, ("OpenFont() failed\n"));

	gGS = GS_Create();
	if (!gGS)
	{
		printf("GS_Create() failed\n");
		return 1;
	}

	GS_AllocLists(gGS, 2, 1024);
	GS_AllocBitmaps(bitmaps, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_TYPE, 1, 1);
	GS_SetDestBuffer(gGS, bitmaps[0]);
	GS_SetZBuffer(gGS, bitmaps[1]);

	gView = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
				VIEWTAG_VIEWTYPE, DISPLAY_TYPE,
				VIEWTAG_BITMAP, GS_GetDestBuffer(gGS),
				TAG_END);
	CHK4ERR(gView, ("CreateItem() of the view failed\n"));

	err = AddViewToViewList(gView, 0);
	CHK4ERR(err, ("AddToViewList() failed\n"));

	/* clear the frame buffer */
	CLT_ClearFrameBuffer(gGS, .0, .0, .6, 0., TRUE, TRUE);
	GS_SendList(gGS);
	GS_WaitIO(gGS);

	/* make the view visible */
	ModifyGraphicsItemVA(gView, VIEWTAG_BITMAP, GS_GetDestBuffer(gGS), TAG_END);

	CLT_SetSrcToCurrentDest(gGS);

	memset(&gPen, 0, sizeof(gPen));
	gPen.pen_BgColor = 1;
	gPen.pen_FgColor = rgbColor;
	gPen.pen_XScale = gPen.pen_YScale = 1.0;

	return 0;
}

void DeinitGraphics(void)
{
	DeleteItem(gView);
	CloseFont(gFont);
	CloseFontFolio();
	CloseGraphicsFolio();
}

int main(int32 argc,char *argv[])
{
	Err	err;
	Item	devItem, iorItem;
	IOInfo	ioInfo;
	uint32	fontID = 1;				/* mono-space courier */
	uint32	rgbColor = 0x0000FF00;	/* green */
	
	switch (argc)
	{
		case 3:
			rgbColor = strtol(argv[2],0,16);
		case 2:
			if ((argv[1][0] == 'h') || (argv[1][0] == '?'))
				goto help;
			fontID = strtol(argv[1],0,10);
			break;
		case 1:
			break;
		default:
help:
			kprintf("\nmonitor:  usage monitor <fontID> <rgbColor>\n");
			return (0);
	}

	memset(&ioInfo,0,sizeof(ioInfo));

	devItem = OpenNamedDeviceStack("cdrom");
	CHK4ERR(devItem, ("M2CD:  >ERROR<  Couldn't open CD-ROM device\n"));
	
	iorItem = CreateIOReq(0,0,devItem,0);
	CHK4ERR(iorItem, ("M2CD:  >ERROR<  Couldn't create ioReqItem\n"));
	
	/* UserData != 0 means enable monitor, allocate messages */
	ioInfo.ioi_Command = CDROMCMD_MONITOR;
	ioInfo.ioi_UserData = (void *)1;

	err = InitGraphics(fontID, rgbColor);
	CHK4ERR(err, ("\nM2CD:  >ERROR<  Problem in InitGraphics()\n"));

	gPort = CreateMsgPort("M2CD Monitor",0,0);
	CHK4ERR(gPort, ("\nM2CD:  >ERROR<  Problem in CreateMsgPort()\n"));
	
	err = DoIO(iorItem, &ioInfo);
	CHK4ERR(err, ("\nM2CD:  >ERROR<  Problem in DoIO(setup)\n"));

	err = WaitForExit();
	CHK4ERR(err, ("\nM2CD:  >ERROR<  Problem in WaitForExit\n"));
	
	/* UserData == 0 means disable monitor, deallocate messages */
	ioInfo.ioi_UserData = NULL;
	err = DoIO(iorItem, &ioInfo);
	CHK4ERR(err, ("\nM2CD:  >ERROR<  Problem in DoIO(teardown)\n"));

	kprintf("done\n");

	DeleteItem(iorItem);
	CloseDeviceStack(devItem);
		
	return (0);
}
