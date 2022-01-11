#ifndef _AlPolysetNode
#define _AlPolysetNode

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
//	.NAME AlPolysetNode - Dag node class for polysets.
//
//	.SECTION Description
//		AlPolysetNode is the class used to access and manipulate
//		polysets within the dag.  This class behaves like other dag nodes
//		(see AlDagNode for a description of the usage and purpose of dag nodes)
//		except that you are not able to instantiate or create one (which is
//		done when creating a polyset).
// .br
//		The polyset() method returns a pointer to an AlPolyset object which
//		provides you with the methods used to modify the polyset. Polysets can
//		be created from scratch by calling the AlPolyset::create() method, or
//		read in from a wire file (see AlUniverse documentation).
//

#include <AlDagNode.h>

class AlPolysetNode : public AlDagNode {
	friend 					class AlFriend;
  public:

	virtual					~AlPolysetNode();
	virtual AlObject		*copyWrapper() const;

	virtual AlObjectType	type() const;
	virtual AlPolysetNode*	asPolysetNodePtr();

	AlPolyset*				polyset() const;
	AlPolyset*				polyset(AlTM&) const;

protected:
							AlPolysetNode();
private:
};

#endif
