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
//  .NAME AlLight - Encapsulates methods common to all lights.
//
//  .SECTION Description
//
//		This virtual class contains methods which are common to
//		all types of lights.  This includes color, linkage to 
//		objects and exclusivity.
//
//		To create a light, the user must instantiate and call 
//		the create method of a specific type of light (eg. a 
//		point light or a spot light).  When a light is created, 
//		three light nodes are created, grouped under a group node,
//		which is inserted into the universe's dag.  These light
//		nodes represent the position, "look at" and "up" points
//		of the light.
//
//		Even though three light nodes are created for all lights,
//		only the spot light class uses the information in the
//		"look at" and "up" nodes.  All other classes either
//		don't have a direction or store direction in a different 
//		manner.  The direction vector of a spot light is the
//		vector between its position point and its "look at"
//		point.  The "up" direction vector of a spot light is
//		the vector between its position point and its "up" point.
//
//		There are two ways to delete a light object.  When an
//		AlLight object is deleted, its three light nodes are
//		deleted.  Alternatively, when a light node is deleted, its
//		associated light (and other light nodes) are deleted.
//		The group node that originally grouped the position, "look at" 
//		and "up" nodes is not deleted.
//
//		The light classes are derived as follows, where a class
//		inherits the functionality defined in the class above it.
//		The user can only instantiate ambient, point, direction,
//		spot, linear and area lights.
//
// .ss 24
// .nf
//	%@	               Light
//	%@	                 |
//	%@	           ____________
//	%@	           |           |
//	%@	        Ambient   Non-ambient
//	%@	                       |
//	%@	                ___________
//	%@	                |          |
//	%@	              Point   Directional
//	%@	                         |
//	%@	                   _____________
//	%@	                   |     |      |
//	%@	                  Spot  Linear  Area
// .fi
// .ss 12
//
//		For directional lights, the light positions are (in Z-up coordinate
//		system) position at (0,0,0), view at (0,0,-1), and up at (0,1,-1).
//		For linear lights, the axis by default starts at the position
//		point and extends (2,0,0).  For area lights, the short axis starts
//		at the position point and extends (0,1,0); the long axis starts
//		at the position point and extends (2,0,0).
//
//		All lights have an "exclusive" flag.  If this flag is TRUE, 
//		then the light will only illuminate objects to which it is 
//		linked.  If the flag is FALSE, the light will illuminate 
//		objects that have no light links.  The default for new
//		lights is FALSE.
//

#ifndef _AlLight
#define _AlLight

#include <AlObject.h>
#include <AlAnim.h>
#include <AlAnimatable.h>

class AlLightNode;

struct Dag_node;
struct AR_LightInfo;

class AlFace;
class AlSurface;

class AlLight	: public AlObject
			 	, public AlAnimatable {
	friend class			AlFriend;
public:

	virtual					~AlLight();

	virtual statusCode		deleteObject();

	virtual AlAnimatable*	asAnimatablePtr();

	virtual AlObjectType	type() const;
	virtual const char*		name() const;
	virtual statusCode   	setName( const char* );
	AlLight*				asLightPtr();	

	statusCode				parameter( const AlLightFields, double& ) const;
	statusCode				parameter( const AlLightFields, double* ) const;

	statusCode				setParameter( const AlLightFields, const double );

	AlLightNode*			lightNode() const;
	virtual AlLightNode*	lookAtNode() const;
	virtual AlLightNode*	upNode() const;

	boolean					hasLinkedObjects() const;
	AlObject*				firstLinkedObject() const;
	AlObject*				nextLinkedObject( AlObject * ) const;
	statusCode				applyIteratorToLinkedObjects( AlIterator *iter, int& rc );

	statusCode				linkObjectToLight( AlObject * );
	statusCode				unlinkObjectFromLight( AlObject * );

	boolean					exclusivity() const;
	statusCode				setExclusivity( boolean );

	statusCode				color( double&, double&, double& ) const; 
	statusCode				setColor( double, double, double );

	statusCode				worldPosition( double&, double&, double& ) const;

protected:
							AlLight();
	statusCode				createLight( int lightType );		

	// Methods for accessing the 'look at' and 'up' data.
	//
	Dag_node*				lightDagNode() const;
	Dag_node*				lookAtDagNode() const;
	Dag_node*				upDagNode() const;

private:
	// used by animatable
	virtual boolean extractType( int&, void*&, void*& ) const;

	static void initMessages();
	static void finiMessages();
};
#endif
