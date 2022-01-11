/* @(#) clock.c 96/09/05 1.2 */

/**
|||	AUTODOC -class Shell_Commands -name Clock
|||	Gets/sets the date and time from the battery-backed clock.
|||
|||	  Format
|||
|||	     clock [-date <dd-mmm-yyyy>]
|||	           [-time <hh:mm:ss>]
|||
|||	  Description
|||
|||	    This command lets you set or get the time and date of the
|||	    system's battery-backed clock.
|||
|||	    If you run the command with no argument, it just prints out the
|||	    current clock settings. Specifying either the -date or -time
|||	    argument will actually change the values in the clock accordingly
|||	    and will display the new time.
|||
|||	  Arguments
|||
|||	    -date <dd-mmm-yyyy>
|||	        Lets you specify a new date for the clock. The day of the month
|||	        is specified in numerals, the month is specified as a three
|||	        letter string, and the year is a four character numeral. For
|||	        example: "29-Feb-1996"
|||
|||	    -time <hh:mm:ss>
|||	        Lets you specify a new time for the clock. You specify the
|||	        hours, minutes, and seconds. For example: "12:34:56".
|||
|||	  Location
|||
|||	    System.m2/Programs/Clock
|||
**/

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/device.h>
#include <kernel/time.h>
#include <kernel/operror.h>
#include <misc/batt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*****************************************************************************/


static const char *months[] =
{
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec",
    NULL
};



/*****************************************************************************/


static void PrintUsage(void)
{
    printf("clock - get/set the time and date of the battery-backed clock\n");
    printf("  -date <dd-mmm-yyyy> - new date (eg 12-Jan-1996)\n");
    printf("  -time <hh:mm:ss>    - new time (eg 14:34:32)\n");
}


/*****************************************************************************/


int main(int argc, char **argv)
{
uint32         day, month, year;
char           monthName[10];
uint32         hours, minutes, seconds;
Err            result;
GregorianDate  gd;
char          *dateStr;
char          *timeStr;
int32          parm;

    dateStr = NULL;
    timeStr = NULL;
    month   = 0;

    for (parm = 1; parm < argc; parm++)
    {
        if ((strcasecmp("-help",argv[parm]) == 0)
         || (strcasecmp("-?",argv[parm]) == 0)
         || (strcasecmp("help",argv[parm]) == 0)
         || (strcasecmp("?",argv[parm]) == 0))
        {
            PrintUsage();
            return (0);
        }

        if (strcasecmp(argv[parm],"-date") == 0)
        {
            parm++;
            if (parm == argc)
            {
                printf("No date string given for '-date' option\n");
                return 1;
            }
            dateStr = argv[parm];
        }
        else if (strcasecmp(argv[parm],"-time") == 0)
        {
            parm++;
            if (parm == argc)
            {
                printf("No time string given for '-time' option\n");
                return 1;
            }
            timeStr = argv[parm];
        }
        else
        {
            printf("'%s' is an invalid option\n", argv[parm]);
            return 1;
        }
    }

    result = OpenBattFolio();
    if (result >= 0)
    {
        if (timeStr)
        {
            if (sscanf(timeStr, "%u:%u:%u", &hours, &minutes, &seconds) != 3)
            {
                printf("'%s' is not a valid time string\n", timeStr);
                return 1;
            }
        }

        if (dateStr)
        {
            if (sscanf(dateStr, "%u-%3[a-zA-Z]-%u", &day, monthName, &year) != 3)
            {
                printf("'%s' is not a valid date string\n", dateStr);
                return 1;
            }
            monthName[3] = 0;

            month = 0;
            while (months[month])
            {
                if (strcasecmp(monthName, months[month]) == 0)
                    break;

                month++;
            }

            if (!months[month])
            {
                printf("'%s' is not a valid month name\n", monthName);
                return 1;
            }
            month++;
        }

        result = ReadBattClock(&gd);
        if (result >= 0)
        {
            if (dateStr || timeStr)
            {
                if (dateStr)
                {
                    gd.gd_Year    = year;
                    gd.gd_Month   = month;
                    gd.gd_Day     = day;
                }

                if (timeStr)
                {
                    gd.gd_Hour   = hours;
                    gd.gd_Minute = minutes;
                    gd.gd_Second = seconds;
                }

                printf("Setting clock to %02d-%s-%u %02u:%02u:%02u\n", gd.gd_Day,
                                                                   months[gd.gd_Month - 1],
                                                                   gd.gd_Year,
                                                                   gd.gd_Hour,
                                                                   gd.gd_Minute,
                                                                   gd.gd_Second);

                result = WriteBattClock(&gd);
                if (result < 0)
                {
                    printf("WriteBattClock() failed: ");
                    PrintfSysErr(result);
                    return 1;
                }
            }

            result = ReadBattClock(&gd);
            if (result >= 0)
            {
                printf("Clock set to %02d-%s-%u %02u:%02u:%02u\n", gd.gd_Day,
                                                                   months[gd.gd_Month - 1],
                                                                   gd.gd_Year,
                                                                   gd.gd_Hour,
                                                                   gd.gd_Minute,
                                                                   gd.gd_Second);
            }
            else
            {
                printf("ReadBattClock() failed: ");
                PrintfSysErr(result);
            }
        }
        else
        {
            printf("ReadBattClock() failed: ");
            PrintfSysErr(result);
        }

        CloseBattFolio();
    }

    return 0;
}
