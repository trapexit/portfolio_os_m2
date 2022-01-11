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
//  .NAME AlBoxLight - A class used to reference a box light
//
//  .SECTION Description
//
//		A box light is a volume light where light exists within a cube.
//		There are no additional parameters other than those already on
//		the volume light.
//
//		A box light could be np-scaled and used to fill a room with light.
//		If it is used as a force field, it could fill the room with turbulence,
//		leaving the air outide the room still.
//


#ifndef _AlBoxLight
#define _AlBoxLight

#include <AlVolumeLight.h>

class AlBoxLight: public AlVolumeLight{

public:
    					AlBoxLight();
	virtual				~AlBoxLight();
	statusCode			create();
    virtual AlObject*	copyWrapper() const;

	AlObjectType		type() const;
	AlBoxLight*			asBoxLightPtr();	

private:
	// init/fini inherited
};
#endif
