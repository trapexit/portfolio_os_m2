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
//	.NAME AlMappedFieldItem - Items for maintaining texture mapped fields.
//
//	.SECTION Description
//		This class is derived from the AlLinkItem class, and is used
//		to maintain one element of a list of fields that may be texture
//		mapped for a particular AlShader, AlTexture, or AlEnvironment.
//

#ifndef _AlMappedFieldItem
#define _AlMappedFieldItem

class AlMappedFieldItem : public AlLinkItem {
	friend class			AlFriend;

	public:
		const char*			field() const;
		AlMappedFieldItem*	nextField() const;

	protected:
							AlMappedFieldItem( const char* );
		virtual				~AlMappedFieldItem();

		static const char*	allocField( const char * );

	private:
		const char*			fField;
};

#endif
