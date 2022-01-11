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
//  .NAME AlSphereLight - Add parameters for sphere shaped volume light
//
//  .SECTION Description
//
//		A sphere light created light within a sphere
//


#ifndef _AlSphereLight
#define _AlSphereLight

#include <AlVolumeLight.h>

class AlSphereLight: public AlVolumeLight
{
public:
							AlSphereLight();
	virtual					~AlSphereLight();

	virtual AlObject*   	copyWrapper() const;

	statusCode				create();

	AlObjectType			type() const;
	AlSphereLight*			asSphereLightPtr();	

	double					arc() const;
	statusCode				setArc(double);

private:
	// init/fini inherited
};
#endif
