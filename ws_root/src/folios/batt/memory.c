/* @(#) memory.c 96/11/20 1.4 */

#include <kernel/types.h>
#include <kernel/operror.h>
#include <kernel/mem.h>
#include <kernel/semaphore.h>
#include <kernel/super.h>
#include <misc/batt.h>
#include <string.h>
#include "hw.h"


/*****************************************************************************/


void GetBattMemInfo(BattMemInfo *info, uint32 infoSize)
{
BattMemInfo bmi;

    bmi.bminfo_NumBytes = numMemoryBytes;

    if (infoSize > sizeof(BattMemInfo))
    {
        memset(info, 0, sizeof(infoSize));
        infoSize = sizeof(BattMemInfo);
    }

    memcpy(info, &bmi, infoSize);
}


/*****************************************************************************/


void SuperLockBattMem(void)
{
    SuperLockSemaphore(battSem, SEM_WAIT);
}


/*****************************************************************************/


void SuperUnlockBattMem(void)
{
    SuperUnlockSemaphore(battSem);
}


/*****************************************************************************/


Err SuperReadBattMem(void *buffer, uint32 numBytes, uint32 offset)
{
uint8  *data;
uint32  i;
uint8   tmp;
Err     result;

    if (!IsMemWritable(buffer, numBytes))
    {
        if (!IsPriv(CURRENTTASK))
            return BATT_ERR_BADPTR;
    }

    if ((offset >= numMemoryBytes) || (offset + numBytes > numMemoryBytes))
        return BADSIZE; /* yeah, I know this is a kernel error... */

    SuperLockBattMem();

    result = LockHardware();
    if (result >= 0)
    {
        data = (uint8 *)buffer;
        for (i = 0; i < numBytes; i++)
        {
            SetMode(MODE_1);
            tmp = ReadReg(i + offset);

            SetMode(MODE_2);
            data[i] = (tmp << 4) | ReadReg(i + offset);
        }

        UnlockHardware();
    }

    SuperUnlockBattMem();

    return result;
}


/*****************************************************************************/


Err SuperWriteBattMem(const void *buffer, uint32 numBytes, uint32 offset)
{
uint8  *data;
uint32  i;
Err     result;

    if (!IsPriv(CURRENTTASK))
        return BATT_ERR_BADPRIV;

    if (!IsMemReadable(buffer, numBytes))
        return BATT_ERR_BADPTR;

    if ((offset >= numMemoryBytes) || (offset + numBytes > numMemoryBytes))
        return BADSIZE; /* yeah, I know this is a kernel error... */

    SuperLockBattMem();

    result = LockHardware();
    if (result >= 0)
    {
        data = (uint8 *)buffer;
        for (i = 0; i < numBytes; i++)
        {
            SetMode(MODE_1);
            WriteReg(i + offset, data[i] >> 4);

            SetMode(MODE_2);
            WriteReg(i + offset, data[i] & 0x0f);
        }

        UnlockHardware();
    }

    SuperUnlockBattMem();

    return result;
}
