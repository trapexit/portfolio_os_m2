/* @(#) clock.c 96/02/28 1.2 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/debug.h>
#include <misc/batt.h>
#include <misc/date.h>
#include "hw.h"


/*****************************************************************************/


Err SuperWriteBattClock(const GregorianDate *gd)
{
Err result;

    if (!IsMemReadable(gd, sizeof(GregorianDate)))
        return BATT_ERR_BADPTR;

    result = OpenDateFolio();
    if (result >= 0)
    {
        result = ValidateDate(gd);
        if (result >= 0)
        {
            result = LockHardware();
            if (result >= 0)
            {
                /* first off, clear the registers */
                WriteReg    (RTC_CNT1, RTC_MASK_CNTR | RTC_MASK_24);
                WriteRegPair(RTC_YR,   0);
                WriteRegPair(RTC_MON,  0);
                WriteRegPair(RTC_DAY,  0);
                WriteRegPair(RTC_HRS,  0);
                WriteRegPair(RTC_MINS, 0);
                WriteRegPair(RTC_SECS, 0);
                WriteReg    (RTC_DOW,  0);
                WriteReg    (RTC_CNT1, RTC_MASK_24);

                /* now set the actual values */
                IncrRegPair(RTC_YR,   (gd->gd_Year - 1900) % 100);
                IncrRegPair(RTC_MON,  gd->gd_Month - 1);
                IncrRegPair(RTC_DAY,  gd->gd_Day - 1);
                IncrReg    (RTC_HRS,  gd->gd_Hour);
                IncrRegPair(RTC_MINS, gd->gd_Minute);
                IncrRegPair(RTC_SECS, gd->gd_Second);

                UnlockHardware();
            }
        }
        CloseDateFolio();
    }

    return result;
}


/*****************************************************************************/


Err SuperReadBattClock(GregorianDate *gd)
{
Err   result;
uint8 tmp;

    if (!IsMemWritable(gd, sizeof(GregorianDate)))
        return BATT_ERR_BADPTR;

    result = LockHardware();
    if (result >= 0)
    {
        tmp = ReadReg(RTC_HRS + 1) & 0x3;
        gd->gd_Hour = tmp * 10 + ReadReg(RTC_HRS);

        gd->gd_Year = ReadRegPair(RTC_YR);
        if (gd->gd_Year < 93)
            gd->gd_Year += 100;

        gd->gd_Year += 1900;

        gd->gd_Month   = ReadRegPair(RTC_MON);
        gd->gd_Day     = ReadRegPair(RTC_DAY);
        gd->gd_Minute  = ReadRegPair(RTC_MINS);
        gd->gd_Second  = ReadRegPair(RTC_SECS);

        UnlockHardware();
    }

    return result;
}

