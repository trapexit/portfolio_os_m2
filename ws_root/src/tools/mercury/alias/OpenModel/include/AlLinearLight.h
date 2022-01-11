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
//  .NAME AlLinearLight - Encapsulates the creation, deletion, and manipulation of linear lights.
//
//  .SECTION Description
//
//		A linear light emits light in all directions.
//		A linear light would be used, for instance, to simulate a 
//		fluorescent light tube.  The length of the line is defined
//		by the light's axis.
//
//		To determine the endpoints of this light, use the worldPosition()
//		method and the endpoint() method.
//
//      Although this class inherits methods to access the 'look at'
//      and 'up' nodes of a light, they are not used.  The direction
//      and twist of an area light is changed by translating and rotating
//      the transformation of the light's position node.
//


#ifndef _AlLinearLight
#define _AlLinearLight

#include <AlDirectionLight.h>

class AlLinearLight: public AlDirectionLight{

public:
    					AlLinearLight();
	virtual				~AlLinearLight();

    virtual AlObject*   copyWrapper() const;

	statusCode			create();

	AlObjectType		type() const;
	AlLinearLight*		asLinearLightPtr();	

	statusCode			axis( double&, double&, double& ) const;
	statusCode			setAxis( double, double, double );

	statusCode			endpoint( double&, double&, double& ) const;

private:
	// init/fini inherited
};
#endif
