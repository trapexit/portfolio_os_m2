#ifndef __HARDWARE_SPLITTERJR_H
#define __HARDWARE_SPLITTERJR_H


/******************************************************************************
**
**  @(#) splitterjr.h 96/02/27 1.1
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************/


/* SplitterJr registers	*/
#define RTCCS	0
#define	RTCWE	1
#define RTCCLK	2
#define	RTCDI	3
#define	RTCDO	3

/* RTC registers */
#define	RTC_SECS	0
#define	RTC_MINS	2
#define	RTC_HRS		4
#define	RTC_DOW		6
#define	RTC_DAY		7
#define	RTC_MON		9
#define	RTC_YR		11
#define	RTC_CNT1	13
#define	RTC_CNT2	14
#define	RTC_CNT3	15

/* CNT1 masks */
#define	RTC_MASK_24	0x01
#define	RTC_MASK_CNTR	0x02

/* CNT2 masks */
#define	RTC_MASK_BUSY	0x08
#define RTC_MASK_PONC	0x04

/* CNT3 masks */
#define	RTC_MASK_SYSR	0x08


/*****************************************************************************/


#endif /* __HARDWARE_SPLITTERJR_H */
