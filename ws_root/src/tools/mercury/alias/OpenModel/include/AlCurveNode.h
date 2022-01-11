#ifndef _AlCurveNode
#define _AlCurveNode

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
//+
//
//	.NAME AlCurveNode - A dag node that refers to a nurbs curve's geometry.
//
//	.SECTION Description
//      AlCurveNode is the class used to access and manipulate
//      curves within the dag.
//      This class behaves like other dag nodes (see AlDagNode for a
//      description of the usage and purpose of dag nodes).
//		Users access the geometry of the curve
//      via the 'curve()' method which returns a
//      pointer to an AlCurve object which provides the user with
//      the methods used to modify the geometry of the curve.
//      Curves can be created from scratch by instantiating and creating
//		an AlCurve, and then by instantiating and creating an AlCurveNode
//		(using the AlCurve as a parameter).
//
//		There are two ways to delete an AlCurveNode.  If the AlCurveNode
//		deleteObject() method is called, then the node's AlCurve is also
//		deleted.  If the AlCurve::deleteObject() method is called, then its
//		associated AlCurveNode is also deleted.
//
*/

typedef enum {	kJoinSuccess, kJoinFailure, kJoinBadCluster,
				kJoinInvalidKeyPoints, kJoinNoAttributes, kJoinBadData,
				kJoinDuplicateCurve, kJoinCurveClosed
			} AlCurveNodeJoinErrors;

#ifdef __cplusplus

#include <AlDagNode.h>
#include <AlModel.h>

class AlCurveNode : public AlDagNode {
	friend					class AlFriend;
public:
							AlCurveNode();
	virtual					~AlCurveNode();
	virtual AlObject*		copyWrapper() const;

	statusCode				create( AlCurve* );

	virtual AlObjectType	type() const;
	virtual AlCurveNode*	asCurveNodePtr();

	AlCurve*				curve() const;
	AlCurve*				curve(AlTM&) const;

	AlCurveNodeJoinErrors	join(AlCurveNode*);

	int						curvePrecision() const;
	statusCode				setCurvePrecision( int );

	// obsolete
	statusCode				getCurvePrecision( int& ) const;
protected:
};

#endif	/* __cplusplus */

#endif	/* _AlCurveNode */
