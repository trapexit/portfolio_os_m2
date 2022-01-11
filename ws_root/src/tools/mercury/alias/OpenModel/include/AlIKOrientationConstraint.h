#ifndef _AlIKOrientationConstraint
#define _AlIKOrientationConstraint

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
//	.NAME AlIKOrientationConstraint - Interface to IK orientation constraint data.
//
//	.SECTION Description
//		The AlIKOrientationConstraint class allows access to the IK
//		orientation constraint information on an AlJointNode. This
//		class is derived from the AlIKConstraint class.
//
//		(See the discussion of skeletons, joints, contraints, ik, etc. in
//		the Alias Studio manuals for a full description.)
//
*/

typedef enum {
	kIKOrientationConstraint_Invalid,
	kIKOrientationConstraint_X, kIKOrientationConstraint_Y,
	kIKOrientationConstraint_Z, kIKOrientationConstraint_XYZ
} AlIKOrientationConstraintAxis;

#ifdef __cplusplus

#include <AlObject.h>
#include <AlIKConstraint.h>

extern "C" {
	struct Ik_constraint_data_s;
}

class AlJointNode;
class AlDagNode;

class AlIKOrientationConstraint : public AlIKConstraint {

public:

										AlIKOrientationConstraint();
	virtual 							~AlIKOrientationConstraint();

	virtual AlObject*					copyWrapper() const;

	statusCode							create(AlJointNode*, AlDagNode*, AlIKOrientationConstraintAxis);

	virtual AlObjectType				type() const;
	virtual AlIKOrientationConstraint*	asIKOrientationConstraintPtr();

	AlIKOrientationConstraintAxis		axis() const;
	statusCode							setAxis( AlIKOrientationConstraintAxis);
};

#endif /* __cplusplus */

#endif	/* _AlIKOrientationConstraint */
