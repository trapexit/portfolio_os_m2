/* @(#) pfcustom.c 96/05/16 1.6 */
/***************************************************************
** Call Custom Functions for PForth
** This file can be modified to add custom user functions.
** You could, for example, call X11 from Forth.
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

#include "pf_all.h"

#ifndef TOUCH
#define TOUCH(x) ((void)x)
#endif

/****************************************************************
** Step 1: Put your own special glue routines here.
****************************************************************/
int32 CTest0( int32 Val )
{
	MSG_NUM_D("CTest0: Val = ", Val);
	return Val+1;
}

void CTest1( int32 Val1, cell Val2 )
{

	MSG("CTest1: Val1 = "); ffDot(Val1);
	MSG_NUM_D(", Val2 = ", Val2);
}

/****************************************************************
** Step 2: Fill out this table of function pointers.
**     Do not change the name of this table! It is used by the
**     PForth kernel.
****************************************************************/
void * CustomFunctionTable[] =
{
	(void *) CTest0,
	(void *) CTest1
};

/****************************************************************
** Step 3: Add them to the dictionary.
**     Do not change the name of this routine! It is called by the
**     PForth kernel.
****************************************************************/

int32 CompileCustomFunctions( void )
{
	int32 i=0;

/* Add them to the dictionary in the same order as above table. */
/* Parameters are: Name in UPPER CASE, Index, Mode, NumParams */
	CreateGlueToC( "CTEST0", i++, C_RETURNS_VALUE, 1 );
	CreateGlueToC( "CTEST1", i++, C_RETURNS_VOID, 2 );
	TOUCH(i);

	return 0;
}


/****************************************************************
** Step 4: Recompile and link with your code.
**         Then rebuild the Forth using "pforth -i"
**         Test:   10 Ctest0 ( should print message then '11' )
****************************************************************/
