#ifndef _AlShellNode
#define _AlShellNode

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
//	.NAME AlShellNode - Dag node class for shells.
//
//	.SECTION Description
//		This method provides DagNode level access to Shells.  Use the shell
//		method to get the actual AlShell underneath this DagNode.
// .br
//

#include <AlDagNode.h>

class AlShell;

class AlShellNode : public AlDagNode {
	friend 					class AlFriend;
  public:
	virtual					~AlShellNode();

	virtual AlObject		*copyWrapper() const;

	virtual AlObjectType	type() const;
	virtual AlShellNode*	asShellNodePtr();

	AlShell*				shell() const;
	AlShell*				shell(AlTM&) const;

protected:
							AlShellNode();
private:
};

#endif
