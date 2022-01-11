/******************************************************************************
**
**  @(#) ta_immed.c 95/08/04 1.1
**
**  Test DSPP IMMED operand packing.
**
**  By: Bill Barton
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950804 WJB  Created.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <dspptouch/dspp_instructions.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[])
{
    int i;

    if (argc < 2) {
        printf ("ta_immed <val>...\n");
        return 0;
    }

    for (i=1; i<argc; i++) {
        const int32 val = strtol (argv[i], NULL, 0);

        printf ("%d 0x%x: ", val, (uint16)val);
        if (dspnCanValueBeImmediate (val)) {
            uint16 codetest[] = {
                DSPN_OPCODE_MOVEREG | 0,
                DSPN_OPERAND_IMMED,
            };
            codetest[1] |= dspnPackImmediate (val);

            dspnDisassemble (codetest, 0, sizeof codetest / sizeof codetest[0], NULL);
        }
        else {
            printf ("out of range\n");
        }
    }

    return 0;
}
