/****************************************************************************
**
**  @(#) getconfig.c 96/11/20 1.2
**
**  code to parse and convert command line arguments according to
**	supplied rules
**
*****************************************************************************/

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

#include "getconfig.h"
	
static	bool	StringToFloat(char *str, float *floatPtr);
static	bool	StringToUInt32(char *str, uint32 *longPtr);
static	bool	StringToInt32(char *str, int32 *longPtr);
static	bool	StringToBool(char *str, bool *boolPtr);

/*
 * look for a string match in the rule table, return the rule table entry 
 */
static	OptConvertRule	*FindArgDef(OptConvertRule *ruleTable, char *str);

OptConvertRule *
FindArgDef(OptConvertRule *ruleTable, char *str)
{
	while ( NULL != ruleTable->argStr )
	{
		/* match the string? */
		if ( 0 == strcmp(ruleTable->argStr, str) )
			return ruleTable;
		
		/* bump along to the next table entry */
		ruleTable++;
	}
	return NULL;
}


/*
 * GetConfigSettings
 *  convert command line strings to variables specified in "ruleTable" 
 *	 "ruleTable" is an array of rules for conversion.  Each entry 
 *	 defines a string to match, the type of variable to convert it
 *	 into, optional flags further defining the conversion, and the 
 *   address into which the converted arg should be written. The
 *	 last entry in ruleTable MUST BE NULL.
 *	arguments matched in "ruleTable" are removed from argv (and the
 *   remaining arguments are shuffled down to account for it, argc
 *	 is modified to account for the removal.
 */
bool
GetConfigSettings(int *argc, char **argv, OptConvertRule *ruleTable, 
					bool *verbose, int32 minArgCount, int32 maxArgCount)
{
#define	GetArg()	(argv[curArg++])
#define	UnGetArg()	(--curArg)
#define	DEBUG_ECHO(printfArgs)		if ( true == *verbose ) {printf printfArgs;}

	char			**processedArgs;
	char			*argValue;
	char			*argCmd;
	int32			curArg = 0;
	int32			argCount;
	int32			argsReturned;
	OptConvertRule	*argDefn;
	bool			success = true;

	/* sanity check the argument count... */
	if ( (*argc < minArgCount) || (*argc > maxArgCount) )
	{
		printf("## ERROR: illegal argument count (min = %ld, max = %ld)\n", minArgCount, maxArgCount);
		success = false;
		goto DONE;
	}

	/* point "processedArgs" at "argv".  since we don't care about args which we have already
	 *  matched and converted, we can copy unused args back to the begining of "processedArgs" 
	 *  so the caller gets an array of all unused args without us having to allocate more memory
	 */
	processedArgs = argv;
	argsReturned = 0;
	argCount = *argc;
	
	while ( curArg < argCount )
	{
		/*  see if there is a match for string in the argument table */
		argCmd = GetArg();
		argDefn = FindArgDef(ruleTable, argCmd);
		if ( NULL == argDefn )
		{
			/* didn't find a match, copy option to the output list */
			DEBUG_ECHO(("skipping arg: \"%s\"\n", *argv));
			*processedArgs++ = argCmd;
			++argsReturned;
			continue;
		}
		
		/* adjust the cmdline arg array and counter */
		argValue = GetArg();
		switch ( argDefn->argType )
		{
			case UINT32_TYPE:
				if ( false == StringToUInt32(argValue, (uint32 *)argDefn->valueAddr) )
				{
					printf("## ERROR: processing arg \"%s\", unable to convert arg \"%s\" to uint32\n", 
							argDefn->argStr, argValue);
					success = false;
				}
				else
					DEBUG_ECHO(("processed arg \"%s\", \"%s\" converted to uint32\n", argDefn->argStr, argValue));
				break;
			case INT32_TYPE:
				if ( false == StringToInt32(argValue, (int32 *)argDefn->valueAddr) )
				{
					printf("## ERROR: processing arg \"%s\", unable to convert arg \"%s\" to int32\n", 
							argDefn->argStr, argValue);
					success = false;
				}
				else
					DEBUG_ECHO(("processed arg \"%s\", \"%s\" converted to int32\n", argDefn->argStr, argValue));
				break;
			case FLOAT_TYPE:
				if ( false == StringToFloat(argValue, (float *)argDefn->valueAddr) )
				{
					printf("## ERROR: processing arg \"%s\", unable to convert arg \"%s\" to float\n", 
							argDefn->argStr, argValue);
					success = false;
				}
				else
					DEBUG_ECHO(("processed arg \"%s\", \"%s\" converted to float\n", argDefn->argStr, argValue));
				break;
			case STRING_TYPE:
				*((char **)argDefn->valueAddr) = argValue;
				DEBUG_ECHO(("processed arg \"%s\", \"%s\" converted to string\n", argDefn->argStr, argValue));
				break;
			case BOOL_W_ARG_TYPE:
				if ( false == StringToBool(argValue, (bool *)argDefn->valueAddr) )
				{
					printf("## ERROR: processing arg \"%s\", unable to convert arg \"%s\" to boolean\n", 
							argDefn->argStr, argValue);
					success = false;
				}
				else
					DEBUG_ECHO(("processed arg \"%s\", \"%s\" converted to boolean\n", argDefn->argStr, argValue));
				break;
			case BOOL_TOGGLE_VAL_TYPE:
			{
				bool		tempBool;
				UnGetArg();				/* push back arg index, no arg value needed */
				/* cast existing value to boolean (!! forces to boolean) and flip value */
				tempBool = !(!!(*(bool *)argDefn->valueAddr));
				*(bool *)argDefn->valueAddr = tempBool;
				DEBUG_ECHO(("processed arg \"%s\", variable set to %s\n", argDefn->argStr, tempBool ? "true" : "false"));
				break;
			}
			case COPY_T0_BUFFER:
			{
				int32	argLen = strlen(argValue);
				memcpy((char *)argDefn->valueAddr, argValue, (argLen <= argDefn->argFlags) ? argLen : argDefn->argFlags);
				DEBUG_ECHO(("processed arg \"%s\", copied %ld bytes\n", argDefn->argStr, (int32)argDefn->argFlags));
				break;
			}
			default:
				/* undefined argument type, throw an error and copy arg to output list*/
				printf("## ERROR: unknown arg type \"%ld\".  arg \"%s\" unprocessed\n", 
							argDefn->argType, argValue);
				*processedArgs++ = argValue;
				++argsReturned;
				success = false;
				break;
		}
	}
	*argc = (int)argsReturned;

DONE:
	return success;
	
#undef	GetArg
#undef	UnGetArg
#undef	DEBUG_ECHO
}


/*
 * StringToBool
 *	Try to convert a string to a boolean.  return true if
 *	 it works, false otherwise.
 */ 
bool
StringToBool(char *str, bool *boolPtr)
{
	bool		success = true;
	
	if ( ('t' == *str) || ('T' == *str) || ('1' == *str) )
		*boolPtr = true;
	else if ( ('f' == *str) || ('F' == *str) || ('0' == *str)  )
		*boolPtr = false;
	else
		success = false;
	return success;
}


/*
 * StringToInt32, StringToUInt32
 *	Try to convert a string to a 32bit integer.  return true if
 *	 it works, false otherwise.
 */ 
bool
StringToInt32(char *str, int32 *longPtr)
{
    int32  tempLong;
    char	*endChar;

	tempLong = strtol(str, &endChar, 0);
	if ( (endChar == str) || (*endChar != 0) )
		return false;
	*longPtr = tempLong;
	return true;
}

bool
StringToUInt32(char *str, uint32 *longPtr)
{
    int32  tempLong;
    char	*endChar;

	tempLong = strtoul(str, &endChar, 0);
	if ( (endChar == str) || (*endChar != 0) )
		return false;
	*longPtr = tempLong;
	return true;
}


/*
 * StringToFloat
 *	Try to convert a string to a 32bit floating point number.  return true if
 *	 it works, false otherwise.
 */ 
bool
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


/* ********************************************************************************* */
/* ******************** TEST CASE FOR ARG PROCESSING CODE ABOVE ******************** */
#define	TEST_CASE	0

#if TEST_CASE
char	*stringPtr	= "default string 1";
uint32	uLongVar	= 0;
int32	longVar		= 0;
float	floatVar	= 0;
char	*stringPtr2	= "default string 2";

OptConvertRule gDefTable[] =
{
	{"-s",		STRING_TYPE,	0,	&stringPtr },
	{"-u",		UINT32_TYPE,	0,	&uLongVar },
	{"-l", 		INT32_TYPE,		0,	&longVar },
	{"-f",		FLOAT_TYPE,		0,	&floatVar },
	{"-s2",		STRING_TYPE,	0,	&stringPtr2 },
	{NULL,		0,				0,	NULL },
};

void main (int argc, char** argv)
{

#if 0
	argc = ccommand(&argv);
#endif

	GetConfigSettings(&argc, argv, gDefTable, true);
	
	printf("stringPtr   = %s\n"\
			"uLongVar   = 0x%lx\n"\
			"longVar    = %ld\n"\
			"floatVar   = %f\n"\
			"stringPtr2 = %s\n",
			stringPtr, uLongVar, longVar, floatVar, stringPtr2);
}
#endif
