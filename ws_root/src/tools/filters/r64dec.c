/* @(#)r64dec.c 95/02/24 1.1 */

/*
 * r64dec: decode radix-64 to binary
 */

#include <stdio.h>

unsigned char           bin2asc[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned char           asc2bin[256];

#define	MAXTEXT	1024
unsigned char           line[MAXTEXT];

#define	MAXBIN	128
unsigned char           obuf[MAXBIN];

int
main (void)
{
    int                     i, paysize;
    unsigned int            i0, i1, i2, i3;
    unsigned int            o1, o2, o3;
    unsigned char          *ip;
    unsigned char          *op;

    for (i = 0; i < 256; i++)
	asc2bin[i] = 0200;
    for (i = 0; i < 64; i++)
	asc2bin[bin2asc[i]] = i;

    while (ip = gets (line))
    {
	paysize = asc2bin[*ip++];
	op = obuf;
	for (i = 0; i < paysize; i += 3)
	{
	    i0 = asc2bin[*ip++];
	    i1 = asc2bin[*ip++];
	    i2 = asc2bin[*ip++];
	    i3 = asc2bin[*ip++];

	    o1 = (i0 << 2) | (i1 >> 4);
	    o2 = (i1 << 4) | (i2 >> 2);
	    o3 = (i2 << 6) | i3;

	    *op++ = o1;
	    *op++ = o2;
	    *op++ = o3;
	}
	write (1, obuf, paysize);
    }
    return 0;
}
