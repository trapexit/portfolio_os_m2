/* @(#) ezmem_tools.c 95/08/16 1.10 */
/* $Id: ezmem_tools.c,v 1.4 1995/01/26 01:36:55 peabody Exp phil $ */
/*
** Memory Allocation tools used in Audio Folio
** By:  Phil Burk
** Copyright (c) 1992, 3D0 Company.
** This program is proprietary and confidential.
**
** Derived from handy_tools.c.
*/

/*
** 00001 PLB 921203 Fixed NULL return in EZMemAlloc, use negative
**       size to indicate kernel allocation to prevent freeing
**       from wrong space.
** 930303 PLB Improved DumpMemory() format.
** 930504 PLB Return -2 error code if resource not allocated.
** 930612 PLB Added PARANOID memory checks.
** 940316 WJB Added triple bang in header about splitting this module.
** 940511 WJB Replaced internalf.h w/ clib.h.
** 940811 PLB Split from handy_tools. Differences are:
**            no EZMemSetCustomVectors
** 950125 WJB Added EZMemClone().
** 950125 WJB Cleaned up.
** 950125 WJB Added NULL trap to EZMemFree().
** 950519 WJB Turned the bulk of this module into macros for normal use.
**            Old code activated by PARANOID.
** 950816 WJB Added MEMDEBUG variants. Removed old paranoid code in favor of memdebug.
*/

#include <kernel/kernel.h>      /* IsUser() */
#include <kernel/mem.h>
#include <string.h>

#include "ezmem_tools.h"


#ifndef MEMDEBUG    /* { */

/*******************************************************************/
void * EZMemAlloc ( int32 size, uint32 type )
{
    type |= MEMTYPE_TRACKSIZE;

    return IsUser()
        ? AllocMem (size, type)
        : SuperAllocMem (size, type);
}

/*******************************************************************/
void EZMemFree ( void *ptr )
{
    if (IsUser()) {
        FreeMem (ptr, TRACKED_SIZE);
    }
    else {
        SuperFreeMem (ptr, TRACKED_SIZE);
    }
}

/*******************************************************************/
void *EZMemClone (const void *src, uint32 type)
{
    const int32 size = EZMemSize (src);
    void *dst;

    if ((dst = EZMemAlloc (size, type)) != NULL) {
        memcpy (dst, src, size);
    }
    return dst;
}

#else   /* } { */

/*******************************************************************/
void * EZMemAllocDebug ( int32 size, uint32 type, const char *file, int32 lineNum )
{
    type |= MEMTYPE_TRACKSIZE;

    return IsUser()
        ? AllocMemDebug (size, type, file, lineNum)
        : SuperAllocMemDebug (size, type, file, lineNum);
}

/*******************************************************************/
void EZMemFreeDebug ( void *ptr, const char *file, int32 lineNum )
{
    if (IsUser()) {
        FreeMemDebug (ptr, TRACKED_SIZE, file, lineNum);
    }
    else {
        SuperFreeMemDebug (ptr, TRACKED_SIZE, file, lineNum);
    }
}

/*******************************************************************/
void *EZMemCloneDebug (const void *src, uint32 type, const char *file, int32 lineNum)
{
    const int32 size = EZMemSize (src);
    void *dst;

    if ((dst = EZMemAllocDebug (size, type, file, lineNum)) != NULL) {
        memcpy (dst, src, size);
    }
    return dst;
}

#endif  /* } MEMDEBUG */


#if 0
/*****************************************************************/
void DumpMemory (const void *addr, int32 cnt)
{
    int32 ln, cn, nlines;
    unsigned char *ptr, *cptr, c;

    nlines = (cnt + 15) / 16;

    ptr = (unsigned char *) addr;

    for (ln=0; ln<nlines; ln++)
    {
        PRT(("%8x: ", ptr));
        cptr = ptr;
        for (cn=0; cn<16; cn++)
        {
            PRT(("%02x ", *cptr++));
        }
        PRT(("  "));
        for (cn=0; cn<16; cn++)
        {
            c = *ptr++;
            if ((c < '\x20') || (c > '\x7e')) c = '.';
            PRT(("%c", c));
        }
        PRT(("\n"));
    }
}
#endif
