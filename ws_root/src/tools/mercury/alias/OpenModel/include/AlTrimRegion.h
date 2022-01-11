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
//	.NAME AlTrimRegion - A list of trim boundaries defining a trim region.
//
//	.SECTION Description
//		This class defines a region of a surface's parametric space which
//		identifies an area of interest on a surface.  It holds at least one
//		AlTrimBoundary object. The first boundary on the list represents
//		the outermost boundary of a trim region and the rest represent
//		the inner boundaries (holes) if there are any.
//
//		For each trim boundary, the surface is to the left of the boundary
//		when viewed from above (hence a hole is to the right of the boundary).
//
//		For example, in this diagram we have a surface represented by A's.
//		Each additional letter is an island trimmed out of A and each would
//		be a separate AlTrimRegion.  The AlTrimRegion for A would have two
//		AlTrimBoundaries, B would have three, and C, D, and E would have
//		one each.
// .nf
// .ft C
//		AAAAAAAAAAAAAAAAAAAAAAAAAAAA
//		AAAAAAAAAAAAAAAAAAAAAAAAAAAA
//		AA                        AA
//		AA BBBBBBBBBBB CCCCCCCCCC AA
//		AA BBBBBBBBBBB CCCCCCCCCC AA
//		AA BB     BBBB            AA
//		AA BB EEE BBBBBBBBBBBBBBB AA
//		AA BB EEE BBBB         BB AA
//		AA BB     BBBB DDDDDDD BB AA
//		AA BBBBBBBBBBB DDDDDDD BB AA
//		AA BBBBBBBBBBB         BB AA
//		AA BBBBBBBBBBBBBBBBBBBBBB AA
//		AA                        AA
//		AAAAAAAAAAAAAAAAAAAAAAAAAAAA
//		AAAAAAAAAAAAAAAAAAAAAAAAAAAA
// .ft P
// .fi
//
//

#ifndef _AlTrimRegion
#define _AlTrimRegion

#include <AlObject.h>
#include <AlTM.h>

class AlTrimBoundary;
class AlIterator;
class AlDagNode;

class AlTrimRegion : public AlObject
{ 
	friend class		AlFriend;

public:
	virtual					~AlTrimRegion();
	virtual AlObject*		copyWrapper() const;

    virtual AlObjectType	type() const;
    virtual AlTrimRegion*	asTrimRegionPtr();

	AlDagNode*				parentDagNode() const;

	AlTrimBoundary*			firstBoundary() const;
	statusCode				applyIteratorToBoundaries( AlIterator*, int& ) const;

	AlTrimRegion*			nextRegion() const;
	statusCode				nextRegionD();

	AlTrimRegion*			prevRegion() const;
	statusCode				prevRegionD();

// Stuff to get surface information
	
    curveFormType           uForm() const;
    curveFormType           vForm() const;
    int                     uDegree() const;
    int                     vDegree() const;
    int                     uNumberOfSpans() const;
    int                     vNumberOfSpans() const;
    int                     uNumberOfCVs() const;
    int                     vNumberOfCVs() const;

    statusCode              CVsWorldPosition( double[], int[], int[])const;
    statusCode              CVsAffectedPosition( const AlTM&, double[], int[], int[]) const;
    statusCode              CVsUnaffectedPosition( double[], int[], int[])const;
	int                     uNumberOfKnots() const;
    int                     vNumberOfKnots() const;
    statusCode              uKnotVector( double[] ) const;
    statusCode              vKnotVector( double[] ) const;
    int                     uNumberOfCVsInclMultiples() const;
    int                     vNumberOfCVsInclMultiples() const;
    statusCode              CVsWorldPositionInclMultiples( double[] ) const;
    statusCode              CVsAffectedPositionInclMultiples( const AlTM&, double[] ) const;
    statusCode              CVsUnaffectedPositionInclMultiples( double[] )const;
	statusCode              unpileEndKnots( int, const double[] );

    int                     realuNumberOfKnots() const;
    int                     realvNumberOfKnots() const;
    statusCode              realuKnotVector( double[] ) const;
    statusCode              realvKnotVector( double[] ) const;

    statusCode				eval(double,double,boolean,double P[3]=NULL, double Pu[3]=NULL, double Pv[3]=NULL, double n[3]=NULL,boolean=FALSE,boolean=FALSE ) const;

protected:
							AlTrimRegion( void* );
private:
	void*					fParent;
};

#endif
