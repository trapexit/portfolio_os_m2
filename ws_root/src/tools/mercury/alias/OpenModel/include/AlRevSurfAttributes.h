#ifndef _AlRevSurfAttributes
#define _AlRevSurfAttributes

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
//	.NAME AlRevSurfAttributes - Interface to Alias arc curve attributes.
//
//	.SECTION Description
//
//		AlRevSurfAttributes is a class derived from the AlAttributes class.
//		This class allows access to some of the attributes of a revolved
//		surface, in this case the start and end angles of revolution.
//
//		When querying the attributes of a revolved surface (retrieved
//		from IGES or created in Alias with exact ON) the first
//		attribute will be an AlRevSurfAttributes, the second will be
//		an AlLineAttributes representing the axis of revolution, and
//		the remainder of the attributes describe the line which was
//		revolved to generate the surface.
//

#include <AlAttributes.h>

class AlRevSurfAttributes : public AlAttributes {
	friend class			AlFriend;

public:
	virtual AlObjectType			type() const;
	virtual AlRevSurfAttributes*	asRevSurfAttributesPtr();
	AlObject*						copyWrapper() const;

	double					startAngle() const;
	double					endAngle() const;

protected:
							AlRevSurfAttributes(struct Spline_surface*);
	virtual					~AlRevSurfAttributes();
};

#endif
