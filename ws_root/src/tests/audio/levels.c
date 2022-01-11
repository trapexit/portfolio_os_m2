/******************************************************************************
**
**  @(#) levels.c 96/05/01 1.1
** $Id: levels.c,v 1.1 1995/05/01 19:48:35 rnm Exp $
********************************************************************************/

#include <audio/audio.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/types.h>
#include <file/fileio.h>
#include <misc/event.h>
#include <stdarg.h>
#include <stdio.h>

#include <string.h>

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

Err MakeLevelPatch(void);
void UnmakeLevelPatch(void);
Err StartMeasuring(void);
Err StopMeasuring(RawFile* fileid);

/* Global instruments */
Item InputIns;
Item OutputIns = 0;

Item LeftMaxIns = 0, LeftMinIns = 0;
Item LeftMaxProbe = 0, LeftMinProbe = 0;
float32 LeftMaxVal = 0, LeftMinVal = 0;

Item RightMaxIns = 0, RightMinIns = 0;
Item RightMaxProbe = 0, RightMinProbe = 0;
float32 RightMaxVal = 0, RightMinVal = 0;

/***********************************************************************/
int main( int32 argc, char *argv[])
{
	int32 Result, doit;
	uint32 joy;
	RawFile* fileid;
	int32 MeasuringState;
	ControlPadEventData cped;

	PRT(("levels: Capture line in levels\n"));

/* Open a file for writing if requested on command line */
	if (argc > 1)
	{
		if ((Result = OpenRawFile( &fileid, argv[1], FILEOPEN_WRITE )) < 0)
		{
			ERR(("Couldn't open file for writing!\n"));
			return(Result);
		}
    }
	else
		fileid = NULL;

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, TRUE);
	if (Result < 0)
	{
		PrintError(0,"InitEventUtility",0,Result);
		return Result;
	}

/* Initialize instruments */
	Result = MakeLevelPatch();
	if (Result < 0)
	{
		PrintError(0,"MakeLevelPatch",0,Result);
		return Result;
	}

/* Loop, sampling min and max until STOP button pressed */
	MeasuringState = FALSE;
	doit = TRUE;
	while( doit )
	{
		Result = GetControlPad (1, TRUE, &cped);
		CHECKRESULT(Result, "GetControlPad");

		joy = cped.cped_ButtonBits;
		
		if ((joy & ControlStart) && !(MeasuringState))
		{
			PRT(("\n----\nStart capture ... "));
			Result = StartMeasuring();
			CHECKRESULT(Result, "StartMeasuring");
			MeasuringState = TRUE;
		} /* StartButton down */

		if (!(joy & ControlStart) && MeasuringState)
		{
			PRT(("stopped.\n"));
			Result = StopMeasuring(fileid);
			CHECKRESULT(Result, "StopMeasuring");
			MeasuringState = FALSE;
		} /* StartButton up */

		if (joy & ControlX)
		{
			if (MeasuringState)
			{
				Result = StopMeasuring(fileid);
				CHECKRESULT(Result, "StopMeasuring");
				MeasuringState = FALSE;
			}
			doit = FALSE;
		} /* StopButton */
	}

/* Clean up */

cleanup:
	UnmakeLevelPatch();
	TOUCH(Result);
	
	Result = KillEventUtility();
	if (Result < 0)
	{
		PrintError(0,"KillEventUtility",0,Result);
	}
	CloseAudioFolio();

	if (fileid) CloseRawFile( fileid );

	PRT(("\n----\n%s all done.\n", argv[0]));

	return Result;
}

/***********************************************************************/
Err MakeLevelPatch(void)
{
	int32 Result;

/* Set up audio input */
	Result = EnableAudioInput( TRUE, NULL );
	CHECKRESULT(Result,"EnableAudioInput");
	InputIns = LoadInstrument( "line_in.dsp", 0, 100);
	CHECKRESULT(InputIns,"LoadInstrument");

	DBUG(("Loaded line in\n"));
	
/* Set up minimum and maximum capture */
	LeftMinIns = LoadInstrument("minimum.dsp", 0, 50);
	CHECKRESULT(LeftMinIns,"LoadInstrument");
	LeftMaxIns = LoadInstrument("maximum.dsp", 0, 50);
	CHECKRESULT(LeftMaxIns,"LoadInstrument");

	DBUG(("Loaded left channel\n"));
	
	LeftMaxProbe = CreateProbe( LeftMaxIns, "Output", NULL );
	CHECKRESULT(LeftMaxProbe,"CreateProbe");
	LeftMinProbe = CreateProbe( LeftMinIns, "Output", NULL );
	CHECKRESULT(LeftMinProbe,"CreateProbe");

	Result = ConnectInstrumentParts( InputIns, "Output", 0, LeftMaxIns, "InputA", 0 );
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstruments( LeftMaxIns, "Output", LeftMaxIns, "InputB" );
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstrumentParts( InputIns, "Output", 0, LeftMinIns, "InputA", 0 );
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstruments( LeftMinIns, "Output", LeftMinIns, "InputB" );
	CHECKRESULT(Result,"ConnectInstruments");


	RightMinIns = LoadInstrument("minimum.dsp", 0, 50);
	CHECKRESULT(RightMinIns,"LoadInstrument");
	RightMaxIns = LoadInstrument("maximum.dsp", 0, 50);
	CHECKRESULT(RightMaxIns,"LoadInstrument");

	RightMaxProbe = CreateProbe( RightMaxIns, "Output", NULL );
	CHECKRESULT(RightMaxProbe,"CreateProbe");
	RightMinProbe = CreateProbe( RightMinIns, "Output", NULL );
	CHECKRESULT(RightMinProbe,"CreateProbe");

	Result = ConnectInstrumentParts( InputIns, "Output", 1, RightMaxIns, "InputA", 0 );
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstruments( RightMaxIns, "Output", RightMaxIns, "InputB" );
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstrumentParts( InputIns, "Output", 1, RightMinIns, "InputA", 0 );
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstruments( RightMinIns, "Output", RightMinIns, "InputB" );
	CHECKRESULT(Result,"ConnectInstruments");

/* Set up output */
	OutputIns = LoadInstrument("line_out.dsp",  0,  100);
	CHECKRESULT(OutputIns,"LoadInstrument");

/* Connect output of ADC to DAC. */
	Result = ConnectInstrumentParts (InputIns, "Output", 0, OutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts 0");
	Result = ConnectInstrumentParts (InputIns, "Output", 1, OutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstrumentParts 1");

/* Start up i/o */
	Result = StartInstrument( InputIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( OutputIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

cleanup:
	return(Result);
}

/***********************************************************************/
void UnmakeLevelPatch(void)
{
/* Stop everything, in case we're measuring still */
	StopInstrument( InputIns, NULL );
	StopInstrument( LeftMaxIns, NULL );
	StopInstrument( LeftMinIns, NULL );
	StopInstrument( RightMaxIns, NULL );
	StopInstrument( RightMinIns, NULL );
	StopInstrument( OutputIns, NULL );

/* Disable audio input */
	EnableAudioInput( FALSE, NULL );

/* Unload all the instruments */
	UnloadInstrument( InputIns );
    UnloadInstrument( LeftMinIns );
	UnloadInstrument( LeftMaxIns );
	UnloadInstrument( RightMinIns );
	UnloadInstrument( RightMaxIns );
	UnloadInstrument( OutputIns );

/* Delete the probes */
	DeleteProbe( LeftMaxProbe );
	DeleteProbe( LeftMinProbe );
	DeleteProbe( RightMaxProbe );
	DeleteProbe( RightMinProbe );

	return;
}

/***********************************************************************/
Err StartMeasuring(void)
{
	int32 Result;

/* Reset min, max to 0 */
	LeftMinVal = 1.0;
	LeftMaxVal = -1.0;
	RightMinVal = 1.0;
	RightMaxVal = -1.0;
	
/* Start capturing sound. */
	Result = StartInstrument( LeftMaxIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( LeftMinIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

	Result = StartInstrument( RightMaxIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( RightMinIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

cleanup:
	return(Result);
}

/***********************************************************************/
Err StopMeasuring(RawFile* fileid)
{
	int32 Result;
	char ResultBuffer[80];  /* for writing output line to file */

	StopInstrument( LeftMaxIns, NULL );
	StopInstrument( LeftMinIns, NULL );
	StopInstrument( RightMaxIns, NULL );
	StopInstrument( RightMinIns, NULL );

	Result = ReadProbe( LeftMaxProbe, &LeftMaxVal );
	CHECKRESULT( Result, "ReadProbe" );
	Result = ReadProbe( LeftMinProbe, &LeftMinVal );
	CHECKRESULT( Result, "ReadProbe" );
	Result = ReadProbe(RightMaxProbe, &RightMaxVal );
	CHECKRESULT( Result, "ReadProbe" );
	Result = ReadProbe(RightMinProbe, &RightMinVal );
	CHECKRESULT( Result, "ReadProbe" );

	DBUG(("LeftMin  = %8g, LeftMax  = %8g\n", LeftMinVal, LeftMaxVal ));
	DBUG(("RightMin = %8g, RightMax = %8g\n", RightMinVal,RightMaxVal ));

/* save or display results */
	if (fileid)
	{
		sprintf(ResultBuffer, "%8g\t%8g\t%8g\t%8g\n",
		  LeftMinVal, LeftMaxVal, RightMinVal, RightMaxVal);
		Result = WriteRawFile(fileid, ResultBuffer,
		  strlen(ResultBuffer));
    	CHECKRESULT(Result, "WriteRawFile");
	}
	else
    {
		PRT(("LeftMin  = %8g, LeftMax  = %8g\n", LeftMinVal, LeftMaxVal ));
		PRT(("RightMin = %8g, RightMax = %8g\n", RightMinVal,RightMaxVal ));
	}

cleanup:
	return(Result);
}


