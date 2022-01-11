#ifndef _AlCurveOnSurface
#define _AlCurveOnSurface

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

//  .NAME AlCurveOnSurface - Interface to curves on surfaces geometry.
//
//  .SECTION Description
//      AlCurveOnSurface is the interface to Alias' curve on surface
//		data objects.  Curves on surfaces are created and added to a surface
//		by allocating an AlCurveOnSurface, then calling the
//		create() method, and then calling AlSurfaceNode's
//		addCurveOnSurface() method to add it to a surface.
//
//		Curves on surfaces can also be read in from wire file and modified
//		with this class.  Curves on surfaces will be deleted when the
//		surface they are attached to is deleted.
//
//		When constructing a curve on surface, you will notice that you
//		need to specify a matrix that has dimensions [numberOfControlPoints][4].
//		Each point has 4 values, for u, v, zero and w.  The "u" and "v" 
//		specify the point in U-V parametric space.  The third component is zero
//		because this component is not used.  The "w" component is the
//		homogeneous value, which is usually 1.0.
//	.br
//		See the example programs for an example.
//		

#include <AlObject.h>
#include <AlCurveNode.h>	// to get curveFormType
#include <AlStyle.h>
#include <AlPickable.h>

struct MO_Curve_on_Surface;
class AlSurfaceNode;

class AlCurveOnSurface : public AlObject
						, public AlPickable {
	friend class			AlFriend;

public:
							AlCurveOnSurface();
	virtual					~AlCurveOnSurface();
	virtual statusCode		deleteObject();
	virtual AlObject*		copyWrapper() const;

	virtual AlObjectType	type() const;
	virtual AlCurveOnSurface* asCurveOnSurfacePtr();
	virtual AlPickable*		asPickablePtr();

	statusCode				create( int, curveFormType, int, const double[], int, const double[][4] );
	statusCode				curveOnSurfaceData( double[], double[][4] ) const;

	int						degree() const;
	curveFormType			form() const;
	int						numberOfSpans() const;
	int						numberOfKnots() const;
	int						numberOfControlPoints() const;

	double					knotValue( int ) const;
	statusCode				controlPoint( int, double[4] ) const;

	statusCode				setKnotValue( int, double );
	statusCode				setControlPoint( int, const double[4] );

	boolean					inTrim() const;
	boolean					visible() const;

	AlSurface*				surface() const;

	statusCode				worldSpaceCopys( int& count, AlCurve **&ws_curves );

	AlCurveOnSurface*		nextCurveOnSurface() const;
	AlCurveOnSurface*		prevCurveOnSurface() const;

	statusCode				nextCurveOnSurfaceD();
	statusCode				prevCurveOnSurfaceD();

	// obsolete
	statusCode				setCurveOnSurfaceData( int, curveFormType, int, const double[], int, const double[][4] );
	statusCode				getCurveOnSurfaceData( double[],double[][4] ) const;
	statusCode				getKnotValue( int, double& ) const;
	statusCode				getControlPoint( int, double[4] ) const;

private:
	static void				initMessages();
	static void				finiMessages();
};

#endif
