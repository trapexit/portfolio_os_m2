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
//  .NAME AlCylinderLight - Add parameters for cylinder volume lights
//
//  .SECTION Description
//
//		A cylinder light creates light within a cylinder.
//


#ifndef _AlCylinderLight
#define _AlCylinderLight

#include <AlVolumeLight.h>

class AlCylinderLight: public AlVolumeLight{

public:
    					AlCylinderLight();
	virtual				~AlCylinderLight();
    virtual AlObject*	copyWrapper() const;

	statusCode			create();

	AlObjectType		type() const;
	AlCylinderLight*	asCylinderLightPtr();	

	double				arc() const;
	statusCode			setArc(double);

private:
    // init/fini inherited
};
#endif
