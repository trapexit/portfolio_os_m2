#ifndef _AlConicAttributes
#define _AlConicAttributes

/*
//-
//	Copyright (C) 1995, Alias|Wavefront
//
//	These coded instructions, statements and computer programs contain
//	unpublished information proprietary to Alias|Wavefront  and are
// 	protected by the Canadian and US Federal copyright law. They may not
//	be disclosed to third parties or copied or duplicated, in whole or
//	in part, without the prior written consent of Alias|Wavefront
//
//	Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+
//
//	.NAME AlConicAttributes - Interface to Alias conic curve attributes.
//
//	.SECTION Description
//
//		AlConicAttributes is a class derived from the AlAttributes class.
//		This class allows access to the attributes for a conic section.
//
//		Conics may not be created. They may be brought into Alias
//		through the IGES interface though.
//
//		A conic is one of ellipse, hyperbola, parabola, circle, or
//		line. Conics exist in the XY plane and are defined by the
//		equation: A * X^2 + B * X * Y + C * Y^2 + D * X + E * Y + F = 0.
//
*/

typedef enum { kNoConic = 0, kEllipse = 1, kHyperbola = 2, kParabola = 3, kCircle = 4, kLine = 5 } AlConicType;

#ifdef __cplusplus

#include <AlAttributes.h>
#include <AlTM.h>

class AlConicAttributes : public AlAttributes {
	friend class					AlFriend;

public:
	virtual AlObjectType			type() const;
	virtual AlConicAttributes* 		asConicAttributesPtr();
	AlObject*						copyWrapper() const;

	statusCode				coefficients( double&, double&, double&, double&, double&, double& ) const;
	double					zDisplacement() const;
	statusCode				startPoint( double&, double& ) const;
	statusCode				endPoint( double&, double& ) const;
	statusCode				centerPoint( double&, double& ) const;
	statusCode				transform( double[4][4] ) const;
	statusCode				transform( AlTM& ) const;
	AlConicType				form() const;

protected:
							AlConicAttributes(struct Spline_surface*);
	virtual					~AlConicAttributes();
};

#endif	/* __cplusplus */

#endif	/* _AlConicAttributes */
