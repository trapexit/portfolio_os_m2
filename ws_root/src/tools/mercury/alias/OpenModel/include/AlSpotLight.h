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
//  .NAME AlSpotLight - Encapsulates the creation, deletion and manipulation of
//		a spot light.
//
//  .SECTION Description
//
//		To create a spot light, the user must instantiate and call
//		the create method on an AlSpotLight object.  (For more 
//		information on lights in general, see the Class Description
//		of the AlDirectionLight object.)
//
//		The spot light is the only light that uses the 'look at'
//		and 'up' nodes of a light.  The direction vector of a spot
//		light is the vector between the spot light's position point
//		and 'look at' point.  The up direction vector is the vector 
//		between the spot light's position point and the 'up' point.
//		
//		Other spot light parameters that can be manipulated are
//		spread angle, dropoff, shadows, exclusivity, multiplication
//		factor, offset, size, bias, and penumbra.
//


#ifndef _AlSpotLight
#define _AlSpotLight

#include <AlDirectionLight.h>

class AlSpotLight: public AlDirectionLight{

public:
							AlSpotLight();
	virtual					~AlSpotLight();

	virtual AlObject*		copyWrapper() const;

	statusCode				create();

	AlObjectType			type() const;
	AlSpotLight*			asSpotLightPtr();	

	double					dropOff() const;
	double					minBias() const;
	double					maxBias() const;
	double					spreadAngle() const;
	int						offset() const;
	int						multFactor() const;
	int						shadowSize() const;
	double					penumbra() const;

	virtual statusCode		direction( double&, double&, double& ) const;
	virtual statusCode		setDirection( double, double, double );
	
	statusCode				setDropOff( double );
	statusCode				setMinBias( double );
	statusCode				setMaxBias( double );
	statusCode				setSpreadAngle( double );
	statusCode				setOffset( int );
	statusCode				setMultFactor( int );
	statusCode				setShadowSize( int );
	statusCode				setPenumbra( double);

private:
	// init/fini inherited
};
#endif
