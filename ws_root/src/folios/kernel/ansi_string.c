/* @(#) ansi_string.c 96/03/29 1.3 */

#include <string.h>
#include <ctype.h>


/*****************************************************************************/


size_t strlen(const char *s)
{
const char *x;

    x = s;
    while (*x)
        x++;

    return (x - s);
}


/*****************************************************************************/


char *strcpy(char *a, const char *b)
{
int  i;
char ch;

    i = 0;
    do
    {
        ch = a[i] = b[i];
        i++;
    }
    while (ch);

    return a;
}


/*****************************************************************************/


char *strncpy(char *a, const char *b, size_t n)
{
char *p = a;

    while (n-- > 0)
    {
        if ((*p++ = *b++) == 0)
        {   char c = 0;
            while (n-- > 0)
                *p++ = c;
            return a;
        }
    }

    return a;
}


/*****************************************************************************/


char *strcat(char *a, const char *b)
{
char *x;

    x = a;
    while (*a)
        a++;

    do
    {
        *a = *b++;
    }
    while (*a++);

    return x;
}


/*****************************************************************************/


char *strncat(char *a, const char *b, size_t n)
{
char *p = a;

    while (*p != 0)
        p++;

    while (n-- > 0)
    {
	if ((*p++ = *b++) == 0)
            return a;
    }
    *p = 0;

    return a;
}


/*****************************************************************************/


int strcmp(const char *a,const char *b)
{
    for (;;)
    {	char c1 = *a++, c2 = *b++;
	int d = c1 - c2;
	if (d != 0) return d;
	if (c1 == 0) return 0;	   /* no need to check c2 */
    }
}


/*****************************************************************************/


int strncmp(const char *a, const char *b, size_t n)
{
    while (n-- > 0)
    {	char c1 = *a++, c2 = *b++;
	int d = c1 - c2;
	if (d != 0) return d;
	if (c1 == 0) return 0;	   /* no need to check c2 */
    }
    return 0;
}


/*****************************************************************************/


int strcasecmp(const char *a,const char *b)
{
    for (;;)
    {   char c1 = *a++, c2 = *b++;
        int d = toupper((int)c1) - toupper((int)c2);
        if (d != 0) return d;
        if (c1 == 0) return 0;     /* no need to check c2 */
    }
}


/*****************************************************************************/


int strncasecmp(const char *a, const char *b, size_t n)
{
    while (n-- > 0)
    {   char c1 = *a++, c2 = *b++;
        int d = toupper((int)c1) - toupper((int)c2);
        if (d != 0) return d;
        if (c1 == 0) return 0;     /* no need to check c2 */
    }
    return 0;
}
