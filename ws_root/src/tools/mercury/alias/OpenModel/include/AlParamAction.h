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
//	.NAME AlParamAction - Basic interface to derived class of actions for parameter curve actions.
//
//	.SECTION Description
//		An AlParamAction is derived from an AlAction.  This particular
//	action has a list of AlKeyframes which are CVs on a Poon-Ross spline.
//	This spline is basically a Hermite-linear spline.  That is, it is
//	a Hermite in y (the vertical axis) and a linear in x (the horizontal
//	axis).
//
//		In order to create an AlParamAction, you must have at least one
//	valid, created AlKeyframe which will be the first keyframe of the
//	action.  After the AlParamAction is created, you can add other keyframes
//	to the AlParamAction.  Note that two AlKeyframes cannot have the same
//	location.  If you add a keyframe to the action that has the same location
//	as an existing keyframe of the action, the existing keyframe will be
//	deleted.  Since an AlKeyframe is created at (0, 0), you cannot create
//	a bunch of AlKeyframes, add them to the AlParamAction, and then modify
//	their locations later, since the AlParamAction will only have one
//	keyframe (the others will have been deleted as each successive keyframe
//	with the same location is added to the AlParamAction).  You must set
//	the location of the AlKeyframe before adding it to the AlParamAction.
//
//		If you copy an AlParamAction, all the keyframes (and the keyframes'
//	streams) will also be copied.  If you delete an AlParamAction, all
//	its Keyframes will be deleted.
//

#ifndef _AlParamAction
#define _AlParamAction

#include <AlIterator.h>
#include <AlAction.h>
#include <AlKeyframe.h>
#include <AlList.h>

class AlParamAction : public AlAction {
	friend					class AlFriend;
	friend					class AlDictionaryOperators;

public:
							AlParamAction();
	virtual					~AlParamAction();
	virtual AlObject*		copyWrapper() const;

	virtual AlObjectType	type() const;
	virtual AlParamAction*	asParamActionPtr();

	AlKeyframe*				firstKeyframe() const;
	AlKeyframe*				lastKeyframe() const;

	statusCode				addKeyframe( double, double, AlKeyframe *&, boolean = TRUE, AlTangentType = kTangentFlat, AlTangentType = kTangentFlat );
	statusCode				addKeyframe( double, double, AlKeyframe &, boolean = TRUE, AlTangentType = kTangentFlat, AlTangentType = kTangentFlat );
	statusCode				addKeyframe( double, double, boolean = TRUE, AlTangentType = kTangentFlat, AlTangentType = kTangentFlat );

	int						numberOfKeyframes() const;

	statusCode 				applyIteratorToKeyframes( AlIterator*, int& );

protected:
	virtual AlAction*		constructAction( Aa_Action* ) const;

private:
	static void				realRecomputeTangents( Aa_Action* );
	statusCode				create();
};

#endif	// _AlParamAction
