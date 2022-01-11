#ifndef _AlTextureNode
#define _AlTextureNode

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
//
//+
//
//	.NAME AlTextureNode - Dag node class for solid textures.
//
//	.SECTION Description
//		AlTextureNode is the class used to access and manipulate
//		the transformations on solid textures within the dag.
//		This class behaves like other dag nodes (see AlDagNode for a
//		description of the usage and purpose of dag nodes) except that
//		you are not able to instantiate or create one.
//

#include <AlDagNode.h>

class AlTexture;
class AlFile;

class AlTextureNode : public AlDagNode {
	friend class			AlFriend;

  public:

	virtual AlObjectType	type() const;
	virtual AlTextureNode*	asTextureNodePtr();
	virtual AlObject*		copyWrapper() const;

	AlTexture*				texture() const;
	AlTexture*				texture(AlTM&) const;

protected:
	AlTextureNode();
	virtual					~AlTextureNode();
	statusCode				create( AlTexture*, struct Dag_node*, AlFile* );

private:
	AlTexture*				alTexture;
};

#endif
