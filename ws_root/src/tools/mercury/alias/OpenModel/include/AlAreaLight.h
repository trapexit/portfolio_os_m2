//-
//	Copyright (C) 1995, Alias|Wavefront
//
//  These coded instructions,  statements and  computer programs contain
//  unpublished information proprietary to Alias|Wavefront  and are
//  protected by the Canadian and US Federal copyright law. They may not
//  be disclosed to third parties  or copied  or duplicated, in whole or
//  in part,  without the prior written consent of Alias|Wavefront
//
//  Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+

//
//  .NAME AlAreaLight - Encapsulates the creation, deletion and manipulation of area lights.
//
//  .SECTION Description
//
//		An area light is a rectangular area that emits light in all 
//		directions.   An area light would be used, for instance, to
//		simulate bright light coming through a rectangular window.  The size 
//		of the rectangle is defined by a long and short axis.
//
//		To create an area light, the user must instantiate and call
//		the create method on an AlAreaLight object.  (For more 
//		information on lights in general, see the Class Description
//		of the AlDirectionLight object.)
//
//		To determine the orientation of the rectangular area of this light,
//		use the worldPosition() of this light and the method cornerPoints().
//		The cornerPoints() method returns points A and B in world space,
//		as in the following diagram: (WP is the World Position)
//
// .nf
// .ft C
//              WP        B
//              +---------+
//              |         |
//              |         |
//              +---------+
//              A
// .ft P
// .fi
//
//		Although this class inherits methods to access the 'look at'
//		and 'up' nodes of a light, they are not used.  The direction
//		and twist of an area light is changed by translating and rotating
//		the transformation of the light's position node.
//


#ifndef _AlAreaLight
#define _AlAreaLight

#include <AlDirectionLight.h>

class AlAreaLight: public AlDirectionLight{

public:

    					AlAreaLight();
	virtual				~AlAreaLight();
	statusCode			create();
    virtual AlObject*	copyWrapper() const;

	AlObjectType		type() const;
	AlAreaLight*		asAreaLightPtr();	

	statusCode			longAxis( double&, double&, double& ) const;
	statusCode			shortAxis( double&, double&, double& ) const;
	
	statusCode			setLongAxis( double, double, double );
	statusCode			setShortAxis( double, double, double );
	statusCode			cornerPoints( double&, double&, double&, double&, double&, double&) const;

private:
	// init/fini inherited
};
#endif
