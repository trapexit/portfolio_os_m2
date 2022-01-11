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
//	.NAME AlGroupNode - A dag node which can contain a list of dag nodes.
//
//  .SECTION Description
//
//		This class encapsulates the functionality for creating,
//		manipulating and deleting a group node.  A group node is a dag
//		node which refers to a list of child dag nodes.  It is this type
//		of dag node which allows the hierarchical grouping of dag nodes.
//
//		The transformations which can be defined for a group node are
//		inherited by each child dag node.  This means that a
//		group node's transformations are combined with a child node's
//		transformations to define a global transformation for the object
//		that the child node refers to.
//
//		A group node's list of child dag nodes can be shared by more
//		than one group node.  If a group node's list of child nodes is
//		shared by another group node, both group nodes are considered
//		"instanced".  This can be achieved by using the createInstance()
//		method to create an instanced group node from another group
//		node.  The instanced group node is created as a sibling of the
//		group node.  There are methods for finding the next and previous
//		instanced group node among its siblings and for determining
//		whether a group node is an instanced node.
//
//		To create a group node, the user must call the constructor and
//		then the create method for an AlGroupNode object.  If a group
//		node is not an instanced group node, deleting it will cause the
//		deletion of all the child dag nodes and the deletion of any
//		objects the child dag nodes refer to.  Deleting an instanced
//		group node will not cause all of its child nodes to be deleted
//		since the list of child nodes is shared by another instanced
//		group node.
//
//		Note on AlGroupNode::deleteObject():
//
//      If a group node is an instanced group node, then only the group
//		node is removed from its list of siblings and is deleted.
//		The list of child dag nodes an instanced dag node refers to is
//		not deleted.  If this group node
//      is not an instanced group node (i.e. none of its siblings share
//      its list of child dag nodes), then the group node is removed
//      from list of siblings it belongs to and the group node and
//      every child node of the group node is deleted.
//

#ifndef _AlGroupNode
#define _AlGroupNode

#include <AlDagNode.h>

class AlIterator;

class AlGroupNode : public AlDagNode {
	friend class			AlFriend;
public:
							AlGroupNode();
	virtual					~AlGroupNode();
	virtual AlObject*		copyWrapper() const;
	statusCode				create();

	virtual AlObjectType	type() const;
	virtual AlGroupNode*	asGroupNodePtr();

	virtual AlDagNode*		childNode() const;
	virtual AlDagNode*		childNode(AlTM&) const;

	virtual AlGroupNode*	nextInstance() const;
	virtual AlGroupNode*	prevInstance() const;

	virtual statusCode		nextInstanceD();
	virtual statusCode		prevInstanceD();

	virtual boolean			isInstanceable();
	boolean					isInstanceNode();
	boolean					isAncestorAnInstance();

	statusCode				addChildNode( AlDagNode* );
	AlGroupNode*			createInstance();

	statusCode				applyIteratorToChildren( AlIterator*, int& );

protected:
	statusCode				attachChild( AlDagNode* );
};

#endif // _AlGroupNode
