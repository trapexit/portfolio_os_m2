
/******************************************************************************
**
**  @(#) ta_api.c 96/06/19 1.13
**
******************************************************************************/

/*
** Test Audio Folio API
*/

#include <audio/audio.h>
#include <kernel/mem.h>     /* memdebug */
#include <kernel/operror.h>
#include <stdio.h>
#include <stdlib.h>         /* malloc()*/

#define VERSION   "1.1"

/* Handy printing and debugging macros. */
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

#define MIN(a,b)    ((a)<(b)?(a):(b))
#define MAX(a,b)    ((a)>(b)?(a):(b))

/* Macro to simplify error checking. */
#define CALL_CHECK(_exp,msg) \
	DBUG(("%s\n",msg)); \
	do \
	{ \
		if ((Result = (Err) (_exp)) < 0) \
		{ \
			TOUCH(Result); \
			PrintError(0,"\\failure in",msg,Result); \
			goto clean; \
		} \
	} while(0)

/* Call and check to make sure we get the expected error code. */
#define CALL_CHECK_ERR(_exp,err,msg) \
	DBUG(("%s\n",msg)); \
	do \
	{ \
		Result = (Err) (_exp); \
		if( Result != err )  \
		{ \
			ERR(("Expected error 0x%x, got 0x%x, in %s\n", err, Result, msg )); \
			ERR(("Expected: ")); PrintfSysErr(err); \
			ERR(("Actual:   ")); PrintfSysErr(Result); \
			goto clean;\
		} \
		Result = 0; \
		(void)Result; /* silence compiler warning for Result set but not used */ \
	} while(0)

/* Macro to simplify error checking. */
#define CALL_REPORT(_exp,msg) \
	do \
	{ \
		if ((Result = (Err) (_exp)) < 0) \
		{ \
			TOUCH(Result); \
			PrintError(0,"\\failure in",msg,Result); \
		} \
	} while(0)


/* Macro to simplify error checking. */
#define PASS_FAIL(_exp,msg) \
	do \
	{ \
		PRT(("============================================\n")); \
		PRT(("Please wait for test to complete.\n")); \
		if ((Result = (Err) (_exp)) < 0) \
		{ \
			TOUCH(Result); \
			PrintError(0,"\\ERROR in",msg,Result); \
		} else { \
			PRT(("SUCCESS in %s\n", msg )); \
		} \
	} while(0)

/*************************************************************
** test ConnectInstrument()
************************************************************/
Err tConnectInstrument( void )
{
	Item AddIns0;
	Item AddIns1=0;
	Item KnobA0=0;
	Item KnobA1=0;
	Err Result;

/* Load a directout instrument to send the sound to the DAC. */
	CALL_CHECK((AddIns0 = LoadInstrument("add.dsp",  0, 0)), "load add.dsp");
	CALL_CHECK( (KnobA0 = CreateKnob( AddIns0, "InputA", NULL )), "grab InputA on 0");

	CALL_CHECK((AddIns1 = LoadInstrument("add.dsp",  0, 0)), "load add.dsp");
	CALL_CHECK( (KnobA1 = CreateKnob( AddIns1, "InputA", NULL )), "grab InputA on 1");

/* Connect two instruments. Read knob that is passed through connection. */
	CALL_CHECK( (ConnectInstruments( AddIns0, "Output", AddIns1, "InputA" )), "connect" );
/* Try to connect again, should fail. */
	CALL_CHECK_ERR( (ConnectInstruments( AddIns0, "Output", AddIns1, "InputA" )),
		AF_ERR_INUSE, "connect again" );

/* Now disconnect. */
	CALL_CHECK( (DisconnectInstrumentParts( AddIns1, "InputA", 0 )), "disconnect" );

/* Connect two instruments. Read knob that is passed through connection. */
	CALL_CHECK( (ConnectInstruments( AddIns0, "Output", AddIns1, "InputA" )), "connect" );

clean:

	CALL_REPORT( (DeleteKnob( KnobA0 )), "DeleteKnob" );
	CALL_REPORT( (DeleteKnob( KnobA1 )), "DeleteKnob" );
	CALL_REPORT( (UnloadInstrument( AddIns0 )), "UnloadInstrument" );
	CALL_REPORT( (UnloadInstrument( AddIns1 )), "UnloadInstrument" );

	return Result;
}

/************************************************************/
Err tLinkAttachments( void )
{
	Item   SamplerIns1;
	Item   SamplerIns2;
	Item   SamplerInsTmp = 0;
	Item   SampleItem;
	Item   AttTmp1, Att1_1, Att1_2, Att2_1;
	int32  Result;
	int16 *Data;

#define NUM_FRAMES   (200)
#define NUM_BYTES    (NUM_FRAMES*(sizeof(int16)))

	Data = (int16 *) malloc( NUM_BYTES );
	if( Data == NULL ) return AF_ERR_NOMEM;

	CALL_CHECK( (SampleItem = CreateSampleVA( AF_TAG_ADDRESS, (TagData) Data,
			AF_TAG_FRAMES, (TagData) NUM_FRAMES, TAG_END)), "CreateSample");

/* Load Sampler instrument */
	CALL_CHECK( (SamplerInsTmp = LoadInsTemplate("sampler_16_f1.dsp", NULL )), "LoadInsTemplate");
/* Attach the sample to the Template. */
	CALL_CHECK( (AttTmp1 = CreateAttachment(SamplerInsTmp, SampleItem, NULL)), "CreateAttachment");

/* Create Sampler instrument */
	CALL_CHECK( (SamplerIns1 = CreateInstrument(SamplerInsTmp, NULL )), "CreateInstrument");
/* Attach the sample to the Instrument. */
	CALL_CHECK( (Att1_1 = CreateAttachment(SamplerIns1, SampleItem, NULL)), "CreateAttachment");
	CALL_CHECK( (Att1_2 = CreateAttachment(SamplerIns1, SampleItem, NULL)), "CreateAttachment");


/* Create another Sampler instrument */
	CALL_CHECK( (SamplerIns2 = CreateInstrument(SamplerInsTmp, NULL )), "CreateInstrument");
/* Attach the sample to the Instrument. */
	CALL_CHECK( (Att2_1 = CreateAttachment(SamplerIns2, SampleItem, NULL)), "CreateAttachment");

/* Attempt to make various illegal links. */
/* Link with Template Attachment */
	CALL_CHECK_ERR( (LinkAttachments( AttTmp1, Att1_1 )),
		AF_ERR_MISMATCH, "link from template attachment" );
	CALL_CHECK_ERR( (LinkAttachments( Att1_1, AttTmp1 )),
		AF_ERR_MISMATCH, "link from template attachment" );

/* Link between different instruments */
	CALL_CHECK_ERR( (LinkAttachments( Att1_1, Att2_1 )),
		AF_ERR_MISMATCH, "link with two instruments" );

/* Try doing it the right way. */
	CALL_CHECK( (Result = LinkAttachments( Att1_1, Att1_2 )), "LinkAttachments");

/* Attempt to start a Template Attachment */
	CALL_CHECK_ERR( (StartAttachment( AttTmp1, NULL )),
		AF_ERR_BADITEM, "StartAttachment() with template item" );
/* Attempt to release a Template Attachment */
	CALL_CHECK_ERR( (ReleaseAttachment( AttTmp1, NULL )),
		AF_ERR_BADITEM, "ReleaseAttachment() with template item" );
/* Attempt to stop a Template Attachment */
	CALL_CHECK_ERR( (StopAttachment( AttTmp1, NULL )),
		AF_ERR_BADITEM, "StopAttachment() with template item" );
/* Attempt to Query a Template Attachment */
	CALL_CHECK_ERR( (WhereAttachment( AttTmp1 )),
		AF_ERR_BADITEM, "WhereAttachment() with template item" );

clean:
	UnloadInsTemplate( SamplerInsTmp );
	return Result;
}

/*************************************************************
** test Delay Lines
************************************************************/
Err tDelayLine( void )
{
	Item   DelayLine;
	Err    Result;

	CALL_CHECK_ERR( (DelayLine = CreateDelayLine(1,0,0)), AF_ERR_OUTOFRANGE, "CreateDelayLine(1,0,0)" );

clean:
	if( DelayLine > 0 ) DeleteDelayLine( DelayLine );
	return Result;
}

/*************************************************************
** test Knobs
************************************************************/
Err tKnobs( void )
{
	Item   SawIns;
	Item   FreqKnob;
	Err    Result;

	CALL_CHECK((SawIns = LoadInstrument("sawtooth.dsp",  0, 0)), "load sawtooth.dsp");
	CALL_CHECK((FreqKnob = CreateKnob(SawIns, "Frequency", NULL)), "create freq knob");
	CALL_CHECK_ERR((SetKnobPart(FreqKnob, -1, 220.0)), AF_ERR_OUTOFRANGE, "set knob[-1]");
	CALL_CHECK((SetKnobPart(FreqKnob, 0, 220.0)), "set knob[0]");
	CALL_CHECK_ERR((SetKnobPart(FreqKnob, 1, 220.0)), AF_ERR_OUTOFRANGE, "set knob[1], CR5497");
	CALL_CHECK_ERR((SetKnobPart(FreqKnob, 2, 220.0)), AF_ERR_OUTOFRANGE, "set knob[2]");

clean:
	if( SawIns )
	{
		CALL_CHECK((UnloadInstrument(SawIns)), "UnloadInstrument(SawIns)");
	}
	return Result;
}

/************************************************************/
int main(int argc, char *argv[])
{
	int32 Result;

	TOUCH(argc);

#ifdef MEMDEBUG
	CALL_CHECK( (CreateMemDebug ( NULL )), "CreateMemDebug");

	CALL_CHECK( (ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
	/**/                           MEMDEBUGF_FREE_PATTERNS |
	/**/                           MEMDEBUGF_PAD_COOKIES |
	/**/                           MEMDEBUGF_BREAKPOINT_ON_ERRORS |
	/**/                           MEMDEBUGF_CHECK_ALLOC_FAILURES |
	/**/                           MEMDEBUGF_KEEP_TASK_DATA)), "ControlMemDebug");
#endif
	PRT(("Begin %s %s\n", argv[0], VERSION ));
/* Initialize audio, return if error. */
	Result = OpenAudioFolio();
	if (Result < 0)
	{
		PrintError(0,"Audio Folio could not be opened.",0,Result);
		return(-1);
	}

#ifdef MEMDEBUG
	DumpMemDebug(NULL);
#endif

	PASS_FAIL( (tConnectInstrument()), "Test Connection Tracking." );

#ifdef MEMDEBUG
	DumpMemDebug(NULL);
#endif

	PASS_FAIL( (tLinkAttachments()), "Test LinkAttachments." );

#ifdef MEMDEBUG
	DumpMemDebug(NULL);
#endif
	PASS_FAIL( (tDelayLine()), "Test Delay Lines." );

#ifdef MEMDEBUG
	DumpMemDebug(NULL);
#endif

	PASS_FAIL( (tKnobs()), "Test Knobs." );

#ifdef MEMDEBUG
	DumpMemDebug(NULL);
#endif
#ifdef MEMDEBUG
clean:
#endif
	CloseAudioFolio();
#ifdef MEMDEBUG
	DumpMemDebug(NULL);
	DeleteMemDebug();
#endif
	PRT(( "%s finished.\n", argv[0] ));
	return((int) Result);
}
