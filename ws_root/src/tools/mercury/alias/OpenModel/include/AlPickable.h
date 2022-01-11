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
//	.NAME AlPickable - Basic Interface to Alias objects which can be picked.
//
//	.SECTION Description
//		This class encapsulates the functionality of Alias objects
//		which have the capacity to be picked.  As expected, pickable 
//		objects can either be picked or unpicked.
//

#ifndef _AlPickable
#define _AlPickable

#include <AlObject.h>

class AlPickable {
	friend					class AlFriend;

public:
	virtual statusCode		pick( void );
	virtual statusCode		unpick( void );
	virtual boolean			isPicked( void );
protected:
							AlPickable();
	virtual 				~AlPickable();
	AlPickable*				pickablePtr();

	virtual boolean			extractType( int&, void *&, void *& ) const;
};

#endif // _AlPickable
