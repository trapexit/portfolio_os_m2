/* @(#) nl.c 96/05/17 1.6 */

/* Dutch language driver for the International folio */

#include <kernel/types.h>
#include <kernel/debug.h>
#include <international/langdrivers.h>
#include <international/intl.h>


/*****************************************************************************/


static const char * const dateStrings[]=
{
    "zondag",
    "maandag",
    "dinsdag",
    "woensdag",
    "donderdag",
    "vrijdag",
    "zaterdag",

    "zo",
    "ma",
    "di",
    "wo",
    "do",
    "vr",
    "za",

    "januari",
    "februari",
    "maart",
    "april",
    "mei",
    "juni",
    "juli",
    "augustus",
    "september",
    "oktober",
    "november",
    "december",
    "lunar",		/* !!! incorrect, get real word !!! */

    "jan",
    "feb",
    "maa",
    "apr",
    "mei",
    "jun",
    "jul",
    "aug",
    "sep",
    "oct",
    "nov",
    "dec",
    "lun",		/* !!! incorrect, get real word !!! */

    "VM",
    "NM"
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
