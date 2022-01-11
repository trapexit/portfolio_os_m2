/* @(#) pf_text.c 96/03/21 1.7 */
/***************************************************************
** Text Strings
**
** For PForth based on 'C'
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
****************************************************************
**
***************************************************************/

#include "pf_all.h"

#define PF_ENGLISH

/*
** Define array of error messages.
** These are defined in one place to make it easier to translate them.
*/
#ifdef PF_ENGLISH
static char *gErrorMessages[] =
{
	"insufficient memory",
	"address misaligned",
	"data chunk too large",
	"incorrect number of parameters",
	"could not open file",
	"wrong type of file format",
	"badly formatted file",
	"file read failed",
	"file write failed",
	"corrupted dictionary",
	"not supported in this version",
	"version from future",
	"version is obsolete. Rebuild new one.",
	"stack depth changed between : and ; . Probably unbalanced conditional",
	"no room left in header space",
	"no room left in code space",
	"attempt to use names in forth compiled with PF_NO_SHELL",
	"dictionary has no names"
};
#endif

#define NUM_ERROR_MSGS (sizeof(gErrorMessages)/sizeof(char *))

/***************************************************************/
void pfReportError( char *FunctionName, Err ErrCode )
{
	uint32 ErrIndex;

	MSG("Error in ");
	MSG(FunctionName);
	MSG(" - ");
	ErrIndex = ErrCode & PF_ERR_INDEX_MASK;
	if( ErrIndex < NUM_ERROR_MSGS )
	{
		MSG(gErrorMessages[ErrIndex]);
	}
	else
	{
		MSG("unknown error");
	}
	MSG("\n");
}
