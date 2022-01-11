/* @(#) intl.c 96/06/07 1.11 */

/**
|||	AUTODOC -class Shell_Commands -name Intl
|||	Gets or sets the international settings of the machine.
|||
|||	  Format
|||
|||	    Intl [-code <USEnglish  |
|||	                 German     |
|||	                 Japanese   |
|||	                 Spanish    |
|||	                 Italian    |
|||	                 Chinese    |
|||	                 Korean     |
|||	                 French     |
|||	                 UKEnglish  |
|||	                 AusEnglish |
|||	                 MexSpanish |
|||	                 CanEnglish> ]
|||
|||	  Description
|||
|||	    This command lets you display the current country and language
|||	    as reported by the international folio. The information
|||	    displayed corresponds to the country codes and language codes
|||	    listed in the <international/intl.h> include file.
|||
|||	    By using the -code option, you can adjust the machine's
|||	    international setting. You supply the name of the international
|||	    setting you want. The system's language and country codes will
|||	    be adjusted accordingly.
|||
|||	  Arguments
|||
|||	    -code <setting>
|||	        Changes the machine's international setting.
|||
|||	  Implementation
|||
|||	    Command implemented in V24.
|||
|||	  Location
|||
|||	    System.m2/Programs/Intl
**/

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <kernel/sysinfo.h>
#include <international/intl.h>
#include <misc/batt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


/*****************************************************************************/


#define Error(x,err) {printf(x); PrintfSysErr(err);}


/*****************************************************************************/


typedef struct
{
    char   *ic_Name;
    uint32  ic_Value;
} IntlCodes;

static const IntlCodes codes[] =
{
    {"USEnglish",  SYSINFO_INTLLANG_USENGLISH },
    {"German",     SYSINFO_INTLLANG_GERMAN },
    {"Japanese",   SYSINFO_INTLLANG_JAPANESE },
    {"Spanish",    SYSINFO_INTLLANG_SPANISH },
    {"Italian",    SYSINFO_INTLLANG_ITALIAN },
    {"Chinese",    SYSINFO_INTLLANG_CHINESE },
    {"Korean",     SYSINFO_INTLLANG_KOREAN },
    {"French",     SYSINFO_INTLLANG_FRENCH },
    {"UKEnglish",  SYSINFO_INTLLANG_UKENGLISH },
    {"AusEnglish", SYSINFO_INTLLANG_AUSENGLISH },
    {"MexSpanish", SYSINFO_INTLLANG_MEXSPANISH },
    {"CanEnglish", SYSINFO_INTLLANG_CANENGLISH },
    {NULL}
};


/*****************************************************************************/


int main(int argc, char **argv)
{
Err      result;
int      parm;
Item     locItem;
Locale  *loc;
char    *code;
uint8    byte;
uint32   i;

    code = NULL;

    for (parm = 1; parm < argc; parm++)
    {
        if ((strcasecmp("-help",argv[parm]) == 0)
         || (strcasecmp("-?",argv[parm]) == 0)
         || (strcasecmp("help",argv[parm]) == 0)
         || (strcasecmp("?",argv[parm]) == 0))
        {
            printf("intl: get/set the international settings of the machine\n");
            printf("  -code <setting>\n");
            return (0);
        }

        if (strcasecmp("-code",argv[parm]) == 0)
        {
            parm++;
            code = argv[parm];
        }
    }

    if (code == NULL)
    {
        result = intlOpenFolio();
        if (result >= 0)
        {
            locItem = intlOpenLocale(NULL);
            if (locItem >= 0)
            {
                loc = (Locale *)LookupItem(locItem);

                printf("The international folio reports: Language '%.2s', Country %d\n",&loc->loc_Language,loc->loc_Country);
                intlCloseLocale(locItem);
            }
            else
            {
                Error("Unable to open Locale item: ",locItem);
            }
            intlCloseFolio();
        }
        else
        {
            Error("Unable to open the international folio: ",result);
        }
    }
    else
    {
        i = 0;
        while (codes[i].ic_Name)
        {
            if (strcasecmp(code, codes[i].ic_Name) == 0)
            {
                result = OpenBattFolio();
                if (result >= 0)
                {
                    LockBattMem();
                    ReadBattMem(&byte, 1, BATTMEM_LANG_OFFSET);
                    byte = (0xf0 & byte) | (codes[i].ic_Value + 1);
                    WriteBattMem(&byte, 1, BATTMEM_LANG_OFFSET);
                    UnlockBattMem();
                    CloseBattFolio();
                }
                else
                {
                    Error("Unable to open the Batt folio: ", result);
                }

                return result;
            }
            i++;
        }

        printf("'%s' is not a valid international code\n", code);
    }

    return 0;
}
