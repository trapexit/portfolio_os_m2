/* @(#) pf_main.c 95/11/15 1.2 */
/***************************************************************
** Forth based on 'C'
**
** main() routine that demonstrates how to call PForth as
** a module from 'C' based application.
** Customize this as needed for your application.
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

#include "pforth.h"
#include <kernel/super.h>
typedef Err (* CallBackFunc)(uint32,uint32,uint32);

#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif

#define MSG(cs) { pfMessage(cs); }

#define DEFAULT_RETURN_DEPTH (512)
#define DEFAULT_USER_DEPTH (512)
#define DEFAULT_HEADER_SIZE (60000)
#define DEFAULT_CODE_SIZE (150000)

/**************************************************************************
** Conveniance routine to execute PForth
*/
Err EnterPForth( void )
{
	cfTaskData *cftd;
	cfDictionary *dic;
	int Result = 0;
	ExecToken  EntryPoint = 0;
	
/* Allocate Task structure. */
	cftd = pfCreateTask( DEFAULT_USER_DEPTH, DEFAULT_RETURN_DEPTH );
	
	if( cftd )
	{
		pfSetCurrentTask( cftd );
		
/* Don't use MSG before task set. */
		MSG("pForth\n");

		dic = pfLoadDictionary( "Ignored", &EntryPoint );
		if( dic == NULL ) goto error;
		
		Result = pfRunForth();
		
		pfDeleteDictionary( dic );
		pfDeleteTask( cftd );
	}
	return Result;
	
error:
	MSG("pForth: Error occured.\n");
	pfDeleteTask( cftd );
	return -1;
}
