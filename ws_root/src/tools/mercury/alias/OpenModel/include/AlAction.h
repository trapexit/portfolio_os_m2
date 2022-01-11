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
//	.NAME AlAction - Basic interface to Alias actions
//
//	.SECTION Description
//		AlAction is the base class for an alias action.  An action
//	is an entity that will map time to a value.  In Alias, there are
//	two types of actions: parameter curve actions, represented by the
//	derived class AlParamAction, and motion path actions, represented by
//	the derived class AlMotionAction.
//
//		You can create new AlActions by creating an AlParamAction or an
//	AlMotionAction.  Existing actions can be accessed either through the
//	global list of actions (AlUniverse::firstAction(), AlUniverse::nextAction())
//	or through the list of actions which a channel uses to animate a field
//	of an item (AlChannel::appliedAction()).  Note that if you delete an
//	action, it may cause other classes to be deleted (for example, AlKeyframe,
//	and AlChannel if the action was the base action of a channel).
//

#ifndef _AlAction
#define _AlAction

#include <AlObject.h>
#include <AlAnim.h>

extern "C" {
	struct Aa_Action;
}

class AlAction : public AlObject
{
	friend					class AlFriend;
public:
	virtual					~AlAction();
	virtual statusCode		deleteObject();

	virtual AlObjectType	type() const;
	virtual AlAction*		asActionPtr();
	virtual const char*		name() const;
	virtual statusCode		setName(const char *);

	const char*				comment() const;
	AlActionExtrapType		extrapTypePRE() const;
	AlActionExtrapType		extrapTypePOST() const;

	statusCode				setComment(const char *);
	statusCode				setExtrapTypePRE(AlActionExtrapType);
	statusCode				setExtrapTypePOST(AlActionExtrapType);

	AlAction*				copy() const;
	statusCode				copyD();

	double					eval( double, AlTripleComponent = kX_COMPONENT ) const;

	int						numChannelReferences() const;
	AlChannel*				channelReference(const int) const;

protected:
							AlAction();

	virtual AlAction* 		constructAction( Aa_Action* ) const = 0;
private:

	static void				initMessages();
	static void				finiMessages();
};

#endif	// _AlAction
