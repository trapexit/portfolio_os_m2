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
//  .NAME AlLightNode - is the dag node class for lights.
//
//  .SECTION Description
//
//		This class is a dag node class used specifically for lights.
//		Each AlLight object has three light nodes for position, 'look at'
//		and 'up' points.  (For more information on how AlLight's
//		and AlLightNode's work together, see the Class Description for
//		the AlLight object.)
//
//		To create a light node, the user must create a specific type
//		of light, which automatically creates the necessary light
//		nodes.  These light nodes are grouped and inserted into
//		the universe's dag.  The user cannot directly instantiate
//		a light node.
//
//      To figure out whether or not a light node represents a position,
//		a 'look at' or an 'up' point, the user can use:  
// .br
//          1) the isPositionNode(), isLookAtNode(), isUpNode() methods, or
// .br
//          2) the type() method of the attached AlLight object.
// .in
//
//      A light node can be deleted in two ways.  When a light node
//      is deleted, its associated light (and other light nodes)
//      are deleted.  Alternatively, when a light is deleted, its light 
//      nodes are also deleted.
//


#ifndef _AlLightNode
#define _AlLightNode

#include <AlDagNode.h>

class AlLight;

struct Dag_node;
struct AR_LightInfo;

class AlLightNode: public AlDagNode {
	friend class AlFriend;

public:

	virtual				~AlLightNode();
	virtual AlObject	*copyWrapper() const;

	statusCode			deleteObject();

	AlObjectType		type() const;
	AlLightNode*		asLightNodePtr();	

	AlLight*			light() const;
	AlLight*			light(AlTM&) const;

	virtual boolean		isInstanceable();
	boolean				isLookAtNode() const;
	boolean				isUpNode() const;
	boolean				isPositionNode() const;

protected:
	// This class can only be instantiated by an AlLight object.
	//
	AlLightNode();
};
#endif
