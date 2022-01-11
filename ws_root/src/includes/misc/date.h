#ifndef __MISC_DATE_H
#define __MISC_DATE_H


/******************************************************************************
**
**  @(#) date.h 96/04/29 1.4
**
**  Date manipulation utilities
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_TIME_H
#include <kernel/time.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif


/*****************************************************************************/


#define MakeDateErr(svr,class,err) MakeErr(ER_FOLI,ER_DATE,svr,ER_E_SSTM,class,err)

#define DATE_ERR_BADPARAM   MakeDateErr(ER_SEVERE,ER_C_STND,ER_ParamError)


/*****************************************************************************/


/* description of a date */
typedef struct GregorianDate
{
    uint32 gd_Year;         /* 1..0xfffff                          */
    uint16 gd_Month;        /* 1..12                               */
    uint8  gd_Day;          /* 1..28/29/30/31   (depends on month) */
    uint8  gd_Hour;         /* 0..23                               */
    uint8  gd_Minute;       /* 0..59                               */
    uint8  gd_Second;       /* 0..59                               */
} GregorianDate;


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif /* cplusplus */


/* folio management */
Err OpenDateFolio(void);
Err CloseDateFolio(void);

/* date format conversion */
Err ConvertTimeValToGregorian(const TimeVal *tv, GregorianDate *gd);
Err ConvertGregorianToTimeVal(const GregorianDate *gd, TimeVal *tv);

/* validation */
Err ValidateDate(const GregorianDate *gd);


#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __MISC_DATE_H */
