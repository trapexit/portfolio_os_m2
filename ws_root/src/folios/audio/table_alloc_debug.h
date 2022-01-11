#ifndef __TABLE_ALLOC_DEBUG_H
#define __TABLE_ALLOC_DEBUG_H


/******************************************************************************
**
**  @(#) table_alloc_debug.h 96/01/11 1.1
**
**  Table Allocator debugging.
**
**  By: Bill Barton
**
**  Copyright (c) 1996, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  960111 WJB  Created.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#ifndef __STDIO_H
#include <stdio.h>
#endif

#ifndef __TABLE_ALLOC_H
#include "table_alloc.h"
#endif


#ifdef BUILD_STRINGS    /* { */

/* -------------------- DumpTableAllocator() */

static void DumpTableAllocator (const TableAllocator *tall, const char *banner)
{
    if (banner) printf ("%s: ", banner);
    printf ("base=0x%08lx size=%ld avail=%ld largest=%ld\n", tall->tall_Base, tall->tall_Size, TotalAvailThings(tall), LargestAvailThings(tall));
    DumpBitRange (tall->tall_Table, 0, tall->tall_Size-1, NULL);
}

#endif  /* } BUILD_STRINGS */


/*****************************************************************************/

#endif /* __TABLE_ALLOC_DEBUG_H */
