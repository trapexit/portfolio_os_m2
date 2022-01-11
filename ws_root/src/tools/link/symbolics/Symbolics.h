/*
	Written by:	Reggie Seagraves
	Copyright:	© 1994 by The 3DO Company, all rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.
*/

#ifndef SYMBOLICS_H
#define SYMBOLICS_H 1

#ifdef __3DO_DEBUGGER__	
#include <types.h>
#endif

#include "predefines.h"

#ifdef __3DO_DEBUGGER__	
struct YYSTYPE;
#include "CExpr_Types.h"
#endif

typedef struct VariableInfo
{
	unsigned long vType;
	unsigned long vKind;
	unsigned long vClass;
	unsigned long vValue;
} VariableInfo;
typedef struct SourceLineInfo
{
	unsigned long	line;
	unsigned long	address;
	unsigned long	fileOffset;
} SourceLineInfo;
typedef struct ModuleInfo
{
	unsigned long fptr;
	unsigned long fLen;
	unsigned long	beginaddr;		// code address
	unsigned long	endaddr;		// end address
	unsigned long	nbLines;		// number of source lines in that module
} ModuleInfo, *ModuleInfoPtr, **ModuleInfoHdl;
typedef struct SymbolInfo
{
	unsigned long address;
	Str255 name;	// typedef unsigned char Str255[256]
} SymbolInfo;

#include "symapi.h"
#ifdef __3DO_DEBUGGER__
#include "xsym.h"
#endif
#endif


