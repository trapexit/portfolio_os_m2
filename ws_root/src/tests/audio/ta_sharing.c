
/******************************************************************************
**
**  @(#) ta_attach.c 95/08/02 1.9
**  $Id: ta_attach.c,v 1.21 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC -private -class tests -group Audio -name ta_sharing
|||	Debug various aspects of DSPP template and instrument sharing.
|||
|||	  Format
|||
|||	    ta_sharing
|||
|||	  Description
|||
|||	    !!!
|||
|||	  Location
|||
|||	    Tests/Audio
**/

#include <audio/audio.h>
#include <audio/parse_aiff.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <stdio.h>

/* sample parameters */
#define SAMPWIDTH sizeof(int16)
#define NUM_CHANNELS (1)   /* Stereo or Mono */
#define NUM_FRAMES ((128*1024) + 1)
#define SAMPSIZE (SAMPWIDTH*NUM_FRAMES*NUM_CHANNELS)

/* assorted macros */
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

#define NUM_SETS   (2)
#define SECOND_INS   0

Err TestSharingWithAttachments( void );

int main (void)
{
	int32 Result;

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

	Result = TestSharingWithAttachments();
	CHECKRESULT(Result,"TestSharingWithAttachment");

cleanup:
	CloseAudioFolio();
	PRT(("All done.\n"));
	return((int) Result);
}

/*******************************************************************/
Err TestSharingWithAttachments( void )
{
/* Declare local variables */
	Item MySamp;
	Item Atts[NUM_SETS];
	Item SamplerTmps[NUM_SETS];
	Item SamplerIns1[NUM_SETS];
#if SECOND_INS == 1
	Item SamplerIns2[NUM_SETS];
#endif
	int32 Result;
	int32 i;

/* Load simple sinewave. */
	MySamp = LoadSystemSample ("sinewave.aiff");
	CHECKRESULT(MySamp,"LoadSystemSample");

	for( i=0; i<NUM_SETS; i++ )
	{
/* Load variable rate mono sample player instrument template. */
		SamplerTmps[i] = LoadInsTemplate( "sampler_16_v1.dsp", NULL );
		CHECKRESULT(SamplerTmps[i],"LoadInsTemplate");

/* Attach the sample to the template. */
		Atts[i] = CreateAttachment(SamplerTmps[i], MySamp, NULL);
		CHECKRESULT(Atts[i],"CreateAttachment");
	}

	for( i=0; i<NUM_SETS; i++ )
	{
/* Now create instruments from template. */
		SamplerIns1[i] = CreateInstrument(SamplerTmps[i],  NULL );
		CHECKRESULT(SamplerIns1[i],"LoadInstrument");
		PRT(("Instrument item 1 = 0x%x\n", SamplerIns1[i] ));
#if SECOND_INS == 1
/* Now create instruments from template. */
		SamplerIns2[i] = CreateInstrument(SamplerTmps[i],  NULL );
		CHECKRESULT(SamplerIns2[i],"LoadInstrument");
		PRT(("Instrument item 2 = 0x%x\n", SamplerIns2[i] ));
#endif
	}

	for( i=0; i<NUM_SETS; i++ )
	{
/* Now delete in this order to trigger bug. */
		Result = DeleteInstrument( SamplerIns1[i] );
		CHECKRESULT( Result,"DeleteInstrument");
#if SECOND_INS == 1
		Result = DeleteInstrument( SamplerIns2[i] );
		CHECKRESULT( Result,"DeleteInstrument");
#endif
		UnloadInsTemplate( SamplerTmps[i] );
		CHECKRESULT( Result,"DeleteInstrument");
	}

cleanup:
	return((int) Result);
}
