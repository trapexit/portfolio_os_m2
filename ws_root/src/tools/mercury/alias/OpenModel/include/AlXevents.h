/*
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
//  .NAME AlXevents - X event interface to the Alias api
//
//  .SECTION Description
//
//		This static class contians methods to handle custom X event
//		handling in the Alias api.
//	
*/

#ifndef	_al_xevents
#define _al_xevents

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/extensions/XI.h>
#include <X11/extensions/XInput.h>


#ifdef __cplusplus

class AlXevents
{
public:
	static statusCode	installHandler( int (*)( XEvent* ) );
	static statusCode 	uninstallHandler( int (*)( XEvent* ) );
	static XtAppContext	getXtAppContext( void );
	static statusCode	addXdevice( const char *, int (*)(Widget, XDeviceInfoPtr) );
    static statusCode	getXdevID( const char *, int& );
	static statusCode	getXdevType( const char *, const char *, int& );
	static statusCode	getXdevice( const char *,  XDevice ** );
	static statusCode	addXdevID( const char *, XDevice *, int );
	static statusCode	addXdevType( char *, const char *, int, int );
	static statusCode	applicationNotify();
};

#endif

#endif
