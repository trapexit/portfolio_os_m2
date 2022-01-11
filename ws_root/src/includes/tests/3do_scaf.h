
/*
 * 3do_scaf.h
 *
 *	Copyright:	© 1996 by The 3DO Company. All rights reserved.
 *				This material constitutes confidential and proprietary
 *				information of the 3DO Company and shall not be used by
 *				any person or for any purpose except as expressly
 *				authorized in writing by the 3DO Company.
 *
 *
 *	Revision History
 *
 *	YY/MM/DD	Name  		Description
 *	--------	----------	-------------------------------------
 *	96/02/16	Bill Weir	Scaffold re-implementation, based on
 *							  components of previous versions by
 *  						  Eric Hegstrom, Eric Matsuno, and 
 *							  Nick Triantos
 *  96/04/23	Bill Weir	Addition of MemDebug capabilities,
 *							  coded by Eric Hegstrom
 *
@(#) 3do_scaf.h 96/09/12 1.2
 */


#ifndef SCAFFOLD_H

#define SCAFFOLD_H
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef K9_ONLY 
#include <kernel/operror.h>
#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/device.h>
#include <kernel/time.h>
#include <varargs.h>
#else
#include <k9types.h>
#endif


/* enumerative symbol definitions */

typedef enum eTestGroups {
	KERNEL			= 1,
	FILESYSTEM		= 2,
	AUDIO			= 3,
	GRAPHICS_3D		= 4,
	GRAPHICS_2D		= 5,
	DATA_STREAM		= 6,
	PERIPHERALS		= 7,
	INTERNATIONAL	= 8,
	GRAPHICS_CLT	= 9,
	MERCURY		= 10
} eTestGroup;

 
typedef enum eTestTypes {
	BAT			= 0x40000000,
	FT			= 0x20000000,
	EST			= 0x10000000,
	DT			= 0x08000000,
	LST			= 0x04000000,
	BT			= 0x02000000,
	PT			= 0x01000000,
	BREADTH		= 0x00800000,
	DEPTH		= 0x00400000,
	HIGH_RISK	= 0x00200000,
	IT			= 0x00100000
} eTestType;


typedef enum eTestResults {
	PASSED			= 2002,
	FAILED			= 2003,
	UNRESOLVED		= 2004,
	SETUP_FAILED	= 2005,
	SKIPPED			= 2006,
	UNAVAILABLE		= 2007
} eTestResult;


typedef enum eActionCodes {
	LOG			= 1,
	NOLOG		= 2,
	QUERY		= 3
} eActionCode;



/* function prototypes */

int32 InitScaffold(eTestGroup testgroup, int32 argc, char * argv[]);
void TermScaffold(void);
void BeginTest(int32 test_type, char * test_name, ...);
void PauseTimer(void);
void ResumeTimer(void);
void ChangeResource(int32 resource_identifier, int32 quantum);
eTestResult InteractiveVerify(char * prompt_string, ...);
void ChangeLogName(char * log_path);
void ChangeLogTargets(Boolean display, Boolean file, Boolean database);
void ChangeLogAction(eActionCode action_code);
void DeferLog(int32 log_bytes);
void LogError(int32 errno);
void LogResult(eTestResult result, char * string, ...);
void LogComment(char * string, ...);
void LogDiagnostic(int32 level, char * string, ...);
void ChangeLogLevel(int32 loglevel);
void DisplayUnconditional(char * string, ...);
void DisplayConditional(char *string, ...);
void SetResultExpected(int32 testgroup, char * testname, eTestResult result);

/* MemDebug function prototypes; 'TermXXX' funtions are hidden, invoked
	by 'TermScaffold' */

void InitMemDebug(char * flag_string);  /* wrapper for 'StartMemDebug' */
void CheckMemDebug(void);				/* new declaration */
void PrintMemDebug(void);				/* new declaration */
void InitMemRation(char * flag_string);	/* wrapper for 'StartMemRation' */

#endif
