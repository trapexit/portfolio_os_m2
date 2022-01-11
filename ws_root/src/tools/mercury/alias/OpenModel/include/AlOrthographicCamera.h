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
//  .NAME AlOrthographicCamera  - orthographic modeling cameras.
//
//  .SECTION Description
//		This class provides an interface to orthographic cameras, primarily
//		so that one can manipulate the image planes on those cameras.  In
//		general, little can be done to ortho cameras, as they are more of
//		a modeling tool than geometry.  In that regard, this class contains
//		little functionality.
//
//		It is possible to create a new orthographic camera through the
//		create() call.  The valid parameters to that call are defined in 
//		AlWindow.h.  Note that creating an orthographic camera automatically
//		creates the associated modeling window.
//

#ifndef _AlOrthographicCamera
#define _AlOrthographicCamera

#include <AlObject.h>
#include <AlCamera.h>
#include <AlWindow.h>

typedef void *	Camera_ptr;

class AlOrthographicCamera	: public AlCamera
{

	friend class			AlFriend;
public:
							AlOrthographicCamera();
	virtual					~AlOrthographicCamera();
	virtual statusCode		deleteObject();
	virtual AlObject*  		copyWrapper() const;

	statusCode				create( AlWindow::AlViewType );

	virtual AlObjectType	type() const;
	virtual AlOrthographicCamera*	asOrthographicCameraPtr();

protected:
private:
};

#endif
