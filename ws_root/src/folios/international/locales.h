/* @(#) locales.h 95/12/01 1.5 */
/* $Id: locales.h,v 1.4 1994/12/08 00:15:53 vertex Exp $ */

#ifndef __LOCALES_H
#define __LOCALES_H


/****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __INTERNATIONAL_INTL_H
#include <international/intl.h>
#endif


/****************************************************************************/


Item CreateLocaleItem(Locale *loc, const TagArg *args);
Err DeleteLocaleItem(Locale *loc);
Err CloseLocaleItem(Locale *loc);
Item FindLocaleItem(const TagArg *args);
Item LoadLocaleItem(const TagArg *args);


/*****************************************************************************/


#endif /* __LOCALES_H */
