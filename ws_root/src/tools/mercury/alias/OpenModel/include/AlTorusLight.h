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
//  .NAME AlTorusLight - Add parameters for torus light
//
//  .SECTION Description
//
//		A torus light creates light within a donut.
//		The relative thickness of the donut is set determined by the 
//		TorusRadius. The arc determines whether you get the whole torus or
//		just a wedge cut out of a torus.
//
//		Torus lights are good for halo-shaped particle system effects.
//

#ifndef _AlTorusLight
#define _AlTorusLight

#include <AlVolumeLight.h>

class AlTorusLight: public AlVolumeLight{

public:
    					AlTorusLight();
	virtual				~AlTorusLight();

    virtual AlObject*   copyWrapper() const;

	statusCode          create();

	AlObjectType		type() const;
	AlTorusLight*		asTorusLightPtr();	

	double				arc() const;
	double				torusRadius() const;

	statusCode			setArc(double);
	statusCode			setTorusRadius(double);

private:
    // init/fini inherited
};
#endif
