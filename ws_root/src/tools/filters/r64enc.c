/* @(#)r64enc.c 95/02/24 1.1 */

/*
 * r64enc: encode binary to radix-64
 */

#include <stdio.h>

unsigned char           bin2asc[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int
main (void)
{
    int                     i, sz;
    int                     i1, i2, i3;
    char                    o0, o1, o2, o3;
    unsigned char           ibuf[48];
    unsigned char          *ip;

    while ((sz = read (0, ibuf, sizeof ibuf)) > 0)
    {
	putchar (bin2asc[sz]);
	ip = ibuf;
	for (i = 0; i < sz; i += 3)
	{
	    i1 = *ip++;
	    i2 = *ip++;
	    i3 = *ip++;

	    o0 = i1 >> 2;
	    o1 = (i1 << 4) | (i2 >> 4);
	    o2 = (i2 << 2) | (i3 >> 6);
	    o3 = i3;

	    putchar (bin2asc[o0 & 077]);
	    putchar (bin2asc[o1 & 077]);
	    putchar (bin2asc[o2 & 077]);
	    putchar (bin2asc[o3 & 077]);
	}
	putchar ('\n');
    }
    return 0;
}
