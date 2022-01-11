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
//	.NAME AlNameItem - List items used for lists of names (strings)
//
//	.SECTION Description
//		This class is derived from the AlLinkItem class, and is used
//		to maintain one element of a list of names.
//

#ifndef _AlNameItem
#define _AlNameItem

class AlNameItem : public AlLinkItem {
	friend class			AlUniverse;

	public:
		const char*			name() const;

		AlNameItem*			nextItem() const;
		AlNameItem*			prevItem() const;

	protected:
							AlNameItem( const char * );
		virtual				~AlNameItem();

	private:
		const char*		fName;
};

#endif
