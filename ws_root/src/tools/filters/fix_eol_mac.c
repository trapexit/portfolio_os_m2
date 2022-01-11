/* @(#)fix_eol_mac.c 95/02/09 1.1 */

/*
 * fix_eol_mac: convert stream lines from UNIX EOL to MAC EOL format.
 */

#include <stdio.h>

int
main()
{
    int	ch;
    while ((ch = getchar()) != EOF) {
	if (ch == '\n')
	    putchar('\r');
	else
	    putchar(ch);
    }
    return 0;
}
