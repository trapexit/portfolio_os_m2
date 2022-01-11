/* @(#) capture_tool.h 96/07/29 1.10 */
/***************************************************************
**
** Capture the output of a DSP instrument in a delay line.
** Perform various measurements of recorded data including:
**    Look for the first non-zero sample, then checksum the next N samples.
**    Display signal using character graphics.
**
** By:  Phil Burk
**
** Copyright (c) 1995, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

#include <audio/audio.h>
#include <kernel/cache.h>
#include <kernel/operror.h>
#include <stdio.h>
#include <stdlib.h>     /* malloc() */
#include <string.h>     /* memset() */

#ifndef PRT
	#define	PRT(x)	{ printf x; }
#endif
#ifndef ERR
	#define	ERR(x)	PRT(x)
#endif
#ifndef DBUG
	#define	DBUG(x)	/* PRT(x) */
#endif

#ifndef CHECKRESULT
/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}
#endif

typedef struct AudioCapture
{
/* Declare local variables */
	Item    acap_DelayLine;
	Item    acap_DelayIns;
	Item    acap_DelayAtt;
	Item    acap_TestIns;
	Item    acap_AttCue;
	int32   acap_CueSignal;
	int32   acap_NumBytes;
	int32   acap_NumFrames;
	int32   acap_NumSamples;
	int32   acap_NumChannels;
	volatile uint16 *acap_DelayData;
} AudioCapture;

#define ACAP_VALID_DATA_MARKER   (0x4000)

/* Prototypes **************************************/
Err   acapCreate( int32 NumSamples, int32 NumChannels, AudioCapture **acapPtr );
Err   acapConnect( AudioCapture *acap, Item TestIns );
Err   acapStart( AudioCapture *acap );
Err   acapWait( AudioCapture *acap );
Err   acapCapture( AudioCapture *acap );
void   acapDisplay( AudioCapture *acap, int32 StartSample, int32 NumSamples );
int32 acapFirstNonZero( AudioCapture *acap );
int32 acapCheckSum( AudioCapture *acap, int32 StartSample, int32 NumSamples );
int32 acapAnalyse( AudioCapture *acap, int32 StartSample, int32 NumSamples,
	int32 *MinVal, int32 *MaxVal );
void   acapDelete( AudioCapture *acap );

/**********************************************************************
** Display single value by drawing stars on terminal.
**********************************************************************/
void PlotLine( int32 Val, int32 Min, int32 Max )
{
	int32 NumStars;
	int32 i;
#define PLOT_NUM_CHARS  (64)
	char StarBuffer[PLOT_NUM_CHARS+1];

	NumStars = ((Val-Min) * PLOT_NUM_CHARS) / (Max - Min);

	for( i=0; i<PLOT_NUM_CHARS; i++)
	{
		StarBuffer[i] = (i < NumStars) ? '*' : '.';
	}
	StarBuffer[PLOT_NUM_CHARS] = (char)0;

	PRT(("0x%08x : %s\n", Val, StarBuffer));
}

/**********************************************************************
**
**********************************************************************/
void   acapDelete( AudioCapture *acap )
{
	if( acap )
	{
		DeleteAttachment( acap->acap_DelayAtt );
		DeleteDelayLine( acap->acap_DelayLine );
		UnloadInstrument( acap->acap_DelayIns );
		DeleteCue( acap->acap_AttCue );
		free( acap );
	}
}
/**********************************************************************
**
**********************************************************************/
Err   acapCreate( int32 NumSamples, int32 NumChannels, AudioCapture **acapPtr )
{
	int32 Result;
	AudioCapture *acap;
	char *DelayName;

/* Allocate structure. */
	acap = malloc( sizeof(AudioCapture) );
	if( acap == NULL ) return -1;
	memset( acap, 0, sizeof(AudioCapture) );

	acap->acap_NumSamples = NumSamples;
	acap->acap_NumChannels = NumChannels;
	acap->acap_NumBytes = NumSamples * NumChannels * 2;
	acap->acap_NumFrames = NumSamples / NumChannels;

	acap->acap_DelayLine = CreateDelayLine( acap->acap_NumBytes, NumChannels, FALSE );
	CHECKRESULT(acap->acap_DelayLine,"CreateDelayLine");

/* Find out the delay line address so we can save the data. */
	{
		TagArg tags[] = {
			{ AF_TAG_ADDRESS, NULL },
			TAG_END
		};

		Result = GetAudioItemInfo(acap->acap_DelayLine, tags);
		CHECKRESULT(Result,"GetAudioItemInfo");
		acap->acap_DelayData = (uint16 *) tags[0].ta_Arg;
	}

/*
** Load the basic delay instrument which just writes data to
** an output DMA channel of the DSP.
*/
	DelayName = (NumChannels == 1) ? "delay_f1.dsp" : "delay_f2.dsp";
	acap->acap_DelayIns = LoadInstrument(DelayName, 0, 50);
	CHECKRESULT(acap->acap_DelayIns,"LoadInstrument");

/* Attach the delay line to the delay instrument output. */
	acap->acap_DelayAtt = CreateAttachment( acap->acap_DelayIns, acap->acap_DelayLine, NULL );
	CHECKRESULT(acap->acap_DelayAtt,"AttachDelay");

	acap->acap_AttCue = CreateCue( NULL );
	CHECKRESULT(acap->acap_AttCue,"CreateCue");
	Result = MonitorAttachment( acap->acap_DelayAtt, acap->acap_AttCue, CUE_AT_END );
	CHECKRESULT(Result,"MonitorAttachment");

	acap->acap_CueSignal = GetCueSignal( acap->acap_AttCue );

	*acapPtr = acap;
	return Result;

cleanup:
	acapDelete( acap );
	return Result;
}

/**********************************************************************
** Connect test Instrument to Delay
**********************************************************************/
Err   acapConnect( AudioCapture *acap, Item TestIns )
{
	int32 Result;

	acap->acap_TestIns = TestIns;

	Result = ConnectInstrumentParts (acap->acap_TestIns, "Output", 0, acap->acap_DelayIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstruments");
	if( acap->acap_NumChannels == 2 )
	{
		Result = ConnectInstrumentParts (acap->acap_TestIns, "Output", 1, acap->acap_DelayIns, "Input", 1);
		CHECKRESULT(Result,"ConnectInstruments");
	}

cleanup:
	return Result;
}

/**********************************************************************
**
**********************************************************************/
Err   acapStart( AudioCapture *acap )
{
	int32 Result;
	Result = StartInstrument( acap->acap_DelayIns, NULL );
	return Result;
}

/**********************************************************************
**
**********************************************************************/

#define acapFlushDCache(acap) FlushDCache (0, (acap)->acap_DelayData, (acap)->acap_NumBytes)


/**********************************************************************
**
**********************************************************************/
Err   acapWait( AudioCapture *acap )
{
/* Wait for delay line to fill. */
	WaitSignal( acap->acap_CueSignal );

/* flush data cache */
	acapFlushDCache(acap);

	return 0;
}

/**********************************************************************
**
**********************************************************************/
Err   acapCapture( AudioCapture *acap )
{
	int32 Result;
	Result = acapStart( acap );
	CHECKRESULT(Result, "acapStart");
	Result = acapWait( acap );
	CHECKRESULT(Result, "acapWait");
cleanup:
	return Result;
}

/**********************************************************************
**
**********************************************************************/
void   acapDisplay( AudioCapture *acap, int32 StartSample, int32 NumSamples )
{
	int32 i;
	int16 Data;
	for( i=0; i<(NumSamples); i++ )
	{
		Data = acap->acap_DelayData[StartSample+i];
		PRT(("%4d: ", i));
		PlotLine( Data, -0x8000, 0x7FFF );
	}
}
/**********************************************************************
**
**********************************************************************/
int32 acapFirstNonZero( AudioCapture *acap )
{
	int32 i;
	int32 FirstNonZeroFrame, FirstNonZeroSample = -1;
/* Scan for first non-zero data. */
	for( i=0; i<acap->acap_NumSamples; i++ ) /* !!! STEREO??? */
	{
		if( acap->acap_DelayData[i] != 0 )
		{
			FirstNonZeroSample = i;
			break;
		}
	}
/* Align First Sample to Frame boundary. */
	FirstNonZeroFrame = FirstNonZeroSample / acap->acap_NumChannels;
	FirstNonZeroSample = FirstNonZeroFrame * acap->acap_NumChannels;
	return FirstNonZeroSample;
}
/**********************************************************************
**
**********************************************************************/
int32 acapCheckSum( AudioCapture *acap, int32 StartSample, int32 NumSamples )
{
	int32 i;
	int16 Data;
	int32 MeasuredSum = 0;
	for( i=0; i<(NumSamples); i++ )
	{
		Data = acap->acap_DelayData[StartSample+i];
		MeasuredSum += ((uint32)Data) & 0xFFFF;
	}
	return MeasuredSum;
}

/**********************************************************************
**
**********************************************************************/
int32 acapAnalyse( AudioCapture *acap, int32 StartSample, int32 NumSamples,
	int32 *MinVal, int32 *MaxVal )
{
	int32 i;
	int16 Data;
	int32 Period = 0;
	int32 CurPeriod = 0;
	int32 TempMinimum = ((uint32)(-1)) >> 1;
	int32 TempMaximum = -TempMinimum;
	int32 TriggerLevel = 0;
	int32 RisingEdgeCount = 0;
	int32 State = TRUE;

	for( i=0; i<(NumSamples); i++ )
	{
		Data = acap->acap_DelayData[StartSample+i];
		if( Data > TempMaximum )
		{
			TempMaximum = Data;
			TriggerLevel = TempMaximum/20;
		}
		if( Data < TempMinimum )
		{
			TempMinimum = Data;
		}
/* State Driven period measurer. */
		if( TriggerLevel > 5 )
		{
			if( State )
			{
				if( Data < -TriggerLevel )
				{
					State = FALSE;
				}
			}
			else
			{
/* Rising edge trigger? */
				if( Data > TriggerLevel )
				{
					State = TRUE;
					RisingEdgeCount++;
					if( RisingEdgeCount == 2 )
					{
						Period = CurPeriod;
					}
					else if( RisingEdgeCount > 2 )
					{
						Period = (Period + CurPeriod)/2;
					}
					CurPeriod = 0;
				}
			}

		}
		CurPeriod++;
	}
	if( MinVal) *MinVal = TempMinimum;
	if( MaxVal) *MaxVal = TempMaximum;
	if( RisingEdgeCount < 2 )
	{
		ERR(("acapAnalyse: insufficient data to measure Period.\n"));
	}
	return Period;
}
