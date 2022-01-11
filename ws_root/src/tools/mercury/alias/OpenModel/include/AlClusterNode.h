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
//  .NAME AlClusterNode - is the dag node class for clusters.
//
//  .SECTION Description
//
//
//		This class is a dag node class used specifically for clusters.
//		Every AlClusterNode object has an AlCluster attached to it.
//		(For more information on how AlCluster's and AlClusterNode's 
//		work together, see the Class Description for the AlCluster object.)
//		
//		To create a cluster node, the user must instantiate and call
//		the create method on an AlCluster object, which will automatically
//		create an AlClusterNode.  The user cannot directly instantiate 
//		an AlClusterNode.
//		
//		A cluster node can be deleted in two ways.  When a cluster node
//		is deleted, its associated cluster is deleted.  When a
//		cluster is deleted, its cluster node is also deleted.
//

#ifndef _AlClusterNode
#define _AlClusterNode

#include <AlDagNode.h>

class AlClusterNode : public AlDagNode {
	friend class			AlFriend;
public:

	virtual					~AlClusterNode();
	virtual AlObject*		copyWrapper() const;

	virtual AlObjectType	type() const;
	virtual AlClusterNode*	asClusterNodePtr();

	virtual boolean			isInstanceable();

	AlCluster				*cluster() const;
	AlCluster				*cluster(AlTM&) const;

protected:
	// AlClusterNode is a virtual class.  It
	// can only be instantiated by an AlCluster
	// object.
	//
							AlClusterNode();
};

#endif
