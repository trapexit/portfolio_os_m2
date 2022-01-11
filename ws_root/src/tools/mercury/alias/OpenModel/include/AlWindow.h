/*
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
//+
*/

/*
//
//  .NAME AlWindow - interface to the Alias modeling windows.
//
//  .SECTION Description
//		Alias modeling windows are the windows in which geometry information
//		is actually viewed and modified.  These are the Top, Front, Right
//		and Perspective windows, as well as the SBD window.  This class
//		allows the creation, positioning and sizing windows.  You can get
//		the camera associated with a window.  Plugins can use the mapping
//		functions to determine where in world space a screen event occured.
//
//		AlUniverse contains methods for getting the first modeling window,
//		the current modeling window, and the sbd window.
//
*/

#ifndef _AlWindow
#define _AlWindow

#include <AlStyle.h>

#ifndef __cplusplus
	typedef enum {
		AlWindow_kViewInvalid,
		AlWindow_kViewOther,
		AlWindow_kFront,
		AlWindow_kBack,				/* unused */
		AlWindow_kRight,	
		AlWindow_kLeft,				/* unused */
		AlWindow_kTop,
		AlWindow_kBottom,			/* unused */
		AlWindow_kSbd,
		AlWindow_kPerspective
	} AlViewType;

	typedef enum {
		AlWindow_kBottomLeft,
		AlWindow_kTopLeft,
		AlWindow_kTopRight,
		AlWindow_kBottomRight
	} AlCornerType;

	typedef enum {
		AlWindow_kNoScale,
		AlWindow_kScaleY,
		AlWindow_kScaleX
	} AlAspectType;
#else

#include <AlObject.h>
#include <AlAnimatable.h>

class AlWindow 	: public AlObject
				, public AlAnimatable
{
	friend						class AlFriend;

public:

	enum AlViewType {
		kViewInvalid,
		kViewOther,
		kFront,
		kBack,				/* unused */
		kRight,	
		kLeft,				/* unused */
		kTop,
		kBottom,			/* unused */
		kSbd,
		kPerspective
	};

	enum AlCornerType {
		kBottomLeft,
		kTopLeft,
		kTopRight,
		kBottomRight
	};

	enum AlAspectType {
		kNoScale,
		kScaleY,
		kScaleX
	};

public:
								AlWindow();
	virtual 					~AlWindow();

	virtual AlObjectType		type() const;

	statusCode					create( AlViewType );
	virtual AlObject*			copyWrapper() const;
	virtual statusCode			deleteObject();

	virtual AlWindow*			asWindowPtr();
	virtual AlAnimatable* 		asAnimatablePtr();

	AlWindow*					next() const;
	AlWindow*					prev() const;

	statusCode					nextD();
	statusCode					prevD();

	statusCode					windowType( AlViewType& );
	statusCode					isZoom( boolean& );

	virtual AlCamera*			camera( void );
	virtual statusCode			setCamera( AlPerspectiveCamera* );

	statusCode					position( Screencoord&, Screencoord& );
	statusCode					setPosition( Screencoord, Screencoord );

	statusCode					resolution( Screencoord&, Screencoord& );
	statusCode					setResolution( Screencoord, Screencoord, AlCornerType = kTopLeft, AlAspectType = kNoScale );

	statusCode 					priority( int& );
	statusCode					setPriority( int );

	double						gridSize();
	statusCode					setGridSize( double size );

	virtual statusCode			mapToWorldSpace( Screencoord, Screencoord, double&, double&, double&, double&, double&, double& );
	virtual statusCode			worldSpaceBounds( double&, double&, double&, double&, double&, double& );

	static statusCode			aliasWindowSize( int&, int& );

private:
	virtual	boolean extractType( int&, void*&, void*& ) const;

	static void					initMessages( void );
	static void					finiMessages( void );
};

#endif /* __cplusplus */

#endif /* _AlWindow */
