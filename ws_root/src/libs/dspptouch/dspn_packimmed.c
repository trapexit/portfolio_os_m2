/******************************************************************************
**
**  @(#) dspn_packimmed.c 95/08/07 1.3
**
******************************************************************************/

#include <dspptouch/dspp_instructions.h>


/*****************************************************************
**
**  Pack value into IMMED operand (13-bit + justify).
**
**  Inputs
**
**      value
**          16-bit unsigned value to pack into IMMED operand.
**
**  Results
**
**      16-bit value complete IMMED operand.
**
**  Caveats
**
**      @@@ Doesn't do any kind of bounds checking of value. Test
**          value with dspnCanValueBeImmediate() prior to packing.
**
*****************************************************************/

uint16 dspnPackImmediate (uint16 value)
{
    return DSPN_OPERAND_IMMED | ((value & 0x7)
        ? value & 0x1fff
        : DSPN_IMMED_F_JUSTIFY | value >> 3) ;
}
