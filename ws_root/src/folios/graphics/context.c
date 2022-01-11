/* @(#) context.c 96/05/23 1.15 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/mem.h>
#include <kernel/item.h>
#include <kernel/tags.h>
#include <graphics/gfx_pvt.h>

#include "gfx_debug.h"

#include "protos.h"


void AbortIOReqsUsingItem(Item itemNumber, uint32 itemType)
{
    /* FIXME: This needs to be implemented */
	TOUCH(itemNumber);
	TOUCH(itemType);
}
