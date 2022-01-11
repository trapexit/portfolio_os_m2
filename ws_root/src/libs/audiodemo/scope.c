
/******************************************************************************
**
**  @(#) scope.c 96/07/29 1.3
**
**  dsp oscilloscope library.
**
******************************************************************************/

/**
|||	AUTODOC -private -class libaudiodemo -group Scope -name scope
|||	A graphic display of a DSP instrument's output.
|||
|||	  Description
|||
|||	    This library defines a set of functions that can be called from a user program
|||	    to capture and display the output of a DSP instrument on the 3DO screen on a
|||	    Cartesian plane, with the horizontal representing time and the vertical
|||	    representing amplitude.  The display can be scaled in both dimensions using
|||	    the 3DO control pad.
|||
|||	    The number of samples to capture is specified by the user program.  The
|||	    display may be scrolled horizontally to display the full width of the captured
|||	    output, again using the control pad.
|||
|||	  Controls
|||
|||	    A
|||	        Capture a buffer's worth of samples.
|||
|||	    C, X
|||	        Quit.
|||
|||	    Up, Down
|||	        Scale amplitude values up or down by a factor of two.
|||
|||	    Left, Right
|||	        Scale time values up or down by a factor of two.
|||
|||	    LShift, RShift
|||	        Pan left or right through the captured buffer.
|||
|||	  Associated Files
|||
|||	    <audiodemo/scope.h>
**/

#ifndef PATCHDEMO_NO_GUI    /* { */

#include <audiodemo/scope.h>

	/* portfolio */
#include <audio/audio.h>
#include <kernel/cache.h>
#include <kernel/mem.h>
#include <kernel/operror.h> /* PrintError() */
#include <misc/event.h>     /* Init/KillEventUtility(), GetControlPadEvent() */
#include <stdio.h>          /* printf() */


/* -------------------- Debugging */

#define DEBUG_ScopeProbe    0
#define DEBUG_Connect       0


/* -------------------- Macros */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,val); \
		goto cleanup; \
	}

#define	PRT(x)	{ printf x; }

#define MakeStdUErr(svr,err) MakeErr(ER_USER,0,svr,ER_E_USER,ER_C_STND,err)

/* Macro to calculate y pos 0.0 .. 1.0 given sample point */
#define SAMPLE_TO_X(s,m) ((float32) s * m/256.0 * SCOPE_WIDTH) + SCOPE_LEFT_EDGE
#define SAMPLE_TO_Y(v,m) SCOPE_Y_AXIS - (v / 32768.0 * m);
	/* convert -32768..32767 to -1.0..0.999... */

/* -------------------- Code */

/***************************************************************
** support for capture of digital signals from DSPP
** Collect signals in a delay line for analysis or graphics display.
**************************************************************/
/**************************************************************/
Err DeleteScopeProbe( ScopeProbe *scpr )
{
	if( scpr == NULL ) return -1;

	DeleteCue (scpr->scpr_Cue);
	DeleteAttachment( scpr->scpr_Attachment );
	DeleteDelayLine( scpr->scpr_DelayLine );
	UnloadInstrument( scpr->scpr_Probe );
	FreeMem( scpr, sizeof(ScopeProbe) );
	return 0;
}

/**************************************************************/
Err CreateScopeProbe( ScopeProbe **resultScopeProbe, int32 NumBytes, PortableGraphicsContext *scopecon )
{
	ScopeProbe *scpr;
	int32 Result;
	/* rnm never used int32 Time0, Time1; */

/* init result */
	*resultScopeProbe = NULL;

/* allocate structure */
	if ((scpr = AllocMem( sizeof(ScopeProbe), MEMTYPE_FILL )) == NULL) {
	    Result = MakeStdUErr (ER_SEVERE, ER_NoMem);
	    goto cleanup;
	}

/* copy the graphics context */
	scpr->scpr_gc = scopecon;

/* Create Probe Instrument */
/* Use low priority to get final output. */
	scpr->scpr_Probe = LoadInstrument( "delay_f1.dsp", 0, 100 );
	CHECKRESULT(scpr->scpr_Probe,"LoadInstrument delaymono");

/* Create DelayLine */
#if DEBUG_ScopeProbe
	printf("Create probe %d bytes long\n", NumBytes );
#endif
	scpr->scpr_DelayLine = CreateDelayLine( NumBytes, 1, FALSE );
	CHECKRESULT(scpr->scpr_DelayLine,"CreateDelayLine");
	scpr->scpr_Size = NumBytes;

/* Get address of delay line */
	{
		TagArg tags[2] = {
			{ AF_TAG_ADDRESS, NULL },
			TAG_END
		};

		Result = GetAudioItemInfo(scpr->scpr_DelayLine, tags);
		CHECKRESULT(Result,"GetAudioItemInfo");
		scpr->scpr_Data = tags[0].ta_Arg;
	}

/* Attach delay line. */
	scpr->scpr_Attachment = CreateAttachmentVA (scpr->scpr_Probe, scpr->scpr_DelayLine,
		AF_TAG_SET_FLAGS, AF_ATTF_NOAUTOSTART,
		TAG_END);
	CHECKRESULT(scpr->scpr_Attachment,"CreateAttachment");
#if DEBUG_ScopeProbe
	printf("scpr->scpr_Attachment = 0x%x\n",
		scpr->scpr_Attachment );
#endif

/* Make CUE for monitoring delay line. */
	scpr->scpr_Cue = CreateCue( NULL );
	CHECKRESULT(scpr->scpr_Cue,"CreateCue");
	scpr->scpr_Signal = GetCueSignal( scpr->scpr_Cue );

	Result = MonitorAttachment( scpr->scpr_Attachment, scpr->scpr_Cue, CUE_AT_END );
	CHECKRESULT(Result,"MonitorAttachment");

	Result = StartInstrument( scpr->scpr_Probe, NULL );
	CHECKRESULT(Result,"StartInstrument");

/* Capture a buffer-full of audio */
/* !!! I'm not sure why this is here since the scope is recaptured when first displayed. */
	Result = CaptureScopeBuffer (scpr);
	CHECKRESULT(Result,"CaptureScopeBuffer");

/* return scpr pointer */
	*resultScopeProbe = scpr;
	return 0;

cleanup:
	DeleteScopeProbe( scpr );
	return Result;
}


/**************************************************************/
/*
    Capture a buffer-full of audio data, then flush data cache
    so that the next reads from scpr_Data will come from memory.
*/
Err CaptureScopeBuffer( ScopeProbe *scpr)
{
	int32 Result;

/* Start Instrument now. */
	Result = StartAttachment( scpr->scpr_Attachment, NULL );
	CHECKRESULT(Result,"StartInstrument");

/* Wait for signal to be returned. */
	WaitSignal( scpr->scpr_Signal );

/* Flush data cache so that any previous delay line contents is purged from the cache. */
	FlushDCache (0, scpr->scpr_Data, scpr->scpr_Size);

cleanup:
	return Result;
}


/**************************************************************/
#define SCOPE_LEFT_EDGE  (0.1)
#define SCOPE_Y_AXIS  (0.5)
#define SCOPE_WIDTH (0.8)
Err DisplayScopeBuffer( ScopeProbe *scpr, int32 XOffset, float32 XScalar, float32 YScalar )
{
	int32 i, NumPnts;
	float32 Data32;
	float32 x,y;
	int32 sampleLength;
	int32 Result;

#if SCOPE_DRAW_OPTIMIZE
	float32 minVal, maxVal;
	int32 j;
#endif

	sampleLength = scpr->scpr_Size / sizeof(int16);

	NumPnts =  256.0 / XScalar;
	if (NumPnts > sampleLength - XOffset) NumPnts = sampleLength - XOffset;

	Result = pgClearDisplay( scpr->scpr_gc, 0.0, 0.0, 0.0 );
	CHECKRESULT(Result, "pgClearDisplay");

/* Draw horizontal axis in a tasteful grey */
	Result = pgSetColor( scpr->scpr_gc, 0.25, 0.25, 0.25 );
	CHECKRESULT(Result, "pgSetColor");

	Result = pgMoveTo( scpr->scpr_gc, SCOPE_LEFT_EDGE, SCOPE_Y_AXIS);
	CHECKRESULT(Result, "pgMoveTo");

	Result = pgDrawTo( scpr->scpr_gc, SAMPLE_TO_X( NumPnts-1, XScalar ), SCOPE_Y_AXIS);
	CHECKRESULT(Result, "pgDrawTo");

/* Draw vertical axis(es) if needed */
	if (XOffset == 0)
	{
		Result = pgMoveTo( scpr->scpr_gc,  SCOPE_LEFT_EDGE, SCOPE_Y_AXIS - 0.1 );
		CHECKRESULT(Result, "pgMoveTo");

		Result = pgDrawTo( scpr->scpr_gc,  SCOPE_LEFT_EDGE, SCOPE_Y_AXIS + 0.1 );
		CHECKRESULT(Result, "pgDrawTo");
	}

	if (XOffset + NumPnts >= sampleLength)
	{
		Result = pgMoveTo( scpr->scpr_gc,  SAMPLE_TO_X( NumPnts-1, XScalar ), SCOPE_Y_AXIS - 0.1 );
		CHECKRESULT(Result, "pgMoveTo");

		Result = pgDrawTo( scpr->scpr_gc,  SAMPLE_TO_X( NumPnts-1, XScalar ), SCOPE_Y_AXIS + 0.1 );
		CHECKRESULT(Result, "pgDrawTo");
	}

/* Reset foreground color to white */
	Result = pgSetColor( scpr->scpr_gc,  1.0, 1.0, 1.0);
	CHECKRESULT(Result, "pgSetColor");

/* Display samples on screen. */
	for( i=0; i<NumPnts; i++ )
	{
		x = SAMPLE_TO_X( i, XScalar );

#if SCOPE_DRAW_OPTIMIZE
		if (XScalar < 1.0)
		{
		/* Draw line from min to max for each frame. */
			minVal = (float32) scpr->scpr_Data[XOffset + i];
			maxVal = minVal;
			for (j = 0; j < (int)1/XScalar ; j++)
			{
				Data32 = (float32) scpr->scpr_Data[XOffset + i + j];
				if (Data32 < minVal) minVal = Data32;
				if (Data32 > maxVal) maxVal = Data32;
			}
			y = SAMPLE_TO_Y( minVal, YScalar);
			Result = pgMoveTo( scpr->scpr_gc, x, y);
			CHECKRESULT(Result, "pgMoveTo");
			y = SAMPLE_TO_Y( maxVal, YScalar);
			Result = pgDrawTo( scpr->scpr_gc, x, y);
			CHECKRESULT(Result, "pgDrawTo");
		}
		else
#endif /* SCOPE_DRAW_OPTIMIZE */
		{
		/* Draw line between each sample. */
			Data32 = (float32) scpr->scpr_Data[XOffset+ i];
			y = SAMPLE_TO_Y( Data32, YScalar);
			if(i == 0)
			{
				Result = pgMoveTo( scpr->scpr_gc, x, y );
				CHECKRESULT(Result, "pgMoveTo");
			}
			else
			{
				Result = pgDrawTo( scpr->scpr_gc, x, y );
				CHECKRESULT(Result, "pgDrawTo");
			}
		}
	}

/* Draw a title */
	Result = pgSetColor( scpr->scpr_gc, 0.0, 0.0, 0.0 );
	CHECKRESULT(Result, "pgSetColor");

	Result = pgDrawRect( scpr->scpr_gc, 0.0, 0.0, 1.0, TEXT_OFFSET + TEXT_HEIGHT );
	CHECKRESULT(Result, "pgDrawRect");

	Result = pgSetColor( scpr->scpr_gc, 1.0, 1.0, 1.0 );
	CHECKRESULT(Result, "pgSetColor");

	Result = pgMoveTo( scpr->scpr_gc, LEFT_VISIBLE_EDGE, TEXT_OFFSET );
	CHECKRESULT(Result, "pgMoveTo");

	Result = pgDrawText( scpr->scpr_gc, "Scope Trace" );
	CHECKRESULT(Result, "pgDrawText");

/* Render the screen */
	Result = pgSwitchScreens(scpr->scpr_gc);
	CHECKRESULT(Result,"pgSwitchScreens");


cleanup:
	return Result;
}

/**************************************************************/
static Err ConnectScope( ScopeProbe *scpr, Item Inst, char *portName, int32 part )
{
	const Item DstInsItem = scpr->scpr_Probe;

#if DEBUG_Connect
	printf ("ConnectScope\n");
#endif

/* Try several connections. */
	return ConnectInstrumentParts( Inst, portName,  part, DstInsItem, "Input", 0 );
}

/**************************************************************/
static void DisconnectScope( ScopeProbe *scpr )
{
	const Item DstInsItem = scpr->scpr_Probe;

#if DEBUG_Connect
	printf ("DisconnectScope\n");
#endif

	DisconnectInstrumentParts( DstInsItem, "Input", 0 );
}

/**************************************************************/
/* Enter digital scope interactive mode.
**
**   Up/Down = change Y scale
**   Left/Right = pan left and right
**   Shift Left/Right = change X scale
**   A = acquire
**   C = return to faders.
*/
Err DoScope( ScopeProbe *scpr, Item inst, char *portName, int32 part )
{
	int32 Result;
	int32 XOffset = 0;
	float32 YScalar=1.0, XScalar=1.0;
	int32 sampleLength;
	int32 DoIt;
	ControlPadEventData cped;
	uint32 CurButtons, LastButtons = 0;
	int32 IfWaitButton = TRUE;

	Result = ConnectScope( scpr, inst, portName, part );
	CHECKRESULT( Result, "ConnectScope" );

	Result = CaptureScopeBuffer( scpr );
	CHECKRESULT( Result, "CaptureScopeBuffer" );

	sampleLength = scpr->scpr_Size / sizeof(int16);

	DoIt = TRUE;
	do
	{

		Result = DisplayScopeBuffer( scpr, XOffset, XScalar, YScalar );
		CHECKRESULT( Result, "DisplayScopeBuffer" );

		if ((Result = GetControlPad (1, IfWaitButton, &cped)) < 0) goto cleanup;
		CurButtons = cped.cped_ButtonBits;

		IfWaitButton = TRUE;

		if( ControlC & CurButtons & ~LastButtons )
		{
			DoIt = FALSE;
			/* break; */
		}
		if( ControlX & CurButtons & ~LastButtons )
		{
			DoIt = FALSE;
			/* break; */
		}

/* Set Vertical Gain. */
		if( ControlUp & CurButtons & ~LastButtons )
		{
			YScalar *= 2.0;
			if(YScalar > 256.0) YScalar = 256.0;
		}
		if( ControlDown & CurButtons & ~LastButtons )
		{
			YScalar *= 0.5;
			if(YScalar < 1.0/256.0) YScalar = 1.0/256.0;
		}
/* Set Time Axis. */
		if( ControlRight & CurButtons & ~LastButtons )
		{
			XScalar *= 2.0;
			if(XScalar > 256.0) XScalar = 256.0;
		}
		if( ControlLeft & CurButtons & ~LastButtons )
		{
			XScalar *= 0.5;
			if(XScalar < 1.0/256.0) XScalar = 1.0/256.0;
		}

/* Pan left or right continuously. */
		if( ControlRightShift & CurButtons )
		{
			XOffset += (int) 8.0 / XScalar;
			if(XOffset >= (sampleLength - (int) (8.0 * XScalar)))
				XOffset = sampleLength - (int) (8.0 * XScalar);
			IfWaitButton = FALSE;
		}
		if( ControlLeftShift & CurButtons )
		{
			XOffset -= (int) 8.0 / XScalar;
			if(XOffset < 0) XOffset = 0;
			IfWaitButton = FALSE;
		}

/* Acquire and display while A held down. */
		if( ControlA & CurButtons & ~LastButtons )
		{

			do
			{
				XOffset = 0;
				Result = CaptureScopeBuffer( scpr );
				CHECKRESULT( Result, "CaptureScopeBuffer" );

				Result = DisplayScopeBuffer( scpr,  XOffset, XScalar, YScalar );
				CHECKRESULT( Result, "DisplayScopeBuffer" );

				if ((Result = GetControlPad (1, FALSE, &cped)) < 0) goto cleanup;
			}
			while( cped.cped_ButtonBits & ControlA );
		}

		LastButtons = cped.cped_ButtonBits;

	} while( DoIt );

/* Clear buttons by waiting for all buttons up. */
	while( cped.cped_ButtonBits )
	{
		if ((Result = GetControlPad (1, TRUE, &cped)) < 0) goto cleanup;
	}

cleanup:
	DisconnectScope (scpr);
	return Result;
}

#endif /* } !defined(PATCHDEMO_NO_GUI) */
