/* @(#) formatdate.c 96/02/28 1.10 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <international/langdrivers.h>
#include <misc/date.h>
#include "locales.h"
#include "utils.h"
#include "international_folio.h"


/*****************************************************************************/


#define DMYToDays(day,month,year) (((year)-1+((month)+9)/12)*365 \
                                   +((year)-1+((month)+9)/12)/4 \
                                   -((year)-1+((month)+9)/12)/100 \
                                   +((year)-1+((month)+9)/12)/400 \
                                   +((((month)+9)%12)*306+5)/10 \
                                   +(day) - 1)


/*****************************************************************************/


#define STR_SIZE 100

uint32 GetDateStr(Locale *loc, DateComponents dc, unichar *result)
{
uint32 len;

    (*loc->loc_GetDateStr)(dc,result,STR_SIZE);
    len = 0;
    while (result[len])
        len++;

    return (len);
}


/*****************************************************************************/


/* put a character at the end of the UniCode result */
#define PutCh(ch) {if (index < maxIndex) result[index++] = (unichar)ch; else tooLong = TRUE;}


/****************************************************************************/


static int32 internalFormatDate(Locale *loc, DateSpec spec,
                                const GregorianDate *date,
                                unichar *result, uint32 resultSize)
{
unichar        ch;
unichar        str[STR_SIZE];
unichar       *ptr;
uint32         num;
uint32         index;
uint32         maxIndex;
bool           tooLong;
uint32         width;
uint32         limit;
uint32         len;
unichar        pad;
bool           leftJustify;
unichar       *start;
uint32         days;

    tooLong  = FALSE;
    index    = 0;
    maxIndex = (resultSize / sizeof(unichar)) - 1;
    days     = DMYToDays(date->gd_Day, date->gd_Month, date->gd_Year);

    /* process the formatting string */

    while ((ch = *spec++) != '\0')
    {
        if (ch != '%')
        {
            PutCh(ch);
        }
        else if (*spec == '%')
        {
            PutCh('%');
            spec++;
        }
        else
        {
            start       = spec;
            width       = 0;
            limit       = 0xffffffff;
            leftJustify = FALSE;
            pad         = ' ';

            /* parse flags... */
            while (TRUE)
            {
                switch (*spec)
                {
                    case '-': leftJustify = TRUE;
                              pad = ' ';
                              spec++;
                              continue;

                    case '0': if (!leftJustify)
                                  pad = '0';      /* left justify prevents 0 padding */
                              spec++;
                              continue;
                }
                break;
            }

            /* gather width argument */
            while ((*spec >= '0') && (*spec <= '9'))
            {
                width = width*10 + (*spec - (unichar)'0');
                spec++;
            }

            if (width >= (sizeof(str) / sizeof(unichar)))
                width = (sizeof(str) / sizeof(unichar)) - 1;

            /* do we have a limit? */
            if (*spec == '.')
            {
                spec++;
                limit = 0;
                while ((*spec >= '0') && (*spec <= '9'))
                {
                    limit = limit*10 + (*spec - (unichar)'0');
                    spec++;
                }
            }

            if (limit >= (sizeof(str) / sizeof(unichar)))
                limit = (sizeof(str) / sizeof(unichar)) - 1;

            switch (*spec++)
            {
                case 'D': len = ConvUnsigned((uint32)date->gd_Day,str);
                          break;

                case 'H': len = ConvUnsigned((uint32)date->gd_Hour,str);
                          break;

                case 'h': if ((num = (uint32)date->gd_Hour % 12) == 0)
                              num = 12;
                          len = ConvUnsigned(num,str);
                          break;

                case 'M': len = ConvUnsigned((uint32)date->gd_Minute,str);
                          break;

                case 'O': len = ConvUnsigned(date->gd_Month,str);
                          break;

                case 'N': len = GetDateStr(loc,(DateComponents)(MONTH_1 + date->gd_Month - 1),str);
                          break;

                case 'n': len = GetDateStr(loc,(DateComponents)(AB_MONTH_1 + date->gd_Month - 1),str);
                          break;

                case 'P': if (date->gd_Hour >= 12)
                              len = GetDateStr(loc,PM,str);
                          else
                              len = GetDateStr(loc,AM,str);
                          break;

                case 'S': len = ConvUnsigned((uint32)date->gd_Second,str);
                          break;

                case 'W': len = GetDateStr(loc,(DateComponents)(DAY_1 + (days+3) % 7),str);
                          break;

                case 'w': len = GetDateStr(loc,(DateComponents)(AB_DAY_1 + (days+3) % 7),str);
                          break;

                case 'Y': len = ConvUnsigned(date->gd_Year,str);
                          break;

                default : len = 0;
                          start--;
                          while (start < spec)
                          {
                              PutCh(*start++);
                              len++;
                          }

                          str[0] = 0;
                          break;
            }

            ptr = str;

            if (limit < len)
            {
                ptr = &str[len - limit];
                len = limit;
            }

            if (!leftJustify)
            {
                while (len < width)
                {
                    PutCh(pad);
                    len++;
                }
            }

            while (*ptr)
                PutCh(*ptr++);

            if (leftJustify)
            {
                while (len < width)
                {
                    PutCh(pad);
                    len++;
                }
            }
        }
    }

    /* null terminate the buffer... */
    result[index] = 0;

    if (tooLong)
        return (INTL_ERR_BUFFERTOOSMALL);

    return ((int32)index);
}


/****************************************************************************/


int32 intlFormatDate(Item locItem, DateSpec spec,
                     const GregorianDate *date,
                     unichar *result, uint32 resultSize)
{
Locale  *loc;
Err      err;

    TRACE(("INTLFORMATDATE: entering\n"));

#ifdef BUILD_PARANOIA
    /* the size is actually longer, but checking it would cause a fault anyway.... */
    if (!IsMemReadable(spec,2))
        return (INTL_ERR_BADDATESPEC);

    if (!IsMemReadable(date,sizeof(GregorianDate)))
        return (INTL_ERR_BADGREGORIANDATE);

    if (!IsMemWritable(result,resultSize))
        return (INTL_ERR_BADRESULTBUFFER);

    if (resultSize < sizeof(unichar))
        return (INTL_ERR_BUFFERTOOSMALL);
#endif

    /* find ourselves... */
    loc = (Locale *)CheckItem(locItem,(uint8)NST_INTL,INTL_LOCALE_NODE);
    if (!loc)
        return (INTL_ERR_BADITEM);

    err = OpenDateFolio();
    if (err >= 0)
    {
        err = ValidateDate(date);
        if (err >= 0)
        {
            err = internalFormatDate(loc, spec, date, result, resultSize);
        }
        CloseDateFolio();
    }

    return err;
}
