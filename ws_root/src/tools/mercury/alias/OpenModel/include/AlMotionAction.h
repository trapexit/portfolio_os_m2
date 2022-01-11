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
//	.NAME AlMotionAction - Basic interface to derived class of actions for motion path actions.
//
//	.SECTION Description
//		An AlMotionAction is derived from an AlAction.  This particular
//	kind of action uses an AlCurve (a 3-D NURBS curve) to evaluate itself
//	at different times.  The AlMotionAction is defined over the range of
//	0 to 100 (the pre- and post-extrapolation types of the action apply
//	to the evaluation of times before 0 and after 100).  An AlMotionAction is
//	evaluated by interpreting the "time" between 0 and 100
//	as a percentage arc length along the AlCurve used by this action.
//	This results in an (x, y, z) point on the curve from which one of the
//	components is extracted into a double value using an "extract" component.
//
//	When this action is used in a channel, the channel will also supply
//	an "extract" component (kX_COMPONENT, kY_COMPONENT or kZ_COMPONENT)
//	to determine which of the (x, y, z) values to use as the final evaluation
//	of the action.
//
//		In order to create an AlMotionAction, you must have a valid AlCurveNode
//	with an AlCurve below it.  If you delete
//	the AlMotionAction, the AlCurveNode will not be deleted.  However, if
//	you delete the AlCurveNode (or the AlCurve below the curve node), this
//	will delete the AlMotionAction that uses that AlCurveNode.
//

#ifndef _AlMotionAction
#define _AlMotionAction

#include <AlAction.h>
#include <AlCurveNode.h>

class AlMotionAction : public AlAction {
	friend 					class AlFriend;
public:
							AlMotionAction();
	virtual					~AlMotionAction();
	virtual AlObject*		copyWrapper() const;

	statusCode				create(AlCurveNode *);

	virtual AlObjectType	type() const;
	virtual AlMotionAction*	asMotionActionPtr();

	AlMotionAction*			copy();

	AlCurveNode*			motionCurve() const;

protected:
	virtual AlAction*		constructAction( Aa_Action* ) const;

private:
};

#endif	// _AlMotionAction
