/******************************************************************************
**
**  @(#) sound3d.c 96/07/02 1.30
**
**  3D Sound
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  940525 PLB  Improved Get3DSoundRadiusTime()
**  940927 WJB  Added package id.
**  960110 RNM  Updated for M2 2.0.
**              - convert to floating point from frac16
**              - use new drift sampler, do away with absolute frame counts
**              - change Folio calls for knobs, attachments, probes, etc
**		- new API
**  960212 RNM	Add autodocs
**              Add Move3DSoundTo()
**
**
**  Initials:
**
**  PLB: Phil Burk (phil)
**  WJB: Bill Barton (peabody)
**  RNM: Robert Marsanyi (rnm)
**
*****************************************************************/

#include <kernel/mem.h>
#include <audio/audio.h>
#include <audio/musicerror.h>
#include <audio/sound3d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "music_internal.h"     /* package id */

MUSICLIB_PACKAGE_ID(sound3d)

#include "sound3d_patch.h"

#ifdef BUILD_STRINGS
#define	PRT(x)	{ printf x; }
#else
#define PRT(x) /* { printf x; } */

#endif

#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)
#define	DBUG2(x) /* PRT(x) */

#ifndef ABS
#define ABS(x) (((x)<0) ? -(x) : (x) )
#endif

#define CLIPTO(x,min,max) \
	{ if ((x) < (min)) \
		{ (x) = (min); } \
		else { if ((x) > (max)) (x) = (max); } }


/* Macro to simplify error checking. */
#define CHECKVAL(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		TOUCH(Result); \
		goto cleanup; \
	}

#define FRAMERATE (44100.0)

/* Private definitions, not in API */

struct Sound3D
{
	uint32 s3d_Flags;                  /* control which cues to use */
	Item s3d_InsTemplate;
	Item s3d_InsItem;                  /* Sampler3D instrument. */
/* Knobs used to control filters. */
	Item s3d_AlphaKnob;
	Item s3d_BetaKnob;
	Item s3d_FeedKnob;
	Item s3d_DelayKnob;
	Item s3d_DelayRateKnob;
	Item s3d_AmpKnob;
	Item s3d_DriftProbe;                /* Probe of sample drifts. */
	Item s3d_EnvRateKnob;
	Item s3d_AmpProbe;		    /* Probe of amplitude envelope */
	PolarPosition4D s3d_LastStart;
	PolarPosition4D s3d_LastEnd;        /* from Move3DSound */
	float32 s3d_MinRadius;              /* at which loudness reaches maximum */
	Sound3DParms s3d_Parms;             /* can be queried by host */
	float32 s3d_BackAmplitude;          /* for directional sound */
};

/* Structure containing filter coefficients */
typedef struct
{
	float32 erp_Alpha;
	float32 erp_Beta;
	float32 erp_Feed;
} EarParams;

/* Two sets of filter coefficients, one for each ear */
typedef struct
{
	EarParams bep_LeftEar;
	EarParams bep_RightEar;
} BothEarParams;

/*
   Useful constants
   MIN_RADIUS is a distance just beyond the ears.  A sound positioned inside
   this radius will have a distance factor of 1.0.

   MAX_AMP_DIFFERENTIAL is the maximum difference in amplitude ear-to-ear when
   using amplitude panning.
*/

#define S3D_MIN_RADIUS (50.0)
#define S3D_MAX_AMP_DIFFERENTIAL (0.6)	/* from Begault, p 130 */

/* Model-dependent defines */
#define S3D_LARGE_DELAY_LINE_LENGTH (88200)	/* for absolute delays */
#define S3D_SMALL_DELAY_LINE_LENGTH (100)	/* for ITD only */
#define S3D_SMALL_DELAY_CENTER (S3D_SMALL_DELAY_LINE_LENGTH / 2)

static Err Load3DSound( Sound3D* Snd3D );
static Err Unload3DSound( Sound3D* Snd3D );
static Err s3dReadLeftRightDrift( const Sound3D* Snd3D, int32* LeftPtr, int32* RightPtr );
static Err s3dReadLeftRightAmp( const Sound3D* Snd3D, float32* LeftAmpPtr, float32* RightAmpPtr );
static void Calc3DSoundEar ( const PolarPosition4D* Pos4D, EarParams* ERP );
static void Calc3DSoundFilters( const PolarPosition4D* Pos4D, BothEarParams* BEP);
static Err Set3DSoundFilters ( const Sound3D* Snd3D, const BothEarParams* BEP );

static void Calc3DSoundDelays( const PolarPosition4D* Start4D, const PolarPosition4D* End4D,
  float32* LeftDelay, float32* RightDelay, float32* DelayRate);
static Err Set3DSoundDelays ( const Sound3D* Snd3D, float32 LeftDelay, float32 RightDelay,
  float32 DelayRate );

static void Calc3DSoundIID( const PolarPosition4D* Start4D, const PolarPosition4D* End4D,
  float32* LeftAmp, float32* RightAmp );
static void Calc3DSoundDirectionality( const Sound3D* Snd3D, const PolarPosition4D* End4D,
  float32* PointAmp );
static void Calc3DSoundEnvRate( const PolarPosition4D* Start4D, const PolarPosition4D* End4D,
  float32* EnvRate );

static Err Set3DSoundAmplitude( const Sound3D* Snd3D, float32 PointAmp, float32 LeftAmp,
  float32 RightAmp, float32 EnvRate );

static float32 Calc3DSoundDoppler( const PolarPosition4D* Start4D, const PolarPosition4D* End4D );
static float32 Calc3DSoundDF( const PolarPosition4D* Pos4D, uint32 s3dFlags );

static Err Get3DSoundRadiusAmpTime ( const Sound3D* Snd3D, float32* RadiusPtr,
  int32* PhaseDelayPtr, float32* AmplitudeDiffPtr, int32* TimePtr);

#define S3D_DEFAULT_FLAGS ( \
   S3D_F_PAN_DELAY | \
   S3D_F_PAN_FILTER | \
   S3D_F_DOPPLER | \
   S3D_F_DISTANCE_AMPLITUDE_SQUARE | \
   S3D_F_OUTPUT_HEADPHONES )

/****************************************************************/
/*
|||	AUTODOC -public -class libmusic -group Sound3D -name Create3DSound
|||	Allocate and initialize the 3D Sound data structures.
|||
|||	  Synopsis
|||
|||	    Err Create3DSound( Sound3D** s3dHandle, const TagArg* tagList )
|||
|||	    Err Create3DSoundVA( Sound3D** s3dHandle, uint32 tag1, ... )
|||
|||	  Description
|||
|||	    Create3DSound() allocates DSP resources required to implement the 3D
|||	    cues selected with the S3D_TAG_FLAGS tag, currently the only valid
|||	    tag in the tagList function argument.  A pointer to the resulting
|||	    structure is returned in the variable pointed to by the argument
|||	    s3dHandle.  The pointer is subsequently passed to other functions
|||	    in the 3DSound API.
|||
|||	    Use Delete3DSound() to free all the resources associated with the
|||	    sound.
|||
|||	  Arguments
|||
|||	    s3dHandle
|||	        pointer to location used to store pointer to 3DSound structure.
|||
|||	    tagList
|||	        a pointer to an array of tags.
|||
|||	  Tags
|||
|||	    S3D_TAG_FLAGS (uint32)
|||	        Specifies which of the 3D cues that 3DSound supports are to be
|||	        used for this particular sound.  The flags should be logically
|||	        OR-ed together to form the tag argument, from the list below.
|||
|||	        If this tag is not specified, a default set of flags will be
|||	        used, consisting of S3D_F_PAN_DELAY | S3D_F_PAN_FILTER |
|||	        S3D_F_DOPPLER | S3D_F_DISTANCE_AMPLITUDE_SQUARE |
|||	        S3D_F_OUTPUT_HEADPHONES.
|||
|||	    S3D_TAG_BACK_AMPLITUDE (float32)
|||	        Sounds may be specified as being omni-directional, where
|||	        the sound radiates equally in all directions, or directional,
|||	        where the sound "points" in a particular direction.  In the
|||	        latter case, the S3D_TAG_BACK_AMPLITUDE tag should be specified.
|||
|||	        The argument value represents a "backward amplitude" factor.
|||	        When this is 1.0, the amplitude of the sound radiated behind
|||	        the source is the same as that in front, so you get an
|||	        omnidirectional response.  When it's 0.0, the amplitude directly
|||	        behind the source falls to 0, according to a modified
|||	        cardioid equation given below.
|||
|||	        Using an intermediate value results in some sound being heard
|||	        from any angle around the source.
|||
|||	        If this tag is not specified, the default value is 1.0
|||	        (an omnidirectional source).
|||
|||	  Unidirectional Cardioid Equation -preformatted
|||
|||	          frontamp(1.0 - ((1.0 - backamp) * cos(theta) )
|||	    amp = ----------------------------------------------
|||	                        (2.0 - backamp)
|||
|||	    where theta is the diffence angle between the direction of
|||	    radiation in 3-space and a line from the sound to the observer.
|||
|||	  Flags
|||
|||	    S3D_F_PAN_DELAY
|||	        Uses a delay line to simulate interaural time differences (ITD).
|||	        For most sounds, this provides a better azimuth cue than simply
|||	        adjusting the left and right channel gain, at the expense of
|||	        three FIFOs.  Can be used with S3D_F_PAN_AMPLITUDE to provide
|||	        interaural intensity differences, but this is usually better
|||	        done by pairing with S3D_F_PAN_FILTER.
|||
|||	    S3D_F_PAN_AMPLITUDE
|||	        Uses gain control to simulate interaural intensity differences
|||	        (IID).  For steady-state sounds with spectral content mostly
|||	        above 1.5K, this provides a reasonable azimuth cue with low
|||	        DSP overhead.  When used with S3D_F_PAN_FILTER tends to over-
|||	        emphasize interaural gain (left/right) differences, but by
|||	        itself provides no front/back or up/down cues.
|||
|||	    S3D_F_PAN_FILTER
|||	        Uses a custom filter instrument to simulate head- and upper-
|||	        body-related effects.  In combination with either the PAN_DELAY
|||	        or PAN_AMPLITUDE flag, this provides much better azimuth cues,
|||	        especially for differentiating between front and back.
|||
|||	    S3D_F_DOPPLER
|||	        This flag indicates that the 3DSound should calculate
|||	        frequency shifts due to the Doppler effect when the sound is
|||	        moving.  Frequency information is passed back to the application
|||	        through the Get3DSoundParms function, where it may be used to
|||	        control frequency-dependent aspects of the source instrument
|||	        (the sample rate, lfo rates and so on).  Primarily useful for
|||	        fast-moving sounds.  The doppler calculation doesn't take any
|||	        DSP resources, but utilizes the host CPU.
|||
|||	    S3D_F_DISTANCE_AMPLITUDE_SQUARE
|||	        This flag indicates that amplitude should be calculated
|||	        according to the inverse square law, whereby intensty drops off
|||	        proportional to the square of the distance between the sound and
|||	        the observer.  While physically correct, studies show that for
|||	        some sounds, particularly familiar ones, an inverse cubed law
|||	        may sound more realistic.  The amplitude information is passed
|||	        back to the application through the Get3DSoundParms function
|||	        as a "distance factor" from 1.0 to 0.0, where 1.0 is the closest
|||	        and 0.0 the farthest away.  The application may use the number
|||	        to directly control the gain of the sound in the final mix, as
|||	        well as control the amount of reverberance of the sound.  The
|||	        distance factor calculation doesn't take any DSP resources.
|||
|||	    S3D_F_DISTANCE_AMPLITUDE_SONE
|||	        This flag indicates that amplitude should be calculated
|||	        according to an approximation of the "sone" scale, where
|||	        intensity drops off according to an inverse cube relationship
|||	        to distance between sound and observer.  Frequency-dependent
|||	        amplitude attenuation is not enbled with this flag, but rather
|||	        is done with a low-pass filter when using the S3D_F_PAN_FILTER
|||	        flag. The amplitude information is passed back to the
|||	        application through the Get3DSoundParms function as a
|||	        "distance factor" from 1.0 to 0.0, where 1.0 is the closest
|||	        and 0.0 the farthest away.  The application may use the number
|||	        to directly control the gain of the sound in the final mix, as
|||	        well as control the amount of reverberance of the sound.  The
|||	        distance factor calculation doesn't take any DSP resources.
|||
|||	    S3D_F_OUTPUT_HEADPHONES
|||	        If this flag is not set, it is assumed that the observer
|||	        will be hearing sound over loudspeakers positioned to either
|||	        side of the video display.  In this case, many of the 3D sound
|||	        cues are inaudible, and so will not be used regardless of the
|||	        settings of other flags.  Additionally, cross-channel
|||	        cancellation will be employed to decorrelate signals from each
|||	        of the loudspeakers.
|||
|||	    S3D_F_SMOOTH_AMPLITUDE
|||	        When you select PAN_AMPLITUDE, or you use the BACK_AMPLITUDE tag
|||	        for a unidirectional sound source, Sound3D updates an gain
|||	        scalar every time Move3DSound is called.  In some situations,
|||	        this can result in audible quantizing of the sound's loudness.
|||	        If this happens, you can smooth out the gain changes by setting
|||	        this flag.  The smoothing uses the envelope.dsp instrument, so
|||	        there's a penalty in dsp resource usage associated with it.  If
|||	        you're running low on resources, try disabling this flag.
|||
|||	        The default is disabled.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if successful or an error code (a negative
|||	    value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V30.
|||
|||	  Caveats
|||
|||	    Elevation cues are not yet supported.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/sound3d.h>, libmusic.a, libspmath.a, System.m2/Modules/audio,
|||	    System.m2/Modules/audiopatch
|||
|||	  See Also
|||
|||	    Delete3DSound(), Start3DSound(), Stop3DSound(), Move3DSound(),
|||	    Move3DSoundTo(), Get3DSoundInstrument(), Get3DSoundPos(),
|||	    Get3DSoundParms()
*/

Err Create3DSound( Sound3D** s3dHandle, const TagArg* tagList )
{
	Sound3D* Snd3D;
	Err Result;
	uint32 flags;

	flags = (uint32)GetTagArg( tagList, S3D_TAG_FLAGS,
	  (TagData)S3D_DEFAULT_FLAGS);

	Snd3D = (Sound3D*) AllocMem(sizeof(Sound3D), MEMTYPE_NORMAL);
DBUG(("Sound3D allocated at 0x%x\n", Snd3D));

	if (Snd3D)
	{
		Snd3D->s3d_Flags = flags;

		/* get directionality info, default to omnidirectional */
		Snd3D->s3d_BackAmplitude = ConvertTagData_FP( GetTagArg( tagList,
		  S3D_TAG_BACK_AMPLITUDE, ConvertFP_TagData(1.0)) );

		Result = Load3DSound( Snd3D );
		CHECKVAL(Result, "Load3DSound");

		*s3dHandle = Snd3D;

		return 0;
	}
	else
	{
		*s3dHandle = NULL;
		return (ML_ERR_NOMEM);
	}

cleanup:
	Delete3DSound( &Snd3D );
	*s3dHandle = NULL;
	return Result;
}

/****************************************************************/
/*
|||	AUTODOC -public -class libmusic -group Sound3D -name Delete3DSound
|||	Release resources and deallocate 3D Sound data structures.
|||
|||	  Synopsis
|||
|||	    void Delete3DSound( Sound3D** a3DSound )
|||
|||	  Description
|||
|||	    This function stops a 3D Sound playing (if it's playing), deletes
|||	    all the items associated with the sound and frees the DSP resources
|||	    and memory associated with the sound.  It does not delete or
|||	    otherwise affect the source sound connected to the 3D Sound.
|||
|||	    After freeing memory, it clears the pointer in the handle a3DSound
|||	    to NULL.
|||
|||	  Arguments
|||
|||	    a3DSound
|||	        a pointer to a pointer to a Sound3D structure, previously set
|||	        using a call to Create3DSound().
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V30.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/sound3d.h>, libmusic.a, libspmath.a, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    Create3DSound(), Start3DSound(), Stop3DSound(), Move3DSound(),
|||	    Move3DSoundTo(), Get3DSoundInstrument(), Get3DSoundPos(),
|||	    Get3DSoundParms()
*/

void Delete3DSound( Sound3D** Snd3D )
{
	if (*Snd3D)
	{
		Stop3DSound( *Snd3D );
		Unload3DSound( *Snd3D );
		FreeMem(*Snd3D, sizeof(Sound3D));
		*Snd3D = NULL;
	}

	return;
}

/****************************************************************/
/*
|||	AUTODOC -public -class libmusic -group Sound3D -name Get3DSoundInstrument
|||	Returns the DSP instrument to which a source should be connected to use
|||	a 3D Sound.
|||
|||	  Synopsis
|||
|||	    Item Get3DSoundInstrument( const Sound3D* a3DSound )
|||
|||	  Description
|||
|||	    The 3D Sound calls process the output of a source instrument to
|||	    provide an illusion of space.  The source instrument (or patch)
|||	    is created, destroyed and otherwise maintained by the application.
|||	    In order to connect the source instrument to the 3D Sound, the
|||	    application uses this function to find the 3D Sound instrument that
|||	    can subsequently be used in a call to ConnectInstruments().
|||
|||	  Arguments
|||
|||	    a3DSound
|||	        a pointer to a Sound3D structure, previously set using a call
|||	        to Create3DSound().
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the 3D Sound instrument
|||	    if successful, or an error code (a negative value) if an error
|||	    occurs.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V30.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/sound3d.h>, libmusic.a, libspmath.a, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    Create3DSound(), Delete3DSound(), Start3DSound(), Stop3DSound(),
|||	    Move3DSound(), Move3DSoundTo(), Get3DSoundPos(), Get3DSoundParms()
*/

Item Get3DSoundInstrument( const Sound3D* Snd3D )
{
	Err Result;

	Result = GetAudioItemInfo (Snd3D->s3d_InsItem, 0);
	if (Result >= 0) return (Snd3D->s3d_InsItem);
	else return ((Item)Result);
}

/****************************************************************/
static Err Unload3DSound( Sound3D* Snd3D )
{
	Err Result;
DBUG2(("Unload3DSound(0x%x)\n", Snd3D));

/*
   This will automatically release the 3D instrument and all knobs. Applications
   have to release their source instrument.
*/
	Result = DeleteSound3DPatchTemplate( Snd3D->s3d_InsTemplate );
	Snd3D->s3d_InsTemplate = 0;
	return Result;
}

/****************************************************************/
static Err Load3DSound( Sound3D* Snd3D )
/*
    Loads a 3D sound instrument.
    Returns 0 or an error code.
*/
{
	Err Result = 0;

DBUG2(("Load3DSound(0x%x)\n", Snd3D));

	Snd3D->s3d_InsTemplate = MakeSound3DPatchTemplate(Snd3D->s3d_Flags);
	CHECKVAL(Snd3D->s3d_InsTemplate, "MakeSound3DPatchTemplate");

	Snd3D->s3d_InsItem = CreateInstrument(Snd3D->s3d_InsTemplate, NULL);
	CHECKVAL(Snd3D->s3d_InsItem, "CreateInstrument");

/* Create knobs for fiddling with filter */
	if(Snd3D->s3d_Flags & S3D_F_PAN_FILTER)
	{
		Snd3D->s3d_AlphaKnob = CreateKnob(Snd3D->s3d_InsItem, "Alpha",
		  NULL);
		CHECKVAL(Snd3D->s3d_AlphaKnob, "CreateKnob");

		Snd3D->s3d_BetaKnob = CreateKnob(Snd3D->s3d_InsItem, "Beta",
		  NULL);
		CHECKVAL(Snd3D->s3d_BetaKnob, "CreateKnob");

		Snd3D->s3d_FeedKnob = CreateKnob(Snd3D->s3d_InsItem, "Feed",
		  NULL);
		CHECKVAL(Snd3D->s3d_FeedKnob, "CreateKnob");
	}

/* Create knobs for fiddling with delay */
	if(Snd3D->s3d_Flags & S3D_F_PAN_DELAY)
	{
		Snd3D->s3d_DelayKnob = CreateKnob(Snd3D->s3d_InsItem, "Delay",
		  NULL);
		CHECKVAL(Snd3D->s3d_DelayKnob, "CreateKnob");

		Snd3D->s3d_DelayRateKnob = CreateKnob(Snd3D->s3d_InsItem, "DelayRate",
		  NULL);
		CHECKVAL(Snd3D->s3d_DelayRateKnob, "CreateKnob");

/* Create probe for frame drifts */
		Snd3D->s3d_DriftProbe = CreateProbeVA(Snd3D->s3d_InsItem,
		  "Drift", AF_TAG_TYPE, AF_SIGNAL_TYPE_WHOLE_NUMBER, TAG_END );
		CHECKVAL(Snd3D->s3d_DriftProbe, "CreateProbeVA");
	}

/* Create knobs for fiddling with amplitude for IID, directionality */
	if((Snd3D->s3d_Flags & S3D_F_PAN_AMPLITUDE) || (Snd3D->s3d_BackAmplitude != 1.0))
	{
		Snd3D->s3d_AmpKnob = CreateKnob(Snd3D->s3d_InsItem, "Amplitude",
		  NULL);
		CHECKVAL(Snd3D->s3d_AmpKnob, "CreateKnob");

		if(Snd3D->s3d_Flags & S3D_F_SMOOTH_AMPLITUDE)
		{
			Snd3D->s3d_EnvRateKnob = CreateKnob(Snd3D->s3d_InsItem,
			  "EnvRate", NULL);
			CHECKVAL(Snd3D->s3d_EnvRateKnob, "CreateKnob");

/* Create probe for amplitude values */
			Snd3D->s3d_AmpProbe = CreateProbeVA(Snd3D->s3d_InsItem,
			  "AmpProbe", AF_TAG_TYPE, AF_SIGNAL_TYPE_GENERIC_SIGNED, TAG_END );
			CHECKVAL(Snd3D->s3d_AmpProbe, "CreateProbeVA");
		}
	}

cleanup:
	return Result;
}

/***********************************************************************/
static Err s3dReadLeftRightDrift( const Sound3D* Snd3D, int32* LeftPtr, int32* RightPtr )
{
	float32 pLeft, pRight;
	Err Result;

	Result = ReadProbePart(Snd3D->s3d_DriftProbe, 0, &pLeft );
	CHECKVAL(Result, "s3dReadLeftRightDrift: ReadProbePart");

	Result = ReadProbePart(Snd3D->s3d_DriftProbe, 1, &pRight );
	CHECKVAL(Result, "s3dReadLeftRightDrift: ReadProbePart");

	*LeftPtr = (int32) pLeft;
	*RightPtr = (int32) pRight;

cleanup:
	return Result;
}

/***********************************************************************/
static Err s3dReadLeftRightAmp( const Sound3D* Snd3D, float32* LeftAmp, float32* RightAmp )
{
	Err Result;

	if (Snd3D->s3d_Flags & S3D_F_SMOOTH_AMPLITUDE)
	{
		Result = ReadProbePart(Snd3D->s3d_AmpProbe, 0, LeftAmp );
		CHECKVAL(Result, "s3dReadLeftRightAmp: ReadProbePart");

		Result = ReadProbePart(Snd3D->s3d_AmpProbe, 1, RightAmp );
		CHECKVAL(Result, "s3dReadLeftRightAmp: ReadProbePart");
	}
	else
	{
		Result = ReadKnobPart(Snd3D->s3d_AmpKnob, 0, LeftAmp );
		CHECKVAL(Result, "s3dReadLeftRightAmp: ReadKnobPart");

		Result = ReadKnobPart(Snd3D->s3d_AmpKnob, 1, RightAmp );
		CHECKVAL(Result, "s3dReadLeftRightAmp: ReadKnobPart");
	}

cleanup:
	return Result;
}

/***********************************************************************/
/*
|||	AUTODOC -public -class libmusic -group Sound3D -name Start3DSound
|||	Starts a 3D Sound, and positions it in space.
|||
|||	  Synopsis
|||
|||	    Err Start3DSound( const Sound3D* a3DSound, const PolarPosition4D* Pos4D )
|||
|||	  Description
|||
|||	    This function starts the DSP instrument associated with a 3D Sound
|||	    and moves the sound from the center of the head to the position
|||	    specified with Pos4D.  The source sound should be started after
|||	    a call to Start3DSound so that this movement is not audible.
|||
|||	  Arguments
|||
|||	    a3DSound
|||	        a pointer to a Sound3D structure, previously set using a call
|||	        to Create3DSound().
|||
|||	    Pos4D
|||	        a pointer to a PolarPosition4D containing coordinates of the
|||	        sound's starting position. See <audio/sound3d.h>.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if successful or an error code (a negative
|||	    value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V30.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/sound3d.h>, libmusic.a, libspmath.a, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    Create3DSound(), Delete3DSound(), Stop3DSound(), Move3DSound(),
|||	    Move3DSoundTo(), Get3DSoundInstrument(), Get3DSoundPos(),
|||	    Get3DSoundParms()
*/

Err Start3DSound( const Sound3D* Snd3D, const PolarPosition4D* Pos4D )
/*
   Starts a 3D sound playing at the given polar coordinates.
   Note that this should be done before starting your source instrument, so that
   the 3D instrument can position itself at the right place before the sound
   is heard.
*/
{
	Err Result;
	PolarPosition4D HeadCenter = { 0, 0.0, 0.0, 0 };

DBUG2(("Start3DSound(0x%x, 0x%x)\n", Snd3D, Pos4D));
	Result = StartInstrument( Snd3D->s3d_InsItem, 0 );
	CHECKVAL(Result, "StartInstrument");

/* Move to the start position quickly */
	HeadCenter.pp4d_Time = Pos4D->pp4d_Time;
	Result = Move3DSound( Snd3D, &HeadCenter, Pos4D );
	CHECKVAL(Result, "Move3DSound");
	Move3DSound( Snd3D, Pos4D, Pos4D );	/* stop - velocity = 0 */
	CHECKVAL(Result, "Move3DSound");

cleanup:
	return Result;
}

/***********************************************************************/
/*
|||	AUTODOC -public -class libmusic -group Sound3D -name Stop3DSound
|||	Stops a 3D Sound.
|||
|||	  Synopsis
|||
|||	    Err Stop3DSound( const Sound3D* a3DSound )
|||
|||	  Description
|||
|||	    This function stops the DSP instrument associated with the sound.
|||	    If stopped before the source sound, this can result in an audible
|||	    click, so the function should be called after the source sound
|||	    has been stopped.
|||
|||	  Arguments
|||
|||	    a3DSound
|||	        a pointer to a Sound3D structure, previously set using a call
|||	        to Create3DSound().
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if successful or an error code (a negative
|||	    value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V30.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/sound3d.h>, libmusic.a, libspmath.a, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    Create3DSound(), Delete3DSound(), Start3DSound(), Move3DSound(),
|||	    Move3DSoundTo(), Get3DSoundInstrument(), Get3DSoundPos(),
|||	    Get3DSoundParms()
*/

int32 Stop3DSound( const Sound3D* Snd3D )
{
	return StopInstrument(Snd3D->s3d_InsItem, 0);
}

/***********************************************************************/
/*
|||	AUTODOC -public -class libmusic -group Sound3D -name Move3DSound
|||	Moves a 3D Sound in a line from start to end over a given time period.
|||
|||	  Synopsis
|||
|||	    Err Move3DSound( Sound3D* a3DSound, const PolarPosition4D* Start4D,
|||	                     const PolarPosition4D* End4D )
|||
|||	  Description
|||
|||	    This function animates a sound, moving it from the position
|||	    specified in the argument Start4D to that in End4D, over a time
|||	    period given by (End4D.pp4d_Time - Start4D.pp4d_Time).  The illusion
|||	    of movement is generated using the cues specified when the 3D Sound
|||	    was created with Create3DSound().  Applications that use host-level
|||	    cues (for example, doppler shift using frequency changes, distance-
|||	    based amplitude and reverberation) can call Get3DSoundParms()
|||	    immediately after calling this function to retrieve current values
|||	    for these cues.
|||
|||	    Since time is specified in frame counts and some parameters are
|||	    set directly rather than ramped, this function should be called
|||	    relatively frequently (at least several times a second) for a moving
|||	    sound.  This is especially important for sounds close to the observer,
|||	    since the angles subtended by the movement may be quite large.
|||
|||	  Arguments
|||
|||	    a3DSound
|||	        a pointer to a Sound3D structure, previously set using a call
|||	        to Create3DSound().
|||
|||	    Start4D
|||	        the position and time vector at the start of the sound's
|||	        movement.  See <audio/sound3d.h>.
|||
|||	    End4D
|||	        the position and time vector to be moved to.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if successful or an error code (a negative
|||	    value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V30.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/sound3d.h>, libmusic.a, libspmath.a, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    Create3DSound(), Delete3DSound(), Start3DSound(), Stop3DSound(),
|||	    Move3DSoundTo(), Get3DSoundInstrument(), Get3DSoundPos(),
|||	    Get3DSoundParms()
*/

Err Move3DSound( Sound3D* Snd3D, const PolarPosition4D* Start4D, const PolarPosition4D* End4D )
/*
  Move the sound from start to end in (end-start) frames
*/
{
	Err Result;
	float32 LeftDelay, RightDelay, DelayRate, LeftAmp, RightAmp, PointAmp,
	  EnvRate;
	BothEarParams BEP;
	PolarPosition4D nStart, nEnd;

DBUG2(("Move3DSound(0x%x, 0x%x, 0x%x)\n", Snd3D, Start4D, End4D));

/* Copy the positions, normalize the angles in the copy */
	nStart = *Start4D;
	nEnd = *End4D;
	nStart.pp4d_Theta = s3dNormalizeAngle( nStart.pp4d_Theta );
	nEnd.pp4d_Theta = s3dNormalizeAngle( nEnd.pp4d_Theta );

	if(Snd3D->s3d_Flags & S3D_F_PAN_DELAY)
	{
		Calc3DSoundDelays ( &nStart, &nEnd , &LeftDelay, &RightDelay, &DelayRate );
		Result = Set3DSoundDelays ( Snd3D, LeftDelay, RightDelay, DelayRate );
		CHECKVAL(Result, "Set3DSoundDelays");
	}

	if(Snd3D->s3d_Flags & S3D_F_DOPPLER)
	{
		Snd3D->s3d_Parms.s3dp_Doppler = Calc3DSoundDoppler( &nStart, &nEnd );
	}
	else
	{
		Snd3D->s3d_Parms.s3dp_Doppler = 1.0;
	}


	if(Snd3D->s3d_Flags & S3D_F_PAN_FILTER)
	{
		Calc3DSoundFilters ( &nEnd, &BEP );
		Result = Set3DSoundFilters ( Snd3D, &BEP );
		CHECKVAL(Result, "Set3DSoundFilters");
	}

	if(Snd3D->s3d_Flags & S3D_F_PAN_AMPLITUDE)
	{
		Calc3DSoundIID( &nStart, &nEnd, &LeftAmp, &RightAmp );
	}
	else
	{
		LeftAmp = RightAmp = 1.0;
	}


	if(Snd3D->s3d_BackAmplitude != 1.0)
	{
		Calc3DSoundDirectionality( Snd3D, &nEnd, &PointAmp );
	}
	else
	{
		PointAmp = 1.0;
	}


	if((Snd3D->s3d_Flags & S3D_F_PAN_AMPLITUDE) || (Snd3D->s3d_BackAmplitude != 1.0))
	{
		if (Snd3D->s3d_Flags & S3D_F_SMOOTH_AMPLITUDE)
		{
			Calc3DSoundEnvRate( &nStart, &nEnd, &EnvRate );
		}


		Result = Set3DSoundAmplitude( Snd3D, PointAmp, LeftAmp, RightAmp,
		  EnvRate );
		CHECKVAL(Result, "Set3DSoundAmplitude");
	}

	Snd3D->s3d_Parms.s3dp_DistanceFactor = Calc3DSoundDF ( &nEnd,
	  Snd3D->s3d_Flags );

/* Update last start and end */
	Snd3D->s3d_LastStart = nStart;
	Snd3D->s3d_LastEnd = nEnd;

cleanup:
	return Result;
}

/***********************************************************************/
/*
|||	AUTODOC -public -class libmusic -group Sound3D -name Move3DSoundTo
|||	Moves a 3D Sound in a line from wherever it currently is to a given
|||	endpoint.
|||
|||	  Synopsis
|||
|||	    Err Move3DSoundTo( Sound3D* a3DSound, const PolarPosition4D* Target4D )
|||
|||	  Description
|||
|||	    This function animates a sound, moving it from its current position
|||	    (as returned by a call to Get3DSoundPos()) to that in Target4D over
|||	    a time period given by (Target4D.pp4d_Time - GetAudioFrameCount()).
|||	    The illusion of movement is generated using the cues specified when
|||	    the 3D Sound was created using Create3DSound().  Applications that
|||	    use host-level cues (for example, doppler shift using frequency
|||	    changes, distance-based amplitude and reverberation) can call
|||	    Get3DSoundParms() immediately after calling this function to
|||	    retrieve current values for these cues.
|||
|||	    This function is equivalent to calling Get3DSoundPos(), and using
|||	    the result as the Start4D argument in a call to Move3DSound().
|||
|||	  Arguments
|||
|||	    a3DSound
|||	        a pointer to a Sound3D structure, previously set using a call
|||	        to Create3DSound().
|||
|||	    Target4D
|||	        the position and time vector to be moved to.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if successful or an error code (a negative
|||	    value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V30.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/sound3d.h>, libmusic.a, libspmath.a, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    Create3DSound(), Delete3DSound(), Start3DSound(), Stop3DSound(),
|||	    Move3DSound(), Get3DSoundInstrument(), Get3DSoundPos(),
|||	    Get3DSoundParms()
*/

Err Move3DSoundTo( Sound3D* Snd3D, const PolarPosition4D* Target4D )
{
	PolarPosition4D s3d_whereamI;
	Err Result;

	Result = Get3DSoundPos( Snd3D, &s3d_whereamI );
	CHECKVAL(Result, "Get3DSoundPos");

	Result = Move3DSound( Snd3D, &s3d_whereamI, Target4D );
	CHECKVAL(Result, "Move3DSound");


cleanup:
	return Result;
}

/****************************************************************/
static float32 s3dInterpolate (float32 Scale, float32 Range, float32 MinVal, float32 MaxVal)
{
	float32 Result;

DBUG2(("s3dInterpolate: Scale = %f, Range = %f\n", Scale, Range ));
DBUG2(("s3dInterpolate: MinVal = %f, MaxVal = %f\n", MinVal, MaxVal ));

	Result = (Scale * (MaxVal - MinVal) / Range) + MinVal;

DBUG2(("s3dInterpolate: Result = %f\n", Result ));

	return Result;
}

/****************************************************************/

/****************************************************************/
/*
|||	AUTODOC -public -class libmusic -group Sound3D -name s3dNormalizeAngle
|||	Normalize an angle to be between -PI and +PI.
|||
|||	  Synopsis
|||
|||	    float32 s3dNormalizeAngle( float32 Angle )
|||
|||	  Description
|||
|||	    This functions adds or subtracts 2*PI so that the resulting angle
|||	    is between negative PI and positive PI.
|||
|||	  Arguments
|||
|||	    Angle
|||	        an angle expressed in radians.  Angle must be between
|||	        -3*PI and +3*PI or the resulting angle will not be
|||	        normalized.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V30.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/sound3d.h>, libmusic.a, libspmath.a, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    Create3DSound(), Start3DSound(), Stop3DSound(), Move3DSound(),
|||	    Move3DSoundTo(), Get3DSoundInstrument(), Get3DSoundPos(),
|||	    Get3DSoundParms()
|||
*/
float32 s3dNormalizeAngle( float32 Angle )
{
/* FIXME - s3dNormalizeAngle - what if Angle is farther than PI
** from being normalized?  Maybe should be WHILE instead of IF. */
	if (Angle > S3D_HALFCIRCLE) Angle -= S3D_FULLCIRCLE;
	if (Angle < -S3D_HALFCIRCLE) Angle += S3D_FULLCIRCLE;
	return Angle;
}

#define S3D_LOWPASS_MAX 0.828125 /* was 0x6A00 in F16 */
#define S3D_NOTCH_MAX 0.125      /* was 0x1000 in F16 */
#define S3D_DSP_ONE 1.0          /* was 0x7FFF in F16 */
/***********************************************************************/
static void Calc3DSoundEar ( const PolarPosition4D* Pos4D, EarParams* ERP )
{
	float32 nt;
	float32 Alpha, Beta, Feed;

	nt = s3dNormalizeAngle(Pos4D->pp4d_Theta);
DBUG2(("Calc3DSoundEar: nt = %f\n", nt));
	if (nt < 0)
	{
		if (nt < -S3D_QUARTERCIRCLE)
		{
/* head shadowed and behind, -180 => -90*/
			Alpha = S3D_LOWPASS_MAX;
			Beta = s3dInterpolate(
			  S3D_HALFCIRCLE+nt, S3D_QUARTERCIRCLE,
			  S3D_NOTCH_MAX/2, 0.0);
		}
		else
		{
/* head shadowed, in front, -90 => 0 */
			Alpha = S3D_LOWPASS_MAX * sinf(S3D_HALFCIRCLE+nt);
			Beta = s3dInterpolate(
			  S3D_QUARTERCIRCLE+nt, S3D_QUARTERCIRCLE,
			  0.0, S3D_NOTCH_MAX);
		}
	}
	else
	{
		if (nt > S3D_QUARTERCIRCLE)
		{
/* head direct and behind, 90 => 180 */
			Alpha = S3D_LOWPASS_MAX * sinf(nt-S3D_QUARTERCIRCLE);
			Beta = s3dInterpolate(
			  nt-S3D_QUARTERCIRCLE, S3D_QUARTERCIRCLE,
			  0.0, S3D_NOTCH_MAX/2);
		}
		else
		{
/* head direct, in front, 0 => 90 */
			Alpha = 0.0;
			Beta = s3dInterpolate(
			  nt, S3D_QUARTERCIRCLE,
			  S3D_NOTCH_MAX, 0.0);
		}
	}

/* Drop off with increasing radius */
	Alpha += Pos4D->pp4d_Radius / 32768.0;
	if (Alpha > S3D_LOWPASS_MAX) Alpha = S3D_LOWPASS_MAX;
	Feed = S3D_DSP_ONE - Alpha - Beta;

	ERP->erp_Alpha = Alpha;
	ERP->erp_Beta = Beta;
	ERP->erp_Feed = Feed;

	return;
}

/***********************************************************************/
static void Calc3DSoundFilters( const PolarPosition4D* Pos4D, BothEarParams* BEP)
{
	PolarPosition4D aPos;

	memcpy (&aPos, Pos4D, sizeof(PolarPosition4D));

	Calc3DSoundEar (&aPos, &BEP->bep_RightEar);

/* Negate theta for opposite ear calculation. */
	aPos.pp4d_Theta = -aPos.pp4d_Theta;
	Calc3DSoundEar (&aPos, &BEP->bep_LeftEar);

	return;
}

/***********************************************************************/
static void Calc3DSoundDelays( const PolarPosition4D* Start4D, const PolarPosition4D* End4D,
  float32* LeftDelay, float32* RightDelay, float32* DelayRate )
{
	float32 DT;  		/* Actual time start to end in samples. */

	DT = (float32)(((uint16) End4D->pp4d_Time - (uint16) Start4D->pp4d_Time) & 0xFFFF);
DBUG2(("Calc3DSoundDelays: DT = %f\n", DT));

/*
	Assume DelayRate knob works as follows: -1.0 => maximum movement, which at the moment means 1/256s to get to
	target from wherever you are.  Does that mean an exponential?  Anyway, -0.5 means take twice as long.  0.0
	means you'll never get there.  Since the sampler can only accelerate by 2x, we should make sure that we don't
	drive the SampleRate below 0.5 or above 2.0.
*/

	if (DT != 0.0) *DelayRate = -(FRAMERATE / (DT * 256.0));
	else *DelayRate = -1.0;

	CLIPTO(*DelayRate, -1.0, 0.0);
	*LeftDelay =  S3D_SMALL_DELAY_CENTER - S3D_DISTANCE_TO_EAR * sinf( End4D->pp4d_Theta );
	*RightDelay = S3D_SMALL_DELAY_CENTER + S3D_DISTANCE_TO_EAR * sinf( End4D->pp4d_Theta );

/* Round up if necessary */
	if (fmodf(*LeftDelay, 1.0) >= 0.5) *LeftDelay += 1.0;
	if (fmodf(*RightDelay, 1.0) >= 0.5) *RightDelay += 1.0;

	return;
}

/***********************************************************************/
static float32 Calc3DSoundDoppler( const PolarPosition4D* Start4D, const PolarPosition4D* End4D )
/*
	Calculate doppler: d(f) = d(time)/d(time) + d(radius)
*/
{
	float32 DT;  		/* Actual time start to end in samples. */
	float32 deltaRadius;	/* Change in radius, in samples. */
	float32 freqScalar;

	DT = (float32)(((uint16) End4D->pp4d_Time - (uint16) Start4D->pp4d_Time) & 0xFFFF);
	deltaRadius = End4D->pp4d_Radius - Start4D->pp4d_Radius;
DBUG2(("DeltaRadius: %f, DT: %f\n", deltaRadius, DT));

	if (DT + deltaRadius != 0.0) freqScalar = DT / (DT + deltaRadius);
	else freqScalar = 1.0;

	return freqScalar;
}

/***********************************************************************/
static void Calc3DSoundIID( const PolarPosition4D* Start4D, const PolarPosition4D* End4D,
  float32* LeftAmp, float32* RightAmp )
{
	TOUCH(Start4D);

	*LeftAmp =  0.5 - ((S3D_MAX_AMP_DIFFERENTIAL/2.0) * sinf( End4D->pp4d_Theta ));
	*RightAmp = 0.5 + ((S3D_MAX_AMP_DIFFERENTIAL/2.0) * sinf( End4D->pp4d_Theta ));

	return;
}

/***********************************************************************/
static void Calc3DSoundDirectionality( const Sound3D* Snd3D, const PolarPosition4D* End4D, float32* PointAmp )
{
	float32 theta1, theta2, phi1, phi2;
	float32 diffcos;

	theta1 = End4D->pp4d_Theta;
	theta2 = End4D->pp4d_or_Theta;
	phi1 = End4D->pp4d_Phi;
	phi2 = End4D->pp4d_or_Phi;
/*
  Assume the modified cardioid response described below, spun around the x
  axis to give three dimensions.  Theta then becomes the angle between the
  radiation direction unit vector and a line to the observer, given by the
  position unit vector.

  From analytic geometry, for radius = 1, direction angles are given by

    cos(alpha) = x = cos(theta).cos(phi)
    cos(beta) = y = sin(theta).cos(phi)
    cos(gamma) = z = sin(phi)

  and cos(difference angle between two lines) =

    cos(alpha1).cos(alpha2) + cos(beta1).cos(beta2) + cos(gamma1).cos(gamma2)
*/

	diffcos = (cosf(theta1) * cosf(phi1) * sinf(theta2) * cosf(phi2)) +
	          (sinf(theta1) * cosf(phi1) * cosf(theta2) * cosf(phi2)) +
	          (sinf(phi1) * sinf(phi2));

/*
  Modified cardioid response:
  amplitude = (1 - (1-b).cos(theta))/(1+(1-b)), where b = back amplitude.  This
  ranges between a circle when b = 1 to a cardioid when b = 0.

  cos(theta) is the variable diffcos calculated above.
*/

	*PointAmp = (1.0 - ((1.0 - Snd3D->s3d_BackAmplitude) * diffcos )) /
	  (2.0 - Snd3D->s3d_BackAmplitude );

	return;
}

#define ENV_PHASERANGE (32767)

/***********************************************************************/
static void Calc3DSoundEnvRate( const PolarPosition4D* Start4D, const PolarPosition4D* End4D,
  float32* EnvRate )
{
	uint16 dT;

	/* This uses the same logic as Envelopes */
  	dT = (uint16)(((uint16)End4D->pp4d_Time - (uint16)Start4D->pp4d_Time)
  	  & 0xFFFF);
  	if (dT == 0) dT = 1;
  	*EnvRate = ((ENV_PHASERANGE + (dT - 1)) / dT) / 32768.0;

DBUG2(("EnvRate: dT = %d, rate = %f\n", dT, *EnvRate));

	return;
}

/***********************************************************************/
static float32 Calc3DSoundDF( const PolarPosition4D* Pos4D, uint32 s3dFlags )
{
	float32 Radius;

/*
  Amplitude calculations:
  Using inverse square law, intensity is inversely proportional to the square
  of distance.  Intensity is proportional to the square of amplitude (gain),
  so gain is inversely proportional to distance.  If MinRadius is the sphere
  defining the head (about 4"), which by definition is at amplitude 1.0, and
  the maximum radius is 32767, at amplitude 0.0, then it's just an interpolation
  to get the amplitude value given a radius value.
  Using the inverse cubed law to approximate the sone scale, amplitude is
  inversely proportional to (distance^3/2).
  If neither flag is specified, just return 1.0 (the amplitude doesn't vary
  with distance).
*/
  	Radius = Pos4D->pp4d_Radius;
	if (Radius < S3D_MIN_RADIUS) Radius = S3D_MIN_RADIUS;
	if (s3dFlags & S3D_F_DISTANCE_AMPLITUDE_SQUARE)
	{
		return (S3D_DSP_ONE / (Radius/S3D_MIN_RADIUS));
	}
	else if (s3dFlags & S3D_F_DISTANCE_AMPLITUDE_SONE)
	{
		return (S3D_DSP_ONE / (powf((Radius/S3D_MIN_RADIUS), 1.5)));
	}
	else
	{
		return (1.0);
	}
}

/***********************************************************************/
static Err Set3DSoundDelays ( const Sound3D* Snd3D, float32 LeftDelay, float32 RightDelay,
  float32 DelayRate )
{
	Err Result;

DBUG2(("LeftDelay = %f, RightDelay = %f, Rate = %f\n", LeftDelay, RightDelay,
  DelayRate));

	Result = SetKnobPart(Snd3D->s3d_DelayKnob, 0, LeftDelay);
	CHECKVAL(Result, "SetKnobPart");

	Result = SetKnobPart(Snd3D->s3d_DelayKnob, 1, RightDelay);
	CHECKVAL(Result, "SetKnobPart");

	Result = SetKnobPart(Snd3D->s3d_DelayRateKnob, 0, DelayRate);
	CHECKVAL(Result, "SetKnobPart");

cleanup:
	return Result;
}

/***********************************************************************/
static Err Set3DSoundAmplitude ( const Sound3D* Snd3D, float32 PointAmp, float32 LeftAmp,
  float32 RightAmp, float32 EnvRate )
{
	Err Result;

	if (Snd3D->s3d_Flags & S3D_F_SMOOTH_AMPLITUDE)
	{

DBUG2(("PointAmp = %f, LeftAmp = %f, RightAmp = %f, Incr = %f\n", PointAmp,
  LeftAmp, RightAmp, EnvRate));

		Result = SetKnob(Snd3D->s3d_EnvRateKnob, EnvRate);
		CHECKVAL(Result, "SetKnob, amplitude envelope increment");
	}

	Result = SetKnobPart(Snd3D->s3d_AmpKnob, 0, LeftAmp * PointAmp);
	CHECKVAL(Result, "SetKnobPart, left amplitude");

	Result = SetKnobPart(Snd3D->s3d_AmpKnob, 1, RightAmp * PointAmp);
	CHECKVAL(Result, "SetKnobPart, right amplitude");

DBUG2(("Set amplitude OK\n"));

cleanup:
	return Result;
}

/***********************************************************************/
static Err Set3DSoundFilters ( const Sound3D* Snd3D, const BothEarParams* BEP )
{
	Err Result;

DBUG2(("lAlpha = %f, lBeta = %f, lFeed = %f\n", BEP->bep_LeftEar.erp_Alpha,
BEP->bep_LeftEar.erp_Beta, BEP->bep_LeftEar.erp_Feed));

	Result = SetKnobPart(Snd3D->s3d_AlphaKnob, 0, BEP->bep_LeftEar.erp_Alpha);
	CHECKVAL(Result, "SetKnobPart");
	Result = SetKnobPart(Snd3D->s3d_AlphaKnob, 1, BEP->bep_RightEar.erp_Alpha);
	CHECKVAL(Result, "SetKnobPart");

	Result = SetKnobPart(Snd3D->s3d_BetaKnob, 0, BEP->bep_LeftEar.erp_Beta);
	CHECKVAL(Result, "SetKnobPart");
	Result = SetKnobPart(Snd3D->s3d_BetaKnob, 1, BEP->bep_RightEar.erp_Beta);
	CHECKVAL(Result, "SetKnobPart");

	Result = SetKnobPart(Snd3D->s3d_FeedKnob, 0, BEP->bep_LeftEar.erp_Feed);
	CHECKVAL(Result, "SetKnobPart");
	Result = SetKnobPart(Snd3D->s3d_FeedKnob, 1, BEP->bep_RightEar.erp_Feed);
	CHECKVAL(Result, "SetKnobPart");

cleanup:
	return Result;
}

/************************************************************************
** Read DSP and calculate current Radius, ITD and Time
************************************************************************/
static Err Get3DSoundRadiusAmpTime ( const Sound3D* Snd3D, float32* RadiusPtr,
  int32* PhaseDelayPtr, float32* AmplitudeDiffPtr, int32 *TimePtr)
{
	int32 LeftDelay, RightDelay, IntraAuralDelay;
	uint16 TimeNow;
	float32 Radius, startRadius, endRadius;
	float32 LeftAmp, RightAmp;
	Err Result = 0;
	float32 dTime, DT;	/* time start->now, time start->end */

/*
   Calculate distance to right and left ear. Get DSP stuff as quickly as
   possible.
*/
	TimeNow = GetAudioFrameCount();
	if( Snd3D->s3d_Flags & S3D_F_PAN_DELAY )
	{
		Result = s3dReadLeftRightDrift( Snd3D, &LeftDelay, &RightDelay );
		CHECKVAL(Result, "Get3DSoundRadiusAmpTime: s3dReadLeftRightDrift");
	}
	else
	{
		LeftDelay = 0;
		RightDelay = 0;
	}

	if( Snd3D->s3d_Flags & S3D_F_PAN_AMPLITUDE )
	{
		Result = s3dReadLeftRightAmp( Snd3D, &LeftAmp, &RightAmp );
		CHECKVAL(Result, "Get3DSoundRadiusAmpTime: s3dReadLeftRightAmp");
	}
	else
	{
		LeftAmp = 1.0;
		RightAmp = 1.0;
	}

/*
   Radius = interpolated radius from start to end, given TimeNow.
   ABS(Theta) > PI/2 => radius is negative.
*/
	if (ABS(Snd3D->s3d_LastEnd.pp4d_Theta) > M_PI/2)
	{
		endRadius = -(Snd3D->s3d_LastEnd.pp4d_Radius);
	}
	else
	{
		endRadius = Snd3D->s3d_LastEnd.pp4d_Radius;
	}

	if (ABS(Snd3D->s3d_LastStart.pp4d_Theta) > M_PI/2)
	{
		startRadius = -(Snd3D->s3d_LastStart.pp4d_Radius);
	}
	else
	{
		startRadius = Snd3D->s3d_LastStart.pp4d_Radius;
	}

	if (endRadius == startRadius) Radius = startRadius;
	else
	{
		/* dTime = delta(now - start), how far we _really_ are */
		dTime = (float32)((TimeNow -
		  (uint16)Snd3D->s3d_LastStart.pp4d_Time) & 0xFFFF);

		/* DT = delta(end - start), how far we wish to be */
		DT = (float32)(((uint16) Snd3D->s3d_LastEnd.pp4d_Time -
		  (uint16) Snd3D->s3d_LastStart.pp4d_Time) & 0xFFFF);

DBUG2(("dTime: %f, DT: %f, lastr1: %i, lastr2: %i\n", dTime, DT, startRadius,
  endRadius));

		if (DT==0) Radius = endRadius;
		else Radius = startRadius + ((endRadius - startRadius) * dTime / DT);
	}

/* Calculate IntraAuralDelay */
	IntraAuralDelay = RightDelay - LeftDelay;

/* Set all pointers to calculated values */
	if(RadiusPtr) *RadiusPtr = Radius;
	if(PhaseDelayPtr) *PhaseDelayPtr = IntraAuralDelay;
	if(AmplitudeDiffPtr) *AmplitudeDiffPtr = RightAmp - LeftAmp;
	if(TimePtr) *TimePtr = (int32)TimeNow;

DBUG2(("Get3DSoundRadiusTime: Radius = %i, PhaseDelay = %i\n", Radius, IntraAuralDelay));

/* Check to see if ears are getting split apart. */
	IntraAuralDelay = ABS( IntraAuralDelay );
	if( IntraAuralDelay > S3D_EAR_TO_EAR )
	{
		DBUG(("Get3DSoundRadiusTime: IntraAuralDelay = %i\n", IntraAuralDelay ));
		Result = ML_ERR_HEAD_EXPLOSION;
	}

cleanup:
	return Result;
}

/***********************************************************************/
/*
|||	AUTODOC -public -class libmusic -group Sound3D -name Get3DSoundPos
|||	Calculate the instantaneous position of a 3D Sound at the time of the
|||	call.
|||
|||	  Synopsis
|||
|||	    Err Get3DSoundPos( const Sound3D* a3DSound, PolarPosition4D* Pos4D )
|||
|||	  Description
|||
|||	    This function calculates the current position of a 3D Sound from
|||	    the values of the cues that the sound is using for spatialization
|||	    (specified when the sound was created using Create3DSound()).  The
|||	    current audio frame count is retrieved, and the sound's polar
|||	    position calculated.
|||
|||	    Obviously, if some cues are not being used, corresponding position
|||	    information will be meaningless.  For example, if neither
|||	    S3D_F_PAN_DELAY or S3D_F_PAN_AMPLITUDE were specified when the
|||	    sound was created, there is no way to derive the sound's azimuth
|||	    angle.
|||
|||	    The radius is derived from a linear interpolation between
|||	    the last-used start and end positions.
|||
|||	  Arguments
|||
|||	    a3DSound
|||	        a pointer to a Sound3D structure, previously set using a call
|||	        to Create3DSound().
|||
|||	    Pos4D
|||	        a pointer to a PolarPosition4D structure to be filled in by
|||	        this function with the calculated current position and time
|||	        of the sound. See <audio/sound3d.h>.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if successful or an error code (a negative
|||	    value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V30.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/sound3d.h>, libmusic.a, libspmath.a, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    Create3DSound(), Delete3DSound(), Start3DSound(), Stop3DSound(),
|||	    Move3DSound(), Move3DSoundTo(), Get3DSoundInstrument(),
|||	    Get3DSoundParms()
*/

Err Get3DSoundPos( const Sound3D* Snd3D, PolarPosition4D* Pos4D )
{
	int32 PhaseDelay;
	float32 AmpDifferential;
	float32 q, w;
	Err Result;

	Result = Get3DSoundRadiusAmpTime( Snd3D, &Pos4D->pp4d_Radius, &PhaseDelay,
	  &AmpDifferential, &Pos4D->pp4d_Time );
	CHECKVAL(Result, "Get3DSoundRadiusAmpTime");

	if( Snd3D->s3d_Flags & S3D_F_PAN_DELAY )
	{
		q = (float32)(PhaseDelay) / S3D_EAR_TO_EAR;
	}
	else if( Snd3D->s3d_Flags & S3D_F_PAN_AMPLITUDE )
	{
		q = AmpDifferential / S3D_MAX_AMP_DIFFERENTIAL;
	}
	else q = 0.0;  /* no panning */

	CLIPTO(q, -1, 1);
	w = sqrtf(1.0 - (q*q));
	if (Pos4D->pp4d_Radius < 0.0) w = -w;

DBUG2(("Get3DSoundPos: q = %f, w = %f\n", q, w));

	Pos4D->pp4d_Theta = s3dNormalizeAngle(atan2f(q, w));

DBUG2(("Get3DSoundPos: Theta %f\n", Pos4D->pp4d_Theta));

	Pos4D->pp4d_Radius = ABS(Pos4D->pp4d_Radius);

	Pos4D->pp4d_Phi = 0.0;

cleanup:
	return Result;
}

/***********************************************************************/
/*
|||	AUTODOC -public -class libmusic -group Sound3D -name Get3DSoundParms
|||	Return application-dependent cue parameters.
|||
|||	  Synopsis
|||
|||	    Err Get3DSoundParms( const Sound3D* a3DSound, Sound3DParms* Snd3DParms,
|||	                         uint32 s3dParmsSize )
|||
|||	  Description
|||
|||	    Some spatialization cues are implemented by the 3D Sound.  Others
|||	    rely on the application to implement.  For example, doppler shifting
|||	    may be implemented by having the application modify frequency
|||	    components of the source sound instrument, or various modifications
|||	    to the reverberation characteristics of the sound based on the
|||	    perceived distance from the observer to the sound may be implemented
|||	    by a reverb/mix patch in the application.
|||
|||	    The application needs information from the 3D Sound in order to
|||	    implement these cues.  This function is used to get that information,
|||	    by filling in the Sound3DParms structure pointed to.  The values
|||	    are recalculated on calls to Move3DSound(), Move3DSoundTo() and
|||	    Start3DSound(), so generally this function will be called immediately
|||	    following one of those.
|||
|||	    The Sound3DParms structure contains the following fields:
|||
|||	    float32 s3dp_Doppler
|||	        a frequency scaling factor.  1.0 implies no change in frequency
|||	        (a relative velocity of 0).  Less than 1.0 implies a lower
|||	        frequency (the object is moving away).  Greater than 1.0 implies
|||	        a higher frequency (the object is moving toward the observer).
|||	        Generally, multiplying the "Frequency" or "SampleRate" of the
|||	        source instrument by this number will give a good approximation
|||	        of doppler shift.
|||
|||	    float32 s3dp_DistanceFactor
|||	        ranges from 1.0 when the sound is located directly beside or
|||	        inside the head, to 0.0 when the sound is maximally distant.
|||	        Intermediate distances are calculated according to the distance/
|||	        amplitude cue selected with Create3DSound(): either proportional
|||	        to 1/distance (using S3D_F_DISTANCE_AMPLITUDE_SQUARE) or to 1/(distance**3/2)
|||	        (using S3D_F_DISTANCE_AMPLITUDE_SONE).  Using this number to control
|||	        the gain of the sound in the output mix provides amplitude cues;
|||	        it can also be used to calculate the wet/dry mix in a reverberant
|||	        space.
|||
|||	    If the s3dParmsSize argument is less than the size of a Sound3DParms
|||	    structure, only the first s3dParmsSize bytes are copied.
|||
|||	  Arguments
|||
|||	    a3DSound
|||	        a pointer to a Sound3D structure, previously set using a call
|||	        to Create3DSound().
|||
|||	    Snd3DParms
|||	        a pointer to a Sound3DParms structure to be filled in by
|||	        this function.
|||
|||	    s3dParmsSize
|||	        the size of the Sound3DParms structure to be filled in.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if successful or an error code (a negative
|||	    value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V30.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/sound3d.h>, libmusic.a, libspmath.a, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    Create3DSound(), Delete3DSound(), Start3DSound(), Stop3DSound(),
|||	    Move3DSound(), Move3DSoundTo(), Get3DSoundInstrument(),
|||	    Get3DSoundPos()
*/

Err Get3DSoundParms( const Sound3D* Snd3D, Sound3DParms* Snd3DParms, uint32 s3dParmsSize )
{
	if (s3dParmsSize <= sizeof(Sound3DParms))
	{
		memcpy( Snd3DParms, &(Snd3D->s3d_Parms), s3dParmsSize );
	}
	else
	{
		memcpy( Snd3DParms, &(Snd3D->s3d_Parms), sizeof(Sound3DParms) );
	}

	return 0;
}
