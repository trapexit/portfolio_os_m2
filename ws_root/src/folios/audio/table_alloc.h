#ifndef __TABLE_ALLOC_H
#define __TABLE_ALLOC_H


/****************************************************
**
**  @(#) table_alloc.h 96/01/11 1.11
**  $Id: table_alloc.h,v 1.4 1995/02/03 02:19:44 peabody Exp phil $
**
**  Includes for Table Allocation for audio folio
**
******************************************************
** 940811 PLB Stripped from handy_tools.c
** 940812 WJB Added types.h
** 950131 WJB Added AllocAlignedThings() prototype.
** 950202 WJB Cleaned up slightly.
** 950531 WJB Added TableAllocData typedef and some support macros as prep for BitArray.
** 950601 WJB Redesigned using bit arrays.
** 950614 WJB Added AllocTheseThings().
******************************************************/

#include <kernel/bitarray.h>
#include <kernel/types.h>


/* -------------------- Structures */

typedef uint32 TableAllocData;
#define TABLE_ALLOC_THINGS_PER_UNIT BITSPERUINT32

#define TableAllocDataSize(nthings) (((nthings) + TABLE_ALLOC_THINGS_PER_UNIT - 1) / TABLE_ALLOC_THINGS_PER_UNIT * sizeof(TableAllocData))

typedef struct TableAllocator {
    int32  tall_Size;               /* How many there are to allocate */
    int32  tall_Base;               /* "Base" of allocated things (must be aligned to same PowerOf2 as passed to AllocAlignedThings()) */
    TableAllocData *tall_Table;     /* Table of allocated markers */
    int32  tall_NumAllocated;       /* How many have been allocated */
} TableAllocator;


/* -------------------- Functions */

    /* allocator */
int32 AllocThings (TableAllocator *, int32 Many);
int32 AllocAlignedThings (TableAllocator *, int32 Many, uint32 PowerOf2);
bool AllocTheseThings (TableAllocator *, int32 Start, int32 Many);
void FreeThings (TableAllocator *, int32 Start, int32 Many);
int32 TotalAvailThings (const TableAllocator *);
int32 LargestAvailThings (const TableAllocator *);


#endif
