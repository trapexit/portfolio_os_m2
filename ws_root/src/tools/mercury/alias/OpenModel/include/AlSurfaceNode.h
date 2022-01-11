#ifndef _AlSurfaceNode
#define _AlSurfaceNode

//-
//	Copyright (C) 1995, Alias|Wavefront
//
//	These coded instructions,  statements and  computer programs contain
//	unpublished information proprietary to Alias|Wavefront  and are
// 	protected by the Canadian and US Federal copyright law. They may not
//	be disclosed to third parties  or copied  or duplicated, in whole or
//	in part,  without the prior written consent of Alias|Wavefront
//
//	Unpublished-rights reserved under the Copyright Laws of the United States.
//+
//
//	.NAME AlSurfaceNode - Dag node class for nurbs surfaces.
//
//	.SECTION Description
//		AlSurfaceNode is the class used to access and manipulate
//		surfaces within the dag.
//		This class behaves like other dag nodes (see AlDagNode for a
//		description of the usage and purpose of dag nodes)
//		and in addition allows users
//		to add curves on surfaces and access the geometry of the surface
//		via the surface() method.  The surface() method returns a
//		pointer to an AlSurface object which provides the user with
//		the methods used to modify the geometry of the surface.
//		Surfaces can be created from scratch by calling 
//		the AlSurfaceNode::create() method,
//		or read in from a wire file (see AlUniverse documentation).
//

#include <AlDagNode.h>

class AlSurface;

class AlSurfaceNode : public AlDagNode {
	friend class			AlFriend;

public:
							AlSurfaceNode();
	virtual					~AlSurfaceNode();
	virtual AlObject*		copyWrapper() const;

	statusCode				create( AlSurface * );

	virtual AlObjectType	type() const;
	virtual AlSurfaceNode*	asSurfaceNodePtr();

	AlSurface*				surface() const;
	AlSurface*				surface(AlTM&) const;

	int						curvePrecision() const;
	statusCode				setCurvePrecision( int precision );

	int						patchPrecision() const;
	statusCode				setPatchPrecision( int precision );

  private:
};

#endif
