/* @(#) table_alloc.c 96/07/10 1.12 */
/* $Id: table_alloc.c,v 1.5 1995/01/19 00:16:29 phil Exp phil $ */
/*
** Table based allocator Used in Audio Folio
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
** 940811 PLB Split from handy_tools.c
** 940812 WJB Commented out dead code (PrintThings()).
**            Removed a ton of extraneous includes and added the one that should be here.
** 940907 PLB Put back includes necessary for proper compilation.
** 950116 PLB Check for NULL table in MarkThings
** 950601 WJB Redesigned using bit arrays.
** 950614 WJB Added AllocTheseThings().
*/

#include "table_alloc.h"


/* -------------------- Local functions */

    /* non-standard bit array stuff */
static int32 FindAlignedClearBitRange (const uint32 *array, uint32 firstBit, uint32 lastBit, uint32 numBits, uint32 alignment);
static int32 FindClearBit (const uint32 *array, uint32 firstBit, uint32 lastBit);


/* -------------------- Allocator */

/*******************************************************************/
int32 AllocThings (TableAllocator *tall, int32 Many)
{
    int32 StartIndex;

        /* find space in table */
        /* Optimize for Many == 1 */
    if (Many == 1) {
        if ((StartIndex = FindClearBit (tall->tall_Table, 0, tall->tall_Size-1)) < 0) return -1;
    }
    else {
        if ((StartIndex = FindClearBitRange (tall->tall_Table, 0, tall->tall_Size-1, Many)) < 0) return -1;
    }

        /* mark it and return result */
    SetBitRange (tall->tall_Table, StartIndex, StartIndex + Many - 1);
    tall->tall_NumAllocated += Many;

    return StartIndex + tall->tall_Base;
}


/*******************************************************************/
int32 AllocAlignedThings (TableAllocator *tall, int32 Many, uint32 PowerOf2)
{
    int32 StartIndex;

  #if 0 /* @@@ old system tolerated tall_Base being on an arbitrary alignment */
        /* Start on an aligned power of 2 boundary. Index must account for offset. */
        /*  P2  Offset  i
             1   0x108  0
             1   0x103  0
             8   0x100  0
             8   0x101  7
             8   0x102  6
             8   0x107  1
             8   0x108  0
        */
    if ((StartIndex = FindAlignedClearBitRange (tall->tall_Table, (PowerOf2 - (tall->tall_Base & (PowerOf2-1))) & (PowerOf2-1), tall->tall_Size-1, Many, PowerOf2)) < 0) return -1;
  #endif

        /* find space in table */
        /* @@@ Requires tall_Base to be on PowerOf2 alignment */
    if ((StartIndex = FindAlignedClearBitRange (tall->tall_Table, 0, tall->tall_Size-1, Many, PowerOf2)) < 0) return -1;

        /* mark it and return result */
    SetBitRange (tall->tall_Table, StartIndex, StartIndex + Many - 1);
    tall->tall_NumAllocated += Many;

    return StartIndex + tall->tall_Base;
}


/*******************************************************************/
bool AllocTheseThings (TableAllocator *tall, int32 Start, int32 Many)
{
    const int32 StartIndex = Start - tall->tall_Base;

        /* find space in table */
    if (!IsBitRangeClear (tall->tall_Table, StartIndex, StartIndex + Many - 1)) return FALSE;

        /* mark it and return success */
    SetBitRange (tall->tall_Table, StartIndex, StartIndex + Many - 1);
    tall->tall_NumAllocated += Many;

    return TRUE;
}


/*******************************************************************/
void FreeThings (TableAllocator *tall, int32 Start, int32 Many)
{
        /* clear it */
    ClearBitRange (tall->tall_Table, Start - tall->tall_Base, Start - tall->tall_Base + Many - 1);
    tall->tall_NumAllocated -= Many;
}


/*******************************************************************/
int32 TotalAvailThings (const TableAllocator *tall)
{
    return tall->tall_Size - tall->tall_NumAllocated;
}


/*******************************************************************/
/* !!! might want to find some suitable BUILD_ define to surround this with (BUILD_DEBUGGER, BUILD_STRINGS, a new one?) */
int32 LargestAvailThings (const TableAllocator *tall)
{
    int32 largestspan = 0;
    int32 span = 0;
    int32 i;

    for (i=0; i<tall->tall_Size; i++) {
        if (IsBitClear (tall->tall_Table, i)) {
            span++;
        }
        else if (span) {
            if (span > largestspan) largestspan = span;
            span = 0;
        }
    }
    if (span > largestspan) largestspan = span;

    return largestspan;
}


/* -------------------- Non-std BitArray support */

/* bitarray constants */
#define BITS    BITSPERUINT32
#define LOG     5           /* 2^LOG == BITS */
#define MASK    (BITS - 1)

/******************************************************************
**
**  Find a range of cleared bits on a specific power of 2 alignment.
**
**  Inputs
**
**      array
**
**      firstBit, lastBit
**
**          Range of bits to search. These do not need to be on any
**          particular alignment.
**
**      numBits
**
**      alignment
**
**          The desired alignment. Basically, this is the value to
**          skip between tests. Must be a power of 2 (well not really,
**          but the client requires this).
**
**  Results
**
**      Number of the first bit on specified alignment within specified
**      range of clear bits, or -1 if no sufficiently large aligned range of
**      clear bits.
**
******************************************************************/

static int32 FindAlignedClearBitRange (const uint32 *array, uint32 firstBit, uint32 lastBit, uint32 numBits, uint32 alignment)
{
    int32 teststart = firstBit;
    int32 testend   = firstBit + numBits - 1;

    while (testend <= lastBit) {
        if (IsBitRangeClear (array, teststart, testend)) return teststart;
        teststart += alignment;
        testend   += alignment;
    }

    return -1;
}


/******************************************************************
**
**  Find a single cleared bit. This if much faster than calling
**  FindClearBitRange() for a single bit.
**
**  Inputs
**
**      array
**
**      firstBit, lastBit
**          Range of bits to search.
**
**  Results
**
**      Number of the clear bit, or -1 if no clear bits in the
**      specified range.
**
******************************************************************/

static int32 FindClearBit (const uint32 *array, uint32 firstBit, uint32 lastBit)
{
    const uint32 firstWord = firstBit >> LOG;
    const uint32 lastWord  = lastBit >> LOG;
    uint32 testWord = firstWord;
    uint32 testBits;
    int32 bitIndex;

    if (firstWord == lastWord)
    {
        testBits = ~array[testWord] & (((uint32)2 << (lastBit - firstBit)) - 1) << (31 - (firstBit & MASK) - (lastBit - firstBit));
    }
    else
    {
        firstBit &= MASK;
        if (testBits = ~array[testWord] & (((uint32)2 << (31 - firstBit)) - 1)) goto done;

        while (++testWord < lastWord)
        {
            if (testBits = ~array[testWord]) goto done;
        }

        lastBit &= MASK;
        testBits = ~array[testWord] & ~(((uint32)1 << (31 - lastBit)) - 1);
    }

done:
    return ((bitIndex = FindMSB (testBits)) >= 0)
        ? (testWord << LOG) + (31-bitIndex)             /* MSB of word is lowest bit number in bitarray word */
        : -1;
}
