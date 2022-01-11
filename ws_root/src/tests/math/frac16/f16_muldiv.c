/******************************************************************************
**
**  @(#) f16_muldiv.c 96/02/07 1.3
**
**  Test frac16 multiply and divide macros.
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
    if (argc < 3) {
        printf ("Usage: %s <float1> <float2>\n", argv[0]);
        return 5;
    }

    {
        const float32 fpa = strtof (argv[1], NULL);
        const float32 fpb = strtof (argv[2], NULL);
        const frac16 sf16a = ConvertFP_SF16(fpa);
        const frac16 sf16b = ConvertFP_SF16(fpb);
        const ufrac16 uf16a = ConvertFP_UF16(fpa);
        const ufrac16 uf16b = ConvertFP_UF16(fpb);

        printf ("  float: %f * %f = %f\n", fpa, fpb, fpa * fpb);
        printf (" frac16: %f * %f = %f\n", ConvertF16_FP(sf16a), ConvertF16_FP(sf16b), ConvertF16_FP(MulSF16(sf16a,sf16b)));
        printf ("ufrac16: %f * %f = %f\n", ConvertF16_FP(uf16a), ConvertF16_FP(uf16b), ConvertF16_FP(MulUF16(uf16a,uf16b)));

        printf ("  float: %f / %f = %f\n", fpa, fpb, fpa / fpb);
        printf (" frac16: %f / %f = %f\n", ConvertF16_FP(sf16a), ConvertF16_FP(sf16b), ConvertF16_FP(DivSF16(sf16a,sf16b)));
        printf ("ufrac16: %f / %f = %f\n", ConvertF16_FP(uf16a), ConvertF16_FP(uf16b), ConvertF16_FP(DivUF16(uf16a,uf16b)));
    }

    return 0;
}
