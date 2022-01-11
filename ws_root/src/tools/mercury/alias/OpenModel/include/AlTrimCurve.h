//-
//	Copyright (C) 1995, Alias|Wavefront
//
//	These coded instructions,  statements and  computer programs contain
//	unpublished information proprietary to Alias|Wavefront  and are
// 	rotected by the Canadian and US Federal copyright law. They may not
//	be disclosed to third parties  or copied  or duplicated, in whole or
//	in part,  without the prior written consent of Alias|Wavefront
//
//	Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+

//
//	.NAME AlTrimCurve - A curve in parametric surface space, part of a trim boundary.
//
//	.SECTION Description
//		This class holds the trim curve data and provides access methods
//		for the data.  A trim curve is defined over a surface's parametric
//		space.
//

#ifndef _AlTrimCurve
#define _AlTrimCurve

#include <AlObject.h>
#include <AlModel.h>

class AlTrimRegion;
class AlTrimBoundary;
class AlTrimRegion;
class AlDagNode;

class AlTrimCurve : public AlObject
{
	friend class			AlFriend;
	friend class			AlTrimBoundary;
	friend class			AlSurface;

public:
	virtual					~AlTrimCurve();
	virtual AlObject*		copyWrapper() const;

    virtual AlObjectType	type() const;
    virtual AlTrimCurve*	asTrimCurvePtr();

	AlTrimBoundary*			parentBoundary() const;
	AlTrimRegion*			parentRegion() const;
	AlDagNode*				parentDagNode() const;

	AlTrimCurve*			getTwinCurve() const;

	boolean					isReversed() const;
	curveFormType			form() const;
	int						degree() const;
	int						numberOfSpans() const;
	int						numberOfKnots() const;
	int						numberOfCVs() const;

	statusCode				CVsUVPosition( double[], double[][3] ) const;
	int						realNumberOfKnots() const;
	statusCode				realKnotVector( double[] ) const;

	AlTrimCurve*			nextCurve() const;
	statusCode				nextCurveD();

	AlTrimCurve*			prevCurve() const;
	statusCode				prevCurveD();

protected:
							AlTrimCurve( void*, void* );
private:
	void*					fSpline;
	void*					fParent;
};

#endif
