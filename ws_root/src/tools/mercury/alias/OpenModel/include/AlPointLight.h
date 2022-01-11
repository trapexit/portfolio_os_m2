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
//  .NAME AlPointLight - Encapsulates the creation, deletion and manipulation of a point light. 
//
//  .SECTION Description
//
//		Point lights are like incandescent light bulbs.  They throw
//		off light in all directions.
//
//		To create a point light, the user must instantiate and call
//		the create method on an AlPointLight object.  (For more 
//		information on lights in general, see the Class Description
//		of the AlNonAmbientLight object.)
//


#ifndef _AlPointLight
#define _AlPointLight

#include <AlNonAmbientLight.h>

class AlPointLight: public AlNonAmbientLight{

public:
    					AlPointLight();
	virtual				~AlPointLight();
    virtual AlObject*   copyWrapper() const;

	statusCode			create();

	AlObjectType		type() const;
	AlPointLight*		asPointLightPtr();	

private:
    // init/fini inherited
};
#endif
