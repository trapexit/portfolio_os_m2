
/******************************************************************************
**
**  @(#) ta_reg_connect.c 96/08/07 1.15
**
******************************************************************************/

/*
** ta_reg_connect.c - test to see whether connecting to a register based
**       port is done correctly.
**
**   Register based port connections are implemented by adding a MOVE
**   instruction to a block of MOVES maintained by the audiofolio.
**   it moves data from the source to the destination register.
**   This test connects an "add.dsp" to a "sampler_16_v1.dsp" which is
**   playing a looping flat sample.  We can set the Knob on the adder
**   which is connected to the Amplitude of the sampler.  We can then
**   test the output of the sampler with a Probe.
**
*/

#ifndef MEMDEBUG
#define MEMDEBUG
#endif

#include <audio/audio.h>
#include <kernel/mem.h>
#include <stdio.h>
#include <stdlib.h>             /* malloc() */

/* Handy printing and debugging macros. */
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

#define MIN(a,b)    ((a)<(b)?(a):(b))
#define MAX(a,b)    ((a)>(b)?(a):(b))
#define ABS(x)      ((x)<0?(-(x)):(x))

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

/* Macro to simplify error checking. */
#define CALL_REPORT(_exp,msg) \
	do \
	{ \
		int32 err; \
		if ((err = (Err) (_exp)) < 0) \
		{ \
			PrintError(0,"\\failure in",msg, err); \
		} \
		TOUCH(err); \
	} while(0)


/* Macro to simplify error checking. */
#define PASS_FAIL(_exp,msg) \
	do \
	{ \
		PRT(("============================================\n")); \
		PRT(("Please wait for test to complete.\n")); \
		if ((Result = (Err) (_exp)) < 0) \
		{ \
			PrintError(0,"\\ERROR in",msg,Result); \
		} else { \
			PRT(("SUCCESS in %s\n", msg )); \
		} \
	} while(0)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}

typedef struct RegConnectStuff
{
	Item     rcs_AddIns;
	Item     rcs_AddKnob;
	Item     rcs_ResultProbe;
	Item     rcs_SamplerIns;
	Item     rcs_Sample;
	Item     rcs_Attachment;
} RegConnectStuff;

RegConnectStuff *tCreateRegConnect( void );
Err tDeleteRegConnect( RegConnectStuff *rcs );
Err tCheckRegConnect( RegConnectStuff *rcs );
#define SAMPWIDTH   (2)
#define SAMPSIZE   (100)

/*************************************************************
** Wait some number of audio frames to allow DSPP to propagate
** values across several instruments.
************************************************************/
int32 WaitAudioFrames( int32 NumFrames )
{
	int32 Start, Current, Elapsed;

	if( NumFrames > 32767 )
	{
		ERR(("WaitAudioFrames: Frame count is 16 bit, give me a break!\n"));
		return -1;
	}
	Start = GetAudioFrameCount();
	do
	{
		Yield();
		Current = GetAudioFrameCount();
		Elapsed = (Current - Start) & 0xFFFF;
	} while ( Elapsed < NumFrames );
	PRT(("Frame count = %d\n", Current));
	return 0;
}

/************************************************************************/
/* Fill in a sample with a level value. */
static	Item tBuildLevelSample ( int32 Level )
{
	int16 *Data;
	Item Samp = 0;
	int32 Result;

/* Allocate memory for sample */
	if ((Data = (int16 *)AllocMem (SAMPSIZE, MEMTYPE_NORMAL)) == NULL) {
		Result = MakeErr (ER_USER, 0, ER_SEVERE, ER_E_USER, ER_C_STND, ER_NoMem);
		goto cleanup;
	}

/* Fill sample with the level value. */
    {
        int32 i;
        int32 *tdata = (int32 *) Data;

        for (i=0; i<(SAMPSIZE/(sizeof(int32))); i++)
        {
            *tdata++ = (Level<<16) | Level;
        }
    }

/* Create sample item from sample parameters */
#if 0
/* VA seems to fail! */
	Samp = CreateSampleVA ( AF_TAG_ADDRESS,   Data,
	                        AF_TAG_WIDTH,     SAMPWIDTH,
	                        AF_TAG_CHANNELS,  1,
	                        AF_TAG_NUMBYTES,   SAMPSIZE,
	                        TAG_END );
	CHECKRESULT(Samp,"CreateSample");
#else
	{
		TagArg Tags[] =
		{	{ AF_TAG_ADDRESS,   (TagData) 0 },
			{ AF_TAG_WIDTH,     (TagData) SAMPWIDTH},
			{ AF_TAG_CHANNELS,  (TagData) 1},
			{ AF_TAG_NUMBYTES,  (TagData) SAMPSIZE},
			{ AF_TAG_SUSTAINBEGIN,  (TagData) 0},
			{ AF_TAG_SUSTAINEND,  (TagData) (SAMPSIZE/SAMPWIDTH)},
			{ TAG_END, NULL}
		};
		Tags[0].ta_Arg = (TagData) Data;
		Samp = CreateSample( Tags );
		CHECKRESULT(Samp,"CreateSample");
	}
#endif

/* Return sample to caller. */
	return Samp;

cleanup:
	DeleteSample (Samp);
	FreeMem (Data, SAMPSIZE);
	return Result;
}

/************************************************************/
static Err tDeleteLevelSample (Item sample)
{
    int16 *data = NULL;

        /* get address of sample data */
    {
        TagArg Tags[] = {
            { AF_TAG_ADDRESS },
            TAG_END
        };

        if (GetAudioItemInfo (sample, Tags) >= 0) {
            data = (int16 *)Tags[0].ta_Arg;
        }
    }

        /* get rid of sample item */
    DeleteSample (sample);

        /* free sample data */
    FreeMem (data, SAMPSIZE);

	return 0;
}

/************************************************************/
Err tDeleteRegConnect( RegConnectStuff *rcs )
{
	int32 Result = 0;

	if( rcs->rcs_SamplerIns )
	{
		CALL_REPORT( (tDeleteLevelSample( rcs->rcs_Sample )), "Delete Sample" );
		CALL_REPORT( (UnloadInstrument( rcs->rcs_AddIns )), "Unload Add" );
		CALL_REPORT( (UnloadInstrument( rcs->rcs_SamplerIns )), "Unload Sampler" );
	}
	rcs->rcs_SamplerIns = 0;
	return Result;
}

/************************************************************/
RegConnectStuff *tCreateRegConnect( void )
{
	RegConnectStuff *rcs;
	int32 Result;

	rcs = malloc( sizeof(RegConnectStuff) );
	if( rcs == NULL ) return NULL;
	CALL_CHECK( (rcs->rcs_AddIns = LoadInstrument("add.dsp",  0, 0)), "load add.dsp");
	CALL_CHECK( (StartInstrument( rcs->rcs_AddIns, NULL )), "start AddIns");

	CALL_CHECK( (rcs->rcs_SamplerIns = LoadInstrument("sampler_16_v1.dsp",  0, 0)), "load sampler.dsp");
	CALL_CHECK( (rcs->rcs_Sample = tBuildLevelSample( 0x4000)), "build sample");
	CALL_CHECK( (rcs->rcs_Attachment = CreateAttachment( rcs->rcs_SamplerIns, rcs->rcs_Sample, NULL)), "create attachment");
	CALL_CHECK( (StartInstrument( rcs->rcs_SamplerIns, NULL )), "start SamplerIns");

	CALL_CHECK( (rcs->rcs_AddKnob = CreateKnob( rcs->rcs_AddIns, "InputA", NULL )), "grab InputA on 0");
	CALL_CHECK( (rcs->rcs_ResultProbe = CreateProbe(rcs->rcs_SamplerIns, "Output", NULL)), "CreateProbe" );
	CALL_CHECK( (Result = ConnectInstruments( rcs->rcs_AddIns, "Output", rcs->rcs_SamplerIns, "Amplitude" )), "connect add to Amp");

	return rcs;
clean:
	tDeleteRegConnect( rcs );
	return NULL;
}

int32  MatchFloats( float32 f1, float32 f2, float32 tolerance )
{
	float32 margin;

	margin = ABS(f1*tolerance);
	return( (f1 > (f2-margin)) || (f1 < (f2+margin)) );
}

/************************************************************/
Err tCheckRegConnect( RegConnectStuff *rcs )
{
	float32 Value;
	int32 Result;

#define TRC_PROBE(_testval) \
	CALL_CHECK( (SetKnob( (rcs->rcs_AddKnob) , (_testval) )), "set Knob" ); \
	WaitAudioFrames(4); \
	CALL_CHECK( (ReadProbe( rcs->rcs_ResultProbe, &Value )), "check Probe" ); \
	if( !MatchFloats( Value, (_testval/2), 0.01 ) ) \
	{ \
		ERR(( "Probe does not match input. Got %g , expected %g\n", Value, (_testval/2) )); \
		Result = -1; \
		goto clean; \
	} else { \
		PRT(("Success: Got expected %g\n\n", Value )); \
	}

	TRC_PROBE( 0.5000 );
	TRC_PROBE( 0.2345 );
clean:
	return Result;
}

/*************************************************************
** Test to see whether a connection to a port through a register works.
************************************************************/
#define NUM_TESTERS   (4)
Err tRegConnect( void )
{
	RegConnectStuff *Testers[NUM_TESTERS];

	int32 Result;
/* test in odd combinations. */
	PRT(("\nTest: first connection.\n"));
	CALL_CHECK( (Testers[0] = tCreateRegConnect()), "create tester 0-A" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[0])), "check tester 0-A" );

	PRT(("\nTest: two connections.\n"));
	CALL_CHECK( (Testers[1] = tCreateRegConnect()), "create tester 1-B" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[1])), "check tester 1-B" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[0])), "check tester 0-B" );

	PRT(("\nTest: delete 0.\n"));
	CALL_CHECK( (Result = tDeleteRegConnect(Testers[0])), "delete tester 0" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[1])), "check tester 1-C" );

	PRT(("\nTest: recreate 0.\n"));
	CALL_CHECK( (Testers[0] = tCreateRegConnect()), "create tester 0" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[0])), "check tester 0-D" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[1])), "check tester 1-D" );

	PRT(("\nTest: test three connections.\n"));
	CALL_CHECK( (Testers[2] = tCreateRegConnect()), "create tester 2" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[0])), "check tester 0-E" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[1])), "check tester 1-E" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[2])), "check tester 2-E" );

	PRT(("\nTest: delete 2.\n"));
	CALL_CHECK( (Result = tDeleteRegConnect(Testers[2])), "delete tester 2" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[0])), "check tester 0-F" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[1])), "check tester 1-F" );

	PRT(("\nTest: recreate 2.\n"));
	CALL_CHECK( (Testers[2] = tCreateRegConnect()), "create tester 2" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[0])), "check tester 0-G" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[1])), "check tester 1-G" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[2])), "check tester 2-G" );

	PRT(("\nTest: delete 1.\n"));
	CALL_CHECK( (Result = tDeleteRegConnect(Testers[1])), "delete tester 1" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[0])), "check tester 0-H" );
	CALL_CHECK( (Result = tCheckRegConnect(Testers[2])), "check tester 2-H" );
clean:
	tDeleteRegConnect(Testers[0]);
	tDeleteRegConnect(Testers[1]);
	tDeleteRegConnect(Testers[2]);
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
	/**/                           MEMDEBUGF_CHECK_ALLOC_FAILURES |
	/**/                           MEMDEBUGF_KEEP_TASK_DATA)), "ControlMemDebug");
#endif
	PRT(("Begin %s\n", argv[0] ));
/* Initialize audio, return if error. */
	Result = OpenAudioFolio();
	if (Result < 0)
	{
		PrintError(0,"Audio Folio could not be opened.",0,Result);
		return(-1);
	}

	PASS_FAIL( (tRegConnect()), "Register Connect." );

clean:
	CloseAudioFolio();
#ifdef MEMDEBUG
	DumpMemDebug(NULL);
	DeleteMemDebug();
#endif
	PRT(( "%s finished.\n", argv[0] ));
	return((int) Result);
}
