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
//	.NAME AlDebug - Base class for OpenAlias/Model debugging utilities.
//
//	.SECTION Description
//		This class provides a set of routines which may aid in debugging
//		OpenAlias applications.  Most of these functions are intended for
//		debugging purposes only and should not be used in a release product.
//
//		These methods can only be used to debug a single plugin module
//		(if multiple plugins are in use, then debugging information will be
//		printed for ALL of the wrappers that have been created or deleted).
//
//		The cleanUpUniverse method provides a means to delete all of the 
//		wrappers that have not been deleted.  It may be possible to use this
//		method in an OpenModel application provided that no wrappers are created
//		on the user stack.
//
//		The applyIterator methods apply an iterator to every wrapper that is
//		created (or possibly invalid).
//
//		The setErrorReportingFunction method sets the function that is used
//		to display errors. 
//
//		In addition, there are several assertion condition macros.  These output
//		errors to stderr and then Alias exits.  They are:
//		
//		AL_ASSERT(condition,msg)
//			- prints out 'msg' if the condition fails
//		AL_CREATED()
//			- prints out an created twice error message (if the AlObject has already been created
//		AL_NOT_CREATED()
//			- if the object has not been created, an error is printed
//		AL_CASE_ERR(caseValue, msg)
//			- this is used to print out an error 'msg' if an unknown case value is
//			detected in a switch statement
//		AL_ELSE_ERR()
//			- this is used to print out an error message if an impossible 'else'
//			branch was reached
//		AL_MEMERR()
//			- this prints out a memory error
//
//		AL_ASSERT_COND(condition)
//			- if the given condition fails, then an assertion error message is
//			printed
*/

#ifndef _AlDebug_h
#define _AlDebug_h

#include <stdio.h>
#include <stdlib.h>

typedef void AlErrorReporter( const char *, ... );

#ifdef __cplusplus

#include <AlStyle.h>
#include <AlModel.h>
#include <AlIterator.h>


class AlDebug {
public:
	static statusCode	cleanUpUniverse( void );

	static statusCode 	applyIteratorToValid( AlIterator*, int& );
	static statusCode	applyIteratorToInvalid( AlIterator*, int& );
	static statusCode	applyIteratorToAll( AlIterator*, int& );

	static statusCode	outputWrapperUsage( AlOutputType );

	static statusCode	setErrorReportingFunction( AlErrorReporter* );
	static void			checkpoint( void );
};

#endif	/* __cplusplus */

/* the assert macros */
#ifndef NDEBUG

#define AL_ASSERT(condition,msg)		\
{	\
	if (!(condition))	\
	{	\
		fprintf( stderr, "%s, %s\nFile: %s\nLine: %d\n",	\
			msg, #condition, __FILE__, __LINE__ );	\
		fflush( stdout ); \
		abort(); \
	}	\
}

#define AL_CREATED()		\
{	\
	if (isCreated())	\
	{	\
		fprintf( stderr, "Create called twice.\nFile: %s\nLine: %d\n",	\
			__FILE__, __LINE__);	\
		fflush( stdout ); \
		abort(); \
	}	\
}

#define AL_NOT_CREATED()		\
{	\
	if (!isCreated())	\
	{	\
		fprintf( stderr, "Create not called.\nFile: %s\nLine: %d\n",	\
			__FILE__, __LINE__);	\
		fflush( stdout ); \
		abort(); \
	}	\
}

#define AL_CASE_ERR(caseValue, msg)		\
{	\
	fprintf( stderr, "%s - Unknown case value: %d\nFile: %s\nLine: %d\n",    \
		msg, (int)caseValue, __FILE__, __LINE__ );	\
	fflush( stdout ); \
	abort(); \
}

#define AL_ELSE_ERR()		\
{	\
	fprintf( stderr, "Impossible else condition.\nFile: %s\nLine: %d\n",	\
		__FILE__, __LINE__);	\
	fflush( stdout ); \
	abort(); \
}

#define AL_MEMERR()		\
{	\
	fflush( stdout ); \
	abort(); \
}

#define AL_ASSERT_COND(condition)        \
{   \
	if (!(condition))   \
	{   \
		fprintf( stderr, "\nFile: %s\nLine: %d\n", __FILE__, __LINE__ );   \
		abort(); \
	}   \
}

#else	/* DEBUG */

#define AL_ASSERT(condition,msg)		\
{	\
	if (!(condition))	\
	{	\
		fprintf( stderr, "%s\n", msg );	\
		fflush( stdout ); \
		abort(); \
	}	\
}

#define AL_CREATED()		\
{	\
	if (isCreated())	\
	{	\
		fprintf( stderr, "Create called twice.\n" );	\
		fflush( stdout ); \
		abort(); \
	}	\
}

#define AL_NOT_CREATED()		\
{	\
	if (!isCreated())	\
	{	\
		fprintf( stderr, "Create not called.\n" );	\
		fflush( stdout ); \
		abort(); \
	}	\
}

#define AL_CASE_ERR(caseValue, msg)		\
{	\
	fprintf( stderr, "%s - Unknown case value: %d\n", msg, (int)caseValue );	\
	fflush( stdout ); \
	abort(); \
}

#define AL_ELSE_ERR()		\
{	\
	fprintf( stderr, "Impossible else condition.\n" );	\
	fflush( stdout ); \
	abort(); \
}

#define AL_MEMERR()		\
{	\
	fflush( stdout ); \
	abort(); \
}

#define AL_ASSERT_COND(condition)        \
{   \
	if (!(condition))   \
	{   \
		fprintf( stderr, "Assertion failed.\n" );   \
		abort(); \
	}   \
}
#endif	/* DEBUG */

#if defined(OLDNDEBUG)
#define	DB_ASSERT AL_ASSERT
#define DB_CREATED AL_CREATED
#define DB_NOT_CREATED AL_NOT_CREATED
#define DB_CASE_ERR AL_CASE_ERR
#define DB_ELSE_ERR AL_ELSE_ERR
#define DB_MEMERR AL_MEMERR
#endif

#endif /* _AlDebug_h */
