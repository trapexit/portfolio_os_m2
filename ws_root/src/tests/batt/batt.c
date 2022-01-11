/* @(#) batt.c 96/02/28 1.2 */

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/time.h>
#include <misc/batt.h>
#include <stdio.h>
#include <string.h>


void main(void)
{
Err           result;
GregorianDate gd;
Item          timerIO;
BattMemInfo   bmi;
uint32        i;
uint8         readBuffer[15];
uint8         writeBuffer[15];

    timerIO = CreateTimerIOReq();

    result = OpenBattFolio();
    if (result >= 0)
    {
        result = ReadBattClock(&gd);
        if (result >= 0)
        {
            printf("Read 1: %d/%02d/%02d %02d:%02d:%02d\n", gd.gd_Year,
                                                            gd.gd_Month,
                                                            gd.gd_Day,
                                                            gd.gd_Hour,
                                                            gd.gd_Minute,
                                                            gd.gd_Second);
        }
        else
        {
            printf("Unable to read (1) batt time: ");
            PrintfSysErr(result);
        }

        gd.gd_Year    = 1999;
        gd.gd_Month   = 12;
        gd.gd_Day     = 31;
        gd.gd_Hour    = 23;
        gd.gd_Minute  = 59;
        gd.gd_Second  = 59;

        result = WriteBattClock(&gd);
        if (result < 0)
        {
            printf("Unable to write batt time: ");
            PrintfSysErr(result);
        }

        result = ReadBattClock(&gd);
        if (result >= 0)
        {
            printf("Read 2: %d/%02d/%02d %02d:%02d:%02d\n", gd.gd_Year,
                                                            gd.gd_Month,
                                                            gd.gd_Day,
                                                            gd.gd_Hour,
                                                            gd.gd_Minute,
                                                            gd.gd_Second);
        }
        else
        {
            printf("Unable to read (2) batt time: ");
            PrintfSysErr(result);
        }

        WaitTime(timerIO, 2, 0);

        result = ReadBattClock(&gd);
        if (result >= 0)
        {
            printf("Read 3: %d/%d/%d %02d:%02d:%02d\n", gd.gd_Year,
                                                        gd.gd_Month,
                                                        gd.gd_Day,
                                                        gd.gd_Hour,
                                                        gd.gd_Minute,
                                                        gd.gd_Second);
        }
        else
        {
            printf("Unable to read (3) batt time: ");
            PrintfSysErr(result);
        }

        GetBattMemInfo(&bmi, sizeof(bmi));
        if (bmi.bminfo_NumBytes != 15)
        {
            printf("GetBattMemInfo() is lying! It says there are %d bytes of memory!\n",bmi.bminfo_NumBytes);
        }

        LockBattMem();

        memset(readBuffer,  0x67, 15);
        memset(writeBuffer, 0, 15);
        WriteBattMem(writeBuffer, 15, 0);
        ReadBattMem (readBuffer,  15, 0);
        for (i = 0; i < 15; i++)
        {
            if (writeBuffer[i] != readBuffer[i])
            {
                printf("Memory test 1, failed on byte %d\n",i);
            }
        }

        memset(readBuffer,  0x67, 15);
        memset(writeBuffer, 0xff, 15);
        WriteBattMem(writeBuffer, 15, 0);
        ReadBattMem (readBuffer,  15, 0);
        for (i = 0; i < 15; i++)
        {
            if (writeBuffer[i] != readBuffer[i])
            {
                printf("Memory test 1, failed on byte %d\n",i);
            }
        }

        memset(readBuffer,  0x67, 15);
        for (i = 0; i < 15; i++)
            writeBuffer[i] = i*4;
        WriteBattMem(writeBuffer, 15, 0);
        ReadBattMem (readBuffer,  15, 0);
        for (i = 0; i < 15; i++)
        {
            if (writeBuffer[i] != readBuffer[i])
            {
                printf("Memory test 1, failed on byte %d\n",i);
            }
        }

        UnlockBattMem();

        CloseBattFolio();
    }
    else
    {
        printf("Unable to open batt folio: ");
        PrintfSysErr(result);
    }

    DeleteTimerIOReq(timerIO);
}
