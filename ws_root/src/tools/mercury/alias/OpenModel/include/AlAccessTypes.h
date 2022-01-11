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

#ifndef _AlAccessTypes
#define _AlAccessTypes

typedef enum {
	kAmbientLightType,
	kAreaLightType,
	kBoxLightType,
	kCameraEyeType,
	kCameraType,
	kCameraUpType,
	kCameraViewType,
	kClusterNodeType,
	kClusterType,
	kClusterMemberType,
	kConeLightType,
	kCurveNodeType,
	kCurveOnSurfaceType,
	kCurveType,
	kCurveCVType,
	kCylinderLightType,
	kDagNodeType,
	kDirectionLightType,
	kFaceNodeType,
	kFaceType,
	kGroupNodeType,
	kImagePlaneType,
	kLightLookAtNodeType,
	kLightNodeType,
	kLightType,
	kLightUpNodeType,
	kLinearLightType,
	kNonAmbientLightType,
	kOrthographicCameraType,
	kPerspectiveCameraType,
	kPointLightType,
	kSetType,
	kSetMemberType,
	kSphereLightType,
	kSpotLightType,
	kSurfaceNodeType,
	kSurfaceType,
	kSurfaceCVType,
	kTorusLightType,
	kVolumeLightType,
	kWindowType,

	kChannelType,
	kActionType,
	kParamActionType,
	kMotionActionType,
	kKeyframeType,
	kStreamType,

	kEnvironmentType,
	kShaderType,
	kTextureType,

	kPolysetNodeType,
	kPolysetType,
	kPolygonType,
	kPolysetVertexType,

	kAttributeType,
	kArcAttributeType,
	kLineAttributeType,
	kCurveAttributeType,
	kPlaneAttributeType,
	kConicAttributeType,
	kRevSurfAttributeType,

	kJointNodeType,
	kIKConstraintType,
	kIKPointConstraintType,
	kIKOrientationConstraintType,
	kIKAimConstraintType,

	kTextureNodeType,

	kShellNodeType,
	kShellType,

	kTrimRegionType,
	kTrimBoundaryType,
	kTrimCurveType,

	kCommandType,
	kCommandRefType,
	kContactType

} AlObjectType;

typedef enum {
	kLightGlowOff			= 0,
	kLightGlowLinear		= 1,
	kLightGlowExponential	= 2,
	kLightGlowBall			= 3,
	kLightGlowSpectral		= 4,
	kLightGlowRainbow		= 5
} AlLightGlowType;

typedef enum {
	kLightHaloOff			= 0,
	kLightHaloLinear		= 1,
	kLightHaloExponential	= 2,
	kLightHaloBall			= 3,
	kLightHaloLensFlare		= 4,
	kLightHaloRimHalo		= 5
} AlLightHaloType;

typedef enum {
	kLightFogOff			= 0,
	kLightFogLinear			= 1,
	kLightFogExponential	= 2,
	kLightFogBall			= 3
} AlLightFogType;

typedef enum {
	kDisplayModeBoundingBox,	/* These first five can only be used */
	kDisplayModeInvisible,		/* by AlDagNode::isDisplayModeSet() */
	kDisplayModeQuickWire,		/* and AlDagNode::setDisplayMode(). */
	kDisplayModeTemplate,
	kDisplayModeDashed,

	kDisplayGeomHull,			/* These four can only be used by the */
	kDisplayGeomEditPoints,		/* AlCurve, AlSurface and AlPolyset */
	kDisplayGeomKeyPoints,		/* isDisplayModeSet() and setDisplayMode() */
	kDisplayGeomCVs				/* methods. */
} AlDisplayModeType;

#endif
