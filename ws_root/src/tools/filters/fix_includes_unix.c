/* @(#) fix_includes_unix.c 95/05/11 1.1 */

/*
 * pass-through to keep the makefiles simpler
 */

#include <stdio.h>

int
main(void)
{
    int	ch;
    while ((ch = getchar()) != EOF)
	putchar(ch);
    return 0;
}
