
/******************************************************************************
**
**  @(#) ez3dsound.c 96/09/23 1.9
**
**  EZ 3DSound Toolbox - load and play using Sound3D library
**
******************************************************************************/

#include <audio/audio.h>
#include <audio/music.h>        /* Sound3D library */
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
#include <math.h>

#include "ez3dsound.h"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

#define EZ3DSE_VERSION  ("0.0")

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

#define EZ3DSE_VALID  (0xDADAB421)

#define RadiansToDegrees(a) (((a) * 180.0) / PI)
#define FramesToMeters(a) (((a) * 340.0) / 44100.0)
#define MetersToFrames(a) (((a) * 44100.0) / 340.0)

static struct EZ3DSoundEnvironment
{
	uint32     ezse_Valid;    /* Set to EZSE_VALID when valid. */
	Item       ezse_LineOut;
	Item       ezse_MixerTmp;
	Item       ezse_Mixer;
	Item       ezse_MixerGainKnob;
	MixerSpec  ezse_MixerSpec;
	uint16     ezse_MaxSounds;   /* Maximum sounds that can be loaded. */
	uint8      ezse_numEffectInputs;
	uint8      ezse_numEffectOutputs;
	Item       ezse_EffectTemplate;
	Item       ezse_Effect;
	Item       ezse_EffectLineOut;
	float32    ezse_Gain;        /* Scale all amplitudes by this amount. */
	EZ3DSound *ezse_Sounds;      /* Preallocated Sound structures. */
	List       ezse_FreeSounds;  /* Unassigned sounds. */
	List       ezse_Parents;
	float32    ezse_EffectsGain; /* for "global" effects mix */
} gEZ3DSE;

typedef struct CartesianCoords
{
	float32 xyz_X;
	float32 xyz_Y;
	float32 xyz_Z;
	float32 xyz_Time;
} CartesianCoords;

static Err  LoadEZ3DSoundParent( const char *name, EZ3DSoundParent **ezspPtr, const TagArg *tags );
static void UnloadEZ3DSoundParent( EZ3DSoundParent *ezsp );
static bool IsSampleFileName (const char *fileName);
static Err EZ3DSoundUpdateHost( EZ3DSound* ezso );

/**********************************************************************/

 /**
 |||	AUTODOC -class EZAudio -group EZ3DSound -name CreateEZ3DSoundEnvironment
 |||	Initializes environment for EZ3DSound toolbox.
 |||
 |||	  Synopsis
 |||
 |||	    Err CreateEZ3DSoundEnvironment( int32 maxSounds, float32 gain,
 |||	                                    float32 fxgain, const char *effectsPatch,
 |||	                                    const TagArg *tags )
 |||
 |||	  Description
 |||
 |||	    CreateEZ3DSoundEnvironment() initializes the environment needed
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
 |||	        passed to SetEZ3DSoundLoudness().
 |||
 |||	    fxgain
 |||	        Overall gain of effect (typically reverberation).
 |||
 |||	    effectsPatch
 |||	        Optional name of an effects processing patch.
 |||	        Pass NULL if you do not want effects processing.
 |||	        The patch should have 1 or 2 "Input" parts and 0,1 or 2
 |||	        "Output" parts. Zero is allowed if the patch has its
 |||	        own internal "line_out.dsp".
 |||	        The amount of a sound that is sent to the effects
 |||	        processor is controlled by information from the Sound3D
 |||	        software. Using a stereo reverberation patch for effects gives
 |||	        the most realistic 3D effect.
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
 |||	    There can only be one EZ3DSound environment for any given
 |||	    application. After calling CreateEZ3DSoundEnvironment(),
 |||	    you should call DeleteEZ3DSoundEnvironment() before
 |||	    calling CreateEZ3DSoundEnvironment() again.
 |||
 |||	  Associated Files
 |||
 |||	    examples/Audio/EZ3DSound/ez3dsound.c, examples/Audio/EZ3DSound/ez3dsound.h
 |||
 |||	  See Also
 |||
 |||	    DeleteEZ3DSoundEnvironment(), LoadEZ3DSound(), StartEZ3DSound(),
 |||	    SetEZ3DSoundLoudness()
 **/

Err  CreateEZ3DSoundEnvironment( int32 maxSounds, float32 gain, float32 fxgain,
	const char *effectsPatch, const TagArg *tags )
{
	int32      Result;
	int32      i;

PRT(("CreateEZ3DSoundEnvironment() = V%s\n", EZ3DSE_VERSION));

DBUG(("CreateEZ3DSoundEnvironment( %d, %g,", maxSounds, gain));
DBUG((" %s, 0x%x\n", effectsPatch, tags ));

	if (tags) return EZ3DSND_ERR_UNIMPLEMENTED;

	if( gEZ3DSE.ezse_MaxSounds > 0 )
	{
		DeleteEZ3DSoundEnvironment();
	}

	gEZ3DSE.ezse_MaxSounds = maxSounds;
	gEZ3DSE.ezse_Gain = gain;
	gEZ3DSE.ezse_EffectsGain = fxgain;

/* Allocate Sound structures. */
	gEZ3DSE.ezse_Sounds = calloc( maxSounds*sizeof(EZ3DSound), 1 );
	if(gEZ3DSE.ezse_Sounds == NULL) return EZ3DSND_ERR_NOMEM;

/* Init Lists. */
/* WARNING - do not call DeleteEZ3DSoundEnvironment() again before prepping these lists. */
	PrepList( &gEZ3DSE.ezse_FreeSounds );
	PrepList( &gEZ3DSE.ezse_Parents );
	gEZ3DSE.ezse_Valid = EZ3DSE_VALID; /* It is now safe to call DeleteEZ3DSoundEnvironment(). */

/* Preassign mixer channels. */
	for( i=0; i<maxSounds; i++ )
	{
		EZ3DSound   *ezso;
		ezso = &gEZ3DSE.ezse_Sounds[i];
		ezso->ezso_MixerChannel = i*2; /* stereo inputs for each EZ3DSound */
/* Prelink sounds into free list. */
		AddTail( &gEZ3DSE.ezse_FreeSounds, (Node *) ezso );
	}

/* Create line out so we can hear it. */
	CALL_CHECK( (gEZ3DSE.ezse_LineOut = LoadInstrument( "line_out.dsp", 0, 100)), "CreateEZ3DSoundEnvironment - load line_out.");

/* Load effect processor. */
	if( effectsPatch != NULL )
	{
		InstrumentPortInfo  PINFO;
		CALL_CHECK( (gEZ3DSE.ezse_EffectTemplate = LoadScoreTemplate(effectsPatch)),
			"load effect processor");
		CALL_CHECK( (gEZ3DSE.ezse_Effect = CreateInstrument(gEZ3DSE.ezse_EffectTemplate, NULL)), "create effect processor.");

/* Base number of effect sends on number of inputs in effects patch. */
		CALL_CHECK( (GetInstrumentPortInfoByName( &PINFO, sizeof(PINFO), gEZ3DSE.ezse_Effect, "Input")),
			"get num inputs");
		gEZ3DSE.ezse_numEffectInputs = PINFO.pinfo_NumParts;
		if( (gEZ3DSE.ezse_numEffectInputs < 0) || (gEZ3DSE.ezse_numEffectInputs > 2) )
		{
			Result = EZ3DSND_ERR_NUM_FX_INPUTS;
			goto clean;
		}

		CALL_CHECK( (GetInstrumentPortInfoByName( &PINFO, sizeof(PINFO), gEZ3DSE.ezse_Effect, "Output")),
			"get num outputs");
		gEZ3DSE.ezse_numEffectOutputs = PINFO.pinfo_NumParts;
		if( gEZ3DSE.ezse_numEffectOutputs > 2 )
		{
			Result = EZ3DSND_ERR_NUM_FX_OUTPUTS;
			goto clean;
		}
	}
	else
	{
		gEZ3DSE.ezse_numEffectInputs = 0;
		gEZ3DSE.ezse_numEffectOutputs = 0;
		gEZ3DSE.ezse_EffectTemplate = 0;
		gEZ3DSE.ezse_Effect = 0;
	}

/* Allocate Mixer with room for effects processor fxsend. */
DBUG(("CreateEZ3DSoundEnvironment: maxsounds = %i, effect inputs = %i\n", maxSounds, gEZ3DSE.ezse_numEffectInputs));
	gEZ3DSE.ezse_MixerSpec = MakeMixerSpec( maxSounds * 2, 2 + (gEZ3DSE.ezse_numEffectInputs), 0 );
	CALL_CHECK( (gEZ3DSE.ezse_MixerTmp = CreateMixerTemplate(gEZ3DSE.ezse_MixerSpec, NULL)), "CreateEZ3DSoundEnvironment - create mixer tmp.");
	CALL_CHECK( (gEZ3DSE.ezse_Mixer = CreateInstrument( gEZ3DSE.ezse_MixerTmp, NULL)), "CreateEZ3DSoundEnvironment - create mixer.");

	CALL_CHECK( (ConnectInstrumentParts( gEZ3DSE.ezse_Mixer, "Output", 0,
		gEZ3DSE.ezse_LineOut, "Input", AF_PART_LEFT)), "mixer to line_out.");
	CALL_CHECK( (ConnectInstrumentParts( gEZ3DSE.ezse_Mixer, "Output", 1,
		gEZ3DSE.ezse_LineOut, "Input", AF_PART_RIGHT)), "mixer to line_out.");

	CALL_CHECK( (gEZ3DSE.ezse_MixerGainKnob = CreateKnob( gEZ3DSE.ezse_Mixer, "Gain", NULL)), "CreateEZ3DSoundEnvironment - create gain knob.");
	CALL_CHECK( (StartInstrument( gEZ3DSE.ezse_Mixer, NULL)), "CreateEZ3DSoundEnvironment - start mixer.");
	CALL_CHECK( (StartInstrument( gEZ3DSE.ezse_LineOut, NULL)), "CreateEZ3DSoundEnvironment - start line_out.");

	if (gEZ3DSE.ezse_Effect > 0)
	{
/* Connect mixer to effects processor. */
		CALL_CHECK( (ConnectInstrumentParts( gEZ3DSE.ezse_Mixer, "Output", 2,
			gEZ3DSE.ezse_Effect, "Input", 0)), "mixer to effect.");
		if( gEZ3DSE.ezse_numEffectOutputs >= 2 )
		{
			CALL_CHECK( (ConnectInstrumentParts( gEZ3DSE.ezse_Mixer, "Output", 3,
			gEZ3DSE.ezse_Effect, "Input", 1)), "mixer to effect.");
		}

/* Hook up effects processor to line_out if present. */
		if( gEZ3DSE.ezse_numEffectOutputs )
		{
			int32 outPort;

/* Create line out so we can hear it. */
			CALL_CHECK( (gEZ3DSE.ezse_EffectLineOut = LoadInstrument( "line_out.dsp", 0, 100)), "CreateEZ3DSoundEnvironment - load line_out.");

/* Connect effect to line out and set master gain. */
			CALL_CHECK( (ConnectInstrumentParts( gEZ3DSE.ezse_Effect, "Output", 0,
				gEZ3DSE.ezse_EffectLineOut, "Input", AF_PART_LEFT)), "effect to line_out_0.");
			outPort = ( gEZ3DSE.ezse_numEffectOutputs == 2 ) ? 1 : 0 ;
			CALL_CHECK( (ConnectInstrumentParts( gEZ3DSE.ezse_Effect, "Output", outPort,
				gEZ3DSE.ezse_EffectLineOut, "Input", AF_PART_RIGHT)), "effect to line_out_1.");

			CALL_CHECK( (StartInstrument( gEZ3DSE.ezse_Effect, NULL)), "CreateEZ3DSoundEnvironment - start effect.");
			CALL_CHECK( (StartInstrument( gEZ3DSE.ezse_EffectLineOut, NULL)), "CreateEZ3DSoundEnvironment - start line_out.");
		}
	}
	
	return Result;

clean:
	DeleteEZ3DSoundEnvironment();
	return Result;
}

/**********************************************************************/
 /**
 |||	AUTODOC -class EZAudio -group EZ3DSound -name DeleteEZ3DSoundEnvironment
 |||	Cleans up environment for EZ3DSound toolbox.
 |||
 |||	  Synopsis
 |||
 |||	    void DeleteEZ3DSoundEnvironment( void )
 |||
 |||	  Description
 |||
 |||	    DeleteEZ3DSoundEnvironment() cleans up the environment created
 |||	    CreateEZ3DSoundEnvironment().
 |||
 |||	  Implementation
 |||
 |||	    Call implemented in example toolbox V32.
 |||
 |||	  Associated Files
 |||
 |||	    examples/Audio/EZ3DSound/ez3dsound.c, examples/Audio/EZ3DSound/ez3dsound.h
 |||
 |||	  See Also
 |||
 |||	    CreateEZ3DSoundEnvironment()(), LoadEZ3DSound(), StartEZ3DSound()
 **/

void DeleteEZ3DSoundEnvironment( void )
{
	EZ3DSoundParent *ezsp;

	if( gEZ3DSE.ezse_Valid != EZ3DSE_VALID ) return;

/* Delete items.*/
	DeleteItem( gEZ3DSE.ezse_Effect );
	UnloadInstrument( gEZ3DSE.ezse_EffectLineOut );
	UnloadInstrument( gEZ3DSE.ezse_LineOut );
	DeleteItem( gEZ3DSE.ezse_MixerTmp );

/* Unload list of parents.*/
	ezsp = (EZ3DSoundParent *) FirstNode(&gEZ3DSE.ezse_Parents);
	while( IsNode(&gEZ3DSE.ezse_Parents, (Node *)ezsp))
	{
		UnloadEZ3DSoundParent( ezsp ); /* removes ezsp from list */
		ezsp = (EZ3DSoundParent *) FirstNode(&gEZ3DSE.ezse_Parents);
	}

/* Free Sound structures. */
	free( gEZ3DSE.ezse_Sounds );  /* free() should be immune to NULL */
	gEZ3DSE.ezse_Sounds = NULL;

	memset( &gEZ3DSE, 0, sizeof(gEZ3DSE) );
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
static void UnloadEZ3DSoundParent( EZ3DSoundParent *ezsp )
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
static Err LoadEZ3DSoundParent( const char *name, EZ3DSoundParent **ezspPtr, const TagArg *tags )
{
	int32    Result;
	EZ3DSoundParent *ezsp;

	*ezspPtr = NULL;

	if (tags) return EZ3DSND_ERR_UNIMPLEMENTED;

/* Allocate Parent */
	ezsp = (EZ3DSoundParent *) AllocNamedNode( sizeof(EZ3DSoundParent), name );
	if(ezsp == NULL) return EZ3DSND_ERR_NOMEM;

	PrepList( &ezsp->ezsp_Sounds );

/* Link Parent to Environment before failing. */
	AddTail( &gEZ3DSE.ezse_Parents, (Node *) ezsp );

/* Determine whether it is a sample or template/patch name */
	if (IsSampleFileName (name))
	{
		Item Attachment;
		char InstrumentName[AF_MAX_NAME_SIZE];


/* Load sample and attach it to Sampler Template */
		CALL_CHECK( (ezsp->ezsp_Sample = LoadSample(name)), "LoadEZ3DSound - load sample." );

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
			ERR(("LoadEZ3DSound failed for instrument %s\n", name));
			Result = ezsp->ezsp_InsTemplate ;
			goto clean;
		}
	}

	*ezspPtr = ezsp;
	return 0;

clean:
	UnloadEZ3DSoundParent( ezsp );
	return Result;
}

/**********************************************************************/
 /**
 |||	AUTODOC -class EZAudio -group EZ3DSound -name LoadEZ3DSound
 |||	Loads a sound effect by name.
 |||
 |||	  Synopsis
 |||
 |||	    Err LoadEZ3DSound( EZ3DSound **ezsoPtr, const char *name,
 |||	                       const TagArg *tags )
 |||
 |||	  Description
 |||
 |||	    LoadEZ3DSound() loads a sound by name for use with the
 |||	    EZ3DSound toolbox.
 |||
 |||	  Arguments
 |||
 |||	    ezsoPtr
 |||	        Pointer to a pointer to an EZ3DSound structure
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
 |||	        Same as Create3DSound(). If you aren't concerned too much
 |||	        with resources, omit the tag list; the defaults use a
 |||	        resource-intensive best-sound approach. If your sound is
 |||	        directional (that is, it's louder in one direction than the
 |||	        others) you'll need to use the S3D_TAG_BACK_AMPLITUDE tag
 |||	        described in the documentation for Create3DSound().
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
 |||	    examples/Audio/EZ3DSound/ez3dsound.c, examples/Audio/EZ3DSound/ez3dsound.h
 |||
 |||	  See Also
 |||
 |||	    CreateEZ3DSoundEnvironment(), UnloadEZ3DSound(), StartEZ3DSound()
 **/

Err LoadEZ3DSound( EZ3DSound **ezsoPtr, const char *name, const TagArg *tags )
{
	int32    Result;
	EZ3DSound *ezso = NULL;
	EZ3DSoundParent  *ezsp;
	uint32   t3dsCues;
	TagData  t3dsDirectional;
	Item     s3dInst;
	float32  NominalFreq;
	char*    dopplerKnobName;
	InstrumentPortInfo  PINFO;

	*ezsoPtr = NULL;

	if( IsListEmpty( &gEZ3DSE.ezse_FreeSounds ) ) return EZ3DSND_ERR_NOSOUNDS;

/* Determine whether parent already loaded by scanning parent list. */
	ezsp = (EZ3DSoundParent  *) FindNamedNode(&gEZ3DSE.ezse_Parents, name);

/* If not already loaded, load parent. */
	if( ezsp == NULL )
	{
		CALL_CHECK( (LoadEZ3DSoundParent( name, &ezsp, NULL )), "LoadEZ3DSound - parent.");
	}

/* Get Sound from free list. Connect to parent list. */
	ezso = (EZ3DSound *) RemHead( &gEZ3DSE.ezse_FreeSounds ); /* We already checked to see if present. */
	ezso->ezso_Parent = ezsp;
	ezso->ezso_Sound3D = NULL;
	AddTail( &ezsp->ezsp_Sounds, (Node *) ezso );

/* Make instrument. */
	CALL_CHECK( (ezso->ezso_Instrument = CreateInstrument( ezsp->ezsp_InsTemplate, NULL)), "LoadEZ3DSound - create ins.");

/* Retrieve the 3DSound tagargs, if present. */
	t3dsCues = (uint32)GetTagArg( tags, S3D_TAG_FLAGS, (TagData)EZ3D_DEFAULT_FLAGS );
DBUG(("LoadEZ3DSound - Cues = 0x%x\n", t3dsCues));
	t3dsDirectional = GetTagArg( tags, S3D_TAG_BACK_AMPLITUDE, ConvertFP_TagData(1.0) );

/* Find a frequency knob, if there is one. */
	if (GetInstrumentPortInfoByName( &PINFO, sizeof(PINFO), ezso->ezso_Instrument, "SampleRate") >= 0)
	{
		dopplerKnobName = "SampleRate";
	}
	else if (GetInstrumentPortInfoByName( &PINFO, sizeof(PINFO), ezso->ezso_Instrument, "Frequency") >= 0)
	{
		dopplerKnobName = "Frequency";
	}
	else dopplerKnobName = NULL;

/* Connect the Frequency knob, for doppler */
	if (dopplerKnobName != NULL)
	{
DBUG(("Doppler Knob Name = %s\n", dopplerKnobName));
		CALL_CHECK( (ezso->ezso_FreqKnob = CreateKnob(ezso->ezso_Instrument, dopplerKnobName, NULL)),
			"LoadEZ3DSound - Create doppler knob.");

/* Read the default knob value, assume that's nominal */
		CALL_CHECK( (ReadKnob(ezso->ezso_FreqKnob, &NominalFreq)), "LoadEZ3DSound - read frequency knob." );
		ezso->ezso_NominalFrequency = NominalFreq;
	}
	else
	{
/* set the knob to null */
		ezso->ezso_FreqKnob = 0;

/* turn off dopplering in the 3DSound flags */
		t3dsCues &= ~S3D_F_DOPPLER;
	}

/* Make a Sound3D processor. */
	CALL_CHECK( (Create3DSoundVA( &(ezso->ezso_Sound3D), S3D_TAG_FLAGS, (TagData)t3dsCues,
		S3D_TAG_BACK_AMPLITUDE, t3dsDirectional, TAG_END )), "LoadEZ3DSound - Create3DSound." );

/* Connect the source to the 3D instrument context*/
	CALL_CHECK( (s3dInst = Get3DSoundInstrument( ezso->ezso_Sound3D )), "LoadEZ3DSound - Get3DSoundInstrument." );
	CALL_CHECK( (ConnectInstruments(ezso->ezso_Instrument, "Output", s3dInst,
		"Input")), "LoadEZ3DSound - Connect instrument->3D." );

/* Connect the 3D instrument to the mixer. */
	CALL_CHECK( (ConnectInstrumentParts( s3dInst, "Output", AF_PART_LEFT,
		gEZ3DSE.ezse_Mixer, "Input", ezso->ezso_MixerChannel)), "LoadEZ3DSound - connect to mixer.");
	CALL_CHECK( (ConnectInstrumentParts( s3dInst, "Output", AF_PART_RIGHT,
		gEZ3DSE.ezse_Mixer, "Input", ezso->ezso_MixerChannel + 1)), "LoadEZ3DSound - connect to mixer.");

/* Set default loudness. */
	CALL_CHECK( (SetEZ3DSoundLoudness( ezso, 1.0 )), "LoadEZ3DSound - set default loudness");

	*ezsoPtr = ezso;
DBUG(("LoadEZ3DSound at 0x%x\n", ezso ));
	return 0;

clean:
	if( ezso ) UnloadEZ3DSound( ezso );
	return Result;
}

 /**
 |||	AUTODOC -class EZAudio -group EZ3DSound -name UnloadEZ3DSound
 |||	Unloads sound loaded by LoadEZ3DSound().
 |||
 |||	  Synopsis
 |||
 |||	    void UnloadEZ3DSound( EZ3DSound *ezso )
 |||
 |||	  Description
 |||
 |||	    Unload sound loaded by LoadEZ3DSound().
 |||
 |||	  Arguments
 |||
 |||	    ezso
 |||	        Pointer to an EZ3DSound structure created by calling
 |||	        LoadEZ3DSound().
 |||
 |||	  Implementation
 |||
 |||	    Call implemented in example toolbox V32.
 |||
 |||	  Associated Files
 |||
 |||	    examples/Audio/EZ3DSound/ez3dsound.c, examples/Audio/EZ3DSound/ez3dsound.h
 |||
 |||	  See Also
 |||
 |||	    CreateEZ3DSoundEnvironment(), LoadEZ3DSound(), StartEZ3DSound()
 **/

/* Can accept partially complete sound. */
void UnloadEZ3DSound( EZ3DSound *ezso )
{
	if( ezso == NULL ) return;

/* Stop Sound */
	StopEZ3DSound( ezso, NULL );

DBUG(("UnloadEZ3DSound: EZ3Dsound stopped.\n"));
/* Remove from Parent and relink with free list. */
	if( ezso->ezso_Parent )
	{
		RemNode( (Node *) ezso );
		AddTail(  &gEZ3DSE.ezse_FreeSounds, (Node *) ezso );

/* If parent list empty, remove from env list, delete parent. */
		if( IsListEmpty( &ezso->ezso_Parent->ezsp_Sounds ) )
		{
			UnloadEZ3DSoundParent( ezso->ezso_Parent );
		}
		ezso->ezso_Parent = NULL;
	}
DBUG(("UnloadEZ3DSound: list cleaned up.\n"));

/* Delete instrument. */
	DeleteItem( ezso->ezso_Instrument );
DBUG(("UnloadEZ3DSound: source instrument deleted.\n"));

/* Delete 3D Sound. */
DBUG(("Deleting 3DSound.\n"));
	if (ezso->ezso_Sound3D) Delete3DSound( &(ezso->ezso_Sound3D) );
DBUG(("UnloadEZ3DSound: 3D sound deleted.\n"));
}

/****************************************************************/
/* PolarToXYZ is commented out to avoid compiler warnings, but  */
/* may be found useful at some point.                           */

#if 0
static void PolarToXYZ( PolarPosition4D* polar, CartesianCoords* cart )
{
	cart->xyz_X = polar->pp4d_Radius * sinf(polar->pp4d_Theta);
	cart->xyz_Y = polar->pp4d_Radius * cosf(polar->pp4d_Theta);
	cart->xyz_Z = 0;
	cart->xyz_Time = polar->pp4d_Time;

	return;
}
#endif

/****************************************************************/
static void XYZToPolar( CartesianCoords* cart, PolarPosition4D* polar )
{
DBUG(("XYZToPolar: x = %f, y = %f, z = %f\n", cart->xyz_X, cart->xyz_Y, cart->xyz_Z));
	polar->pp4d_Radius = sqrtf(cart->xyz_X * cart->xyz_X +
	  cart->xyz_Y * cart->xyz_Y);
DBUG(("XYZToPolar: radius = %f\n", polar->pp4d_Radius));
	polar->pp4d_Theta = atan2f(cart->xyz_X, cart->xyz_Y);
DBUG(("XYZToPolar: theta = %f\n", polar->pp4d_Theta));
	polar->pp4d_Phi = 0.0;
	polar->pp4d_Time = cart->xyz_Time;
DBUG(("XYZToPolar: done\n"));

	return;
}

/****************************************************************/
static void GraphicsToSound (float32 x, float32 y, float32 z, PolarPosition4D* polar)
{
	CartesianCoords cart_xyz;

	cart_xyz.xyz_X = MetersToFrames(x);
	cart_xyz.xyz_Y = MetersToFrames(-z);
	cart_xyz.xyz_Z = MetersToFrames(y);
	cart_xyz.xyz_Time = 0;

	XYZToPolar(&cart_xyz, polar);

}

/***********************************************************************/
static Err EZ3DSoundUpdateHost( EZ3DSound* ezso )
{
	Err Result;

	float32 Amplitude, Dry, Wet, Doppler;
	Sound3DParms s3dParms;
	int32 part;

/* Do all the host updates that the Sound3D API doesn't do */

	CALL_CHECK( (Get3DSoundParms( ezso->ezso_Sound3D, &s3dParms, sizeof(s3dParms) )),
		"EZ3DSoundUpdateHost - Get3DSoundParms." );

/* Doppler */
	if (ezso->ezso_FreqKnob)
	{
DBUG(("Doing doppler\n"));
		Doppler = s3dParms.s3dp_Doppler;
		CALL_CHECK( (SetKnob(ezso->ezso_FreqKnob, Doppler * ezso->ezso_NominalFrequency)),
			"EZ3DSoundUpdateHost - SetKnob Doppler." );
	}

/*
   Reverb calculations (from Dodge and Jerse):
   Dry signal attenuates proportionally to distance (1/D). Wet signal
   attenuates proportionally to the square root of distance (1/sqrt(D)).
   We don't differentiate between local and global reverberation here.
*/

	Amplitude = s3dParms.s3dp_DistanceFactor;
	Dry = Amplitude * ezso->ezso_Gain * gEZ3DSE.ezse_Gain;
	Wet = sqrtf(Amplitude) * ezso->ezso_Gain * gEZ3DSE.ezse_EffectsGain;

/* Set Mixer Gains for dry output. */
	part = CalcMixerGainPart(gEZ3DSE.ezse_MixerSpec, ezso->ezso_MixerChannel, AF_PART_LEFT );
	CALL_CHECK( (SetKnobPart( gEZ3DSE.ezse_MixerGainKnob, part, Dry)),
		"EZ3DSoundUpdateHost - dry left.");

	part = CalcMixerGainPart(gEZ3DSE.ezse_MixerSpec, ezso->ezso_MixerChannel + 1, AF_PART_RIGHT );
	CALL_CHECK( (SetKnobPart( gEZ3DSE.ezse_MixerGainKnob, part, Dry )),
		"EZ3DSoundUpdateHost - dry right.");

/* Set mixer gains for wet (fx) output. */
	if( gEZ3DSE.ezse_numEffectInputs == 1 )
	{
/* mono effect send */
		part = CalcMixerGainPart(gEZ3DSE.ezse_MixerSpec, ezso->ezso_MixerChannel, AF_PART_LEFT+2 );
		CALL_CHECK( (SetKnobPart( gEZ3DSE.ezse_MixerGainKnob, part, Wet)),
			"EZ3DSoundUpdateHost - send fx left 3D sound.");
		part = CalcMixerGainPart(gEZ3DSE.ezse_MixerSpec, ezso->ezso_MixerChannel + 1, AF_PART_LEFT+2 );
		CALL_CHECK( (SetKnobPart( gEZ3DSE.ezse_MixerGainKnob, part, Wet)),
			"EZ3DSoundUpdateHost - send fx right 3D sound.");
	}
	else if( gEZ3DSE.ezse_numEffectInputs == 2 )
	{
/* stereo effect send */
		part = CalcMixerGainPart(gEZ3DSE.ezse_MixerSpec, ezso->ezso_MixerChannel, AF_PART_LEFT+2 );
		CALL_CHECK( (SetKnobPart( gEZ3DSE.ezse_MixerGainKnob, part, Wet)),
			"EZ3DSoundUpdateHost - send left fx left 3D sound.");
		part = CalcMixerGainPart(gEZ3DSE.ezse_MixerSpec, ezso->ezso_MixerChannel + 1, AF_PART_RIGHT+2 );
		CALL_CHECK( (SetKnobPart( gEZ3DSE.ezse_MixerGainKnob, part, Wet)),
			"EZ3DSoundUpdateHost - send right fx right 3D sound.");
	}

clean:
	return (Result);
}

/**********************************************************************/
 /**
 |||	AUTODOC -class EZAudio -group EZ3DSound -name StartEZ3DSound
 |||	Starts playing an EZ3DSound.
 |||
 |||	  Synopsis
 |||
 |||	    Err StartEZ3DSound( EZ3DSound *ezso,
 |||	                        float32 x, float32 y, float32 z,
 |||	                        const TagArg *tags )
 |||
 |||	  Description
 |||
 |||	    Start playing an EZ3DSound at a given spatial location.
 |||
 |||	  Arguments
 |||
 |||	    ezso
 |||	        Pointer to an EZ3DSound structure created by calling
 |||	        LoadEZ3DSound().
 |||
 |||	    x,y,z
 |||	        distances from the listener in meters. The implied Cartesian coordinate system
 |||	        is the same as that used in the graphics libraries: "x" is positive to the right,
 |||	        "y" is positive upward and "z" is positive back toward the observer. You should
 |||	        be able to use the same matrix transforms used to derive camera-relative positions
 |||	        to generate x, y and z.
 |||
 |||	        Note that EZ3DSound maps these values assuming the speed of sound in air at sea
 |||	        level, roughly 340 meters per second.
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
 |||	    examples/Audio/EZ3DSound/ez3dsound.c, examples/Audio/EZ3DSound/ez3dsound.h
 |||
 |||	  See Also
 |||
 |||	    CreateEZ3DSoundEnvironment(), LoadEZ3DSound(), StopEZ3DSound()
 |||	    ReleaseEZ3DSound()
 **/

Err StartEZ3DSound( EZ3DSound *ezso, float32 x, float32 y, float32 z, const TagArg *tags )
{
	PolarPosition4D soundPos;
	int32 Result;

/* Map coordinates to Sound3D PolarPosition4D system. */
	GraphicsToSound(x, y, z, &soundPos);
	soundPos.pp4d_Time = GetAudioFrameCount();
	ezso->ezso_LastTarget = soundPos;

/* Start 3DSound processing */
	CALL_CHECK( (Start3DSound( ezso->ezso_Sound3D, &soundPos )), "StartEZ3DSound - start 3D processor." );
	CALL_CHECK( (EZ3DSoundUpdateHost( ezso )), "StartEZ3DSound - update host parameters." );

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

/**********************************************************************/
 /**
 |||	AUTODOC -class EZAudio -group EZ3DSound -name SetEZ3DSoundLoudness
 |||	Sets mixer gains for an EZ3DSound.
 |||
 |||	  Synopsis
 |||
 |||	    Err SetEZ3DSoundLoudness( EZ3DSound *ezso, float32 loudness )
 |||
 |||	  Description
 |||
 |||	    Set the overall loudness of an EZ3DSound, relative to other
 |||	    sounds.
 |||
 |||	  Arguments
 |||
 |||	    ezso
 |||	        Pointer to an EZ3DSound structure created by calling
 |||	        LoadEZ3DSound().
 |||
 |||	    loudness
 |||	        This value will be multiplied by the gain value
 |||	        passed to CreateEZ3DSoundEnvironment() and used to set
 |||	        the mixer gain. Range is 0.0 to 1.0.
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
 |||	    examples/Audio/EZ3DSound/ez3dsound.c, examples/Audio/EZ3DSound/ez3dsound.h
 |||
 |||	  See Also
 |||
 |||	    CreateEZ3DSoundEnvironment(), LoadEZ3DSound(), StopEZ3DSound()
 |||	    ReleaseEZ3DSound()
 **/

Err SetEZ3DSoundLoudness( EZ3DSound *ezso, float32 loudness )
{
	int32    Result=0;

/* FIXME - for now, just set the variable ready for EZ3DSoundUpdateHost() */
	ezso->ezso_Gain = loudness;

/* clean: */
	return Result;
}

 /**********************************************************************/
/**
 |||	AUTODOC -class EZAudio -group EZ3DSound -name MoveEZ3DSound
 |||	Changes the position of a sound over time.
 |||
 |||	  Synopsis
 |||
 |||	    Err MoveEZ3DSound( EZ3DSound *ezso, float32 x, float32 y, float32 z )
 |||
 |||	  Description
 |||
 |||	    Move a sound smoothly to a new position in 3D space.
 |||
 |||	  Arguments
 |||
 |||	    ezso
 |||	        Pointer to an EZ3DSound structure created by calling
 |||	        LoadEZ3DSound().
 |||
 |||	    x,y,z
 |||	        distances from the listener in meters, giving the sound's new position.
 |||	        The implied Cartesian coordinate system
 |||	        is the same as that used in the graphics libraries: "x" is positive to the right,
 |||	        "y" is positive upward and "z" is positive back toward the observer. You should
 |||	        be able to use the same matrix transforms used to derive camera-relative positions
 |||	        to generate x, y and z.
 |||
 |||	        Note that EZ3DSound maps these values assuming the speed of sound in air at sea
 |||	        level, roughly 340 meters per second.
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
 |||	    examples/Audio/EZ3DSound/ez3dsound.c, examples/Audio/EZ3DSound/ez3dsound.h
 |||
 |||	  Caveats
 |||
 |||	    Generally you will call this function from your main frame loop, so it gets called
 |||	    many times per second. Because the Sound3D library uses a 16-bit counter to track
 |||	    the audio frame rate, calling MoveEZ3DSound at intervals greater than about once
 |||	    every 1.5 seconds will result in bad velocity calculations, which in turn will
 |||	    affect things like doppler shifting and sudden changes in amplitude.
 |||
 |||	    Similarly, the sound moves in straight line segments between calls to MoveEZ3DSound.
 |||	    For this reason, too, you should call MoveEZ3DSound at more or less your frame rate
 |||	    to give more accurate trajectories.
 |||
 |||	  See Also
 |||
 |||	    CreateEZ3DSoundEnvironment(), LoadEZ3DSound(), StopEZ3DSound()
 |||	    ReleaseEZ3DSound()
 **/

Err MoveEZ3DSound( EZ3DSound *ezso, float32 x, float32 y, float32 z )
{
	PolarPosition4D relativeTarget;
	Err Result;

	GraphicsToSound(x, y, z, &relativeTarget);
	relativeTarget.pp4d_Time = GetAudioFrameCount();

	CALL_CHECK( (Move3DSound(ezso->ezso_Sound3D, &(ezso->ezso_LastTarget), &relativeTarget)),
		"MoveEZ3DSound - Move3DSound" );

	EZ3DSoundUpdateHost( ezso );
	ezso->ezso_LastTarget = relativeTarget;

clean:
	return(Result);
}

 /**
 |||	AUTODOC -class EZAudio -group EZ3DSound -name ReleaseEZ3DSound
 |||	Releases a sound started with StartEZ3DSound.
 |||
 |||	  Synopsis
 |||
 |||	    Err ReleaseEZ3DSound( EZ3DSound *ezso, const TagArg *tags )
 |||
 |||	  Description
 |||
 |||	    Release an EZ3DSound. This will allow the sound
 |||	    to decay at its natural rate. See ReleaseInstrument().
 |||
 |||	  Arguments
 |||
 |||	    ezso
 |||	        Pointer to an EZ3DSound structure created by calling
 |||	        LoadEZ3DSound().
 |||
 |||	    tags
 |||	        None currently supported.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Caveats
 |||
 |||	    Calling this macro does not stop the associated 3D processor, which
 |||	    continues to use DSP resources. To free up those resources, you
 |||	    should call ReleaseEZ3DSound(), wait until you know the source
 |||	    instrument is quiet, and call StopEZ3DSound() to stop the processing.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in example toolbox V32.
 |||
 |||	  Associated Files
 |||
 |||	    examples/Audio/EZ3DSound/ez3dsound.c, examples/Audio/EZ3DSound/ez3dsound.h
 |||
 |||	  See Also
 |||
 |||	    CreateEZ3DSoundEnvironment(), LoadEZ3DSound(), StopEZ3DSound()
 |||	    StartEZ3DSound()
 **/


 /**
 |||	AUTODOC -class EZAudio -group EZ3DSound -name StopEZ3DSound
 |||	Stops a sound started with StartEZ3DSound.
 |||
 |||	  Synopsis
 |||
 |||	    Err StopEZ3DSound( EZ3DSound *ezso, const TagArg *tags )
 |||
 |||	  Description
 |||
 |||	    Stop an EZ3DSound. Sound will stop immediately which
 |||	    may cause a "pop". See StopInstrument().
 |||
 |||	  Arguments
 |||
 |||	    ezso
 |||	        Pointer to an EZ3DSound structure created by calling
 |||	        LoadEZ3DSound().
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
 |||	    examples/Audio/EZ3DSound/ez3dsound.c, examples/Audio/EZ3DSound/ez3dsound.h
 |||
 |||	  See Also
 |||
 |||	    CreateEZ3DSoundEnvironment(), LoadEZ3DSound(), ReleaseEZ3DSound()
 |||	    StartEZ3DSound()
 **/

Err StopEZ3DSound( EZ3DSound *ezso, const TagArg *tags )
{
	Err Result;

	CALL_CHECK( (StopInstrument( ezso->ezso_Instrument, tags )), "StopEZ3DSound - stop source." );
	if (ezso->ezso_Sound3D)
	{
		CALL_CHECK( (Stop3DSound( ezso->ezso_Sound3D )), "StopEZ3DSound - stop 3D processor." );
	}

clean:
	return(Result);
}
