#ifndef _AlIKConstraint
#define _AlIKConstraint

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
//	.NAME AlIKConstraint - Interface to IK constraint data.
//
//	.SECTION Description
//		The AlIKConstraint class allows access to the IK constraint information
//		on an AlJointNode.
//
//		(See the discussion of skeletons, joints, contraints, ik, etc. in
//		the Alias Studio manuals for a full description.)
//
//		Briefly, the constraint data constrains the joint node to another
//		object. The constraint can be a point constraint, an orientation
//		constraint, or an aim constraint.
//
//		This is the base class for constraints. Point, orientation, and
//		aim constraints are derived from this class.
//

#include <AlObject.h>

extern "C" {
	struct Ik_constraint_data_s;
}

class AlDagNode;
struct Dag_node;

class AlIKConstraint : public AlObject {
	friend class			AlFriend;

public:
	virtual 				~AlIKConstraint();

	virtual statusCode		deleteObject();

	virtual AlObjectType	type() const;
	virtual AlIKConstraint*	asIKConstraintPtr();

	AlIKConstraint*			next() const;

	boolean					on() const;
	double					weight() const;

	statusCode				setOn( boolean );
	statusCode				setWeight( double );

	AlDagNode*				dagNode() const;

protected:
							AlIKConstraint();

	statusCode				makeAConstraint( AlJointNode*, AlDagNode*, int, int );
	Dag_node*				getJointDagNode( const AlJointNode *joint ) const;

private:
	// extract type not needed since not anim/clust/settable

	static void initMessages();
	static void finiMessages();
};

#endif	/* _AlIKConstraint */
