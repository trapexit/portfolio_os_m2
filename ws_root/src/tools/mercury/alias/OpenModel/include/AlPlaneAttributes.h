#ifndef _AlPlaneAttributes
#define _AlPlaneAttributes

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
//	.NAME AlPlaneAttributes - Interface to Alias plane surface attributes.
//
//	.SECTION Description
//
//		AlPlaneAttributes is a class derived from the AlAttributes class.
//		This class allows access to the attributes for an plane.
//
//		The attributes which define a plane are the coefficients of
//		Ax + By + Cz + D = 0 and coordinates of the center of the plane.
//
//		At this time it is only possible to query an AlPlaneAttribute.
//		Further it is currently not possible to create a plane with
//		attributes either in OpenModel or Alias. This attribute is
//		provided for compatibility with IGES, and as such the only
//		way to get a plane with attributes into an Alias wire file
//		is to import an IGES file into Alias and then save a wire
//		file.
//

#include <AlAttributes.h>

class AlPlaneAttributes : public AlAttributes {
	friend class	AlFriend;

public:
	virtual AlObjectType			type() const;
	virtual AlPlaneAttributes* 		asPlaneAttributesPtr();
	AlObject*						copyWrapper() const;

	statusCode		coefficients( double&, double&, double&, double& ) const;
	statusCode		centerPoint( double&, double&, double& ) const;

protected:
					AlPlaneAttributes(struct Spline_surface*);
	virtual			~AlPlaneAttributes();
};

#endif
