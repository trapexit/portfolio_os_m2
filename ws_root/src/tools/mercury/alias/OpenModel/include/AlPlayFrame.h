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
*/

/*
//
//	.NAME AlPlayFrame - An optimization of the view frame operation.
//
//	.SECTION Description
//
//		Viewframes can be quite slow when done individually.  The problem
//		is that some information is being recomputed every time the viewFrame
//		operation is called.  This class encapsulates that dependency, 
//		performing the computation once, allowing for a series of viewframes
//		to take place in rapid succession.
//
//      Be VERY VERY CAREFUL about how you use this class.  As part of
//      its optimisation technique, a lot of animation information is
//      cached outside of the normal messaging system.  This means that
//      if you delete some animation objects while an AlPlayFrame object
//      is active, then at some stage PowerAnimator will become very
//      confused (as in core dump confused).
//
//      Ordinarily the AlPlayFrame class will not recompute a requested
//      frame if it is already the current frame.  However for certain
//      uses (for example, if you are changing a keyframe position) it may
//      be necessary to recompute the current frame.  Passing TRUE to
//      the setShowSameFrame method will allow you to override the default
//      behavior.
//
//		To use AlPlayFrame, create an instance of AlPlayFrame at the point
//		in your code where the viewFrames are to begin.  Call the viewFrame
//		member of that AlPlayframe object for as many frames as are necessary.
//		When that AlPlayFrame object goes out of scope, the destructor undoes
//		the optimization and it becomes safe to modify Alias structures.
//		For example:
//
//	.nf
// %@ make list_of_frames;
// %@ {
// %@%@ AlPlayFrame playframe;
// %@%@ for frm in list_of_frames do
// %@%@ {
// %@%@%@ playframe.viewFrame( frm );
// %@%@ }
// %@ }
//	.fi
//
*/

#ifndef _AlPlayFrame
#define _AlPlayFrame

#include <statusCodes.h>
#include <AlStyle.h>

extern "C" {
struct Ai_Animation_info_s;
typedef struct Ai_Animation_info_s Ai_Animation_info;
}

class AlPlayFrame {
public:
	AlPlayFrame (boolean bAllChannels = FALSE);
	~AlPlayFrame ();

	statusCode viewFrame (const double frame, const boolean doRedraw = TRUE);
	void setShowSameFrame (const boolean showSameFrame = FALSE) {fShowSameFrame = showSameFrame;}

private:
	static int fRefCount;
	Ai_Animation_info *fAnimInfo;
	boolean fDrawParticles;
	boolean fShowSameFrame;
};


#endif // _AlPlayFrame
