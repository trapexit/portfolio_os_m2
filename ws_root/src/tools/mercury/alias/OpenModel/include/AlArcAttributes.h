#ifndef _AlArcAttributes
#define _AlArcAttributes

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
//	.NAME AlArcAttributes - Interface to Alias arc curve attributes.
//
//	.SECTION Description
//
//		AlArcAttributes is a class derived from the AlAttributes class.
//		This class allows access to the attributes for an arc.
//
//		The attributes which define an arc are its radius, sweep
//		angle, center point, start point, and end point.
//		Coordinates are given in object space, and not world space.
//

#include <AlAttributes.h>

class AlArcAttributes : public AlAttributes {
	friend class			AlFriend;

public:
	virtual AlObjectType			type() const;
	virtual AlArcAttributes* 		asArcAttributesPtr();
	AlObject*						copyWrapper() const;

	statusCode				centerPoint(double&, double&, double&) const;
	statusCode				startPoint(double&, double&, double&) const;
	statusCode				endPoint(double&, double&, double&) const;
	double					sweep() const;
	double					radius() const;

	statusCode				setStartPoint(double, double, double);
	statusCode				setEndPoint(double, double, double);
	statusCode				setSweepFromStartPoint(double);
	statusCode				setSweepFromEndPoint(double);
	statusCode				setRadius(double);

protected:
							AlArcAttributes(struct Spline_surface*);
	virtual					~AlArcAttributes();
};

#endif
