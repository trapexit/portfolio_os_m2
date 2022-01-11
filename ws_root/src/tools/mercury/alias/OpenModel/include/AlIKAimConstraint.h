#ifndef _AlIKAimConstraint
#define _AlIKAimConstraint

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
//	.NAME AlIKAimConstraint - Interface to IK aim constraint data.
//
//	.SECTION Description
//		The AlIKAimConstraint class allows access to the IK aim constraint
//		information on an AlJointNode. This class is derived from the
//		AlIKConstraint class.
//
//		(See the discussion of skeletons, joints, contraints, ik, etc. in
//		the Alias Studio manuals for a full description.)
//
*/

typedef enum {
	kIKAimConstraint_Invalid,
	kIKAimConstraint_X, kIKAimConstraint_Y, kIKAimConstraint_Z
} AlIKAimConstraintAxis;

#ifdef __cplusplus

#include <AlObject.h>
#include <AlIKConstraint.h>

extern "C" {
	struct Ik_constraint_data_s;
}

class AlJointNode;
class AlDagNode;

class AlIKAimConstraint : public AlIKConstraint {

public:
								AlIKAimConstraint();
	virtual		 				~AlIKAimConstraint();

	virtual AlObject*			copyWrapper() const;
	statusCode					create(AlJointNode*, AlDagNode*, AlIKAimConstraintAxis);

	virtual AlObjectType		type() const;
	virtual AlIKAimConstraint*	asIKAimConstraintPtr();

	AlIKAimConstraintAxis		axis() const;
	statusCode					setAxis( AlIKAimConstraintAxis );
};

#endif /* __cplusplus */

#endif	/* _AlIKAimConstraint */
