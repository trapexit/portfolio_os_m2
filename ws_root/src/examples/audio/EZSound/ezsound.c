
/******************************************************************************
**
**  @(#) ezsound.c 96/08/28 1.11
**
**  EZ Sound Toolbox - load and play
**
******************************************************************************/

#include <audio/audio.h>
#include <audio/musicerror.h>   /* ML_ERR_ */
#include <audio/parse_aiff.h>   /* LoadSample() */
#include <audio/score.h>
#include <kernel/nodes.h>       /* MKNODEID() */
#include <kernel/operror.h>
#include <kernel/list.h>
#include <kernel/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ezsound.h"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CALL_CHECK(_exp,msg) \
	DBUG(("%s\n",msg)); \
	do \
	{ \
		if ((Result = (Err) (_exp)) < 0) \
		{ \
			PrintError(0,"\\failure in",msg,Result); \
			goto clean; \
		} \
	} while(0)


static struct EZSoundEnvironment
{
	uint32    ezse_Valid;    /* Set to EZSE_VALID when valid. */
	Item      ezse_LineOut;
	Item      ezse_MixerTmp;
	Item      ezse_Mixer;
	Item      ezse_MixerGainKnob;
	MixerSpec ezse_MixerSpec;
	uint16    ezse_MaxSounds;   /* Maximum sounds that can be loaded. */
	uint8     ezse_numEffectInputs;
	uint8     ezse_numEffectOutputs;
	Item      ezse_EffectTemplate;
	Item      ezse_Effect;
	Item      ezse_EffectLineOut;
	float32   ezse_Gain;        /* Scale all amplitudes by this amount. */
	EZSound  *ezse_Sounds;      /* Preallocated Sound structures. */
	List      ezse_FreeSounds;  /* Unassigned sounds. */
	List      ezse_Parents;
} gEZSE;

#define EZSE_VERSION  ("0.5")
#define EZSE_VALID  (0xDADAB421)

static Err  LoadEZSoundParent( const char *name, EZSoundParent **ezspPtr, const TagArg *tags );
static void UnloadEZSoundParent( EZSoundParent *ezsp );
static bool IsSampleFileName (const char *fileName);

/**********************************************************************/

 /**
 |||	AUTODOC -class EZAudio -group EZSound -name CreateEZSoundEnvironment
 |||	Initializes environment for EZSound toolbox.
 |||
 |||	  Synopsis
 |||
 |||	    Err CreateEZSoundEnvironment( int32 maxSounds, float32 gain,
 |||	                                  const char *effectsPatch,
 |||	                                  const TagArg *tags )
 |||
 |||	  Description
 |||
 |||	    CreateEZSoundEnvironment() initializes the environment needed
 |||	    to play sounds. It creates a mixer with room for maxSounds.
 |||	    It also creates an optional effects processor patch
 |||	    and hooks it up to the mixer.
 |||
 |||	  Arguments
 |||
 |||	    maxSounds
 |||	        Maximum number of sounds that can be created.
 |||	        Determines size of mixer.
 |||
 |||	    gain
 |||	        Overall gain which is used to scale amplitudes
 |||	        passed to SetEZSoundMix().
 |||
 |||	    effectsPatch
 |||	        Optional name of an effects processing patch.
 |||	        Pass NULL if you do not want effects processing.
 |||	        The patch should have 1 or 2 "Input" parts and 0,1 or 2
 |||	        "Output" parts. Zero is allowed if the patch has its
 |||	        own internal "line_out.dsp".
 |||	        The amount of a sound that is sent to the effects
 |||	        processor is controlled by the fxsend parameter in
 |||	        SetEZSoundMix().
 |||
 |||	    tags
 |||	        None currently supported.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Call implemented in example toolbox V32.
 |||
 |||	  Caveats
 |||
 |||	    There can only be one EZSound environment for any given
 |||	    application. After calling CreateEZSoundEnvironment(),
 |||	    you should call DeleteEZSoundEnvironment() before
 |||	    calling CreateEZSoundEnvironment() again.
 |||
 |||	  Associated Files
 |||
 |||	    Examples/Audio/EZSound/ezsound.c, Examples/Audio/EZSound/ezsound.h
 |||
 |||	  See Also
 |||
 |||	    DeleteEZSoundEnvironment(), LoadEZSound(), StartEZSound(),
 |||	    SetEZSoundMix()
 **/

Err  CreateEZSoundEnvironment( int32 maxSounds, float32 gain,
	const char *effectsPatch, const TagArg *tags )
{
	int32      Result;
	int32      i;

PRT(("CreateEZSoundEnvironment() = V%s\n", EZSE_VERSION));

DBUG(("CreateEZSoundEnvironment( %d, %g,", maxSounds, gain));
DBUG((" %s, 0x%x\n", effectsPatch, tags ));

	if (tags) return EZSND_ERR_UNIMPLEMENTED;

	if( gEZSE.ezse_MaxSounds > 0 )
	{
		DeleteEZSoundEnvironment();
	}

	gEZSE.ezse_MaxSounds = maxSounds;
	gEZSE.ezse_Gain = gain;

/* Allocate Sound structures. */
	gEZSE.ezse_Sounds = calloc( maxSounds*sizeof(EZSound), 1 );
	if(gEZSE.ezse_Sounds == NULL) return EZSND_ERR_NOMEM;

/* Init Lists. */
/* WARNING - do not call DeleteEZSoundEnvironment() again before prepping these lists. */
	PrepList( &gEZSE.ezse_FreeSounds );
	PrepList( &gEZSE.ezse_Parents );
	gEZSE.ezse_Valid = EZSE_VALID; /* It is now safe to call DeleteEZSoundEnvironment(). */

/* Preassign mixer channels. */
	for( i=0; i<maxSounds; i++ )
	{
		EZSound   *ezso;
		ezso = &gEZSE.ezse_Sounds[i];
		ezso->ezso_MixerChannel = i;
/* Prelink sounds into free list. */
		AddTail( &gEZSE.ezse_FreeSounds, (Node *) ezso );
	}

/* Create line out so we can hear it. */
	CALL_CHECK( (gEZSE.ezse_LineOut = LoadInstrument( "line_out.dsp", 0, 100)), "CreateEZSoundEnvironment - load line_out.");

/* Load effect processor. */
	if( effectsPatch != NULL )
	{
		InstrumentPortInfo  PINFO;
		CALL_CHECK( (gEZSE.ezse_EffectTemplate = LoadScoreTemplate(effectsPatch)),
			"load effect processor");
		CALL_CHECK( (gEZSE.ezse_Effect = CreateInstrument(gEZSE.ezse_EffectTemplate, NULL)), "create effect processor.");

/* Base number of effect sends on number of inputs in effects patch. */
		CALL_CHECK( (GetInstrumentPortInfoByName( &PINFO, sizeof(PINFO), gEZSE.ezse_Effect, "Input")),
			"get num inputs");
		gEZSE.ezse_numEffectInputs = PINFO.pinfo_NumParts;
		if( (gEZSE.ezse_numEffectInputs < 0) || (gEZSE.ezse_numEffectInputs > 2) )
		{
			Result = EZSND_ERR_NUM_FX_INPUTS;
			goto clean;
		}

		CALL_CHECK( (GetInstrumentPortInfoByName( &PINFO, sizeof(PINFO), gEZSE.ezse_Effect, "Output")),
			"get num outputs");
		gEZSE.ezse_numEffectOutputs = PINFO.pinfo_NumParts;
		if( gEZSE.ezse_numEffectOutputs > 2 )
		{
			Result = EZSND_ERR_NUM_FX_OUTPUTS;
			goto clean;
		}
	}
	else
	{
		gEZSE.ezse_numEffectInputs = 0;
		gEZSE.ezse_numEffectOutputs = 0;
		gEZSE.ezse_EffectTemplate = 0;
		gEZSE.ezse_Effect = 0;
	}

/* Allocate Mixer with room for effects processor fxsend. */
	gEZSE.ezse_MixerSpec = MakeMixerSpec( maxSounds, 2+gEZSE.ezse_numEffectInputs, 0 );
	CALL_CHECK( (gEZSE.ezse_MixerTmp = CreateMixerTemplate(gEZSE.ezse_MixerSpec, NULL)), "CreateEZSoundEnvironment - create mixer tmp.");
	CALL_CHECK( (gEZSE.ezse_Mixer = CreateInstrument( gEZSE.ezse_MixerTmp, NULL)), "CreateEZSoundEnvironment - create mixer.");

	CALL_CHECK( (ConnectInstrumentParts( gEZSE.ezse_Mixer, "Output", 0,
		gEZSE.ezse_LineOut, "Input", AF_PART_LEFT)), "mixer to line_out.");
	CALL_CHECK( (ConnectInstrumentParts( gEZSE.ezse_Mixer, "Output", 1,
		gEZSE.ezse_LineOut, "Input", AF_PART_RIGHT)), "mixer to line_out.");

	CALL_CHECK( (gEZSE.ezse_MixerGainKnob = CreateKnob( gEZSE.ezse_Mixer, "Gain", NULL)), "CreateEZSoundEnvironment - create gain knob.");
	CALL_CHECK( (StartInstrument( gEZSE.ezse_Mixer, NULL)), "CreateEZSoundEnvironment - start mixer.");
	CALL_CHECK( (StartInstrument( gEZSE.ezse_LineOut, NULL)), "CreateEZSoundEnvironment - start line_out.");

	if (gEZSE.ezse_Effect > 0)
	{
/* Connect mixer to effects processor. */
		CALL_CHECK( (ConnectInstrumentParts( gEZSE.ezse_Mixer, "Output", 2,
			gEZSE.ezse_Effect, "Input", 0)), "mixer to effect.");
		if( gEZSE.ezse_numEffectOutputs >= 2 )
		{
			CALL_CHECK( (ConnectInstrumentParts( gEZSE.ezse_Mixer, "Output", 3,
			gEZSE.ezse_Effect, "Input", 1)), "mixer to effect.");
		}

/* Hook up effects processor to line_out if present. */
		if( gEZSE.ezse_numEffectOutputs )
		{
			int32 outPort;

/* Create line out so we can hear it. */
			CALL_CHECK( (gEZSE.ezse_EffectLineOut = LoadInstrument( "line_out.dsp", 0, 100)), "CreateEZSoundEnvironment - load line_out.");

/* Connect effect to mixer fx send and return. */
			CALL_CHECK( (ConnectInstrumentParts( gEZSE.ezse_Effect, "Output", 0,
				gEZSE.ezse_EffectLineOut, "Input", AF_PART_LEFT)), "effect to line_out_0.");
			outPort = ( gEZSE.ezse_numEffectOutputs == 2 ) ? 1 : 0 ;
			CALL_CHECK( (ConnectInstrumentParts( gEZSE.ezse_Effect, "Output", outPort,
				gEZSE.ezse_EffectLineOut, "Input", AF_PART_RIGHT)), "effect to line_out_1.");

			CALL_CHECK( (StartInstrument( gEZSE.ezse_Effect, NULL)), "CreateEZSoundEnvironment - start effect.");
			CALL_CHECK( (StartInstrument( gEZSE.ezse_EffectLineOut, NULL)), "CreateEZSoundEnvironment - start line_out.");
		}
	}
	
	return Result;

clean:
	DeleteEZSoundEnvironment();
	return Result;
}

/**********************************************************************/
 /**
 |||	AUTODOC -class EZAudio -group EZSound -name DeleteEZSoundEnvironment
 |||	Cleans up environment for EZSound toolbox.
 |||
 |||	  Synopsis
 |||
 |||	    void DeleteEZSoundEnvironment( void )
 |||
 |||	  Description
 |||
 |||	    DeleteEZSoundEnvironment() cleans up the environment created
 |||	    CreateEZSoundEnvironment().
 |||
 |||	  Implementation
 |||
 |||	    Call implemented in example toolbox V32.
 |||
 |||	  Associated Files
 |||
 |||	    Examples/Audio/EZSound/ezsound.c, Examples/Audio/EZSound/ezsound.h
 |||
 |||	  See Also
 |||
 |||	    CreateEZSoundEnvironment()(), LoadEZSound(), StartEZSound()
 **/

void DeleteEZSoundEnvironment( void )
{
	EZSoundParent *ezsp;

	if( gEZSE.ezse_Valid != EZSE_VALID ) return;

/* Delete items.*/
	DeleteItem( gEZSE.ezse_Effect );
	UnloadInstrument( gEZSE.ezse_EffectLineOut );
	UnloadInstrument( gEZSE.ezse_LineOut );
	DeleteItem( gEZSE.ezse_MixerTmp );

/* Unload list of parents.*/
	ezsp = (EZSoundParent *) FirstNode(&gEZSE.ezse_Parents);
	while( IsNode(&gEZSE.ezse_Parents, (Node *)ezsp))
	{
		UnloadEZSoundParent( ezsp ); /* removes ezsp from list */
		ezsp = (EZSoundParent *) FirstNode(&gEZSE.ezse_Parents);
	}

/* Free Sound structures. */
	free( gEZSE.ezse_Sounds );  /* free() should be immune to NULL */
	gEZSE.ezse_Sounds = NULL;

	memset( &gEZSE, 0, sizeof(gEZSE) );
}

/**********************************************************************/
static bool IsSampleFileName (const char *fileName)
{
	const char * const suffix = strrchr (fileName, '.');

	return suffix && (
		!strcasecmp (suffix, ".aiff") ||
		!strcasecmp (suffix, ".aifc") ||
		!strcasecmp (suffix, ".aif"));
}

/**********************************************************************
** We do not need to unload sounds because we are either called when
** last sound has already been unloaded, or when deleting environment.
*/
static void UnloadEZSoundParent( EZSoundParent *ezsp )
{
	if( ezsp == NULL ) return;
	RemNode( (Node *) ezsp );
	DeleteItem( ezsp->ezsp_InsTemplate );
	free( ezsp );
}

/**********************************************************************/
static Node *AllocNamedNode( int32 size, const char *name )
{
	Node *n;
	n = calloc(size + strlen(name)+1, 1);   /* Allocate both together. */
	if( n == NULL ) return NULL;
	n->n_Name = ((uint8 *)n) + size;  /* name comes right after end of node. */
	strcpy( n->n_Name, name );
	return n;
}

/**********************************************************************/
static Err LoadEZSoundParent( const char *name, EZSoundParent **ezspPtr, const TagArg *tags )
{
	int32    Result;
	EZSoundParent *ezsp;

	*ezspPtr = NULL;

	if (tags) return EZSND_ERR_UNIMPLEMENTED;

/* Allocate Parent */
	ezsp = (EZSoundParent *) AllocNamedNode( sizeof(EZSoundParent), name );
	if(ezsp == NULL) return EZSND_ERR_NOMEM;

	PrepList( &ezsp->ezsp_Sounds );

/* Link Parent to Environment before failing. */
	AddTail( &gEZSE.ezse_Parents, (Node *) ezsp );

/* Determine whether it is a sample or template/patch name */
	if (IsSampleFileName (name))
	{
		Item Attachment;
		char InstrumentName[AF_MAX_NAME_SIZE];


/* Load sample and attach it to Sampler Template */
		CALL_CHECK( (ezsp->ezsp_Sample = LoadSample(name)), "LoadEZSound - load sample." );

		Result = SampleItemToInsName( ezsp->ezsp_Sample , TRUE, InstrumentName, AF_MAX_NAME_SIZE );
		if (Result < 0)
		{
			ERR(("No instrument to play that sample.\n"));
			goto clean;
		}

DBUG(("Use instrument: %s for %s\n", InstrumentName, name));
		ezsp->ezsp_InsTemplate = LoadInsTemplate (InstrumentName, NULL);
		if (ezsp->ezsp_InsTemplate < 0)
		{
			ERR(("LoadPIMap failed for %s\n", InstrumentName));
			Result = ezsp->ezsp_InsTemplate;
			goto clean;
		}

		Attachment = CreateAttachmentVA (ezsp->ezsp_InsTemplate, ezsp->ezsp_Sample,
			AF_TAG_SET_FLAGS,         AF_ATTF_FATLADYSINGS,
			AF_TAG_AUTO_DELETE_SLAVE, TRUE,
			TAG_END);
		if( Attachment < 0 )
		{
			Result = Attachment;
			goto clean;
		}
	}
	else
	{
		ezsp->ezsp_InsTemplate = LoadScoreTemplate (name);
		if (ezsp->ezsp_InsTemplate < 0)
		{
			ERR(("LoadEZSound failed for instrument %s\n", name));
			Result = ezsp->ezsp_InsTemplate ;
			goto clean;
		}
	}

	*ezspPtr = ezsp;
	return 0;

clean:
	UnloadEZSoundParent( ezsp );
	return Result;
}

/**********************************************************************/
 /**
 |||	AUTODOC -class EZAudio -group EZSound -name LoadEZSound
 |||	Loads a sound effect by name.
 |||
 |||	  Synopsis
 |||
 |||	    Err LoadEZSound( EZSound **ezsoPtr, const char *name,
 |||	                     const TagArg *tags )
 |||
 |||	  Description
 |||
 |||	    LoadEZSound() loads a sound by name for use with the
 |||	    EZSound toolbox. When loaded, the mix is set by default
 |||	    to amplitude=1.0, pan=0.5, fxsend=1.0 by calling
 |||	    SetEZSoundMix().
 |||
 |||	  Arguments
 |||
 |||	    ezsoPtr
 |||	        Pointer to a pointer to an EZSound structure
 |||	        that will be created by this call.
 |||
 |||	    name
 |||	        Name of a sound to be loaded. May be a sample
 |||	        ending in ".aiff" or ".aifc", a ".dsp" instrument,
 |||	        or a ".patch" file. Samples will have the appropriate
 |||	        instrument loaded depending on the sample format.
 |||	        If you want to play a sample using a specific instrument
 |||	        combine them in a patch.
 |||
 |||	    tags
 |||	        None currently supported.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Call implemented in example toolbox V32.
 |||
 |||	  Associated Files
 |||
 |||	    Examples/Audio/EZSound/ezsound.c, Examples/Audio/EZSound/ezsound.h
 |||
 |||	  See Also
 |||
 |||	    CreateEZSoundEnvironment(), UnloadEZSound(), StartEZSound()
 **/

Err LoadEZSound( EZSound **ezsoPtr, const char *name, const TagArg *tags )
{
	int32    Result;
	EZSound *ezso = NULL;
	EZSoundParent  *ezsp;

	*ezsoPtr = NULL;

	if( IsListEmpty( &gEZSE.ezse_FreeSounds ) ) return EZSND_ERR_NOSOUNDS;

/* Determine whether parent already loaded by scanning parent list. */
	ezsp = (EZSoundParent  *) FindNamedNode(&gEZSE.ezse_Parents, name);

/* If not already loaded, load parent. */
	if( ezsp == NULL )
	{
		CALL_CHECK( (LoadEZSoundParent( name, &ezsp, tags )), "LoadEZSound - parent.");
	}

/* Get Sound from free list. Connect to parent list. */
	ezso = (EZSound *) RemHead( &gEZSE.ezse_FreeSounds ); /* We already checked to see if present. */
	ezso->ezso_Parent = ezsp;
	AddTail( &ezsp->ezsp_Sounds, (Node *) ezso );

/* Make instrument. */
	CALL_CHECK( (ezso->ezso_Instrument = CreateInstrument( ezsp->ezsp_InsTemplate, NULL)), "LoadEZSound - create ins.");

/* Connect to mixer. */
	CALL_CHECK( (ConnectInstrumentParts( ezso->ezso_Instrument, "Output", 0,
		gEZSE.ezse_Mixer, "Input", ezso->ezso_MixerChannel)), "LoadEZSound - connect to mixer.");

/* Set default amplitude and pan. */
	CALL_CHECK( (SetEZSoundMix( ezso, 1.0, 0.5, 1.0 )), "LoadEZSound - set default mix");

	*ezsoPtr = ezso;
DBUG(("LoadEZSound at 0x%x\n", ezso ));
	return 0;

clean:
	if( ezso ) UnloadEZSound( ezso );
	return Result;
}

 /**
 |||	AUTODOC -class EZAudio -group EZSound -name UnloadEZSound
 |||	Unloads sound loaded by LoadEZSound().
 |||
 |||	  Synopsis
 |||
 |||	    void UnloadEZSound( EZSound *ezso )
 |||
 |||	  Description
 |||
 |||	    Unload sound loaded by LoadEZSound().
 |||
 |||	  Arguments
 |||
 |||	    ezso
 |||	        Pointer to an EZSound structure created by calling
 |||	        LoadEZSound().
 |||
 |||	  Implementation
 |||
 |||	    Call implemented in example toolbox V32.
 |||
 |||	  Associated Files
 |||
 |||	    Examples/Audio/EZSound/ezsound.c, Examples/Audio/EZSound/ezsound.h
 |||
 |||	  See Also
 |||
 |||	    CreateEZSoundEnvironment(), LoadEZSound(), StartEZSound()
 **/

/* Can accept partially complete sound. */
void UnloadEZSound( EZSound *ezso )
{
	if( ezso == NULL ) return;

/* Stop Sound */
	StopEZSound( ezso, NULL );

/* Remove from Parent and relink with free list. */
	if( ezso->ezso_Parent )
	{
		RemNode( (Node *) ezso );
		AddTail(  &gEZSE.ezse_FreeSounds, (Node *) ezso );

/* If parent list empty, remove from env list, delete parent. */
		if( IsListEmpty( &ezso->ezso_Parent->ezsp_Sounds ) )
		{
			UnloadEZSoundParent( ezso->ezso_Parent );
		}
		ezso->ezso_Parent = NULL;
	}

/* Delete instrument. */
	DeleteItem( ezso->ezso_Instrument );
}

/**********************************************************************/
 /**
 |||	AUTODOC -class EZAudio -group EZSound -name StartEZSound
 |||	Starts playing an EZSound.
 |||
 |||	  Synopsis
 |||
 |||	    Err StartEZSound( EZSound *ezso, const TagArg *tags )
 |||
 |||	  Description
 |||
 |||	    Start playing an EZSound.
 |||
 |||	  Arguments
 |||
 |||	    ezso
 |||	        Pointer to an EZSound structure created by calling
 |||	        LoadEZSound().
 |||
 |||	    tags
 |||	        Same as StartInstrument().
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Call implemented in example toolbox V32.
 |||
 |||	  Associated Files
 |||
 |||	    Examples/Audio/EZSound/ezsound.c, Examples/Audio/EZSound/ezsound.h
 |||
 |||	  See Also
 |||
 |||	    CreateEZSoundEnvironment(), LoadEZSound(), StopEZSound()
 |||	    ReleaseEZSound()
 **/

Err StartEZSound( EZSound *ezso, const TagArg *tags )
{
	int32 Result;
/* Start Instrument */
	if( (ezso->ezso_Parent->ezsp_Sample == 0) || (tags != NULL) )
	{
		CALL_CHECK( (StartInstrument( ezso->ezso_Instrument, tags)), "StartEZSound - start ins.");
	}
	else
	{
/* Play at original sample rate if sound is just a sample. */
		CALL_CHECK( (StartInstrumentVA( ezso->ezso_Instrument,
			AF_TAG_DETUNE_FP, ConvertFP_TagData(1.0),
			TAG_END )), "StartEZSound - start ins.");
	}
clean:
	return Result;
}

 /**
 |||	AUTODOC -class EZAudio -group EZSound -name SetEZSoundMix
 |||	Sets mixer gains for an EZSound.
 |||
 |||	  Synopsis
 |||
 |||	    Err SetEZSoundMix( EZSound *ezso,
 |||	                       float32 amplitude, float32 pan,
 |||	                       float32 fxsend )
 |||
 |||	  Description
 |||
 |||	    Start playing an EZSound. The mixer gains will be
 |||	    set based on amplitude, pan and fxsend.
 |||
 |||	  Arguments
 |||
 |||	    ezso
 |||	        Pointer to an EZSound structure created by calling
 |||	        LoadEZSound().
 |||
 |||	    amplitude
 |||	        This amplitude will be multiplied by the gain value
 |||	        passed to CreateEZSoundEnvironment() and used to set
 |||	        the mixer gain. Range is 0.0 to 1.0.
 |||
 |||	    pan
 |||	        Controls the left/right panning. Range is 0.0 for
 |||	        full left to 1.0 for full right pan.
 |||
 |||	    fxsend
 |||	        Controls the amount of this sound sent to the
 |||	        effects processor. The fxsend parameter is multiplied
 |||	        by the amplitude and gain parameters. Range is 0.0 to 1.0.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Call implemented in example toolbox V32.
 |||
 |||	  Associated Files
 |||
 |||	    Examples/Audio/EZSound/ezsound.c, Examples/Audio/EZSound/ezsound.h
 |||
 |||	  See Also
 |||
 |||	    CreateEZSoundEnvironment(), LoadEZSound(), StopEZSound()
 |||	    ReleaseEZSound()
 **/

/**********************************************************************/
Err SetEZSoundMix( EZSound *ezso, float32 amplitude, float32 pan, float32 fxsend )
{
	int32    Result;
	int32    part;
	float32  leftGain, rightGain;
DBUG(("SetEZSoundMix( 0x%x, %g, %g, %g )\n", ezso, amplitude, pan, fxsend ));

/* Set Mixer Gains for panning and effect send. */
	part = CalcMixerGainPart(gEZSE.ezse_MixerSpec, ezso->ezso_MixerChannel, 0 );
	leftGain = pan * amplitude * gEZSE.ezse_Gain;
	CALL_CHECK( (SetKnobPart( gEZSE.ezse_MixerGainKnob, part, leftGain)),
		"SetEZSoundMix - pan left.");

	part = CalcMixerGainPart(gEZSE.ezse_MixerSpec, ezso->ezso_MixerChannel, 1 );
	rightGain = (1.0 - pan) * amplitude * gEZSE.ezse_Gain;
	CALL_CHECK( (SetKnobPart( gEZSE.ezse_MixerGainKnob, part, rightGain )),
		"SetEZSoundMix - pan right.");

	if( gEZSE.ezse_numEffectInputs == 1 )
	{
/* mono effect send */
		part = CalcMixerGainPart(gEZSE.ezse_MixerSpec, ezso->ezso_MixerChannel, 2 );
		CALL_CHECK( (SetKnobPart( gEZSE.ezse_MixerGainKnob, part, fxsend * amplitude * gEZSE.ezse_Gain)),
			"SetEZSoundMix - send fx.");
	}
	else if( gEZSE.ezse_numEffectInputs == 2 )
	{
/* pan stereo effect send */
		part = CalcMixerGainPart(gEZSE.ezse_MixerSpec, ezso->ezso_MixerChannel, 2 );
		CALL_CHECK( (SetKnobPart( gEZSE.ezse_MixerGainKnob, part, fxsend * leftGain)),
			"SetEZSoundMix - send fx left.");
		part = CalcMixerGainPart(gEZSE.ezse_MixerSpec, ezso->ezso_MixerChannel, 3 );
		CALL_CHECK( (SetKnobPart( gEZSE.ezse_MixerGainKnob, part, fxsend * rightGain)),
			"SetEZSoundMix - send fx right.");
	}

clean:
	return Result;
}

 /**
 |||	AUTODOC -class EZAudio -group EZSound -name ReleaseEZSound
 |||	Releases a sound started with StartEZSound.
 |||
 |||	  Synopsis
 |||
 |||	    Err ReleaseEZSound( EZSound *ezso, const TagArg *tags )
 |||
 |||	  Description
 |||
 |||	    Release an EZSound. This will allow the sound
 |||	    to decay at its natural rate. See ReleaseInstrument().
 |||
 |||	  Arguments
 |||
 |||	    ezso
 |||	        Pointer to an EZSound structure created by calling
 |||	        LoadEZSound().
 |||
 |||	    tags
 |||	        None currently supported.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in example toolbox V32.
 |||
 |||	  Associated Files
 |||
 |||	    Examples/Audio/EZSound/ezsound.c, Examples/Audio/EZSound/ezsound.h
 |||
 |||	  See Also
 |||
 |||	    CreateEZSoundEnvironment(), LoadEZSound(), StopEZSound()
 |||	    StartEZSound()
 **/


 /**
 |||	AUTODOC -class EZAudio -group EZSound -name StopEZSound
 |||	Stops a sound started with StartEZSound.
 |||
 |||	  Synopsis
 |||
 |||	    Err StopEZSound( EZSound *ezso, const TagArg *tags )
 |||
 |||	  Description
 |||
 |||	    Stop an EZSound. Sound will stop immediately which
 |||	    may cause a "pop". See StopInstrument().
 |||
 |||	  Arguments
 |||
 |||	    ezso
 |||	        Pointer to an EZSound structure created by calling
 |||	        LoadEZSound().
 |||
 |||	    tags
 |||	        None currently supported.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in example toolbox V32.
 |||
 |||	  Associated Files
 |||
 |||	    Examples/Audio/EZSound/ezsound.c, Examples/Audio/EZSound/ezsound.h
 |||
 |||	  See Also
 |||
 |||	    CreateEZSoundEnvironment(), LoadEZSound(), ReleaseEZSound()
 |||	    StartEZSound()
 **/
