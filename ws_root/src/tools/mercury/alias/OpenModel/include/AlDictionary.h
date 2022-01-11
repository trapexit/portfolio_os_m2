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

//-
//	.NAME AlDictionary - Class used for fast storage and retrieval of
//		AlObjects keyed on Alias data.
//
//	.SECTION Description
//		This class provides methods for storage, lookup, and deletion of
//		AlObjects.  The objects are keyed on a low-level AlHashKey value
//		which is used as a general storage mechanism for references to
//		Alias data (see AlHashable.h)
//
//	This file also contains the call AlDictionaryOperators, which should
//	hold all the different functions that operate on elements in the dictionary
//	through the applyToAll method.  The canonical example of this is 
//	invalidateAll.
//+

#ifndef _AlDictionary
#define _AlDictionary

#include <AlStyle.h>
#include <AlHashable.h>
#include <AlList.h>

#define DEFAULT_MIN_DICT_SIZE 256

class AlDictionaryIterator {
public:
	virtual int			apply( AlHashable* ) = 0;
};

class AlDictionary {
	friend				class AlFriend;
	friend				class AlDictionaryOperators;
public:
						AlDictionary( int = DEFAULT_MIN_DICT_SIZE );
						~AlDictionary( void );

	int					insertAs( AlHashable*, const AlHashKey& );
	int					remove( AlHashable* );
	int					applyToAll( const AlHashKey&, AlDictionaryIterator* );
	int					applyToValid( AlDictionaryIterator* );
	int					applyToInvalid( AlDictionaryIterator* );

	void				collect( void );
	void				moveAll( AlDictionary* );
	int					changeSize( int newSize );

// What would we ever use this for?
//	AlHashable*			lookup( AlHashKey& );

private:
	AlList				*fTable;
	AlList				fGarbage;
	int					minSize, fSize;
	int					count;
	int					numberFloor, numberCeiling;
};

#endif // _AlDictionary
