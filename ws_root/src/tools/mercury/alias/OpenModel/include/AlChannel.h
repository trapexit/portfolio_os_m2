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
//	.NAME AlChannel - Basic interface to Alias channels
//
//	.SECTION Description
//		A channel controls how a field of an animatable item should change
//	over time.  It contains one or more actions which are evaluated at a
//	given time and combined to produce an overall value of the channel at
//	that time.  When a channel is evaluated, for example at playback time,
//	or, in the method AlViewFrame::viewFrame(), the value it is evaluated to
//	is "stuffed" into the parameter of the item, thereby changing the
//	value of that item's parameter over time.
//
//		A channel belongs solely to one field or parameter, of an
//	animatable item.  A parameter of an item is animated
//	by at most one channel.  Thus, the create() methods will fail if
//	an attempt is made to create a channel of a field that already has
//	a channel (i.e. is already animated).  Under similar conditions, the copy()
//	method will fail.
//
//		Currently a channel must contain at least one action (the base
//	action of the channel), and thus the create() method requires you to
//	supply this base action.  You can modify this base action using the
//	link() method.  The applyWarp() and removeWarp() methods modify the
//	timewarp actions applied to the base action of the channel.  They
//	cannot affect the base action of the channel.
//
//		The numAppliedActions() method will tell you how many actions
//	are currently used by channel.  This number will always be at
//	least 1, since a channel must be animated by at least a base action.
//	The appliedActions() will tell you which actions animate the channel.
//	appliedActions(1) is the base action, and appliedActions(2 to n) are
//	the timewarp actions, where n is numAppliedActions().  If any of
//	the actions are an AlMotionAction, then you may also want to know
//	which of the X, Y or Z components the channel is using from the
//	AlMotionAction (since this type of action evaluates to a triple of values).
//	The method appliedActionComponent() can be used to determine this.
//

#ifndef _AlChannel
#define _AlChannel

#include <AlObject.h>
#include <AlAnim.h>
#include <AlChanData.h>

class AlAction;
class AlAnimatable;

struct Aa_Channel_s;
typedef struct Aa_Channel_s Aa_Channel;

class AlChannel : public AlObject {
	friend class			AlFriend;
public:
							AlChannel();
	virtual					~AlChannel();
	virtual statusCode		deleteObject();
	virtual AlObject*		copyWrapper() const;

	statusCode				create( AlAnimatable*, int, AlAction*, AlTripleComponent = kX_COMPONENT );
	statusCode				create( AlAnimatable *, int, const char *);

	virtual AlObjectType	type() const;
	virtual AlChannel*		asChannelPtr();

	AlObject*				animatedItem() const;
	int 					parameter() const;
	const char*				parameterName() const;
	AlChannelDataType		channelType() const;

	AlChannel*				copy( AlAnimatable*, int );
	statusCode				copyD( AlAnimatable*, int );

	statusCode				link(AlAction*, AlTripleComponent);

	statusCode				applyWarp();
	AlParamAction*			applyWarpO();

	statusCode				removeWarp(AlAction*);

	double					eval( double ) const;

	int						numAppliedActions() const;
	AlAction*				appliedAction(const int) const; 
	AlTripleComponent		appliedActionComponent( const int ) const;

	const char*				expressionString () const;

private:

	static void				initMessages();
	static void				finiMessages();
};

#endif	// _AlChannel
