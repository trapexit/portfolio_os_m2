/* @(#) strtoul.c 95/10/20 1.7 */

#include <kernel/types.h>
#include <string.h>
#include <ctype.h>


#define NUMNEG	  01000    /* ditto + rd_int */

static int _chval(int ch, int radix)
{
    int val;

	val = (isdigit(ch) ? (ch) - '0' :
	       islower(ch) ? (ch) - 'a' + 10 :
	       isupper(ch) ? (ch) - 'A' + 10 :
	       -1);
    return (val < radix ? val : -1);
}

unsigned long int _strtoul(const char *nsptr, char **endptr, int base)
{
    const unsigned char *nptr = (const unsigned char *)nsptr;  /* see scanf */
    int c, ok = 0;

    while ((c = *nptr++)!=0 && isspace(c));
    if (c=='0')
    {	ok = 1;
	c = *nptr++;
	if (c=='x' || c=='X')
	{   if (base==0 || base==16)
	    {	ok = 0;
		base = 16;
		c = *nptr++;
	    }
	}
	else if (base==0) base = 8;
    }
    if (base==0) base = 10;
    {	unsigned long dhigh = 0, dlow = 0;
	int digit;
	while ((digit = _chval(c,base)) >= 0)
	{   ok = 1;
	    dlow = base * dlow + digit;
	    dhigh = base * dhigh + (dlow >> 16);
	    dlow &= 0xffff;
	    c = *nptr++;
	}
	if (endptr) *endptr = ok ? (char *)nptr-1 : (char *)nsptr;
						/* extra result */
#ifdef ERRNO
	return overflowed ? (errno = ERANGE, ULONG_MAX)
			  : (dhigh << 16) | dlow;
#else
	return (dhigh << 16) | dlow;
#endif
    }
}

unsigned long int strtoul(const char *nsptr, char **endptr, int base)
/*
 * I don't think the way negation is treated in this is right, but its closer
 *  than before
 */
{
    const unsigned char *nptr = (const unsigned char *)nsptr;  /* see scanf */
    int flag = 0, c;
#ifdef ERRNO
    int errno_saved = errno;
#endif
    while ((c = *nptr++)!=0 && isspace(c));
    switch (c)
    {
case '-': flag |= NUMNEG;
	  /* drop through */
case '+': break;
default:  nptr--;
	  break;
    }
#ifdef ERRNO
    errno = 0;
#endif
    {	char *endp;
	unsigned long ud = _strtoul((char *)nptr, &endp, base);
	if (endptr) *endptr = endp==(char *)nptr ? (char *)nsptr : endp;
/* ??? The following lines depend on the fact that unsigned->int casts and   */
/* ??? unary '-' cannot cause arithmetic traps.  Recode to avoid this?	     */
#ifdef ERRNO
	if (errno == ERANGE) return ud;
	errno = errno_saved;
#endif
	if (flag & NUMNEG)
#ifdef ERRNO
	  return (ud <= LONG_MAX) ? -(unsigned long)ud : (errno = ERANGE, ULONG_MAX);
#else
	  return  -(unsigned long)ud;
#endif
	else return +(unsigned long)ud;
    }
}
