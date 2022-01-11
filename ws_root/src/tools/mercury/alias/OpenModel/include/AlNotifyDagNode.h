//-
//	Copyright (C) 1995, Alias|Wavefront
//
//  These coded instructions,  statements and  computer programs contain
//  unpublished information proprietary to Alias|Wavefront  and are
//  rotected by the Canadian and US Federal copyright law. They may not
//  be disclosed to third parties  or copied  or duplicated, in whole or
//  in part,  without the prior written consent of Alias|Wavefront
//
//  Unpublished-rights reserved under the Copyright Laws of the United States.
//
//
//  $Source: /var/tmp/RCS/AlNotifyDagNode.h,v $
//  $Revision: 1.3 $
//  $Author: ptokarch $
//  $Date: 95/10/15 14:21:18 $
//
//+

//
//	.NAME AlNotifyDagNode - Encapsulates the handling of dag node notifications
//
//	.SECTION Description
//		This class provides a means for an application function to notify
//		Alias of a list of possible dag nodes that will be affected if an
//		object is modified.
//
//  This class is passed to the application function along with an AlObject.
//	For each dag node that is affected if the AlObject is modifed, the
//	application should call AlNotifyDagNode::notify.
//
//	This is a restricted version of an iterator which can only be applied
//	to AlDagNodes.
//

#ifndef _AlNotifyDagNode
#define _AlNotifyDagNode

#include <AlStyle.h>

class AlDagNode;
class AlNotifyDagNode
{
	friend class AlFriend;

public:
	virtual statusCode notify( AlDagNode * ) const;

protected:
	AlNotifyDagNode()			{};
	virtual ~AlNotifyDagNode()	{};
};

#endif // AlNotifyDagNode
