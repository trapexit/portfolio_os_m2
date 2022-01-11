/* @(#) dspp_duck.c 96/06/10 1.24 */
/* $Id: dspp_duck.c,v 1.4 1995/01/31 21:06:12 peabody Exp phil $ */
/****************************************************************
**
** DSPP Duck and Cover Support
** When DIPIR occurs we either ramp the sound up or down.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/

#include <dspptouch/touch_hardware.h>

#include "audio_folio_modes.h"
#include "audio_internal.h"

/* #define DEBUG */
#define DBUG(x)     /* PRT(x) */

/* Constants ***************************************/

#define GAININCR  (25) /* Amount to change per frame. */
#define MAXTRYS  (200) /* Typically 1 try is enough but if the processor gets real fast... */
#define RAMP_UP    (0)
#define RAMP_DOWN  (1)

/* Global Data ************************************/

static int32 gGainBeforeDuck = 0x7FFF;

/* Code *******************************************/

static void RampAudioAmplitude( int32 IfRampDown )
{
	int32 Gain;
	int32 OldFrameCount;
	int32 FrameCount;
	int32 CountDown;
	int32 DoIt = TRUE;
	AudioKnob *aknob;
	DSPPResource *drsc;

/* Find out where Amplitude knob info is. */
	aknob = (AudioKnob *)CheckItem(AB_FIELD(af_NanokernelAmpKnob), AUDIONODE, AUDIO_KNOB_NODE);
	if (aknob == NULL) return;

	drsc = dsppKnobToResource(aknob);
	if( (drsc->drsc_Allocated < 0) || (drsc->drsc_Allocated >= DSPI_DATA_MEMORY_SIZE)) return;

	Gain = dsphReadDataMem( drsc->drsc_Allocated );
	if( IfRampDown )
	{
		gGainBeforeDuck = Gain;
	}

	OldFrameCount = ReadHardware(DSPX_FRAME_UP_COUNTER);

/* Ramp gain up or down each frame. */
	while( DoIt )
	{
		if( IfRampDown )
		{
			Gain -= GAININCR;  /* Reduce gain */
			if( Gain < 0 )
			{
				Gain = 0;
				DoIt = FALSE;
			}
		}
		else
		{
			Gain += GAININCR;  /* Reduce gain */
			if( Gain > gGainBeforeDuck )
			{
				Gain = gGainBeforeDuck;
				DoIt = FALSE;
			}
		}


		dsphWriteDataMem( drsc->drsc_Allocated, Gain );

/* Wait for next frame. */
		CountDown = MAXTRYS;
		do
		{
			if( CountDown-- <= 0 ) return;
			FrameCount = ReadHardware(DSPX_FRAME_UP_COUNTER);
		} while( OldFrameCount == FrameCount );
		OldFrameCount = FrameCount;
	}
}

/*******************************************************************************
** dsppInitDuckAndCover()
**
** Register Duck and Recover functions.
**
** Called during folio initialization.
*******************************************************************************/
Err dsppInitDuckAndCover( void )
{
	Err result;

	result = RegisterDuck( RampAudioAmplitude, RAMP_DOWN );
	if( result < 0 ) return result;
	result = RegisterRecover( RampAudioAmplitude, RAMP_UP );
	if( result < 0 ) return result;
	DBUG(("Installed Duck and Recover hooks.\n"));
	return result;
}

/* Remove IO Requests in case folio is shut down *******/
void dsppTermDuckAndCover( void )
{
	UnregisterDuck( RampAudioAmplitude );
	UnregisterRecover( RampAudioAmplitude );
}
