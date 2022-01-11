/* @(#) pf_io.h 96/03/21 1.6 */
#ifndef _pforth_io_h
#define _pforth_io_h

/***************************************************************
** Include file for PForth IO
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

cell ioAccept( char *Target, cell n1, FileStream *Stream );
cell ioKey( void);
void ioEmit( char c, FileStream *Stream );
void ioFlush( FileStream *Stream );
void ioType( const char *s, int32 n, FileStream *Stream);
void ioTypeCString( const char *s, FileStream *Stream );

#ifdef __cplusplus
}   
#endif

#endif /* _pforth_io_h */
