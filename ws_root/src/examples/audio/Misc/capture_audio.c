
/******************************************************************************
**
**  @(#) capture_audio.c 96/07/29 1.16
**  $Id: capture_audio.c,v 1.8 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name capture_audio
|||	Record the output from the DSPP to a host file.
|||
|||	  Format
|||
|||	    capture_audio [num frames] [dest file]
|||
|||	  Description
|||
|||	    Captures the output from the DSP into a file on the development station's
|||	    filesystem. It can be used to check the sound output of a program at
|||	    important points.
|||
|||	    Once the output from the DSP has been captured, you can load it into
|||	    SoundHack and edit it as follows:
|||
|||	    1. From the File menu in SoundHack, choose Open Any.
|||
|||	    2. Select the file that was captured with capture_audio.
|||
|||	    3. Use the Header Change command and set the captured file's header to a
|||	    rate of 44100, with 2 channels, and a format of 16-bit Linear.
|||
|||	    4. Select Save As command and save the sound file as a 16-bit AIFF format
|||	    file.
|||
|||	    You can then load the AIFF file you created and listen to the captured
|||	    sounds.
|||
|||	  Arguments
|||
|||	    [num frames]
|||	        Number of sample frames to capture. Defaults to 10000.
|||
|||	    [dest file]
|||	        Name of the file to save the captured frames to. Defaults to
|||	        capture.raw. The pathname is a 3DO pathname. The data
|||	        is written to disk using the WriteRawFile().
|||
|||	  Caveats
|||
|||	    This program adds an instrument at priority 0. Because the execution order
|||	    of equal priority instruments depends isn't as predictable as those of
|||	    differing priority, this program may not be able to capture from other
|||	    instruments at priority 0.
|||
|||	  Associated Files
|||
|||	    capture_audio.c
|||
|||	  Location
|||
|||	    Examples/Audio/Misc
|||
|||	  See Also
|||
|||	    Sample(@) (Delay Lines)
**/

#include <audio/audio.h>
#include <file/fileio.h>
#include <kernel/cache.h>
#include <kernel/operror.h>
#include <stdio.h>
#include <stdlib.h>

#define NUMCHANNELS (2)   /* Stereo */

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
int main(int argc, char *argv[])
{
/* Declare local variables */
	Item DelayLine = 0, DelayIns = 0;
	Item DelayAtt;
	Item TapIns;
	Item SleepCue = 0;
	int32 NumBytes, NumFrames, NumTicks;
	int32 Result;
	char *fileName;
	uint16 *DelayData;

	PRT(("%s <#frames> <3DOFileName>\n", argv[0] ));

/* Get input parameters. */
	NumFrames = (argc > 1) ? atoi(argv[1]) : 10000 ;
	PRT(("Capture %d frames.\n", NumFrames ));
	NumBytes = NumFrames * NUMCHANNELS * sizeof(int16);

	fileName = (argc > 2) ? argv[2] : "capture.raw" ;

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

/* Load instrument to tap output.  Zero priority to be at end. */
	TapIns = LoadInstrument( "tapoutput.dsp",  0, 0);
	CHECKRESULT(TapIns,"LoadInstrument");

/* Create a Cue for signalback */
	SleepCue = CreateCue( NULL );
	CHECKRESULT(SleepCue, "CreateCue");

	DelayLine = CreateDelayLine( NumBytes, 2, FALSE );
	CHECKRESULT(DelayLine,"CreateDelayLine");

/* Find out the delay line address so we can save the data. */
    {
        TagArg Tags[] = {
            { AF_TAG_ADDRESS },
            TAG_END
        };

    	Result = GetAudioItemInfo(DelayLine, Tags);
    	CHECKRESULT(Result,"GetAudioItemInfo");
    	DelayData = (uint16 *) Tags[0].ta_Arg;
    }
/*
** Load the basic delay instrument which just writes data to
** an output DMA channel of the DSP.
*/
	DelayIns = LoadInstrument("delay_f2.dsp", 0, 50);
	CHECKRESULT(DelayIns,"LoadInstrument");

/* Attach the delay line to the delay instrument output. */
	DelayAtt = CreateAttachment( DelayIns, DelayLine, NULL );
	CHECKRESULT(DelayAtt,"AttachDelay");

/* Connect tap Instrument to Delays */
	Result = ConnectInstrumentParts (TapIns, "Output", AF_PART_LEFT, DelayIns, "Input", AF_PART_LEFT);
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstrumentParts (TapIns, "Output", AF_PART_RIGHT, DelayIns, "Input", AF_PART_RIGHT);
	CHECKRESULT(Result,"ConnectInstruments");

/* Start capturing sound. */
	Result = StartInstrument( TapIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( DelayIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

/* Wait for delay line to fill. */
	NumTicks = NumFrames / GetAudioDuration();
	SleepUntilTime( SleepCue, GetAudioTime() + NumTicks + 1  );

/* Before reading from delay line, flush any part of it which may be lingering
** in the data cache. This is necessary because the DSP can't affect the CPU's
** data cache. */
	FlushDCache (0, DelayData, NumBytes);

/* Write data to disk using raw file access routines. */
	Result = WriteDataToFile( (char *) DelayData, NumBytes, fileName );
	CHECKRESULT( Result, "WriteDataToFile" );

	PRT(("Captured audio written to file named %s\n", fileName ));

cleanup:
	DeleteDelayLine( DelayLine );
	UnloadInstrument( DelayIns );
	DeleteCue( SleepCue );
	UnloadInstrument( TapIns );

	CloseAudioFolio();
	PRT(("All done.\n"));
	return((int) Result);
}
