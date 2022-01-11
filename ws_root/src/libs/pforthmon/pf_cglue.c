/* @(#) pf_cglue.c 96/05/16 1.8 */
/***************************************************************
** 'C' Glue support for Forth based on 'C'
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

typedef cell (*CFunc0)( void );
typedef cell (*CFunc1)( cell P1 );
typedef cell (*CFunc2)( cell P1, cell P2 );
typedef cell (*CFunc3)( cell P1, cell P2, cell P3 );
typedef cell (*CFunc4)( cell P1, cell P2, cell P3, cell P4 );
typedef cell (*CFunc5)( cell P1, cell P2, cell P3, cell P4, cell P5 );


extern void * CustomFunctionTable[];

/***************************************************************/
int32 CallUserFunction( int32 Index, int32 ReturnMode, int32 NumParams )
{
	cell P1, P2, P3, P4, P5;
	cell Result;
	void *CF;

DBUG(("CallUserFunction: Index = %d, ReturnMode = %d, NumParams = %d\n",
	Index, ReturnMode, NumParams ));

	CF = CustomFunctionTable[Index];

	switch( NumParams )
	{
	case 0:
		Result = ((CFunc0) CF) ( );
		break;
	case 1:
		P1 = POP_DATA_STACK;
		Result = ((CFunc1) CF) ( P1 );
		break;
	case 2:
		P2 = POP_DATA_STACK;
		P1 = POP_DATA_STACK;
		Result = ((CFunc2) CF) ( P1, P2 );
		break;
	case 3:
		P3 = POP_DATA_STACK;
		P2 = POP_DATA_STACK;
		P1 = POP_DATA_STACK;
		Result = ((CFunc3) CF) ( P1, P2, P3 );
		break;
	case 4:
		P4 = POP_DATA_STACK;
		P3 = POP_DATA_STACK;
		P2 = POP_DATA_STACK;
		P1 = POP_DATA_STACK;
		Result = ((CFunc4) CF) ( P1, P2, P3, P4 );
		break;
	case 5:
		P5 = POP_DATA_STACK;
		P4 = POP_DATA_STACK;
		P3 = POP_DATA_STACK;
		P2 = POP_DATA_STACK;
		P1 = POP_DATA_STACK;
		Result = ((CFunc5) CF) ( P1, P2, P3, P4, P5 );
		break;
	default:
		pfReportError("CallUserFunction", PF_ERR_NUM_PARAMS);
		ABORT;
	}

/* Push result on Forth stack if requested. */
	if(ReturnMode == C_RETURNS_VALUE) PUSH_DATA_STACK( Result );

	return 0;
}

/***************************************************************/
int32 CreateGlueToC( char *CName, int32 Index, int32 ReturnMode, int32 NumParams )
{
	uint32 Packed;
	char FName[40];
	CStringToForth( FName, CName );
	Packed = (Index & 0xFFFF) | 0 | (NumParams << 24) |
		(ReturnMode << 31);
	DBUG(("Packed = 0x%8x\n", Packed));

	ffCreateSecondaryHeader( FName );
	CODE_COMMA( ID_CALL_C );
	CODE_COMMA(Packed);
	ffSemiColon();

	return 0;
}
