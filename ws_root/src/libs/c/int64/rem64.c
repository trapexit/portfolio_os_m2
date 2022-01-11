/* @(#) rem64.c 96/04/29 1.3 */

/* Copyright 1995 Diab Data, Inc, USA
 *
 * History :
 * When Who What
 * 950413   fb  Initial
 */

#include <kernel/types.h>


/*****************************************************************************/


static uint64 rem64(uint64 j, uint64 k)
{
int64  i, m;
uint32 c;

    if (((k-1) & k) == 0)
    {
        /* Reminder by power of 2. */
        j &= k-1;
    }
    else if (((m = j | k) & 0xffffffff00000000ll) == 0)
    {
        j = (uint32)j % (uint32)k;
    }
    else
    {
        i = 0xff00000000000000ll;
        if ((m & i) == 0)
        {
            i >>= 8;
            if ((m & i) == 0)
            {
                i >>= 8;
                if ((m & i) == 0)
                    i >>= 8;
            }
        }

        c = 0;
        while ((k & i) == 0)
        {
            k <<= 8;
            c += 8;
        }

        if(j & (1 << 63))
        {
            while ((k & (1 << 63)) == 0)
            {
                k <<= 1;
                c++;
            }

            if (j >= k)
                j -= k;
        }
        else
        {
            while (k <= j)
            {
                k <<= 1;
                c++;
            }
        }

        while (c-- > 0)
        {
            k >>= 1;
            if (j >= k)
                j -= k;
        }
    }

    return j;
}


/*****************************************************************************/


int64 __rem64(int64 j, int64 k)
{
bool   negate;

    negate = FALSE;
    if (j < 0)
    {
        j = -j;
        negate = !negate;
    }

    if (k <= 0)
    {
        if (k == 0)
            return negate ? (1 << 63) : ((1 << 63) - 1);

        k = -k;
    }

    if (negate)
        return -(int64)rem64((uint64)j, (uint64)k);
    else
        return rem64(j,k);
}


/*****************************************************************************/


uint64 __urem64(uint64 j, uint64 k)
{
    if (k == 0)
        return ~(uint64)0;

    return rem64(j, k);
}
