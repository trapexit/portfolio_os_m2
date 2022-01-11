/* @(#) pf_cglue.h 96/03/21 1.5 */
#ifndef _pf_c_glue_h
#define _pf_c_glue_h

/***************************************************************
** Include file for PForth 'C' Glue support
**
** Author: Phil Burk
** Copyright 1994 3DO, Phil Burk, Larry Polansky, Devid Rosenboom
**
** The pForth software code is dedicated to the public domain,
** and any third party may reproduce, distribute and modify
** the pForth software code or any derivative works thereof
** without any compensation or license.  The pForth software
** code is provided on an "as is" basis without any warranty
** of any kind, including, without limitation, the implied
** warranties of merchantability and fitness for a particular
** purpose and their equivalents under the laws of any jurisdiction.
**
***************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

int32 CreateGlueToC( char *CName, int32 Index, int32 ReturnMode, int32 NumParams );
int32 CompileCustomFunctions( void );
int32 CallUserFunction( int32 Index, int32 ReturnMode, int32 NumParams );

#ifdef __cplusplus
}   
#endif

#define C_RETURNS_VOID (0)
#define C_RETURNS_VALUE (1)

#endif /* _pf_c_glue_h */
