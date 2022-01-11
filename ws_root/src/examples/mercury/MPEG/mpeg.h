
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/task.h>
#include <graphics/frame2d/frame2d.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/font.h>
#include <graphics/clt/clt.h>
#include <graphics/clt/cltmacros.h>
#include <graphics/clt/clttxdblend.h>
#include <graphics/blitter.h>
#include <graphics/clt/gstate.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <file/fileio.h>
#include <kernel/operror.h>
#include <kernel/mem.h>
#include <device/mpegvideo.h>
#include <misc/event.h>
#include <misc/frac16.h>
#include <kernel/random.h>
#include <kernel/time.h>
#include <math.h>

#define YES		1
#define NO		0
#define CHECK_RES(str, val)			\
	if (val < 0)					\
	{								\
		printf("%s\n", str);		\
		PrintfSysErr(val);			\
		return (val);				\
	}

/* MPEG STUFF */
typedef struct {
	void	*data;
	int32	size;
	int32	stillID;
} MPEGStillInfo;
Item	mpegDevItem = -1;
Item	vreadItem = -1;
Item	vwriteItem1 = -1;
Item	vwriteItem2 = -1;
Item	vwriteItem3 = -1;
Item	eosItem = -1;

MPEGStillInfo MStill;
/************************************************/

bool InGameLoop = YES;

/* Function Protos */
Err
	MPS_InitMPEGStill(void), MPS_DrawMPEGStill(GState *, Item , char *, int32 ),
	MPS_TeardownMPEGStill(void), MPS_LoadMPEGStill(char *, void **, int32 *)
	;
void
	MPS_FreeMPEGStill(void *data),
	SetUpDisplay(void), GeneralInit(void), 
	CalcFrameTime(void), WaitForAnyButton(ControlPadEventData *)
	;

/* General Necessities */
GState	*gs;
uint32	renderSig;
Item	viewItem;
Item	bitmaps[3];
Item	zbuffer;
long	gScreenSelect = 1;
Bitmap	*fbp;

int32 ContinueBits[8] = {0,0,0,0,0,0,0,0}, Error[8] = {0,0,0,0,0,0,0,0};
uint32 LastBits[8] = {0,0,0,0,0,0,0,0};
ControlPadEventData PodControl[8];
long newievent, ievent;

TimeVal		prevTV, tv, curTV, frametv, _frametv;
Item			font;
PenInfo			pen;
StringExtent	se;
char *GameFontName[5] = NULL;
char txtBuffer[360];

float _frametime;
TimerTicks prevTT, curTT, resultTT;
