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
//  .NAME AlCamera - A base class that encapsulates behavior of perspective
//		  and orthographic cameras.
//
//  .SECTION Description
//		This class used to represent all the functionality of Alias perspective
//		cameras.  When the AlWindow class was born and it became possible to
//		get and set the window associated with a camera, it became necessary
//		not only to provide an interface to orthographic cameras as well, but
//		also to separate the differences in functionality into two child 
//		classes.  The AlCamera class remains as a base class, providing access
//		to the behavior that all cameras share.  In places where AlCameras
//		were formerly used, it is possible to easily convert old code simply
//		by substituting AlPerspectiveCamera for AlCamera almost everywhere.
//		(A few other small modifications may be necessary).
//
//		Any camera can have image planes attached to it.  To create an image
//		plane on a camera, use the addImagePlane command.
//
//		If a window is associated with this camera (which will always be the
//		case for orthographic cameras, but not so for perspective cameras)
//		the firstWindow() function will return it.
//

#ifndef _AlCamera
#define _AlCamera

#include <AlObject.h>
#include <AlImagePlane.h>
#include <AlWindow.h>

#include <AlIterator.h>

class AlCameraNode;

struct Dag_node;
class AlDagNode;

class AlCamera	: public AlObject
{

	friend class			AlFriend;
public:
	virtual					~AlCamera();

	virtual statusCode		deleteObject();
	virtual AlObject*  		copyWrapper() const;

	virtual AlObjectType	type() const;
	virtual AlCamera*		asCameraPtr();

	virtual const char*		name() const;
	virtual statusCode		setName( const char* );

	statusCode				addImagePlane( const char * );
	AlImagePlane*			firstImagePlane( void );
	statusCode				applyIteratorToImagePlanes( AlIterator*, int& );

	AlWindow*				firstWindow( void );
	AlWindow*				nextWindow( AlWindow *lastWindow );
	statusCode				nextWindowD( AlWindow *lastWindow );

	statusCode				nearClippingPlane( double& ) const;
	statusCode				farClippingPlane( double& ) const;

	statusCode				setNearClippingPlane( double );
	statusCode				setFarClippingPlane( double );

	statusCode				stereoView(boolean&, double&) const;
	statusCode				setStereoView(boolean, double);

protected:
							AlCamera();
private:
	static void				initMessages();
	static void				finiMessages();
};

#endif
