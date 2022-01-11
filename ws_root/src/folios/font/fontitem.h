/* @(#) fontitem.h 95/09/04 1.2 */

#ifndef __FONTITEM_H
#define __FONTITEM_H


/****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __GRAPHICS_FONT_H
#include <graphics/font.h>
#endif


/****************************************************************************/


Item CreateFontItem(FontDescriptor *font, TagArg *args);
Err DeleteFontItem(FontDescriptor *font);
Err CloseFontItem(FontDescriptor *font);
Item FindFontItem(TagArg *args);
Item LoadFontItem(TagArg *args);


/*****************************************************************************/


#endif /* __FONTITEM_H */
