/*
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
//	.NAME AlKeyframe - Basic interface to Alias keyframes on parameter
//						curve actions.
//
//	.SECTION Description
//		AlKeyframe represents a keyframe belonging to an AlParamAction.
//	The keyframe class does not have a create method.  New keyframes aer
//	created using the AlParamAction::addKeyframe() methods.
//	(this is faster than creating the keyframes, then adding them to the
//	paramaction).
//
//		A keyframe cannot belong to more than one action.  Thus attempting
//	to add a keyframe that has already been added to an AlParamAction
//	will fail.
//
//		If an AlKeyframe is deleted and if the AlKeyframe belongs to an
//	AlParamAction and it is the last keyframe of that parameter curve
//	action, then the AlParamAction will also be deleted.  This ensures that
//	no AlParamAction parameter curves will exist with no keyframes.
//
//		If a keyframe is added to an AlParamAction, and another keyframe
//	already exists in that AlParamAction with the same location as the
//	keyframe to be added, then the existing keyframe in the AlParamAction
//	will be deleted.
//
//		The method that applies tangent types to a keyframe (i.e.
//	setTangentTypes()) does not make much sense if the keyframe is not part
//	of an AlParamAction.  It is best to add all the desired keyframes
//	to an AlParamAction, and then walk the list of keyframes, and calling
//	the tangent methods.
//
//		You can lock a keyframe by calling the method AlKeyframe::setLock(TRUE).
//	If an AlKeyframe is locked, then none of the AlKeyframe
//	methods that change its location, value or tangents will succeed.
//	The keyframe is non-modifiable.  You can unlock the keyframe again by
//	calling AlKeyframe::setLock(FALSE).
//
*/

#ifndef _AlKeyframe
#define _AlKeyframe

typedef enum {
	kTangentSmooth, kTangentLinear, kTangentFlat, kTangentStep,
	kTangentSlow, kTangentInOut, kTangentFast, kTangentFixed, kTangentUnchanged
} AlTangentType;

#ifdef __cplusplus

#include <AlObject.h>
#include <AlParamAction.h>

class AlKeyframe : public AlObject {
	friend				class AlFriend;
public:
						AlKeyframe();
	virtual				~AlKeyframe();
	virtual statusCode	deleteObject();
	virtual AlObject*	copyWrapper() const;

	virtual AlObjectType	type() const;
	virtual AlKeyframe*		asKeyframePtr();

	boolean				isLocked() const;
	statusCode			setLock(boolean);

	AlKeyframe*			prev() const;
	AlKeyframe*			next() const;

	statusCode			prevD();
	statusCode			nextD();

	double				value() const;
	double				location() const;
	double				inTangent() const;
	double				outTangent() const;
	statusCode			tangentTypes( AlTangentType&, AlTangentType& ) const;

	statusCode			setValue(double, boolean = TRUE );
	statusCode			setLocation(double, boolean = TRUE );
	statusCode			setInTangent(double);
	statusCode			setOutTangent(double);
	statusCode			setTangentTypes(AlTangentType, AlTangentType, boolean = TRUE );

private:
	static void			initMessages();
	static void 		finiMessages();
};

#endif	/* __cplusplus */
#endif	/* _AlKeyframe */
