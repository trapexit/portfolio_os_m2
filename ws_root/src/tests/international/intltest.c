/* @(#) intltest.c 95/10/03 1.5 */

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <stdio.h>
#include "intltest.h"
#include "formatdate.h"
#include "formatnumber.h"
#include "transliteratestring.h"
#include "comparestrings.h"
#include "convertstring.h"


/*****************************************************************************/


bool verbose;


/*****************************************************************************/


void UniCodeToASCII(unichar *uni, char *asc)
{
    while (*uni)
    {
        *asc++ = (char)*uni++;
    }

    *asc = 0;
}


/*****************************************************************************/


void ASCIIToUniCode(char *asc, unichar *uni)
{
    while (*asc)
    {
        *uni++ = (unichar)*asc++;
    }

    *uni = 0;
}


/*****************************************************************************/


bool CompareUniCodeASCII(unichar *uni, char *asc)
{
    while (*uni == *asc)
    {
        if (!*uni)
            return (TRUE);

        uni++;
        asc++;
    }

    return FALSE;
}


/*****************************************************************************/


void PrintUniCode(unichar *uni)
{
    while (*uni)
    {
        printf("%c",*uni);
        uni++;
    }
}


/*****************************************************************************/


int main(void)
{
Item    locItem;
Err     err;
Locale *loc;

    verbose = FALSE;

    err = intlOpenFolio();
    if (err >= 0)
    {
        locItem = intlOpenLocale(NULL);
        if (locItem >= 0)
        {
            loc = intlLookupLocale(locItem);
            if (loc->loc_Language == INTL_LANG_ENGLISH)
            {
                if (loc->loc_Country == INTL_CNTRY_UNITED_STATES)
                {
                    TestFormatDate(locItem);
                    TestFormatNumber(locItem);
                    TestTransliterateString(locItem);
                    TestCompareStrings(locItem);
                    TestConvertString(locItem);

                    printf("INTLTEST: test suite complete\n");
                }
                else
                {
                    printf("INTLTEST: This test requires that the machine be set to the United States\n");
                }
            }
            else
            {
                printf("INTLTEST: This test requires that the machine be set to English\n");
            }
            intlCloseLocale(locItem);
        }
        else
        {
            PrintError(NULL,"open","locale",locItem);
        }

        intlCloseFolio();
    }
    else
    {
        PrintError(NULL,"open","international folio",err);
    }

    return (0);
}
