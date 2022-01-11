
/******************************************************************************
**
**  @(#) pd_scope.c 95/08/29 1.7
**
**  patchdemo oscilloscope module.  See patchdemo.c for documentation.
**
******************************************************************************/

#ifndef PATCHDEMO_NO_GUI    /* { */

	/* local */
#include "patchdemo.h"

	/* audiodemo */
#include <audiodemo/graphic_tools.h>    /* rendering */

	/* portfolio */
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
	DetachSample( scpr->scpr_Attachment );
	DeleteDelayLine( scpr->scpr_DelayLine );
	UnloadInstrument( scpr->scpr_Probe );
	FreeMem( scpr, sizeof(ScopeProbe) );
	return 0;
}

/**************************************************************/
Err CreateScopeProbe( ScopeProbe **resultScopeProbe, int32 NumBytes )
{
	ScopeProbe *scpr;
	int32 Result;
	TagArg Tags[2];
	int32 Time0, Time1;

/* init result */
	*resultScopeProbe = NULL;

/* allocate structue */
	if ((scpr = AllocMem( sizeof(ScopeProbe), MEMTYPE_FILL )) == NULL) {
	    Result = MakeStdUErr (ER_SEVERE, ER_NoMem);
	    goto cleanup;
	}

/* Create Probe Instrument */
/* Use low priority to get final output. */
	scpr->scpr_Probe = LoadInstrument( "delaymono.dsp", 0, 100 );
	CHECKRESULT(scpr->scpr_Probe,"LoadInstrument delaymono");

/* Create DelayLine */
#if DEBUG_ScopeProbe
	printf("Create probe %d bytes long\n", NumBytes );
#endif
	scpr->scpr_DelayLine = CreateDelayLine( NumBytes, 1, FALSE );
	CHECKRESULT(scpr->scpr_DelayLine,"CreateDelayLine");
	scpr->scpr_Size = NumBytes;
	Tags[0].ta_Tag = AF_TAG_ADDRESS;
	Tags[0].ta_Arg = NULL;
	Tags[1].ta_Tag = TAG_END;
	Result = GetAudioItemInfo(scpr->scpr_DelayLine, Tags);
	CHECKRESULT(Result,"GetAudioItemInfo");
	scpr->scpr_Data = (char *) Tags[0].ta_Arg;

/* Attach delay line. */
	scpr->scpr_Attachment = AttachSample(scpr->scpr_Probe,
		scpr->scpr_DelayLine, NULL );
	CHECKRESULT(scpr->scpr_Attachment,"AttachSample");
#if DEBUG_ScopeProbe
	printf("scpr->scpr_Attachment = 0x%x\n",
		scpr->scpr_Attachment );
#endif

/* Make CUE for monitoring delay line. */
	scpr->scpr_Cue = CreateCue( NULL );
	CHECKRESULT(scpr->scpr_Cue,"CreateCue");
	scpr->scpr_Signal = GetCueSignal( scpr->scpr_Cue );

/* MonitorAttachment() for Delays is broken until Portfolio 2.2 */
#define USE_MONITOR_ATTACHMENT
#ifdef USE_MONITOR_ATTACHMENT
/* Why does this cause the delay line to stop prematurely? */
/* Because of a bug in audiofolio that was fixed on 940810 and
** released in Portfolio 2.2 */
	Result = MonitorAttachment( scpr->scpr_Attachment, scpr->scpr_Cue, CUE_AT_END );
	CHECKRESULT(Result,"MonitorAttachment");
#endif

	Result = StartInstrument( scpr->scpr_Probe, NULL );
	CHECKRESULT(Result,"StartInstrument");

#ifdef USE_MONITOR_ATTACHMENT
/* Wait for signal to be returned. */
	WaitSignal( scpr->scpr_Signal );
#endif

/* return scpr pointer */
	*resultScopeProbe = scpr;
	return 0;

cleanup:
	DeleteScopeProbe( scpr );
	return Result;
}


/**************************************************************/
Err CaptureScopeBuffer( ScopeProbe *scpr, int32 XShift)
{
	int32 Result;

/* Start Instrument now. */
	Result = StartAttachment( scpr->scpr_Attachment, NULL );
	CHECKRESULT(Result,"StartInstrument");

#ifdef USE_MONITOR_ATTACHMENT
/* Wait for signal to be returned. */
	WaitSignal( scpr->scpr_Signal );
#else
	SleepAudioTicks( (SCOPE_MAX_SHOW << XShift) / 180 );    /* !!! no longer supported */
#endif

cleanup:
	return Result;
}


/**************************************************************/
#define SCOPE_LEFT_EDGE  (30)
#define SCOPE_Y_AXIS  (120)
Err DisplayScopeBuffer( ScopeProbe *scpr, int32 XOffset, int32 XShift, int32 YShift )
{
	int32 i, NumPnts;
	int16 *Data16Ptr;
	int32 Data32;
	int32 x,y;
	int32 si, j, SamplesPerPixel;
	int32 MinVal, MaxVal;

	ToggleScreen();
	ClearScreen();

	NumPnts =  256;

	SetFGPen( &GCon[0], MakeRGB15(31, 31, 0) );

	MoveTo( &GCon[0], SCOPE_LEFT_EDGE, SCOPE_Y_AXIS );
	Data16Ptr = scpr->scpr_Data;

	SamplesPerPixel = 1<<XShift;

/* Display samples on screen. */
	for( i=0; i<NumPnts; i++ )
	{
		x = i + SCOPE_LEFT_EDGE;
		si = i<<XShift;
		if( XShift > 0 )
		{
/* Scan for extreme values.  Always span or include zero. */
			MinVal = 0;
			MaxVal = 0;
			for( j = 0; j<SamplesPerPixel; j++ )
			{
				Data32 = Data16Ptr[j+si+XOffset];
				if( Data32 < MinVal ) MinVal = Data32;
				if( Data32 > MaxVal ) MaxVal = Data32;
			}
/* Scale Y to fit on screen. */
			y = SCOPE_Y_AXIS - (MinVal>>YShift);
			MoveTo( &GCon[0], x, y );
			y = SCOPE_Y_AXIS - (MaxVal>>YShift);
			DrawTo( CURBITMAPITEM,  &GCon[0], x, y );
		}
		else
		{
/* Draw line between each sample. */
			Data32 = Data16Ptr[si+XOffset];
			y = SCOPE_Y_AXIS - (Data32>>YShift);
			DrawTo( CURBITMAPITEM,  &GCon[0], x, y );
		}
	}

	return DisplayScreen (ScreenItems[ScreenSelect], 0);
}

/**************************************************************/
static Err ConnectScopePage( ScopeProbe *scpr, PatchPage *Page )
{
	const Item SrcInsItem = Page->ppage_Instrument->pinst_Instrument;
	const Item DstInsItem = scpr->scpr_Probe;

#if DEBUG_Connect
	printf ("ConnectScopeProbe\n");
#endif

/* Try several connections. */
	if (   ConnectInstruments( SrcInsItem, "Output",     DstInsItem, "Input" ) >= 0) return 0;
	return ConnectInstruments( SrcInsItem, "LeftOutput", DstInsItem, "Input" );
}

/**************************************************************/
static void DisconnectScope( ScopeProbe *scpr )
{
	const Item DstInsItem = scpr->scpr_Probe;

#if DEBUG_Connect
	printf ("DisconnectScope\n");
#endif

	DisconnectInstruments( 0, NULL, DstInsItem, "Input" );  /* @@@ note: this is the M2 method, but then only M2 insists on disconnection before reconnecting */
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
Err DoScope( ScopeProbe *scpr, PatchPage *Page )
{
	int32 Result;
	int32 XShift = 0, YShift = 9, XOffset = 0;
	int32 DoIt;
	ControlPadEventData cped;
	uint32 CurButtons, LastButtons = 0;
	int32 IfWaitButton = TRUE;

	Result = ConnectScopePage( scpr, Page );
	CHECKRESULT( Result, "ConnectScopePage" );

	Result = CaptureScopeBuffer( scpr, XShift );
	CHECKRESULT( Result, "CaptureScopeBuffer" );

	DoIt = TRUE;
	do
	{

		Result = DisplayScopeBuffer( scpr, XOffset, XShift, YShift );
		CHECKRESULT( Result, "DisplayScopeBuffer" );

		if ((Result = GetControlPad (1, IfWaitButton, &cped)) < 0) goto cleanup;
		CurButtons = cped.cped_ButtonBits;

		IfWaitButton = TRUE;

		if( ControlC & CurButtons & ~LastButtons )
		{
			DoIt = FALSE;
			break;
		}
		if( ControlX & CurButtons & ~LastButtons )
		{
			DoIt = FALSE;
			break;
		}

/* Set Vertical Gain. */
		if( ControlDown & CurButtons & ~LastButtons )
		{
			YShift++;
			if(YShift > 15) YShift = 15;
		}
		if( ControlUp & CurButtons & ~LastButtons )
		{
			YShift--;
			if(YShift < 1) YShift = 1;
		}
/* Set Time Axis. */
		if( ControlRightShift & CurButtons & ~LastButtons )
		{
			XShift--;
			if(XShift < 0) XShift = 0;
		}
		if( ControlLeftShift & CurButtons & ~LastButtons )
		{
			XShift++;
			if(XShift >  SCOPE_MAX_SHIFT ) XShift =  SCOPE_MAX_SHIFT;
		}

/* Pan left or right continuously. */
		if( ControlRight & CurButtons )
		{
			XOffset += 8 << XShift;
			if(XOffset > (SCOPE_MAX_SAMPLES - (SCOPE_MAX_SHOW<<XShift)))
				XOffset = SCOPE_MAX_SAMPLES - (SCOPE_MAX_SHOW<<XShift);
			IfWaitButton = FALSE;
		}
		if( ControlLeft & CurButtons )
		{
			XOffset -= 8 << XShift;
			if(XOffset < 0) XOffset = 0;
			IfWaitButton = FALSE;
		}

/* Acquire and display while A held down. */
		if( ControlA & CurButtons & ~LastButtons )
		{

			do
			{
				XOffset = 0;
				Result = CaptureScopeBuffer( scpr, XShift );
				CHECKRESULT( Result, "CaptureScopeBuffer" );

				Result = DisplayScopeBuffer( scpr, XOffset, XShift, YShift );
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
