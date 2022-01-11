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
//  .NAME AlAmbientLight - Encapsulates the creation, deletion and manipulation of an ambient light.
//
//  .SECTION Description
//
//		An ambient light is similar to a point light, except that only
//		a portion of the illumination comes from the point.  The
//		remainder of the illumination comes from all directions and
//		lights everything uniformly.
//
//		(For more information on lights in general, see the
//		Class Description of the AlLight class.)
//


#ifndef _AlAmbientLight
#define _AlAmbientLight

#include <AlLight.h>

class AlAmbientLight : public AlLight {

public:
    					AlAmbientLight();
	virtual				~AlAmbientLight();

	virtual AlObject*	copyWrapper() const;

	statusCode			create();

	AlObjectType		type() const;
	AlAmbientLight*		asAmbientLightPtr();	

	double				shadeFactor() const;
	double				intensity() const;

	statusCode			setShadeFactor( double );
	statusCode			setIntensity( double );

private:
	// init/fini messages inherited
};
#endif
