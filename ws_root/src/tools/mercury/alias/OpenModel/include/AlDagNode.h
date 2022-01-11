//-
//	Copyright (C) 1995, Alias|Wavefront
//
//	These coded instructions,  statements and  computer programs contain
//	unpublished information proprietary to Alias|Wavefront  and are
// 	rotected by the Canadian and US Federal copyright law. They may not
//	be disclosed to third parties  or copied  or duplicated, in whole or
//	in part,  without the prior written consent of Alias|Wavefront
//
//	Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+

//
//	.NAME AlDagNode - Basic Interface to Alias Dag Node objects.
//
//	.SECTION Description
//		This class encapsulates the basic functionality for creating,
//		manipulating and deleting a dag node.  
//		A dag node is part of a tree-like hierarchical structure
//		known as a directed acyclic graph or DAG.  All dag nodes belong
//		to a single DAG which is contained in the current universe.  The
//		head of the DAG is accessible by calling the AlUniverse::firstDagNode()
//		static method.
//
//		Each dag node can be mutually
//		attached to a previous and next dag node to form a list of dag
//		nodes.  This list can belong to a group (or parent) dag node.
//		This list of dag nodes that belong to a group node are called
//		"children" of the group node.
//
//		The purpose of a dag node is to contain information which
//		defines particular affine transformations such as scale,
//		rotation and translation.  There is a specific fixed order in
//		which the transformations are combined.  There are methods for
//		accessing each particular transformation and a method exists for
//		obtaining the local tranformation matrix built from these
//		transformations.  For more information, see the description for
//		the method localTransormationMatrix().
//
//		Classes derived from this class will refer to a particular type
//		of object.  For example, the AlCameraNode class is derived from
//		AlDagNode and refers to AlCamera objects only.  The shape,
//		position and/or orientation of the object referred to is
//		defined by combining the transformations for the dag node with
//		all the transformations inherited from the dag nodes above it
//		in the DAG.  There is a method for
//		obtaining the global tranformation matrix, which is built from the local
//		transformations and all inherited transformations.  For example,
//		if dag node A is a parent of dag node B which is a parent of dag
//		node C, then the global transformation of C is the matrix product
//		[C]*[B]*[A], where [C], [B], and [A] are local transformations
//		of each dag node.
//
//		A dag node can have some user-defined text associated with it
//		and so there are methods for getting and setting the text.
//
//		To create a dag node, the user must call the constructor and
//		then the create() method for an AlDagNode object.  This will
//		automatically insert the dag node into the DAG as a parent-less
//		node.  A dag node can be moved so that it is a sibling of any
//		dag node in the DAG.  Deleting a dag node removes the node from
//		its list of siblings.  For derived classes, deletion of a dag
//		node will cause the deletion of the object that it refers to.
//
//		Since an AlDagNode simply contains transformations but does not
//		refer to any transformable object, this class is of little
//		practical use on its own.  It is primarily an abstract base class
//		from which other dag node classes are derived.  NOTE that NULL
//		dag nodes created in the Alias Interactive package are retrieved
//		by this library as group nodes without children.
//
//		What does a transformation matrix mean?  This matrix is the product
//		of matrices for scale, rotate and translate.  One useful piece of
//		information from this matrix is the POSITION of an object.  The first
//		three values in the bottom row of the matrix (indices (3,0), (3,1),
//		and (3,2)) represent the translation of the origin (0,0,0)  in the x, y,
//		and z directions.  For instance, if you placed a sphere at (20, 30, 16)
//		in the Alias interactive package, then moved it to (-30, 16, 28), 
//		you would notice that its global transformation matrix would have
//		(-30, 16, 28) in the bottom row.
//

#ifndef _AlDagNode
#define _AlDagNode

#include <AlClusterable.h>
#include <AlAnimatable.h>
#include <AlSettable.h>
#include <AlPickable.h>
#include <AlTM.h>

// for size_t
#include <stddef.h>

struct Dag_node;

class AlDagNode : public AlObject 
				, public AlClusterable
				, public AlAnimatable
				, public AlSettable
				, public AlPickable
{
	friend 					class AlFriend;
public:
							AlDagNode();
	virtual					~AlDagNode();
	virtual statusCode		deleteObject();
	virtual AlObject*		copyWrapper() const;
	statusCode				create();

	virtual AlAnimatable*	asAnimatablePtr();
	virtual AlSettable*		asSettablePtr();
	virtual AlClusterable*	asClusterablePtr();
	virtual AlPickable*		asPickablePtr();

	virtual AlObjectType	type() const;
	virtual AlDagNode*		asDagNodePtr();
	virtual const char*		name() const;
	virtual statusCode		setName( const char* );

	AlGroupNode*	parentNode() const;

	AlDagNode*		nextNode() const;
	AlDagNode*		prevNode() const;

	virtual boolean	isInstanceable();
	statusCode		addSiblingNode( AlDagNode* );

	statusCode		comment( long&, const char*& );
	statusCode		setComment( long, const char* );
	statusCode		removeComment( void );

	statusCode		blindData( int, long&, const char*& );
	statusCode		setBlindData( int, long, const char* );
	statusCode		removeBlindData( int );

	statusCode		localTransformationMatrix( double[4][4] ) const;
	statusCode		globalTransformationMatrix( double[4][4] ) const;
	statusCode		inverseGlobalTransformationMatrix( double[4][4] ) const;
	statusCode		affectedTransformationMatrix( const AlTM&, double[4][4] ) const;

	statusCode		localTransformationMatrix( AlTM& ) const;
	statusCode		globalTransformationMatrix( AlTM& ) const;
	statusCode		inverseGlobalTransformationMatrix( AlTM& ) const;
	statusCode		affectedTransformationMatrix( const AlTM&, AlTM& ) const;

	statusCode		translation( double&, double&, double& ) const;
	statusCode		rotation( double&, double&, double& ) const;
	statusCode		scale( double&, double&, double& ) const;
	statusCode		rotatePivot( double&, double&, double& ) const;
	statusCode		scalePivot( double&, double&, double& ) const;
	statusCode		rotatePivotIn( double &x, double &y, double &z ) const;
	statusCode		rotatePivotOut( double &x, double &y, double &z ) const;
	statusCode		scalePivotIn( double &x, double &y, double &z ) const;
	statusCode		scalePivotOut( double &x, double &y, double &z ) const;

	statusCode		setTranslation( double, double, double );
	statusCode		setWorldTranslation( double, double, double );
	statusCode		setRotation( double, double, double );
	statusCode		setScale( double, double, double );

	statusCode		setRotatePivot( double, double, double );
	statusCode		setScalePivot( double, double, double );

	statusCode		localRotationAxes( double[3], double[3], double[3] ) const;
	statusCode		localRotationAngles( double&, double&, double& ) const;
	statusCode		setLocalRotationAngles( double, double, double );

	statusCode		boundingBox( double[8][4] ) const;

	boolean			isDisplayModeSet( AlDisplayModeType ) const;
	statusCode		setDisplayMode( AlDisplayModeType, boolean );

	AlJointNode*	jointNode() const;
	statusCode		addJointNode();
	statusCode		removeJointNode();

	AlDagNode*		searchBelow( const char * ) const;
	AlDagNode*		searchAcross( const char * ) const;

	statusCode		updateDrawInfo( void ) const;
	statusCode		sendGeometryModifiedMessage();

	statusCode		doUpdates( boolean newState = TRUE );

protected:
	void			detach();
	void			attachSibling( Dag_node* );

private:
	boolean			updateOn;
	boolean			updateNeeded;
	void			updatePerform();

	virtual	boolean extractType( int&, void*&, void*& ) const;

	static void		initMessages();
	static void		finiMessages();
};

#endif // _AlDagNode
