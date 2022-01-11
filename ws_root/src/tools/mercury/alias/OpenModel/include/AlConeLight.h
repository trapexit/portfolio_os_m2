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
//  .NAME AlConeLight - Adds parameters for Cone shaped Volume Lights
//
//  .SECTION Description
//
//		A cone light allows light to exist inside the volume of a cone.
//		
//		A cone light could be made similar to a spotlight by setting
//		Concentric and radial to 0, directional to 1, decay to 0
//		and dropoff to slightly less than 1.
//
//		A cone light would make a good vortex field by setting the
//		Radial to 1, concentric to .5, directional depending on whether
//		the vortex is sucking up things or dropping them.
//


#ifndef _AlConeLight
#define _AlConeLight

#include <AlVolumeLight.h>

class AlConeLight: public AlVolumeLight{

public:
    					AlConeLight();
	virtual				~AlConeLight();

	virtual AlObject*	copyWrapper() const;

	statusCode          create();

	AlObjectType		type() const;
	AlConeLight*		asConeLightPtr();	

	double 				arc() const;
	double 				coneEndRadius() const;
	
	statusCode			setArc(double);
	statusCode			setConeEndRadius(double);

private:
    // init/fini inherited
};
#endif
