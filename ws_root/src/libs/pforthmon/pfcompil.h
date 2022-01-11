/* @(#) pfcompil.h 96/07/15 1.9 */
#ifndef _pforth_compile_h
#define _pforth_compile_h

/***************************************************************
** Include file for PForth Compiler
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

int32 FindSpecialCFAs( void );
cell *NameToCode( ForthString *NFA );
ForthString *NameToPrevious( ForthString *NFA );

cfDictionary *pfBuildDictionary( int32 HeaderSize, int32 CodeSize );
void  ffAbort( void );
void  ffColon( void );
void  ffCreate( void );
int32 FindSpecialXTs( void );
void  ffStringCreate( char *FName);
ExecToken NameToToken( ForthString *NFA );
void  ffDefer( void );
void  ffStringDefer( char *FName, ExecToken DefaultXT );
cell  ffTokenToName( ExecToken XT, ForthString **NFAPtr );
cell  ffFind( ForthString *WordName, ExecToken *pXT );
cell  ffFindC( char *WordName, ExecToken *pXT );
cell  ffNumberQ( char *FWord, cell *Num );
cell  ffRefill( void );
char *ffWord( char c );
void  ffOK( void );
void  CreateDicEntryC( ExecToken XT, char *CName, uint32 Flags );
void  ffCreateSecondaryHeader( char *FName);
void  ffQuit( void );
FileStream *ffPopInputStream( void );
Err   ffPushInputStream( FileStream *InputFile );
cell  ffConvertStreamToSourceID( FileStream *Stream );
void  ffALiteral( cell Num );
cell  ffFindNFA( char *WordName, char **NFAPtr );
int32 ffInterpret( void );
cell  ffIncludeFile( FileStream *InputFile );
void  ffLiteral( cell Num );
void  ff2Literal( cell dHi, cell dLo );
void  ffSemiColon( void );
int32 NotCompiled( const char *FunctionName );

#ifdef PF_SUPPORT_FP
void ffFPLiteral( PF_FLOAT fnum );
#endif

#ifdef __cplusplus
}   
#endif

#endif /* _pforth_compile_h */
