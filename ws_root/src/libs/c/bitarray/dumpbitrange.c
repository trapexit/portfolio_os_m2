/* @(#) dumpbitrange.c 95/08/29 1.6 */

#include <kernel/types.h>
#include <kernel/bitarray.h>
#include <stdio.h>
#include <string.h>

/**
|||	AUTODOC -public -class Kernel -group BitArrays -name DumpBitRange
|||	Display a range of bits to the debugging terminal.
|||
|||	  Synopsis
|||
|||	    void DumpBitRange(const uint32 *array, uint32 firstBit,
|||	                      uint32 lastBit, const char *banner)
|||
|||	  Desription
|||
|||	    This function sends a range of bits within a bit array to
|||	    the debugging terminal. This is useful during debugging.
|||
|||	  Arguments
|||
|||	    array
|||	        The array of bits.
|||
|||	    firstBit
|||	        Marks the beginning of the range
|||	        within the bit array to dump.
|||
|||	    lastBit
|||	        Marks the end of the range
|||	        within the bit array to dump.
|||
|||	    banner
|||	        Descriptive text dumped before the
|||	        main output, used to identify the
|||	        origin of this call. May be NULL.
|||
|||	  Implementation
|||
|||	    Library call implemented in libc.a V27.
|||
|||	  Associated Files
|||
|||	    <kernel/bitarray.h>, libc.a
|||
|||	  See Also
|||
|||	    SetBitRange(), ClearBitRange(), IsBitRangeSet(), IsBitRangeClear(),
|||	    IsBitSet(), IsBitClear(), FindSetBitRange(), FindClearBitRange()
|||
**/

/* dump a range of bits to the debugging terminal */
void DumpBitRange(const uint32 *array, uint32 firstBit, uint32 lastBit,
                  const char *banner)
{
uint32 bit;
uint32 banLen;
uint32 i;

    banLen = 0;
    if (banner)
    {
        printf("%s: ",banner);
        banLen = strlen(banner) + 2;
    }

    bit = firstBit;
    while (TRUE)
    {
        if (IsBitSet(array,bit))
            printf("1");
        else
            printf("0");

        bit++;
        if (bit > lastBit)
            break;

        if (((bit - firstBit) % 8) == 0)
        {
            if (((bit - firstBit) % 48) == 0)
            {
                printf("\n");
                for (i = 0; i < banLen; i++)
                    printf(" ");
            }
            else
            {
                printf(" ");
            }
        }
    }
    printf("\n");
}
