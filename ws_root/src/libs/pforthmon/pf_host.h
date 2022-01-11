/* @(#) pf_system.h 95/04/25 1.4 */
#ifndef _pf_system_h
#define _pf_system_h

/***************************************************************
** System Dependant Includes for PForth based on 'C'
**
** Author: Phil Burk
**
** Copyright Phil Burk 1994
** All rights reserved.
**
****************************************************************
***************************************************************/

#ifdef PF_CDE_SERIAL
	int32 m2PutChar( int32 theChar, FileStream * Stream );
	int32 m2GetChar( FileStream * Stream  );

	#define sdOutputChar    m2PutChar
	#define sdInputChar     m2GetChar
#else
	#define sdOutputChar    putc
	#define sdInputChar     getc
#endif /* PF_CDE_SERIAL */


#define PF_FAM_READ_ONLY (0)
#define PF_FAM_READ_WRITE (1)

#ifdef PF_HOST_DOS
	#define PF_FAM_CREATE  ("wb+")
	#define PF_FAM_OPEN_RO  ("rb")
	#define PF_FAM_OPEN_RW  ("rb+")
#else
	#define PF_FAM_CREATE  ("w+")
	#define PF_FAM_OPEN_RO  ("r")
	#define PF_FAM_OPEN_RW  ("r+")
#endif

#if  defined(PF_NO_FILEIO)

FileStream *m2OpenFile( char *FileName, char *Mode );
int32 m2FlushFile( FileStream * Stream  );
int32 m2ReadFile( void *ptr, int32 Size, int32 nItems, FileStream * Stream  );
int32 m2WriteFile( void *ptr, int32 Size, int32 nItems, FileStream * Stream  );
int32 m2SeekFile( FileStream * Stream, int32 Position, int32 Mode );
int32 m2TellFile( FileStream * Stream );
int32 m2CloseFile( FileStream * Stream );
	
/* Stubs */
#define sdFlushFile     m2FlushFile
#define sdOpenFile      m2OpenFile
#define sdReadFile      m2ReadFile
#define sdWriteFile     m2WriteFile
#define sdSeekFile      m2SeekFile
#define sdTellFile      m2TellFile
#define sdCloseFile     m2CloseFile

/*
** printf() is only used for debugging purposes.
** It is not required for normal operation.
*/
#define PRT(x) { printf x; } /* sdFlushFile(stdout); } */

#else

/*
** This version uses ANSI standard I/O calls.
** Some embedded systems may not have support for these
** at early stages of development.
*/
#define sdFlushFile     fflush
#define sdOpenFile      fopen
#define sdReadFile      fread
#define sdWriteFile     fwrite
#define sdSeekFile      fseek
#define sdTellFile      ftell
#define sdCloseFile     fclose
typedef FILE FileStream;

/*
** printf() is only used for debugging purposes.
** It is not required for normal operation.
*/
#define PRT(x) { printf x; sdFlushFile(stdout); }

#endif /* File I/O */

#endif /* _pf_system_h */

