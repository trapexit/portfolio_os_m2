/* @(#) strchr.c 95/08/29 1.4 */

char *strchr(const char *s, char ch)
					/* find first instance of ch in s */
{   char c1 = ch;
    for (;;)
    {	char c = *s++;
	if (c == c1) return (char *)s-1;
	if (c == 0) return 0;
    }
}

