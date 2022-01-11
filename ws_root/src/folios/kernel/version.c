/* @(#) version.c 96/07/31 1.14 */

/* Note: The date conversion code in this source is stolen from the date folio */

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/super.h>
#include <loader/header3do.h>
#include <loader/loader3do.h>
#include <misc/date.h>
#include <string.h>
#include <stdio.h>


/*****************************************************************************/


extern char copyright[];
extern int  timezone;


/*****************************************************************************/


/* convert parms to number of days... */
#define DMYToDays(day,month,year) (((year)-1+((month)+9)/12)*365 \
                                   +((year)-1+((month)+9)/12)/4 \
                                   -((year)-1+((month)+9)/12)/100 \
                                   +((year)-1+((month)+9)/12)/400 \
                                   +((((month)+9)%12)*306+5)/10 \
                                   +(day) - 1)


/*****************************************************************************/


/* convert number of days to day/month/year */
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


/*****************************************************************************/


void internalPrint3DOHeader(Item module, char *whatstr, char *copystr)
{
char          *n;
GregorianDate  gd;
int32          days;
int32          secs;
_3DOBinHeader  *p3do;
Module	       *modulePtr;

    if (!module)
    {
        module = CURRENTTASK->t_Module; /* Default to the current task */

        if (!module)
        {
            printf("<3DO HEADER NOT AVAILABLE>\n");
            return;
        }
    }
    modulePtr = (Module *) CheckItem(module, KERNELNODE, MODULENODE);
    if(!modulePtr)
	{
	printf("Module %x not found!\n", module);
	return;
	}

    p3do = modulePtr->li->header3DO;

    if (copystr && ((copystr == copyright) || strcmp(copystr, copyright)))
    {
        /* only print the kernel's copyright message */
        printf("\n%s\n\n", copystr);
    }

    n = p3do->_3DO_Name;
    if (!n || !*n)
        n = "Anonymous";

    secs  = p3do->_3DO_Time - timezone;

    /* The 3DOBinHeader specifies a time value relative to 01-Jan-93, while
     * DMYToDays() is relative to some other base date which comes earlier.
     * To get the number of days in the format we want, we add the number of
     * actual days specified by the TimeVal to the number of days from the
     * base date to 01-Jan-93. We then proceed to convert this day count
     * into a Gregorian date, and get it formatted.
     */

    days = (secs / (60*60*24)) + (DMYToDays(1,1,1993));

    /* now convert the number of days into a gregorian date */
    DaysToDMY(days, &gd);

    secs          = secs % (24*60*60);
    gd.gd_Hour    = secs / (60*60);
    gd.gd_Minute  = (secs / 60) % 60;
    gd.gd_Second  = secs % 60;

    printf("%-26.26s ($%08X $%08X) V%d.%d OS%d.%d %02d/%02d/%02d %02d:%02d",
            n,
           modulePtr->li->codeBase,
           modulePtr->li->dataBase,
           p3do->_3DO_Item.n_Version,
           p3do->_3DO_Item.n_Revision,
           p3do->_3DO_OS_Version,
           p3do->_3DO_OS_Revision,
           gd.gd_Year,
           gd.gd_Month,
           gd.gd_Day,
           gd.gd_Hour,
           gd.gd_Minute);

    printf(" %s\n", whatstr ? whatstr : "");
}
