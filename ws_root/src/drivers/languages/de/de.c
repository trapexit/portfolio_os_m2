/* @(#) de.c 96/05/17 1.6 */

/* German language driver for the International folio */

#include <kernel/types.h>
#include <kernel/debug.h>
#include <international/langdrivers.h>
#include <international/intl.h>


/*****************************************************************************/


static const char * const dateStrings[]=
{
    "Sonntag",
    "Montag",
    "Dienstag",
    "Mittwoch",
    "Donnerstag",
    "Freitag",
    "Samstag",

    "So",
    "Mo",
    "Di",
    "Mi",
    "Do",
    "Fr",
    "Sa",

    "Januar",
    "Februar",
    "März",
    "April",
    "Mai",
    "Juni",
    "Juli",
    "August",
    "September",
    "Oktober",
    "November",
    "Dezember",
    "Lunar",     /* !!! incorrect, get real word !!! */

    "Jan",
    "Feb",
    "Mär",
    "Apr",
    "Mai",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Okt",
    "Nov",
    "Dez",
    "Lun",	/* !!! incorrect, get real word !!! */

    "vorm.",
    "nachm."
};


/*****************************************************************************/


static bool GetDateStr(DateComponents dc, unichar *result, uint32 resultSize)
{
uint32 i;

    if (dc > PM)
        return (FALSE);

    i = 0;
    while (dateStrings[dc][i] && (i < resultSize - 1))
    {
        result[i] = dateStrings[dc][i];
        i++;
    }
    result[i] = 0;

    return (TRUE);
}


/*****************************************************************************/


static const LanguageDriverInfo driverInfo =
{
    sizeof(LanguageDriverInfo),

    NULL,
    NULL,
    NULL,
    GetDateStr
};


LanguageDriverInfo *main(void)
{
    return (&driverInfo);
}
