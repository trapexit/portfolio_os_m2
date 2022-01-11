#ifndef __AUDIO_SOUND3D_H
#define __AUDIO_SOUND3D_H


/****************************************************************************
**
**  @(#) sound3d.h 96/06/26 1.15
**
**  3D Sound
**
****************************************************************************/

/* Structure allocated by Create3DSound */

typedef struct Sound3D Sound3D;

/* PolarPosition4D
   This structure is used to specify the position and orientation of a sound
   in space at a given time.

   Position is specified relative to the listener, in polar coordinates
   (radius, theta, phi), where radius is the distance from the listener, theta
   the azimuth angle (left-right-front-back) and phi the elevation angle (up-down).
   
   Units:
   Radius  in frame units.  One frame unit is the distance sound travels in the
           medium in one sample frame (1/44100 seconds).  In air at sea level,
           the speed of sound is about 340 m/s, so one meter is about 130 frame
           units.
           
   Theta   in radians.  0 is defined as straight in front of the listener;
           theta is positive moving clockwise, and negative moving counter-
           clockwise.  Theta should be normalized to the range -PI to PI using
           the function s3dNormalizeAngle, if necessary.
           
   Phi     in radians.  0 is defined as horizontal, with phi positive going up
           and negative going down.
   
   Time    in sample frames, as in GetAudioFrameCount().  One sample frame is
           1/44100 seconds.  Currently the Sound3D library assumes an unsigned
           16-bit frame value.
   
   or_Theta, or_Phi
           in radians.  This number is relative to the listener's orientation,
           so (0,0) means the sound is facing in the same direction as the
           listener.  These angles only affect directional sounds (those with
           the BACK_AMPLITUDE tag < 1.0).

   To convert from X,Y in the listeners coordinate system to Polar Position,
   where:
       X = distance along axis extending through ears of listener
           in frame units.  Positive is to the right.
       Y = distance along axis extending forward through nose of
           listener in frame units.  Positive is forward.

    calculate:
        Radius = sqrtf( (X*X) + (Y*Y) );
	Theta = atan2f( X, Y );

    See the example programs for more examples of converting coordinates.
*/

typedef struct PolarPosition4D
{
	float32  pp4d_Radius;	/* in frame ticks */
	float32  pp4d_Theta;	/* in radians */
	float32  pp4d_Phi;	/* in radians */
	int32    pp4d_Time;	/* in frame ticks */
	float32  pp4d_or_Theta;	/* orientation, in radians */
	float32  pp4d_or_Phi;   /* orientation, in radians */
} PolarPosition4D;

/* Sound3DParms
   This structure is used to return information from a given 3DSound, which
   can then be used by the application to control the source sound or the
   3DSound output.  For example, distance factor can be used to control
   amplitude and/or reverb settings, and doppler can be used to change the
   source instrument's frequency setting.
*/

typedef struct Sound3DParms
{
	float32 s3dp_Doppler;              /* 1.0 => no doppler */
	float32 s3dp_DistanceFactor;       /* 1.0 at minRadius to 0.0 at max */
} Sound3DParms;
	
/* Option flags.  Specified when creating a Sound3D structure */
#define S3D_F_PAN_DELAY                 0x1 /* Use delay-based panning */
#define S3D_F_PAN_AMPLITUDE             0x2 /* Use amplitude-based panning */
#define S3D_F_PAN_FILTER                0x4 /* Use filtering */
#define S3D_F_DISTANCE_AMPLITUDE_SQUARE 0x8 /* Use distance-squared intensity attenuation */
#define S3D_F_DISTANCE_AMPLITUDE_SONE   0x10 /* Use Sone scale for distance-cubed intensity attenuation */
#define S3D_F_OUTPUT_HEADPHONES         0x20 /* Adjust cues for headphone listening */
#define S3D_F_DOPPLER                   0x40 /* Use doppler calculations */
#define S3D_F_SMOOTH_AMPLITUDE		0x80 /* Use ramps between amplitude changes */

/* Option tags.  Specified when calling Create3DSound */
#define S3D_TAG_FLAGS 1                     /* OR-d flags from above */
#define S3D_TAG_BACK_AMPLITUDE 2            /* float indicating 

/* Useful constants
   DISTANCE_TO_EAR is the "average" distance, in frame units, from the center
   of the listener's head to each ear; in other words, the head radius.
*/
   
#define S3D_DISTANCE_TO_EAR (20.0)
#define S3D_EAR_TO_EAR (S3D_DISTANCE_TO_EAR * 2.0)
#define S3D_QUARTERCIRCLE (M_PI_2)
#define S3D_HALFCIRCLE (M_PI)
#define S3D_FULLCIRCLE (M_PI * 2.0)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Convert an angle to the range -PI to PI */
float32 s3dNormalizeAngle( float32 Angle );

/* Create and delete Sound3D structures */
Err Create3DSound( Sound3D** s3dHandle, const TagArg* tagList );
Err Create3DSoundVA( Sound3D** s3dHandle, uint32 tag1, ... );
void Delete3DSound( Sound3D** Snd3D );

/* Get item id of 3D instrument, so we can connect inputs and outputs */
Item Get3DSoundInstrument( const Sound3D* Snd3D );

/* Start and stop the Sound3D processor */
Err Start3DSound( const Sound3D* Snd3D, const PolarPosition4D* Pos4D );
Err Stop3DSound( const Sound3D* Snd3D );

/* Move the sound from start to end in (end-start) dsp frames */
Err Move3DSound( Sound3D* Snd3D, const PolarPosition4D* Start4D, const PolarPosition4D* End4D );

/* Move the sound from wherever it is now to target */
Err Move3DSoundTo( Sound3D* Snd3D, const PolarPosition4D* Target4D);

/* Read frame counts and calculate current position and time. */
Err Get3DSoundPos( const Sound3D* Snd3D, PolarPosition4D* Pos4D );

/* Read application-implemented cue structure */
Err Get3DSoundParms( const Sound3D* Snd3D, Sound3DParms* Snd3DParms, uint32 s3dParmsSize );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __AUDIO_SOUND3D_H */
