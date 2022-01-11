/* @(#) it.c 96/05/17 1.6 */

/* Italian language driver for the International folio */

#include <kernel/types.h>
#include <kernel/debug.h>
#include <international/langdrivers.h>
#include <international/intl.h>


/*****************************************************************************/


static const char * const dateStrings[]=
{
    "Domenica",
    "Lunedì",
    "Martedì",
    "Mercoledì",
    "Giovedì",
    "Venerdì",
    "Sabato",

    "Dom",
    "Lun",
    "Mar",
    "Mer",
    "Gio",
    "Ven",
    "Sab",

    "Gennaio",
    "Febbraio",
    "Marzo",
    "Aprile",
    "Maggio",
    "Giugno",
    "Luglio",
    "Agosto",
    "Settembre",
    "Ottobre",
    "Novembre",
    "Dicembre",
    "Lunario",		/* !!! incorrect, get real word !!! */

    "Gen",
    "Feb",
    "Mar",
    "Apr",
    "Mag",
    "Giu",
    "Lug",
    "Ago",
    "Set",
    "Ott",
    "Nov",
    "Dic",
    "Lun",		/* !!! incorrect, get real word !!! */

    "AM",
    "PM"
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
