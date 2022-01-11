/* @(#) pf_save.h 96/03/21 1.6 */
#ifndef _pforth_save_h
#define _pforth_save_h

/***************************************************************
** Include file for PForth SaveForth
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
**	941031	rdg		fix redefinition of MAKE_ID and EVENUP to be conditional
**
***************************************************************/


typedef struct DictionaryInfoChunk
{
	int32  sd_Version;
	int32  sd_RelContext;      /* relative ptr to Dictionary Context */
	int32  sd_RelHeaderPtr;    /* relative ptr to Dictionary Header Ptr */
	int32  sd_RelCodePtr;      /* relative ptr to Dictionary Header Ptr */
	ExecToken  sd_EntryPoint;  /* relative ptr to entry point or NULL */
	int32  sd_UserStackSize;   /* in cells */
	int32  sd_ReturnStackSize; /* in cells %Q make it bytes */
	int32  sd_NameSize;        /* in bytes */
	int32  sd_CodeSize;        /* in bytes */
	int32  sd_NumPrimitives;   /* To distinguish between primitive and secondary. */
} DictionaryInfoChunk;

#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((a<<24)|(b<<16)|(c<<8)|d)
#endif

#define ID_FORM MAKE_ID('F','O','R','M')
#define ID_P4TH MAKE_ID('P','4','T','H')
#define ID_P4DI MAKE_ID('P','4','D','I')
#define ID_P4NM MAKE_ID('P','4','N','M')
#define ID_P4CD MAKE_ID('P','4','C','D')
#define ID_BADF MAKE_ID('B','A','D','F')

#ifndef EVENUP
#define EVENUP(n) ((n+1)&(~1))
#endif

#ifdef __cplusplus
extern "C" {
#endif

int32 ffSaveForth( char *FileName, ExecToken EntryPoint, int32 NameSize, int32 CodeSize );
cfDictionary *pfLoadDictionary( char *FileName, ExecToken *EntryPointPtr );

#ifdef __cplusplus
}   
#endif

#endif /* _pforth_save_h */
