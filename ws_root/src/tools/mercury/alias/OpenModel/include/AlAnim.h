/*
//-
//	Copyright (C) 1995, Alias|Wavefront
//
//  These coded instructions,  statements and  computer programs contain
//  unpublished information proprietary to Alias|Wavefront  and are
//  protected by the Canadian and US Federal copyright law. They may not
//  be disclosed to third parties  or copied  or duplicated, in whole or
//  in part,  without the prior written consent of Alias|Wavefront
//
//  Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+
*/

/*
 *	This file contains enumeration types that are used by the animation
 *	class methods.
 *
 *	This file MUST be compilable under C as well as C++ for the
 *	C interface.  If you add to this file, please make sure C-style
 *	comments are used.
 */


#ifndef _AlAnim
#define _AlAnim

#include <AlShadingFields.h>

/*
 * The component of an AlMotionAction that should be extracted when
 * evaluating a channel that contains an AlMotionAction
 */

typedef enum {
	kINVALID_COMPONENT = 0,
	kX_COMPONENT	= 1,
	kY_COMPONENT	= 2,
	kZ_COMPONENT	= 3
} AlTripleComponent;


/*
 * Pre- and Post- extrapolation (infinite) types for actions
 * (These types define the behaviour of the action before and after
 * its defined range).
 */

typedef enum {
	kEXTRAP_INVALID,
	kEXTRAP_CONSTANT,
	kEXTRAP_CYCLE,
	kEXTRAP_OSCILLATE,
	kEXTRAP_LINEAR,
	kEXTRAP_IDENTITY
} AlActionExtrapType;



/*
 * The different data types represented by AlStream objects
 */

typedef enum {
	kAA_DATA_FLOAT				= 0,
	kAA_DATA_DOUBLE				= 1,
	kAA_DATA_INT				= 2,
	kAA_DATA_DOUBLE_VECTOR		= 5,
	kAA_DATA_FLOAT_VECTOR		= 6,

	kAA_DATA_INVALID			= 7
} AlStreamDataType;


/*
 * Channel field ID's for the various animatable Alias objects
 */

typedef enum {
	kFLD_DAGNODE_XTRANSLATE		= 0,
	kFLD_DAGNODE_YTRANSLATE		= 1,
	kFLD_DAGNODE_ZTRANSLATE		= 2,
	kFLD_DAGNODE_XROTATE		= 3,
	kFLD_DAGNODE_YROTATE		= 4,
	kFLD_DAGNODE_ZROTATE		= 5,
	kFLD_DAGNODE_XSCALE			= 6,
	kFLD_DAGNODE_YSCALE			= 7,
	kFLD_DAGNODE_ZSCALE			= 8,
	kFLD_DAGNODE_VISIBILITY		= 9
} AlDagNodeFields;

typedef enum {
	kFLD_CAMERA_FOV				= 0,
	kFLD_CAMERA_CLIP_NEAR		= 1,
	kFLD_CAMERA_CLIP_FAR		= 2,
	kFLD_CAMERA_STEREO_EYE_OFFSET = 3,
	kFLD_CAMERA_FSTOP			= 4,
/*	kFLD_CAMERA_FOCAL_LENGTH	= 5, */
	kFLD_CAMERA_FOCAL_DISTANCE	= 6,
	kFLD_CAMERA_STEREO			= 8,
	kFLD_CAMERA_DEPTH_OF_FIELD	= 9,
	kFLD_CAMERA_MOTION_BLUR		= 10,
	kFLD_CAMERA_SCALING_FACTOR	= 11,
	kFLD_CAMERA_FILM_OFFSET_X	= 12,
	kFLD_CAMERA_FILM_OFFSET_Y	= 13,
	kFLD_CAMERA_FILM_BACK_X  	= 14,
	kFLD_CAMERA_FILM_BACK_Y  	= 15
} AlCameraFields;

typedef enum {
	kFLD_CURVECV_XPOSITION		= 0,
	kFLD_CURVECV_YPOSITION		= 1,
	kFLD_CURVECV_ZPOSITION		= 2,
	kFLD_CURVECV_WEIGHT			= 3
} AlCurveCVFields;

typedef enum {
	kFLD_SURFACECV_XPOSITION	= 0,
	kFLD_SURFACECV_YPOSITION	= 1,
	kFLD_SURFACECV_ZPOSITION	= 2,
	kFLD_SURFACECV_WEIGHT		= 3
} AlSurfaceCVFields;

typedef enum {
	kFLD_POLYSETVERTEX_XPOSITION					= 0,
	kFLD_POLYSETVERTEX_YPOSITION					= 1,
	kFLD_POLYSETVERTEX_ZPOSITION					= 2
} AlPolysetVertexFields;

typedef enum {
	/* Common Light parameter */
	kFLD_LIGHT_COLOR_RED							= 0,
	kFLD_LIGHT_COLOR_GREEN							= 1,
	kFLD_LIGHT_COLOR_BLUE							= 2,
	kFLD_LIGHT_INTENSITY							= 3,
	kFLD_LIGHT_SHADOWS								= 4,

	/* Spot Light parameter */
	kFLD_SPOT_DROPOFF								= 5,
	kFLD_SPOT_CUTOFF								= 6,
	kFLD_SPOT_PENUMBRA								= 7,

	/* Ambient Light parameter */
	kFLD_AMBIENT_SHADE								= 8,

	/* Light Glow parameters */
	kFLD_LIGHT_GLOW_TYPE							= 10,
	kFLD_LIGHT_HALO_TYPE							= 11,
	kFLD_LIGHT_FOG_TYPE								= 12,
	kFLD_LIGHT_GLOW_INTENSITY						= 13,
	kFLD_LIGHT_HALO_INTENSITY						= 14,
	kFLD_LIGHT_FOG_INTENSITY						= 15,
	kFLD_LIGHT_GLOW_SPREAD							= 16,
	kFLD_LIGHT_HALO_SPREAD							= 17,
	kFLD_LIGHT_FOG_SPREAD							= 18,
	kFLD_LIGHT_GLOW_2DNOISE							= 19,
	kFLD_LIGHT_FOG_2DNOISE							= 20,
	kFLD_LIGHT_GLOW_RADIAL_NOISE					= 21,
	kFLD_LIGHT_FOG_RADIAL_NOISE						= 22,
	kFLD_LIGHT_GLOW_STAR_LEVEL						= 23,
	kFLD_LIGHT_FOG_STAR_LEVEL						= 24,
	kFLD_LIGHT_GLOW_OPACITY							= 25,
	kFLD_LIGHT_FOG_OPACITY							= 26,
	kFLD_LIGHT_RADIAL_FREQUENCY						= 27,
	kFLD_LIGHT_STAR_POINTS							= 28,
	kFLD_LIGHT_ROTATION								= 29,
	kFLD_LIGHT_NOISE_USCALE							= 30,
	kFLD_LIGHT_NOISE_VSCALE							= 31,
	kFLD_LIGHT_NOISE_UOFFSET						= 32,
	kFLD_LIGHT_NOISE_VOFFSET						= 33,
	kFLD_LIGHT_NOISE_THRESHOLD						= 34,

	kFLD_LIGHT_FORCE_TYPE							= 35,
	kFLD_LIGHT_FORCE_INTENSITY						= 36,
	kFLD_LIGHT_ACTIVE								= 40,
	kFLD_LIGHT_DECAY	/* NOT IMPLEMENTED */		= 41,
	kFLD_LIGHT_SHADOW_RED							= 42,
	kFLD_LIGHT_SHADOW_GREEN							= 43,
	kFLD_LIGHT_SHADOW_BLUE							= 44,
	kFLD_LIGHT_RADIUS								= 45,
	kFLD_LIGHT_SAMPLES								= 46,
	kFLD_LIGHT_DITHER								= 47,

	kFLD_LIGHT_GLOW_COLOR_R							= 50,
	kFLD_LIGHT_GLOW_COLOR_G							= 51,
	kFLD_LIGHT_GLOW_COLOR_B							= 52,
	kFLD_LIGHT_HALO_COLOR_R							= 53,
	kFLD_LIGHT_HALO_COLOR_G							= 54,
	kFLD_LIGHT_HALO_COLOR_B							= 55,
	kFLD_LIGHT_FOG_COLOR_R							= 56,
	kFLD_LIGHT_FOG_COLOR_G							= 57,
	kFLD_LIGHT_FOG_COLOR_B							= 58,
	kFLD_LIGHT_FLARE_COLOR_R						= 59,
	kFLD_LIGHT_FLARE_COLOR_G						= 60,
	kFLD_LIGHT_FLARE_COLOR_B						= 61,
	kFLD_LIGHT_FLARE_INTENSITY						= 62,
	kFLD_LIGHT_FLARE_NUM_CIRCLES					= 63,
	kFLD_LIGHT_FLARE_MIN_SIZE						= 64,
	kFLD_LIGHT_FLARE_MAX_SIZE						= 65,
	kFLD_LIGHT_FLARE_COLOR_SPREAD					= 66,
	kFLD_LIGHT_FLARE_FOCUS							= 67,
	kFLD_LIGHT_HEXAGON_FLARE						= 68,

	/* Volume light parameters */
	kFLD_LIGHT_VOLUME_DECAY							= 80,
	kFLD_LIGHT_VOLUME_SHAPE							= 81,
	kFLD_LIGHT_VOLUME_SPECULAR						= 82,
	kFLD_LIGHT_VOLUME_DECAY_START					= 83,
	kFLD_LIGHT_VOLUME_DIRECTIONALITY				= 84,
	kFLD_LIGHT_VOLUME_CONCENTRIC					= 85,
	kFLD_LIGHT_VOLUME_DIRECTIONAL					= 86,
	kFLD_LIGHT_VOLUME_RADIAL						= 87,
	kFLD_LIGHT_VOLUME_TORUS_RADIUS					= 88,
	kFLD_LIGHT_VOLUME_CONE_END_RADIUS				= 89,
	kFLD_LIGHT_VOLUME_DROPOFF						= 90,
	kFLD_LIGHT_VOLUME_DROPOFF_START					= 91,
	kFLD_LIGHT_VOLUME_ARC							= 92,
	kFLD_LIGHT_VOLUME_DIRECTIONAL_TURB				= 93,
	kFLD_LIGHT_VOLUME_EXCLUDE						= 94,
	kFLD_LIGHT_VOLUME_TURB_TYPE						= 95,
	kFLD_LIGHT_VOLUME_TURB_INTENSITY				= 96,
	kFLD_LIGHT_VOLUME_TURB_SPREAD					= 97,
	kFLD_LIGHT_VOLUME_TURB_PERSISTENCE				= 98,
	kFLD_LIGHT_VOLUME_TURB_SPACE_RES				= 99,
	kFLD_LIGHT_VOLUME_TURB_TIME_RES					= 100,
	kFLD_LIGHT_VOLUME_TURB_ROUGHNESS				= 101,
	kFLD_LIGHT_VOLUME_TURB_VARIABILITY				= 102,
	kFLD_LIGHT_VOLUME_TURB_GRANULARITY				= 103,
	kFLD_LIGHT_VOLUME_TURB_ANIMATED					= 104,

	kFLD_LIGHT_FLARE								= 105,
	kFLD_LIGHT_FLARE_VERTICAL						= 106,
	kFLD_LIGHT_FLARE_HORIZONTAL						= 107,
	kFLD_LIGHT_FLARE_LENGTH							= 108,

	kFLD_LIGHT_DEFORM								= 109,
	kFLD_LIGHT_DEFORM_INTENSITY						= 110,
	kFLD_LIGHT_DEFORM_STEPS							= 111,
	kFLD_LIGHT_DEFORM_SAMPLE_SIZE					= 112

} AlLightFields;

typedef enum {
	kFLD_IMAGE_PLANE_SUFFIX       = 0,
	kFLD_IMAGE_PLANE_XORIGIN      = 1,
	kFLD_IMAGE_PLANE_YORIGIN      = 2,
	kFLD_IMAGE_PLANE_XSIZE        = 3,
	kFLD_IMAGE_PLANE_YSIZE        = 4,
	kFLD_IMAGE_PLANE_VIEWORGX     = 5,
	kFLD_IMAGE_PLANE_VIEWORGY     = 6,
	kFLD_IMAGE_PLANE_VIEWSIZEX    = 7,
	kFLD_IMAGE_PLANE_VIEWSIZEY    = 8,
	kFLD_IMAGE_PLANE_WINORGX      = 9,
	kFLD_IMAGE_PLANE_WINORGY      = 10,
	kFLD_IMAGE_PLANE_WINSIZEX     = 11,
	kFLD_IMAGE_PLANE_WINSIZEY     = 12,
	kFLD_IMAGE_PLANE_DISPLAY      = 13,
	kFLD_IMAGE_PLANE_DEPTH        = 14,
	kFLD_IMAGE_PLANE_XWRAP        = 15,
	kFLD_IMAGE_PLANE_YWRAP        = 16,
	kFLD_IMAGE_PLANE_MASK_SUFFIX  = 17,
	kFLD_IMAGE_PLANE_RGBMULT_R    = 18,
	kFLD_IMAGE_PLANE_RGBMULT_G    = 19,
	kFLD_IMAGE_PLANE_RGBMULT_B    = 20,
	kFLD_IMAGE_PLANE_RGBOFFSET_R  = 21,
	kFLD_IMAGE_PLANE_RGBOFFSET_G  = 22,
	kFLD_IMAGE_PLANE_RGBOFFSET_B  = 23,
	kFLD_IMAGE_PLANE_CHROMAKEY_R          = 24,
	kFLD_IMAGE_PLANE_CHROMAKEY_G          = 25,
	kFLD_IMAGE_PLANE_CHROMAKEY_B          = 26,
	kFLD_IMAGE_PLANE_CHROMAKEY_HUE        = 27,
	kFLD_IMAGE_PLANE_CHROMAKEY_SATURATION = 28,
	kFLD_IMAGE_PLANE_CHROMAKEY_VALUE      = 29,
	kFLD_IMAGE_PLANE_CHROMAKEY_THRESHOLD  = 30
} AlImagePlaneFields;

typedef enum {
	kFLD_CLUSTER_MEMBER_PERCENT			= 0
} AlClusterMemberFields;

typedef enum {
	kFLD_WINDOW_CAMERA					= 0
} AlWindowFields;

#endif	/* _AlAnim */
