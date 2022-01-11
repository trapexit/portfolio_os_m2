/******************************************************************************
**
**  @(#) dspn_canvaluebeimmed.c 95/08/04 1.1
**
******************************************************************************/

#include <dspptouch/dspp_instructions.h>


/*****************************************************************
**
**  Test whether value can be expressed as an IMMED operand.
**
**  IMMED operands have only 13 significant bits used to express
**  a 16-bit value. Either the lower 3 bits must be 0, or the the
**  upper 4 bits must all be the same (all ones or all zeros).
**
**  Inputs
**
**      value
**          16-bit unsigned value to test.
**
**  Results
**
**      TRUE if value fits in an IMMED, FALSE otherwise.
**
*****************************************************************/

bool dspnCanValueBeImmediate (uint16 value)
{
    return (value & 0x0007) == 0 ||
           (value & 0xf000) == 0 ||
           (value & 0xf000) == 0xf000;
}
