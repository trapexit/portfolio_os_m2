/* @(#) audio_diagnostic.c 96/08/27 1.6 */
/******************************************************************************
**
**  $Id: audio_diagnostic.c,v 1.1 1995/02/16 03:21:17 phil Exp $
**
******************************************************************************/

/*
** Audio Diagnostics
** Author: Phil Burk
** Copyright 3DO 1995
**
******************************************************************
** 950427 PLB Added left/right test.
******************************************************************
*/

/* !!! autodoc commented out */
/**
|||	NOAUTODOC -private -class ??? -name audio_diagnostic
|||	Diagnostic CD for audio
|||
|||	  Format
|||
|||	    audio_diagnostic
|||
|||	  Description
|||
|||	    Miscellaneous diagnostics for the audio hardware.
|||	    Uses graphics display instead of terminal I/O.
|||
|||	    A) Read heads of FIFOs
|||
|||	    B) Play each FIFO
|||
|||	    C) Left/Right Impulse wave
**/

#include "types.h"
#include "kernel.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "mem.h"
#include "semaphore.h"
#include "io.h"
#include "strings.h"
#include "stdlib.h"
#include "debug.h"
#include "stdio.h"
#include "filefunctions.h"
#include "graphics.h"
#include "event.h"
#include "audio.h"


#define VERSION "0.2"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */


/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	DBUG(("Did %s\n", name)); \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\Failure in",name,val); \
		goto cleanup; \
	}

/* Graphic constants */
#define DISPLAY_WIDTH	(320)
#define DISPLAY_HEIGHT	(240)
#define LEFT_MARGIN      (40)
#define TOP_MARGIN       (10)
#define LINE_HEIGHT      (14)

#define RED_MASK      0x7C00
#define GREEN_MASK    0x03E0
#define BLUE_MASK     0x001F
#define RED_SHIFT     10
#define GREEN_SHIFT   5
#define BLUE_SHIFT    0
#define ONE_RED       (1<<REDSHIFT)
#define ONE_GREEN     (1<<GREENSHIFT)
#define ONE_BLUE      (1<<BLUESHIFT)
#define MAX_RED       (RED_MASK>>RED_SHIFT)
#define MAX_GREEN     (GREEN_MASK>>GREEN_SHIFT)
#define MAX_BLUE      (BLUE_MASK>>BLUE_SHIFT)


/* *************************************************************************
 * ***                       ***********************************************
 * ***  Function Prototypes  ***********************************************
 * ***                       ***********************************************
 * *************************************************************************
 */

int32 InitDemo( void );
int32 AudioMonitor( void );
int32 ClearScreen( void );
uint32 GetButtons( void );

/* *************************************************************************
 * ***                     *************************************************
 * ***  Data Declarations  *************************************************
 * ***                     *************************************************
 * *************************************************************************
 */


/* Graphics Context contains drawing information */
GrafCon GCon[2];

Item ScreenItems[2];
Item ScreenGroupItem = 0;
Item BitmapItems[2];
Bitmap *Bitmaps[2];

#define NUM_SCREENS 1

/********************************************************************/

char *myitoa(int32 num, char *s, int32 radix)
{
#define MAXITOA 64
	char Pad[MAXITOA];
	int32 nd = 0, a, rem, div, i;
	int32 ifneg = FALSE;
	char *p, c;

	if (num < 0)
	{
		ifneg = TRUE;
		num = -num;
	}

#define HOLD(ch) { Pad[nd++] = (ch); }
	a = num;
	do
	{
		div = a/radix;
		rem = a - (div*radix);
		if( rem < 10 )
		{
			c = (char) rem + '0';
		}
		else
		{
			c = (char) rem + 'A';
		}
PRT(("rem = 0x%x, c = %c\n", rem, c ));
		HOLD(c);
		a = div;
	} while(a > 0);

	if (ifneg) HOLD('-');

/* Copy string to s. */
	p = s;
	for (i=0; i<nd; i++)
	{
		*p++ = Pad[nd-i-1];
	}
	*p = '\0';
	return s;
}
char *itoar(int32 num, char *s, int32 radix, int32 width)
{
#define MAXITOA 64
	char Pad[MAXITOA];
	int32 nd = 0, a, rem, div, i;
	int32 nc;

	int32 ifneg = FALSE;
	char *p, c;

	if (num < 0)
	{
		ifneg = TRUE;
		num = -num;
	}

	nc = 0;

#define HOLD(ch) { Pad[nd++] = (ch); }
	a = num;
	do
	{
		div = a/radix;
		rem = a - (div*radix);
		if( rem < 10 )
		{
			c = (char) rem + '0';
		}
		else
		{
			c = (char) rem - 10 + 'A';
		}
DBUG(("rem = 0x%x, c = %c = 0x%x\n", rem, c, c ));
		HOLD(c);
		a = div;
		nc++;
	} while((a > 0) || (nc < width)) ;

	if (ifneg) HOLD('-');

/* Copy string to s. */
	p = s;
	for (i=0; i<nd; i++)
	{
		*p++ = Pad[nd-i-1];
	}
	*p = '\0';
	return s;
}

/********************************************************************/
int32 DrawNumber ( int32 num, int32 width)
{
	char Pad[100];
	itoar(num, Pad, 16, width);
	DrawText8( &GCon[0], BitmapItems[0], Pad );
	return 0;
}

/********************************************************************/
void DrawValue( int32 x, int32 y , char *msg, int32 num)
{
	MoveTo( &GCon[0], x, y);
	DrawText8( &GCon[0], BitmapItems[0], msg );
	DrawNumber(num, 3);
}

/********************************************************************/

void DrawRect( int32 XLeft, int32 YTop, int32 XRight, int32 YBottom )
{
	Rect WorkRect;
	WorkRect.rect_XLeft = XLeft;
	WorkRect.rect_XRight = XRight;
	WorkRect.rect_YTop = YTop;
	WorkRect.rect_YBottom = YBottom;
	FillRect( BitmapItems[0], &GCon[0], &WorkRect );
}

/********************************************************************/
void DrawTextLine( char *Msg, int32 LineIndex )
{
	MoveTo( &GCon[0], LEFT_MARGIN, TOP_MARGIN + (LINE_HEIGHT*(LineIndex+1)));
	DrawText8( &GCon[0], BitmapItems[0], Msg );
}

/********************************************************************/
int32 ClearScreen()
{
DBUG(("Clear Screen\n"));
	SetFGPen( &GCon[0], MakeRGB15(6,6,6) );
	DrawRect (0, 0, DISPLAY_WIDTH-1, DISPLAY_HEIGHT-1);

	return 0;
}

/**********************************************************************/
void DisplayBackdrop( void )
{

	MoveTo( &GCon[0], 80, 30);
	DrawText8( &GCon[0], BitmapItems[0], "Audio Monitor " );
	DrawText8( &GCon[0], BitmapItems[0], VERSION );
}

TagArg ScreenTags[] =
{
	CSG_TAG_SCREENCOUNT,	(void *)NUM_SCREENS,
	CSG_TAG_DONE,			0
};

/**********************************************************************/
int32 InitDemo( void )
/* This routine does all the main initializations.  It should be
 * called once, before the program does much of anything.
 * Returns non-FALSE if all is well, FALSE if error
 */
{
	int32 Result;
	Screen *screen;
	int32 i;

	Result = OpenAudioFolio();
	if (Result < 0)
	{
		ERR(("Could not open audio folio!, Error = 0x%x\n", Result));
		goto DONE;
	}
	Result = OpenGraphicsFolio();
	if (Result < 0)
	{
		ERR(("Could not open graphics folio!, Error = 0x%x\n", Result));
		goto DONE;
	}

/*	DumpMemory(ScreenTags,sizeof(TagArg)*2); */

	ScreenGroupItem = CreateScreenGroup( ScreenItems, ScreenTags );
/*	DumpMemory(ScreenTags,sizeof(TagArg)*2); */
	if ( ScreenGroupItem < 0 )
	{
		printf( "Error:  CreateScreenGroup() returned %ld\n", ScreenGroupItem );
		goto DONE;
	}
	AddScreenGroup( ScreenGroupItem, NULL );

	for ( i = 0; i < NUM_SCREENS; i++ )
	{
		screen = (Screen *)LookupItem( ScreenItems[i] );
		if ( screen == 0 )
		{
			ERR(( "Huh?  Couldn't locate screen?" ));
			goto DONE;
		}
		BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
		Bitmaps[i] = screen->scr_TempBitmap;
	}

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, TRUE);
	if (Result < 0)
	{
		PrintError(0,"InitEventUtility",0,Result);
		goto cleanup;
	}
cleanup:
DONE:
	return( Result );
}

Err QueryFIFO_Heads( void )
{
#define DSPI_NUM_CHANNELS   (13)
	int32 i;
	int32 Result;
	Item   Instruments[DSPI_NUM_CHANNELS];
	Item   Probes[DSPI_NUM_CHANNELS];
	int32  Val;

	for( i=0; i<DSPI_NUM_CHANNELS; i++ )
	{
		Instruments[i] = LoadInstrument("fixedmonosample.dsp", 0, 100);
		CHECKRESULT( Instruments[i], "LoadInstrument" );
		StartInstrument( Instruments[i], NULL );
		Probes[i] = CreateProbe( Instruments[i], "Output", NULL );
		CHECKRESULT(Probes[i],"CreateProbe");
	}

	ClearScreen();
	MoveTo( &GCon[0], LEFT_MARGIN, TOP_MARGIN + LINE_HEIGHT);
	DrawText8( &GCon[0], BitmapItems[0], "FIFO Head Current Values." );

/* Read Probes to see what value is passed from uninitialized FIFO head.
** Display result on screen.
*/
	for( i=0; i<DSPI_NUM_CHANNELS; i++ )
	{
		int32 x,y;
		Result = ReadProbe( Probes[i], &Val );
		CHECKRESULT( Result, "ReadProbe" );

		if( Val > 0 ) Val += 1; /* Correct for Amplitude multiply. */
		Val &= 0xFFFF;

		x = LEFT_MARGIN + 50;
		y = TOP_MARGIN + (i+2)*LINE_HEIGHT;
		MoveTo( &GCon[0], x, y);
		DrawNumber(i, 1);
		MoveTo( &GCon[0], x + 50, y);
		DrawNumber(Val, 4);
	}

	GetButtons();

cleanup:

	for( i=0; i<DSPI_NUM_CHANNELS; i++ )
	{
		UnloadInstrument( Instruments[i] );
		DeleteProbe( Probes[i] );
	}
	return Result;
}


/********************************************************************/
Err PlayEachFIFO( void )
{
#define DSPI_NUM_CHANNELS   (13)
	int32  i;
	int32  Result;
	Item   OutputIns = 0;
	Item   SampleItem = 0;
	Item   Instruments[DSPI_NUM_CHANNELS];
	Item   Attachments[DSPI_NUM_CHANNELS];
	int32  Indx = 0;
	int32  DoIt = TRUE;
	int32  DoneCue;

	DoneCue = CreateCue( NULL );
	CHECKRESULT(DoneCue,"CreateCue");

/* Load a directout instrument to send the sound to the DAC. */
	OutputIns = LoadInstrument("directout.dsp",  0, 0);
	CHECKRESULT(OutputIns,"LoadInstrument");
	StartInstrument( OutputIns, NULL );

/* Load digital audio AIFF Sample file from disk. */
	SampleItem = LoadSample("sinewave.aiff");
	CHECKRESULT(SampleItem,"LoadSample");

	for( i=0; i<DSPI_NUM_CHANNELS; i++ )
	{
		Instruments[i] = LoadInstrument("fixedmonosample.dsp", 0, 100);
		CHECKRESULT( Instruments[i], "LoadInstrument" );
/* Attach the sample to the instrument for playback. */
		Attachments[i] = AttachSample(Instruments[i], SampleItem, 0);
		CHECKRESULT(Attachments[i],"AttachSample");

		Result = MonitorAttachment( Attachments[i], DoneCue, CUE_AT_END );
		CHECKRESULT(Result,"MonitorAttachment");
	}


	ClearScreen();
	MoveTo( &GCon[0], LEFT_MARGIN, TOP_MARGIN + LINE_HEIGHT);
	DrawText8( &GCon[0], BitmapItems[0], "Play each FIFO, use Up/Down" );

	while (DoIt)
	{

		uint32 Buttons;

		Result = ConnectInstruments (Instruments[Indx], "Output", OutputIns, "InputLeft");
		CHECKRESULT(Result,"ConnectInstruments");
		Result = ConnectInstruments (Instruments[Indx], "Output", OutputIns, "InputRight");
		CHECKRESULT(Result,"ConnectInstruments");

		Result = StartInstrument( Instruments[Indx], NULL );
		CHECKRESULT(Result,"StartInstrument");

		MoveTo( &GCon[0], 100, 100);
		DrawText8( &GCon[0], BitmapItems[0], "FIFO NUM =  " );
		DrawNumber(Indx, 1);

		Buttons = GetButtons();

		Result = ReleaseInstrument( Instruments[Indx], NULL );
		CHECKRESULT(Result,"StopInstrument");

		WaitSignal( GetCueSignal( DoneCue ) );

		switch( Buttons )
		{
		case ControlUp:
			Indx = (Indx < (DSPI_NUM_CHANNELS-1)) ? Indx+1 : 0;
			break;

		case ControlDown:
			Indx = (Indx > 0) ? Indx-1 : (DSPI_NUM_CHANNELS-1);
			break;

		case ControlX:
			DoIt = FALSE;
			break;
		}
	}


cleanup:

	for( i=0; i<DSPI_NUM_CHANNELS; i++ )
	{
		UnloadInstrument( Instruments[i] );
	}
	UnloadSample( SampleItem );
	UnloadInstrument( OutputIns );
	DeleteCue( DoneCue );

	return Result;
}

/***********************************************************************/

int32 PlayLeftRight( void )
{
	Item PulseTmp = 0;
	Item PulseIns = 0;
	Item FreqKnob = 0;
	int32 Result;
	Item OutputIns;
	int32 IfLeftOn = 0;
	int32 IfRightOn = 0;
	int32 DoIt = TRUE;
	int32 ln = 0;

	ClearScreen();

	DrawTextLine( "Left/Right Test", ln++ );
	DrawTextLine( "    A = Toggle Left Channel", ln++ );
	DrawTextLine( "    B = Toggle Right Channel", ln++ );
	DrawTextLine( "    X = eXit", ln++ );
	ln++;
	DrawTextLine( "Play both channels and use scope", ln++ );
	DrawTextLine( " to test phase coherence.", ln++ );

/* Load "directout" for connecting to DAC. */
	OutputIns = LoadInstrument("directout.dsp",  0,  100);
	CHECKRESULT(OutputIns,"LoadInstrument");

/* Load description of instrument */
	PulseTmp = LoadInsTemplate( "impulse.dsp", 0);
	CHECKRESULT(PulseTmp,"LoadInsTemplate");

/* Make an instrument based on template. */
	PulseIns = AllocInstrument(PulseTmp, 0);
	CHECKRESULT(PulseIns,"AllocInstrument");

/* Attach the Frequency knob. */
	FreqKnob = GrabKnob( PulseIns, "Frequency" );
	CHECKRESULT(FreqKnob,"GrabKnob");

	Result = StartInstrument( PulseIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( OutputIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

	while( DoIt)
	{
		uint32 Buttons;

		Buttons = GetButtons();

/* Process buttons pressed. */
		switch(Buttons)
		{
		case ControlX:
			DoIt = FALSE;
			break;

#define ONSTRING "ON-ON-ON-ON-ON"
#define OFFSTRING "OFF-----------"

		case ControlA:
			if( IfLeftOn )
			{
				Result = DisconnectInstruments (PulseIns, "Output", OutputIns, "InputLeft");
				CHECKRESULT(Result,"ConnectInstruments");
				DrawTextLine( "   Left  is "OFFSTRING, ln+1);
			}
			else
			{
				Result = ConnectInstruments (PulseIns, "Output", OutputIns, "InputLeft");
				CHECKRESULT(Result,"ConnectInstruments");
				DrawTextLine( "   Left  is "ONSTRING, ln+1);
			}
			IfLeftOn = !IfLeftOn;
			break;

		case ControlB:
			if( IfRightOn )
			{
				Result = DisconnectInstruments (PulseIns, "Output", OutputIns, "InputRight");
				CHECKRESULT(Result,"ConnectInstruments");
				DrawTextLine( "   Right is "OFFSTRING, ln+2);
			}
			else
			{
				Result = ConnectInstruments (PulseIns, "Output", OutputIns, "InputRight");
				CHECKRESULT(Result,"ConnectInstruments");
				DrawTextLine( "   Right is "ONSTRING, ln+2);
			}
			IfRightOn = !IfRightOn;
			break;

		default:
			break;
		}
	}


	StopInstrument(PulseIns, NULL);
	StopInstrument(OutputIns, NULL);

cleanup:
/* The Audio Folio is immune to passing NULL values as Items. */
	ReleaseKnob( FreqKnob);
	FreeInstrument( PulseIns );
	UnloadInsTemplate( PulseTmp );
	UnloadInstrument( OutputIns );
	CloseAudioFolio();
	return((int32) Result);
}

/********************************************************************/
/* Get complete button press, flushes button ups. */
uint32 GetButtons( void )
{
	uint32 Buttons = 0;
	ControlPadEventData cped;
	int32 Result;

	while( Buttons == 0 )
	{
		Result = GetControlPad (1, TRUE, &cped);
		if (Result < 0) {
			PrintError(0,"read control pad in","diagnostic",Result);
		}
		Buttons = cped.cped_ButtonBits;
	}

	return Buttons;
}


/********************************************************************/
int main( int argc, char *argv[] )
{
	char *progname;
	int32 Result;
	int32 DoIt = TRUE;
	int32 Buttons;

	progname = argv[0];
	printf( "%s %s\n", progname, VERSION );

	if ( (Result = InitDemo()) != 0 ) goto DONE;

	Result = DisplayScreen( ScreenItems[0], 0 );
	if ( Result < 0 )
	{
		printf( "DisplayScreen() failed, error=%d\n", Result );
		goto DONE;
	}

	ClearScreen();

/* Interactive event loop. */
	while(DoIt)
	{


		ClearScreen();

		{
			int32 ln = 0;
			DrawTextLine( "Audio Diagnostic", ln++ );
			DrawTextLine( "    A = Read FIFO heads", ln++ );
			DrawTextLine( "    B = Play each FIFO", ln++ );
			DrawTextLine( "    C = Left/Right Impulse", ln++ );
			DrawTextLine( "    X = eXit", ln );
		}

/* Get User input. */
		Buttons = GetButtons();

/* Process buttons pressed. */
		if(Buttons & ControlX) /* EXIT */
		{
			DoIt = FALSE;
		}

		if(Buttons & ControlA)
		{
			Result = QueryFIFO_Heads();
			CHECKRESULT( Result, "QueryFIFO_Heads" );
		}

		if(Buttons & ControlB)
		{
			Result = PlayEachFIFO();
			CHECKRESULT( Result, "PlayEachFIFO" );
		}

		if(Buttons & ControlC)
		{
			Result = PlayLeftRight();
			CHECKRESULT( Result, "PlayLeftRight" );
		}
	}

cleanup:
/* Cleanup the EventBroker. */
	Result = KillEventUtility();
	if (Result < 0)
	{
		PrintError(0,"KillEventUtility",0,Result);
	}
	CloseAudioFolio();

DONE:
	printf( "\n%s sez:  bye!\n", progname );
	return( (int)Result );
}
