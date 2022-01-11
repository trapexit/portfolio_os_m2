
/******************************************************************************
**
**  @(#) dspfaders.c 96/08/28 1.50
**
******************************************************************************/

/**
|||	AUTODOC -class Shell_Commands -group Audio -name dspfaders
|||	Try out DSP instruments or patches.
|||
|||	  Format -preformatted
|||
|||	    dspfaders [-2|-8] [-LineIn] [-Mute] [-NoLineOut]
|||	        <instrument name> [-sample <sample name>]
|||	        [ <instrument name> [-sample <sample name>] ... ]
|||
|||	  Description
|||
|||	    This program is a diagnostic and experimentation tool to work with DSP
|||	    instruments and patches. It loads the named DSP instruments or patches,
|||	    and connects them in a chain by connecting the output port "Output" of
|||	    one to the input port "Input" of the next. You may specify as many
|||	    instrument or patch names as you wish. If the last instrument in the
|||	    chain has an output named "Output", it is connected to line_out.dsp(@). If
|||	    the first instrument in the chain has an input named "Input", a signal
|||	    (either noise.dsp(@) or line_in.dsp(@)) is injected into this port.
|||
|||	    If an instrument name is followed by a -sample argument, the program
|||	    attempts to attach the named sample to the instrument.
|||
|||	    The instrument is triggered using the 3DO control pad. Faders representing
|||	    instrument knobs are displayed on the 3DO display, and can be controlled
|||	    using the 3DO control pad.
|||
|||	    Also displays tick benchmarking information about the instrument being
|||	    tested:
|||
|||	    Alloc
|||	        Number of ticks per frame that instrument allocates.
|||
|||	    Cur
|||	        Number of ticks required in the current frame for the instrument to
|||	        execute.
|||
|||	    Max
|||	        Largest value of Cur since the program was started.
|||
|||	  Arguments
|||
|||	    <instrument name>
|||	        Name of a DSP instrument or binary patch file to demo (e.g.,
|||	        svfilter.dsp(@)).
|||
|||	    -sample <sample name>
|||	        Sample to connect to sample playing instruments (e.g.,
|||	        sampler_16_v1.dsp(@)). If not specified, no sample is connected to the
|||	        instrument. There is also no provision to ensure that the sample format
|||	        and the sample player match. The sample is attached to the hook named
|||	        'InFIFO'.
|||
|||	    -2
|||	        Run test instruments at 1/2 rate. Defaults to full rate.
|||
|||	    -8
|||	        Run test instruments at 1/8 rate. Defaults to full rate.
|||
|||	    -LineIn
|||	        By default, white noise (noise.dsp(@)) is injected into the port named
|||	        Input of the first test instrument. When this switch is used, line_in.dsp(@)
|||	        is injected instead. If the first test instrument has no port named Input,
|||	        no signal is injected.
|||
|||	    -Mute
|||	        Set Amplitude knob (if there is one) to 0 before starting instruments.
|||
|||	    -NoLineOut
|||	        Do not connect last output to line_out.dsp. It is still available for
|||	        oscilloscope however. This is useful when you want to put tapoutput.dsp
|||	        on the scope.
|||
|||	  Controls
|||
|||	    Fader Mode:
|||
|||	    A
|||	        Toggle between Start and Release of instrument chain.
|||
|||	    LShift-A
|||	        Reset max tick accumulator.
|||
|||	    B
|||	        Start instrument chain when pressed, Release instrument chain when
|||	        released (like a key on a MIDI keyboard).
|||
|||	    C
|||	        Reset knobs to default values.
|||
|||	    P
|||	        Enter oscilloscope mode.
|||
|||	    X
|||	        Quit
|||
|||	    Up, Down
|||	        Select a fader by moving up or down.
|||
|||	    Left, Right
|||	        Adjust current fader (coarse).
|||
|||	    LShift + Left, Right
|||	        Adjust current fader (fine).
|||
|||	    LShift + Up, Down
|||	        Switch to previous/next instrument fader screen.
|||
|||	    Oscilloscope Mode:
|||
|||	    A
|||	        Capture new sample into oscilloscope.
|||
|||	    C, X
|||	        Return to fader mode.
|||
|||	    Up, Down
|||	        Increase, decrease vertical magnification of oscilloscope.
|||
|||	    Left, Right
|||	        Decrease, increase horizontal magnification of oscilloscope.
|||
|||	    LShift, RShift
|||	        Scroll oscilloscope left, right.
|||
|||	  Implementation
|||
|||	    Command implemented in V24.
|||
|||	  Location
|||
|||	    System.m2/Programs/dspfaders
|||
|||	  See Also
|||
|||	    makepatch(@)
**/

#include <audio/audio.h>
#include <audio/parse_aiff.h>
#include <audio/score.h>            /* LoadScoreTemplate() */
#include <audiodemo/faders.h>
#include <audiodemo/portable_graphics.h>
#include <audiodemo/scope.h>
#include <kernel/mem.h>
#include <misc/event.h>
#include <stdio.h>
#include <string.h>

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		TOUCH(Result); \
		PrintError (NULL, name, NULL, Result); \
		goto cleanup; \
	}

#define TestLeadingEdge(newset,oldset,mask)  ( (newset) & ~(oldset) & (mask) )
#define TestTrailingEdge(newset,oldset,mask) \
	TestLeadingEdge ((oldset),(newset),(mask))

#define PRT(x)          { printf x; }
#define ERR(x)          PRT(x)
#define DBUG(x)         /* PRT(x) */
#define DBUGTICKS(x)    /* PRT(x) */
#define DBUGKNOB(x)     /* PRT(x) */

#define MIXER_PRIORITY (40)
#define LOW_PRIORITY   (50)
#define TEST_PRIORITY  (90)                 /* so you can tap the output of other things */
#define HIGH_PRIORITY  (150)

#define MAX_KNOBS MAX_FADERS
#define MAX_INSTS (10)

typedef struct KnobExtra {
	InstrumentPortInfo knbe_PortInfo;       /* port info for knob */
	Item    knbe_Knob;                      /* knob item */
} KnobExtra;

#define FADER_MAX_NAME_SIZE (AF_MAX_NAME_SIZE + 1 + 5 + 1)   /* space for: name[nnnnn] */

typedef struct FaderExtra {                 /* fdr_UserData */
	Fader  *fdre_Fader;                     /* associated fader */
	const KnobExtra *fdre_KnobExtra;        /* associated KnobExtra */
	int32   fdre_PartNum;                   /* part number of knob controlled by this fader */
	char    fdre_Name[FADER_MAX_NAME_SIZE]; /* fader name buffer */
	float32 fdre_Default;                   /* default value for fader */
} FaderExtra;

static ScopeProbe *gFaderScope = NULL;

	/* test instruments */

typedef struct TestInstrumentData {
	Node        ti_Node;                    /* for list traversal */
	char*       ti_InstrumentName;
	char*       ti_SampleName;              /* point at argv strings */
	Item        ti_Template;
	Item        ti_Sample;
	Item        ti_Instrument;
	KnobExtra   ti_KnobExtra[MAX_KNOBS];    /* extra knob info for this instrument */
	Fader       ti_Faders[MAX_FADERS];      /* faders in this instrument's block */
	FaderExtra	ti_FaderExtra[MAX_FADERS];  /* knob-related fader information */
	FaderBlock  ti_FaderBlock;              /* all this instrument's fader info */
	int32       ti_NumKnobs;                /* number of knobs for this instrument */
} TestInstrumentData;

static List gTestInsList;
static int32 gNumInsts;

/* !!! some of this ought to go to faders.h or portable_graphics.h */
#define TOP_VISIBLE_EDGE    (0.1)
#define LINE_HEIGHT         (0.1)
#define ROW_1               (TOP_VISIBLE_EDGE)
#define COL_1               (0.550)
#define COL_2               (0.675)
#define COL_3               (0.800)

	/* test flags */
#define TEST_F_LINE_IN      0x0001
#define TEST_F_MUTE         0x0002
#define TEST_F_LINE_OUT     0x0004

static void SetFaderDefaults( TestInstrumentData* thisIns );

/********************************************************************/
/*********** Benchmark DSP Tools ************************************/
/********************************************************************/
typedef struct BenchmarkPatch
{
	Item   bmp_BenchBefore;
	Item   bmp_BenchAfter;
	Item   bmp_Maximum;
	Item   bmp_Subtract;
	Item   bmp_CurTicksProbe;
	Item   bmp_MaxTicksProbe;
} BenchmarkPatch;

/********************************************************************/
void DeleteBenchmarkPatch( BenchmarkPatch *bmp )
{
	if( bmp == NULL ) return;

	UnloadInstrument( bmp->bmp_BenchBefore );
	UnloadInstrument( bmp->bmp_BenchAfter );
	UnloadInstrument( bmp->bmp_Maximum );
	UnloadInstrument( bmp->bmp_Subtract );
	DeleteProbe( bmp->bmp_CurTicksProbe );
	DeleteProbe( bmp->bmp_MaxTicksProbe );

	FreeMem( bmp, sizeof( BenchmarkPatch ) );
}

/********************************************************************/
BenchmarkPatch *CreateBenchmarkPatch( uint8 calcRateDivide, int32 HighPri, int32 LowPri )
{
	BenchmarkPatch *bmp;
	Err Result;

	bmp = (BenchmarkPatch *) AllocMem( sizeof( BenchmarkPatch ), MEMTYPE_FILL );
	if( bmp == NULL ) return NULL;

/* Create instruments */
	bmp->bmp_BenchBefore = LoadInstrument( "benchmark.dsp", calcRateDivide, HighPri );
	CHECKRESULT( bmp->bmp_BenchBefore, "LoadInstrument benchmark" );

	bmp->bmp_BenchAfter = LoadInstrument( "benchmark.dsp", calcRateDivide, LowPri );
	CHECKRESULT( bmp->bmp_BenchAfter, "LoadInstrument benchmark" );

	bmp->bmp_Subtract = LoadInstrument( "subtract.dsp", calcRateDivide, LowPri-2 );
	CHECKRESULT( bmp->bmp_Subtract, "LoadInstrument subtract" );

	bmp->bmp_Maximum = LoadInstrument( "maximum.dsp", calcRateDivide, LowPri-3 );
	CHECKRESULT( bmp->bmp_Maximum, "LoadInstrument maximum" );

/* Connect them into a patch. */
	Result = ConnectInstruments( bmp->bmp_BenchBefore, "Output",
	/**/                         bmp->bmp_Subtract, "InputB" );
	CHECKRESULT( Result, "ConnectInstruments" );

	Result = ConnectInstruments( bmp->bmp_BenchAfter, "Output",
	/**/                         bmp->bmp_Subtract, "InputA" );
	CHECKRESULT( Result, "ConnectInstruments" );

	Result = ConnectInstruments( bmp->bmp_Subtract, "Output",
	/**/                         bmp->bmp_Maximum, "InputA" );
	CHECKRESULT( Result, "ConnectInstruments" );
/* Loop output of Maximum back to input for historical maximum. */
	Result = ConnectInstruments( bmp->bmp_Maximum, "Output",
	/**/                         bmp->bmp_Maximum, "InputB" );
	CHECKRESULT( Result, "ConnectInstruments" );

/* Attach Probes to outputs. */
	bmp->bmp_CurTicksProbe = CreateProbeVA( bmp->bmp_Subtract, "Output",
		AF_TAG_TYPE, AF_SIGNAL_TYPE_WHOLE_NUMBER, TAG_END);
	CHECKRESULT( Result, "CreateProbe" );
	bmp->bmp_MaxTicksProbe = CreateProbeVA( bmp->bmp_Maximum, "Output",
		AF_TAG_TYPE, AF_SIGNAL_TYPE_WHOLE_NUMBER, TAG_END);
	CHECKRESULT( Result, "CreateProbe" );

/* Start everything. */
	Result = StartInstrument( bmp->bmp_BenchBefore, NULL );
	CHECKRESULT( Result, "StartInstrument" );
	Result = StartInstrument( bmp->bmp_BenchAfter, NULL );
	CHECKRESULT( Result, "StartInstrument" );
	Result = StartInstrument( bmp->bmp_Subtract, NULL );
	CHECKRESULT( Result, "StartInstrument" );
	Result = StartInstrument( bmp->bmp_Maximum, NULL );
	CHECKRESULT( Result, "StartInstrument" );

	return bmp;

cleanup:
	DeleteBenchmarkPatch( bmp );
	return NULL;
}

/********************************************************************/
static Err ResetBenchmarkPatch (BenchmarkPatch *bmp)
{
	return StartInstrument (bmp->bmp_Maximum, NULL);
}

/********************************************************************/
Err ReadBenchmarkPatch( BenchmarkPatch *bmp, int32 *CurTicksPtr, int32 *MaxTicksPtr )
{
	Err Result;
	float32 CurTicksFP, MaxTicksFP;

	Result = ReadProbe( bmp->bmp_CurTicksProbe, &CurTicksFP );
	CHECKRESULT( Result, "ReadProbe" );
	*CurTicksPtr = CurTicksFP;
	Result = ReadProbe( bmp->bmp_MaxTicksProbe, &MaxTicksFP );
	CHECKRESULT( Result, "ReadProbe" );
	*MaxTicksPtr = MaxTicksFP;

cleanup:
	return Result;
}

/********************************************************************/
static void DrawTicks (float32 X, float32 Y, const char *desc, int32 ticks, bool warn)
{
	char b[64];

	sprintf (b, "%s: ", desc);
	pgSetColor (gPGCon, 1.0, 1.0, 1.0);
	pgMoveTo (gPGCon, X, Y);
	pgDrawText (gPGCon, b);

	sprintf (b, "%d", ticks);
	if (warn) pgSetColor (gPGCon, 0.9, 0.3, 0.2);
	pgDrawText (gPGCon, b);
}

/********************************************************************/
static Err RunFaders (uint8 calcRateDivide, bool enableScope)
{
	bool doit = TRUE;
	Err Result = 0;
	uint32 joy, oldjoy=0;
	bool IfNoteOn = TRUE;
	int32 allocatedTicks;
	int32 overheadTicks;
	BenchmarkPatch *bmp = NULL;
	Item SleepCue;
	ControlPadEventData cped;
	TestInstrumentData* currentIns = (TestInstrumentData*)FirstNode (&gTestInsList);

		/* get allocated ticks for instruments */
	{
		InstrumentResourceInfo info;
		TestInstrumentData* thisIns;

		allocatedTicks = 0;
		ScanList (&gTestInsList, thisIns, TestInstrumentData)
		{
			Result = GetInstrumentResourceInfo (&info, sizeof info, thisIns->ti_Instrument, AF_RESOURCE_TYPE_TICKS);
			CHECKRESULT (Result, "get resource info");
			allocatedTicks += info.rinfo_PerInstrument + info.rinfo_MaxOverhead;
			DBUGTICKS(("allocatedTicks = %d\n", allocatedTicks));
		}
	}

	bmp = CreateBenchmarkPatch( calcRateDivide, HIGH_PRIORITY, LOW_PRIORITY );
	if( bmp == NULL )
	{
		ERR(("Could not make BenchmarkPatch!\n"));
		goto cleanup;
	}

	SleepCue = CreateCue( NULL );
	Result = SleepUntilTime( SleepCue, GetAudioTime() + 240 );  /* Get background measurement. */
	CHECKRESULT(Result,"SleepUntilTime");
	DeleteCue( SleepCue );

	{
		int32 currentTicks;

		Result = ReadBenchmarkPatch( bmp, &currentTicks, &overheadTicks );
		CHECKRESULT(Result,"ReadBenchmarkPatch");
		DBUGTICKS(("CurrentTicks = %d, OverheadTicks = %d\n", currentTicks, overheadTicks));
	}

/* Start the instruments we want to test */
	{
		TestInstrumentData* thisIns;

		ScanList (&gTestInsList, thisIns, TestInstrumentData)
		{
			Result = StartInstrument( thisIns->ti_Instrument, NULL );
			CHECKRESULT(Result,"StartInstrument");
		}
	}

	while (doit)
	{
		Result = GetControlPad (1, FALSE, &cped);
		if (Result < 0)
		{
			PrintError(0,"get control pad data in","RunFaders",Result);
		}
		joy = cped.cped_ButtonBits;

		if (TestLeadingEdge(joy,oldjoy,ControlX))
		{
			doit = FALSE;
		}

		if ((joy&ControlLeftShift) && TestLeadingEdge(joy,oldjoy,ControlUp))
		{
			if (FirstNode (&gTestInsList) == (Node*)currentIns)
			{
				currentIns = (TestInstrumentData*)LastNode (&gTestInsList);
			}
			else
			{
				currentIns = (TestInstrumentData*)PrevNode (currentIns);
			}
		}

		if ((joy&ControlLeftShift) && TestLeadingEdge(joy,oldjoy,ControlDown))
		{
			if (LastNode (&gTestInsList) == (Node*)currentIns)
			{
				currentIns = (TestInstrumentData*)FirstNode (&gTestInsList);
			}
			else
			{
				currentIns = (TestInstrumentData*)NextNode (currentIns);
			}
		}

		pgClearDisplay( gPGCon, 0.0, 0.0, 0.0 );
		DriveFaders ( &(currentIns->ti_FaderBlock), joy );

/* Toggle with ControlA */
		if (!(joy&ControlLeftShift) && TestLeadingEdge(joy,oldjoy,ControlA))
		{
			if(IfNoteOn)
			{
				TestInstrumentData* thisIns;

				ScanList (&gTestInsList, thisIns, TestInstrumentData)
				{
					Result = ReleaseInstrument( thisIns->ti_Instrument, NULL );
				}

				IfNoteOn = FALSE;
			}
			else
			{
				TestInstrumentData* thisIns;

				ScanList (&gTestInsList, thisIns, TestInstrumentData)
				{
					Result = StartInstrument( thisIns->ti_Instrument, NULL );
				}

				IfNoteOn = TRUE;
			}
			CHECKRESULT(Result, "Start/ReleaseInstrument");
		}

/* Reset benchmark on shift-A */
		if ((joy&ControlLeftShift) && TestLeadingEdge(joy,oldjoy,ControlA)) {
		    ResetBenchmarkPatch (bmp);
		}

/* Play with ControlB */
		if (TestLeadingEdge(joy,oldjoy,ControlB))
		{
			TestInstrumentData* thisIns;

			ScanList (&gTestInsList, thisIns, TestInstrumentData)
			{
				Result = StartInstrument( thisIns->ti_Instrument, NULL );
			}

			IfNoteOn = TRUE;
		}
		else if (TestTrailingEdge(joy,oldjoy,ControlB))
		{
			TestInstrumentData* thisIns;

			ScanList (&gTestInsList, thisIns, TestInstrumentData)
			{
				Result = ReleaseInstrument( thisIns->ti_Instrument, NULL );
			}

			IfNoteOn = FALSE;
		}
		CHECKRESULT(Result, "Start/ReleaseInstrument");

/* Set defaults on C */
		if (TestLeadingEdge(joy,oldjoy,ControlC))
		{
			SetFaderDefaults(currentIns);
		}

/* Toggle with ControlStart */
		if (TestLeadingEdge (joy,oldjoy,ControlStart))
		{
			if (enableScope) {
				Result = DoScope( gFaderScope, ((TestInstrumentData*)LastNode (&gTestInsList))->ti_Instrument, "Output", 0 );
				CHECKRESULT(Result, "DoScope");
			}
			else {
				PRT(("Last instrument has no Output. Nowhere to place oscilloscope probe.\n"));
			}
		}

		pgSetColor( gPGCon, 1.0, 1.0, 1.0 );
		pgMoveTo( gPGCon, LEFT_VISIBLE_EDGE, ROW_1 );
		pgDrawText( gPGCon, currentIns->ti_InstrumentName );

		{
			int32 currentTicks, maximumTicks;

			Result = ReadBenchmarkPatch( bmp, &currentTicks, &maximumTicks );
			CHECKRESULT(Result,"ReadBenchmarkPatch");
			currentTicks -= overheadTicks;
			maximumTicks -= overheadTicks;
			DBUGTICKS(("CurrentTicks = %d, MaximumTicks = %d\n", currentTicks, maximumTicks));

			DrawTicks (COL_1, ROW_1, "Alloc", allocatedTicks, FALSE);
			DrawTicks (COL_2, ROW_1, "Cur",   currentTicks, currentTicks > allocatedTicks);
			DrawTicks (COL_3, ROW_1, "Max",   maximumTicks, maximumTicks > allocatedTicks);
		}

		Result = pgSwitchScreens(gPGCon);
		CHECKRESULT(Result,"pgSwitchScreens");

		oldjoy = joy;
	}

cleanup:
	if( bmp ) DeleteBenchmarkPatch( bmp );
	return Result;
}


/********************************************************************/
Err InitFaders( FaderBlock* thisFaderBlock, Fader* theseFaders, int32 NumFaders, FaderEventFunctionP EventFunc )
/* This routine does all the main initializations. It should be
 * called once for eack block, before the program uses the faders.
 * Returns non-FALSE if all is well, FALSE if error
 */
{
	return InitFaderBlock ( thisFaderBlock, theseFaders, NumFaders, EventFunc );
}

/********************************************************************/
Err FaderEventHandler (int32 faderIndex, float32 faderValue, FaderBlock *fdbl)
{
	Fader * const fdr = &fdbl->fdbl_Faders[faderIndex];
	FaderExtra * const fdre = fdr->fdr_UserData;
	Err errcode;

	DBUG(("%g => %s\n", faderValue, fdr->fdr_Name));
	if ((errcode = SetKnobPart (fdre->fdre_KnobExtra->knbe_Knob, fdre->fdre_PartNum, faderValue)) < 0) {
		PrintError (NULL, "set knob", NULL, errcode);
	}

	return errcode;
}

/********************************************************************/
static void SetFaderDefaults( TestInstrumentData* thisIns )
{
	int32 i;

	for (i=0; i<thisIns->ti_FaderBlock.fdbl_NumFaders; i++) {
		FaderExtra const *fdre = &(thisIns->ti_FaderExtra[i]);

		FaderEventHandler (i, fdre->fdre_Fader->fdr_Value = fdre->fdre_Default, &(thisIns->ti_FaderBlock));
	}
}


/*
	Connect first N parts of srcPort to N parts of dstPort.

	If numSrcParts >= numDstParts, connect first numDstParts of srcPort to
	numDstParts of dstPort. for example,

		0 -> 0
		1 -> 1
		2
		3

	If numSrcParts < numDstParts, connect first numSrcParts of srcPort to
	dstPort 0..numSrcParts-1, and again to dstPort
	numSrcParts..numSrcParts*2-1, and so on. For example,

		0 -> 0
		1 -> 1
		0 -> 2
		1 -> 3

	Arguments
		srcIns, srcPort
			source instrument and port name

		srcNumParts
			Number of parts of srcPort

		dstIns, dstPort
			dest instrument and port name

		dstNumParts
			Number of parts of dstPort

	Results
		0 on success, Err code on failure.
*/
static Err ConnectBus (Item srcIns, const char *srcPort, int32 numSrcParts, Item dstIns, const char *dstPort, int32 numDstParts)
{
	Err errcode;
	int32 i;

	for (i=0; i<numDstParts; i++) {
		if ((errcode = ConnectInstrumentParts (srcIns, srcPort, i % numSrcParts, dstIns, dstPort, i)) < 0) return errcode;
	}
	return 0;
}

static void TestInstrument (uint8 calcRateDivide, uint32 testFlags)
{
	TestInstrumentData* thisIns;
	Item sourceIns = -1;        /* if non-negative, then we're using it */
	Item outputIns = -1;        /* if non-negative, then we're using it */
	Err Result;
	bool chainStereo = (testFlags & TEST_F_LINE_IN) != 0;       /* TRUE => previous in chain is stereo output, FALSE otherwise */
	bool enableScope = FALSE;

	pgClearDisplay( gPGCon, 0.0, 0.0, 0.0 );
	pgMoveTo( gPGCon, 0.2, 0.3 );
	pgDrawText( gPGCon, "Loading..." );

	Result = CreateScopeProbe( &gFaderScope, 10000, gPGCon );
	CHECKRESULT(Result, "CreateScopeProbe");


		/* Load instrument definitions/templates */
	ScanList (&gTestInsList, thisIns, TestInstrumentData)
	{
		if ((Result = thisIns->ti_Template = LoadScoreTemplate (thisIns->ti_InstrumentName)) < 0)
		{
			PrintError (NULL, "load template", thisIns->ti_InstrumentName, Result);
			goto cleanup;
		}

		DumpInstrumentResourceInfo (thisIns->ti_Template, thisIns->ti_InstrumentName);
		thisIns->ti_Instrument = CreateInstrumentVA (thisIns->ti_Template,
			AF_TAG_PRIORITY,        thisIns->ti_Node.n_Priority,
			AF_TAG_CALCRATE_DIVIDE, calcRateDivide,
			TAG_END);
		CHECKRESULT(thisIns->ti_Instrument,"CreateInstrument");
		PRT(("%s: instrument item 0x%x\n", thisIns->ti_InstrumentName, thisIns->ti_Instrument));

		/* load sample, if requested */
		if (thisIns->ti_SampleName)
		{
			thisIns->ti_Sample = LoadSample( thisIns->ti_SampleName );
			CHECKRESULT(thisIns->ti_Sample,"LoadSample");
			PRT(("Attach sample '%s' to 'InFIFO'\n", thisIns->ti_SampleName));
			Result = CreateAttachmentVA( thisIns->ti_Instrument, thisIns->ti_Sample, AF_TAG_NAME, "InFIFO", TAG_END );
			CHECKRESULT(Result,"CreateAttachment");
		}

		/* connect input(s) to outputs */
		{
			InstrumentPortInfo info;

			Result = GetInstrumentPortInfoByName (&info, sizeof info, thisIns->ti_Instrument, "Input");
			if (Result >= 0)
			{
				if (info.pinfo_Type == AF_PORT_TYPE_INPUT)    /* !!! also AF_PORT_TYPE_KNOB? */
				{
					char * prevInsName;
					Item prevIns;

					if (&(thisIns->ti_Node) == FirstNode (&gTestInsList))      /* first node in list */
					{
						prevInsName = (testFlags & TEST_F_LINE_IN) ? "line_in.dsp" : "noise.dsp";
						sourceIns = prevIns = LoadInstrument(prevInsName, 0, 100);
						CHECKRESULT(prevIns,"load input instrument");

					}
					else
					{
						prevInsName = ((TestInstrumentData*)PrevNode (&(thisIns->ti_Node)))->ti_InstrumentName;
						prevIns = ((TestInstrumentData*)PrevNode (&(thisIns->ti_Node)))->ti_Instrument;
					}

					PRT(("Connect %s (%s) -> Input (%d part(s))\n", prevInsName, chainStereo ? "stereo" : "mono", info.pinfo_NumParts));

					Result = ConnectBus (prevIns, "Output", chainStereo ? 2 : 1, thisIns->ti_Instrument, "Input", info.pinfo_NumParts);
					CHECKRESULT(Result,"connect Input");
				}
			}
			else if (Result != AF_ERR_NAME_NOT_FOUND)
			{
				PrintError (NULL, "get port info for", "Input", Result);
				goto cleanup;
			}

			Result = GetInstrumentPortInfoByName (&info, sizeof info, thisIns->ti_Instrument, "Output");
			if (Result >= 0)
			{
				if (info.pinfo_Type == AF_PORT_TYPE_OUTPUT)
				{
					chainStereo = (info.pinfo_NumParts == 1 ? FALSE : TRUE);
					/* TOUCH(chainStereo); */ /* compiler's confused, because it's used before set */
				}
			}
			else if (&(thisIns->ti_Node) != LastNode (&gTestInsList))   /* not the last instrument in the chain */
			{
				PrintError (NULL, "get port info for", "Output", Result);
				goto cleanup;
			}
		}

		/* get knobs and knob info */
		{
			const int32 numPorts = GetNumInstrumentPorts (thisIns->ti_Instrument);
			InstrumentPortInfo info;
			int32 i, numKnobs;

			numKnobs = 0;
			for (i=0; i<numPorts; i++)
			{
				Result = GetInstrumentPortInfoByIndex (&info, sizeof info, thisIns->ti_Instrument, i);
				CHECKRESULT(Result,"get port info");
				if (info.pinfo_Type == AF_PORT_TYPE_KNOB)
				{
					if (numKnobs >= MAX_KNOBS)
					{
						PRT(("Too many knobs. Using only the first %d.\n", numKnobs));
						break;
					}
					thisIns->ti_KnobExtra[numKnobs].knbe_PortInfo = info;
					Result = thisIns->ti_KnobExtra[numKnobs].knbe_Knob = CreateKnob (thisIns->ti_Instrument, thisIns->ti_KnobExtra[numKnobs].knbe_PortInfo.pinfo_Name, NULL);
					CHECKRESULT(Result,"CreateKnob");
					numKnobs++;
				}
			}
			thisIns->ti_NumKnobs = numKnobs;
		}

		/* get number of faders and init faders */
		{
			int32 numFaders = 0;

			{
				int32 i;

				for (i=0; i<thisIns->ti_NumKnobs; i++)
				{
					numFaders += thisIns->ti_KnobExtra[i].knbe_PortInfo.pinfo_NumParts;
				}
			}

			if (numFaders > MAX_FADERS)
			{
				PRT(("Too many knob parts (%d). Displaying only the first %d as faders.\n", numFaders, MAX_FADERS));
				numFaders = MAX_FADERS;
			}

			Result = InitFaders (&(thisIns->ti_FaderBlock), thisIns->ti_Faders, numFaders, FaderEventHandler);
			if (Result < 0)
			{
				PrintError (NULL, "init faders", NULL, Result);
				goto cleanup;
			}
		}

		/* bind knob parts to faders */
		{
			int32 knobIndex, faderIndex;

				/* walk knobs */
			for (knobIndex=0, faderIndex=0; knobIndex < thisIns->ti_NumKnobs && faderIndex < thisIns->ti_FaderBlock.fdbl_NumFaders; knobIndex++)
			{
				const KnobExtra * const knbe = &(thisIns->ti_KnobExtra[knobIndex]);
				int32 partNum;

				DBUGKNOB(("Knob '%s'\n", knbe->knbe_PortInfo.pinfo_Name));

					/* bind each knob part to a fader */
				for (partNum=0; partNum < knbe->knbe_PortInfo.pinfo_NumParts && faderIndex < thisIns->ti_FaderBlock.fdbl_NumFaders; partNum++, faderIndex++)
				{
					Fader * const fdr = &(thisIns->ti_Faders[faderIndex]);
					FaderExtra * const fdre = &(thisIns->ti_FaderExtra[faderIndex]);

					if ((testFlags & TEST_F_MUTE) && !strcasecmp(knbe->knbe_PortInfo.pinfo_Name, "Amplitude"))
					{
						SetKnobPart (knbe->knbe_Knob, partNum, 0.0);
					}

						/* init FaderExtra */
					fdre->fdre_Fader     = fdr;
					fdre->fdre_KnobExtra = knbe;
					fdre->fdre_PartNum   = partNum;

						/* set fader name */
					if (knbe->knbe_PortInfo.pinfo_NumParts > 1)
					{
						sprintf (fdre->fdre_Name, "%s[%u]", knbe->knbe_PortInfo.pinfo_Name, partNum);
					}
					else
					{
						sprintf (fdre->fdre_Name, "%s", knbe->knbe_PortInfo.pinfo_Name);
					}

						/* get default */
					Result = ReadKnobPart (knbe->knbe_Knob, partNum, &fdre->fdre_Default);
					CHECKRESULT(Result, "query knob");

						/* init Fader */
					fdr->fdr_Name     = fdre->fdre_Name;
					fdr->fdr_UserData = fdre;
					fdr->fdr_Value    = fdre->fdre_Default;

						/* get min/max based (includes effect of execution rate) */
					{
						TagArg tags[] = {
							{ AF_TAG_MIN_FP },
							{ AF_TAG_MAX_FP },
							TAG_END
						};

						Result = GetAudioItemInfo (knbe->knbe_Knob, tags);
						CHECKRESULT(Result, "query knob");
						fdr->fdr_VMin = ConvertTagData_FP (tags[0].ta_Arg);
						fdr->fdr_VMax = ConvertTagData_FP (tags[1].ta_Arg);
					}

						/* get increment size */
						/* !!! should really be 2 increment sizes:
							normal - perhaps current computation
							min - precision of signal type (w/ execution rate taken into account!)
						*/
					fdr->fdr_Increment = (fdr->fdr_VMax - fdr->fdr_VMin) / 100.0;

					DBUGKNOB(("  part %d: range=%g..%g default=%g\n", partNum, fdr->fdr_VMin, fdr->fdr_VMax, fdre->fdre_Default));
				}
			}
		}
	}

		/* If TEST_F_LINE_OUT is set and last instrument has an output, connect to line_out.dsp
		** Also set enableScope if last instrument has an output. */
	{
		InstrumentPortInfo info;
		Item lastInsItem;

		lastInsItem = ((TestInstrumentData*)LastNode (&gTestInsList))->ti_Instrument;

		Result = GetInstrumentPortInfoByName (&info, sizeof info, lastInsItem, "Output");
		if (Result >= 0)
		{
			if (info.pinfo_Type == AF_PORT_TYPE_OUTPUT)
			{
				enableScope = TRUE;

				if (testFlags & TEST_F_LINE_OUT)
				{
					PRT(("Connect Output (%d part(s)) -> line_out.dsp (stereo)\n", info.pinfo_NumParts));

					outputIns = LoadInstrument("line_out.dsp", 0, MIXER_PRIORITY);
					CHECKRESULT(outputIns,"LoadInstrument");

					Result = ConnectBus (lastInsItem, "Output", info.pinfo_NumParts, outputIns, "Input", 2);
					CHECKRESULT(Result,"connect Output");
				}
			}
		}
		else if (Result != AF_ERR_NAME_NOT_FOUND)
		{
			PrintError (NULL, "get port info for", "Output", Result);
			goto cleanup;
		}
	}

		/* Start input and output instruments, if used */
	if (sourceIns >= 0) StartInstrument (sourceIns, NULL);
	if (outputIns >= 0) StartInstrument (outputIns, NULL);

	RunFaders (calcRateDivide, enableScope);

	ScanList (&gTestInsList, thisIns, TestInstrumentData)
	{
		StopInstrument (thisIns->ti_Instrument, NULL);
	}

	if (sourceIns >= 0) StopInstrument (sourceIns, NULL);
	if (outputIns >= 0) StopInstrument (outputIns, NULL);

cleanup:

	ScanList(&gTestInsList, thisIns, TestInstrumentData)
	{
		UnloadScoreTemplate (thisIns->ti_Template); /* also delete test instrument and knobs */
		UnloadSample (thisIns->ti_Sample);
	}
	DeleteScopeProbe (gFaderScope);
	UnloadInstrument (sourceIns);
	UnloadInstrument (outputIns);
	TOUCH(Result);      /* silence warnings for not using Result */
}


int main (int argc, char *argv[])
{
	TestInstrumentData* thisIns;
	uint32 testFlags = TEST_F_LINE_OUT;
	uint8 calcRateDivide = 1;
	Err errcode;

	PRT(("%s V%d\n", argv[0], PORTFOLIO_OS_VERSION));

  #ifdef MEMDEBUG
	if ((errcode = CreateMemDebug (NULL)) < 0) {
		PrintError (NULL, NULL, NULL, errcode);
		goto clean;
	}

	if ((errcode = ControlMemDebug (MEMDEBUGF_ALLOC_PATTERNS |
	/**/                            MEMDEBUGF_FREE_PATTERNS |
	/**/                            MEMDEBUGF_PAD_COOKIES |
	/**/                            MEMDEBUGF_CHECK_ALLOC_FAILURES |
	/**/                            MEMDEBUGF_KEEP_TASK_DATA)) < 0) {
		PrintError (NULL, NULL, NULL, errcode);
		goto clean;
	}

	DumpMemDebug (NULL);

  #endif

	if ((errcode = OpenAudioFolio()) < 0) {
		PrintError (NULL, "open audio folio", NULL, errcode);
		goto clean;
	}
  #ifdef MEMDEBUG
	DumpMemDebug(NULL);
  #endif
	if ((errcode = EnableAudioInput (TRUE, NULL)) < 0) {
		PrintError (NULL, "enable audio input", NULL, errcode);
		goto clean;
	}
	if ((errcode = InitFaderGraphics(2)) < 0) {
		PrintError (NULL, "init graphics", NULL, errcode);
		goto clean;
	}
  #ifdef MEMDEBUG
	DumpMemDebug(NULL);
  #endif
	pgClearDisplay( gPGCon, 0.0, 0.0, 0.0 );        /* !!! maybe InitGraphics() ought to do this! */
	if ((errcode = InitEventUtility (1, 0, TRUE)) < 0) {
		PrintError (NULL, "init event utility", NULL, errcode);
		goto clean;
	}
  #ifdef MEMDEBUG
	DumpMemDebug(NULL);
  #endif

	PrepList (&gTestInsList);
	gNumInsts = 0;

		/* parse args */
	{
		int i;
		int32 sampleNextArg = FALSE;
		TestInstrumentData* thisIns = NULL;

		for (i=1; i<argc; i++) {
			const char * const arg = argv[i];

			if (arg[0] == '-') {
				if (!strcasecmp (arg, "-2")) calcRateDivide = 2;
				else if (!strcasecmp (arg, "-8")) calcRateDivide = 8;
				else if (!strcasecmp (arg, "-LineIn")) testFlags |= TEST_F_LINE_IN;
				else if (!strcasecmp (arg, "-Mute")) testFlags |= TEST_F_MUTE;
				else if (!strcasecmp (arg, "-NoLineOut")) testFlags &= ~TEST_F_LINE_OUT;
				else if (!strcasecmp (arg, "-Sample"))
				{
					if (gNumInsts == 0)
					{
						PRT(("No instrument for sample!\n"));
						goto clean;
					}

					if (sampleNextArg == TRUE)
					{
						PRT(("Expecting sample name!\n"));
						goto clean;
					}

					sampleNextArg = TRUE;
				}
				else {
					PRT(("%s: unknown switch '%s'\n", argv[0], arg));
				}
			}
			else switch (sampleNextArg) {
				case FALSE:
					if (gNumInsts > MAX_INSTS)
					{
						PRT(("Program only handles up to %i instruments.\n", MAX_INSTS));
						goto clean;
					}

					/* Allocate new node in the list */
					if ((thisIns = AllocMem (sizeof(TestInstrumentData), MEMTYPE_NORMAL)) == NULL)
					{
						PrintError (NULL, "allocate list node", arg, thisIns);
						goto clean;
					}

					/* Chain it into the tail of the list */
					AddTail (&gTestInsList, &(thisIns->ti_Node));

					/* Clear out all the other fields in the structure */
					thisIns->ti_Node.n_Priority = TEST_PRIORITY;
					thisIns->ti_SampleName = NULL;
					thisIns->ti_Template = -1;
					thisIns->ti_Sample = -1;
					thisIns->ti_Instrument = -1;
					thisIns->ti_NumKnobs = 0;

					thisIns->ti_InstrumentName = (char*)arg;
					gNumInsts++;

DBUG(("Instrument %i: %s\n", gNumInsts-1, arg));
					break;

				case TRUE:
					if (thisIns) thisIns->ti_SampleName = (char*)arg;
					else
					{
						PrintError(NULL,"add sample", arg, thisIns);
						goto clean;
					}

					sampleNextArg = FALSE;
DBUG(("Sample %i: %s\n", gNumInsts-1, arg));
					break;
			}
		}

		if (gNumInsts == 0) {
			PRT(("You forgot to specify an instrument!\n"));
			PRT((
				"Usage: %s [-2|-8] [-LineIn] [-Mute] [-NoLineOut]\n"
				"       <ins name> [-Sample <sample name>]\n"
				"       [ <ins name> [-Sample <sample name>] ... ]\n",
				argv[0]));
			goto clean;
		}
	}

	TestInstrument (calcRateDivide, testFlags);

clean:
	TOUCH(errcode);     /* silence warning for not using errcode */
	while (!IsListEmpty (&gTestInsList))
	{
		thisIns = (TestInstrumentData*)RemHead (&gTestInsList);
		if (thisIns) FreeMem (thisIns, sizeof(TestInstrumentData));
	}
	CloseAudioFolio();
  #ifdef MEMDEBUG
	DumpMemDebug(NULL);
  #endif
	KillEventUtility();
  #ifdef MEMDEBUG
	DumpMemDebug(NULL);
  #endif
	TermFaderGraphics();
  #ifdef MEMDEBUG
	DumpMemDebug(NULL);
	DeleteMemDebug();
  #endif
	PRT(("%s: done\n", argv[0]));

	return 0;
}
