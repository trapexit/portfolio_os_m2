/*
 *	@(#) rommisc.c 95/09/05 1.7
 *
 * Miscellaneous functions used by the ROM dipir.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "insysrom.h"

/*****************************************************************************
 strncpyz
 Like strncpy, but guarantees destination is null-terminated.
*/
	void
strncpyz(char *to, char *from, uint32 tolen, uint32 fromlen)
{
	for (;;)
	{
		if (tolen == 0)
		{
			to--;
			break;
		}
		if (fromlen == 0)
			break;
		if (*from == '\0')
			break;
		*to++ = *from++;
		tolen--;
		fromlen--;
	}
	*to = '\0';
}

/*****************************************************************************
 strcmp
*/
	int
strcmp(char *s1, char *s2)
{
	int r;
	int c1, c2;

	for (;;)
	{
		c1 = *s1++;
		c2 = *s2++;
		r = c1 - c2;
		if (r != 0)
			return r;
		if (c1 == '\0')
			return 0;
	}
}

/*****************************************************************************
 strncmp
*/
	int
strncmp(char *s1, char *s2, int n)
{
	while (n-- > 0)
	{	char c1 = *s1++, c2 = *s2++;
		int d = c1 - c2;
		if (d != 0) return d;
		if (c1 == 0) return 0;     /* no need to check c2 */
	}
	return 0;
}

/*****************************************************************************
*/
	char *
strcat(char *a, const char *b)
{
	char *p;

	p = a;
	while (*a) a++;
	do {
		*a = *b++;
	} while (*a++);
	return p;
}

/*****************************************************************************
*/
	char *
strcpy(char *a, const char *b)
{
	char *p = a;
	while ((*p++ = *b++) != 0);
	return a;
}

/*****************************************************************************
*/
	int
strlen(const char *s)
{
	const char *p;

	p = s;
	while (*p != '\0')
		p++;
	return (p - s);
}

/*****************************************************************************
*/
	int 
memcmp(const void *s1, const void *s2, int n)
{
	char *cs1 = s1;
	char *cs2 = s2;
	while (n-- > 0)
	{
		if (*cs1++ != *cs2++)
			return 1;
	}
	return 0;
}

#if 0
/*****************************************************************************
*/
	void
memset(void *p, int c, int n)
{
	char *cp = p;
	while (n--)
		*cp++ = c;
}

/*****************************************************************************
*/
	void
memcpy(void *dst, void *src, int n)
{
	char *d = dst;
	char *s = src;
	unsigned int i;

	if (d == s) return;
	if (d > s)
	{
		for (i = n; i > 0; i--)
			d[i-1] = s[i-1];
	} else
	{
		for (i = 0; i < n; i++)
			d[i] = s[i];
	}
}
#endif
