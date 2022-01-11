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
//  .NAME AlCameraNode - Dag node class for cameras.
//
//  .SECTION Description
//
//		This class is a dag node class used specifically for cameras.
//		Each AlCamera object has three camera nodes. One for each of eye,
//		view and
//		up positions.  (For more information on how AlCamera's and
//		AlCameraNode's work together, see the Class Description for
//		the AlCamera object.)
//
//		To create a camera node, the user must create a camera,
//		which will automatically create the necessary camera nodes.
//		The user cannot directly instantiate an AlCameraNode.
//
//		To figure out which position a camera node represents, the
//		user can use:  
// .br
//			1) the isEyeNode(), isViewNode(), isUpNode() methods, or
// .br
//			2) the type() method to compare types with kCameraEyeType,
//				kCameraUpType, or kCameraViewType.
// 
//		A camera node can be deleted in two ways.  When a camera node
//		is deleted, its associated camera (and other camera nodes)
//		are deleted.  When a camera is deleted, its camera nodes
//		are also deleted.
//

#ifndef _AlCameraNode
#define _AlCameraNode

#include <AlDagNode.h>

class AlCamera;

extern "C" {
	struct Dag_node;
}

class AlCameraNode : public AlDagNode {
	friend class AlFriend;
	friend class AlCamera;

public:
	virtual					~AlCameraNode();
	virtual AlObject*		copyWrapper() const;
	virtual statusCode		deleteObject();

	virtual AlObjectType	type() const;
	virtual AlCameraNode*	asCameraNodePtr();	
	virtual boolean			isInstanceable();

	boolean				isEyeNode() const;
	boolean				isViewNode() const;
	boolean				isUpNode() const;

	AlCamera*			camera() const;
	AlCamera*			camera(AlTM&) const;

private:
    AlCameraNode( AlObjectType thisType, Dag_node *node );

	//	Store the object type
	//
	AlObjectType		fType;
};
#endif
