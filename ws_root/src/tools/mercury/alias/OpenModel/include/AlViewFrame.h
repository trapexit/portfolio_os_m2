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
//	.NAME AlViewFrame - All the methods to do a viewframe operation.
//
//	.SECTION Description
//		This is a static class in that all of its member functions are
//		static.  It provides the functionality to do a viewframe on
//		the entire universe or a single file.
//
//		When doing a viewframe it is important to remember that if
//		the model contains construction history, fillet surfaces
//		may be recreated. As a result, any existing AlSurfaceCVs
//		from these surfaces will no longer be valid after the
//		viewframe. They will not be deleted, but will instead be
//		made inactive; all their methods will fail. Any object can
//		be tested to see whether it is no longer active with the
//		isGarbage() method. An inactive object may be deleted, or
//		all inactive objects may be deleted with the
//		AlUniverse::cleanUpUniverse() method.
//
*/

#ifndef _AlViewFrame
#define _AlViewFrame

#include <statusCodes.h>

class AlSurface;
class AlCurve;
class AlAnimatable;

struct Dag_node;

#ifndef __cplusplus
	typedef enum {
		AlViewFrame_kObject,
		AlViewFrame_kObjectAndAbove,
		AlViewFrame_kObjectAndBelow,
		AlViewFrame_kObjectAndAboveBelow
	} AlViewFrame_Options;
#else

class AlViewFrame
{
public:
	enum Options
	{
		kObject,
		kObjectAndAbove,
		kObjectAndBelow,
		kObjectAndAboveBelow
	};

	static statusCode 	viewFrame(const double);
	static statusCode	viewFrame(AlAnimatable*, const double,Options=kObject);
	static statusCode	viewFrame(AlSurface*, const double, Options=kObject);
	static statusCode	viewFrame(AlCurve*, const double, Options=kObject);
};
#endif /* __cplusplus */

#endif // _AlViewFrame

