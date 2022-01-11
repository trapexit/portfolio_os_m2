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
//  .NAME AlDirectionLight - Encapsulates the functionality common to all directional lights.
//
//  .SECTION Description
//
//		There are two ways that this class is used.  If the user
//		instantiates and calls the create method on an AlDirectionLight,
//		the resulting object is a light that has color, intensity,
//		and direction but no obvious source in a scene.  This type
//		of direction light does not decay with distance.  Sunlight
//		is a direction light of this type.   
//
//		Alternatively, the  user could instantiate and create an
//		AlSpotLight, AlLinearLight, or AlAreaLight (which are all
//		considered "directional" and which are all derived from
//		this light class).  
//
//		Although all direction lights can access the
//		"look at" and "up" nodes of a light, only the spot light
//		uses these nodes.
//		


#ifndef _AlDirectionLight
#define _AlDirectionLight

#include <AlNonAmbientLight.h>

class AlLightNode;

class AlDirectionLight: public AlNonAmbientLight {

public:
    					AlDirectionLight();
	virtual				~AlDirectionLight();
    virtual AlObject*	copyWrapper() const;

	statusCode			create();

	AlObjectType		type() const;
	AlDirectionLight*	asDirectionLightPtr();	

	AlLightNode*  		lookAtNode() const;
	AlLightNode*      	upNode() const;

	virtual statusCode	direction( double&, double&, double& ) const;
	virtual statusCode	setDirection( double, double, double );

private:
    // init/fini inherited
};
#endif
