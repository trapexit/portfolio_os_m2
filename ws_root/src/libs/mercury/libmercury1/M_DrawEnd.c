#include "mercury.h"

void
M_DrawEnd(CloseData *pc)
{
    if (pc->mp) {
	pc->whichlist = 1-pc->whichlist;
    }
}
