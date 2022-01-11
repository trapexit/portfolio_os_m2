/* @(#) mp.c 96/09/11 1.14 */

#include <kernel/types.h>
#include <kernel/cache.h>
#include <kernel/mem.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <kernel/spinlock.h>
#include <hardware/PPCasm.h>
#include <device/mp.h>
#include <string.h>
#include <stdio.h>


/*****************************************************************************/


#define DATA_SIZE  4096*1200
#define STACK_SIZE 4096
#define SPIN_COUNT 200000

static uint8           *data;
static uint32          *stack;
static SpinLock        *sl;
static volatile uint32 *count;


/*****************************************************************************/


void TestSpinLock(void)
{
uint32 i;

    for (i = 0; i < SPIN_COUNT; i++)
    {
        while (ObtainSpinLock(sl) == FALSE)
        {
        }

        (*count)++;
        FlushDCache(0, count, sizeof(uint32));
        ReleaseSpinLock(sl);
    }
}



/*****************************************************************************/


static Err SlaveFunction(void *data)
{
    TestSpinLock();

    memset(data, 0x7f, DATA_SIZE);
    memset(data, 0x6f, DATA_SIZE);
    FlushDCache(0, data, DATA_SIZE);

    printf("Slave is exiting with %f, life is %s\n", 123.0, "good");

    return 123;

}


/*****************************************************************************/


void main(void)
{
Item       ioreq;
Err        result;
Err        funcResult;
CacheInfo  ci;

    GetCacheInfo(&ci, sizeof(ci));
    stack = AllocMemAligned(ALLOC_ROUND(STACK_SIZE, ci.cinfo_DCacheLineSize),
                            MEMTYPE_FILL,
                            ci.cinfo_DCacheLineSize);

    data = AllocMemAligned(ALLOC_ROUND(DATA_SIZE, ci.cinfo_DCacheLineSize),
                           MEMTYPE_FILL,
                           ci.cinfo_DCacheLineSize);

    count = AllocMemAligned(ALLOC_ROUND(sizeof(uint32), ci.cinfo_DCacheLineSize),
                            MEMTYPE_FILL,
                            ci.cinfo_DCacheLineSize);

    if (count && data && stack)
    {
        result = CreateSpinLock(&sl);
        if (result >= 0)
        {
            ioreq = CreateMPIOReq(FALSE);
            if (ioreq >= 0)
            {
                FlushDCacheAll(0);

                result = DispatchMPFunc(ioreq, SlaveFunction, data, &stack[STACK_SIZE / 4], STACK_SIZE, &funcResult);
                if (result >= 0)
                {
                    TestSpinLock();

                    result = WaitIO(ioreq);
                    if (result >= 0)
                    {
                        printf("SlaveFunction() returned %d\n", funcResult);
                        printf("Global count variable holds %u, should be %u\n", *count, SPIN_COUNT*2);
                    }
                    else
                    {
                        printf("WaitIO() failed: ");
                        PrintfSysErr(result);
                    }
                }
                else
                {
                    printf("DispatchMPFunc() failed: ");
                    PrintfSysErr(result);
                }

                DeleteMPIOReq(ioreq);
            }
            else
            {
                printf("CreateMPIOReq() failed: ");
                PrintfSysErr(ioreq);
            }
            DeleteSpinLock(sl);
        }
        else
        {
            printf("CreateSpinLock() failed: ");
            PrintfSysErr(result);
        }
    }
    else
    {
        PrintfSysErr(NOMEM);
    }

    FreeMem(stack, ALLOC_ROUND(STACK_SIZE, ci.cinfo_DCacheLineSize));
    FreeMem(data,  ALLOC_ROUND(DATA_SIZE, ci.cinfo_DCacheLineSize));
    FreeMem(count, ALLOC_ROUND(sizeof(uint32), ci.cinfo_DCacheLineSize));
}
