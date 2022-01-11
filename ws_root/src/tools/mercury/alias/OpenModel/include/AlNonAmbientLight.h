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
//
//+

//
//  .NAME AlNonAmbientLight - Contains functionality common to non-ambient lights.
//
//  .SECTION Description
//
//		This virtual class encapsulates all methods that are common
//		to non-ambient lights such as point, directional, spot, linear,
//		and area lights.  All non-ambient lights have variable 
//		intensity and decay and all have a 'shadows' flag.  
//		
//		To create a non-ambient light, the user must instantiate and
//		call the create method on a specific type of light (as listed
//		above).  For more information on lights in general, see the
//		Class Description of the AlLight object.
//


#ifndef _AlNonAmbientLight
#define _AlNonAmbientLight

#include <AlLight.h>

class AlNonAmbientLight: public AlLight{

public:
	virtual				~AlNonAmbientLight();
    virtual AlObject*	copyWrapper() const = 0;

	AlObjectType		type() const;
	AlNonAmbientLight*	asNonAmbientLightPtr();	

	double				intensity() const;
	int					decay() const;
	boolean				shadows() const;

	statusCode			setIntensity( double );
	statusCode			setDecay( int );
	statusCode			setShadows( boolean );

protected:
	
	// This class is a pure virtual class.  It cannot
	// be instantiated.
						AlNonAmbientLight();
private:
};
#endif
