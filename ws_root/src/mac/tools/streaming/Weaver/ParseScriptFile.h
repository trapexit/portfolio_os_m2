/******************************************************************************
**
**  @(#) ParseScriptFile.h 96/11/20 1.4
**
**	File:		ParseScriptFile.h
**
**	Contains:	provide simple command file parsing for runtime variable setting
**
**	Written by:	Eric Carlson
**
**	To Do:
**
******************************************************************************/

#ifndef	__STREAMING_PARSE_SCRIPT_FILE_H
#	define	__STREAMING_PARSE_SCRIPT_FILE_H

#include <stdio.h>		/* for printf() in macros */
#include <types.h>
#include <ctype.h>		/* for isspace() macros */

#ifndef __KERNEL_TYPES_H
	#include <kernel/types.h>
#endif

#define kCommentChar		'#'
#define kNullChar			0
#define kDoubleQuoteChar	'"'
#define kSingleQuoteChar	'\''
#define kBlankChar			' '
#define kTabChar			'\t'
#define kReturnChar			'\r'
#define kNewLineChar		'\n'
#define kColonChar			':'

#define IsComment(c) ((c) == kCommentChar)
#define IsNull(c) ((c) == kNullChar)
#define IsDoubleQuote(c) ((c) == kDoubleQuoteChar)
#define IsSingleQuote(c) ((c) == kSingleQuoteChar)
#define	IsEOLN(c) (IsReturn(c) || IsLineFeed(c))
#define IsColon(c) ((c) == kColonChar)
#define IsChar(c, char) ((c) == char)
#define IsDigit(c) InRange((c), '0', '9')
#define IsHexDigit(c) (InRange((c), '0', '9') || InRange((c), 'A', 'F') || InRange((c), 'a', 'f'))

typedef void (CommandFunc)(char *verb, char *parms); 
typedef struct CmdFunctionTable_tag
{
	char 			*verb;		/* the command name	*/
	CommandFunc 	*handler;	/* ptr to the function	*/
} CmdFunctionTable, *CmdFunctionTablePtr;

/* structure to associate a token name with it's value	*/
typedef struct TokenValue
{
	char 	*string;		/* the variable/command name	*/
	ulong 	value;			/* ptr to the function	*/
} TokenValue, *TokenValuePtr;


void	Parse_ScriptError(char *fmt, ...);
char	*Parse_NextToken(char **line);
bool	Parse_StringLookup(long *strValue, char *strParam, TokenValuePtr tokenArray);
bool	Parse_EvalString(long *strValue, char **parms, char **strParam, TokenValuePtr tokenArray);
bool	Parse_GetInt(long *longVal, char **parms, char **intParam);
bool	Parse_GetFloat(float *floatVal, char **parms, char **intParam);
bool	ParseScriptFile(FILE* file, CmdFunctionTablePtr commandTbl);

#endif /* __STREAMING_PARSE_SCRIPT_FILE_H */

