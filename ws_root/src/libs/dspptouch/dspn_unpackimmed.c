/******************************************************************************
**
**  @(#) dspn_unpackimmed.c 95/08/04 1.2
**
******************************************************************************/

#include <dspptouch/dspp_instructions.h>


/*****************************************************************
**
**  Unpack value IMMED operand (13-bit + justify).
**
**  Inputs
**
**      immedop
**          IMMED operand word, or just value packed by
**          dspnPackImmediate()
**
**  Results
**
**      16-bit unsigned value stored in IMMED operand.
**
*****************************************************************/

uint16 dspnUnpackImmediate (uint16 immedop)
{
    uint16 value = immedop & DSPN_IMMED_MASK;

    if (immedop & DSPN_IMMED_F_JUSTIFY) {   /* left justify */
        value <<= 3;
    }
    else {                                  /* right justify */
        if (value & 0x1000) value |= 0xe000;
    }

    return value;
}
