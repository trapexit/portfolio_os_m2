/* @(#) pf_main.c 96/02/08 1.3 */
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

/* #define MSG(cs) { pfMessage(cs); } */

#define PRT(x)  { printf x; }
#define DBUG(x) PRT(x)
#define MSG(cs) PRT((cs))

#define DEFAULT_RETURN_DEPTH (512)
#define DEFAULT_USER_DEPTH (512)
#define DEFAULT_HEADER_SIZE (60000)
#define DEFAULT_CODE_SIZE (150000)

int DoForth( char *DicName, char *SourceName, int IfInit );

int main( int argc, char **argv )
{
	char *DicName = "pforth.dic";
	char *SourceName = NULL;
	char IfInit = FALSE;
	char *s;
	int Len;
	int i;
	int Result;

/* Parse command line. */
	for( i=1; i<argc; i++ )
	{
		s = argv[i];
		Len = strlen(s);

		if( *s == '-' )
		{
			char c;
			s++;
			while((c = *s++) != 0 )
			{
				switch(c)
				{
				case 'i':
					IfInit = TRUE;
					DicName = NULL;
					break;
				case 'q':
					pfSetQuiet( TRUE );
					break;
				default:
					MSG("Unrecognized option!\n");
					MSG("pforth {-iq} {dicname.dic} {sourcefilename}\n");
					exit(1);
					break;
				}
			}
		}
		else if( (Len > 4) && (strcmp(".dic",&s[Len-4]) == 0) )
		{
/* If the first parameter ends in ".dic", it is a dictionary name. */
			DicName = s;
		}
		else
		{
			SourceName = s;
		}
	}
/* Force Init */
#ifdef PF_INIT_MODE
	IfInit = TRUE;
	DicName = NULL;
#endif

	Result = DoForth( DicName, SourceName, IfInit);
	return Result;
}

/**************************************************************************
** Conveniance routine to execute PForth
*/
int DoForth( char *DicName, char *SourceName, int IfInit )
{
	cfTaskData *cftd;
	cfDictionary *dic;
	int Result = 0;
	ExecToken  EntryPoint = 0;
	
	DBUG(("DoForth: DicName = %s, SourceName = %s\n", DicName, SourceName ));

/* Allocate Task structure. */
	cftd = pfCreateTask( DEFAULT_USER_DEPTH, DEFAULT_RETURN_DEPTH );
	
	if( cftd )
	{
		pfSetCurrentTask( cftd );
		
#if 1
/* Don't use MSG before task set. */
	MSG("DoForth: \n");
	if( IfInit ) MSG("IfInit TRUE\n");
	
	if( DicName )
	{
		MSG("DicName = "); MSG(DicName); MSG("\n");
	}
	if( SourceName )
	{
		MSG("SourceName = "); MSG(SourceName); MSG("\n");
	}
#endif


#if (!defined(PF_NO_INIT)) || (!defined(PF_NO_SHELL))
		if( IfInit )
		{
			dic = pfBuildDictionary( DEFAULT_HEADER_SIZE, DEFAULT_CODE_SIZE );
		}
		else
#endif /* !PF_NO_INIT || !PF_NO_SHELL*/
		{
			dic = pfLoadDictionary( DicName, &EntryPoint );
		}
		if( dic == NULL ) goto error;
		
		if( EntryPoint != 0 )
		{

#ifdef PF_SUPER_MODE
			pfExecuteToken( EntryPoint );
#else
			CallBackSuper((CallBackFunc)pfExecuteToken,EntryPoint,0,0);
#endif
		}
#ifndef PF_NO_SHELL
		else
		{
			if( SourceName == NULL )
			{
DBUG(("Call pfRunForth()\n"));
				Result = pfRunForth();
			}
			else
			{
				MSG("Including: ");
				MSG(SourceName);
				MSG("\n");
				Result = pfIncludeFile( SourceName );
			}
		}
#endif /* PF_NO_SHELL */
		
		pfDeleteDictionary( dic );
		pfDeleteTask( cftd );
	}
	return Result;
	
error:
	MSG("DoForth: Error occured.\n");
	DBUG(("DoForth: Error occured.\n"));
	pfDeleteTask( cftd );
	return -1;
}
