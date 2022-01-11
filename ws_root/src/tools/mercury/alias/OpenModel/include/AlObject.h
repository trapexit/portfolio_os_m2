//-
//	Copyright (C) 1995, Alias|Wavefront
//
//	These coded instructions,  statements and  computer programs contain
//	unpublished information proprietary to Alias|Wavefront  and are
// 	protected by the Canadian and US Federal copyright law. They may not
//	be disclosed to third parties  or copied  or duplicated, in whole or
//	in part,  without the prior written consent of Alias|Wavefront
//
//	Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+

//
//	.NAME AlObject - Base class for all Alias Data types.
//
//	.SECTION Description
//		This is an abstract base class which holds a reference to an
//		anonymous data structure.  Derived classes will initialize the
//		reference to refer to a particular data structure.  This class
//		provides a mechanism for completely hiding the internal data
//		structures from the user.
//
//		This class gives some homogeneity between several different
//		classes by providing methods which derived classes will redefine
//		so that the name and type of the object can be obtained and so
//		that object down casting can be done safely.
//
//		There are a group of functions (the 'downcasting' methods )for casting
//		an AlObject to one of its derived classes.  They have the form
//:
//:		Al{DerivedObject} *AlObject::as{DerivedObject}Ptr()
//:
//		where {DerivedObject} is the name of the derived class.
//:
//		All these functions return NULL in the base class, and the derived
//		class of the appropriate type simply returns a pointer to itself.
//		In this way, safe typecasting can be maintained.
//

#ifndef _AlObject
#define _AlObject

#include <AlStyle.h>

#if defined( __cplusplus  ) || defined( _cplusplus ) || defined( _cplusplus_ ) || defined( c_plusplus ) // ridiculous.

#include <AlDictionary.h>
#include <AlAccessTypes.h>

class AlAmbientLight;
class AlAreaLight;
class AlCamera;
class AlCameraNode;
class AlCluster;
class AlClusterMember;
class AlClusterNode;
class AlCurveCV;
class AlCurve;
class AlCurveNode;
class AlCurveOnSurface;
class AlDagNode;
class AlDirectionLight;
class AlFace;
class AlFaceNode;
class AlGroupNode;
class AlLight;
class AlLightLookAtNode;
class AlLightNode;
class AlLightUpNode;
class AlLinearLight;
class AlNonAmbientLight;
class AlObjectList;
class AlPointLight;
class AlSet;
class AlSetMember;
class AlSpotLight;
class AlVolumeLight;
class AlSphereLight;
class AlCylinderLight;
class AlBoxLight;
class AlTorusLight;
class AlSurfaceCV;
class AlSurface;
class AlSurfaceNode;
class AlShellNode;
class AlShell;
class AlTrimBoundary;
class AlTrimCurve;
class AlTrimRegion;
class AlShader;
class AlTexture;
class AlTextureNode;
class AlEnvironment;
class AlUniverse;
class AlStream;
class AlChannel;
class AlAction;
class AlKeyframe;
class AlParamAction;
class AlMotionAction;
class AlPolysetNode;
class AlPolysetVertex;
class AlPolygon;
class AlPolyset;
class AlCreate;
class AlAttributes;
class AlArcAttributes;
class AlLineAttributes;
class AlCurveAttributes;
class AlPlaneAttributes;
class AlConicAttributes;
class AlRevSurfAttributes;
class AlJointNode;
class AlIKConstraint;
class AlIKOrientationConstraint;
class AlIKPointConstraint;
class AlIKAimConstraint;

class AlClusterable;
class AlAnimatable;
class AlSettable;
class AlPickable;

class AlContact;
class AlCommandRef;
class AlCommand;

class AlOrthographicCamera;
class AlPerspectiveCamera;
class AlWindow;
class AlImagePlane;

class AlObject : protected AlHashable {
	friend class					AlFriend;
	friend class					AlDebugLib;

public:
	virtual 						~AlObject();

	virtual AlObjectType			type() const = 0;

	virtual const char*				name() const;
	virtual statusCode				setName( const char* );
	virtual statusCode				deleteObject();
	virtual AlObject*				copyWrapper() const;

	// Down casting methods

	virtual AlAnimatable*			asAnimatablePtr();
	virtual AlClusterable*			asClusterablePtr();
	virtual AlSettable*				asSettablePtr();
	virtual AlPickable*				asPickablePtr();

	virtual AlCamera*				asCameraPtr();
	virtual AlCameraNode*			asCameraNodePtr();

	virtual AlCluster*				asClusterPtr();
	virtual AlClusterNode*			asClusterNodePtr();
	virtual AlClusterMember*		asClusterMemberPtr();

	virtual AlCurveCV*				asCurveCVPtr();
	virtual AlCurve*				asCurvePtr();
	virtual AlCurveNode*			asCurveNodePtr();
	virtual AlCurveOnSurface*		asCurveOnSurfacePtr();

	virtual AlDagNode*				asDagNodePtr();

	virtual AlFace*					asFacePtr();
	virtual AlFaceNode*				asFaceNodePtr();

	virtual AlGroupNode*			asGroupNodePtr();

	virtual AlLight*				asLightPtr();
	virtual AlLightNode*			asLightNodePtr();
	virtual AlAmbientLight*			asAmbientLightPtr();
	virtual AlAreaLight*			asAreaLightPtr();
	virtual AlDirectionLight*		asDirectionLightPtr();
	virtual AlLinearLight*			asLinearLightPtr();
	virtual AlNonAmbientLight*		asNonAmbientLightPtr();
	virtual AlPointLight*			asPointLightPtr();
	virtual AlSpotLight*			asSpotLightPtr();
	virtual AlVolumeLight*			asVolumeLightPtr();
	virtual AlSphereLight*			asSphereLightPtr();
	virtual AlCylinderLight*		asCylinderLightPtr();
	virtual AlBoxLight*				asBoxLightPtr();
	virtual AlTorusLight*			asTorusLightPtr();

	virtual AlSurfaceCV*			asSurfaceCVPtr();
	virtual AlSurface*				asSurfacePtr();
	virtual AlSurfaceNode*			asSurfaceNodePtr();

	virtual AlSet*					asSetPtr();
	virtual AlSetMember*			asSetMemberPtr();

	virtual AlShader*				asShaderPtr();
	virtual AlTexture*				asTexturePtr();
	virtual AlEnvironment*			asEnvironmentPtr();

	virtual AlKeyframe*				asKeyframePtr();
	virtual AlStream*				asStreamPtr();
	virtual AlChannel*				asChannelPtr();
	virtual AlAction*				asActionPtr();
	virtual AlParamAction*			asParamActionPtr();
	virtual AlMotionAction*			asMotionActionPtr();

	virtual AlPolysetVertex*		asPolysetVertexPtr();
	virtual AlPolysetNode*			asPolysetNodePtr();
	virtual AlPolygon*				asPolygonPtr();
	virtual AlPolyset*				asPolysetPtr();

	virtual AlAttributes*			asAttributesPtr();
	virtual AlArcAttributes*		asArcAttributesPtr();
	virtual AlLineAttributes*		asLineAttributesPtr();
	virtual AlCurveAttributes*		asCurveAttributesPtr();
	virtual AlPlaneAttributes*		asPlaneAttributesPtr();
	virtual AlConicAttributes*		asConicAttributesPtr();
	virtual AlRevSurfAttributes*	asRevSurfAttributesPtr();

	virtual AlJointNode*			asJointNodePtr();
	virtual AlIKConstraint*			asIKConstraintPtr();
	virtual AlIKPointConstraint*	asIKPointConstraintPtr();
	virtual AlIKOrientationConstraint*	asIKOrientationConstraintPtr();
	virtual AlIKAimConstraint*		asIKAimConstraintPtr();

	virtual AlTextureNode*			asTextureNodePtr();

	virtual AlShellNode*			asShellNodePtr();
	virtual AlShell*				asShellPtr();

	virtual AlTrimRegion*			asTrimRegionPtr();
	virtual AlTrimBoundary*			asTrimBoundaryPtr();
	virtual AlTrimCurve*			asTrimCurvePtr();

	virtual AlContact*				asContactPtr();
	virtual AlCommandRef*			asCommandRefPtr();
	virtual AlCommand*				asCommandPtr();

	virtual AlOrthographicCamera*	asOrthographicCameraPtr();
	virtual AlPerspectiveCamera* 	asPerspectiveCameraPtr();
	virtual AlWindow*				asWindowPtr();
	virtual AlImagePlane*			asImagePlanePtr();

protected:
	boolean							viewObject( void *, const AlStream& );

									AlObject();
									AlObject( const AlHashKey& );
private:
};

#ifndef THIS_IS_ALOBJECT
extern "C" boolean AlIsValid( const AlObject* );
extern "C" boolean AlAreEqual( const AlObject*, const AlObject* );
#endif

#else

typedef void AlObject;
extern boolean AlIsValid( const AlObject* );
extern boolean AlAreEqual( const AlObject*, const AlObject* );

#endif

#endif // _AlObject
