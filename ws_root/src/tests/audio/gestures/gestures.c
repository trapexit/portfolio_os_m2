/******************************************************************************
**
**  @(#) gestures.c 96/06/19 1.19
**
** Author: Phil Burk
** Copyright 1995 3DO
**
******************************************************************************/

/**
|||	AUTODOC -private -class tests -group Audio -name Gestures
|||	Capture gestures that control synthesiser patch.
|||
|||	  Format
|||
|||	    Gestures <PatchFileName>
|||
|||	  Description
|||
|||	    This program loads a patch then uses the mouse to control various
|||	    knobs on that patch. There are three main control areas.  One is
|||	    the 2D gesture pad, a large box that you draw shapes into using the
|||	    mouse.  The X and Y values of the shape are mapped to various knobs
|||	    using a second grid.  The third grid is an array of icons.  You
|||	    can select an icon using the B to put it into record mode. Any shape
|||	    you draw in the first grid is then recorded into that icon. If you
|||	    select the icon using the A button then the shape will play back.
|||
|||	  Arguments
|||
|||	    <PatchFileName>
|||	        Binary patch file created using MakePatch.
|||
|||	  Associated Files
|||
|||	    !!!
|||
|||	  Location
|||
|||	    Examples/Audio/Getsures
**/

#if 0
O- ToDo
O- Draw trails of color.
O- Put Freq1 and Amp1 first.
O- Add filter.
#endif

#include <audio/audio.h>
#include <audio/patchfile.h>
#include <audio/music.h>
#include <graphics/graphics.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/types.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <audiodemo/portable_graphics.h>

#define	PRT(x)	 { printf x; }
#define	ERR(x)	   PRT(x)
#define	DBUG(x)	 /* PRT(x) */
/* Debug Recording. */
#define	DBUGR(x) /* PRT(x) */
/* Debug Mouse. */
#define	DBUGM(x) /* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	DBUG(("Did %s\n", name)); \
	if (val < 0) \
	{ \
		result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		PrintfSysErr(result); \
		goto cleanup; \
	}

#define CHECK(expr,msg) { result = (expr); CHECKRESULT(result,msg); }
#define VERSION "0.3"

#define GCAP_NUM_ROWS        (2)
#define GCAP_NUM_COLUMNS     (12)
#define GCAP_NUM_GESTURES    (GCAP_NUM_ROWS*GCAP_NUM_COLUMNS)
#define GCAP_MAX_KNOBS       (12)
#define GCAP_MAX_EVENTS      (60*20)  /* 20 seconds worth */
#define GCAP_POS_SCALAR      ((float32) 0x7FFF)

#define USE_HIGH_RES         (1)
#if USE_HIGH_RES
	#define GCAP_PIXELS_X (640)
	#define GCAP_PIXELS_Y (480)
#else
	#define GCAP_PIXELS_X (320)
	#define GCAP_PIXELS_Y (240)
#endif
#define PIXEL_SIZE  (1.0/GCAP_PIXELS_X)

typedef enum GestureState
{
	GCON_STATE_RESTING,
	GCON_STATE_PLAYING,
	GCON_STATE_RECORDING,
} GestureState;

    /* forward references for ControlGrid */
struct ControlGrid;
typedef struct ControlGrid ControlGrid;
/* Structure used for generic grid of controls. */
typedef Err (*ControlGridMouseFunctionP) ( ControlGrid *cgr, float32 xPos, float32 yPos, uint32 Buttons );
typedef Err (*ControlGridDrawFunctionP)  ( PortableGraphicsContext *pgcon, ControlGrid *cgr );

typedef struct ControlGrid
{
	MinNode   cgr_Node;
	void     *cgr_UserData;
	uint32    cgr_LastButtons;
	int32     cgr_NumRows;
	int32     cgr_NumColumns;
	int32     cgr_CurrentRow;
	int32     cgr_CurrentColumn;
	float32   cgr_TopY;        /* Position of grid. */
	float32   cgr_LeftX;
	float32   cgr_DeltaX;      /* Width and height of grid cells. */
	float32   cgr_DeltaY;
	ControlGridMouseFunctionP  cgr_MouseFunction;
	ControlGridDrawFunctionP   cgr_DrawFunction;
} ControlGrid;

typedef struct ControlScreen
{
	List      cscr_GridList;  /* grids on this screen */
} ControlScreen;

/* Juggler structure for each gesture event. */
typedef struct GestureEvent
{
	Time   gev_TimeStamp;
	uint16 gev_XVal;
	uint16 gev_YVal;
} GestureEvent;

typedef struct GestureCapture *GesCapPtr;

/* Juggler context for each gesture. */
typedef struct GestureContext
{
	int8       gcon_KnobIndex_X;
	int8       gcon_KnobIndex_Y;
	int8       gcon_State;
	AudioTime  gcon_StartTime;
	GesCapPtr  gcon_Capture;
} GestureContext;

typedef struct MouseHandler
{
	Item            maus_EBPortItem;
	Item            maus_MsgPortItem;
	Item            maus_MessageItem;
	Item            maus_EventItem;
	int32           maus_Signal;
	MsgPort        *maus_MsgPort;
	float32         maus_Speed;
	int32           maus_HorizPrevious;
	int32           maus_VertPrevious;
	float32         maus_XPos;
	float32         maus_YPos;
	uint32          maus_Buttons;
} MouseHandler;

typedef struct GestureCapture
{
	ControlScreen   gcap_Screen;
	ControlGrid     gcap_GesturePicker;
	ControlGrid     gcap_GestureBox;
	ControlGrid     gcap_KnobPicker;
	PortableGraphicsContext *gcap_pgcon;
	Item            gcap_PatchTemplate;
	Item            gcap_PatchIns;
	Item            gcap_OutputIns;
	Item            gcap_SleepCue;
	Item            gcap_SleepSignal;
	int32           gcap_NumKnobs;
	MouseHandler   *gcap_maus;
	int32           gcap_RecordingGestureIndex;
	int32           gcap_EnableGraphics;
	Item            gcap_Knobs[GCAP_MAX_KNOBS];
	InstrumentPortInfo gcap_KnobInfo[GCAP_MAX_KNOBS];       /* port info for knob */
	const char     *gcap_KnobNames[GCAP_MAX_KNOBS];
	GestureContext *gcap_KnobOwner[GCAP_MAX_KNOBS];
	GestureContext  gcap_Gestures[GCAP_NUM_GESTURES];
	Sequence       *gcap_Sequences[GCAP_NUM_GESTURES];
	int32           gcap_IfBumpJuggler;    /* Set if there is a new thing added to Juggler. */
} GestureCapture;

Err gcapTweakKnob( GestureCapture *gcap, int32 knobIndex, int32 position );
Err gcapRecordGestureEvent ( GestureCapture *gcap, uint32 ixval, uint32 iyval );

/*****************************************************************
** Draw an empty ControlGrid.
*****************************************************************/
Err cgrDrawEmptyGrid( PortableGraphicsContext *pgcon, ControlGrid *cgr )
{
	int32 result;
	int32 ix, iy;

	result = 0;

/* Draw grid. */
	pgSetColor( pgcon, 1.0, 1.0, 1.0 );
	for( iy=0; iy<cgr->cgr_NumRows+1; iy++)
	{
		pgMoveTo( pgcon, cgr->cgr_LeftX, iy*cgr->cgr_DeltaY + cgr->cgr_TopY );
		pgDrawTo( pgcon, cgr->cgr_NumColumns*cgr->cgr_DeltaX + cgr->cgr_LeftX,
			iy*cgr->cgr_DeltaY + cgr->cgr_TopY );
	}
	for( ix=0; ix<cgr->cgr_NumColumns+1; ix++)
	{
		pgMoveTo( pgcon, ix*cgr->cgr_DeltaX + cgr->cgr_LeftX, cgr->cgr_TopY );
		pgDrawTo( pgcon, ix*cgr->cgr_DeltaX + cgr->cgr_LeftX,
			cgr->cgr_NumRows*cgr->cgr_DeltaY + cgr->cgr_TopY );
	}

	return result;
}

/*****************************************************************
** Fill in a single cell of a ControlGrid in the current color.
*****************************************************************/
void cgrFillGridCell( PortableGraphicsContext *pgcon, ControlGrid *cgr, int32 row, int32 column, float32 Fraction )
{

	float32 X1,Y1, X2,Y2;
	float32 halfWidth = Fraction/2.0;

DBUG(("cgrFillGridCell: row = %d, col = %d\n", row, column));
	X1 = cgr->cgr_LeftX + (column * cgr->cgr_DeltaX) + ((0.5 - halfWidth) * cgr->cgr_DeltaX);
	Y1 = cgr->cgr_TopY + (row * cgr->cgr_DeltaY) + ((0.5 - halfWidth) * cgr->cgr_DeltaY);
	X2 = cgr->cgr_LeftX + (column * cgr->cgr_DeltaX) + ((0.5 + halfWidth) * cgr->cgr_DeltaX);
	Y2 = cgr->cgr_TopY + (row * cgr->cgr_DeltaY) + ((0.5 + halfWidth) * cgr->cgr_DeltaY);
	pgDrawRect( pgcon, X1, Y1, X2, Y2);

/* As a hack, draw X in cell. FIXME */
#ifdef HACK_FILL
	X1 = cgr->cgr_LeftX + (column * cgr->cgr_DeltaX);
	Y1 = cgr->cgr_TopY + (row * cgr->cgr_DeltaY);
	X2 = cgr->cgr_LeftX + ((column+1) * cgr->cgr_DeltaX);
	Y2 = cgr->cgr_TopY + ((row+1) * cgr->cgr_DeltaY);
	pgMoveTo( pgcon, X1, Y1 );
	pgDrawTo( pgcon, X2, Y2 );
	pgMoveTo( pgcon, X2, Y1 );
	pgDrawTo( pgcon, X1, Y2 );
#endif
}

/*****************************************************************
** Which cell within grid did we hit.
*****************************************************************/
Err cgrCalcGridHit( ControlGrid *cgr, float32 xPos, float32 yPos, int32 *rowPtr, int32 *columnPtr )
{
	int32 row, column;

	row = (yPos - cgr->cgr_TopY) / cgr->cgr_DeltaY;
	column = (xPos - cgr->cgr_LeftX) / cgr->cgr_DeltaX;
DBUG(("cgrCalcGridHit: row = %d, col = %d\n", row, column));
	if( (row < 0) || (row >= cgr->cgr_NumRows) ) return -1;
	if( (column < 0) || (column >= cgr->cgr_NumColumns) ) return -1;

	*rowPtr = row;
	*columnPtr = column;
	return 0;
}
/*********************************************************************************/
Err  DrawToDisplayScreen( PortableGraphicsContext *pgcon )
{
	int32 drawScreenIndex;

	drawScreenIndex = pgQueryCurrentDrawScreen( pgcon );
	pgDrawToScreen( pgcon, 1^drawScreenIndex );
	return 0;
}

/*****************************************************************
** Mouse Hook for 2D grid.
*****************************************************************/
Err gcapHandleGestureHook ( ControlGrid *cgr, float32 xPos, float32 yPos, uint32 buttons )
{
	GestureCapture *gcap;
	GestureContext *gcon;
	int32  ri, ix, iy;
	uint32 buttonsUp  = (buttons ^ cgr->cgr_LastButtons) & cgr->cgr_LastButtons;
	uint32 buttonsDown = (buttons ^ cgr->cgr_LastButtons) & buttons;

	gcap = (GestureCapture *) cgr->cgr_UserData;

DBUGR(("gcapHandleGestureHook: x,y,b = %g,%g,0x%x\n", xPos, yPos, buttons));
	cgr->cgr_LastButtons = buttons;

	ri = gcap->gcap_RecordingGestureIndex;
	gcon = &gcap->gcap_Gestures[ri];

/* Start recording. */
	if( buttonsDown & MouseLeft )
	{
		Sequence       *seq;
		gcon->gcon_StartTime = GetAudioTime();
		seq = gcap->gcap_Sequences[ri];
		StopObject( seq, gcon->gcon_StartTime );
		seq->jglr_Many = 0;
		gcon->gcon_State = GCON_STATE_RECORDING;
DBUGR(("Start recording at time 0x%x\n", gcon->gcon_StartTime));
#ifdef TRACE_GESTURE
		gcap->gcap_EnableGraphics = FALSE;
		DrawToDisplayScreen( gcap->gcap_pgcon );
		pgMoveTo( gcap->gcap_pgcon, xPos, yPos );
#endif
	}

	if( buttons & MouseLeft )
	{
/* Translate float x,y into integer. */
		ix = ((xPos - cgr->cgr_LeftX)/ cgr->cgr_DeltaX) * GCAP_POS_SCALAR;
		iy = (1.0 - ((yPos - cgr->cgr_TopY)/ cgr->cgr_DeltaY) ) * GCAP_POS_SCALAR;

		gcap = (GestureCapture *) cgr->cgr_UserData;
		gcapRecordGestureEvent( gcap, ix, iy );
#ifdef TRACE_GESTURE
		pgDrawTo( gcap->gcap_pgcon, xPos, yPos );
#endif
	}

	if( buttonsUp & MouseLeft )
	{
		gcon->gcon_State = GCON_STATE_RESTING;
#ifdef TRACE_GESTURE
		gcap->gcap_EnableGraphics = TRUE;
#endif
	}
	return 0;
}

/*****************************************************************
** Draw 2D gesture box.
*****************************************************************/
Err gcapDrawGestureBox( PortableGraphicsContext *pgcon, ControlGrid *cgr)
{
	cgrDrawEmptyGrid( pgcon, cgr );
	return 0;
}

/*****************************************************************
** Mouse Hook for knob picker grid.
*****************************************************************/
Err gcapPickKnobHook ( ControlGrid *cgr, float32 xPos, float32 yPos, uint32 buttons )
{
	int32 row, column;
	GestureCapture *gcap;
	GestureContext *gcon;
	int32 ri;

	DBUG(("gcapPickKnobHook: %g,%g,0x%x\n", xPos, yPos, buttons ));

	if( buttons & MouseLeft )
	{
		if( cgrCalcGridHit( cgr, xPos, yPos, &row, &column ) < 0 ) return -1;
		DBUG(("gcapPickKnobHook: row = %d, col = %d\n", row, column));

		gcap = (GestureCapture *) cgr->cgr_UserData;
		if( row >= gcap->gcap_NumKnobs ) return 1;
		ri = gcap->gcap_RecordingGestureIndex;
		gcon = &gcap->gcap_Gestures[ri];

		if( column == 0 )
		{
			gcon->gcon_KnobIndex_X = row;
		}
		else
		{
			gcon->gcon_KnobIndex_Y = row;
		}
	}

	return 0;
}

/*****************************************************************
** Draw knob picker grid.
*****************************************************************/
Err gcapDrawKnobPicker( PortableGraphicsContext *pgcon, ControlGrid *cgr)
{
	float32 xPos, yPos;
	GestureCapture *gcap;
	GestureContext *gcon;
	int32 ri;
	int32 i;

	cgrDrawEmptyGrid( pgcon, cgr );

	gcap = (GestureCapture *) cgr->cgr_UserData;

/* Draw names of knobs along side grid. */
	xPos = 0.1;
	yPos = cgr->cgr_TopY + (0.2 * cgr->cgr_DeltaY);
	for( i=0; i<gcap->gcap_NumKnobs; i++ )
	{
		pgMoveTo( pgcon, xPos, yPos );
		pgDrawText( pgcon, gcap->gcap_KnobInfo[i].pinfo_Name );
		yPos += cgr->cgr_DeltaY;
	}

/* Draw the currently selected X,Y mapping. */
	ri = gcap->gcap_RecordingGestureIndex;
DBUG(("gcapDrawKnobPicker: ri = 0x%x\n", ri));
	gcon = &gcap->gcap_Gestures[ri];
	pgSetColor( pgcon, 0.5, 0.9, 0.0 );
	cgrFillGridCell( pgcon, cgr, gcon->gcon_KnobIndex_X, 0, 0.5 );
	cgrFillGridCell( pgcon, cgr, gcon->gcon_KnobIndex_Y, 1, 0.5 );
	return 0;
}

#define CalcGestureIndex(r,c) ( ( (r) * GCAP_NUM_COLUMNS ) + (c) )

/*****************************************************************
** Mouse Hook for gesture picker grid.
*****************************************************************/
Err gcapSelectGesture ( ControlGrid *cgr, float32 xPos, float32 yPos, uint32 buttons )
{
	int32 row, column;
	Err   result;
	GestureCapture *gcap;
	int32 ip;
/*	uint32 buttonsUp  = (buttons ^ cgr->cgr_LastButtons) & cgr->cgr_LastButtons; */
	uint32 buttonsDown = (buttons ^ cgr->cgr_LastButtons) & buttons;

	cgr->cgr_LastButtons = buttons;

	DBUG(("gcapSelectGesture: %g,%g,0x%x\n", xPos, yPos, buttons ));

	if( buttonsDown )
	{
		Sequence *seq;
		result = cgrCalcGridHit( cgr, xPos, yPos, &row, &column );
		if( result < 0 ) return -1;
		DBUG(("gcapPickKnobHook: row = %d, col = %d\n", row, column));
		gcap = (GestureCapture *) cgr->cgr_UserData;
		ip = CalcGestureIndex(row,column);
		seq = gcap->gcap_Sequences[ip];
		if( seq->jglr_Many > 0 )
		{
			StopObject( seq, GetAudioTime() );
		}

		if( buttonsDown & MouseLeft )
		{
			PRT(("Play %d\n", ip ));
			StartObject( seq, GetAudioTime(), 1, NULL );
/* Force start of juggler processing. */
			gcap->gcap_IfBumpJuggler = TRUE;
		}
		if( buttonsDown & MouseMiddle )
		{
			PRT(("Loop %d\n", ip ));
			StartObject( seq, GetAudioTime(), 1000000, NULL );
/* Force start of juggler processing. */
			gcap->gcap_IfBumpJuggler = TRUE;
		}
		if( buttonsDown & MouseRight )
		{
			PRT(("Record %d\n", ip ));
			gcap->gcap_RecordingGestureIndex = ip;
		}
	}

	return 0;
}

/*****************************************************************
** Setup control grids.
*****************************************************************/
Err gcapDrawGestureSelector( PortableGraphicsContext *pgcon, ControlGrid *cgr)
{
	GestureCapture *gcap;
	GestureContext *gcon;
	GestureEvent   *gev;
	Sequence       *seq;
	int32           ig,ip;
	int32           incr;
	float32         x,y;
	float32         x0,y0;
	int32           row,column;

	gcap = (GestureCapture *) cgr->cgr_UserData;

	cgrDrawEmptyGrid( pgcon, cgr );
	pgSetColor( pgcon, 0.0, 1.0, 0.0 );
	for( ig=0; ig<GCAP_NUM_GESTURES; ig++)
	{
		gcon = &gcap->gcap_Gestures[ig];
		row = ig/GCAP_NUM_COLUMNS;
		column = ig%GCAP_NUM_COLUMNS;
		if( gcon->gcon_State == GCON_STATE_PLAYING )
		{
			pgSetColor( pgcon, 0.0, 0.7, 0.0 );
			cgrFillGridCell( pgcon, cgr, row, column, 0.7 );
		}
		else if( gcon->gcon_State == GCON_STATE_RECORDING )
		{
			pgSetColor( pgcon, 0.9, 0.7, 0.0 );
			cgrFillGridCell( pgcon, cgr, row, column, 0.7 );
		}
	}

/* Indicate selected record grid. */
	pgSetColor( pgcon, 0.7, 0.0, 0.0 );
	row = gcap->gcap_RecordingGestureIndex/GCAP_NUM_COLUMNS;
	column = gcap->gcap_RecordingGestureIndex%GCAP_NUM_COLUMNS;
	cgrFillGridCell( pgcon, cgr, row, column, 0.5 );

/* Draw outline of gesture. */
	pgSetColor( pgcon, 0.0, 1.0, 1.0 );
	for( ig=0; ig<GCAP_NUM_GESTURES; ig++)
	{
		seq = gcap->gcap_Sequences[ig];
		if( seq->jglr_Many > 1 )
		{
			incr = seq->jglr_Many / 40;  /* Draw about 40 points. */
			if( incr < 1 ) incr = 1;
			row = ig/GCAP_NUM_COLUMNS;
			column = ig%GCAP_NUM_COLUMNS;
			x0 = cgr->cgr_LeftX + (column * cgr->cgr_DeltaX);
			y0 = cgr->cgr_TopY + ((row+1) * cgr->cgr_DeltaY);
			for( ip=0; ip<seq->jglr_Many; ip += incr )
			{
				gev = ((GestureEvent *)seq->seq_Events) + ip;
				x = x0 + ((gev->gev_XVal / GCAP_POS_SCALAR) * cgr->cgr_DeltaX);
				y = y0 - ((gev->gev_YVal / GCAP_POS_SCALAR) * cgr->cgr_DeltaY);
				if(ip == 0)
				{
					pgMoveTo( pgcon, x0, y0 );
				}
				else
				{
					pgDrawTo( pgcon, x, y );
				}
			}
		}
	}
	return 0;
}
/*****************************************************************
** Setup control grids.
*****************************************************************/


Err gcapInitControl( GestureCapture *gcap )
{
	ControlGrid   *cgr;
	ControlScreen *cscr;

	cscr = &gcap->gcap_Screen;
	PrepList( &cscr->cscr_GridList );

/* Setup 2D gesture control. */
	DBUG(("2D Gesture setup.\n"));
	cgr = &gcap->gcap_GestureBox;
	cgr->cgr_UserData = gcap;
	cgr->cgr_NumRows = 1;
	cgr->cgr_NumColumns = 1;
	cgr->cgr_DeltaX = 0.45;
	cgr->cgr_DeltaY = 0.55;
	cgr->cgr_LeftX = 0.45;
	cgr->cgr_TopY = 0.1;
	cgr->cgr_MouseFunction = gcapHandleGestureHook;
	cgr->cgr_DrawFunction = gcapDrawGestureBox;
	AddTail( &cscr->cscr_GridList, (Node *) cgr );

/* Setup knob index selector. */
	DBUG(("Knob index setup.\n"));
	cgr = &gcap->gcap_KnobPicker;
	cgr->cgr_UserData = gcap;
	cgr->cgr_NumRows = GCAP_MAX_KNOBS;
	cgr->cgr_NumColumns = 2;
	cgr->cgr_DeltaX = 1.0/24.0;
	cgr->cgr_DeltaY = 1.0/25.0;
	cgr->cgr_LeftX = 0.35;
	cgr->cgr_TopY = 0.1;
	cgr->cgr_MouseFunction = gcapPickKnobHook;
	cgr->cgr_DrawFunction = gcapDrawKnobPicker;
	AddTail( &cscr->cscr_GridList, (Node *) cgr );

/* Setup gesture picker. */
	DBUG(("Gesture Picker setup.\n"));
	cgr = &gcap->gcap_GesturePicker;
	cgr->cgr_UserData = gcap;
	cgr->cgr_NumRows = GCAP_NUM_ROWS;
	cgr->cgr_NumColumns = GCAP_NUM_COLUMNS;
	cgr->cgr_DeltaX = 1.0/16.0;
	cgr->cgr_DeltaY = 1.0/10.0;
	cgr->cgr_LeftX = 0.1;
	cgr->cgr_TopY = 0.7;
	cgr->cgr_MouseFunction = gcapSelectGesture;
	cgr->cgr_DrawFunction = gcapDrawGestureSelector;
	AddTail( &cscr->cscr_GridList, (Node *) cgr );

	return 0;
}

/*****************************************************************
** Draw title screen and help.
*****************************************************************/
Err gcapDrawAboutScreen( PortableGraphicsContext *pgcon )
{
	float32 y;

#define LINEHEIGHT (0.1)
#define LEFTMARGIN (0.2)

#define NEXTLINE(msg) \
	pgMoveTo ( pgcon, LEFTMARGIN,y ); \
	y += LINEHEIGHT; \
	TOUCH(y); \
	pgDrawText( pgcon, msg );

DBUG(("gcapDrawAboutScreen()\n"));
	pgSetColor( pgcon, 0, 1.0, 0.5 );
	y = 0.1;
	NEXTLINE("Gestures");
	pgDrawText( pgcon, VERSION );
	NEXTLINE("(c) 3DO, August 1995");
	NEXTLINE("by Phil Burk");
	NEXTLINE("Please wait a few seconds.");

	return 0;
}

/*****************************************************************
** Draw wait screen.
*****************************************************************/
Err gcapDrawWaitScreen( PortableGraphicsContext *pgcon )
{
	float32 y;

DBUG(("gcapDrawWaitScreen()\n"));
	pgSetColor( pgcon, 0, 1.0, 0.5 );
	y = 0.3;
	NEXTLINE("PLease wait...");

	return 0;
}

/***********************************************************************
** Draw screen along with component grids.
*/
Err cscrDrawScreen( PortableGraphicsContext *pgcon, ControlScreen *cscr )
{
	Node *n;
	Err   result = 0;

	n = FirstNode( &cscr->cscr_GridList );
	while( IsNode( &cscr->cscr_GridList, n) )
	{
		result = ((ControlGrid *)n)->cgr_DrawFunction( pgcon, ((ControlGrid *)n) );
		if (result < 0)
		{
			ERR(("cscrDrawScreen: error in DrawFunction\n"));
			PrintfSysErr(result);
			goto cleanup;
		}
		n = NextNode( n );
	}

cleanup:
	return result;
}

/************************************************************************/
Err cscrHandleMouseReport( ControlScreen *cscr, float32 XPos, float32 YPos, uint32 buttons )
{
/* Scan list of screens and look for hits. */
	Node        *n;
/*	float32      xlo, xhi, ylo, yhi; */
	ControlGrid *cgr;
	Err          result = 0;

	n = FirstNode( &cscr->cscr_GridList );
	while( IsNode( &cscr->cscr_GridList, n) )
	{
		cgr = (ControlGrid *) n;
/* Check to see if x,y is hit. */
		if( (XPos >= cgr->cgr_LeftX ) &&
		    (YPos >= cgr->cgr_TopY ) &&
		    (XPos <= (cgr->cgr_LeftX + (cgr->cgr_NumColumns * cgr->cgr_DeltaX))) &&
		    (YPos <= (cgr->cgr_TopY + (cgr->cgr_NumRows * cgr->cgr_DeltaY))) )
		{

			result = ((ControlGrid *)n)->cgr_MouseFunction( ((ControlGrid *)n), XPos, YPos, buttons );
			if (result < 0)
			{
				ERR(("cscrHandleMouseReport: error in MouseFunction\n"));
				PrintfSysErr(result);
				return result;
			}
			break;
		}
		n = NextNode( n );
	}
	return result;
}

/************************************************************************/
int32 gcapStartFunc ( Jugglee *Self, Time StartTime )
{

	GestureContext *gcon;
	GestureCapture *gcap;

	TOUCH(StartTime);

	gcon = Self->jglr_UserContext;
	gcap = gcon->gcon_Capture;

/* Grab control of parameters. */
	gcap->gcap_KnobOwner[ gcon->gcon_KnobIndex_X ] = gcon;
	gcap->gcap_KnobOwner[ gcon->gcon_KnobIndex_Y ] = gcon;

/* Set state for status display. */
	gcon->gcon_State = GCON_STATE_PLAYING;

	return 0;
}

/************************************************************************/
int32 gcapStopFunc ( Jugglee *Self, Time StopTime )
{

	GestureContext *gcon;
	GestureCapture *gcap;

	TOUCH(StopTime);

	gcon = Self->jglr_UserContext;
	gcap = gcon->gcon_Capture;

/* Clear control of parameters. */
	gcap->gcap_KnobOwner[ gcon->gcon_KnobIndex_X ] = NULL;
	gcap->gcap_KnobOwner[ gcon->gcon_KnobIndex_Y ] = NULL;

/* Set state for status display. */
	gcon->gcon_State = GCON_STATE_RESTING;

	return 0;
}

/*************************************************************************
** Translate it into knob coordinates and tweak knob.
*/
Err gcapTweakKnob( GestureCapture *gcap, int32 knobIndex, int32 ival )
{
	float32  val;

	val = ((float32) ival) / GCAP_POS_SCALAR;
DBUG(("gcapTweakKnob: ki = %d, val = %g\n", knobIndex, val ));
	return SetKnob( gcap->gcap_Knobs[ knobIndex ], val );
}

/*************************************************************************
** Tweak knob if owned.
*/
Err gcapTweakByPosition( GestureContext *gcon, uint32 ixval, uint32 iyval )
{
	Err result;
	GestureCapture *gcap = gcon->gcon_Capture;

/* Do we own it? */
	if( gcap->gcap_KnobOwner[ gcon->gcon_KnobIndex_X ] == gcon )
	{
		CHECK( (gcapTweakKnob( gcap, gcon->gcon_KnobIndex_X, ixval )), "gcapTweakKnob");
	}

/* Do we own it? */
	if( gcap->gcap_KnobOwner[gcon->gcon_KnobIndex_Y] == gcon )
	{
		CHECK( (gcapTweakKnob( gcap, gcon->gcon_KnobIndex_Y, iyval )), "gcapTweakKnob");
	}

cleanup:
	return result;
}

/************************************************************************/
int32 gcapInterpFunc ( Jugglee *Self, GestureEvent *gev )
{
	GestureContext *gcon;

	gcon = Self->jglr_UserContext;

	return gcapTweakByPosition( gcon, gev->gev_XVal, gev->gev_YVal );
}

/************************************************************************/
Err gcapRecordGestureEvent ( GestureCapture *gcap, uint32 ixval, uint32 iyval )
{
	GestureEvent *gev;
	GestureContext *gcon;
	int32 ri;
	Sequence  *seq;
	Err result;

DBUGR(("gcapRecordGestureEvent: ix,iy = %d,%d\n", ixval, iyval ));
	ri = gcap->gcap_RecordingGestureIndex;
	gcon = &gcap->gcap_Gestures[ri];
	seq = gcap->gcap_Sequences[ri];
DBUGR(("gcapRecordGestureEvent: seq = 0x%x, many = %d, max = %d\n",
	seq, seq->jglr_Many, seq->seq_Max));

	if( seq->jglr_Many < (seq->seq_Max - 1))
	{
		CHECK( (gcapTweakKnob( gcap, gcon->gcon_KnobIndex_X, ixval )), "gcapTweakKnob");
		CHECK( (gcapTweakKnob( gcap, gcon->gcon_KnobIndex_Y, iyval )), "gcapTweakKnob");

/* Add current event to sequence data. */
		gev = ((GestureEvent *)seq->seq_Events) + seq->jglr_Many;
DBUGR(("gcapRecordGestureEvent: gev = 0x%x, Many = %d\n", gev, seq->jglr_Many));
		gev->gev_TimeStamp = GetAudioTime() - gcon->gcon_StartTime;
DBUGR(("gcapRecordGestureEvent: gev->gev_TimeStamp %d\n", gev->gev_TimeStamp));
		gev->gev_XVal = ixval;
		gev->gev_YVal = iyval;
		seq->jglr_Many++;
	}
	else
	{
		result = 1;
	}
cleanup:
	return result;
}

/************************************************************************/
void gcapTermSound( GestureCapture *gcap )
{
/* Delete patch template and all associated instruments, knobs, etc. */
	UnloadPatchTemplate( gcap->gcap_PatchTemplate );

/* Delete output instrument. */
	UnloadInstrument( gcap->gcap_OutputIns );
}


/************************************************************************/
Err gcapInitGestures( GestureCapture *gcap )
{
	Err   result;
	int32 i,k;
	GestureEvent *gev;
	TagArg        Tags[12];

/* Initialise Juggler. */
	CHECK( (InitJuggler()), "InitJuggler");

	for( i=0; i<GCAP_NUM_GESTURES; i++ )
	{
/* Allocate memory to store gesture data. */
		gev = malloc( GCAP_MAX_EVENTS * sizeof(GestureEvent) );
		if( gev == NULL )
		{
			result = AF_ERR_NOMEM;
			goto cleanup;
		}

/* Create a sequence to contain and play the gesture data. */
		gcap->gcap_Sequences[i] = (Sequence *) CreateObject( &SequenceClass );
		if(gcap->gcap_Sequences[i] == NULL)
		{
			result = AF_ERR_NOMEM;
			goto cleanup;
		}

		k = 0;
		Tags[k].ta_Tag = JGLR_TAG_START_FUNCTION;
		Tags[k++].ta_Arg = (void *) gcapStartFunc;
		Tags[k].ta_Tag = JGLR_TAG_STOP_FUNCTION;
		Tags[k++].ta_Arg = (void *) gcapStopFunc;
		Tags[k].ta_Tag = JGLR_TAG_INTERPRETER_FUNCTION;
		Tags[k++].ta_Arg = (void *) gcapInterpFunc;
		Tags[k].ta_Tag = JGLR_TAG_MAX;
		Tags[k++].ta_Arg = (void *) GCAP_MAX_EVENTS;
		Tags[k].ta_Tag = JGLR_TAG_MANY;
		Tags[k++].ta_Arg = (void *) 0;
		Tags[k].ta_Tag = JGLR_TAG_EVENTS;
		Tags[k++].ta_Arg = (void *) gev;
		Tags[k].ta_Tag = JGLR_TAG_EVENT_SIZE;
		Tags[k++].ta_Arg = (void *) sizeof(GestureEvent);
		Tags[k].ta_Tag = JGLR_TAG_CONTEXT;
		Tags[k++].ta_Arg = (void *) &gcap->gcap_Gestures[i];
		Tags[k].ta_Tag =  0;

		SetObjectInfo(gcap->gcap_Sequences[i], Tags);
DBUGR(("gcapInitGestures: seq[%d] = 0x%x, Events at 0x%x\n", i, gcap->gcap_Sequences[i], gev ));
/* Make each Gesture point back to the main context for the callback functions to use. */
		gcap->gcap_Gestures[i].gcon_Capture = gcap;
		gcap->gcap_Gestures[i].gcon_KnobIndex_X = 0;
		gcap->gcap_Gestures[i].gcon_KnobIndex_Y = 1;
		gcap->gcap_Gestures[i].gcon_State = GCON_STATE_RESTING;

	}

	return result;

cleanup:
	gcapTermSound( gcap );
	return result;
}

/************************************************************************/
Err gcapInitSound( GestureCapture *gcap, char *patchFile )
{
	Err   result;
	int32 i, numPorts;
	InstrumentPortInfo info;

/* Load patch template. */
	CHECK( (gcap->gcap_PatchTemplate = LoadPatchTemplate( patchFile )), "LoadPatchTemplate" );

/* Create instrument from it. */
	CHECK( (gcap->gcap_PatchIns = CreateInstrument( gcap->gcap_PatchTemplate, NULL )), "CreateInstrument" );

/* Load output instrument. */
	CHECK( (gcap->gcap_OutputIns = LoadInstrument( "line_out.dsp", 1, 100 )), "load line_out.dsp" );

/* Connect patch to output. */
	CHECK( (result = ConnectInstrumentParts( gcap->gcap_PatchIns, "Output", 0,
	                 gcap->gcap_OutputIns, "Input", 0 )), "connect to output" );
	ConnectInstrumentParts( gcap->gcap_PatchIns, "Output", 1, gcap->gcap_OutputIns, "Input", 1 );

/* Find out how many ports it has. */
	numPorts  = GetNumInstrumentPorts (gcap->gcap_PatchIns);
	gcap->gcap_NumKnobs = 0;
	for (i=0; i<numPorts; i++)
	{
/* Query for Port Info */
		result = GetInstrumentPortInfoByIndex (&info, sizeof(info), gcap->gcap_PatchIns, i);
		CHECKRESULT(result,"get port info");
/* Is this port a knob? */
		if (info.pinfo_Type == AF_PORT_TYPE_KNOB)
		{
			if (gcap->gcap_NumKnobs >= GCAP_MAX_KNOBS)
			{
				ERR(("gcap->gcap_NumKnobs > GCAP_MAX_KNOBS"));
				result = AF_ERR_OUTOFRANGE;
				goto cleanup;
			}
			gcap->gcap_KnobInfo[gcap->gcap_NumKnobs] = info;
			gcap->gcap_KnobOwner[gcap->gcap_NumKnobs] = NULL;
			CHECK( (gcap->gcap_Knobs[gcap->gcap_NumKnobs] = CreateKnobVA (gcap->gcap_PatchIns,
					gcap->gcap_KnobInfo[gcap->gcap_NumKnobs].pinfo_Name,
					AF_TAG_TYPE, AF_SIGNAL_TYPE_GENERIC_SIGNED, TAG_END)), "CreateKnobVA");
			gcap->gcap_NumKnobs++;
		}
	}

/* Create Cue */
	CHECK( (gcap->gcap_SleepCue = CreateCue( NULL )), "CreateCue" );
	CHECK( (gcap->gcap_SleepSignal = GetCueSignal( gcap->gcap_SleepCue )), "GetCueSignal" );

/* Start Instruments. */
	CHECK( (StartInstrument(gcap->gcap_PatchIns, NULL)), "StartInstrument" );
	CHECK( (StartInstrument(gcap->gcap_OutputIns, NULL)), "StartInstrument" );

	return result;

cleanup:
	gcapTermSound( gcap );
	return result;
}

/************************************************************************/

void DeleteMouseHandler( MouseHandler **mausPtr )
{
	MouseHandler *maus;
	maus = *mausPtr;
	if( maus == NULL ) return;
	DeleteMsg(maus->maus_MessageItem);
	DeleteMsgPort(maus->maus_MsgPortItem);
	*mausPtr = NULL;
	free( maus );
}

/************************************************************************/
Err CreateMouseHandler( MouseHandler **mausPtr )
{
	MouseHandler *maus;
	Err           err;
	ConfigurationRequest  config;

/* Allocate maus structure. */
	maus = malloc( sizeof( MouseHandler ) );
	if( maus == NULL ) return AF_ERR_NOMEM;
	memset( maus, 0, sizeof(MouseHandler) );
	maus->maus_Speed = 0.5;
	*mausPtr = maus;

	maus->maus_EBPortItem = FindMsgPort(EventPortName);
	if (maus->maus_EBPortItem >= 0)
	{
		maus->maus_MsgPortItem = CreateMsgPort(NULL, 0, 0);
		if (maus->maus_MsgPortItem >= 0)
		{
			maus->maus_MessageItem = CreateMsg(NULL, 0, maus->maus_MsgPortItem);
			if (maus->maus_MessageItem >= 0)
			{
				memset(&config,0,sizeof(config));
				config.cr_Header.ebh_Flavor = EB_Configure;
				config.cr_Category          = LC_Observer;
				config.cr_Category = LC_FocusListener;
				config.cr_TriggerMask[0] =
                                          EVENTBIT0_MouseButtonPressed |
                                          EVENTBIT0_MouseButtonReleased |
                                          EVENTBIT0_MouseUpdate |
                                          EVENTBIT0_MouseMoved;

				config.cr_TriggerMask[2] = EVENTBIT2_ControlPortChange |
                                          EVENTBIT2_DeviceChanged;

				config.cr_CaptureMask[2] = 0; /* Was = EVENTBIT2_DetailedEventTiming; !!! */

				err = SendMsg(maus->maus_EBPortItem, maus->maus_MessageItem, &config, sizeof(config));
				if (err >= 0)
				{
					maus->maus_MsgPort = (MsgPort *)LookupItem(maus->maus_MsgPortItem);
					maus->maus_Signal = maus->maus_MsgPort->mp_Signal;

				}
				else
				{
					ERR(("Cannot send message: "));
					PrintfSysErr(err);
					goto cleanup;
				}
			}
			else
			{
				ERR(("Cannot create message: "));
				PrintfSysErr( err = maus->maus_MessageItem);
				goto cleanup;
			}
		}
		else
		{
			ERR(("Cannot create event-listener port: "));
			PrintfSysErr( err = maus->maus_MsgPortItem);
			goto cleanup;
		}
	}
	else
	{
		ERR(("Can't find Event Broker port: "));
		PrintfSysErr( err = maus->maus_EBPortItem);
		goto cleanup;
	}
	DBUG(("CreateMouseHandler: returns 0x%x\n", err));
	return err;

cleanup:
	DeleteMouseHandler( mausPtr );
	return err;
}

/************************************************************************/
static void ExtractMouseInfo( MouseHandler *maus, MouseEventData *med )
{
DBUGM(("ExtractMouseInfo: ix,iy = %d,%d\n", med->med_HorizPosition, med->med_VertPosition ));
/* Make sure mouse stays on screen. */
	maus->maus_XPos += (med->med_HorizPosition - maus->maus_HorizPrevious) * maus->maus_Speed;
	maus->maus_HorizPrevious = med->med_HorizPosition;
	if( maus->maus_XPos < 0.0 )
	{
		maus->maus_XPos = 0.0;
	}
	else if( maus->maus_XPos > (float32)GCAP_PIXELS_X )
	{
		maus->maus_XPos = (float32)GCAP_PIXELS_X;
	}

	maus->maus_YPos += (med->med_VertPosition - maus->maus_VertPrevious) * maus->maus_Speed;
	maus->maus_VertPrevious = med->med_VertPosition;
	if( maus->maus_YPos < 0.0 )
	{
		maus->maus_YPos = 0.0;
	}
	else if( maus->maus_YPos > (float32)GCAP_PIXELS_Y )
	{
		maus->maus_YPos = (float32)GCAP_PIXELS_Y;
	}

	maus->maus_Buttons = med->med_ButtonBits;
DBUGM(("GetMouseReport: x,y,b = %g,%g,0x%x\n",
	maus->maus_XPos, maus->maus_YPos, maus->maus_Buttons ));

}

/************************************************************************/
Err GetMouseReport( MouseHandler *maus )
{
	Item                  eventItem;
	Message              *event;
	EventBrokerHeader    *msgHeader;
	MouseEventData       *med;
	Err                   result = 0;

	while (TRUE)
	{
		eventItem = GetMsg(maus->maus_MsgPortItem);
		if (eventItem < 0)
		{
			ERR(("GetMsg() failed: "));
			PrintfSysErr( result = eventItem);
			goto cleanup;
		}

		if (eventItem == 0) break;

		event     = (Message *) LookupItem(eventItem);
		msgHeader = (EventBrokerHeader *) event->msg_DataPtr;
		if (eventItem == maus->maus_MessageItem)
		{
			if ((int32) event->msg_Result < 0)
			{
				ERR(("Event broker refused configuration request: "));
				PrintfSysErr(result = event->msg_Result);
				goto cleanup;
			}
			DBUG(("GetMouseReport says broker has accepted event-config request\n"));
		}
		else
		{
			switch (msgHeader->ebh_Flavor)
			{
				EventFrame *ef;
				case EB_EventRecord:
					ef = (EventFrame *) (msgHeader + 1);
					while( ef->ef_ByteCount != 0 )
					{
DBUG(("GetMouseReport: ef_ByteCount = %d, ef_EventNumber = %d, ef_GenericPosition = %d\n",
		ef->ef_ByteCount, ef->ef_EventNumber, ef->ef_GenericPosition ));
						switch( ef->ef_EventNumber )
						{
						case EVENTNUM_MouseButtonPressed:
						case EVENTNUM_MouseButtonReleased:
						case EVENTNUM_MouseUpdate:
						case EVENTNUM_MouseMoved:
    							med = (MouseEventData *) ef->ef_EventData;
							ExtractMouseInfo( maus, med );
							break;

						default:
							break;
						}

        					ef = (EventFrame *) (ef->ef_ByteCount + (char *) ef);
					}
					break;

				default:
					DBUG(("Got event-message type %d\n", msgHeader->ebh_Flavor));
					break;
			}
			ReplyMsg(eventItem, 0, NULL, 0);
		}
	}
cleanup:
	return result;
}

Err DrawMouseCursor( PortableGraphicsContext *pgcon, MouseHandler *maus   )
{

	float32 XPos, YPos;

	XPos = maus->maus_XPos / GCAP_PIXELS_X;
	YPos = maus->maus_YPos / GCAP_PIXELS_Y;

	pgSetColor( pgcon, 1.0, 0.5, 0.5 );
	pgMoveTo( pgcon, XPos , YPos );
	pgDrawTo( pgcon, XPos + 0.03, YPos + 0.02 );
	pgDrawTo( pgcon, XPos + 0.02, YPos + 0.03 );
	pgDrawTo( pgcon, XPos , YPos );
	return 0;
}

/***********************************************************************/
Err gcapThread( void )
{
	int32 result;
	GestureCapture *gcap;
	PortableGraphicsContext *pgcon;

	gcap = CURRENTTASK->t_UserData;
	PRT(("gcap = 0x%x\n", gcap));

/* Set up double buffered display. */
	result = pgCreateDisplay( &pgcon, 2 );
	CHECKRESULT(result,"pgCreateDisplay");
	gcap->gcap_pgcon = pgcon;
	gcap->gcap_EnableGraphics = TRUE;

/* Draw hello. */
	pgClearDisplay( pgcon, 0.0, 0.0, 0.0 );
	gcapDrawAboutScreen( pgcon );
	pgSwitchScreens( pgcon );

	while(1)
	{
/* Generate the next video image. */
		pgClearDisplay(  gcap->gcap_pgcon, 0.0, 0.0, 0.0 );

/* Draw control screen. */
		result = cscrDrawScreen( gcap->gcap_pgcon, &gcap->gcap_Screen );
		CHECKRESULT( result, "cscrDrawScreen");
		result = DrawMouseCursor( gcap->gcap_pgcon, gcap->gcap_maus );
		CHECKRESULT( result, "DrawMouseCursor");

/* Switch double buffered screens. */
		result = pgSwitchScreens(  gcap->gcap_pgcon );
		CHECKRESULT( result, "pgSwitchScreens");
	}

cleanup:
	pgDeleteDisplay( &gcap->gcap_pgcon );
	return result;
}

/************************************************************************/
Err gcapInteractiveLoop( GestureCapture *gcap )
{
	int32      signalMask, signals, nextSignals;
	float32    XPos, YPos;
	uint32     buttons;
	AudioTime  nextTime;
	Err        result;
	MouseHandler *maus = gcap->gcap_maus;

	signalMask = gcap->gcap_SleepSignal | maus->maus_Signal;
DBUG(("gcapInteractiveLoop: signalMask = 0x%x\n", signalMask ));
	while( TRUE )
	{
/* Wait for signal from mouse or timer. */
		signals = WaitSignal( signalMask );
		CHECKRESULT( signals, "WaitSignal");
DBUG(("gcapInteractiveLoop: got signal = 0x%x\n", signals ));

/* If mouse signal, get position and button info. */
		if( signals & maus->maus_Signal )
		{

			result = GetMouseReport( maus );
			CHECKRESULT( result, "GetMouseReport");
/* Pull values out of mouse structure. */
			XPos = maus->maus_XPos / GCAP_PIXELS_X;
			YPos = maus->maus_YPos / GCAP_PIXELS_Y;
			buttons = maus->maus_Buttons;

/* Bail? */
			if( (buttons & (MouseLeft|MouseMiddle|MouseRight)) == (MouseLeft|MouseMiddle|MouseRight)) return 0;
			result = cscrHandleMouseReport( &gcap->gcap_Screen,
					XPos, YPos, buttons );
			CHECKRESULT( result, "cscrHandleMouseReport");
		}

/* If timer signal, bump Juggler. */
		if( (signals & gcap->gcap_SleepSignal) || gcap->gcap_IfBumpJuggler )
		{
			gcap->gcap_IfBumpJuggler = FALSE;
#if 0
			if( (signals & gcap->gcap_SleepSignal) == 0)
			{
#endif
				AbortTimerCue( gcap->gcap_SleepCue );
#if 0
			}
#endif
			result = BumpJuggler( GetAudioTime(), &nextTime, 0, &nextSignals );
			CHECKRESULT( result, "BumpJuggler");

			if( result == 0)
			{
				result = SignalAtTime( gcap->gcap_SleepCue, nextTime );
				CHECKRESULT( result, "SignalAtTime");
			}
		}

		
	}

cleanup:
	return result;
}

/************************************************************************/
void gcapTerm( GestureCapture *gcap )
{
	DeleteMouseHandler( &gcap->gcap_maus );
	gcapTermSound( gcap );
}

/************************************************************************/
Err gcapInit( GestureCapture *gcap, char *patchFile )
{
	Err result;

	memset(gcap, 0, sizeof(GestureCapture));

/* Initialize audio, return if error. */
	if ( (result = OpenAudioFolio()) < 0 )
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(result);
	}

/* Initialize the Mouse. */
	result = CreateMouseHandler( &gcap->gcap_maus );
	CHECKRESULT( result, "CreateMouseHandler");

/* Load patch. */
	result = gcapInitSound( gcap, patchFile );
	CHECKRESULT( result, "gcapInitSound");

/* Initialise gestures and juggler. */
	result = gcapInitGestures( gcap );
	CHECKRESULT( result, "gcapInitGestures");

/* Init Control Grids */
	result = gcapInitControl( gcap );
	CHECKRESULT( result, "gcapInitControl");

	return result;

cleanup:
	gcapTerm( gcap );
	return result;
}

/************************************************************************/

int main( int argc, char *argv[] )
{
	char *progname;
	char *patchFile;
	int32 result;
	GestureCapture  myCapture;
	Item  graphicsThread = 0;

/* Get arguments from command line. */
	progname = argv[0];
	patchFile = (argc > 1) ? argv[1] : "gestures.patch";

	PRT(( "\n%s %s by Phil Burk\n", progname, VERSION ));
	PRT(( "  Patch file is %s\n", patchFile ));
	PRT(( "Copyright 1995 3DO\n" ));

/* Initialize DrumBox structure. */
	if ( (result = gcapInit(  &myCapture, patchFile )) < 0 ) goto cleanup;

/* Spawn graphics thread. */
	if ( (graphicsThread = CreateThreadVA(
		(void (*)())gcapThread, "gestureGraphics",
		50, 4096,
		CREATETASK_TAG_USERDATA, (void *) &myCapture,
		TAG_END)) < 0 ) goto cleanup;

/* Play gestures. */
	gcapInteractiveLoop( &myCapture );

cleanup:
	DeleteThread(graphicsThread);
	gcapTerm( &myCapture );
	PRT(( "\n%s finished!\n", progname ));
	return( (int) result );
}

