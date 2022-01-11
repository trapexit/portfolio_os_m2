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
//	.NAME AlList - Simple List class for AlLinkItem objects.
//
//	.SECTION Description
//		This simple container class provides the ability to create and
//		access a list of objects derived from the AlLinkItem class.
//

#ifndef _AlList
#define _AlList

class AlLinkItem;

class AlList {
  public:
				AlList( AlLinkItem *item = (AlLinkItem*)0 );
	virtual		~AlList();

	AlLinkItem*	first( void ) const { return fHead; }
	AlLinkItem*	last( void ) const { return fTail; }

	void		append( AlLinkItem* );
	int			remove( AlLinkItem* );
	void		clear();

  private:
	AlLinkItem*	fHead;
	AlLinkItem*	fTail;
};

#endif
