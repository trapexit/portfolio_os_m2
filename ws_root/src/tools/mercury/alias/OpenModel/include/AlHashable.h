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
//	.NAME AlHashable - Base class for objects which can be inserted into
//		an AlDictionary.
//
//	.SECTION Description
//		AlHashable provides a contract for any class which wishes to be 
//		stored in an AlDictionary, namely the necessity of providing a hash
//		key and the ability to be stored in a list.
//

#ifndef _AlHashable
#define _AlHashable

#include <AlStyle.h>
#include <AlLinkItem.h>
#include <AlList.h>

class AlDictionary;
class AlObject;

struct hashkeydata
{
	void*		ptr1;
	void*		ptr2;
	long int	num;
};

class AlHashKey {
public:
						AlHashKey(); 
						AlHashKey( const void* ); 
						AlHashKey( const void *, long int ); 
						AlHashKey( const void *, const void * );
						AlHashKey( const void *, const void *, long int );
						// AlHashKey( long int, long int )

						AlHashKey( const AlHashKey& );
						~AlHashKey();

	AlHashKey&			operator =( const AlHashKey& );

	int					operator ==( const AlHashKey& ) const;
						operator int() const;

	void				*getPtr1( void ) const; 
	void				*getPtr2( void ) const;
	long int 			getNum( void ) const;

	inline unsigned long int hash( int table_size ) const;
	void				output( void ) const;

private:
	hashkeydata			data;
	void				resetData( void );
};

class AlHashable : public AlLinkItem {
	friend 				class AlDictionary;
	friend 				class AlFriend;
	friend				class AlDictionaryOperators;
	friend				class AlInvalidatorIterator;

protected:
						AlHashable();
	virtual				~AlHashable();

	int					setData( void* );
	int					setData( void*, void* );
	int					setData( void*, long int );
	int					setData( void*, void*, long int);
	int					invalidate( void );

	int					setData( const AlHashKey& );

	boolean				isValid( void ) const;

private:
	AlHashKey			fData;
	AlDictionary		*fDict;

	AlLinkItem*			getNext( void ) const;
};

#endif // _AlHashable
