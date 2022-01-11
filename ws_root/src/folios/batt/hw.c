/* @(#) hw.c 96/02/28 1.1 */

#include <kernel/types.h>
#include <kernel/super.h>
#include <misc/batt.h>
#include "hw.h"


/*****************************************************************************/


static uint32 oldints;


/*****************************************************************************/


static void WriteByte(uint8 reg)
{
uint8 i;

   for (i = 0; i < 8; i++)
   {
        splitterAddr[RTCDI] = (reg & 1);
        SetClock(0);
        SetClock(1);
	reg = (reg >> 1);
    }
}


/*****************************************************************************/


static void BusyWait(void)
{
    /* wait till it ain't busy no more... */
    SetMode(MODE_0);
    while (ReadReg(RTC_CNT2) & RTC_MASK_BUSY)
    {
    }
}


/*****************************************************************************/


Err LockHardware(void)
{
    if (splitterAddr == NULL)
        return BATT_ERR_NOHARDWARE;

    oldints = Disable();

    BusyWait();

    SetMode(MODE_0);
    if (ReadReg(RTC_CNT2) & RTC_MASK_PONC)
    {
        /* reinit the HW when this bit is set */

        SetWriteEnabled(1);
        SetChipSelect(1);
        WriteByte(RTC_CNT3 | (RTC_MASK_SYSR << 4));
        SetChipSelect(0);
        SetChipSelect(1);
        SetClock(0);
        SetClock(1);
        SetChipSelect(0);
    }

    return 0;
}


/*****************************************************************************/


void UnlockHardware(void)
{
    Enable(oldints);
}


/*****************************************************************************/


uint8 ReadReg(uint8 reg)
{
uint8 i;
uint8 byte;

    SetWriteEnabled(0);
    SetChipSelect(1);
    WriteByte(reg);

    /* read the data bits */
    byte = 0;
    for (i = 0; i < 8; i++)
    {
        SetClock(0);
        SetClock(1);
        byte |= ((splitterAddr[RTCDO] & 1) << i);
    }

    SetChipSelect(0);

    return byte >> 4;
}


/*****************************************************************************/


void WriteReg(uint8 reg, uint8 value)
{
    SetWriteEnabled(1);
    SetChipSelect(1);
    WriteByte(reg | (value << 4));
    SetChipSelect(0);
    SetWriteEnabled(0);
}


/*****************************************************************************/


void IncrReg(uint8 reg, uint8 value)
{
    BusyWait();

    SetWriteEnabled(1);
    SetChipSelect(1);

    while (value--)
	WriteByte(reg);

    SetChipSelect(0);
    SetWriteEnabled(0);
}


/*****************************************************************************/


uint8 ReadRegPair(uint8 reg)
{
uint8 tmp;

    tmp = ReadReg(reg + 1) * 10;
    return tmp + ReadReg(reg);
}


/*****************************************************************************/


void WriteRegPair(uint8 reg, uint8 value)
{
    WriteReg(reg + 1, value / 10);
    WriteReg(reg,     value % 10);
}



/*****************************************************************************/


void IncrRegPair(uint8 reg, uint8 value)
{
    IncrReg(reg + 1, value / 10);
    IncrReg(reg,     value % 10);
}
