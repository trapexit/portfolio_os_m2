/* @(#) esa.c 96/08/30 1.4 */

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/internalf.h>
#include <kernel/cache.h>
#include <kernel/bitarray.h>
#include <kernel/spinlock.h>
#include <device/mp.h>
#include <device/te.h>
#include <hardware/PPC.h>
#include <hardware/PPCasm.h>
#include <stdlib.h>
#include <stdio.h>


/*****************************************************************************/


/* Enable ESA instructions for the given address range.
 *
 * The SEBR register indicates which 128K region of RAM can support ESA
 * instructions, and the SER register indicates which 4K pages within this
 * 128K range actually allow ESA instructions.
 *
 * So everything in the system that calls ESA must be within this same 128K
 * bank.
 */
static void EnableESA(void *addr, uint32 size)
{
uint32 startAddr;
uint32 startRegion;
uint32 startBit;
uint32 endAddr;
uint32 endRegion;
uint32 endBit;
uint32 oldRegion;
uint32 bits;

    startAddr   = (uint32)addr;
    startRegion = (startAddr / ESA_REGION_SIZE);
    startBit    = (startAddr % ESA_REGION_SIZE) / ESA_PAGE_SIZE;

    endAddr   = (uint32)addr + size - 1;
    endRegion = (endAddr / ESA_REGION_SIZE);
    endBit    = (endAddr % ESA_REGION_SIZE) / ESA_PAGE_SIZE;

    if (startRegion != endRegion)
    {
        printf("ERROR: can't have ESA region span a 128K boundary\n");
        return;
    }

    oldRegion = _mfsebr() / ESA_REGION_SIZE;
    if (oldRegion && (oldRegion != startRegion))
    {
        printf("ERROR: conflicting ESA regions, old region %d, new region %d\n", oldRegion, startRegion);
        return;
    }

    bits = _mfser();
    SetBitRange(&bits, startBit, endBit);

    _mtser(bits);
    _mtsebr(startRegion * ESA_REGION_SIZE);
}


/*****************************************************************************/


void InitESARegion(void)
{
    EnableESA(IsUser,            40);
    EnableESA(setjmp,            128);
    EnableESA(longjmp,           128);
    EnableESA(FlushDCacheAll,    80);
    EnableESA(InvalidateICache,  80);
    EnableESA(_GetCacheState,    40);
    EnableESA(IsMasterCPU,       20);
    EnableESA(GetTEWritePointer, 20);
    EnableESA(SetTEWritePointer, 20);
    EnableESA(GetTEReadPointer,  20);
    EnableESA(SetTEReadPointer,  20);
    EnableESA(ObtainSpinLock,    120);
    EnableESA(ReleaseSpinLock,   40);
}
