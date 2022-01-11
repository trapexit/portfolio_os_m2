/*
//-
//	Copyright (C) 1995, Alias|Wavefront
//
//  These coded instructions, statements and computer programs contain
//  unpublished information proprietary to Alias|Wavefront  and are
//  protected by the Canadian and US Federal copyright law. They may not
//  be disclosed to third parties or copied or duplicated, in whole or
//  in part, without the prior written consent of Alias|Wavefront
//
//  Unpublished-rights reserved under the Copyright Laws of the United States.
//
//-
*/

/*
//
//	.NAME AlPickList - A static class which gives access to the Alias pick list
//
//	.SECTION Description
//
//		This class gives access to the pick list, that is, those objects
//		which are currently active. Note that this will include objects
//		which are active in the MultiLister and not just those objects
//		active in the modeling windows.
//
*/

#ifndef _AlPickList_H
#define _AlPickList_H

#include <statusCodes.h>
#include <AlStyle.h>

#ifndef __cplusplus
    typedef enum {
        AlPickList_kMaskUnchanged      = 0x0,
        AlPickList_kMaskTemplate       = 0x1,
        AlPickList_kMaskRoot           = 0x2,
        AlPickList_kMaskInterior       = 0x4,
        AlPickList_kMaskLeaf           = 0x8,
        AlPickList_kMaskLight          = 0x10,
        AlPickList_kMaskLocator        = 0x20,
        AlPickList_kMaskCamera         = 0x40,
        AlPickList_kMaskLine           = 0x80,
        AlPickList_kMaskPoint          = 0x100,
        AlPickList_kMaskEditPoint      = 0x200,
        AlPickList_kMaskParamCurve     = 0x400
	} AlPickMaskType;

#else
class AlObject;
class AlIterator;
class AlPickable;

extern "C" {
	struct Pick_item;
	struct IR_ShaderEntry;
};

class AlPickList
{
	friend					class AlUniverse;
	friend					class AlFriend;
	friend					void handlePickListModified( void* );

public: 
	enum AlPickMaskType {
		kMaskUnchanged 		= 0x0,
		kMaskTemplate 		= 0x1,
		kMaskRoot 			= 0x2,
		kMaskInterior		= 0x4,
		kMaskLeaf			= 0x8,
		kMaskLight			= 0x10,
		kMaskLocator		= 0x20,
		kMaskCamera			= 0x40,
		kMaskLine			= 0x80,
		kMaskPoint			= 0x100,
		kMaskEditPoint		= 0x200,
		kMaskParamCurve		= 0x400
	};

public:
	static boolean			isValid();

	static AlObject*		getObject();

	static statusCode		firstPickItem();
	static statusCode		nextPickItem();
	static statusCode		prevPickItem();
	static statusCode		applyIteratorToItems( AlIterator*, int& );

	static statusCode		clearPickList();
	static statusCode		pickByName( char* );
	static statusCode		pickFromScreen( Screencoord x, Screencoord y );

	static statusCode		pushPickList(boolean);
	static statusCode		popPickList();

	static statusCode		setPickMask( int );
	static statusCode		getPickMask( int& );


private:
	static void				invalidate( void );
	static void				setPickItem( Pick_item* );
	static void				initMessages( void );
	static void				finiMessages( void );
	static boolean			canMakeObject( Pick_item* );
	static AlObject*		constructObject( Pick_item* );

	static Pick_item*		fPickItem;
	static IR_ShaderEntry*	fPickShader;
};
#endif	/* __cplusplus */

#endif	/* _AlPickList_h */
