/* @(#) pt.c 96/05/17 1.6 */

/* Portuguese language driver for the International folio */

#include <kernel/types.h>
#include <kernel/debug.h>
#include <international/langdrivers.h>
#include <international/intl.h>


/*****************************************************************************/


static const char * const dateStrings[]=
{
    "Domingo",
    "Segunda-feira",
    "Terça-feira",
    "Quarta-feira",
    "Quinta-feira",
    "Sexta-feira",
    "Sábado",

    "Dom",
    "Seg",
    "Ter",
    "Qua",
    "Qui",
    "Sex",
    "Sab",

    "Janeiro",
    "Fevereiro",
    "Março",
    "Abril",
    "Maio",
    "Junho",
    "Julho",
    "Agosto",
    "Setembro",
    "Outubro",
    "Novembro",
    "Dezembro",
    "Lunar",		/* !!! incorrect, get real word !!! */

    "Jan",
    "Fev",
    "Mar",
    "Abr",
    "Mai",
    "Jun",
    "Jul",
    "Ago",
    "Set",
    "Out",
    "Nov",
    "Dez",
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
