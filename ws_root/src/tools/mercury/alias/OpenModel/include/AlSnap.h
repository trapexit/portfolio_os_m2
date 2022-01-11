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
//+

//
//  .NAME AlSnap - Basic Interface to Alias grid snapping functions
//
//  .SECTION Description
//		This class contains various utility functions for performing grid
//		snapping.

#ifndef	_AlSnap
#define _AlSnap

#ifdef __cplusplus

class AlWindow;
class AlObject;

class AlSnap
{
public:
	static statusCode	toGrid( Screencoord x, Screencoord y, double worldPos[3], AlWindow* window = NULL );
	static statusCode	toCV( Screencoord x, Screencoord y, AlObject* &obj, AlWindow *window = NULL );
	static statusCode	toCurve( Screencoord x, Screencoord y, AlObject* &obj, double &curveParam, AlWindow *window = NULL );
};
#endif

#endif
