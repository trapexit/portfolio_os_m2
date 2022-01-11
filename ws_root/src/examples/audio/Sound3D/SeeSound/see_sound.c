/***************************************************************
**
** @(#) see_sound.c 96/08/27 1.12
**
** Demo sound3d library: ta_bee3d + graphics
**
** By:  Robert Marsanyi
**      Graphics code shamelessly lifted from newview.c
**
** Copyright (c) 1996, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name SeeSound
|||	Sounds in an audio-visual space, using 3DSound and Framework/Pipeline.
|||
|||	  Format
|||
|||	    seesound [reverb [cuelist]]
|||
|||	  Description
|||
|||	    This program positions three objects in a three-dimensional space,
|||	    and allows the user to move about in the space using the control
|||	    pad.  The space is rendered graphically using the Framework/Pipeline
|||	    API, and sonically using the 3DSound API.  By varying the command
|||	    line arguments, the user can hear the relative effect of the various
|||	    3DSound cues on the three-dimensional illusion.
|||
|||	    To move around using the control pad:
|||
|||	    Up
|||	        moves you forward
|||	    Down
|||	        moves you backward
|||	    Left
|||	        rotates you anticlockwise
|||	    Right
|||	        rotates you clockwise
|||	    A
|||	        repositions you under the "bee".
|||
|||	    Two of the objects are stationary: a large rectangular block with
|||	    an associated looped "clonking" sample and a large cylinder, with
|||	    an associated synthetic telephone sound.  The third, a synthetic bee
|||	    represented as a small cone, does a random walk
|||	    confined to a predetermined distance from its initial location.
|||
|||	    All the geometry for this example is done with Pipeline matrix
|||	    operations, and then mapped into the coordinate system of the
|||	    3DSound library.
|||
|||	    Note: to use this example, you first need to install the General MIDI
|||	    Sample Library in /remote/Samples, and then run the shell script
|||	    "makepatch.script" in the directory Examples/Audio/Sound3D to build the
|||	    set of patches loaded by the program.
|||
|||	  Arguments
|||
|||	    reverb
|||	        A floating point number from 0.0 to 1.0, determining the global
|||	        reverberation amount in the final mix.  The amount of sound
|||	        sent to the reverb is independently calculated for each sound,
|||	        based on the distance of the sound from the observer, and the
|||	        overall wet/dry mix is controlled with this parameter.  By
|||	        default, the value is 0.05.  The higher the number, the harder
|||	        it becomes to accurately determine the position of a distant
|||	        sound.
|||
|||	    cuelist
|||	        a string of digits selected from the list below.  Including
|||	        a digit in the string enables the corresponding cue.  The default
|||	        cuelist is equivalent to the string 1347.
|||
|||	    1
|||	        Interaural Time Delay
|||	    2
|||	        Interaural Amplitude Difference
|||	    3
|||	        Pseudo-HRTF Filter
|||	    4
|||	        Distance-squared rule for amplitude attenuation
|||	    5
|||	        Distance-cubed rule for amplitude attenuation
|||	    7
|||	        Doppler shift.  Note that the dopplering sounds a little
|||	        strange, due to the lack of momentum in movement - effectively
|||	        velocity changes are instantaneous, and hence so are frequency
|||	        changes.
|||
|||	  Associated Files
|||
|||	    Sound3D/patches/bee3d.mp, Sound3D/patches/bee3dspace.mp,
|||	    Sound3D/patches/clonk.mp, Sound3D/patches/phone.mp,
|||	    Sound3D/makepatch.script
|||
|||	  Location
|||
|||	    Examples/Audio/Sound3D/SeeSound
|||
|||	  See Also
|||
|||	    ta_bee3d(@)
|||
**/

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <audio/audio.h>
#include <audio/music.h>
#include <audio/parse_aiff.h>
#include <misc/event.h>
#include <graphics/fw.h>
#include <graphics/gfxutils/putils.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */
#define	DBUG2(x) /* PRT(x) */
#define MAX_WETNESS (0.25)	/* default maximum amount of reverb */
#define TIME_INCR (240/60)	/* delta(t) for velocity calculations, in audio ticks */

#define NUMBER_OF_BEES (1)
#define NUMBER_OF_SOUNDS (NUMBER_OF_BEES + 2)
#define BEE_MAX_RADIUS (10 * S3D_DISTANCE_TO_EAR)
#define BEE_CHANGE_DIRECTION_THRESHOLD (0.8)
#define BEE_MAX_DELTA_RADIUS (S3D_DISTANCE_TO_EAR)
#define BEE_MAX_DELTA_THETA (M_PI / 6.0)
#define OBSERVER_ANGLE_INCR (M_PI / 120.0)
#define OBSERVER_RADIUS_INCR (4 * S3D_DISTANCE_TO_EAR)

#define CUE_MASK (S3D_F_PAN_FILTER \
  | S3D_F_PAN_DELAY \
  | S3D_F_OUTPUT_HEADPHONES \
  | S3D_F_DISTANCE_AMPLITUDE_SQUARE \
  | S3D_F_DOPPLER)

#ifndef SIGNEXTEND
#define SIGNEXTEND(n) (((n) & 0x8000) ? (n)|0xFFFF0000 : (n))
#endif

/* Graphics Constants */
#define	kRotateIncrement	(1.0)

#define	XROT_START		(10*kRotateIncrement)
#define YROT_START		(10*kRotateIncrement)
#define	ZROT_START		(10*kRotateIncrement)
#define ZPOS_START		(-250 * zPosIncInc)
#define PAN_START		(0.0)
#define CAM_DISTANCE	(6.0)
#define NOMINAL_USECS	(12000.0)	/* Video Frame */

#define	CAM_RotX		0x01
#define	CAM_RotY		0x02
#define	CAM_RotZ		0x04
#define	CAM_Zoom		0x08
#define	CAM_PanX		0x10
#define	CAM_PanY		0x20

#define RadiansToDegrees(a) (((a) * 180.0) / PI)
#define FramesToMeters(a) (((a) * 340.0) / 44100.0)
#define MetersToFrames(a) (((a) * 44100.0) / 340.0)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		goto cleanup; \
	}

/* Structure to keep track of the source sound items */
typedef struct beeSound
{
	Item bee_Source;
	Sound3D* bee_Sound3DPtr;
	Item bee_FreqKnob;
	Item bee_NominalFrequency;
	PolarPosition4D bee_Target;
	Character* bee_Char;
} beeSound;

/* Structure to keep track of environmental items */
typedef struct Sound3DSpace
{
	Item bees_Template;
	Item bees_ClonkTemplate;
	Item bees_PhoneTemplate;
	Item bees_OutputTemplate;
	Item bees_OutputInst;
	Item bees_LineOut;
	Item bees_MixerTemplate;
	Item bees_Mixer;
	Item bees_GainKnob;
	Item bees_PresendKnob;
} Sound3DSpace;

typedef struct CartesianCoords
{
	float32 xyz_X;
	float32 xyz_Y;
	float32 xyz_Z;
	float32 xyz_Time;
} CartesianCoords;

/* Globals */
float32 beeGlobalReverb;

/* Graphics Globals */
static Character* theScene;
static Err        theErr;
static GP*        gp;
static SDF*       gTheSDF = NULL;
static int        cull_mode = GP_Back;
Character         *theSmallCube;
TimeVal           gStarttime, gStoptime;

/* Functions */
uint32 beeSetCues( char* cueList );
Sound3DSpace *CreateSoundSpace( void );
int32 DeleteSoundSpace( Sound3DSpace *Snd3DSpace );
Err beeLoadSourcePatch(beeSound* sound, uint32 beeCues, char* patchName );
Err beeInit(beeSound sounds[], Sound3DSpace** beespaceHandle, uint32 beeCues);
void beeTerm(beeSound sounds[], Sound3DSpace* beespace);
Err beeAnimate(beeSound sounds[], Sound3DSpace* beeSpace);
Err beeSteer(beeSound sounds[], Sound3DSpace* beeSpace);

/* Graphics functions */
void	InitM2Graphics (int* argc, char* argv[]);
bool	SetupScene (Character** theScenePtrPtr);
void	TermScene (SDF* gTheSDF);

int32 main( int argc, char *argv[])
{

	int32 Result, i;
	beeSound beeSounds[NUMBER_OF_SOUNDS];
	Sound3DSpace* beespace = NULL;
	uint32 beeCues;

	PRT(("Usage: see_sound [reverb [cuelist]] [InitGP parms]\n"));
	PRT(("reverb: 0.0-1.0, global reverberation level\n"));
	PRT(("cuelist: abc.., where a,b,c, and so on are digits from the\n"));
	PRT(("  list below.\n"));
	PRT(("  1: Interaural Time Delay\n"));
	PRT(("  2: Interaural Amplitude Difference\n"));
	PRT(("  3: Pseudo-HRTF Filter\n"));
	PRT(("  4: Distance-squared rule for amplitude attenuation\n"));
	PRT(("  5: Distance-cubed rule for amplitude attenuation\n"));
	PRT(("  7: Doppler shift\n"));
	PRT(("There are three sounds in the space: a moving bee, a clanking\n"));
	PRT(("machine and a telephone.  Use left/right to turn, up to move\n"));
	PRT(("forward, down to move backward, A to position back under the bee.\n"));

	/* Retrieve command line arguments */
	beeGlobalReverb = (argc > 1) ? strtof( argv[1], NULL ) : MAX_WETNESS;
	beeCues = (argc > 2) ? beeSetCues( argv[2] ) : CUE_MASK ;
DBUG(("Cues: %x\n",beeCues));

	if ((Result = InitEventUtility (1, 0, TRUE)) < 0)
	{
		PrintError (NULL, "init event utility", NULL, Result);
		goto cleanup;
	}

	for (i=0;i<NUMBER_OF_SOUNDS;i++)
		beeSounds[i].bee_Sound3DPtr = NULL;

	InitM2Graphics(&argc, argv);
	DBUG(("Inited graphics\n"));

	Result = beeInit(beeSounds, &beespace, beeCues);
	CHECKRESULT(Result, "beeInit");

	Result = beeAnimate(beeSounds, beespace);
	CHECKRESULT(Result, "beeAnimate");

cleanup:
	beeTerm(beeSounds, beespace);
	TermScene (gTheSDF);
	KillEventUtility();
	printf("see_sound done\n");

	return((int32) Result);

}

/****************************************************************/
void CheckResources( void )
{
	AudioResourceInfo* info;

	/* check resource usage */
	if ((info = malloc(sizeof(AudioResourceInfo))) != NULL)
	{
		GetAudioResourceInfo (info, sizeof(AudioResourceInfo), AF_RESOURCE_TYPE_TICKS);
		DBUG(("Free ticks: %i\n", info->rinfo_Free));
		GetAudioResourceInfo (info, sizeof(AudioResourceInfo), AF_RESOURCE_TYPE_CODE_MEM);
		DBUG(("Free code: %i\n", info->rinfo_Free));
		GetAudioResourceInfo (info, sizeof(AudioResourceInfo), AF_RESOURCE_TYPE_DATA_MEM);
		DBUG(("Free data: %i\n", info->rinfo_Free));
		GetAudioResourceInfo (info, sizeof(AudioResourceInfo), AF_RESOURCE_TYPE_FIFOS);
		DBUG(("Free fifos: %i\n", info->rinfo_Free));
		free(info);
	}
}

/****************************************************************/
uint32 beeSetCues( char* cueList )
{
	int32 i, result=0;
	char digit;

	for (i=0;i<strlen(cueList);i++)
	{
		digit = *(cueList+i);
		result |= 1<<(atoi(&digit)-1);
	}

	return (result);
}

/****************************************************************/
Sound3DSpace *CreateSoundSpace( void )
/* Allocate in this routine in case size changes. */
{
	Sound3DSpace *Snd3DSpace;
	int32 Result;

	Snd3DSpace = (Sound3DSpace *) malloc(sizeof(Sound3DSpace));
	if (Snd3DSpace != NULL)
	{
		Snd3DSpace->bees_OutputTemplate = LoadScoreTemplate("../bee3dspace.patch");
		CHECKRESULT(Snd3DSpace->bees_OutputTemplate, "LoadScoreTemplate");

		Snd3DSpace->bees_OutputInst = CreateInstrument(Snd3DSpace->bees_OutputTemplate, NULL);
		CHECKRESULT(Snd3DSpace->bees_OutputInst, "CreateInstrument");

/* Load the output instrument */
		Snd3DSpace->bees_LineOut = LoadInstrument("line_out.dsp", 0, 100);
		CHECKRESULT(Snd3DSpace->bees_LineOut, "LoadInstrument");

/* Connect the space to the output instrument */
		Result = ConnectInstrumentParts(
		   Snd3DSpace->bees_OutputInst, "Output", 0,
		   Snd3DSpace->bees_LineOut, "Input", 0);
		CHECKRESULT(Result, "ConnectInstrumentParts");
		Result = ConnectInstrumentParts(
		   Snd3DSpace->bees_OutputInst, "Output", 1,
		   Snd3DSpace->bees_LineOut, "Input", 1);
		CHECKRESULT(Result, "ConnectInstrumentParts");

		Snd3DSpace->bees_GainKnob = CreateKnob(
		  Snd3DSpace->bees_OutputInst, "Gain", 0 );
		CHECKRESULT(Snd3DSpace->bees_GainKnob, "CreateKnob");

		Snd3DSpace->bees_PresendKnob = CreateKnob(
		  Snd3DSpace->bees_OutputInst, "Presend", 0 );
		CHECKRESULT(Snd3DSpace->bees_GainKnob, "CreateKnob");

		return Snd3DSpace;
	}
	else
		return NULL;

cleanup:
	DeleteSoundSpace( Snd3DSpace );
	TOUCH(Result);

	return NULL;
}

/****************************************************************/
int32 DeleteSoundSpace( Sound3DSpace *Snd3DSpace )
{
	int32 Result=0;

	if (Snd3DSpace)
	{
		UnloadScoreTemplate( Snd3DSpace->bees_OutputTemplate );
		UnloadInstrument( Snd3DSpace->bees_LineOut );

		free (Snd3DSpace);
	}

	return Result;
}

/****************************************************************/
Err beeMakeSource(beeSound* sound, uint32 beeCues, Item beeTemplate )
{
	Err Result;
	float32 NominalFreq;
	Item s3dInst;
	TagArg Tags[] = { { S3D_TAG_FLAGS }, TAG_END };

	/* Allocate and set up the 3D context */
	Tags[0].ta_Arg = (TagData) beeCues;
	Result = Create3DSound( &(sound->bee_Sound3DPtr), Tags );
	CHECKRESULT(Result, "Create3DSound");

	/* Set up the source instrument */
	sound->bee_Source = CreateInstrument(beeTemplate, 0);
	CHECKRESULT(sound->bee_Source, "CreateInstrument");

DBUG(("Made source sound, item %i\n", sound->bee_Source));

	/* Connect the source to the 3D instrument context*/
	s3dInst = Get3DSoundInstrument( sound->bee_Sound3DPtr );
	CHECKRESULT(s3dInst, "Get3DSoundInstrument");

	Result = ConnectInstruments(sound->bee_Source, "Output", s3dInst,
	  "Input");
	CHECKRESULT(Result, "ConnectInstrument");

	/* Connect the Frequency knob, for doppler */
	sound->bee_FreqKnob = CreateKnob(sound->bee_Source, "SampleRate",
	  NULL);
	CHECKRESULT(sound->bee_FreqKnob,
	  "CreateKnob: can't connect to doppler");

	/* Read the default knob value, assume that's nominal */
	Result = ReadKnob(sound->bee_FreqKnob, &NominalFreq);
	CHECKRESULT(Result, "ReadKnob");
	sound->bee_NominalFrequency = NominalFreq;

cleanup:

DBUG(("New 3DSound at 0x%x\n", sound->bee_Sound3DPtr));

	return (Result);
}

/****************************************************************/
void InitM2Graphics (int* argc, char* argv[])
{
	theErr = 0;
	Gfx_Init();
	gp = InitGP (argc, argv);
}

/****************************************************************/
static Err SceneShowAll (Character* m_Scene)
{
	Camera*		cam = Scene_GetCamera (m_Scene);
	Point3		ctr, loc;
	Box3		bound, b;
	gfloat		bound_rad, cam_dist, fov_angle;
	Vector3		vec;
	static bool	firstTime = TRUE;

	/* get static bounding box */
	if (Scene_GetStatic (m_Scene))
		Char_GetBound (Scene_GetStatic (m_Scene), &bound, CHAR_Local);
	else Box3_Set (&bound, 0, 0, 0, 0, 0, 0);

	/* get dynamic bounding box */
	if (Scene_GetDynamic (m_Scene))
	{
		Char_GetBound (Scene_GetDynamic(m_Scene), &b, CHAR_Local);
		/* combine static & dynamic */
		if (Box3_IsEmpty (&bound)) bound = b;
		else Box3_ExtendBox (&bound, &b);
	}

	/* center of bounding volume */
	Box3_Center (&bound, &ctr);

	/* get maximum dimension and use it to position camera */
	Vec3_Set (&vec, bound.max.x - ctr.x, bound.max.y - ctr.y,
				bound.max.z - ctr.z);
	bound_rad = Vec3_Length (&vec);
	cam_dist = CAM_DISTANCE * bound_rad;

	fov_angle = 2.0 * atan2f (bound_rad , cam_dist);
	fov_angle = RadiansToDegrees (fov_angle);

	Vec3_Set (&loc, ctr.x, ctr.y, ctr.z - cam_dist); /* look "north" */

	/* adjust the clipping planes appropriately */
	Cam_SetFOV (cam, fov_angle);

	Cam_SetHither (cam, 0.01 * cam_dist);
	Cam_SetYon (cam, 10 * cam_dist);

	if (firstTime)
	{
		printf ("Object Center = (%f, %f, %f), Radius = %f\n",
				loc.x, ctr.y, ctr.z, bound_rad);
		printf ("       FOV = %f, Hither = %f, Yon = %f\n", Cam_GetFOV (cam),
				Cam_GetHither (cam), Cam_GetYon (cam));
		firstTime = FALSE;
	}

	/* reset the camera location to "ctr" and lookat to "loc" */
	Char_Reset (cam);
	/* Char_Move (cam, ctr.x, ctr.y, ctr.z); */
	Char_LookAt (cam, &loc, 0);

	return GFX_OK;
}

/****************************************************************/
bool SetupScene (Character** theScenePtrPtr)
{
	Character*	theScene;
	bool		retVal;
	Character*	theObj = NULL;
	Character*	theSDFObj;
	const char*	sdfFilename = "see_sound.bdf";

	/* Create a scene, into which the objects will be placed. */
	theScene = Scene_Create();
	if (theScene == NULL) goto SetupFailed;

	/* Load the object(s) from the SDF file. */
	gTheSDF = SDF_Open (sdfFilename);
	if (gTheSDF == NULL) goto SetupFailed;

	/* Get the group object from the SDF file. */
	theSDFObj = SDF_FindObj (gTheSDF, "character", "All");
	if (theSDFObj == NULL) goto SetupFailed;
	Obj_Assign (&theObj, theSDFObj);

	/* Add the group to the scene. */
	Scene_SetStatic (theScene, theObj);

	/* Prepare the scene for rendering */
	SceneShowAll (theScene);

	Scene_SetVisibility (theScene, TRUE);
	Scene_SetAutoClip (theScene, TRUE);
	Scene_SetAutoAdjust (theScene, TRUE);

	/* Set up the final bits of the pipeline for the framework,	*/
	/* then display the scene.									*/
	GP_SetHiddenSurf (gp, GP_ZBuffer);
	GP_SetCullFaces (gp, cull_mode);
	retVal = TRUE;
	goto Exit;

SetupFailed:
	if (gTheSDF != NULL) TermScene (gTheSDF);
	retVal = FALSE;

Exit:
	*theScenePtrPtr = theScene;
	return retVal;
}

/****************************************************************/
Err beeInit(beeSound sounds[], Sound3DSpace** beespaceHandle, uint32 beeCues)
{
	Err Result;
	Item Inst3D, SpaceInst, beeTemplate;
	int32 i;
	Character* group;

	CheckResources();

	/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

	/* Allocate and set up the space */
	if ((*beespaceHandle = CreateSoundSpace()) == NULL)
	{
		ERR(("Couldn't allocate 3D sound space!\n"));
		return(-1);
	}

	CheckResources();

	SpaceInst = (*beespaceHandle)->bees_OutputInst;

	/* Allocate and set up the sounds */
	beeTemplate = LoadScoreTemplate("../bee3d.patch");
	CHECKRESULT(beeTemplate, "LoadScoreTemplate");

	/* Save the source template info so we can destroy it on exit */
	(*beespaceHandle)->bees_Template = beeTemplate;

	for (i=0;i<NUMBER_OF_BEES;i++)
	{
		Result = beeMakeSource( &(sounds[i]), beeCues, beeTemplate );
		CHECKRESULT(Result, "beeMakeSource");

		Inst3D = Get3DSoundInstrument( sounds[i].bee_Sound3DPtr );
		CHECKRESULT(Inst3D, "Get3DSoundInstrument");

		Result = ConnectInstrumentParts(Inst3D, "Output", 0,
		  SpaceInst, "Input", i*2);
		CHECKRESULT(Result, "ConnectInstrumentParts");
		Result = ConnectInstrumentParts(Inst3D, "Output", 1,
		  SpaceInst, "Input", (i*2)+1);
		CHECKRESULT(Result, "ConnectInstrumentParts");

		CheckResources();
	}

	/* Other sounds */
	beeTemplate = LoadScoreTemplate("../clonk.patch");
	CHECKRESULT(beeTemplate, "LoadScoreTemplate");

	(*beespaceHandle)->bees_ClonkTemplate = beeTemplate;
	Result = beeMakeSource( &(sounds[NUMBER_OF_BEES]), beeCues,
	  beeTemplate );
	CHECKRESULT(Result, "beeMakeSource");

	Inst3D = Get3DSoundInstrument( sounds[NUMBER_OF_BEES].bee_Sound3DPtr );
	CHECKRESULT(Inst3D, "Get3DSoundInstrument");

	Result = ConnectInstrumentParts(Inst3D, "Output", 0,
	  SpaceInst, "Input", NUMBER_OF_BEES*2);
	CHECKRESULT(Result, "ConnectInstrumentParts");
	Result = ConnectInstrumentParts(Inst3D, "Output", 1,
	  SpaceInst, "Input", (NUMBER_OF_BEES*2)+1);
	CHECKRESULT(Result, "ConnectInstrumentParts");

	CheckResources();

	beeTemplate = LoadScoreTemplate("../phone.patch");
	CHECKRESULT(beeTemplate, "LoadScoreTemplate");

	(*beespaceHandle)->bees_PhoneTemplate = beeTemplate;
	Result = beeMakeSource( &(sounds[NUMBER_OF_BEES+1]), beeCues,
	  beeTemplate );
	CHECKRESULT(Result, "beeMakeSource");

	Inst3D = Get3DSoundInstrument( sounds[NUMBER_OF_BEES+1].bee_Sound3DPtr );
	CHECKRESULT(Inst3D, "Get3DSoundInstrument");

	Result = ConnectInstrumentParts(Inst3D, "Output", 0,
	  SpaceInst, "Input", (NUMBER_OF_BEES+1)*2);
	CHECKRESULT(Result, "ConnectInstrumentParts");
	Result = ConnectInstrumentParts(Inst3D, "Output", 1,
	  SpaceInst, "Input", ((NUMBER_OF_BEES+1)*2)+1);
	CHECKRESULT(Result, "ConnectInstrumentParts");

	CheckResources();

	if (!SetupScene (&theScene))
	{
		ERR(("Couldn't set up the scene!\n"));
		Result = -1;  /* force an error higher up */
	}

	/* copy the character info into the sounds array */
	group = Scene_GetStatic(theScene);
	for (i=0; i<NUMBER_OF_SOUNDS; i++)
	{
		sounds[i].bee_Char = Char_GetAt(group, i);
	}

cleanup:
	return(Result);
}

/****************************************************************/
void TermScene (SDF* gTheSDF)
{
	if (gTheSDF) SDF_Close (gTheSDF);
}

/****************************************************************/
void beeTerm(beeSound sounds[], Sound3DSpace* beespace)
{
	int32 i;

	UnloadScoreTemplate(beespace->bees_Template);
	UnloadScoreTemplate(beespace->bees_ClonkTemplate);
	UnloadScoreTemplate(beespace->bees_PhoneTemplate);

	for (i=0;i<NUMBER_OF_SOUNDS;i++)
	{


DBUG(("Deleting 3DSound at 0x%x\n", sounds[i].bee_Sound3DPtr));

		Delete3DSound(&(sounds[i].bee_Sound3DPtr));
	}

DBUG(("Deleting beespace 0x%x\n", beespace));

	DeleteSoundSpace(beespace);
	CloseAudioFolio();
	return;
}

/****************************************************************/
void PolarToXYZ( PolarPosition4D* polar, CartesianCoords* cart )
{
	cart->xyz_X = polar->pp4d_Radius * sinf(polar->pp4d_Theta);
	cart->xyz_Y = polar->pp4d_Radius * cosf(polar->pp4d_Theta);
	cart->xyz_Z = 0;
	cart->xyz_Time = polar->pp4d_Time;

	return;
}

/****************************************************************/
void XYZToPolar( CartesianCoords* cart, PolarPosition4D* polar )
{
	polar->pp4d_Radius = sqrtf(cart->xyz_X * cart->xyz_X +
	  cart->xyz_Y * cart->xyz_Y);
	polar->pp4d_Theta = atan2f(cart->xyz_X, cart->xyz_Y);
	polar->pp4d_Phi = 0.0;
	polar->pp4d_Time = cart->xyz_Time;

	return;
}

/****************************************************************/
void TestCoordConversion( void )
{
	PolarPosition4D polar, pback;
	CartesianCoords cart;
	int32 i;

	polar.pp4d_Radius = 1000;

	for (i=0;i<=12;i++)
	{
		polar.pp4d_Theta = s3dNormalizeAngle( i * S3D_FULLCIRCLE / 12 );
		PolarToXYZ( &polar, &cart );
		XYZToPolar( &cart, &pback );
		PRT(("%2i2PI/6: %i  %f  %f  %f  %i  %f\n",
		  i,
		  polar.pp4d_Radius, polar.pp4d_Theta,
		  cart.xyz_X, cart.xyz_Y,
		  pback.pp4d_Radius, pback.pp4d_Theta));
	}

	return;
}

/*******************************************************************/
float32 Choose ( float32 range )
{
        float32 val, r;

        r = (float32)(rand() & 0x0000FFFF);
        val = (r / 65536.0) * range;
        return val;
}

#define wChoose(min, max) (Choose( max - min ) + min)

/****************************************************************/
PolarPosition4D AddPolar(PolarPosition4D p1,
  PolarPosition4D p2, float32 orientation)
{
	PolarPosition4D sumPos;
	CartesianCoords c1, c2, sumPosXYZ;

	PolarToXYZ( &p1, &c1 );
	PolarToXYZ( &p2, &c2 );

	sumPosXYZ.xyz_X = c1.xyz_X + c2.xyz_X;
	sumPosXYZ.xyz_Y = c1.xyz_Y + c2.xyz_Y;

	XYZToPolar( &sumPosXYZ, &sumPos );
	sumPos.pp4d_Theta += orientation;

	return sumPos;
}

/****************************************************************/
PolarPosition4D SubtractPolar(PolarPosition4D p1,
  PolarPosition4D p2, float32 orientation)
{
	PolarPosition4D sumPos;

	p2.pp4d_Radius = -(p2.pp4d_Radius);
	sumPos = AddPolar(p1, p2, orientation);

	return sumPos;
}

/****************************************************************/
static void GraphicsToSound (Point3* xyz, PolarPosition4D* polar)
{
	CartesianCoords cart_xyz;

	cart_xyz.xyz_X = MetersToFrames(xyz->x);
	cart_xyz.xyz_Y = MetersToFrames(-xyz->z);
	cart_xyz.xyz_Z = MetersToFrames(xyz->y);

	XYZToPolar(&cart_xyz, polar);

}

/****************************************************************/
static void SoundToGraphics (PolarPosition4D* polar, Point3* xyz)
{
	CartesianCoords cart_xyz;

	PolarToXYZ(polar, &cart_xyz);
	xyz->x = FramesToMeters(cart_xyz.xyz_X);
	xyz->z = FramesToMeters(-cart_xyz.xyz_Y);
	xyz->y = FramesToMeters(cart_xyz.xyz_Z);
}


/****************************************************************/
static void calcRelativePos (Character *theChar, Character *theCamera,
  Point3* Pos)
{
	Point3 charCtr;
	Transform* camTransform;
	Transform* temp;

	Char_GetCenter(theChar, &charCtr, CHAR_World);
	camTransform = Char_GetTransform(theCamera);
	temp = Trans_Create(NULL);

	Trans_Inverse(temp, camTransform);
	Pt3_Transform(&charCtr, temp);
		/* charCtr is now relative to camera position */

	Pos->x = charCtr.x;
	Pos->y = charCtr.y;
	Pos->z = charCtr.z;

	Trans_Delete(temp);
}

/*
   printScene is a useful debugging routine to show you what you have
   in your graphic scene, and where everything is.  It's commented out
   to avoid compiler warnings about the function not being referenced.
*/

#if 0

/****************************************************************/
static void printScene (Character *m_Scene)
{
	Camera *cam = Scene_GetCamera (m_Scene);
	Point3 center;
	Character* staticChars;
	Character* dynamicChars;
	int32 nobj;
	CharIter iter;
	Character *c;
	Point3 Pos;
	PolarPosition4D polarPos;


	Char_GetCenter(cam, &center, CHAR_World);
	PRT(("Camera: x %f, y %f, z %f\n",
	  center.x, center.y, center.z));

	staticChars = Scene_GetStatic (theScene);
	PRT(("Static:\n"));
	nobj = 0;
	if (staticChars != NULL)
	{
		Char_ForAll(staticChars, &iter, CHAR_BreadthFirst);
		while (c = Char_Next(&iter))
		{
			++nobj;
			calcRelativePos(c, cam, &Pos);
			GraphicsToSound(&Pos, &polarPos);

			PRT(("  Object %i: radius %f, azimuth %f\n",
			  nobj, polarPos.pp4d_Radius, polarPos.pp4d_Theta));
		}
	}

	dynamicChars = Scene_GetDynamic (theScene);
	PRT(("Dynamic:\n"));
	nobj = 0;
	if (dynamicChars != NULL)
	{
		Char_ForAll(dynamicChars, &iter, CHAR_BreadthFirst);
		while (c = Char_Next(&iter))
		{
			++nobj;
			calcRelativePos(c, cam, &Pos);
			GraphicsToSound(&Pos, &polarPos);

			PRT(("  Object %i: radius %f, azimuth %f\n",
			  nobj, polarPos.pp4d_Radius, polarPos.pp4d_Theta));
		}
	}

}

#endif
/***********************************************************************/
Err beeUpdateHost( beeSound* bee3D, Sound3DSpace* beeSpace, int32 soundNum )
{
	Err Result;

	float32 Amplitude, Dry, Wet, Doppler;
	Sound3DParms s3dParms;

/* Do all the host updates that the API doesn't do */

	Result = Get3DSoundParms( bee3D->bee_Sound3DPtr, &s3dParms, sizeof(s3dParms) );
	CHECKRESULT(Result, "Get3DSoundParms");

/* Doppler */

	Doppler = s3dParms.s3dp_Doppler;
	Result = SetKnob(bee3D->bee_FreqKnob, Doppler * bee3D->bee_NominalFrequency);
	CHECKRESULT(Result, "SetKnob Doppler");

/*
   Reverb calculations (from Dodge and Jerse):
   Dry signal attenuates proportionally to distance (1/D).  Wet signal
   attenuates proportionally to the square root of distance (1/sqrt(D)).
   We don't differentiate between local and global reverberation here.
*/

	Amplitude = s3dParms.s3dp_DistanceFactor;
	Dry = (1.0 - beeGlobalReverb) * Amplitude;
	Wet = sqrtf(Amplitude);

	Result = SetKnobPart(beeSpace->bees_PresendKnob, soundNum, Wet);
	CHECKRESULT(Result, "SetKnobPart");

	Result = SetKnobPart(beeSpace->bees_GainKnob, soundNum, Dry);
	CHECKRESULT(Result, "SetKnobPart");

cleanup:
	return (Result);
}

/****************************************************************/
static Err SceneMoveCamera (Character *m_Scene, uint32 inMode,
  gfloat *inValue)
{
	Camera	*cam = Scene_GetCamera (m_Scene);
	Point3	ctr, loc;
	Box3	bound;
	gfloat	max_dim;

#if 0
	if (inMode & CAM_RotX) fprintf (stderr, "CAM_RotX = %g \n\n", inValue[0]);
	if (inMode & CAM_RotY) fprintf (stderr, "CAM_RotY = %g \n\n", inValue[1]);
	if (inMode & CAM_RotZ) fprintf (stderr, "CAM_RotZ = %g \n\n", inValue[2]);
	if (inMode & CAM_Zoom) fprintf (stderr, "CAM_Zoom = %g \n\n", inValue[3]);
	if (inMode & CAM_PanX) fprintf (stderr, "CAM_PanX = %g \n\n", inValue[4]);
	if (inMode & CAM_PanY) fprintf (stderr, "CAM_PanY = %g \n\n", inValue[5]);
#endif

	/* Dynamically update the World bounding box	*/
	/* Note : later use look at object bounding box	*/
	Scene_GetBound (m_Scene, &bound);

	/* center of bounding volume */
	Box3_Center (&bound, &ctr);

	/* Get maximum dimension and use it to position camera. */
	max_dim = Box3_Height (&bound);
	if (Box3_Width (&bound) > max_dim) max_dim = Box3_Width (&bound);
	if (Box3_Depth (&bound) > max_dim) max_dim = Box3_Depth (&bound);
	Vec3_Set (&loc, ctr.x, ctr.y, ctr.z + 2.0 * max_dim);

#if	0
	/* Adjust the clipping planes appropriately. */
	Cam_SetHither (cam, 0.1 * Pt3_Distance ( &loc, &bound.max));
	Cam_SetYon (cam, 1.5 * Pt3_Distance ( &loc, &bound.min));
#endif

	/* rotate camera */
	if (inMode & CAM_RotX) Char_Rotate (cam, TRANS_XAxis, inValue[0]);
	if (inMode & CAM_RotY) Char_Rotate (cam, TRANS_YAxis, inValue[1]);
	if (inMode & CAM_RotZ) Char_Rotate (cam, TRANS_ZAxis, inValue[2]);

	/* zoom camera */
	if (inMode & CAM_Zoom) Char_Move (cam, 0, 0, inValue[3]);

	/* pan camera */
	if (inMode & CAM_PanX) Char_Turn (cam, TRANS_XAxis, inValue[4]);
	if (inMode & CAM_PanY) Char_Turn (cam, TRANS_YAxis, inValue[5]);

	/* print the translations for all the components in the scene */
	/* printScene(m_Scene); */

	return GFX_OK;
}

/****************************************************************/
Err beeMoveSound(Camera* cam, beeSound* sound, Sound3DSpace* beeSpace, int32 i)
{
	Point3 Pos;
	Character* c;
	PolarPosition4D relativeTarget;
	Err Result;

	c = sound->bee_Char;
	calcRelativePos(c, cam, &Pos);
	GraphicsToSound(&Pos, &relativeTarget);
	relativeTarget.pp4d_Time = GetAudioFrameCount();

	Result = Move3DSound(sound->bee_Sound3DPtr,
	  &(sound->bee_Target), &relativeTarget);
	CHECKRESULT(Result, "Move3DSound");

	beeUpdateHost(sound, beeSpace, i);
	sound->bee_Target = relativeTarget;

cleanup:
	return(Result);
}

/****************************************************************/
Err beeAnimate(beeSound sounds[], Sound3DSpace* beeSpace)
{
	Err Result;

DBUG(("Starting everything up\n"));

	/* Preset the global reverb mixer channel gains */
	Result = SetKnobPart(beeSpace->bees_GainKnob, NUMBER_OF_SOUNDS,
	  beeGlobalReverb);
	CHECKRESULT(Result, "SetKnobPart");

	/* Start the output instrument */
	Result = StartInstrument(beeSpace->bees_LineOut, 0);
	CHECKRESULT(Result, "StartInstrument");
	Result = StartInstrument(beeSpace->bees_OutputInst, 0);
	CHECKRESULT(Result, "StartInstrument");

	/* Go into the movement refresh loop */
	Result = beeSteer( sounds, beeSpace );

cleanup:

DBUG(("Stopping everything\n"));

	StopInstrument(beeSpace->bees_OutputInst, 0);
	StopInstrument(beeSpace->bees_LineOut, 0);

	return(Result);
}

/***********************************************************************/
Err beeSteer(beeSound sounds[], Sound3DSpace* beeSpace)
{
	Err Result = 0;
	int32 doit=TRUE, i;
	ControlPadEventData cped;
	uint32 joy;
	PolarPosition4D targetPos;
	Item sleepCue;

	/* graphics variables */
	Character* theStatic;
	gfloat xPan, yPan;
	gfloat zPosIncInc, zPosInc = 0.0, zPos, zClosest;
	Point3 center;
	Box3 bound;		/* group bounding box */
	gfloat xyz[6];
	uint32 cam_state;
	bool needUpdate = TRUE;
	Camera* cam;
	Point3 Pos;

	xPan = PAN_START;
	yPan = PAN_START;
	theStatic = Scene_GetStatic (theScene);
	cam = Scene_GetCamera(theScene);

	/* Get info about Character. */
	Char_GetBound (theStatic, &bound, CHAR_Local);
	Char_GetCenter (theStatic, &center, CHAR_Local);

	/* Scale Z translation as a fraction of Character dimensions. */
	zPosIncInc = (Box3_Width (&bound) + Box3_Height (&bound)) / 100.0;
	zClosest = zPosIncInc * 120.0;
	zPos = ZPOS_START;

	/* Allocate cue for waiting */
	sleepCue = CreateCue( NULL );
	CHECKRESULT(sleepCue, "CreateCue");

	/* set all sounds to start at corresponding graphic positions */
	for (i=0;i<NUMBER_OF_SOUNDS;i++)
	{
		calcRelativePos(sounds[i].bee_Char, cam, &Pos);
		GraphicsToSound(&Pos, &(sounds[i].bee_Target));

		targetPos = sounds[i].bee_Target;
		Start3DSound( sounds[i].bee_Sound3DPtr, &targetPos );
		beeUpdateHost(&(sounds[i]), beeSpace, i);
		StartInstrument( sounds[i].bee_Source, 0 );
	}

	while (doit)
	{
		cam_state = 0;

		/* Poll the joypad */
		Result = GetControlPad (1, FALSE, &cped);
		if (Result < 0)
		{
			PrintError(0,"get control pad data in","see_sound",Result);
		}

		joy = cped.cped_ButtonBits;

		if (joy & ControlX)
		{
			doit = FALSE;
		}

		if (joy & ControlLeft)
		{
			yPan = RadiansToDegrees(OBSERVER_ANGLE_INCR);
			cam_state |= CAM_PanY;
			needUpdate = TRUE;
		}

		if (joy & ControlRight)
		{
			yPan = RadiansToDegrees(-OBSERVER_ANGLE_INCR);
			cam_state |= CAM_PanY;
			needUpdate = TRUE;
		}

		if (joy & ControlUp)
		{
			zPosInc = -FramesToMeters(OBSERVER_RADIUS_INCR);
			cam_state |= CAM_Zoom;
			needUpdate = TRUE;
		}

		if (joy & ControlDown)
		{
			zPosInc = FramesToMeters(OBSERVER_RADIUS_INCR);
			cam_state |= CAM_Zoom;
			needUpdate = TRUE;
		}

		if (joy & ControlA)
		{
			xPan=0.0; yPan=0.0;
			zPosInc = 0.0;
			needUpdate = TRUE;
			SceneShowAll (theScene);
		}

		/* change a bee's direction stochastically */
		if (Choose(1.0) >= BEE_CHANGE_DIRECTION_THRESHOLD)
		{
			i = (int32)Choose(NUMBER_OF_BEES);	/* pick a bee */
			targetPos.pp4d_Radius = wChoose(-BEE_MAX_DELTA_RADIUS, BEE_MAX_DELTA_RADIUS);
			targetPos.pp4d_Theta = wChoose(-BEE_MAX_DELTA_THETA, BEE_MAX_DELTA_THETA);

DBUG2(("Moving %i to: %f, %f\n", i, targetPos.pp4d_Radius, targetPos.pp4d_Theta));

			SoundToGraphics(&targetPos, &Pos);
			DBUG2(("to xyz: %f, %f, %f\n", Pos.x, Pos.y, Pos.z));

			Char_Move(sounds[i].bee_Char, Pos.x, Pos.y, Pos.z);
		}

		if (needUpdate)
		{
			/* Apply rotations/motions to accumulated rotation/position. */
			zPos += zPosInc;
			if (zPos > zClosest) zPos = zClosest;

			xyz[0] = 0;
			xyz[1] = 0;
			xyz[2] = 0; /* in this demo, we only pan and zoom */
			xyz[3] = zPosInc;
			xyz[4] = xPan;
			xyz[5] = yPan;

			SceneMoveCamera (theScene, cam_state, xyz);
			needUpdate = FALSE;
		}

		/* move the audio */
		for (i=0;i<NUMBER_OF_SOUNDS;i++)
		{
			beeMoveSound(cam, &sounds[i], beeSpace, i);
		}

		/* update the display while the audio moves */
		Scene_Display (theScene, gp);
		SwapBuffers();

/*
Removing the comments below enables a fixed frame rate of 60 fps.
*/
		/* SleepUntilTime( sleepCue, GetAudioTime() + TIME_INCR ); */
	}

cleanup:
	for (i=0;i<NUMBER_OF_SOUNDS;i++)
	{
		StopInstrument( sounds[i].bee_Source, 0);
		Stop3DSound( sounds[i].bee_Sound3DPtr );
	}

	DeleteCue( sleepCue );

	return Result;
}
