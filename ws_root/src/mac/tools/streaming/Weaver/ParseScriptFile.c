/******************************************************************************
**
**  @(#) ParseScriptFile.c 96/11/20 1.5
**
**	File:		ParseScriptFile.c
**	
**	Contains:	provide simple command file parsing for runtime variable setting
**	
**	Written by:	Eric Carlson
**	
**	To Do:
**
******************************************************************************/

#include <types.h>

#include <stdio.h>		/* for printf() */

#include <stdarg.h>		/* for va_xxx() */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>		/* for isspace() macro */
#include "ParseScriptFile.h"


/*----------------------------------------------------------------------------
 * private prototypes...
 *--------------------------------------------------------------------------*/

static	bool 		ParseScriptLine(char *line, CmdFunctionTablePtr commandTbl);
static	char		*SkipSpaces(char *str);
static	bool		StringToInt32(char *str, long *longPtr);
static	bool		StringToFloat(char *str, float *floatPtr);
static	int			strcasecmp(const char *a, const char *b);


/*----------------------------------------------------------------------------
 * internal stuff...
 *--------------------------------------------------------------------------*/
/* globals */
static	long 	gScriptLineNum  = 0;


/*----------------------------------------------------------------------------
 * functions...
 *--------------------------------------------------------------------------*/

/*
 *  SkipSpaces
 *	 skip whitespace in the current line buffer
 */
static char *
SkipSpaces(char *str)
{
	if ( str == NULL ) 
		return NULL;
	
	while (isspace(*str)) 
		++str;
	
	return str;
}


/*
 * StringToInt32
 *	Try to convert a string to a 32bit integer.  return true if it works, false 
 *	 otherwise.
 */ 
static bool
StringToInt32(char *str, long *longPtr)
{
	long	tempLong;
    char	*endChar;

	/* ask the C lib to convert the number to a long (any base), see if  */
	/*	it advanced the char pointer to the end of the string.  if not, */
	/*	it didn't like what it found */
	tempLong = strtol(str, &endChar, 0);
	if ( (endChar == str) || (*endChar != 0) )
		return 0;
	
	*longPtr = (long)tempLong;
	return 1;
}

/*
 * StringToFloat
 *	Try to convert a string to a 32bit floating point number.  return true if
 *	 it works, false otherwise.
 */ 
static bool
StringToFloat(char *str, float *floatPtr)
{
	char	*endChar;
	float	tempFloat;
	
	/* ask the C lib to convert the number, see if it advanced the char 
	 *  pointer to the end of the string.  if not, it didn't like what 
	 *  it found 
	 */
#ifdef __DCC__
	tempFloat = strtof(str, &endChar);
#else
	/* building on the Mac/PC, no such thing as "strtof()" */
	tempFloat = (float)strtod(str, &endChar);
#endif
	
	if ( (endChar == str) || (*endChar != 0) )
	{
		int32	tempLong;
		/* that didn't work try it as a long */
		if ( false == StringToInt32(str, &tempLong) )
			return 0;
		tempFloat = (float)tempLong;
	}
	
	*floatPtr = tempFloat;
	return 1;
}


/*
 * strcasecmp
 *	case insensitive string compare
 */
static int
strcasecmp(const char *a, const char *b)
{
	char	char1;
	char	char2;
	int		diff;
	
    while ( 1 )
    {	
		/* are the chars different yet? */
		diff = (int)(char1 = toupper(*a++)) - (int)(char2 = toupper(*b++));
		if ( 0 != diff )
			return diff;
		
		/* at the end of first string yet? (no need to check char2, we would have  */
		/*	returned a non-zero result above if only one char was NULL) */
		if ( 0 == char1 )
			return 0;
    }
}


/*
 * Parse_ScriptError
 *	Throw an error message.
 */
void
Parse_ScriptError(char *fmt, ...)
{
	va_list args;

	fflush(NULL);

	/* dump the message we've been passed */
	fprintf(stderr, "\n###\n### Error: ");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	/* add line number info if possible */
	if ( gScriptLineNum > 0 )
		fprintf(stderr, "Line %ld\n\n", gScriptLineNum);
	fprintf(stderr, "\n###\n");
}


/*
 * Parse_StringLookup
 *	Look for a string in the string array passed.
 *	 Return the index of the matching string
 *
 *	NOTE: this code assumes that the last entry in the string array is NULL
 */
bool
Parse_StringLookup(long *strValue, char *strParam, TokenValuePtr tokenArray)
{
	long	stringNdx;

	/* and look for it in the difficulty string table */
	stringNdx = 0;
	while ( tokenArray[stringNdx].string != NULL ) 
	{
		if ( 0 == strcasecmp(strParam, tokenArray[stringNdx].string) ) 
			goto STRING_VALID;

		++stringNdx;
	}
	
	/* nothing found... */
	return false;

STRING_VALID:
	*strValue = tokenArray[stringNdx].value;
	return true;
}
	
	
/*
 * Parse_EvalString
 *	Look for a string matching the next token in the string array passed.
 *	 Return the index of the matching string
 *
 *	NOTE: this code assumes that the last entry in the string array is NULL
 */
bool
Parse_EvalString(long *strValue, char **parms, char **strParam, TokenValuePtr tokenArray)
{
	*strParam = Parse_NextToken(parms);
	if ( *strParam == NULL )
		return false;

	/* look for the string in the array */
	return Parse_StringLookup(strValue, *strParam, tokenArray);	
}


/*
 * Parse_GetInt
 *	Grab a signed int from the input stream.  either base 10 or base 16
 *	 numbers are valid as strtol does the correct thing in either case
 */
bool
Parse_GetInt(long *longVal, char **parms, char **intParam)
{
	*intParam = Parse_NextToken(parms);
	if ( *intParam == NULL ) 
		goto ERROR_EXIT;
	else 
	{
		if ( false == StringToInt32(*intParam, longVal) )
			goto ERROR_EXIT;
	}
	
	return true;

ERROR_EXIT:
	return false;
}

/*
 * Parse_GetFloat
 *	Grab a float from the input stream.
 */
bool
Parse_GetFloat(float *floatVal, char **parms, char **intParam)
{
	*intParam = Parse_NextToken(parms);
	if ( *intParam == NULL ) 
		goto ERROR_EXIT;
	else 
	{
		if ( false == StringToFloat(*intParam, floatVal) )
			goto ERROR_EXIT;
	}
	
	return true;

ERROR_EXIT:
	return false;
}


/*
 * Parse_NextToken
 *	Parse the next token out of the string passed.  Return a C string of all 
 *	 non blank chars up to the first space, or of everything within quotes.
 *	 Advance the line pointer beyond the token parsed
 */
char * 
Parse_NextToken(char **line)
{
	char	*token;
	char	*str;
	char	*wrk;
	char	aChar;
	
	/* first, strip away leading whitespace */
	str = SkipSpaces(*line);
	
	/* if already at end of line, or rest of line is a comment, return NULL to  */
	/*  indicate such */
	aChar = *str;
	if ( IsNull(aChar) || IsComment(aChar) ) 
	{							 
		return NULL;
	}
	
	/* skip past quotes, making sure they are matched */
	if ( IsDoubleQuote(aChar) || IsSingleQuote(aChar) ) 
	{
		token = ++str;
		if ( NULL == (wrk = strchr(str, aChar)) ) 
		{
			Parse_ScriptError(("mismatched quotes"));
		}
		*wrk++ = 0;
		*line = wrk;
	} 
	else 
	{
		token = wrk = str;
		for (;;) 
		{
			switch (*wrk) 
			{
			  case kNullChar:
			  case kCommentChar:
			  	*wrk = 0;
			  	goto DONE;
			  case kBlankChar:
			  case kTabChar:
			  case kReturnChar:
			  case kNewLineChar:
			  	*wrk++ = 0;
				goto DONE;
			  default:
			  	++wrk;
				break;
			}
		}
DONE:
		*line = wrk;
	}

	return token;
}


/*
 *  ParseScriptLine
 *	 Parse a line of the script buffer
 */
static bool 
ParseScriptLine(char *line, CmdFunctionTablePtr commandTbl)
{
	char			*verb;
	CmdFunctionTablePtr	command;	

	/* if it is an empty line, ignore it */
	if ( NULL == (verb = Parse_NextToken(&line)) ) 
	{
		return true;
	}

	/* look for it in the caller's command table */
	for ( command = commandTbl; command->verb != NULL; ++command ) 
	{
		if ( 0 == strcasecmp(verb, command->verb) ) 
		{
			command->handler(verb, line);
			return true;
		}
	}

	/* we don't get this far unless we don't understand the input, holler and die */
	Parse_ScriptError(("unknown command: %s %s", verb, line));
	return false;
}


/*
 *  ParseScriptFile
 *	 Parse a line of the script buffer
 */
bool 
ParseScriptFile(FILE* file, CmdFunctionTablePtr commandTbl)
{
static	char	gLineBuff[512];
	int		charNdx;
	bool	success = true;
	char	c;

	/* read and process the file a line at a time */
	charNdx = 0;
	gScriptLineNum = 0;
	while ( (c = getc(file)) > 0 )
	{
		gLineBuff[charNdx] = c;
		if ( '\n' == c )
		{
			++gScriptLineNum;
			gLineBuff[charNdx] = '\0';
			if ( false == ParseScriptLine(gLineBuff, commandTbl) )
			{
				success = false;
				break;
			}
			charNdx = 0;
		}
		else
			++charNdx;
	}
	
	return success;
}

