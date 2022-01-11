/* @(#) bitarray.c 95/07/08 1.8 */
/* $Id: bitarray.c,v 1.1 1994/12/15 18:27:00 vertex Exp $ */

#include <kernel/types.h>
#include <kernel/bitarray.h>
#include <stdio.h>


/*****************************************************************************/


#define BITS    BITSPERUINT32
#define LOG     5           /* 2^LOG == BITS */
#define MASK    (BITS - 1)

#ifdef BUILD_PARANOIA
#define INFO(x) printf x
#else
#define INFO(x)
#endif


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group BitArrays -name SetBitRange
|||	Set a range of bits within a bit array.
|||
|||	  Synopsis
|||
|||	    void SetBitRange(uint32 *array, uint32 firstBit, uint32 lastBit)
|||
|||	  Desription
|||
|||	    This function turns a range of bits on within a bit array. All
|||	    bits within the range [firstBit..lastBit] are set to 1.
|||
|||	    firstBit must be <= to lastBit, otherwise the results are
|||	    undefined.
|||
|||	  Arguments
|||
|||	    array
|||	        The array of bits.
|||
|||	    firstBit
|||	        The first bit within the range to set.
|||
|||	    lastBit
|||	        The last bit within the range to set.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/bitarray.h>, libc.a
|||
|||	  See Also
|||
|||	    ClearBitRange(), IsBitRangeSet(), IsBitRangeClear(), IsBitSet(),
|||	    IsBitClear(), FindSetBitRange(), FindClearBitRange(),
|||	    DumpBitRange()
|||
**/

/* set a range of bits within a bit array */
void SetBitRange(uint32 *array, uint32 firstBit, uint32 lastBit)
{
uint32 firstWord;
uint32 lastWord;
uint32 i;

#ifdef BUILD_PARANOIA
    if (lastBit < firstBit)
    {
        INFO(("WARNING: SetBitRange() called with lastBit < firstBit\n"));
        INFO(("         (firstBit %d, lastBit %d)\n",firstBit,lastBit));
        return;
    }
#endif

    firstWord = (firstBit >> LOG);
    lastWord  = (lastBit >> LOG);

    if (firstWord == lastWord)
    {
        array[firstWord] |= (((uint32)2 << (lastBit - firstBit)) - 1) << (31 - (firstBit & MASK) - (lastBit - firstBit));
    }
    else
    {
        firstBit &= MASK;
        array[firstWord] |= ((uint32)2 << (31 - firstBit)) - 1;

        for (i = firstWord + 1; i < lastWord; i++)
            array[i] = ~0;

        lastBit &= MASK;
        array[lastWord] |= ~(((uint32)1 << (31 - lastBit)) - 1);
    }
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group BitArrays -name ClearBitRange
|||	Clear a range of bits within a bit array.
|||
|||	  Synopsis
|||
|||	    void ClearBitRange(uint32 *array, uint32 firstBit, uint32 lastBit)
|||
|||	  Desription
|||
|||	    This function turns a range of bits off within a bit array. All
|||	    bits within the range [firstBit..lastBit] are set to 0.
|||
|||	    firstBit must be <= to lastBit, otherwise the results are
|||	    undefined.
|||
|||	  Arguments
|||
|||	    array
|||	        The array of bits.
|||
|||	    firstBit
|||	        The first bit within the range to clear.
|||
|||	    lastBit
|||	        The last bit within the range to clear.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/bitarray.h>, libc.a
|||
|||	  See Also
|||
|||	    SetBitRange(), IsBitRangeSet(), IsBitRangeClear(), IsBitSet(),
|||	    IsBitClear(), FindSetBitRange(), FindClearBitRange(),
|||	    DumpBitRange()
|||
**/

/* clear a range of bits within a bit array */
void ClearBitRange(uint32 *array, uint32 firstBit, uint32 lastBit)
{
uint32 firstWord;
uint32 lastWord;
uint32 i;

#ifdef BUILD_PARANOIA
    if (lastBit < firstBit)
    {
        INFO(("WARNING: ClearBitRange() called with lastBit < firstBit\n"));
        INFO(("         (firstBit %d, lastBit %d)\n",firstBit,lastBit));
        return;
    }
#endif

    firstWord = (firstBit >> LOG);
    lastWord  = (lastBit >> LOG);

    if (firstWord == lastWord)
    {
        array[firstWord] &= ~((((uint32)2 << (lastBit - firstBit)) - 1) << (31 - (firstBit & MASK) - (lastBit - firstBit)));
    }
    else
    {
        firstBit &= MASK;
        array[firstWord] &= ~(((uint32)2 << (31 - firstBit)) - 1);

        for (i = firstWord + 1; i < lastWord; i++)
            array[i] = 0;

        lastBit &= MASK;
        array[lastWord] &= ((uint32)1 << (31 - lastBit)) - 1;
    }
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group BitArrays -name IsBitClear
|||	Test whether a bit within a bit array is set to 0.
|||
|||	  Synopsis
|||
|||	    bool IsBitClear(const uint32 *array, uint32 bit)
|||
|||	  Desription
|||
|||	    This function lets you test to see if a single bit within a bit
|||	    array is set to 0.
|||
|||	  Arguments
|||
|||	    array
|||	        The array of bits.
|||
|||	    bit
|||	        The bit number to test.
|||
|||	  Return Value
|||
|||	    This function returns TRUE if the given bit is set to 0, and
|||	    return FALSE if it is set to 1.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/bitarray.h> V27.
|||
|||	  Associated Files
|||
|||	    <kernel/bitarray.h>, libc.a
|||
|||	  See Also
|||
|||	    SetBitRange(), ClearBitRange(), IsBitRangeSet(), IsBitRangeClear(),
|||	    IsBitSet(), FindSetBitRange(), FindClearBitRange(), DumpBitRange()
|||
**/

/**
|||	AUTODOC -public -class Kernel -group BitArrays -name IsBitRangeClear
|||	Test whether all bits within a range of a bit array
|||	                  are all set to 0.
|||
|||	  Synopsis
|||
|||	    bool IsBitRangeClear(const uint32 *array, uint32 firstBit,
|||	                         uint32 lastBit)
|||
|||	  Desription
|||
|||	    This function lets you test to see if all bits within a range of
|||	    bits in a bit array are all set to 0.
|||
|||	    firstBit must be <= to lastBit, otherwise the results are
|||	    undefined.
|||
|||	  Arguments
|||
|||	    array
|||	        The array of bits.
|||
|||	    firstBit
|||	        The first bit within the range to test.
|||
|||	    lastBit
|||	        The last bit within the range to test.
|||
|||	  Return Value
|||
|||	    This function returns TRUE if all bits within the range
|||	    [firstBit..lastBit] are set to 0, and FALSE otherwise.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/bitarray.h>, libc.a
|||
|||	  See Also
|||
|||	    SetBitRange(), ClearBitRange(), IsBitRangeSet(), IsBitSet(),
|||	    IsBitClear(), FindSetBitRange(), FindClearBitRange(),
|||	    DumpBitRange()
|||
**/

/* return TRUE if an entire range of bits is all set to 0 within a bit array */
bool IsBitRangeClear(const uint32 *array, uint32 firstBit, uint32 lastBit)
{
uint32 firstWord;
uint32 lastWord;
uint32 i;

#ifdef BUILD_PARANOIA
    if (lastBit < firstBit)
    {
        INFO(("WARNING: IsBitRangeClear() called with lastBit < firstBit\n"));
        INFO(("         (firstBit %d, lastBit %d)\n",firstBit,lastBit));
        return FALSE;
    }
#endif

    firstWord = (firstBit >> LOG);
    lastWord  = (lastBit >> LOG);

    if (firstWord == lastWord)
    {
        if (array[firstWord] & (((uint32)2 << (lastBit - firstBit)) - 1) << (31 - (firstBit & MASK) - (lastBit - firstBit)))
            return FALSE;
    }
    else
    {
        firstBit &= MASK;
        if (array[firstWord] & (((uint32)2 << (31 - firstBit)) - 1))
            return FALSE;

        for (i = firstWord + 1; i < lastWord; i++)
        {
            if (array[i])
                return FALSE;
        }

        lastBit &= MASK;
        if (array[lastWord] & ~(((uint32)1 << (31 - lastBit)) - 1))
            return FALSE;
    }

    return TRUE;
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group BitArrays -name IsBitSet
|||	Test whether a bit within a bit array is set to 1.
|||
|||	  Synopsis
|||
|||	    bool IsBitSet(const uint32 *array, uint32 bit)
|||
|||	  Desription
|||
|||	    This function lets you test to see if a single bit within a bit
|||	    array is set to 1.
|||
|||	  Arguments
|||
|||	    array
|||	        The array of bits.
|||
|||	    bit
|||	        The bit number to test.
|||
|||	  Return Value
|||
|||	    This function returns TRUE if the given bit is set to 1, and
|||	    return FALSE if it is set to 0.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/bitarray.h> V27.
|||
|||	  Associated Files
|||
|||	    <kernel/bitarray.h>, libc.a
|||
|||	  See Also
|||
|||	    SetBitRange(), ClearBitRange(), IsBitRangeSet(), IsBitRangeClear(),
|||	    IsBitClear(), FindSetBitRange(), FindClearBitRange(),
|||	    DumpBitRange()
|||
**/

/**
|||	AUTODOC -public -class Kernel -group BitArrays -name IsBitRangeSet
|||	Test whether all bits within a range of a bit array
|||	                are all set to 1.
|||
|||	  Synopsis
|||
|||	    bool IsBitRangeSet(const uint32 *array, uint32 firstBit,
|||	                       uint32 lastBit)
|||
|||	  Desription
|||
|||	    This function lets you test to see if all bits within a range of
|||	    bits in a bit array are all set to 1.
|||
|||	    firstBit must be <= to lastBit, otherwise the results are
|||	    undefined.
|||
|||	  Arguments
|||
|||	    array
|||	        The array of bits.
|||
|||	    firstBit
|||	        The first bit within the range to test.
|||
|||	    lastBit
|||	        The last bit within the range to test.
|||
|||	  Return Value
|||
|||	    This function returns TRUE if all bits within the range
|||	    [firstBit..lastBit] are set to 1, and FALSE otherwise.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/bitarray.h>, libc.a
|||
|||	  See Also
|||
|||	    SetBitRange(), ClearBitRange(), IsBitRangeClear(), IsBitSet(),
|||	    IsBitClear(), FindSetBitRange(), FindClearBitRange(),
|||	    DumpBitRange()
|||
**/

/* return TRUE if an entire range of bits is all set to 1 within a bit array */
bool IsBitRangeSet(const uint32 *array, uint32 firstBit, uint32 lastBit)
{
uint32 firstWord;
uint32 lastWord;
uint32 i;

#ifdef BUILD_PARANOIA
    if (lastBit < firstBit)
    {
        INFO(("WARNING: IsBitRangeSet() called with lastBit < firstBit\n"));
        INFO(("         (firstBit %d, lastBit %d)\n",firstBit,lastBit));
        return FALSE;
    }
#endif

    firstWord = (firstBit >> LOG);
    lastWord  = (lastBit >> LOG);

    if (firstWord == lastWord)
    {
        if (~array[firstWord] & (((uint32)2 << (lastBit - firstBit)) - 1) << (31 - (firstBit & MASK) - (lastBit - firstBit)))
            return FALSE;
    }
    else
    {
        firstBit &= MASK;
        if (~array[firstWord] & (((uint32)2 << (31 - firstBit)) - 1))
            return FALSE;

        for (i = firstWord + 1; i < lastWord; i++)
        {
            if ((~array[i]) != 0)
                return FALSE;
        }

        lastBit &= MASK;
        if (~array[lastWord] & ~(((uint32)1 << (31 - lastBit)) - 1))
            return FALSE;
    }

    return TRUE;
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group BitArrays -name FindSetBitRange
|||	Find a range of set bits within a bit array.
|||
|||	  Synopsis
|||
|||	    int32 FindSetBitRange(const uint32 *array, uint32 firstBit,
|||	                          uint32 lastBit, uint32 numBits)
|||
|||	  Desription
|||
|||	    This function searches through a bit array for a range of bits
|||	    that are all set to 1. The search is limited to bits within
|||	    [firstBit..lastBit].
|||
|||	    firstBit must be <= to lastBit, otherwise the results are
|||	    undefined.
|||
|||	  Arguments
|||
|||	    array
|||	        The array of bits.
|||
|||	    firstBit
|||	        Marks the beginning of the search range
|||	        within the bit array.
|||
|||	    lastBit
|||	        Marks the end of the search range
|||	        within the bit array.
|||
|||	    numBits
|||	        The number of bits in a row that must
|||	        be set. If this value is 0, this
|||	        function always returns -1.
|||
|||	  Return Value
|||
|||	    This function returns the number of the first bit within the
|||	    range of set bits that was found, or -1 if there was no
|||	    sufficiently large range of set bits within the bit array.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/bitarray.h>, libc.a
|||
|||	  See Also
|||
|||	    SetBitRange(), ClearBitRange(), IsBitRangeSet(), IsBitRangeClear(),
|||	    IsBitSet(), IsBitClear(), FindClearBitRange(), DumpBitRange()
|||
**/

int32 FindSetBitRange(const uint32 *array, uint32 firstBit, uint32 lastBit,
                      uint32 numBits)
{
uint32 index;

#ifdef BUILD_PARANOIA
    if (lastBit < firstBit)
    {
        INFO(("WARNING: FindSetBitRange() called with lastBit < firstBit\n"));
        INFO(("         (firstBit %d, lastBit %d)\n",firstBit,lastBit));
        return -1;
    }
#endif

    if (numBits == 0)
        return -1;

    /* do a type of Boyer-Moore search through the bit array */
    while (firstBit + numBits - 1 <= lastBit)
    {
        index = firstBit + numBits - 1;

        if (IsBitRangeSet(array, firstBit, index))
            return (int32)firstBit;

        /* find the last bit in the current range which wasn't set, and resume
         * the search with the following bit.
         */
        while (IsBitSet(array, index))
            index--;

        firstBit = index + 1;
    }

    return -1;
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group BitArrays -name FindClearBitRange
|||	Find a range of clear bits within a bit array.
|||
|||	  Synopsis
|||
|||	    int32 FindClearBitRange(const uint32 *array, uint32 firstBit,
|||	                            uint32 lastBit, uint32 numBits)
|||
|||	  Desription
|||
|||	    This function searches through a bit array for a range of bits
|||	    that are all set to 0. The search is limited to bits within
|||	    [firstBit..lastBit].
|||
|||	    firstBit must be <= to lastBit, otherwise the results are
|||	    undefined.
|||
|||	  Arguments
|||
|||	    array
|||	        The array of bits.
|||
|||	    firstBit
|||	        Marks the beginning of the search range
|||	        within the bit array.
|||
|||	    lastBit
|||	        Marks the end of the search range
|||	        within the bit array.
|||
|||	    numBits
|||	        The number of bits in a row that must
|||	        be clear. If this value is 0, this
|||	        function always returns -1.
|||
|||	  Return Value
|||
|||	    This function returns the number of the first bit within the
|||	    range of clear bits that was found, or -1 if there was no
|||	    sufficiently large range of clear bits within the bit array.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/bitarray.h>, libc.a
|||
|||	  See Also
|||
|||	    SetBitRange(), ClearBitRange(), IsBitRangeSet(), IsBitRangeClear(),
|||	    IsBitSet(), IsBitClear(), FindSetBitRange(), DumpBitRange()
|||
**/

int32 FindClearBitRange(const uint32 *array, uint32 firstBit, uint32 lastBit,
                        uint32 numBits)
{
uint32 index;

#ifdef BUILD_PARANOIA
    if (lastBit < firstBit)
    {
        INFO(("WARNING: FindClearBitRange() called with lastBit < firstBit\n"));
        INFO(("         (firstBit %d, lastBit %d)\n",firstBit,lastBit));
        return -1;
    }
#endif

    if (numBits == 0)
        return -1;

    /* do a type of Boyer-Moore search through the bit array */
    while (firstBit + numBits - 1 <= lastBit)
    {
        index = firstBit + numBits - 1;

        if (IsBitRangeClear(array, firstBit, index))
            return (int32)firstBit;

        /* find the last bit in the current range which wasn't clear, and resume
         * the search from the following bit.
         */
        while (IsBitClear(array, index))
            index--;

        firstBit = index + 1;
    }

    return -1;
}
