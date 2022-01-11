/* @(#) pf_core.c 96/07/15 1.12 */
/***************************************************************
** Forth based on 'C'
**
** This file has the main entry points to the pForth library.
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
** 940502 PLB Creation.
** 940505 PLB More macros.
** 940509 PLB Moved all stack handling into inner interpreter.
**        Added Create, Colon, Semicolon, HNumberQ, etc.
** 940510 PLB Got inner interpreter working with secondaries.
**        Added (LITERAL).   Compiles colon definitions.
** 940511 PLB Added conditionals, LITERAL, CREATE DOES>
** 940512 PLB Added DO LOOP DEFER, fixed R>
** 940520 PLB Added INCLUDE
** 940521 PLB Added NUMBER?
** 940930 PLB Outer Interpreter now uses deferred NUMBER?
** 941005 PLB Added ANSI locals, LEAVE, modularised
** 950320 RDG Added underflow checking for FP stack
***************************************************************/

#include "pf_all.h"
 
/***************************************************************
** Global Data
***************************************************************/

cfTaskData   *gCurrentTask;
cfDictionary *gCurrentDictionary;
int32         gNumPrimitives;
char          gScratch[TIB_SIZE];
ExecToken     gLocalCompiler_XT = 0;   /* custom compiler for local variables */

/* Global Forth variables. */
char *gVarContext = 0;      /* Points to last name field. */
cell  gVarState = 0;        /* 1 if compiling. */
cell  gVarBase = 10;        /* Numeric Base. */
cell  gVarEcho = 0;	        /* Echo input. */
cell  gVarTraceLevel = 0;   /* Trace Level for Inner Interpreter. */
cell  gVarTraceStack = 1;   /* Dump Stack each time if true. */
cell  gVarTraceFlags = 0;   /* Enable various internal debug messages. */
cell  gVarQuiet = 0;        /* Suppress unnecessary messages, OK, etc. */
cell  gVarReturnCode = 0;   /* Returned to caller of Forth, eg. UNIX shell. */

/***************************************************************
** Task Management
***************************************************************/

void pfDeleteTask( cfTaskData *cftd )
{
	FREE_VAR( cftd->td_ReturnLimit );
	FREE_VAR( cftd->td_StackLimit );
	pfFreeMem( cftd );
}
/* Allocate some extra cells to protect against mild stack underflows. */
#define STACK_SAFETY  (8)
cfTaskData *pfCreateTask( int32 UserStackDepth, int32 ReturnStackDepth )
{
	cfTaskData *cftd;

	cftd = ( cfTaskData * ) pfAllocMem( sizeof( cfTaskData ) );
	if( !cftd ) goto nomem;
	pfSetMemory( cftd, 0, sizeof( cfTaskData ));

/* Allocate User Stack */
	cftd->td_StackLimit = (cell *) pfAllocMem((uint32)(sizeof(int32) *
				(UserStackDepth + STACK_SAFETY)));
	if( !cftd->td_StackLimit ) goto nomem;
	cftd->td_StackBase = cftd->td_StackLimit + UserStackDepth;
	cftd->td_StackPtr = cftd->td_StackBase;

/* Allocate Return Stack */
	cftd->td_ReturnLimit = (cell *) pfAllocMem((uint32)(sizeof(int32) * ReturnStackDepth) );
	if( !cftd->td_ReturnLimit ) goto nomem;
	cftd->td_ReturnBase = cftd->td_ReturnLimit + ReturnStackDepth;
	cftd->td_ReturnPtr = cftd->td_ReturnBase;

/* Allocate Float Stack */
#ifdef PF_SUPPORT_FP
/* Allocate room for as many Floats as we do regular data. */
	cftd->td_FloatStackLimit = (PF_FLOAT *) pfAllocMem((uint32)(sizeof(PF_FLOAT) * UserStackDepth) );
	if( !cftd->td_FloatStackLimit ) goto nomem;
	cftd->td_FloatStackBase = cftd->td_FloatStackLimit + UserStackDepth;
	cftd->td_FloatStackPtr = cftd->td_FloatStackBase;
#endif

	cftd->td_OutputStream = (FileStream *)stdout;
	cftd->td_InputStream = (FileStream *)stdin;

	cftd->td_SourcePtr = &cftd->td_TIB[0];
	cftd->td_SourceNum = 0;
	
	return cftd;

nomem:
	ERR("CreateTaskContext: insufficient memory.\n");
	if( cftd ) pfDeleteTask( cftd );
	return NULL;
}

/***************************************************************
** Dictionary Management
***************************************************************/

/***************************************************************
** Delete a dictionary created by pfCreateDictionary()
*/
void pfDeleteDictionary( cfDictionary *dic )
{
	if( !dic ) return;
	
/* Cleanup based on AUTO.TERM chain. */
	if( NAME_BASE != NULL)
	{
		ExecToken  autoInitXT;
		if( ffFindC( "AUTO.TERM", &autoInitXT ) )
		{
			pfExecuteToken( autoInitXT );
		}
	}

	if( dic->dic_Flags & PF_DICF_ALLOCATED_SEGMENTS )
	{
		FREE_VAR( dic->dic_HeaderBase );
		FREE_VAR( dic->dic_CodeBase );
	}
	pfFreeMem( dic );
}

/***************************************************************
** Create a complete dictionary.
** The dictionary consists of two parts, the header with the names,
** and the code portion.
** Delete using pfDeleteDictionary().
** Return pointer to dictionary management structure.
*/
cfDictionary *pfCreateDictionary( uint32 HeaderSize, uint32 CodeSize )
{
/* Allocate memory for initial dictionary. */
	cfDictionary *dic;

	dic = ( cfDictionary * ) pfAllocMem( sizeof( cfDictionary ) );
	if( !dic ) goto nomem;
	pfSetMemory( dic, 0, sizeof( cfDictionary ));

	dic->dic_Flags |= PF_DICF_ALLOCATED_SEGMENTS;
	
/* Allocate memory for header. */
	if( HeaderSize > 0 )
	{
		dic->dic_HeaderBase = ( uint8 * ) pfAllocMem( (uint32) HeaderSize );
		if( !dic->dic_HeaderBase ) goto nomem;
		pfSetMemory( dic->dic_HeaderBase, 0xA5, (uint32) HeaderSize);
		dic->dic_HeaderLimit = dic->dic_HeaderBase + HeaderSize;
		dic->dic_HeaderPtr.Byte = dic->dic_HeaderBase;
	}
	else
	{
		dic->dic_HeaderBase = NULL;
	}

/* Allocate memory for code. */
	dic->dic_CodeBase = ( uint8 * ) pfAllocMem( (uint32) CodeSize );
	if( !dic->dic_CodeBase ) goto nomem;
	pfSetMemory( dic->dic_CodeBase, 0x5A, (uint32) CodeSize);

	dic->dic_CodeLimit = dic->dic_CodeBase + CodeSize;
	dic->dic_CodePtr.Byte = dic->dic_CodeBase + QUADUP(NUM_PRIMITIVES); 
	
	return dic;
nomem:
	pfDeleteDictionary( dic );
	return NULL;
}

/***************************************************************
** Used by Quit and other routines to restore system.
***************************************************************/

void ResetForthTask( void )
{
/* Go back to terminal input. */
	CURRENT_INPUT = (FileStream *)stdin;
	
/* Reset stacks. */
	gCurrentTask->td_StackPtr = gCurrentTask->td_StackBase;
	gCurrentTask->td_ReturnPtr = gCurrentTask->td_ReturnBase;
#ifdef PF_SUPPORT_FP  /* Reset Floating Point stack too! */
	gCurrentTask->td_FloatStackPtr = gCurrentTask->td_FloatStackBase;
#endif

/* Advance >IN to end of input. */
	gCurrentTask->td_IN = gCurrentTask->td_SourceNum;
	gVarState = 0;
}

/***************************************************************
** Set current task context.
***************************************************************/

void pfSetCurrentTask( cfTaskData *cftd )
{	
	gCurrentTask = cftd;
}

/***************************************************************
** Set Quiet Flag.
***************************************************************/

void pfSetQuiet( int32 IfQuiet )
{	
	gVarQuiet = (cell) IfQuiet;
}

/***************************************************************
** RunForth
***************************************************************/

int32 pfRunForth( void )
{
		
	if( !gVarQuiet )
	{
		MSG_NUM_D( "PForth V", PFORTH_VERSION );
	}
	ffQuit();
	if( !gVarQuiet ) MSG("Exiting PForth\n");
	return gVarReturnCode;
}


/***************************************************************
** Include file based on 'C' name.
***************************************************************/

int32 pfIncludeFile( char *FileName )
{
	FileStream *fid;
	int32 Result;
	
/* Open file. */
	fid = sdOpenFile( FileName, "r" );
	if( fid == NULL )
	{
		ERR("pfIncludeFile could not open ");
		ERR(FileName);
		EMIT_CR;
		return -1;
	}
	Result = ffIncludeFile( fid );
	sdCloseFile(fid);
	return Result;
}

/***************************************************************
** Output 'C' string message.
** This is provided to help avoid the use of printf() and other I/O
** which may not be present on a small embedded system.
***************************************************************/

void pfMessage( const char *CString )
{
	ioTypeCString( CString, CURRENT_OUTPUT );
	sdFlushFile( CURRENT_OUTPUT );
}
