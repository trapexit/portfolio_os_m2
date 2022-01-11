#ifndef __INTERNATIONAL_LANGDRIVERS_H
#define __INTERNATIONAL_LANGDRIVERS_H


/******************************************************************************
**
**  @(#) langdrivers.h 96/02/20 1.6
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/****************************************************************************/


typedef enum DateComponents
{
    DAY_1,
    DAY_2,
    DAY_3,
    DAY_4,
    DAY_5,
    DAY_6,
    DAY_7,

    AB_DAY_1,
    AB_DAY_2,
    AB_DAY_3,
    AB_DAY_4,
    AB_DAY_5,
    AB_DAY_6,
    AB_DAY_7,

    MONTH_1,
    MONTH_2,
    MONTH_3,
    MONTH_4,
    MONTH_5,
    MONTH_6,
    MONTH_7,
    MONTH_8,
    MONTH_9,
    MONTH_10,
    MONTH_11,
    MONTH_12,
    MONTH_13,

    AB_MONTH_1,
    AB_MONTH_2,
    AB_MONTH_3,
    AB_MONTH_4,
    AB_MONTH_5,
    AB_MONTH_6,
    AB_MONTH_7,
    AB_MONTH_8,
    AB_MONTH_9,
    AB_MONTH_10,
    AB_MONTH_11,
    AB_MONTH_12,
    AB_MONTH_13,

    AM,
    PM
} DateComponents;


typedef int32 (* COMPAREFUNC)(const unichar *, const unichar *);
typedef int32 (* CONVERTFUNC)(const unichar *, unichar *, uint32, uint32);
typedef int32 (* GETATTRSFUNC)(unichar);
typedef bool (* GETDATESTRFUNC)(DateComponents, unichar *, uint32);


typedef struct LanguageDriverInfo
{
    uint32         ldi_StructSize;    /* number of bytes in this structure */

    COMPAREFUNC    ldi_CompareStrings;
    CONVERTFUNC    ldi_ConvertString;
    GETATTRSFUNC   ldi_GetCharAttrs;
    GETDATESTRFUNC ldi_GetDateStr;
} LanguageDriverInfo;


/*****************************************************************************/


#endif /* __INTERNATIONAL_LANGDRIVERS_H */
