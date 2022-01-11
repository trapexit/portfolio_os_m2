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
//  .NAME AlPerspectiveCamera - Encapsulates creation, deletion and manipulation of perspective cameras
//
//  .SECTION Description
//
//		This class encapsulates all the functionality for creating,
//		deleting and manipulating a perspective camera.  The user
//		cannot create, delete or manipulate orthographic cameras.
//
//		A camera is made of 4 parts - an AlPerspectiveCamera object and 3 attached
//		AlCameraNodes which represent eye, view and up position of 
//		the camera.  These camera nodes are members of the universe's 
//		dag structure.  
//		
//		The view direction vector is the vector between the eye 
//		position and the view position.  The up direction vector is 
//		the vector between the eye position and the up position.
//		There are methods to get these positions and to access the 
//		attached camera nodes.
//
//		To create a perspective camera, the user must instantiate and 
//		call the create method on an AlPerspectiveCamera object.  
//		This creates the necessary eye, view and up AlCameraNodes,
//		groups them under an AlGroupNode and inserts the group into 
//		the universe's dag structure.  The user cannot instantiate 
//		an AlCameraNode directly. 
//		
//		When a camera is created, if the coordinate system is specified
//		as kZUp (when initializing the universe) the camera's default eye, 
//		view and up positions are respectively (0.0, -12.0, 0.0), 
//		(0.0, 0.0, 0.0), (0.0, -12.0, 1.0).  If the coordinate system is
//		specified as kYUp, then eye, view and up positions are
//		(0.0, 0.0, 12.0), (0.0, 0.0, 0.0), (0.0, 1.0, 12.0).
//
//		There are two ways to delete a camera object.  When the
//		deleteObejct() method of an AlPerspectiveCamera object is called, its
//		three camera nodes are
//		deleted.  Alternatively, when a camera node is deleted, its
//		associated camera (and other camera nodes) are deleted.
//		The group node that originally grouped the eye, view and 
//		up nodes is not deleted.
//

#ifndef _AlPerspectiveCamera
#define _AlPerspectiveCamera

#include <AlObject.h>
#include <AlAnimatable.h>
#include <AlSettable.h>
#include <AlPickable.h>
#include <AlCamera.h>

class AlCameraNode;

typedef void *	Camera_ptr;
struct Dag_node;
class AlDagNode;

class AlPerspectiveCamera	: public AlCamera
							, public AlSettable
							, public AlPickable
							, public AlAnimatable
{

	friend class			AlFriend;
public:
    enum AlCameraWindowFitType {
        kFillFit,
        kHorizontalFit,
        kVerticalFit
    };

public:
							AlPerspectiveCamera();
	virtual					~AlPerspectiveCamera();
	virtual statusCode		deleteObject();
	virtual AlObject*  		copyWrapper() const;
	statusCode				create();

	virtual AlSettable*		asSettablePtr();
	virtual AlPickable*		asPickablePtr();
	virtual AlAnimatable*	asAnimatablePtr();

	virtual AlObjectType	type() const;
	virtual AlPerspectiveCamera*		asPerspectiveCameraPtr();

	AlCameraNode*			eye() const;
	AlCameraNode*			view() const;
	AlCameraNode*			up() const;

	statusCode				worldEye( double&, double&, double& ) const;
	statusCode				worldView( double&, double&, double& ) const;
	statusCode				worldUp( double&, double&, double& ) const;
	statusCode				worldEyeViewUp( double&, double&, double&, double&, double&, double&, double&, double&, double& ) const;

	statusCode				setWorldEye( double, double, double);
	statusCode				setWorldView( double, double, double );
	statusCode				setWorldUp( double, double, double );
	statusCode				setWorldEyeViewUp( double, double, double, double, double, double, double, double, double );

	double					twistAngle() const;

	statusCode				setTwistAngle( double );
	statusCode				changeTwistAngleBy( double );

	statusCode				filmBack(double&, double&) const;
	statusCode				filmOffset(double&, double&) const;
	statusCode				setFilmBack(double, double);
	statusCode				setFilmOffset(double, double);

	double					focalLength() const;
	statusCode				setFocalLength(double);

	double					angleOfView() const;
	statusCode				setAngleOfView( double );

	statusCode				depthOfField(boolean&, double&, double&) const;
	statusCode				setDepthOfField(boolean, double, double);

	int						placementFitCode() const;
	double					placementShift() const;

protected:
	Dag_node				*eyeDagNode() const;
	Dag_node				*viewDagNode() const;
	Dag_node				*upDagNode() const;

private:
	// used by AlAnimatable .. parent is not needed
	virtual boolean extractType( int&, void*&, void*& ) const;

	static void				initMessages();
	static void				finiMessages();
};

#endif
