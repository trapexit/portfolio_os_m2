#ifndef _AlAttributes
#define _AlAttributes

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
//	.NAME AlAttributes - Interface to Alias curve and surface attributes.
//
//	.SECTION Description
//
//		AlAttributes is the base class for a set of classes which
//		allow access to curve and surface attributes. It is not
//		possible to create AlAttributes directly, instead they are
//		created when you use the AlCurve::create*() methods.
//		
//		An attribute is an alternate way to specify an
//		object.  For example, in Alias normally a circle would be
//		represented by a NURBS curve which approximates a circle.
//		With attributes instead, a circle is represented by an
//		origin, a radius, etc.  Several types of attributes are
//		derived from this class which are used for particular
//		types of objects.
//

#include <AlObject.h>

class AlAttributes : public AlObject {
	friend class				AlFriend;

public:
	virtual						~AlAttributes();
	virtual AlObjectType		type() const;
	virtual AlAttributes* 		asAttributesPtr();
	AlObject*					copyWrapper() const;

	AlAttributes*				nextAttribute() const;

protected:
								AlAttributes(struct Spline_surface*);
	Spline_surface*				fParent;

private:
	static void					initMessages();
	static void					finiMessages();
};

#endif
