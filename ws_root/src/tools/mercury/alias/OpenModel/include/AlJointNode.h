#ifndef _AlJointNode
#define _AlJointNode

/*
//-
//	Copyright (C) 1995, Alias|Wavefront
//
//  These coded instructions,  statements and  computer programs contain
//  unpublished information proprietary to Alias|Wavefront  and are
//  rotected by the Canadian and US Federal copyright law. They may not
//  be disclosed to third parties  or copied  or duplicated, in whole or
//  in part,  without the prior written consent of Alias|Wavefront
//
//  Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+
//
//	.NAME AlJointNode - Interface to joint node data.
//
//	.SECTION Description
//		The AlJointNode class allows access to the joint information
//		on an AlDagNode.
//
//		See the discussion of skeletons, joints, constraints, ik, character
//		builder, ..etc.. in the Alias Studio manuals for a full description.
//
//		Briefly, the joint node is used by the inverse kinematic algorithm
//		to define properties of a joint.  The character joint attributes
//		are used to define properties of the bone below the joint for
//		skinning operations.
//
//		In general, minimum values must be less than maximum values and
//		stiffness values must be between 0 and 100 inclusive.
//
*/

/*
// Some enumerated type used by character joint attributes
//
*/

typedef enum {
	kJointInterpLinear		= 0,
	kJointInterpExponential	= 1,
	kJointInterpSine		= 2
} AlJointInterpolationType;

typedef enum {
	kBulgeDefinition		= 0,
	kBulgeEvenlySpaced		= 1
} AlBulgeSections;

typedef enum {
	kBulgeLowerJoint		= 0,
	kBulgeOtherNode			= 1
} AlBulgeRelateTo;

#ifdef __cplusplus

#include <AlAnim.h>
#include <AlObject.h>
#include <AlIKConstraint.h>
#include <AlIKAimConstraint.h>
#include <AlIKPointConstraint.h>
#include <AlIKOrientationConstraint.h>

struct SK_joint_s;
struct Dag_node;

class AlDagNode;
class AlJointNode : public AlObject {
	friend class			AlFriend;

public:
	virtual					~AlJointNode();
	virtual AlObject*		copyWrapper() const;
	
	virtual AlObjectType	type() const;
	virtual AlJointNode*	asJointNodePtr();

	boolean					anchor() const;
	statusCode				useTransforms( boolean[3], boolean[3] ) const;
	statusCode				useLimits( boolean[3], boolean[3] ) const;
	statusCode				rotation( double[3], double[3], int[3] ) const;
	statusCode				translation( double[3], double[3], int[3] ) const;
	AlIKConstraint*			firstConstraint() const;

	statusCode				setAnchor( boolean );
	statusCode				setUseTransforms( const boolean[3], const boolean[3] );
	statusCode				setUseLimits( const boolean[3], const boolean[3] );
	statusCode				setRotation( const double[3], const double[3], const int[3] );
	statusCode				setTranslation( const double[3], const double[3], const int[3] );

	boolean					charJoint() const;
	statusCode				charJointLimits(double&, double&, AlJointInterpolationType&) const;
	statusCode				charJointLimits(double*, double*, AlJointInterpolationType*) const;
	boolean					useBulge() const;
	statusCode				bulgeAttributes(int*, int*, double*, AlBulgeSections*, int*, int*, AlDagNodeFields*, AlBulgeRelateTo*, char*, double*) const;

	statusCode				setCharJoint(boolean);
	statusCode				setCharJointLimits(double, double, AlJointInterpolationType);
	statusCode				setUseBulge(boolean);
	statusCode				setBulgeAttributes(int, int, double, AlBulgeSections, int, int, AlDagNodeFields, AlBulgeRelateTo, const char *, double);

protected:
							AlJointNode( Dag_node* );
	Dag_node*				fParentDagNode;

private:
	static void       		initMessages();
	static void       		finiMessages();
};

#endif /* __cplusplus */ 

#endif	/* _AlJointNode */
