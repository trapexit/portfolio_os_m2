/* @(#) pf_core.h 96/04/23 1.5 */
#ifndef _pf_core_h
#define _pf_core_h

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

cfTaskData *pfCreateTask( int32 UserStackDepth, int32 ReturnStackDepth );
void   pfDeleteTask( cfTaskData *cftd );
void   pfSetCurrentTask( cfTaskData *cftd );

cfDictionary *pfCreateDictionary( uint32 HeaderSize, uint32 CodeSize );
void   pfDeleteDictionary( cfDictionary *dic );

void   pfSetQuiet( int32 IfQuiet );
int32  pfRunForth( void );
int32  pfIncludeFile( char *FileName );
void   pfMessage( const char *CString );

void   ResetForthTask( void );

#ifdef __cplusplus
}   
#endif


#endif /* _pf_core_h */
