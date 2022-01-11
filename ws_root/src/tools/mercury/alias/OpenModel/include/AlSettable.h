//-
//	Copyright (C) 1995, Alias|Wavefront
//
//  These coded instructions,  statements and  computer programs contain
//  unpublished information proprietary to Alias|Wavefront  and are
//  protected by the Canadian and US Federal copyright law. They may not
//  be disclosed to third parties  or copied  or duplicated, in whole or
//  in part,  without the prior written consent of Alias|Wavefront
//
//  Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+

//
//  .NAME AlSettable - Encapsulates methods common to Alias objects which 
//		  can belong to sets.
//
//  .SECTION Description
//
//		This class is a base class for all objects which can be
//		contained in a set. It provides the methods necessary
//		to access the set methods of these objects.
//

//-
//	Note that this class relies on the fact the classes derived from it 
//	overload the extractType function in AlObject.  In theory, we should be
//	introducing a new base class, AlTypeable, which has a pure virtual 
//	extractType method.  But this introduces a lot of extra code.  Let's just
//	assume for now that if AlFoo inherits AlSettable, then AlFoo provides
//	a working extractType method.
//+

#ifndef _AlSettable
#define _AlSettable

#include <AlObject.h>
#include <AlTM.h>
#include <AlIterator.h>

class AlSet;
class AlSetMember;

class AlSettable {
	friend class		AlFriend;
public:
	virtual AlSet*		firstSet() const;

	virtual AlSet*		nextSet( const AlSet* ) const;
	virtual AlSet*		prevSet( const AlSet* ) const;

	virtual statusCode	nextSetD( AlSet* ) const;
	virtual statusCode	prevSetD( AlSet* ) const;

	virtual statusCode	applyIteratorToSets( AlIterator*, int& );

	statusCode			removeFromAllSets();
	statusCode			removeFromSet( AlSet *set );
	statusCode			addToSet( AlSet *set );
	AlSetMember*		isSetMember( const AlSet *set ) const;

protected:
						AlSettable();
	virtual				~AlSettable();
	AlSettable*			settablePtr();	

	virtual boolean		extractType( int&, void *&, void *& ) const;
};
#endif

