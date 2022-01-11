/* @(#)fix_eol_unix.c 95/02/09 1.1 */

/*
 * fix_eol_unix: convert stream lines from UNIX EOL to UNIX EOL format.
 */

#include <stdio.h>

int
main()
{
    int	ch;
    while ((ch = getchar()) != EOF)
	putchar(ch);
    return 0;
}
