/* @(#)fix_eol_pc.c 95/02/09 1.1 */

/*
 * fix_eol_pc: convert stream lines from UNIX EOL to PC EOL format.
 */

#include <stdio.h>

int
main()
{
    int	ch;
    while ((ch = getchar()) != EOF) {
	if (ch == '\n')
	    putchar('\r');
	putchar(ch);
    }
    return 0;
}
