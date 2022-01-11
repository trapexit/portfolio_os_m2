
/******************************************************************************
**
**  @(#) drumbox.c 96/08/27 1.13
**
******************************************************************************/

/**
|||	NOAUTODOC -public -class examples -group Audio -name DrumBox    !!! temp disabled
|||	Simple single-pattern drum machine.
|||
|||	  Format
|||
|||	    DrumBox <PIMap>
|||
|||	  Description
|||
|||	    This program loads and plays up to eight rhythm sounds in
|||	    a repeating measure of sixteen beats. Use the D-pad and the
|||	    A or B button to place or remove a sound from a specific point
|||	    in the rhythm.
|||
|||	    To use this program, it is necessary to copy the AIFF Samples
|||	    folder into the /remote/Samples folder so that this program can use
|||	    them.
|||
|||	  Arguments
|||
|||	    <PIMap>
|||	        The path and filename of a PIMap file to use. A standard "drumbox.pimap"
|||	        is provided, so try using drumbox.pimap as the argument to drumbox.
|||
|||	  Associated Files
|||
|||	    drumbox.c, drumbox.pimap
|||
|||	  Location
|||
|||	    Examples/Audio/DrumBox
|||
**/

#include <audio/audio.h>
#include <audio/music.h>
#include <graphics/graphics.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/types.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <audiodemo/portable_graphics.h>

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
/* PRT(("Did %s\n", name)); */ \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		PrintfSysErr(Result); \
		goto cleanup; \
	}

#define VERSION "2.2"

#define DEFAULT_VELOCITY      (64)
#define MAX_VELOCITY          (127)
#define DELTA_VELOCITY      (32)

#define DRB_NUM_ROWS (8)
#define DRB_NUM_COLUMNS (16)
#define MAX_SCORE_VOICES  (16)
#define MIXER_AMPLITUDE (5.0/MAX_SCORE_VOICES)
#define PIXEL_SIZE  (1.0/320)

typedef struct DrumLine
{
	Item   drl_Instrument;
	int32  drl_Velocities[DRB_NUM_COLUMNS];
} DrumLine;


    /* forward references for ControlGrid */
struct ControlGrid;
typedef struct ControlGrid ControlGrid;
/* Structure used for generic grid of controls. */
typedef int32 (*ControlGridButtonFunctionP) ( ControlGrid *cgr, void *Data, uint32 Button );
typedef int32 (*ControlGridDrawFunctionP) ( ControlGrid *cgr, void *Data);

struct ControlGrid
{
	int32     cgr_NumRows;
	int32     cgr_NumColumns;
	int32     cgr_CurrentRow;
	int32     cgr_CurrentColumn;
	float32   cgr_TopY;        /* Position of grid. */
	float32   cgr_LeftX;
	float32   cgr_DeltaX;      /* Width and height of grid cells. */
	float32   cgr_DeltaY;
	ControlGridButtonFunctionP  cgr_ButtonFunction;
	ControlGridDrawFunctionP    cgr_DrawFunction;
};

typedef struct DrumBox
{
	uint32    drb_Flags;
	PortableGraphicsContext *drb_pgcon;
	int32     drb_ActiveGrid;
	ControlGrid drb_SoloGrid;
	ControlGrid drb_BeatGrid;
	struct DrumLine drb_Lines[DRB_NUM_ROWS];  /* One row/line of drum hits. */
	struct ScoreContext *drb_ScoreContext;
	Item      drb_Cue;         /* For use with timer. */
	int32     drb_IfOnPhase;      /* Are notes on or off? */
	int32     drb_CurrentIndex; /* Currently playing column. */
	uint32    drb_CurTime;   /* Internal time clock. */
	int32     drb_Duration;    /* Time between notes/2 */
} DrumBox;

Err drbDrawAboutScreen( PortableGraphicsContext *pgcon );
Err drbProcessUserInput ( PortableGraphicsContext *pgcon, DrumBox *drb, uint32 Joy );
Err drbInteractiveLoop( PortableGraphicsContext *pgcon, DrumBox *drb );
Err drbDrawNextScreen( PortableGraphicsContext *pgcon, DrumBox *drb );
Err drbDrawRect( PortableGraphicsContext *pgcon, int32 LeftX, int32 Topy, int32 RightX, int32 BottomY );
Err drbDrawGrid( PortableGraphicsContext *pgcon, DrumBox *drb );

static DrumBox gMyDrumBox;

/*****************************************************************
** Terminate sound system.
*****************************************************************/
Err drbTermSound ( DrumBox *drb )
{
	int32 Result = 0;
	if (drb->drb_Cue) DeleteItem( drb->drb_Cue );
	if (drb->drb_ScoreContext)
	{
		UnloadPIMap( drb->drb_ScoreContext );
		DeleteScoreMixer( drb->drb_ScoreContext );
		DeleteScoreContext( drb->drb_ScoreContext );
	}
	return Result;
}

/*****************************************************************
** Initialize sound system.
*****************************************************************/
int32 drbInitSound( DrumBox *drb, char *mapfile )
{
	int32 Result;

/* Create a context for interpreting a MIDI score and tracking notes. */
	drb->drb_ScoreContext = CreateScoreContext ( 128 );
	if( drb->drb_ScoreContext == NULL )
	{
		Result = AF_ERR_NOMEM;
		goto cleanup;
	}

/* Specify a mixer to use for the score voices. */
	Result = CreateScoreMixer( drb->drb_ScoreContext,
		MakeMixerSpec(MAX_SCORE_VOICES,2,AF_F_MIXER_WITH_LINE_OUT),
		MAX_SCORE_VOICES, MIXER_AMPLITUDE);
	CHECKRESULT(Result,"CreateScoreMixer");

/* Load Instrument Templates from disk and fill Program Instrument Map. */
	Result = LoadPIMap ( drb->drb_ScoreContext, mapfile );
	CHECKRESULT(Result, "LoadPIMap");

/* Create cue for use with timer. */
	drb->drb_Cue = CreateCue ( NULL );
	CHECKRESULT(drb->drb_Cue, "CreateCue");

	return Result;

cleanup:
	drbTermSound( drb );
	return Result;
}


/*****************************************************************
** Turn on all the selected notes in a column.
*****************************************************************/
Err drbColumnOn( DrumBox *drb, int32 ColumnIndex )
{
	int32 Vel, i;
	int32 Result;

	Result = 0;
	for( i=0; i<DRB_NUM_ROWS; i++)
	{
		Vel = drb->drb_Lines[i].drl_Velocities[ColumnIndex];
		if( Vel > 0)  /* Is it on? */
		{
			Result = StartScoreNote( drb->drb_ScoreContext, i, 60, Vel );
			CHECKRESULT(Result, "StartScoreNote");
		}
	}
cleanup:
	return Result;
}
/*****************************************************************
** Turn off all the notes in a column.
*****************************************************************/
Err drbColumnOff( DrumBox *drb, int32 ColumnIndex )
{
	int32 i;
	int32 Result = 0;

	TOUCH(ColumnIndex);

	for( i=0; i<DRB_NUM_ROWS; i++)
	{
/* Release all notes in case velocity was turned off since note turned on. */
		Result = ReleaseScoreNote( drb->drb_ScoreContext, i, 60, 0 );
		CHECKRESULT(Result, "ReleaseScoreNote");
	}
cleanup:
	return Result;
}

/*****************************************************************
** Draw an empty ControlGrid.
*****************************************************************/
Err cgrDrawEmptyGrid( PortableGraphicsContext *pgcon, ControlGrid *cgr )
{
	int32 Result;
	int32 ix, iy;

	Result = 0;

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

	return Result;
}

/*****************************************************************
** Fill in a single cell of a ControlGrid in the current color.
*****************************************************************/
void cgrDrawGridCell( PortableGraphicsContext *pgcon, ControlGrid *cgr, int32 Row, int32 Column, float32 Fraction )
{
	float32 X1,Y1, X2,Y2;
	float32 HalfWidth = Fraction/2.0;

	X1 = cgr->cgr_LeftX + (Column * cgr->cgr_DeltaX) + ((0.5 - HalfWidth) * cgr->cgr_DeltaX);
	Y1 = cgr->cgr_TopY + (Row * cgr->cgr_DeltaY) + ((0.5 - HalfWidth) * cgr->cgr_DeltaY);
	X2 = cgr->cgr_LeftX + (Column * cgr->cgr_DeltaX) + ((0.5 + HalfWidth) * cgr->cgr_DeltaX);
	Y2 = cgr->cgr_TopY + (Row * cgr->cgr_DeltaY) + ((0.5 + HalfWidth) * cgr->cgr_DeltaY);
	pgDrawRect( pgcon, X1, Y1, X2, Y2);
}

/*****************************************************************
** Draw a drum grid.
*****************************************************************/
Err drbDrawBeatGrid( DrumBox *drb )
{
	int32 Result;
	int32 ix, iy;
	float32 Red, Green, Blue;
	PortableGraphicsContext *pgcon;
	ControlGrid *cgr;
	int32 vel;

	Result = 0;

	pgcon = drb->drb_pgcon;

	cgr = &drb->drb_BeatGrid;
	cgrDrawEmptyGrid( pgcon, cgr );

/* Draw grid cells on/off */
	for( iy=0; iy<DRB_NUM_ROWS; iy++)
	{
		for( ix=0; ix<DRB_NUM_COLUMNS; ix++)
		{
			vel = drb->drb_Lines[iy].drl_Velocities[ix];
DBUG(("iy = %d, ix = %d, vel = %d\n", iy, ix, drb->drb_Lines[iy].drl_Velocities[ix] ));
			if(vel > 0)  /* Is it on? */
			{
	/* Draw a different color if on the beat. */
				Red = ( ix == drb->drb_CurrentIndex ) ? 0.7 : 0.0;
				Green = ((float32) vel) / MAX_VELOCITY;
				Blue = 0.5;
				pgSetColor( pgcon, Red, Green, Blue );

				cgrDrawGridCell( pgcon, &drb->drb_BeatGrid, iy, ix, 0.8 );
			}
		}
	}

/* Draw Flashing Input Cursor */
	if( drb->drb_CurrentIndex & 1 ) /* Change color for each beat. */
	{
		pgSetColor( pgcon, 1.0, 0, 0 );
	}
	else
	{
		pgSetColor( pgcon, 1.0, 0, 1.0 );
	}
	cgrDrawGridCell( pgcon, &drb->drb_BeatGrid,
		drb->drb_BeatGrid.cgr_CurrentRow, drb->drb_BeatGrid.cgr_CurrentColumn, 0.5 );

/* Draw beat indicator. */
	pgSetColor( pgcon, 0, 0.5, 1.0 );
	cgrDrawGridCell( pgcon, &drb->drb_BeatGrid,
		drb->drb_BeatGrid.cgr_NumRows+1, drb->drb_CurrentIndex, 0.8 );

	return Result;
}

/*****************************************************************
** Draw screen for this demo.
*****************************************************************/
Err drbDrawNextScreen( PortableGraphicsContext *pgcon, DrumBox *drb )
{
	int32 Result;

	pgMoveTo( pgcon, 0.2, 0.1 );
	pgDrawText( pgcon, "DrumBox - 3DO" );

	Result = drbDrawBeatGrid( drb );
	CHECKRESULT(Result,"drbDrawGrid");

cleanup:
	return Result;
}


/*****************************************************************
** Setup DrumBox data structure to defaults.
*****************************************************************/
Err drbSetupBeatGrid( ControlGrid *cgr )
{
	cgr->cgr_NumRows = DRB_NUM_ROWS;
	cgr->cgr_NumColumns = DRB_NUM_COLUMNS;

	cgr->cgr_CurrentRow = 0;
	cgr->cgr_CurrentColumn = 0;

	cgr->cgr_DeltaX = 1.0/24.0;
	cgr->cgr_DeltaY = 1.0/20.0;

	cgr->cgr_LeftX = 0.1;
	cgr->cgr_TopY = 0.2;

	return 0;
}

/*****************************************************************
** Setup DrumBox data structure to defaults.
*****************************************************************/
void drbSetupDrumBox( DrumBox *drb )
{
	drbSetupBeatGrid( &drb->drb_BeatGrid );
	drb->drb_Flags = 0;
	drb->drb_CurrentIndex = 0;
	drb->drb_Duration = 20;
}
/****************************************************************/
Err drbInit( DrumBox *drb )
{
	drbSetupDrumBox( drb );
	return 0;
}

/*****************************************************************
** Draw title screen and help.
*****************************************************************/
Err drbDrawAboutScreen( PortableGraphicsContext *pgcon )
{
	float32 y;

#define LINEHEIGHT (0.1)
#define LEFTMARGIN (0.1)

#define NEXTLINE(msg) \
	pgMoveTo ( pgcon, LEFTMARGIN,y ); \
	y += LINEHEIGHT; \
	pgDrawText( pgcon, msg );

DBUG(("drbDrawAboutScreen()\n"));
	pgSetColor( pgcon, 0, 1.0, 0.5 );
	y = 0.1;
	NEXTLINE("DrumBox");
	pgDrawText( pgcon, VERSION );
	NEXTLINE("(c) 3DO, August 1993");
	NEXTLINE("by Phil Burk");
	NEXTLINE("Hit any button to continue.");
	TOUCH(y);
	return 0;
}
/*****************************************************************
** Draw Wait message.
*****************************************************************/
Err drbDrawWaitScreen( PortableGraphicsContext *pgcon )
{
	pgSetColor( pgcon, 1.0, 1.0, 0.0 );
	pgMoveTo ( pgcon, LEFTMARGIN,0.3 );
	pgDrawText( pgcon, "Please wait for samples to load." );
	return 0;
}

/*****************************************************************
** Move around within a control grid.
*****************************************************************/
int32 cgrHandleArrowKeys( ControlGrid *cgr, uint32 Buttons )
{
	if(Buttons & ControlUp)
	{
		cgr->cgr_CurrentRow -= 1;
		if( cgr->cgr_CurrentRow < 0 ) cgr->cgr_CurrentRow = cgr->cgr_NumRows-1;
	}
	else if(Buttons & ControlDown)
	{
		cgr->cgr_CurrentRow += 1;
		if( cgr->cgr_CurrentRow >= cgr->cgr_NumRows ) cgr->cgr_CurrentRow = 0;
	}

	if(Buttons & ControlLeft)
	{
		cgr->cgr_CurrentColumn -= 1;
		if( cgr->cgr_CurrentColumn < 0 ) cgr->cgr_CurrentColumn = cgr->cgr_NumColumns-1;
	}
	else if(Buttons & ControlRight)
	{
		cgr->cgr_CurrentColumn += 1;
		if( cgr->cgr_CurrentColumn >= cgr->cgr_NumColumns ) cgr->cgr_CurrentColumn = 0;
	}
	return 0;
}
/*****************************************************************
** Respond appropriately to button presses.
*****************************************************************/
Err drbProcessUserInput (  PortableGraphicsContext *pgcon, DrumBox *drb, uint32 Buttons )
{
	int32 Result;
	int32 Row, Column;
	Result = 0;

	TOUCH(pgcon);

	cgrHandleArrowKeys( &drb->drb_BeatGrid, Buttons );

	Row = drb->drb_BeatGrid.cgr_CurrentRow;

	if( Buttons & ControlLeftShift )
	{
/* Timed play mode, sets or clears drums as the timer passes over them. */
		Column = drb->drb_CurrentIndex;
		if( Buttons & ControlA )
		{
			drb->drb_Lines[Row].drl_Velocities[Column] = MAX_VELOCITY / 2;
		}
		if( Buttons & ControlB )
		{
			drb->drb_Lines[Row].drl_Velocities[Column] = 0;
		}
	}
	else
	{
/* Add or subtract velocity under cursor. */
		Column = drb->drb_BeatGrid.cgr_CurrentColumn;
		if( Buttons & ControlA )
		{
			drb->drb_Lines[Row].drl_Velocities[Column] += DELTA_VELOCITY;
			if( drb->drb_Lines[Row].drl_Velocities[Column] > MAX_VELOCITY )
			{
				drb->drb_Lines[Row].drl_Velocities[Column] = MAX_VELOCITY;
			}
		}

		if( Buttons & ControlB )
		{
			drb->drb_Lines[Row].drl_Velocities[Column] -= DELTA_VELOCITY;
			if( drb->drb_Lines[Row].drl_Velocities[Column] < 0 )
			{
				drb->drb_Lines[Row].drl_Velocities[Column] = 0;
			}
		}
	}

/* C button clears Row */
	if( Buttons & ControlC )
	{
		for( Column=0; Column<DRB_NUM_COLUMNS; Column++)
		{
			drb->drb_Lines[Row].drl_Velocities[Column] = 0;
		}
	}

	return Result;
}

static uint32 OldButtons;

/*********************************************************************/
Err drbInteractiveLoop( PortableGraphicsContext *pgcon, DrumBox *drb )
{
	int32 Result;
	uint32 Buttons;
	ControlPadEventData cped;

	drb->drb_CurTime = GetAudioTime();

/* Loop until the user presses ControlX */
	while(1)
	{
		SleepUntilTime( drb->drb_Cue, drb->drb_CurTime + drb->drb_Duration );
		drb->drb_CurTime += drb->drb_Duration;

/* Get User input. */
		Result = GetControlPad (1, FALSE, &cped);
		if (Result < 0) {
			ERR(("Error in GetControlPad\n"));
			PrintfSysErr(Result);
		}
		Buttons = cped.cped_ButtonBits;
		if(Buttons & ControlX) break;

/* Did the control pad state change? */
		if( Buttons != OldButtons )
		{
			Result = drbProcessUserInput( pgcon, drb, Buttons );
			if (Result < 0)
			{
				ERR(("main: error in drbProcessUserInput\n"));
				PrintfSysErr(Result);
				goto cleanup;
			}
			OldButtons = Buttons;
		}

/* Toggle notes on and off. */
		if( drb->drb_IfOnPhase != 0 )
		{
			Result = drbColumnOn( drb, drb->drb_CurrentIndex );
		}
		else
		{
			Result = drbColumnOff( drb, drb->drb_CurrentIndex );
			drb->drb_CurrentIndex++;
			if( drb->drb_CurrentIndex >= DRB_NUM_COLUMNS )
			{
				drb->drb_CurrentIndex = 0;
			}
		}
		if (Result < 0) {
			ERR(("Error in drbColumnOn/Off\n"));
			PrintfSysErr(Result);
		}

/* Generate the next video image. */
		pgClearDisplay( pgcon, 0.0, 0.0, 0.0 );
		Result = drbDrawNextScreen( pgcon, drb );
		if (Result < 0)
		{
			ERR(("drbInteractiveLoop: error in drbDrawNextScreen\n"));
			PrintfSysErr(Result);
			goto cleanup;
		}

/* Switch double buffered screens. */
		Result = pgSwitchScreens( pgcon );
		if (Result < 0)
		{
			PrintfSysErr(Result);
			return Result;
		}
		drb->drb_IfOnPhase = !drb->drb_IfOnPhase;
	}
cleanup:
	return Result;
}

/************************************************************************/

int main( int argc, char *argv[] )
{
	char *progname;
	char *mapfile;
	int32 Result;
	PortableGraphicsContext *pgcon;
	ControlPadEventData cped;

	memset (&gMyDrumBox, 0, sizeof gMyDrumBox);

/* Get arguments from command line. */
	progname = argv[0];
	mapfile = (argc > 1) ? argv[1] : "drumbox.pimap";

	printf( "\n%s %s by Phil Burk\n", progname, VERSION );
	printf( "  PIMAP file is %s\n", mapfile );
	printf( "Copyright 1993 3DO\n" );

/* Initialize audio, return if error. */
	if (OpenAudioFolio() < 0)
	{
		PRT(("Audio Folio could not be opened!\n"));
		return(-1);
	}

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, TRUE);
	if (Result < 0)
	{
		ERR(("main: error in InitEventUtility\n"));
		PrintfSysErr(Result);
		goto cleanup;
	}

/* Set up double buffered display. */
	Result = pgCreateDisplay( &pgcon, 2 );
	CHECKRESULT(Result,"pgCreateDisplay");

/* Initialize DrumBox structure. */
	if ( (Result = drbInit(  &gMyDrumBox )) != 0 ) goto cleanup;
	gMyDrumBox.drb_pgcon = pgcon;

/* Draw hello. */
	pgClearDisplay( pgcon, 0.0, 0.0, 0.0 );
	drbDrawAboutScreen( pgcon );
	pgSwitchScreens( pgcon );
	GetControlPad (1, TRUE, &cped);   /* Wait for button press. */

/* Load samples. */
	pgClearDisplay( pgcon, 0.0, 0.0, 0.0 );
	drbDrawWaitScreen( pgcon );
	pgSwitchScreens( pgcon );
	Result = drbInitSound( &gMyDrumBox, mapfile );
	if(Result < 0)
	{
		ERR(("drbInitSound failed 0x%x\n"));
		return (int) Result;
	}

/* Play drum box. */
	drbInteractiveLoop( pgcon, &gMyDrumBox );

/* Cleanup the EventBroker. */
	Result = KillEventUtility();
	if (Result < 0)
	{
		ERR(("main: error in KillEventUtility\n"));
		PrintfSysErr(Result);
	}

cleanup:
	pgDeleteDisplay( &pgcon );
	drbTermSound( &gMyDrumBox );
	printf( "\n%s finished!\n", progname );
	return( (int) Result );
}

