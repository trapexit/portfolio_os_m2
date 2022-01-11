/* @(#) mul64.c 95/08/29 1.2 */

/* Copyright 1995 Diab Data, Inc, USA
 *
 * History :
 * When	Who	What
 * 950413	fb	Initial
 */

#include <kernel/types.h>


/*****************************************************************************/


uint64 __mul64(uint64 j, uint64 k)
{
uint64 n;

    if (j > k)
    {
         n = j;
        j = k;
        k = n;
    }

    n = 0;
    while (j)
    {
        if (j & 1)
            n += k;

        k <<= 1;
        j >>= 1;
    }

    return n;
}
