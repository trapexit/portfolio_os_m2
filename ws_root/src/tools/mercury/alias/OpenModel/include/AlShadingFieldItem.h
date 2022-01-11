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
//	.NAME AlShadingFieldItem - Items for maintaining AlShader parameters.
//
//	.SECTION Description
//		This class is derived from the AlLinkItem class, and is used
//		to maintain one element of a list of parameters which are
//		valid for a particular AlShader, AlTexture, or AlEnvironment.
//

#ifndef _AlShadingFieldItem
#define _AlShadingFieldItem

class AlShadingFieldItem : public AlLinkItem {
	friend class			AlShader;
	friend class			AlTexture;
	friend class			AlEnvironment;

	public:
		AlShadingFields		field() const;
		AlShadingFieldItem*	nextField() const;

	protected:
							AlShadingFieldItem(AlShadingFields);
		virtual				~AlShadingFieldItem();

	private:
		AlShadingFields		fField;
};

#endif
