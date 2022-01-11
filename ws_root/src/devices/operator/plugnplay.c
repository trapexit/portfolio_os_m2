/* @(#) plugnplay.c 96/08/07 1.51 */

/*#define DEBUG*/

#include <kernel/mem.h>
#include <kernel/kernel.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/debug.h>
#include <kernel/sysinfo.h>
#include <kernel/ddfnode.h>
#include <kernel/ddfuncs.h>
#include <kernel/dipir.h>
#include <file/directoryfunctions.h>
#include <file/filesystem.h>
#include <file/fileio.h>
#include <file/filefunctions.h>
#include <misc/event.h>
#include <stdio.h>
#include <string.h>


#ifndef BUILD_STRINGS
#undef DEBUG
#endif

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x)
#define PrintfSysErr(err)
#endif


Err
CreateBuiltinDDFs(void)
{
#define	PROCESS_BUILTIN(x) { \
	extern char x##_Buffer[]; \
	extern int32 x##_Len; \
	ProcessDDFBuffer(x##_Buffer+12, x##_Len-12, 0, 0); }

	DBUG(("CreateBuiltinDDFs\n"));
	PROCESS_BUILTIN(operatorDDF)
	return 0;
}

/*
 * 2.11 Hot insertion/removal
 * Whenever the system hardware configuration changes, the HWConfigDaemon task
 * (or perhaps the Operator) performs the following steps:
 */

