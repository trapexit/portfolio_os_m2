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
//	.NAME AlLinkItem - Base class for objects contained in an AlList.
//
//	.SECTION Description
//		This is an abstract base class which classes will derive from
//		so that the objects can be contained in AlList objects.
//

#ifndef _AlLinkItem
#define _AlLinkItem

class AlLinkItem {
	friend class AlList;

  protected:
				AlLinkItem() { fNext = (AlLinkItem* )0; fPrev = (AlLinkItem* )0; };

	virtual		~AlLinkItem() {};

	AlLinkItem*	next( void ) const { return fNext; }
	AlLinkItem*	prev( void ) const { return fPrev; }

	void		append( AlLinkItem* );
	void		remove();

  private:
	AlLinkItem*	fNext;
	AlLinkItem* fPrev;
};

#endif
