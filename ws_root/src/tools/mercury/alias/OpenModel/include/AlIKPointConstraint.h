#ifndef _AlIKPointConstraint
#define _AlIKPointConstraint

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
//	.NAME AlIKPointConstraint - Interface to IK point constraint data.
//
//	.SECTION Description
//		The AlIKPointConstraint class allows access to the IK point constraint
//		information on an AlJointNode. This class is derived from the
//		AlIKConstraint class.
//
//		(See the discussion of skeletons, joints, contraints, ik, etc. in
//		the Alias Studio manuals for a full description.)
//
*/

typedef enum {
	kIKPointConstraint_Invalid,
	kIKPointConstraint_X, kIKPointConstraint_Y, kIKPointConstraint_Z,
	kIKPointConstraint_XY, kIKPointConstraint_XZ, kIKPointConstraint_YZ,
	kIKPointConstraint_XYZ
} AlIKPointConstraintCoordinate;

#ifdef __cplusplus

#include <AlObject.h>
#include <AlIKConstraint.h>

extern "C" {
	struct Ik_constraint_data_s;
}

class AlJointNode;
class AlDagNode;

class AlIKPointConstraint : public AlIKConstraint {

public:
									AlIKPointConstraint();
	virtual 						~AlIKPointConstraint();

	virtual AlObject*				copyWrapper() const;

	statusCode						create(AlJointNode*, AlDagNode*, AlIKPointConstraintCoordinate);

	virtual AlObjectType			type() const;
	virtual AlIKPointConstraint*	asIKPointConstraintPtr();

	AlIKPointConstraintCoordinate	coordinate() const;
	statusCode						setCoordinate( AlIKPointConstraintCoordinate );
};

#endif /* __cplusplus */

#endif	/* _AlIKPointConstraint */
