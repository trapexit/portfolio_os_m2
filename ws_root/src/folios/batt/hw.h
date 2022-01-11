/* @(#) hw.h 96/02/28 1.1 */

#ifndef __HW_H
#define __HW_H


/*****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __HARDWARE_SPLITTERJR_H
#include <hardware/splitterjr.h>
#endif


/*****************************************************************************/


extern volatile uint8 *splitterAddr;
extern uint32          numMemoryBytes;
extern Item            battSem;


/*****************************************************************************/


/* for use with the SetMode() macro */
typedef enum
{
    MODE_0 = 0,
    MODE_1 = 2,
    MODE_2 = 3
} Modes;


/*****************************************************************************/


#define SetClock(value)        splitterAddr[RTCCLK] = ((value) ? 1 : 0)
#define SetWriteEnabled(value) splitterAddr[RTCWE]  = ((value) ? 0 : 1)
#define SetChipSelect(value)   splitterAddr[RTCCS]  = ((value) ? 0 : 1)
#define SetMode(m)             WriteReg(RTC_CNT3, (m))


/*****************************************************************************/


Err   LockHardware(void);
void  UnlockHardware(void);

uint8 ReadReg(uint8 reg);
void  WriteReg(uint8 reg, uint8 value);
void  IncrReg(uint8 reg, uint8 value);

uint8 ReadRegPair(uint8 reg);
void  WriteRegPair(uint8 reg, uint8 value);
void  IncrRegPair(uint8 reg, uint8 value);


/*****************************************************************************/


#endif /* __HW_H */
