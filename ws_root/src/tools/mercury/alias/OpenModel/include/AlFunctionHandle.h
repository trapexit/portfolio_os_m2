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
//
//+

//
//  .NAME AlFunction - Class for creating the OpenAlias interface to Alias
//
//  .SECTION Description
//		This class provides a means to interface the OpenAlias application
//		to the Alias user interface.
//

#ifndef _AlFunctionHandle
#define _AlFunctionHandle

#include <AlStyle.h>

class AlFunction;
class TuiChoice;

class AlFunctionHandle {
public:
						AlFunctionHandle(); 
						~AlFunctionHandle();

	statusCode			create( const char*, const char* );
	statusCode			create( const char*, AlFunction *func );

	AlFunctionHandle&	operator =( const AlFunctionHandle& f );
	int					operator !() const;

	statusCode			setAttributeString( const char * );
	statusCode			setOptionBox( const char *, const char *, const char *dirname = NULL );
	statusCode			setIconPath( const char* path );

	statusCode			addToMenu( const char * );
	statusCode			appendToMenu( const char * );

	statusCode			removeFromMenu();
	statusCode			deleteObject();

private:
	statusCode			attachToMenu( const char *, boolean );
	TuiChoice*			fChoice;
};

#endif	// _AlFunctionHandle

