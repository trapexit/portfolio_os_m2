/* @(#) plane_graphics.c 96/06/21 1.6 */

/*************************************************
** Airplane graphics
**
** Author: Phil Burk
** Copyright 1995, The 3DO Company
**************************************************/

#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/item.h>
#include <kernel/random.h>
#include <kernel/time.h>
#include <misc/event.h>

#include <audio/audio.h>
#include <audio/music.h>

#include <graphics/gp.h>
#include <graphics/fw.h>
#include <graphics/graphics.h>
#include <graphics/pipe/tex.h>
#include <graphics/frame/mod.h>
#include <graphics/frame/char.h>
#include <graphics/font.h>
#include <graphics/frame2d/frame2d.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/frame2d/gridobj.h>
#include <graphics/gfxutils/putils.h>

#include <stdio.h>

#include "plane.h"

/* Handy printing and debugging macros. */
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* #define CULLING_OPTION       GP_Back */
#define CULLING_OPTION       GP_Front
/* #define CULLING_OPTION       GP_None */

/*********************************************************************************/
Err   dgcDrawText( DemoGraphicsContext *dgc, float32 X, float32 Y, char *Text )
{
	PenInfo  pen;
	Err err;

	DBUG(("dgcDrawText: dgc = 0x%x, Text = %s\n", dgc,Text ));
	pen.pen_XScale = pen.pen_YScale = 1.0;
	pen.pen_X = X;
	pen.pen_Y = Y;
	pen.pen_Flags = 0;
	pen.pen_reserved = 0;
	err = DrawTextStencil(GP_GetGState(dgc->dgc_GP), dgc->dgc_TextStencil, &pen, Text, strlen(Text) );
	GS_SendList(GP_GetGState(dgc->dgc_GP));
	GS_WaitIO(GP_GetGState(dgc->dgc_GP));
        
	return err;
}

/********************************************************************************/
Err TermDemoGraphics( DemoGraphicsContext *dgc )
{
	Err Result;
	if(dgc == NULL) return -1;

	if( dgc->dgc_Font ) CloseFont(dgc->dgc_Font);
	if( dgc->dgc_TextState ) DeleteTextState(dgc->dgc_TextState);
	if( dgc->dgc_TextStencil ) DeleteTextStencil(dgc->dgc_TextStencil);

	Result = KillEventUtility();
	if (Result < 0)
	{
		PrintError(0,"KillEventUtility",0,Result);
	}
	return 0;
}

/********************************************************************************/
#define LEFT_TEXT   (0.07 * FBWIDTH)
#define RIGHT_TEXT  (0.52 * FBWIDTH)
#define TOP_TEXT    (0.15 * FBHEIGHT)
#define LINE_HEIGHT (0.07 * FBHEIGHT)
#if IF_DISPLAY_FRAME_RATE
#define STRINGS 4
#else
#define STRINGS 3
#endif

Err InitDemoGraphics( DemoGraphicsContext *dgc, int argc, char **argv )
{
	Color4          tempColor;
	int32           Result;
	AppInitStruct   appData;
	uint32          i;
	FontTextArray   fta[STRINGS];
	TextStencilInfo   tsi;
	char *strings[STRINGS] =
	{
		"Throttle=",
		"Altitude=",
		"AirSpeed=",
#if IF_DISPLAY_FRAME_RATE
		"FrameRate="
#endif
	};
	gfloat posns[STRINGS][2] =
	{
		{
			LEFT_TEXT,  TOP_TEXT + (0 * LINE_HEIGHT)
		},
		{
			LEFT_TEXT,  TOP_TEXT + (1 * LINE_HEIGHT)
		},
		{
                RIGHT_TEXT, TOP_TEXT + (0 * LINE_HEIGHT)
		},
#if IF_DISPLAY_FRAME_RATE
		{
                RIGHT_TEXT, TOP_TEXT + (1 * LINE_HEIGHT)
		}
#endif
	};


	memset( dgc, 0, sizeof(DemoGraphicsContext) );

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, LC_ISFOCUSED);
	if (Result < 0)
	{
		PrintError(0,"InitEventUtility",0,Result);
		goto cleanup;
	}

	Gfx_PipeInit();			/* pipeline initialization */

	CALL_CHECK((GfxUtil_ParseArgs( &argc, argv, &appData )), "GfxUtil_ParseArgs");;

	dgc->dgc_GP = GfxUtil_SetupGraphics( &appData ); /* create a GP */
	if( dgc->dgc_GP == NULL )
	{
		ERR(("InitDemoGraphics: GfxUtil_SetupGraphics() failed.\n"));
		Result = -1;
		goto cleanup;
	}

	CALL_CHECK((GP_Enable(dgc->dgc_GP, GP_Lighting)), "GP_Enable");
	CALL_CHECK((GP_SetHiddenSurf(dgc->dgc_GP, GP_ZBuffer)), "GP_SetHiddenSurf");
	CALL_CHECK((GP_SetCullFaces(dgc->dgc_GP, CULLING_OPTION)), "GP_SetCullFaces");

/* Set background color and ambient light */
	Col_Set( &tempColor, 0.4, 0.2, 0.7, 1.0 );
	CALL_CHECK((GP_SetBackColor(dgc->dgc_GP, &tempColor)), "GP_SetBackColor");
	Col_Set( &tempColor, 0.1, 0.1, 0.1, 1.0 );
	CALL_CHECK((GP_SetAmbient(dgc->dgc_GP, &tempColor)), "GP_SetAmbient");

	CALL_CHECK((Trans_Perspective(GP_GetProjection(dgc->dgc_GP), 45.0, 1.333, 2.5, 200000.0)), "Trans_Perspective");

/* set model view matrix */
	dgc->dgc_ModelView = GP_GetModelView(dgc->dgc_GP);           /* get model/view matrix */

/* Push extra copy onto stack. */
	CALL_CHECK((Trans_Push(dgc->dgc_ModelView)), "Trans_Push");

#if IF_DISPLAY_COCKPIT
	Result = OpenFontFolio();
	if (Result < 0)
	{
		ERR(("pgOpenDisplay: OpenFontFolio() failed: "));
		PrintfSysErr(Result);
		return Result;
	}

	if ((dgc->dgc_Font = OpenFont(SYSTEM_FONT)) < 0)
		ERR(("pgOpenDisplay: Couldn't open font('%s'): err=0x%x\n", "font.file", dgc->dgc_Font ));
#endif

	/* Build the TextState */
	for (i = 0; i < STRINGS; i++)
	{
		memset(&fta[i].fta_Pen, 0, sizeof(PenInfo));
		fta[i].fta_StructSize = sizeof(FontTextArray);
		fta[i].fta_Pen.pen_X = posns[i][0];
		fta[i].fta_Pen.pen_Y = posns[i][1];
		fta[i].fta_Pen.pen_BgColor = 0;
		fta[i].fta_Pen.pen_FgColor = 0xffffff;
		fta[i].fta_Pen.pen_XScale = 1.0;
		fta[i].fta_Pen.pen_YScale = 1.0;
		fta[i].fta_Clip.min.x = 0;
		fta[i].fta_Clip.min.y = 0;
		fta[i].fta_Clip.max.x = (FBWIDTH - 1);
		fta[i].fta_Clip.max.y = (FBHEIGHT - 1);
		fta[i].fta_String = strings[i];
		fta[i].fta_NumChars = strlen(strings[i]);
	}
	Result = CreateTextState(&dgc->dgc_TextState, dgc->dgc_Font, fta, STRINGS);
	if (Result < 0)
	{
		ERR(("Couldn't create textstate\n"));
		goto cleanup;
	}
       	/* Build the TextStencil */
 	tsi.tsi_Font = dgc->dgc_Font;
 	tsi.tsi_MinChar = ' ';
  	tsi.tsi_MaxChar = '9';
 	tsi.tsi_NumChars = 8;
 	tsi.tsi_FgColor = 0x00ffffff;
 	tsi.tsi_BgColor = 0x00000000;
 	tsi.tsi_reserved = 0;
       	Result = CreateTextStencil(&dgc->dgc_TextStencil, &tsi);
	if (Result < 0)
	{
		ERR(("Couldn't create textstencil\n"));
		goto cleanup;
	}

	return 0;

cleanup:
	TermDemoGraphics( dgc );
	return Result;
}

/********************************************************************************/
#define TEXTLEN 80
Err DisplayCockPit( DemoGraphicsContext *dgc, PlaneStatus *plst, float32 frameRate )
{
	int32 Result;
	char StringBuf[64];


#define DISPLAY_VAL(msg,val,xx,yy) \
	sprintf( &StringBuf[0], msg, (val) ); \
	Result = dgcDrawText( dgc, (xx),(yy), StringBuf ); \
	if( Result < 0 ) \
	{ \
		ERR(("dgcDrawText returned 0x%x\n", Result)); \
		printf("String was %s\n", StringBuf); \
		goto cleanup; \
	}

	DrawText(GP_GetGState(dgc->dgc_GP), dgc->dgc_TextState);
	DISPLAY_VAL("%6.1f", plst->plst_Throttle,    LEFT_TEXT + TEXTLEN,  TOP_TEXT + (0 * LINE_HEIGHT));
	DISPLAY_VAL("%6.1f", plst->plst_Altitude,    LEFT_TEXT + TEXTLEN,  TOP_TEXT + (1 * LINE_HEIGHT));
	DISPLAY_VAL("%6.1f", plst->plst_AirVelocity, RIGHT_TEXT + TEXTLEN, TOP_TEXT + (0 * LINE_HEIGHT));

#if IF_DISPLAY_FRAME_RATE
	DISPLAY_VAL("%6.1f", frameRate, RIGHT_TEXT + TEXTLEN, TOP_TEXT + (1 * LINE_HEIGHT));
#else
	TOUCH(frameRate);
#endif

cleanup:
	return Result;
}

/***********************************************
** Draw everything.
*/
Err DrawWorld( DemoGraphicsContext *dgc, PlaneStatus *plst, Terrain *tran )
{
	Point3          tempPoint;
	Err             Result;
	
/* Get extra copy from stack. */
	Trans_Pop(dgc->dgc_ModelView);
	Trans_Push(dgc->dgc_ModelView);

	Pt3_Set( &tempPoint,
		-plst->plst_XPos,
		-plst->plst_Altitude,
		plst->plst_YPos
		 );
	Trans_Translate(dgc->dgc_ModelView, &tempPoint);

#define RADIAN_SCALAR  (180.0/PI)
	Trans_Rotate(dgc->dgc_ModelView, TRANS_YAxis,  (-plst->plst_Yaw * RADIAN_SCALAR) + 90.0);
	Trans_Rotate(dgc->dgc_ModelView, TRANS_ZAxis, plst->plst_Roll * RADIAN_SCALAR);
	Trans_Rotate(dgc->dgc_ModelView, TRANS_XAxis, -plst->plst_Pitch * RADIAN_SCALAR);

	CALL_CHECK((GP_Clear(dgc->dgc_GP, GP_ClearAll)), "GP_Clear");			/* start a frame */

	CALL_CHECK((Trans_Push(dgc->dgc_ModelView)), "Trans_Push");
	
/* Draw Terrain as a QuadMesh primitive. */
	CALL_CHECK((GP_DrawQuadMesh(dgc->dgc_GP, GEO_Colors,
				tran->tran_NumRows, tran->tran_NumCols,
				tran->tran_Vertices, NULL, tran->tran_VertexColors, NULL)),"GP_DrawQuadMesh");
	CALL_CHECK((Trans_Pop(dgc->dgc_ModelView)), "Trans_Pop");

/* Display plane status. */
#if IF_DISPLAY_COCKPIT
	Result = DisplayCockPit( dgc, plst, dgc->dgc_FrameRate );
	if( Result < 0 )
	{
		ERR(("DisplayCockPit returned 0x%x\n", Result));
		goto cleanup;
	}
#endif

	CALL_CHECK((GP_Flush(dgc->dgc_GP)), "GP_Flush");       /* end the frame */
	SwapBuffers();         /* make it visible */
	
cleanup:
	return Result;
}


