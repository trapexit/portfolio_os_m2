/* @(#) capture_ins.c 96/06/14 1.18 */
/* $Id: capture_ins.c,v 1.3 1995/03/23 05:31:00 phil Exp phil $ */
/***************************************************************
**
** Capture the output of a DSP instrument in a delay line.
** Look for the first non-zero sample, then checksum the next N samples.
** Compare against expected checksum and report deviations.
** Optionally display signal using character graphics.
**
** See PrintHelp() routine below for more info.
**
** By:  Phil Burk
**
** Copyright (c) 1994, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <audio/audio.h>
#include <audio/score.h>
#include <audio/parse_aiff.h>
#include <stdarg.h>
#include <stdio.h>

#define DEFAULT_INS_NAME "sawtooth.dsp"
#define DEFAULT_SAMPLE_NAME         (NULL)
#define DEFAULT_NUM_FRAMES            (32)
#define DEFAULT_CHECKSUM               (0)
#define DEFAULT_RATE              (1.0)
#define DEFAULT_DISPLAY                (1)
#define DEFAULT_FRAMES_PREROLL      (50000)

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}

#include "capture_tool.h"

/*********************************************************************/
void PrintHelp( void )
{
	printf("Usage: capture_ins {options}\n");
	printf("   -iInsname       = DSP Instrument Name (default = %s)\n", DEFAULT_INS_NAME);
	printf("   -sSamplename    = Sample .AIF* or .RAW filename (default = NULL)\n");
	printf("   -nNumFrames     = Number of Sample Frames to Analyse (default = %d)\n", DEFAULT_NUM_FRAMES);
	printf("   -pPreRoll       = Number of Frames to PreRoll (default = %d)\n", DEFAULT_FRAMES_PREROLL);
	printf("   -cChecksum      = CheckSum (default = %d)\n", DEFAULT_CHECKSUM);
	printf("   -gGraphicsLevel = 0=No Display, 1=Terminal Plot (default = %d)\n", DEFAULT_DISPLAY);
	printf("   -rRate          = DetuneIn1000ths = (n/1000) * 22050\n");
	printf("   -fFreq          = in Hertz\n");
	printf("   -mPitch         = MIDI Note Index\n");
	printf("   -xSamplerate    = Sample Rate\n");
	printf("   -dDivisor       = Execution Rate Divisor\n");
	printf("   -l              = Loop until error occurs.\n");
	printf("   -k              = Suppress exit code\n");
	printf("   -oFileName      = write captured date to output filename if specified.\n");
}

/***********************************************************************/
Err WriteDataToFile( char *DataPtr, int32 NumBytes, char *fileName )
{
	RawFile *fid;
	int32 Result;

	Result = OpenRawFile(&fid, fileName, FILEOPEN_WRITE_NEW);
	if (Result < 0)
	{
		ERR(("Could not open %s\n", fileName));
		return Result;
	}

	Result = WriteRawFile( fid, DataPtr, NumBytes );
	if(Result != NumBytes)
	{
		ERR(("Error writing file.\n"));
		goto cleanup;
	}

cleanup:
	CloseRawFile( fid );
	return Result;
}


/***********************************************************************/
Err LoadRawSample( char *fileName )
{
	RawFile  *fid;
	int32     Result;
	FileInfo  FI;
	int32     numBytes;
	uint8    *data = NULL;
	Item      sampleItem;

	Result = OpenRawFile(&fid, fileName, FILEOPEN_READ);
	if (Result < 0)
	{
		ERR(("LoadRawSample: Could not open %s\n", fileName));
		return Result;
	}

	
	Result = GetRawFileInfo(fid, &FI, sizeof(FileInfo));
	if(Result < 0)
	{
		ERR(("LoadRawSample: Error getting file info.\n"));
		goto cleanup;
	}
	numBytes = FI.fi_ByteCount;

	data = AllocMem( numBytes, MEMTYPE_TRACKSIZE );
	if( data == NULL )
	{
		ERR(("LoadRawSample: Error allocating memory.\n"));
		goto cleanup;
	}
	
	Result = ReadRawFile( fid, data, numBytes );
	if(Result != numBytes)
	{
		ERR(("LoadRawSample: Error reading file.\n"));
		goto cleanup;
	}

	sampleItem = CreateSampleVA (
        	AF_TAG_AUTO_FREE_DATA, TRUE,
		AF_TAG_ADDRESS,        data,
		AF_TAG_FRAMES,         numBytes,
		AF_TAG_CHANNELS,       1,
		AF_TAG_WIDTH,          1,
		TAG_END);
	if(sampleItem <= 0)
	{
		ERR(("LoadRawSample: Error creating sample.\n"));
		goto cleanup;
	}

	PRT(("Loaded RAW file containing %d bytes.\n", numBytes ));

	CloseRawFile( fid );
	return sampleItem;

cleanup:
	CloseRawFile( fid );
	if( data ) FreeMem( data, TRACKED_SIZE );
	return Result;
}

#define FREQ_TYPE_NONE        (0)
#define FREQ_TYPE_RATE        (1)
#define FREQ_TYPE_FREQ        (2)
#define FREQ_TYPE_SAMPLE_RATE (3)
#define FREQ_TYPE_DETUNE      (4)
#define FREQ_TYPE_PITCH       (5)

int32 QueryNumOutputs( Item InsOrTemplate )
{
	int32 Result;
	InstrumentPortInfo  PINFO;

	Result = GetInstrumentPortInfoByName ( &PINFO, sizeof(PINFO),
                             InsOrTemplate, "Output");
	if( Result < 0 ) return Result;
	return PINFO.pinfo_NumParts;
}

/**********************************************************************
** Capture output of instrument and do CheckSum
**********************************************************************/
int32 CaptureIns( char *InsName, char *SampleName, float32 Rate, int32 NumSum, int32 FramesPreroll,
	int32 CheckSum, int32 DisplayLevel, int32 Pitch, int32 FreqType,
	int32 calcRateDivide, char *fileName )
{

/* Declare local variables */
	AudioCapture *acap = NULL;
	Item    SampleItem = 0, SampleAtt = 0;
	Item    TestTemplate, TestIns;
	int32   Result;
	int32   NumChannels, NumFrames, NumSamples;
	int32   FirstNonZeroSample;
	int32   CheckSumResult = 0, FreqResult = 0;

/* Load instrument to be tested. */
    TestTemplate = LoadScoreTemplate (InsName);
	CHECKRESULT(TestTemplate,"LoadScoreTemplate");
	TestIns = CreateInstrumentVA (TestTemplate,
        AF_TAG_CALCRATE_DIVIDE, calcRateDivide,
        TAG_END);
	CHECKRESULT(TestIns,"CreateInstrument");

/* Load digital audio AIFF Sample file from disk. */
	if( SampleName != NULL )
	{
		if( strcasecmp(".raw", &SampleName[strlen(SampleName)-4]) == 0 )
		{
			SampleItem = LoadRawSample(SampleName);
			CHECKRESULT(SampleItem,"LoadSample");
		}
		else
		{
			SampleItem = LoadSample(SampleName);
			CHECKRESULT(SampleItem,"LoadSample");
		}

/* Attach the sample to the instrument for playback. */
		SampleAtt = CreateAttachment(TestIns, SampleItem, NULL);
		if( SampleAtt > 0 )
		{
			DBUG(("Sample %s attached\n", SampleName));
		}
	}

/* Query for the number of parts for "Output". */
	NumChannels = QueryNumOutputs( TestIns );
	if( NumChannels < 0 )
	{
		Result = NumChannels;
		ERR(("Instrument has no Output! 0x%x\n", Result));
		goto cleanup;
	}
	DBUG(("NumChannels = %d\n", NumChannels));

	NumFrames = NumSum + FramesPreroll;  /* Give time for instrument to fire up. */
	NumSamples = NumFrames * NumChannels;
	DBUG(("NumFrames = 0x%x\n", NumFrames));
	Result = acapCreate( NumSamples, NumChannels, &acap );
	CHECKRESULT(Result,"acapCreate");

	Result = acapConnect( acap, TestIns );
	CHECKRESULT(Result,"acapConnect");

/* Start capturing sound. */
	Result = acapStart( acap );
	CHECKRESULT(Result,"acapStart");

#if 1
	switch( FreqType )
	{
	case  FREQ_TYPE_NONE:
		Result = StartInstrument( TestIns, NULL );
		break;
	case FREQ_TYPE_RATE:
		Result = StartInstrumentVA( TestIns, AF_TAG_RATE_FP, ConvertFP_TagData(Rate), TAG_END );
		break;
	case FREQ_TYPE_FREQ:
		Result = StartInstrumentVA( TestIns, AF_TAG_FREQUENCY_FP, ConvertFP_TagData(Rate), TAG_END );
		break;
	case FREQ_TYPE_PITCH:
		Result = StartInstrumentVA( TestIns, AF_TAG_PITCH, Pitch, TAG_END );
		break;
	case FREQ_TYPE_SAMPLE_RATE:
		Result = StartInstrumentVA( TestIns, AF_TAG_SAMPLE_RATE_FP, ConvertFP_TagData(Rate), TAG_END );
		break;
	case FREQ_TYPE_DETUNE:
	default:
		PRT(("Invalid FreqType = %d\n", FreqType ));
		break;
	}
	CHECKRESULT(Result,"StartInstrument");
#else
	{
		Item FreqKnob;
		FreqKnob = CreateKnob( TestIns, "Frequency", NULL );
		CHECKRESULT(FreqKnob,"StartInstrument");

		Result = SetKnob( FreqKnob, Rate );
		CHECKRESULT(Result,"SetRawKnob");

		Result = DeleteKnob( FreqKnob );
		CHECKRESULT(Result,"DeleteKnob");

		Result = StartInstrument( TestIns, NULL );
		CHECKRESULT(Result,"StartInstrument");
	}
#endif
	Result = acapWait( acap );
	CHECKRESULT(Result,"acapWait");

/* Stop instruments. */
	Result = StopInstrument( TestIns, NULL );
	CHECKRESULT(Result,"StopInstrument");

/* Scan for first non-zero data. */
	FirstNonZeroSample = acapFirstNonZero( acap );

	if( FirstNonZeroSample < 0 )
	{
		ERR(("CaptureIns: all data zero!\n"));
	}
	else if( (FirstNonZeroSample + NumSum) > NumSamples )
	{
		ERR(("CaptureIns: did not capture full signal! Insufficient pre-roll.\n"));
	}
	else
	{
		uint32 MeasuredSum;

		if( FirstNonZeroSample == 0 )
		{
			ERR(("CaptureIns: warning, first sample is non-zero.\n"));
			ERR(("            May have missed some data.\n"));
		}

/* Measure checksum if check sum requested. */
		if( CheckSum != 0 )
		{
			MeasuredSum = acapCheckSum( acap, FirstNonZeroSample, NumSum );
			if( MeasuredSum != CheckSum )
			{
				PRT(("CHECKSUM ERROR - MeasuredSum = 0x%08x, ExpectedSum = 0x%08x, InsName = %s\n",
					MeasuredSum, CheckSum, InsName ));
				CheckSumResult = -1;
			}
			else
			{
				PRT(("CHECKSUM OK = 0x%08x, InsName = %s\n", CheckSum, InsName ));
			}
		}

		if( DisplayLevel == 1)
		{
			acapDisplay( acap, FirstNonZeroSample, NumSum );
		}

/* Measure frequency if a frequency requested. */
		if( FreqType != FREQ_TYPE_NONE )
		{
			int32   Period, MinVal, MaxVal;
			float32 MeasuredRate, MeasuredFreq, ExpectedFreq, Ratio;

			Period = acapAnalyse( acap, FirstNonZeroSample, NumSum, &MinVal, &MaxVal );
			PRT(("Period = %d, MinVal = 0x%x, MaxVal = 0x%x\n", Period, MinVal, MaxVal ));
			if( Period > 0 )
			{
		/* What is the Rate relative to sample rate. */
			MeasuredRate = 2.0 / (float32) Period;
		/* What is the Freq in Hertz. */
			MeasuredFreq = 44100.0 / (float32) Period;
			PRT(("Measured Rate = %g, Measured Freq = %g Hertz\n",
				   MeasuredRate, MeasuredFreq ));

			switch( FreqType )
			{
			case FREQ_TYPE_FREQ:
				ExpectedFreq = Rate;
				break;
			case FREQ_TYPE_PITCH:
				{
					float32 Fraction;
					Convert12TET_FP( Pitch-AF_A440_PITCH, 0, &Fraction );
					ExpectedFreq = Fraction * 440.0;
				}
				break;
			default:
				ExpectedFreq = 440.0;
				break;
			}

			Ratio = MeasuredFreq / ExpectedFreq;
			PRT(("Desired Freq = %g, Ratio = %g\n", ExpectedFreq, Ratio ));

			if( (Ratio < 0.98) || (Ratio > 1.02) )
			{
				PRT(("ERROR - Freq test FAILED! InsName = %s\n",  InsName ));
				FreqResult = -2;
			}
			else
			{
				PRT(("SUCCESS - Freq test OK, InsName = %s\n",  InsName ));
			}
			}
		}
	}


/* Write data to disk using file access routines. */
	if( fileName != NULL)
	{
		Result = WriteDataToFile( (char *) &acap->acap_DelayData[FirstNonZeroSample],
			NumSum*sizeof(int16), fileName );
		CHECKRESULT( Result, "WriteDataToFile" );
		PRT(("Captured audio written to file named %s\n", fileName ));
	}

cleanup:
	DeleteAttachment( SampleAtt );
	UnloadScoreTemplate( TestTemplate );
	UnloadSample( SampleItem );
	acapDelete( acap );

	if( CheckSumResult < 0 ) return CheckSumResult;
	if( FreqResult < 0 ) return FreqResult;
	return(Result);
}

/***********************************************************************/
int main(int argc, char *argv[])
{
	char   *InsName, *SampleName;
	int32   NumFrames;
	int32   FramesPreroll;
	int32   CheckSum;
	char   *s, c;
	char   *fileName = NULL;
	int     i;
	int32   Result;
	int32   DisplayLevel;
	int32   Pitch = 0;
	int32   FreqType = FREQ_TYPE_NONE;
	int32   calcRateDivide = 1;
	float32 Rate;
	int32   ifLoop = FALSE;
	bool keepGoing = FALSE;

	InsName =  DEFAULT_INS_NAME;
	SampleName =  DEFAULT_SAMPLE_NAME;
	NumFrames = DEFAULT_NUM_FRAMES;
	FramesPreroll = DEFAULT_FRAMES_PREROLL;
	CheckSum =  DEFAULT_CHECKSUM;
	DisplayLevel =  DEFAULT_DISPLAY;
	Rate = DEFAULT_RATE;

/* Get input parameters. */
	for( i=1; i<argc; i++ )
	{
		s = argv[i];

		if( *s++ == '-' )
		{
			c = *s++;
			switch(c)
			{
			case 'i':
				InsName = s;
				break;
			case 's':
				SampleName = s;
				break;
			case 'n':
				NumFrames = atoi(s);
				break;
			case 'p':
				FramesPreroll = atoi(s);
				break;
			case 'r':
				Rate = (float32)atoi(s) / 1000.0;
				FreqType = FREQ_TYPE_RATE;
				break;
			case 'f':
				Rate = (float32)atoi(s);
				FreqType = FREQ_TYPE_FREQ;
				break;
			case 'm':
				Pitch = atoi(s);
				FreqType = FREQ_TYPE_PITCH;
				break;
			case 'x':
				Rate = atoi(s);
				FreqType = FREQ_TYPE_SAMPLE_RATE;
				break;
			case 'c':
				CheckSum = atoi(s);
				if( CheckSum == 0 ) PRT(("-c0 means don't do a CheckSum!\n"));
				break;
			case 'g':
				DisplayLevel = atoi(s);
				break;
			case 'd':
				calcRateDivide = atoi(s);
				break;
			case 'l':
				ifLoop = TRUE;
				break;
			case 'k':
				keepGoing = TRUE;
				break;
			case 'o':
				fileName = s;
				break;

			case '?':
			default:
				PrintHelp();
				exit(1);
				break;
			}
		}
		else
		{
			PrintHelp();
			exit(1);
		}
	}


/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

	DBUG(("\nCaptureIns: Ins = %s, Sample = %s, DivideRate = %d\n",
		 InsName, ((SampleName) ? SampleName : "NULL"), calcRateDivide));
	DBUG(("CaptureIns: NumFrames = %d, FreqType = %d, Rate = %g\n",
		 NumFrames, FreqType, Rate ));
	do
	{
		Result = CaptureIns( InsName, SampleName, Rate, NumFrames, FramesPreroll,
			CheckSum, DisplayLevel, Pitch, FreqType, calcRateDivide, fileName );
		PRT(("---------------\n"));
	} while( ifLoop && (Result >= 0) );
	if (keepGoing) Result = 0;

	CloseAudioFolio();
	PRT(("%s finished with Result = 0x%x\n", argv[0], Result));
	return (int) Result;
}
