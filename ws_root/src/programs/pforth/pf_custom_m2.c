/***************************************************************
** Call Custom Functions for PForth
**
** Hardware access for debugging.
**
** This file can be modified to add custom user functions.
** You could, for example, call X11 from Forth.
**
** Author: Phil Burk
**
** Copyright 3DO and Phil Burk 1994
**
***************************************************************/

/* These files comprise the main body of the pForth kernel. */
#include "pf_all.h"

#include <kernel/super.h>
typedef Err (* CallBackFunc)(uint32,uint32,uint32);

/****************************************************************
** Step 1: Put your own special glue routines here.
****************************************************************/
int32 ReadHardwareHook( uint32 hardAddr, uint32 dummy1, uint32 dummy2 )
{
	TOUCH(dummy1);
	TOUCH(dummy2);
	return *((vuint32 *) hardAddr);
}
int32 ReadHardware( uint32 hardAddr )
{
	return CallBackSuper((CallBackFunc) ReadHardwareHook, hardAddr, 0, 0 );
}

void WriteHardwareHook( uint32 hardAddr, uint32 Val, uint32 dummy2 )
{
	TOUCH(dummy2);
	*((vuint32 *) hardAddr) = Val;
}
void WriteHardware( uint32 hardAddr, uint32 Val )
{
	CallBackSuper((CallBackFunc) WriteHardwareHook, hardAddr, Val, 0 );
}

/****************************************************************
** Step 2: Fill out this table of function pointers.
**     Do not change the name of this table! It is used by the
**     PForth kernel.
****************************************************************/
void * CustomFunctionTable[] =
{
	(void *) WriteHardware,
	(void *) ReadHardware
};

/****************************************************************
** Step 3: Add them to the dictionary.
**     Do not change the name of this routine! It is called by the
**     PForth kernel.
****************************************************************/

int32 CompileCustomFunctions( void )
{
	int i=0;

/* Add them to the dictionary in the same order as above table. */
/* Parameters are: Name in UPPER CASE, Index, Mode, NumParams */
	CreateGlueToC( "WRITEHARDWARE()", i++, C_RETURNS_VOID, 2 );
	CreateGlueToC( "READHARDWARE()", i++, C_RETURNS_VALUE, 1 );
	return i;
}


/****************************************************************
** Step 4: Recompile and link with your code.
**         Then rebuild the Forth using "pforth -i"
**         Test:   10 Ctest0 ( should print message then '11' )
****************************************************************/
