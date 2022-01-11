/******************************************************************************
**
**  @(#) f16_cvtfloat.c 96/02/07 1.2
**
**  Test float <-> frac16 conversion macros.
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
**  950617 WJB  Created.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <misc/frac16.h>
#include <stdio.h>
#include <stdlib.h>


int main (int argc, char *argv[])
{
    int i;

    for (i=1; i<argc; i++) {
        float fpval = strtof (argv[i], NULL);
        frac16 sf16val = ConvertFP_SF16(fpval);
        ufrac16 uf16val = ConvertFP_UF16(fpval);

        printf ("%f  $%08lx %d.%04ld %f  $%08lx %d.%04ld %f\n",
            fpval,
            sf16val, sf16val / 65536, ((sf16val < 0 ? -sf16val : sf16val) % 65536) * 10000 / 65536, ConvertF16_FP(sf16val),
            uf16val, uf16val / 65536, (uf16val % 65536) * 10000 / 65536, ConvertF16_FP(uf16val));
    }

    return 0;
}
