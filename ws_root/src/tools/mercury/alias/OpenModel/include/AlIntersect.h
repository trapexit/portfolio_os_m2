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
//	.NAME AlIntersect - Support for intersection of objects
//
//	.SECTION Description
//		This collection of classes provides the programmer with the ability
//		to determine points of intersection between two curves, a curve and
//		a surface, or two surfaces.
//
//		Each type of intersection responds with its own class of results:
//		AlIntersectCurveCurveInfo, AlIntersectCurveSurfInfo, and
//		AlIntersectSurfSurfInfo.  The first two of these are simple AlList
//		classes.  The last one is a stucture storing pointers to AlCurves
//		and (x/y/z) points.
//

#ifndef _AlIntersect
#define _AlIntersect

#include <AlLinkItem.h>
#include <AlList.h>

class AlCurve;
class AlSurface;
class AlIntersectSurfSurfInfo;

class AlIntersect
{
	public:

	static statusCode	intersect( AlCurve* curve1, AlCurve* curve2,
						AlList* &cclist );
	static statusCode	intersect( AlCurve* curve, AlSurface* surface,
						AlList* &cslist );
	static statusCode	intersect( AlSurface* surface1, AlSurface* surface2,
						AlIntersectSurfSurfInfo& ss );
};

class AlIntersectCurveCurveInfo : public AlLinkItem
{
	friend					AlIntersect;

	public:
		AlIntersectCurveCurveInfo*	Next() const;

		double				tCurve1, tCurve2;
		double				point[3];

	protected:
							AlIntersectCurveCurveInfo() {};
		virtual				~AlIntersectCurveCurveInfo();
};

class AlIntersectCurveSurfInfo : public AlLinkItem
{
	friend					AlIntersect;

	public:
		AlIntersectCurveSurfInfo*	Next() const;

		double				t;
		double				pointOnCurve[3];

		double				u,v;
		double				pointOnSurface[3];

	protected:
							AlIntersectCurveSurfInfo() {};
		virtual				~AlIntersectCurveSurfInfo();
};

typedef double double3[3];
class AlIntersectSurfSurfInfo
{
public:
	int			numberIsolatedPoints;
	double3*	isolatedPoints;

	int			numberIntersectionCurves;
	AlCurve**	intersectionCurves;

	int			numberBoundaryCurves;
	AlCurve**	boundaryCurves;

    			~AlIntersectSurfSurfInfo();
};

#endif
