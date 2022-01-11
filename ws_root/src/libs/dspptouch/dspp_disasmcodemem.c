/******************************************************************************
**
**  @(#) dspp_disasmcodemem.c 95/08/09 1.1
**
**  Disassemble DSPP code memory
**
**  By: Bill Barton
**
**  Copyright (c) 1994, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950809 WJB  Created.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <dspptouch/dspp_instructions.h>
#include <dspptouch/dspp_touch.h>
#include <kernel/mem.h>

/*
    Disassemble DSPP code memory.

    Inputs
        codeAddr
            DSPI code memory address to disassemble

        codeSize
            Number of words to disassemble

        banner
            Optional description of disassembly. Can be NULL.
*/
void dsphDisassembleCodeMem (uint16 codeAddr, uint16 codeSize, const char *banner)
{
    uint16 *codeImage;

    if (codeImage = AllocMem (codeSize * sizeof (uint16), MEMTYPE_ANY)) {
            /* build local code image */
        {
            uint32 i;

            for (i=0; i<codeSize; i++) {
                codeImage[i] = dsphReadCodeMem (codeAddr + i);
            }
        }

            /* disassemble */
        dspnDisassemble (codeImage, codeAddr, codeSize, banner ? "%s" : NULL, banner);

            /* clean up */
        FreeMem (codeImage, codeSize * sizeof (uint16));
    }
}
