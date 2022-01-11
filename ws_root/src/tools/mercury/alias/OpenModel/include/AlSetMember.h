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
//	.NAME AlSetMember - Basic Interface to the members of Alias set structures.
//
//	.SECTION Description
//

#ifndef _AlSetMember
#define _AlSetMember

#include <AlObject.h>
#include <AlSet.h>
#include <AlSettable.h>


struct Set;

extern "C" {
	struct ST_SetItemsAVLNode;
}

class AlSetMember : public AlObject {
	friend class AlFriend;

public:
	virtual					~AlSetMember();
	virtual AlObject*		copyWrapper() const;
	
	virtual	AlObjectType 	type() const;
	virtual	AlSetMember*	asSetMemberPtr();

	AlSetMember*			nextSetMember() const;
	AlSetMember*			prevSetMember() const;

	statusCode				nextSetMemberD();
	statusCode				prevSetMemberD();

	AlObject*				object() const;
	AlSet*					set() const;

	statusCode				removeFromSet( AlSet *set );

private:
	Set *fSet;

private:
							AlSetMember( Set *fSet );
	static void				initMessages();
	static void				finiMessages();
};

#endif // _AlSetMember

