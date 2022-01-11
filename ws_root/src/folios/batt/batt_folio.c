/* @(#) batt_folio.c 96/07/31 1.6 */

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/sysinfo.h>
#include <kernel/semaphore.h>
#include <kernel/super.h>
#include <kernel/debug.h>
#include <loader/loader3do.h>
#include <misc/batt.h>


/****************************************************************************/


static int32 CreateFolio(Folio *folio);


extern void SuperWriteBattMem(void);
extern void SuperReadBattMem(void);
extern void SuperWriteBattClock(void);
extern void SuperReadBattClock(void);
extern void SuperLockBattMem(void);
extern void SuperUnlockBattMem(void);

static void *(*sysCalls[])() =
{
    (void *(*)())SuperWriteBattMem,    /* 0 */
    (void *(*)())SuperReadBattMem,     /* 1 */
    (void *(*)())SuperWriteBattClock,  /* 2 */
    (void *(*)())SuperReadBattClock,   /* 3 */
    (void *(*)())SuperLockBattMem,     /* 4 */
    (void *(*)())SuperUnlockBattMem,   /* 5 */
};
#define	NUM_SYSCALLS (sizeof(sysCalls) / sizeof(sysCalls[0]))


/****************************************************************************/


uint32          numMemoryBytes = 0;
volatile uint8 *splitterAddr   = NULL;
Item            battSem        = -1;
static Item     folioItem      = -1;


/*****************************************************************************/


static int32 __CreateModule(void)
{
Err result;

    battSem = result = CreateSemaphore(NULL, 0);
    if (battSem >= 0)
    {
        folioItem = result = CreateItemVA(MKNODEID(KERNELNODE,FOLIONODE),
                                          TAG_ITEM_NAME,         "batt",
                                          CREATEFOLIO_TAG_ITEM,  NST_BATT,
                                          CREATEFOLIO_TAG_SWIS,  sysCalls,
                                          CREATEFOLIO_TAG_NSWIS, NUM_SYSCALLS,
                                          CREATEFOLIO_TAG_INIT,  CreateFolio,
                                          TAG_END);
        if (folioItem >= 0)
            return folioItem;

        DeleteSemaphore(battSem);
    }

    return result;
}


/****************************************************************************/


static int32 __DeleteModule(void)
{
    DeleteSemaphore(battSem);
    return DeleteItem(folioItem);
}


/****************************************************************************/


int main(int32 op)
{
    switch (op)
    {
	case DEMANDLOAD_MAIN_CREATE: return __CreateModule();
        case DEMANDLOAD_MAIN_DELETE: return __DeleteModule();
	default                    : return 0;
    }
}


/****************************************************************************/


static int32 CreateFolio(Folio *folio)
{
SysBatt sb;

    TOUCH(folio);

    SuperQuerySysInfo(SYSINFO_TAG_BATT, &sb, sizeof(sb));

    numMemoryBytes = sb.sb_NumBytes;
    splitterAddr   = sb.sb_Addr;

    if (splitterAddr == NULL)
        return BATT_ERR_NOHARDWARE;

    return folio->fn.n_Item;
}
