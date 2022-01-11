/* @(#) pfcompil.c 96/07/15 1.16 */
/***************************************************************
** Compiler for PForth based on 'C'
**
** These routines could be left out of an execute only version.
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
** 941004 PLB Extracted IO calls from pforth_main.c
** 950320 RDG Added underflow checking for FP stack
***************************************************************/

#include "pf_all.h"
#include "pfcompil.h"

#define ABORT_RETURN_CODE   (10)

/***************************************************************/
/************** GLOBAL DATA ************************************/
/***************************************************************/
/* data for INCLUDE that allows multiple nested files. */
static IncludeFrame gIncludeStack[MAX_INCLUDE_DEPTH];
static int32 gIncludeIndex = 0;

/* Depth of data stack when colon called. */
#define DEPTH_AT_COLON_INVALID -1
static int32 gDepthAtColon = DEPTH_AT_COLON_INVALID;

static ExecToken gNumberQ_XT = 0;         /* XT of NUMBER? */
static ExecToken gQuitP_XT = 0;           /* XT of (QUIT) */

/***************************************************************/
/************** Static Prototypes ******************************/
/***************************************************************/

static void ffStringColon( char *FName );
static int32 CheckRedefinition( char *FName );
static void CreateDicEntry( ExecToken XT, ForthStringPtr FName, uint32 Flags );
static void CreateDeferredC( ExecToken DefaultXT, char *CName );
static void ReportIncludeState( void );
static void ffUnSmudge( void );
static void FindAndCompile( char *theWord );
  
int32 NotCompiled( const char *FunctionName )
{
	MSG("Function ");
	MSG(FunctionName);
	MSG(" not compiled in this version of PForth.\n");
	return -1;
}

#ifndef PF_NO_SHELL
/***************************************************************
** Create an entry in the Dictionary for the given ExecutionToken.
** FName is name in Forth format.
*/
static void CreateDicEntry( ExecToken XT, ForthStringPtr FName, uint32 Flags )
{
	cfNameLinks *cfnl;

	cfnl = (cfNameLinks *) gCurrentDictionary->dic_HeaderPtr.Byte;

/* Set link to previous header, if any. */
	if( gVarContext )
	{
		cfnl->cfnl_PreviousName = ABS_TO_NAMEREL( gVarContext );
	}
	else
	{
		cfnl->cfnl_PreviousName = 0;
	}

/* Put Execution token in header. */
	cfnl->cfnl_ExecToken = XT;

/* Advance Header Dictionary Pointer */
	gCurrentDictionary->dic_HeaderPtr.Byte += sizeof(cfNameLinks);

/* Laydown name. */
	gVarContext = (char *) gCurrentDictionary->dic_HeaderPtr.Byte;
	pfCopyMemory( gCurrentDictionary->dic_HeaderPtr.Byte, FName, (*FName)+1 );
	gCurrentDictionary->dic_HeaderPtr.Byte += (*FName)+1;

/* Set flags. */
	*gVarContext |= (char) Flags;
	
/* Align to quad byte boundaries with zeroes. */
	while( ((uint32) gCurrentDictionary->dic_HeaderPtr.Byte) & 3)
	{
		*gCurrentDictionary->dic_HeaderPtr.Byte++ = 0;
	}
}

/***************************************************************
** Convert name then create dictionary entry.
*/
void CreateDicEntryC( ExecToken XT, char *CName, uint32 Flags )
{
	ForthString FName[40];
	CStringToForth( FName, CName );
	CreateDicEntry( XT, FName, Flags );
}

/***************************************************************
** Convert absolute namefield address to previous absolute name
** field address or NULL.
*/
ForthString *NameToPrevious( ForthString *NFA )
{
	cell RelNamePtr;
	cell *RelLinkPtr;

DBUG(("\nNameToPrevious: NFA = 0x%x\n", (int32) NFA));
	RelLinkPtr = NAME_TO_LINK( NFA );
DBUG(("\nNameToPrevious: RelLinkPtr = 0x%x\n", (int32) RelLinkPtr ));
	RelNamePtr = *RelLinkPtr;
DBUG(("\nNameToPrevious: RelNamePtr = 0x%x\n", (int32) RelNamePtr ));
	if( RelNamePtr )
	{
		return ( NAMEREL_TO_ABS( RelNamePtr ) );
	}
	else
	{
		return NULL;
	}
}
/***************************************************************
** Convert NFA to Absolute code address.
*/
ExecToken NameToToken( ForthString *NFA )
{
	ExecToken *pXT;
	pXT = NAME_TO_CODEPTR( NFA );
	return ( *pXT );
}

/***************************************************************
** Find XTs needed by compiler.
*/
int32 FindSpecialXTs( void )
{
	
	if( ffFindC( "(QUIT)", &gQuitP_XT ) == 0) goto nofind;
	if( ffFindC( "NUMBER?", &gNumberQ_XT ) == 0) goto nofind;
DBUG(("gNumberQ_XT = 0x%x\n", gNumberQ_XT ));
	return 0;
	
nofind:
	ERR("FindSpecialXTs failed!\n");
	return -1;
}

/***************************************************************
** Build a dictionary from scratch.
*/
#ifndef PF_NO_INIT
cfDictionary *pfBuildDictionary( int32 HeaderSize, int32 CodeSize )
{
	cfDictionary *dic;

	dic = pfCreateDictionary( HeaderSize, CodeSize );
	if( !dic ) goto nomem;

	gCurrentDictionary = dic;
	gNumPrimitives = NUM_PRIMITIVES;

	CreateDicEntryC( ID_EXIT, "EXIT", 0 );
	CreateDicEntryC( ID_1MINUS, "1-", 0 );
	CreateDicEntryC( ID_1PLUS, "1+", 0 );
	CreateDicEntryC( ID_2DUP, "2DUP", 0 );
	CreateDicEntryC( ID_2LITERAL, "2LITERAL", FLAG_IMMEDIATE );
	CreateDicEntryC( ID_2LITERAL_P, "(2LITERAL)", 0 );
	CreateDicEntryC( ID_2MINUS, "2-", 0 );
	CreateDicEntryC( ID_2PLUS, "2+", 0 );
	CreateDicEntryC( ID_2OVER, "2OVER", 0 );
	CreateDicEntryC( ID_2SWAP, "2SWAP", 0 );
	CreateDicEntryC( ID_ACCEPT, "ACCEPT", 0 );
	CreateDicEntryC( ID_ALITERAL, "ALITERAL", FLAG_IMMEDIATE );
	CreateDicEntryC( ID_ALITERAL_P, "(ALITERAL)", 0 );
	CreateDicEntryC( ID_ALLOCATE, "ALLOCATE", 0 );
	CreateDicEntryC( ID_ARSHIFT, "ARSHIFT", 0 );
	CreateDicEntryC( ID_AND, "AND", 0 );
	CreateDicEntryC( ID_BAIL, "BAIL", 0 );
	CreateDicEntryC( ID_BRANCH, "BRANCH", 0 );
	CreateDicEntryC( ID_BODY_OFFSET, "BODY_OFFSET", 0 );
	CreateDicEntryC( ID_BYE, "BYE", 0 );
	CreateDicEntryC( ID_CFETCH, "C@", 0 );
	CreateDicEntryC( ID_CMOVE, "CMOVE", 0 );
	CreateDicEntryC( ID_CMOVE_UP, "CMOVE>", 0 );
	CreateDicEntryC( ID_COLON, ":", 0 );
	CreateDicEntryC( ID_COMP_EQUAL, "=", 0 );
	CreateDicEntryC( ID_COMP_NOT_EQUAL, "<>", 0 );
	CreateDicEntryC( ID_COMP_GREATERTHAN, ">", 0 );
	CreateDicEntryC( ID_COMP_U_GREATERTHAN, "U>", 0 );
	CreateDicEntryC( ID_COMP_LESSTHAN, "<", 0 );
	CreateDicEntryC( ID_COMP_U_LESSTHAN, "U<", 0 );
	CreateDicEntryC( ID_COMP_ZERO_EQUAL, "0=", 0 );
	CreateDicEntryC( ID_COMP_ZERO_NOT_EQUAL, "0<>", 0 );
	CreateDicEntryC( ID_COMP_ZERO_GREATERTHAN, "0>", 0 );
	CreateDicEntryC( ID_COMP_ZERO_LESSTHAN, "0<", 0 );
	CreateDicEntryC( ID_CR, "CR", 0 );
	CreateDicEntryC( ID_CREATE, "CREATE", 0 );
	CreateDicEntryC( ID_D_PLUS, "D+", 0 );
	CreateDicEntryC( ID_D_MINUS, "D-", 0 );
	CreateDicEntryC( ID_D_UMSMOD, "UM/MOD", 0 );
	CreateDicEntryC( ID_D_MUSMOD, "MU/MOD", 0 );
	CreateDicEntryC( ID_D_MTIMES, "M*", 0 );
	CreateDicEntryC( ID_D_UMTIMES, "UM*", 0 );
	CreateDicEntryC( ID_DEFER, "DEFER", 0 );
	CreateDicEntryC( ID_CSTORE, "C!", 0 );
	CreateDicEntryC( ID_DEPTH, "DEPTH",  0 );
	CreateDicEntryC( ID_DIVIDE, "/", 0 );
	CreateDicEntryC( ID_DOT, ".",  0 );
	CreateDicEntryC( ID_DOTS, ".S",  0 );
	CreateDicEntryC( ID_DO_P, "(DO)", 0 );
	CreateDicEntryC( ID_DROP, "DROP", 0 );
	CreateDicEntryC( ID_DUMP, "DUMP", 0 );
	CreateDicEntryC( ID_DUP, "DUP",  0 );
	CreateDicEntryC( ID_EMIT_P, "(EMIT)",  0 );
	CreateDeferredC( ID_EMIT_P, "EMIT");
	CreateDicEntryC( ID_EOL, "EOL",  0 );
	CreateDicEntryC( ID_ERRORQ_P, "(?ERROR)",  0 );
	CreateDicEntryC( ID_ERRORQ_P, "?ERROR",  0 );
	CreateDicEntryC( ID_EXECUTE, "EXECUTE",  0 );
	CreateDicEntryC( ID_FETCH, "@",  0 );
	CreateDicEntryC( ID_FIND, "FIND",  0 );
	CreateDicEntryC( ID_FILE_CREATE, "CREATE-FILE",  0 );
	CreateDicEntryC( ID_FILE_OPEN, "OPEN-FILE",  0 );
	CreateDicEntryC( ID_FILE_CLOSE, "CLOSE-FILE",  0 );
	CreateDicEntryC( ID_FILE_READ, "READ-FILE",  0 );
	CreateDicEntryC( ID_FILE_SIZE, "FILE-SIZE",  0 );
	CreateDicEntryC( ID_FILE_WRITE, "WRITE-FILE",  0 );
	CreateDicEntryC( ID_FILE_POSITION, "FILE-POSITION",  0 );
	CreateDicEntryC( ID_FILE_REPOSITION, "REPOSITION-FILE",  0 );
	CreateDicEntryC( ID_FILE_RO, "R/O",  0 );
	CreateDicEntryC( ID_FILE_RW, "R/W",  0 );
	CreateDicEntryC( ID_FINDNFA, "FINDNFA",  0 );
	CreateDicEntryC( ID_FLUSHEMIT, "FLUSHEMIT",  0 );
	CreateDicEntryC( ID_FREE, "FREE",  0 );
#include "pfcompfp.h"
	CreateDicEntryC( ID_HERE, "HERE",  0 );
	CreateDicEntryC( ID_HEXNUMBERQ_P, "(HEXNUMBER?)",  0 );
	CreateDicEntryC( ID_I, "I",  0 );
	CreateDicEntryC( ID_J, "J",  0 );
	CreateDicEntryC( ID_INCLUDE_FILE, "INCLUDE-FILE",  0 );
	CreateDicEntryC( ID_KEY, "KEY",  0 );
	CreateDicEntryC( ID_LEAVE_P, "(LEAVE)", 0 );
	CreateDicEntryC( ID_LITERAL, "LITERAL", FLAG_IMMEDIATE );
	CreateDicEntryC( ID_LITERAL_P, "(LITERAL)", 0 );
	CreateDicEntryC( ID_LOADSYS, "LOADSYS", 0 );
	CreateDicEntryC( ID_LOCAL_COMPILER, "LOCAL-COMPILER", 0 );
	CreateDicEntryC( ID_LOCAL_ENTRY, "(LOCAL.ENTRY)", 0 );
	CreateDicEntryC( ID_LOCAL_EXIT, "(LOCAL.EXIT)", 0 );
	CreateDicEntryC( ID_LOCAL_FETCH, "(LOCAL@)", 0 );
	CreateDicEntryC( ID_LOCAL_STORE, "(LOCAL!)", 0 );
	CreateDicEntryC( ID_LOOP_P, "(LOOP)", 0 );
	CreateDicEntryC( ID_LSHIFT, "LSHIFT", 0 );
	CreateDicEntryC( ID_MAX, "MAX", 0 );
	CreateDicEntryC( ID_MIN, "MIN", 0 );
	CreateDicEntryC( ID_MINUS, "-", 0 );
	CreateDicEntryC( ID_NAME_TO_TOKEN, "NAME>", 0 );
	CreateDicEntryC( ID_NAME_TO_PREVIOUS, "PREVNAME", 0 );
	CreateDicEntryC( ID_NOOP, "NOOP", 0 );
	CreateDeferredC( ID_HEXNUMBERQ_P, "NUMBER?" );
	CreateDicEntryC( ID_OR, "OR", 0 );
	CreateDicEntryC( ID_OVER, "OVER", 0 );
	CreateDicEntryC( ID_PICK, "PICK",  0 );
	CreateDicEntryC( ID_PLUS, "+",  0 );
	CreateDicEntryC( ID_PLUSLOOP_P, "(+LOOP)", 0 );
	CreateDicEntryC( ID_PLUS_STORE, "+!",  0 );
	CreateDicEntryC( ID_QUIT_P, "(QUIT)",  0 );
	CreateDeferredC( ID_QUIT_P, "QUIT" );
	CreateDicEntryC( ID_QDO_P, "(?DO)", 0 );
	CreateDicEntryC( ID_QDUP, "?DUP",  0 );
	CreateDicEntryC( ID_QTERMINAL, "?TERMINAL",  0 );
	CreateDicEntryC( ID_REFILL, "REFILL",  0 );
	CreateDicEntryC( ID_RESIZE, "RESIZE",  0 );
	CreateDicEntryC( ID_ROT, "ROT",  0 );
	CreateDicEntryC( ID_RSHIFT, "RSHIFT",  0 );
	CreateDicEntryC( ID_R_DROP, "RDROP",  0 );
	CreateDicEntryC( ID_R_FETCH, "R@",  0 );
	CreateDicEntryC( ID_R_FROM, "R>",  0 );
	CreateDicEntryC( ID_RP_FETCH, "RP@",  0 );
	CreateDicEntryC( ID_RP_STORE, "RP!",  0 );
	CreateDicEntryC( ID_SEMICOLON, ";",  FLAG_IMMEDIATE );
	CreateDicEntryC( ID_SP_FETCH, "SP@",  0 );
	CreateDicEntryC( ID_SP_STORE, "SP!",  0 );
	CreateDicEntryC( ID_STORE, "!",  0 );
	CreateDicEntryC( ID_SAVE_FORTH_P, "(SAVE-FORTH)",  0 );
	CreateDicEntryC( ID_SCAN, "SCAN",  0 );
	CreateDicEntryC( ID_SKIP, "SKIP",  0 );
	CreateDicEntryC( ID_SOURCE, "SOURCE",  0 );
	CreateDicEntryC( ID_SOURCE_SET, "SET-SOURCE",  0 );
	CreateDicEntryC( ID_SOURCE_ID, "SOURCE-ID",  0 );
	CreateDicEntryC( ID_SOURCE_ID_PUSH, "PUSH-SOURCE-ID",  0 );
	CreateDicEntryC( ID_SOURCE_ID_POP, "POP-SOURCE-ID",  0 );
	CreateDicEntryC( ID_SWAP, "SWAP",  0 );
	CreateDicEntryC( ID_TEST1, "TEST1",  0 );
	CreateDicEntryC( ID_TICK, "'", 0 );
	CreateDicEntryC( ID_TIMES, "*", 0 );
	CreateDicEntryC( ID_TO_R, ">R", 0 );
	CreateDicEntryC( ID_TYPE, "TYPE", 0 );
	CreateDicEntryC( ID_VAR_BASE, "BASE", 0 );
	CreateDicEntryC( ID_VAR_CODE_BASE, "CODE-BASE", 0 );
	CreateDicEntryC( ID_VAR_CONTEXT, "CONTEXT", 0 );
	CreateDicEntryC( ID_VAR_DP, "DP", 0 );
	CreateDicEntryC( ID_VAR_ECHO, "ECHO", 0 );
	CreateDicEntryC( ID_VAR_HEADERS_PTR, "HEADERS-PTR", 0 );
	CreateDicEntryC( ID_VAR_HEADERS_BASE, "HEADERS-BASE", 0 );
	CreateDicEntryC( ID_VAR_RETURN_CODE, "RETURN-CODE", 0 );
	CreateDicEntryC( ID_VAR_TRACE_FLAGS, "TRACE-FLAGS", 0 );
	CreateDicEntryC( ID_VAR_TRACE_LEVEL, "TRACE-LEVEL", 0 );
	CreateDicEntryC( ID_VAR_TRACE_STACK, "TRACE-STACK", 0 );
	CreateDicEntryC( ID_VAR_OUT, "OUT", 0 );
	CreateDicEntryC( ID_VAR_STATE, "STATE", 0 );
	CreateDicEntryC( ID_VAR_TO_IN, ">IN", 0 );
	CreateDicEntryC( ID_VLIST, "VLIST", 0 );
	CreateDicEntryC( ID_WORD, "WORD", 0 );
	CreateDicEntryC( ID_WORD_FETCH, "W@", 0 );
	CreateDicEntryC( ID_WORD_STORE, "W!", 0 );
	CreateDicEntryC( ID_XOR, "XOR", 0 );
	CreateDicEntryC( ID_ZERO_BRANCH, "0BRANCH", 0 );
	
	if( FindSpecialXTs() < 0 ) goto error;
	
	CompileCustomFunctions(); /* Call custom 'C' call builder. */

#ifdef PF_DEBUG
	DumpMemory( dic->dic_HeaderBase, 256 );
	DumpMemory( dic->dic_CodeBase, 256 );
#endif

	return dic;
	
error:
	pfDeleteDictionary( dic );
	return NULL;
	
nomem:
	return NULL;
}
#endif /* !PF_NO_INIT */

/*
** ( xt -- nfa 1 , x 0 , find NFA in dictionary from XT )
** 1 for IMMEDIATE values
*/
cell ffTokenToName( ExecToken XT, ForthString **NFAPtr )
{
	ForthString *NameField;
	int32 Searching = TRUE;
	cell Result = 0;
	ExecToken TempXT;
	
	NameField = gVarContext;
DBUGX(("\ffCodeToName: gVarContext = 0x%x\n", gVarContext));

	do
	{
		TempXT = NameToToken( NameField );
		
		if( TempXT == XT )
		{
DBUGX(("ffCodeToName: NFA = 0x%x\n", NameField));
			*NFAPtr = NameField ;
			Result = 1;
			Searching = FALSE;
		}
		else
		{
			NameField = NameToPrevious( NameField );
			if( NameField == NULL )
			{
				*NFAPtr = 0;
				Searching = FALSE;
			}
		}
	} while ( Searching);
	
	return Result;
}

/*
** ( $name -- $addr 0 | nfa -1 | nfa 1 , find NFA in dictionary )
** 1 for IMMEDIATE values
*/
cell ffFindNFA( ForthString *WordName, ForthString **NFAPtr )
{
	ForthString *WordChar;
	int8 WordLen;
	char *NameField, *NameChar;
	int8 NameLen;
	int32 Searching = TRUE;
	cell Result = 0;
	
	WordLen = *WordName & 0x1F;
	WordChar = WordName+1;
	
	NameField = gVarContext;
DBUG(("\nffFindNFA: gVarContext = 0x%x\n", gVarContext));
	do
	{
		NameLen = (*NameField) & MASK_NAME_SIZE;
		NameChar = NameField+1;
DBUG(("   %c\n", (*NameField & FLAG_SMUDGE) ? 'S' : 'V' ));
		if(	((*NameField & FLAG_SMUDGE) == 0) &&
			(NameLen == WordLen) &&
			ffCompareTextCaseN( NameChar, WordChar, WordLen ) ) /* FIXME - slow */
		{
DBUG(("ffFindNFA: found it at NFA = 0x%x\n", NameField));
			*NFAPtr = NameField ;
			Result = ((*NameField) & FLAG_IMMEDIATE) ? 1 : -1;
			Searching = FALSE;
		}
		else
		{
			NameField = NameToPrevious( NameField );
			if( NameField == NULL )
			{
				*NFAPtr = WordName;
				Searching = FALSE;
			}
		}
	} while ( Searching);
DBUG(("ffFindNFA: returns 0x%x\n", Result));
	return Result;
}


/***************************************************************
** ( $name -- $name 0 | xt -1 | xt 1 )
** 1 for IMMEDIATE values
*/
cell ffFind( ForthString *WordName, ExecToken *pXT )
{
	ForthString *NFA;
	int32 Result;
	
	Result = ffFindNFA( WordName, &NFA );
DBUG(("ffFind: %8s at 0x%x\n", WordName+1, NFA)); /* WARNING, not NUL terminated. %Q */
	if( Result )
	{
		*pXT = NameToToken( NFA );
	}
	else
	{
		*pXT = (ExecToken) WordName;
	}

	return Result;
}

/****************************************************************
** Find name when passed 'C' string.
*/
cell ffFindC( char *WordName, ExecToken *pXT )
{
DBUG(("ffFindC: %s\n", WordName ));
	CStringToForth( gScratch, WordName );
	return ffFind( gScratch, pXT );
}


/***********************************************************/
/********* Compiling New Words *****************************/
/***********************************************************/
#define DIC_SAFETY_MARGIN  (400)

/*************************************************************
**  Check for dictionary overflow. 
*/
int32 ffCheckDicRoom( void )
{
	int32 RoomLeft;
	RoomLeft = gCurrentDictionary->dic_HeaderLimit -
	           gCurrentDictionary->dic_HeaderPtr.Byte;
	if( RoomLeft < DIC_SAFETY_MARGIN )
	{
		pfReportError("ffCheckDicRoom", PF_ERR_HEADER_ROOM);
		return PF_ERR_HEADER_ROOM;
	}

	RoomLeft = gCurrentDictionary->dic_CodeLimit -
	           gCurrentDictionary->dic_CodePtr.Byte;
	if( RoomLeft < DIC_SAFETY_MARGIN )
	{
		pfReportError("ffCheckDicRoom", PF_ERR_CODE_ROOM);
		return PF_ERR_CODE_ROOM;
	}
	return 0;
}

/*************************************************************
**  Create a dictionary entry given a string name. 
*/
void ffCreateSecondaryHeader( char *FName)
{
/* Check for dictionary overflow. */
	if( ffCheckDicRoom() ) return;

	CheckRedefinition( FName );
/* Align CODE_HERE */
	CODE_HERE = (cell *)( (((uint32)CODE_HERE) + 3) & ~3);
	CreateDicEntry( (ExecToken) ABS_TO_CODEREL(CODE_HERE), FName, FLAG_SMUDGE );
DBUG(("ffCreateSecondaryHeader, XT = 0x%x, Name = %8s\n"));
}

/*************************************************************
** Begin compiling a secondary word.
*/
static void ffStringColon( char *FName)
{
	ffCreateSecondaryHeader( FName );
	gVarState = 1;
}

/*************************************************************
** Read the next ExecToken from the Source and create a word.
*/
void ffColon( void )
{
	char *FName;
	
	gDepthAtColon = DATA_STACK_DEPTH;
	
	FName = ffWord( BLANK );
	if( *FName > 0 )
	{
		ffStringColon( FName );
	}
}

/*************************************************************
** Check to see if name is already in dictionary.
*/
static int32 CheckRedefinition( char *FName )
{
	int32 Flag;
	ExecToken XT;
	
	Flag = ffFind( FName, &XT);
	if( Flag )
	{
		ioType( FName+1, (int32) *FName, (FileStream *) stdout );
		MSG( " already defined.\n" );
	}
	return Flag;
}

void ffStringCreate( char *FName)
{
	ffCreateSecondaryHeader( FName );
	
	CODE_COMMA( ID_CREATE_P );
	CODE_COMMA( ID_EXIT );
	ffSemiColon();
	
}

/* Read the next ExecToken from the Source and create a word. */
void ffCreate( void )
{
	char *FName;
	
	FName = ffWord( BLANK );
	if( *FName > 0 )
	{
		ffStringCreate( FName );
	}
}

void ffStringDefer( char *FName, ExecToken DefaultXT )
{
	
	ffCreateSecondaryHeader( FName );
	
	CODE_COMMA( ID_DEFER_P );
	CODE_COMMA( DefaultXT );
	
	ffSemiColon();
	
}

/* Convert name then create deferred dictionary entry. */
static void CreateDeferredC( ExecToken DefaultXT, char *CName )
{
	char FName[40];
	CStringToForth( FName, CName );
	ffStringDefer( FName, DefaultXT );
}

/* Read the next token from the Source and create a word. */
void ffDefer( void )
{
	char *FName;
	
	FName = ffWord( BLANK );
	if( *FName > 0 )
	{
		ffStringDefer( FName, ID_QUIT_P );
	}
}

/* Unsmudge the word to make it visible. */
void ffUnSmudge( void )
{
	*gVarContext &= ~FLAG_SMUDGE;
}

/* Finish the definition of a Forth word. */
void ffSemiColon( void )
{
	gVarState = 0;
	
	if( (gDepthAtColon != DEPTH_AT_COLON_INVALID) &&
	    (gDepthAtColon != DATA_STACK_DEPTH) )
	{
		pfReportError("ffSemiColon", PF_ERR_COLON_STACK);
		ffAbort();
	}
	else
	{
		CODE_COMMA( ID_EXIT );
		ffUnSmudge();
	}
	gDepthAtColon = DEPTH_AT_COLON_INVALID;
}

/**************************************************************/
/* Used to pull a number from the dictionary to the stack */
void ff2Literal( cell dHi, cell dLo )
{
	CODE_COMMA( ID_2LITERAL_P );
	CODE_COMMA( dHi );
	CODE_COMMA( dLo );
}
void ffALiteral( cell Num )
{
	CODE_COMMA( ID_ALITERAL_P );
	CODE_COMMA( Num );
}
void ffLiteral( cell Num )
{
	CODE_COMMA( ID_LITERAL_P );
	CODE_COMMA( Num );
}

#ifdef PF_SUPPORT_FP
void ffFPLiteral( PF_FLOAT fnum )
{
	/* Hack for Metrowerks complier which won't compile the 
	 * original expression. 
	 */
	PF_FLOAT  *temp;
	cell      *dicPtr;

/* Make sure that literal float data is float aligned. */
	dicPtr = CODE_HERE + 1;
	while( (((uint32) dicPtr++) & (sizeof(PF_FLOAT) - 1)) != 0)
	{
		PRT((" comma NOOP to align FPLiteral\n"));
		CODE_COMMA( ID_NOOP );
	}
	CODE_COMMA( ID_FP_FLITERAL_P );

	temp = (PF_FLOAT *)CODE_HERE;
	*temp = fnum;   /* Write to dictionary. */
	temp++;
	CODE_HERE = (cell *) temp;

/* ORIGINAL		*(((PF_FLOAT *)CODE_HERE)++) = fnum; */
}
#endif /* PF_SUPPORT_FP */

/**************************************************************/
void FindAndCompile( char *theWord )
{
	int32 Flag;
	ExecToken XT;
	cell Num;
	
	Flag = ffFind( theWord, &XT);
DBUG(("FindAndCompile: theWord = %8s, XT = 0x%x, Flag = %d\n", theWord, XT, Flag ));

/* Is it a normal word ? */
	if( Flag == -1 )
	{
		if( gVarState )  /* compiling? */
		{
			CODE_COMMA( XT );
		}
		else
		{
			pfExecuteToken( XT );
		}
	}
	else if ( Flag == 1 ) /* or is it IMMEDIATE ? */
	{
DBUG(("FindAndCompile: IMMEDIATE, theWord = 0x%x\n", theWord ));
		pfExecuteToken( XT );
	}
	else /* try to interpret it as a number. */
	{
/* Call deferred NUMBER? */
		int32 NumResult;
		
DBUG(("FindAndCompile: not found, try number?\n" ));
		PUSH_DATA_STACK( theWord );   /* Push text of number */
		pfExecuteToken( gNumberQ_XT );
DBUG(("FindAndCompile: after number?\n" ));
		NumResult = POP_DATA_STACK;  /* Success? */
		switch( NumResult )
		{
		case NUM_TYPE_SINGLE:
			if( gVarState )  /* compiling? */
			{
				Num = POP_DATA_STACK;
				ffLiteral( Num );
			}
			break;
			
		case NUM_TYPE_DOUBLE:
			if( gVarState )  /* compiling? */
			{
				Num = POP_DATA_STACK;  /* get hi portion */
				ff2Literal( Num, POP_DATA_STACK );
			}
			break;

#ifdef PF_SUPPORT_FP
		case NUM_TYPE_FLOAT:
			if( gVarState )  /* compiling? */
			{
				ffFPLiteral( *gCurrentTask->td_FloatStackPtr++ );
			}
			break;
#endif

		case NUM_TYPE_BAD:
		default:
			ioType( theWord+1, *theWord, CURRENT_OUTPUT );
			MSG( "  ? - unrecognized word!\n" );
			ffAbort( );
			break;
		
		}
	}
}
/**************************************************************
** Forth outer interpreter.  Parses words from Source.
** Executes them or compiles them based on STATE.
*/
int32 ffInterpret( void )
{
	int32 Flag;
	char *theWord;
	
/* Is there any text left in Source ? */
	while( (gCurrentTask->td_IN < (gCurrentTask->td_SourceNum-1) ) &&
		!CHECK_ABORT)
	{
DBUGX(("ffInterpret: IN=%d, SourceNum=%d\n", gCurrentTask->td_IN,
	gCurrentTask->td_SourceNum ) );
		theWord = ffWord( BLANK );
DBUGX(("ffInterpret: theWord = 0x%x, Len = %d\n", theWord, *theWord ));
		if( *theWord > 0 )
		{
			Flag = 0;
			if( gLocalCompiler_XT )
			{
				PUSH_DATA_STACK( theWord );   /* Push word. */
				pfExecuteToken( gLocalCompiler_XT );
				Flag = POP_DATA_STACK;  /* Compiled local? */
			}
			if( Flag == 0 )
			{
				FindAndCompile( theWord );
			}
		}
	}
	DBUG(("ffInterpret: CHECK_ABORT = %d\n", CHECK_ABORT));
	return( CHECK_ABORT ? -1 : 0 );
}
		
/**************************************************************/
void ffOK( void )
{
/* Check for stack underflow.   %Q what about overflows? */
	if( (gCurrentTask->td_StackBase - gCurrentTask->td_StackPtr) < 0 )
	{
		MSG("Stack underflow!\n");
		ResetForthTask( );
	}
#ifdef PF_SUPPORT_FP  /* Check floating point stack too! */
	else if((gCurrentTask->td_FloatStackBase - gCurrentTask->td_FloatStackPtr) < 0)
	{
		MSG("FP stack underflow!\n");
		ResetForthTask( );
	}
#endif
	else if( CURRENT_INPUT == (FileStream *)stdin)
	{
		if( !gVarState )  /* executing? */
		{
			if( !gVarQuiet )
			{
				ioTypeCString( "   ok\n", CURRENT_OUTPUT );
				if(gVarTraceStack) ffDotS();
			}
			else
			{
				EMIT_CR;
			}
		}
	}
}

/***************************************************************
** Report state of include stack.
***************************************************************/
static void ReportIncludeState( void )
{
	int32 i;
/* If not INCLUDing, just return. */
	if( gIncludeIndex == 0 ) return;
	
/* Report line number and nesting level. */
	MSG_NUM_D("INCLUDE error on line #", gCurrentTask->td_LineNumber );
	MSG_NUM_D("INCLUDE nesting level = ", gIncludeIndex );
	
/* Dump line of error and show offset in line for >IN */
	MSG( gCurrentTask->td_SourcePtr );
	for( i=0; i<(gCurrentTask->td_IN - 1); i++ ) EMIT('^');
	EMIT_CR;
}


/***************************************************************
** Interpret input in a loop.
***************************************************************/
void ffQuit( void )
{
	gCurrentTask->td_Flags |= CFTD_FLAG_GO;

	while( gCurrentTask->td_Flags & CFTD_FLAG_GO )
	{
		if(!ffRefill())
		{
			ERR("ffQuit: Refill returns FALSE!\n");
			return;
		}
		ffInterpret();
		DBUG(("gCurrentTask->td_Flags = 0x%x\n",  gCurrentTask->td_Flags));	
		if(CHECK_ABORT)
		{
			CLEAR_ABORT;
		}
		else
		{
			ffOK( );
		}
	}
}

/***************************************************************
** Include a file
***************************************************************/

cell ffIncludeFile( FileStream *InputFile )
{
	cell Result;
	cell Go;
	
	Result = ffPushInputStream( InputFile );
	if( Result < 0 ) return Result;
		
	Go = TRUE;
	while( Go )
	{
		if( !ffRefill() )
		{
			Go = FALSE;
		}
		else
		{
			if(ffInterpret() < 0)
			{
				Go = FALSE;
			}
			else
			{
				ffOK();
			}
		}
	}
	
/* Pop file stream. */
	ffPopInputStream();
	
	return gVarReturnCode;
}

#endif /* !PF_NO_SHELL */

/***************************************************************
** Save current input stream on stack, use this new one.
***************************************************************/
Err ffPushInputStream( FileStream *InputFile )
{
	cell Result = 0;
	IncludeFrame *inf;
	
/* Push current input state onto special include stack. */
	if( gIncludeIndex < MAX_INCLUDE_DEPTH )
	{
		inf = &gIncludeStack[gIncludeIndex];
		inf->inf_FileID = CURRENT_INPUT;
		inf->inf_LineNumber = gCurrentTask->td_LineNumber;
		gCurrentTask->td_LineNumber = 0;
		gIncludeIndex++;
	}
	else
	{
		ERR("ffPushInputStream: max depth exceeded.\n");
		return -1;
	}
	
/* Set new current input. */
DBUG(( "ffPushInputStream: InputFile = 0x%x\n", InputFile ));
	CURRENT_INPUT = InputFile;
	
	return Result;
}

/***************************************************************
** Go back to reading previous stream.
** Just return CURRENT_INPUT upon underflow.
***************************************************************/
FileStream *ffPopInputStream( void )
{
	IncludeFrame *inf;
	FileStream *Result;
	
DBUG(("ffPopInputStream: gIncludeIndex = %d\n", gIncludeIndex));
	Result = CURRENT_INPUT;
	
/* Restore input state. */
	if( gIncludeIndex > 0 )
	{
		gIncludeIndex--;
		inf = &gIncludeStack[gIncludeIndex];
		CURRENT_INPUT = inf->inf_FileID;
		gCurrentTask->td_LineNumber = inf->inf_LineNumber;
	}

	return Result;
}

/***************************************************************
** Convert file pointer to value consistent with SOURCE-ID.
***************************************************************/
cell ffConvertStreamToSourceID( FileStream *Stream )
{
	cell Result;
	if(Stream == (FileStream *)stdin)
	{
		Result = 0;
	}
	else if(Stream == NULL)
	{
		Result = -1;
	}
	else
	{
		Result = (cell) Stream;
	}
	return Result;
}

/***************************************************************
** Cleanup Include stack by popping and closing files.
***************************************************************/
void ffCleanIncludeStack( void )
{
	FileStream *cur;
	
	while( (cur = ffPopInputStream()) != stdin)
	{
		DBUG(("ffCleanIncludeStack: closing 0x%x\n", cur ));
		sdCloseFile(cur);
	}
}

/**************************************************************/
void ffAbort( void )
{
#ifndef PF_NO_SHELL
	ReportIncludeState();
#endif /* PF_NO_SHELL */
	ffCleanIncludeStack();
	ResetForthTask();
	SET_ABORT;
	if( gVarReturnCode == 0 ) gVarReturnCode = ABORT_RETURN_CODE;
}

/* ( -- , fill Source from current stream ) */
cell ffRefill( void )
{
	cell Num, Result = FTRUE;

/* get line from current stream */
	Num = ioAccept( gCurrentTask->td_SourcePtr,
		TIB_SIZE, CURRENT_INPUT );
	if( Num < 0 )
	{
		Result = FFALSE;
		Num = 0;
	}
/* reset >IN for parser */
	gCurrentTask->td_IN = 0;
	gCurrentTask->td_SourceNum = Num;
	gCurrentTask->td_LineNumber++;  /* Bump for include. */
	
/* echo input if requested */
	if( gVarEcho && ( Num > 0))
	{
		MSG( gCurrentTask->td_SourcePtr );
	}
	
	return Result;
}
