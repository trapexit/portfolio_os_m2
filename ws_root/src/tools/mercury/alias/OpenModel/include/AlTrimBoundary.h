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
//	.NAME AlTrimBoundary - A list of trim curves defining a trim boundary.
//
//	.SECTION Description
//		This class holds a list of AlTrimCurve objects.  The trim curves
//		form a closed boundary which partition a surface. The trim curves
//		are defined in a surface's parametric space.
//
//		Note that trim boundaries have the surface to the right of the
//		defining curves.
//
//		Also, in the list of trim curves, the end point of a trim curve
//		is coincident with the first point of its following trim curve.
//		The end point of the last trim curve is coincident with the
//		first point of the first trim curve.
//

#ifndef _AlTrimBoundary
#define _AlTrimBoundary

#include <AlObject.h>

class AlIterator;
class AlTrimRegion;
class AlTrimCurve;

class AlTrimBoundary : public AlObject
{	
	friend class			AlFriend;

public:
	virtual					~AlTrimBoundary();
	virtual AlObject*		copyWrapper() const;

    virtual AlObjectType	type() const;
    virtual AlTrimBoundary*	asTrimBoundaryPtr();

	AlTrimRegion*			parentRegion() const;
	AlDagNode*				parentDagNode() const;

	AlTrimCurve*			firstCurve() const;
	statusCode				applyIteratorToCurves( AlIterator*, int& );

	AlTrimBoundary*			nextBoundary() const;
	statusCode				nextBoundaryD();

	AlTrimBoundary*			prevBoundary() const;
	statusCode				prevBoundaryD();

	statusCode				convertToUVPolyline( int &np, double *&CVList );
protected:
							AlTrimBoundary( void * );
private:
	void*					fParent;
};

#endif
