#ifndef _AlLineAttributes
#define _AlLineAttributes

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
//	.NAME AlLineAttributes - Interface to Alias line attributes.
//
//	.SECTION Description
//
//		AlLineAttributes is a class derived from the AlAttributes class.
//		This class allows access to the attributes for a line.
//
//		The attributes which define a line are just its start point, and
//		its end point. Coordinates are given in object space, and not
//		world space.
//

#include <AlAttributes.h>

class AlLineAttributes : public AlAttributes {
	friend class			AlFriend;

public:
	virtual AlObjectType			type() const;
	virtual AlLineAttributes* 		asLineAttributesPtr();
	AlObject*						copyWrapper() const;

	statusCode				startPoint(double&, double&, double&) const;
	statusCode				endPoint(double&, double&, double&) const;

	statusCode				setStartPoint(double, double, double);
	statusCode				setEndPoint(double, double, double);

protected:
							AlLineAttributes(struct Spline_surface*);
	virtual					~AlLineAttributes();
};

#endif
