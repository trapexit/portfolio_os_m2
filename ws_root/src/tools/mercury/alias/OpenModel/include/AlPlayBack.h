/*
//-
//	Copyright (C) 1995, Alias|WaveFront
//
//	These coded instructions,  statements and  computer programs contain
//	unpublished information proprietary to Alias Research, Inc.  and are
// 	rotected by the Canadian and US Federal copyright law. They may not
//	be disclosed to third parties  or copied  or duplicated, in whole or
//	in part,  without the prior written consent of Alias|WaveFront
//
//	Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+
*/

/*
//
//	.NAME AlPlayBack - A static class for managing animation playback.
//
//	.SECTION Description
//		These set of static methods provide a plugin developer with the
//		ability to initiate and manage animation playback.  A callback
//		facility is also provided, which allows polling type plugins to
//		perform a limited set of operations after each frame has been
//		displayed.
//
//      Be VERY VERY CAREFUL about what you do in this callback.  As part of
//      its optimisation technique, a lot of animation information is
//      cached outside of the normal messaging system.  This means that
//      if you delete some animation objects while playback is active
//      then at some stage PowerAnimator will become very confused
//		(as in core dump confused).
//
//		Note that this class only functions in OpenAlias. In OpenModel
//		the class exists but does not do anything.
//
*/

#ifndef _AlPlayBack
#define _AlPlayBack

#include <AlStyle.h>

typedef	void (AlCallBack (void));

class AlPlayBack {
public:
static void playForward ();
static void playReverse ();
static void stop ();
static boolean inPlayBack ();
static void nextFrame ();
static void previousFrame ();
static void nextKeyframe ();
static void previousKeyframe ();
static void gotoStart ();
static void gotoEnd ();
static void gotoFrame (const double frame);
static AlCallBack *setCallBack (AlCallBack *callBack);
static void getStartEndBy (double &start, double &end, double &by);

private:
	// Do not create an instance of this
	//
	AlPlayBack ();
	~AlPlayBack ();
};


#endif // _AlPlayBack
