/* @(#) externalcode.c 96/01/17 1.11 */

/* #define TRACING */

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <international/langdrivers.h>
#include <file/filefunctions.h>
#include <loader/loader3do.h>
#include <string.h>
#include <stdio.h>
#include "locales.h"
#include "englishdriver.h"
#include "international_folio.h"
#include "externalcode.h"


/*****************************************************************************/


/* This function returns the function pointer stored at the given offset
 * in "driverInfo". If this structure is not large enough to hold this
 * pointer, or if the pointer is NULL, the English version of the function
 * is returned instead
 */

static void *ReturnFunc(LanguageDriverInfo *driverInfo,
                        LanguageDriverInfo *englishDriverInfo,
                        uint32 funcOffset)
{
void **result;

    result = NULL;
    if (driverInfo->ldi_StructSize >= funcOffset + sizeof(void *))
        result = (void **)((uint32)driverInfo + funcOffset);

    if (!result || !*result)
        result = (void **)((uint32)englishDriverInfo + funcOffset);

    return (*result);
}



/*****************************************************************************/


/* Load in an external language driver, or use the built-in English one
 * if appropriate. The function pointers within the Locale structure are
 * initialized to point to functions contained within the language driver.
 * If any function is not supplied by the loaded code, the built-in
 * English version is used instead.
 */

static const TagArg driverSearchTags[] =
{
    FILESEARCH_TAG_SEARCH_FILESYSTEMS,  (TagData) DONT_SEARCH_UNBLESSED,
    TAG_END
};

Err BindExternalCode(Locale *loc)
{
Err                 err;
LanguageDriverInfo *driverInfo;
LanguageDriverInfo *englishDriverInfo;
char                buffer[48];
char                path[80];

    TRACE(("BINDEXTERNALCODE: entering, need language $%x\n",loc->loc_Language));

    englishDriverInfo = EnglishDriverMain();

    if (loc->loc_Language == INTL_LANG_ENGLISH)
    {
        loc->loc_DriverModule = -1;
        driverInfo            = englishDriverInfo;
    }
    else
    {
        sprintf(buffer,"System.m2/International/%c%c.language",
                (loc->loc_Language >> 24) & 0xff,
                (loc->loc_Language >> 16) & 0xff);

        err = FindFileAndIdentify(path, sizeof(path), buffer, driverSearchTags);
        if (err < 0)
        {
            TRACE(("BINDEXTERNALCODE: could not find driver '%s'\n",buffer));
            return err;
        }

        loc->loc_DriverModule = err = OpenModule(path, OPENMODULE_FOR_THREAD, NULL);
        if (err < 0)
        {
            TRACE(("BINDEXTERNALCODE: could not load driver '%s'\n",path));
            return (err);
        }

        driverInfo = (LanguageDriverInfo *)ExecuteModule(loc->loc_DriverModule, 0, NULL);
        if (!driverInfo)
        {
            CloseModule(loc->loc_DriverModule);
            loc->loc_DriverModule = -1;

            return INTL_ERR_NOMEM;
        }

        err = OpenItemAsTask(loc->loc_DriverModule, NULL, InternationalBase->iff.fn.n_Owner);
        CloseModule(loc->loc_DriverModule);

        if (err < 0)
            return err;
    }

    /* STOOPID compiler gives warnings on the following assignments even
     * though there are explicit casts to tell it that I know what I'm
     * doing. Argh.
     */
    loc->loc_CompareStrings = (COMPAREFUNC)ReturnFunc(driverInfo,englishDriverInfo,
                                                      Offset(LanguageDriverInfo *,ldi_CompareStrings));

    loc->loc_ConvertString = (CONVERTFUNC)ReturnFunc(driverInfo,englishDriverInfo,
                                                     Offset(LanguageDriverInfo *,ldi_ConvertString));

    loc->loc_GetCharAttrs = (GETATTRSFUNC)ReturnFunc(driverInfo,englishDriverInfo,
                                                     Offset(LanguageDriverInfo *,ldi_GetCharAttrs));

    loc->loc_GetDateStr = (GETDATESTRFUNC)ReturnFunc(driverInfo,englishDriverInfo,
                                                     Offset(LanguageDriverInfo *,ldi_GetDateStr));

    TRACE(("BINDEXTERNALCODE: returning with 0\n"));

    return 0;
}


/****************************************************************************/


void UnbindExternalCode(Locale *loc)
{
    CloseItemAsTask(loc->loc_DriverModule, InternationalBase->iff.fn.n_Owner);
}


/****************************************************************************/


int32 intlCompareStrings(Item locItem, const unichar *string1, const unichar *string2)
{

Locale *loc;

#ifdef BUILD_PARANOIA
    /* the size can actually be longer, but checking it would cause a fault anyway.... */
    if (!IsMemReadable(string1,2))
        return (INTL_ERR_BADSOURCEBUFFER);

    /* the size can actually be longer, but checking it would cause a fault anyway.... */
    if (!IsMemReadable(string2,2))
        return (INTL_ERR_BADSOURCEBUFFER);
#endif

    /* find ourselves */
    loc = (Locale *)CheckItem(locItem,NST_INTL,INTL_LOCALE_NODE);
    if (!loc)
        return (INTL_ERR_BADITEM);

    return ((*loc->loc_CompareStrings)(string1,string2));
}


/****************************************************************************/


int32 intlConvertString(Item locItem, const unichar *string, unichar *result,
                        uint32 resultSize, uint32 flags)
{
Locale *loc;

#ifdef BUILD_PARANOIA
    /* the size can actually be longer, but checking it would cause a fault anyway.... */
    if (!IsMemReadable(string,2))
        return (INTL_ERR_BADSOURCEBUFFER);

    if (!IsMemWritable(result,resultSize))
        return (INTL_ERR_BADRESULTBUFFER);

    if (resultSize < sizeof(unichar))
        return (INTL_ERR_BUFFERTOOSMALL);

    if (flags & ~(INTL_CONVF_UPPERCASE |
                  INTL_CONVF_LOWERCASE |
                  INTL_CONVF_STRIPDIACRITICALS |
                  INTL_CONVF_HALF_WIDTH |
                  INTL_CONVF_FULL_WIDTH))
        return (INTL_ERR_BADFLAGS);
#endif

    /* find ourselves */
    loc = (Locale *)CheckItem(locItem,NST_INTL,INTL_LOCALE_NODE);
    if (!loc)
        return (INTL_ERR_BADITEM);

    return ((*loc->loc_ConvertString)(string,result,resultSize,flags));
}


/****************************************************************************/


int32 intlGetCharAttrs(Item locItem, unichar character)
{
Locale *loc;

    /* find ourselves */
    loc = (Locale *)CheckItem(locItem,NST_INTL,INTL_LOCALE_NODE);
    if (!loc)
        return (INTL_ERR_BADITEM);

    return ((*loc->loc_GetCharAttrs)(character));
}
