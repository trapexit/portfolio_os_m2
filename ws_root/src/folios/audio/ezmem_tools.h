#ifndef __EZMEM_TOOLS_H
#define __EZMEM_TOOLS_H


/****************************************************
**
**  @(#) ezmem_tools.h 96/07/02 1.10
**  $Id: ezmem_tools.h,v 1.7 1995/01/31 19:23:38 peabody Exp phil $
**  Include for AudioFolio Memory Allocation Tools
**
******************************************************
** 940811 PLB Stripped from handy_tools.c
** 950125 WJB Removed prototype for EZMemSetCustomVectors().
** 950125 WJB Added EZMemClone().
** 950125 WJB Cleaned up.
** 950125 WJB Added prototypes for SuperMemAlloc/Free().
** 950519 WJB Turned the bulk of this module into macros for normal use.
**            Old code activated by PARANOID.
******************************************************/

#include <kernel/mem.h>
#include <kernel/types.h>


/* -------------------- Functions */

    /* normal */
void *EZMemAlloc (int32 size, uint32 type);
void *EZMemClone (const void *src, uint32 type);
void  EZMemFree (void *ptr);

#ifdef MEMDEBUG

    /* when doing memory debugging, redirect these functions to the debugging versions */
#define EZMemAlloc(s,f) EZMemAllocDebug(s,f,__FILE__,__LINE__)
#define EZMemClone(p,f) EZMemCloneDebug(p,f,__FILE__,__LINE__)
#define EZMemFree(p)    EZMemFreeDebug(p,__FILE__,__LINE__)

void *EZMemAllocDebug (int32 size, uint32 type, const char *file, int32 lineNum);
void *EZMemCloneDebug (const void *src, uint32 type, const char *file, int32 lineNum);
void  EZMemFreeDebug (void *ptr, const char *file, int32 lineNum);

#endif

#ifdef PARANOID
	int32 EZMemSize (const void *ptr);
/*	void *SuperMemAlloc (int32 size, uint32 type);  @@@ no longer used */
/*	void  SuperMemFree (void *p, int32 size);       @@@ no longer used */
/*	void *UserMemAlloc (int32 size, uint32 type);   @@@ no longer used */
/*	void  UserMemFree (void *p, int32 size);        @@@ no longer used */
#else
	#define EZMemSize GetMemTrackSize
/*	#define SuperMemAlloc SuperAllocMem @@@ no longer used */
/*	#define SuperMemFree  SuperFreeMem  @@@ no longer used */
/*	#define UserMemAlloc  AllocMem      @@@ no longer used */
/*	#define UserMemFree   FreeMem       @@@ no longer used */
#endif

    /* debug */
#ifdef DEBUG
	void  DumpMemory (const void *addr, int32 cnt);
#endif

#endif
