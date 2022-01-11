#ifndef __KERNEL_BITARRAY_H
#define __KERNEL_BITARRAY_H


/******************************************************************************
**
**  @(#) bitarray.h 95/09/04 1.9
**
**  Routines to manage arrays of bits.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************/


/* bits in one uint32 */
#define BITSPERUINT32 32

/* is a single bit set within a bit array */
#define IsBitSet(array,bit)   (((array)[(bit)/BITSPERUINT32] & ((uint32)0x80000000 >> ((bit) % BITSPERUINT32))) ? TRUE : FALSE)

/* is a single bit clear within a bit array */
#define IsBitClear(array,bit) (((array)[(bit)/BITSPERUINT32] & ((uint32)0x80000000 >> ((bit) % BITSPERUINT32))) ? FALSE : TRUE)


/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


void SetBitRange(uint32 *array, uint32 firstBit, uint32 lastBit);
void ClearBitRange(uint32 *array, uint32 firstBit, uint32 lastBit);
bool IsBitRangeClear(const uint32 *array, uint32 firstBit, uint32 lastBit);
bool IsBitRangeSet(const uint32 *array, uint32 firstBit, uint32 lastBit);
int32 FindSetBitRange(const uint32 *array, uint32 firstBit, uint32 lastBit, uint32 numBits);
int32 FindClearBitRange(const uint32 *array, uint32 firstBit, uint32 lastBit, uint32 numBits);

/* useful operations on single words */
int32 FindMSB(uint32 mask);
int32 FindLSB(uint32 mask);
uint32 CountBits(uint32 mask);

/* atomically change bits in a single word */
uint32 AtomicSetBits(uint32 *addr, uint32 bits);
uint32 AtomicClearBits(uint32 *addr, uint32 bits);

/* dump a range of bits to the debugging terminal */
void DumpBitRange(const uint32 *array, uint32 firstBit, uint32 lastBit,
                  const char *banner);


#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#ifdef __DCC__
#pragma no_side_effects IsBitRangeSet, IsBitRangeClear
#pragma no_side_effects FindSetBitRange, FindClearBitRange
#pragma no_side_effects DumpBitRange
#pragma no_side_effects AtomicSetBits(1), AtomicClearBits(1)
#pragma pure_function FindMSB, FindLSB, CountBits
#endif


/*****************************************************************************/


#endif /* __KERNEL_BITARRAY_H */
