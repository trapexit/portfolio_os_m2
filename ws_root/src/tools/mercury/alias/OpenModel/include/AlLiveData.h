/*
//-
//  Copyright (C) 1995, Alias|Wavefront
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
//  .NAME AlLiveData - Utility functions for OpenAlias
//
//  .SECTION Description
//		These functions are support functions for use with OpenAlias plugins
//
*/

#ifndef	_al_livedata
#define _al_livedata

#include <stdarg.h>
#include <AlStyle.h>

typedef enum {
	kFileBrowseRead,
	kFileBrowseWrite
} AlFileBrowseMode;

typedef enum {
	kOK_Cancel,
	kYes_No_Cancel
} AlConfirmType;

typedef enum {
	kOK,
	kYes,
	kNo,
	kCancel
} AlAnswerType;

#ifdef __cplusplus
extern "C"
{
#endif

/* C linkages for convience */
void AlPrintf( AlOutputType, const char*, ... );
void AlVprintf( AlOutputType, const char*, va_list ap );
const char *AlGetAliasPreference( const char * );

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
//
// C++ functions
//

void AlAllowMenuRebuilds( boolean );
void AlRebuildMenus( void );
void AlResetAllMenus( void );
statusCode AlDebugResetOptionBox( const char *editorName );
const char *AlInvokeSchemeCommand( const char *command );
const char *AlInvokeSchemeFile( const char* fname, const char *prefix = NULL );

statusCode  AlFileBrowser(	AlFileBrowseMode, char **, const char *,
							boolean, const char * );
statusCode AlPromptBox( AlConfirmType, char*, AlAnswerType*, short, short );

statusCode AlGetInteger( const char *, int& );
statusCode AlGetDouble( const char *, double& );
statusCode AlGetString( const char *, const char *& );
statusCode AlSetInteger( const char *, int );
statusCode AlSetDouble( const char *, double );
statusCode AlSetString( const char *, const char* );

#endif

#endif /* _al_livedata */
