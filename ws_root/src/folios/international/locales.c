/* @(#) locales.c 96/07/23 1.19 */

/* #define TRACING */

#include <kernel/types.h>
#include <kernel/folio.h>
#include <kernel/item.h>
#include <kernel/semaphore.h>
#include <kernel/mem.h>
#include <kernel/sysinfo.h>
#include <kernel/super.h>
#include <kernel/usermodeservices.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/fileio.h>
#include <international/countrydb.h>
#include <misc/batt.h>
#include <string.h>
#include "international_folio.h"
#include "englishdriver.h"
#include "externalcode.h"
#include "locales.h"


/****************************************************************************/


#define CREATELOCALE_TAG_TEMPLATE TAG_ITEM_LAST+1

#define ALLOC        (0xff)
#define INLINE(size) (size)
#define END_DATA     (0)


/* This is a data table to drive the country file parser. This
 * data is also used when freeing a Locale structure, to determine where
 * pointers are located within the structure in order to free the memory
 * pointed by them.
 *
 * INLINE(X) says that there are X 32-bit words of data to be loaded
 * sequentially.
 *
 * ALLOC says that the following byte in the file indicates the number
 * of 32-bit words of memory to allocate. The data to copy into this memory
 * follows the length byte.
 *
 * END_DATA tells the parser to stop...
 */

static const uint8 PackedLocaleData[] =
{
    ALLOC,       /* loc_Dialects                                     */
    INLINE(6),   /* loc_Country                                      */
                 /* loc_GMTOffset                                    */
                 /* loc_MeasuringSystem                              */
                 /* loc_CalendarType                                 */
                 /* loc_DrivingType                                  */

                 /* loc_Numbers.ns_PosGroups                         */
    ALLOC,       /* loc_Numbers.ns_PosGroupSep                       */
    ALLOC,       /* loc_Numbers.ns_PosRadix                          */
    INLINE(1),   /* loc_Numbers.ns_PosFractionalGroups               */
    ALLOC,       /* loc_Numbers.ns_PosFractionalGroupSep             */
    ALLOC,       /* loc_Numbers.ns_PosFormat                         */
    INLINE(3),   /* loc_Numbers.ns_PosMinFractionalDigits            */
                 /* loc_Numbers,ns_PosMaxFractionalDigits            */
                 /* loc_Numbers.ns_NegGroups                         */
    ALLOC,       /* loc_Numbers.ns_NegGroupSep                       */
    ALLOC,       /* loc_Numbers.ns_NegRadix                          */
    INLINE(1),   /* loc_Numbers.ns_NegFractionalGroups               */
    ALLOC,       /* loc_Numbers.ns_NegFractionalGroupSep             */
    ALLOC,       /* loc_Numbers.ns_NegFormat                         */
    INLINE(2),   /* loc_Numbers.ns_NegMinFractionalDigits            */
                 /* loc_Numbers.ns_NegMaxFractionalDigits            */
    ALLOC,       /* loc_Numbers.ns_Zero                              */
    INLINE(1),   /* loc_Numbers.ns_Flags                             */

    INLINE(1),   /* loc_Currency.ns_PosGroups                        */
    ALLOC,       /* loc_Currency.ns_PosGroupSep                      */
    ALLOC,       /* loc_Currency.ns_PosRadix                         */
    INLINE(1),   /* loc_Currency.ns_PosFractionalGroups              */
    ALLOC,       /* loc_Currency.ns_PosFractionalGroupSep            */
    ALLOC,       /* loc_Currency.ns_PosFormat                        */
    INLINE(3),   /* loc_Currency.ns_PosMinFractionalDigits           */
                 /* loc_Currency,ns_PosMaxFractionalDigits           */
                 /* loc_Currency.ns_NegGroups                        */
    ALLOC,       /* loc_Currency.ns_NegGroupSep                      */
    ALLOC,       /* loc_Currency.ns_NegRadix                         */
    INLINE(1),   /* loc_Currency.ns_NegFractionalGroups              */
    ALLOC,       /* loc_Currency.ns_NegFractionalGroupSep            */
    ALLOC,       /* loc_Currency.ns_NegFormat                        */
    INLINE(2),   /* loc_Currency.ns_NegMinFractionalDigits           */
                 /* loc_Currency.ns_NegMaxFractionalDigits           */
    ALLOC,       /* loc_Currency.ns_Zero                             */
    INLINE(1),   /* loc_Currency.ns_Flags                            */

    INLINE(1),   /* loc_SmallCurrency.ns_PosGroups                   */
    ALLOC,       /* loc_SmallCurrency.ns_PosGroupSep                 */
    ALLOC,       /* loc_SmallCurrency.ns_PosRadix                    */
    INLINE(1),   /* loc_SmallCurrency.ns_PosFractionalGroups         */
    ALLOC,       /* loc_SmallCurrency.ns_PosFractionalGroupSep       */
    ALLOC,       /* loc_SmallCurrency.ns_PosFormat                   */
    INLINE(3),   /* loc_SmallCurrency.ns_PosMinFractionalDigits      */
                 /* loc_SmallCurrency,ns_PosMaxFractionalDigits      */
                 /* loc_SmallCurrency.ns_NegGroups                   */
    ALLOC,       /* loc_SmallCurrency.ns_NegGroupSep                 */
    ALLOC,       /* loc_SmallCurrency.ns_NegRadix                    */
    INLINE(1),   /* loc_SmallCurrency.ns_NegFractionalGroups         */
    ALLOC,       /* loc_SmallCurrency.ns_NegFractionalGroupSep       */
    ALLOC,       /* loc_SmallCurrency.ns_NegFormat                   */
    INLINE(2),   /* loc_SmallCurrency.ns_NegMinFractionalDigits      */
                 /* loc_SmallCurrency.ns_NegMaxFractionalDigits      */
    ALLOC,       /* loc_SmallCurrency.ns_Zero                        */
    INLINE(1),   /* loc_SmallCurrency.ns_Flags                       */

    ALLOC,       /* loc_Date                                         */
    ALLOC,       /* loc_ShortDate                                    */
    ALLOC,       /* loc_Time                                         */
    ALLOC,       /* loc_ShortTime                                    */

    END_DATA
};


/*****************************************************************************/


typedef struct
{
    LanguageCodes  umd_Language;
    CountryCodes   umd_Country;
} UserModeData;


/*****************************************************************************/


static Err UnloadLocale(Locale *loc)
{
uint32    i;
void    **ptr;
uint8     type;

    TRACE(("UNLOADLOCALE: entering with loc = $%lx\n",loc));

    UnbindExternalCode(loc);

    i   = 0;
    ptr = (void **)&loc->loc_Dialects;
    while (TRUE)
    {
        type = PackedLocaleData[i];
        if (type == END_DATA)
            break;

        if (type == ALLOC)
        {
            FreeMem(*ptr, TRACKED_SIZE);
            ptr = &ptr[1];
        }
        else /* if (type == INLINE(x)) */
        {
            ptr = &ptr[type];
        }
        i++;
    }

    TRACE(("UNLOADLOCALE: exiting\n"));

    return (0);
}


/*****************************************************************************/


static const TagArg dbSearchTags[] =
{
    FILESEARCH_TAG_SEARCH_FILESYSTEMS, (TagData) DONT_SEARCH_UNBLESSED,
    TAG_END
};

static Item LoadLocale(UserModeData *umd)
{
RawFile     *file;
uint32      *dest;
void        *ptr;
uint8        type;
uint8        allocSize;
uint8        i;
Item         result;
Err          err;
FormHdr      form;
ChunkHdr     chunk;
bool         stop;
CountryEntry ce;
Locale       loc;
char         dbPath[80];

    TRACE(("LOADLOCALE: entering with umd = $%lx\n",umd));

    LockSemaphore(InternationalBase->if_LocaleLock, SEM_WAIT);

    if (InternationalBase->if_DefaultLocale >= 0)
    {
        UnlockSemaphore(InternationalBase->if_LocaleLock);
        return InternationalBase->if_DefaultLocale;
    }

    memset(&loc, 0, sizeof(Locale));
    loc.loc_Language = umd->umd_Language;
    loc.loc_Country  = umd->umd_Country;

    TRACE(("LOADLOCALE: looking for country %d, language $%x\n",loc.loc_Country,loc.loc_Language));

    /* Given a country code, look in our database file for the relevant
     * country information. If we can't find it, revert back to
     * US and try again.
     */

    result = FindFileAndIdentify(dbPath, sizeof(dbPath), "System.m2/International/CountryDatabase", dbSearchTags);
    if (result >= 0)
    {
        result = OpenRawFile(&file, dbPath, FILEOPEN_READ);
        if (result >= 0)
        {
            TRACE(("LOADLOCALE: opened the country database\n"));

            result = INTL_ERR_CANTFINDCOUNTRY;

            if (ReadRawFile(file, &form, sizeof(form)) == sizeof(form))
            {
                if ((form.ID == ID_FORM) && (form.FormType == ID_INTL))
                {
                    TRACE(("LOADLOCALE: found INTL form\n"));

                    stop = FALSE;
                    while (TRUE)
                    {
                        if (ReadRawFile(file, &chunk, sizeof(chunk)) != sizeof(chunk))
                            break;

                        if (chunk.ID == ID_CTRY)
                        {
                            TRACE(("LOADLOCALE: found CTRY chunk\n"));

                            while (!stop)
                            {
                                /* read a country entry */
                                if (ReadRawFile(file, &ce, sizeof(ce)) != sizeof(ce))
                                {
                                    stop = TRUE;
                                    break;
                                }

                                TRACE(("LOADLOCALE: found country entry $%d\n",ce.ce_Country));

                                if (ce.ce_Country == 0)
                                {
                                    /* we've reached the end of the table... */
                                    if (loc.loc_Country != INTL_CNTRY_UNITED_STATES)
                                    {
                                        loc.loc_Country = INTL_CNTRY_UNITED_STATES;
                                        SeekRawFile(file, sizeof(form) + sizeof(chunk), FILESEEK_START);
                                        continue;     /* try again */
                                    }

                                    stop = TRUE;
                                    break;
                                }

                                if (ce.ce_Country == loc.loc_Country)
                                {
                                    TRACE(("LOADLOCALE: found needed country\n"));

                                    if (SeekRawFile(file, ce.ce_SeekOffset, FILESEEK_CURRENT) < 0)
                                    {
                                        stop = TRUE;
                                        break;
                                    }

                                    i    = 0;
                                    dest = (uint32 *)&loc.loc_Dialects;
                                    while (TRUE)
                                    {
                                        type = PackedLocaleData[i];
                                        if (type == END_DATA)
                                        {
                                            result = 0; /* it worked! */
                                            break;
                                        }

                                        if (type == ALLOC)
                                        {
                                            if (ReadRawFile(file, &allocSize, 1) != 1)
                                            {
                                                break;
                                            }

                                            if (allocSize)
                                            {
                                                ptr = AllocMem((uint32)allocSize * 4,MEMTYPE_TRACKSIZE);
                                                if (!ptr)
                                                {
                                                    result = INTL_ERR_NOMEM;
                                                    break;
                                                }

                                                if (ReadRawFile(file, ptr, (uint32)allocSize * 4) < (uint32)allocSize * 4)
                                                {
                                                    break;
                                                }
                                            }
                                            else
                                            {
                                                ptr = NULL;
                                            }

                                            *dest++ = (uint32)ptr;
                                        }
                                        else /* if (type == INLINE(x)) */
                                        {
                                            if (ReadRawFile(file, dest, (uint32)type * 4) != (uint32)type * 4)
                                            {
                                                break;
                                            }

                                            dest = &dest[type];
                                        }

                                        i++;
                                    }

                                    TRACE(("LOADLOCALE: done loading country info\n"));
                                    stop = TRUE;
                                }
                            }
                        }

                        if (stop)
                            break;

                        if (SeekRawFile(file,IFF_ROUND(chunk.Size),FILESEEK_CURRENT) < 0)
                            break;
                    }
                }
            }
            CloseRawFile(file);
        }
    }

    TRACE(("LOADLOCALE: after country info stuff, result $%x\n",result));

    if (result >= 0)
    {
        /* Given a language code, try to load in the appropriate language driver.
         * If we can't find it, revert back to English.
         */

        TRACE(("LOADLOCALE: loading language driver\n"));

        result = BindExternalCode(&loc);
        if (result < 0)
        {
            TRACE(("LOADLOCALE: no language driver, result = %d\n",result));

            /* We couldn't load the needed driver. Try to load the driver for the
             * machine's default language.
             */

            loc.loc_Language = INTL_LANG_ENGLISH;
            result = BindExternalCode(&loc);
        }

        if (result >= 0)
        {
            TRACE(("LOADLOCALE: creating Locale item\n"));

            result = CreateItemVA(MKNODEID(NST_INTL, INTL_LOCALE_NODE),
                                  TAG_ITEM_NAME,             "Default Locale",
                                  TAG_ITEM_VERSION,          InternationalBase->iff.fn.n_Version,
                                  TAG_ITEM_REVISION,         InternationalBase->iff.fn.n_Revision,
                                  CREATELOCALE_TAG_TEMPLATE, &loc,
                                  TAG_END);
        }
    }

    if (result >= 0)
    {
        err = SetItemOwner(result, InternationalBase->iff.fn.n_Owner);
        if (err < 0)
            result = err;
    }
    else
    {
        UnloadLocale(&loc);
    }

    TRACE(("LOADLOCALE: exiting with $%x\n",result));

    return result;
}


/****************************************************************************/


static int32 TagCallBack(Locale *loc, void *p, uint32 tag, uint32 arg)
{
Locale *template;

    TOUCH(p);

    switch (tag)
    {
        case CREATELOCALE_TAG_TEMPLATE: template      = (Locale *)arg;
                                        template->loc = loc->loc;
                                        *loc          = *template;
                                        break;

        default                       : return INTL_ERR_BADTAG;
    }

    return 0;
}


/*****************************************************************************/


Item CreateLocaleItem(Locale *loc, const TagArg *args)
{
Item result;

    TRACE(("CREATELOCALEITEM: entering with loc = $%lx, args = %lx\n",loc,args));

    if (!IsPriv(CURRENTTASK))
        return INTL_ERR_BADPRIV;

    result = TagProcessor(loc, args, TagCallBack, 0);
    if (result >= 0)
    {
    	InternationalBase->if_DefaultLocale = loc->loc.n_Item;
    	result                              = InternationalBase->if_DefaultLocale;
    }

    TRACE(("CREATELOCALEITEM: exiting with $%x\n",result));

    return result;
}


/****************************************************************************/


Err DeleteLocaleItem(Locale *loc)
{
uint8 oldPriv;

    TRACE(("DELETELOCALEITEM: entering with loc = $%lx\n",loc));

    SuperLockSemaphore(InternationalBase->if_LocaleLock, SEM_WAIT);

    InternationalBase->if_DefaultLocale = INTL_ERR_ITEMNOTFOUND;

    oldPriv = PromotePriv(CURRENTTASK);
    CallAsItemServer(UnloadLocale, loc, TRUE);
    DemotePriv(CURRENTTASK, oldPriv);

    SuperUnlockSemaphore(InternationalBase->if_LocaleLock);

    TRACE(("DELETELOCALEITEM: exiting with 0\n"));

    return 0;
}


/****************************************************************************/


Err CloseLocaleItem(Locale *loc)
{
    TRACE(("CLOSELOCALEITEM: entering with $%x\n",loc));

    if (loc->loc.n_OpenCount == 0)
        SuperInternalDeleteItem(loc->loc.n_Item);

    return 0;
}


/****************************************************************************/


Item FindLocaleItem(const TagArg *args)
{
    TRACE(("FINDLOCALEITEM: entering\n"));

    if (args)
        return (INTL_ERR_BADTAG);

    TRACE(("FINDLOCALEITEM: exiting with defaultLocaleItem = %lx\n",InternationalBase->if_DefaultLocale));

    return InternationalBase->if_DefaultLocale;
}


/*****************************************************************************/


Item LoadLocaleItem(const TagArg *tags)
{
UserModeData  umd;
LanguageCodes lang;
CountryCodes  country;
IntlLangInfo  langCountry;
uint8         oldPriv;
Item          result;
uint8         byte;

    TRACE(("LOADLOCALEITEM: entering with tags $%x\n",tags));

    if (tags)
        return INTL_ERR_BADTAG;

    if (InternationalBase->if_DefaultLocale < 0)
    {
        langCountry = SYSINFO_INTLLANG_USENGLISH;
        SuperQuerySysInfo(SYSINFO_TAG_INTLLANG, (void *)&langCountry, sizeof(IntlLangInfo));

	oldPriv = PromotePriv(CURRENTTASK);
        if (OpenBattFolio() >= 0)
        {
            if (ReadBattMem(&byte, 1, BATTMEM_LANG_OFFSET) >= 0)
            {
                if (GetBattLangCode(byte))
                    langCountry = GetBattLangCode(byte) - 1;
            }
            CloseBattFolio();
        }

        switch (langCountry)
        {
            case SYSINFO_INTLLANG_GERMAN     : lang    = INTL_LANG_GERMAN;
                                               country = INTL_CNTRY_GERMANY;
                                               break;

            case SYSINFO_INTLLANG_JAPANESE   : lang    = INTL_LANG_JAPANESE;
                                               country = INTL_CNTRY_JAPAN;
                                               break;

            case SYSINFO_INTLLANG_SPANISH    : lang    = INTL_LANG_SPANISH;
                                               country = INTL_CNTRY_SPAIN;
                                               break;

            case SYSINFO_INTLLANG_ITALIAN    : lang    = INTL_LANG_ITALIAN;
                                               country = INTL_CNTRY_ITALY;
                                               break;

            case SYSINFO_INTLLANG_CHINESE    : lang    = INTL_LANG_CHINESE;
                                               country = INTL_CNTRY_CHINA;
                                               break;

            case SYSINFO_INTLLANG_KOREAN     : lang    = INTL_LANG_KOREAN;
                                               country = INTL_CNTRY_KOREA_SOUTH;
                                               break;

            case SYSINFO_INTLLANG_FRENCH     : lang    = INTL_LANG_FRENCH;
                                               country = INTL_CNTRY_FRANCE;
                                               break;

            case SYSINFO_INTLLANG_UKENGLISH  : lang    = INTL_LANG_ENGLISH;
                                               country = INTL_CNTRY_UNITED_KINGDOM;
                                               break;

            case SYSINFO_INTLLANG_AUSENGLISH : lang    = INTL_LANG_ENGLISH;
                                               country = INTL_CNTRY_AUSTRALIA;
                                               break;

            case SYSINFO_INTLLANG_MEXSPANISH : lang    = INTL_LANG_SPANISH;
                                               country = INTL_CNTRY_MEXICO;
                                               break;

            case SYSINFO_INTLLANG_CANENGLISH : lang    = INTL_LANG_ENGLISH;
                                               country = INTL_CNTRY_CANADA;
                                               break;

            default                          : lang    = INTL_LANG_ENGLISH;
                                               country = INTL_CNTRY_UNITED_STATES;
                                               break;
        }

        umd.umd_Language = lang;
        umd.umd_Country  = country;

        TRACE(("LOADLOCALEITEM: calling LoadLocale with language $%x, country $%x\n", lang, country));

        result = CallAsItemServer(LoadLocale, &umd, TRUE);
        DemotePriv(CURRENTTASK,oldPriv);

        TRACE(("LOADLOCALEITEM: LoadLocale returned $%x\n",result));

        return result;
    }

    return InternationalBase->if_DefaultLocale;
}


/*****************************************************************************/


Item intlOpenLocale(const TagArg *tags)
{
    return FindAndOpenItem(MKNODEID(NST_INTL, INTL_LOCALE_NODE), tags);
}
