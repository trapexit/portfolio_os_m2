/* @(#) conversions.c 96/09/24 1.5 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <misc/date.h>


/*****************************************************************************/


/* convert parms to number of days... */
#define DMYToDays(day,month,year) (((year)-1+((month)+9)/12)*365 \
                                   +((year)-1+((month)+9)/12)/4 \
                                   -((year)-1+((month)+9)/12)/100 \
                                   +((year)-1+((month)+9)/12)/400 \
                                   +((((month)+9)%12)*306+5)/10 \
                                   +(day) - 1)


/*****************************************************************************/


static void DaysToDMY(uint32 days, GregorianDate *date)
{
uint32 year;
uint32 month;
uint32 day;
uint32 x;

    /* find year */
    x     = days;
    x    -= (days + 1) / 146097;     /* subtract quadcentury leap days */
    x    += x / 36524;               /* add century leap days          */
    x    -= (x + 1) / 1461;          /* subtract all leap days         */
    year  = x / 365;

    /* find day of year */
    x = days - (year * 365) - (year / 4) + (year / 100) - (year / 400);

    /* find month */
    month  = x / 153;
    month *= 5;
    month += 10 * (x % 153) / 305;

    /* find day of month */
    day  = 1 + x;
    day -= (int32)((month * 306 + 5) / 10);

    /* final adjustments... */
    month += 2;
    year  += month / 12;
    month %= 12;
    month++;

    date->gd_Day   = (uint8)day;
    date->gd_Month = (uint8)month;
    date->gd_Year  = (uint32)year;
}


/****************************************************************************/


Err ConvertTimeValToGregorian(const TimeVal *tv, GregorianDate *gd)
{
uint32 seconds;
uint32 days;

    /* The TimeVal specifies a time value relative to 01-Jan-93, while
     * DMYToDays() is relative to some other base date which comes earlier.
     * To get the number of days in the format we want, we add the number of
     * actual days specified by the TimeVal to the number of days from the
     * base date to 01-Jan-93. We then proceed to convert this day count
     * into a Gregorian date, and get it formatted.
     */

    days = (tv->tv_Seconds / (60*60*24)) + (DMYToDays(1,1,1993));

    /* now convert the number of days into a gregorian date */
    DaysToDMY(days, gd);

    seconds        = tv->tv_Seconds % (24*60*60);
    gd->gd_Hour    = seconds / (60*60);
    gd->gd_Minute  = (seconds / 60) % 60;
    gd->gd_Second  = seconds % 60;

    return 0;
}


/*****************************************************************************/


Err ConvertGregorianToTimeVal(const GregorianDate *gd, TimeVal *tv)
{
int32 days;

    days = ValidateDate(gd);
    if (days < 0)
        return days;

    tv->tv_Seconds      = ((uint32)days*24*60*60) + (gd->gd_Hour*60*60) + (gd->gd_Minute*60) + gd->gd_Second;
    tv->tv_Microseconds = 0;

    return 0;
}


/*****************************************************************************/


Err ValidateDate(const GregorianDate *gd)
{
GregorianDate temp;
uint32        days;

    if ((gd->gd_Hour    >= 24)
     || (gd->gd_Minute  >= 60)
     || (gd->gd_Second  >= 60)
     || (gd->gd_Day     >= 32)
     || (gd->gd_Year    <  1993))
    {
        return DATE_ERR_BADPARAM;
    }

    /* convert the date into a number of days... */
    days = DMYToDays(gd->gd_Day, gd->gd_Month, gd->gd_Year);

    /* now convert the number of days back into a date */
    DaysToDMY(days, &temp);

    /* if the original and the retranslated dates don't agree, we've got a
     * bogus original date...
     */
    if ((gd->gd_Day   != temp.gd_Day)
     || (gd->gd_Month != temp.gd_Month)
     || (gd->gd_Year  != temp.gd_Year))
    {
        return DATE_ERR_BADPARAM;
    }

    return days - DMYToDays(1,1,1993);
}
