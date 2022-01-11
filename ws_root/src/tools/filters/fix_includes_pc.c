/* @(#) fix_includes_pc.c 95/05/23 1.3 */

/* fix_includes_pc: copy stdin to stdout, converting #include
 * statements to a form suitable for the PC.
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>


#define	MAXLINELEN 1024


int main (void)
{
char line[MAXLINELEN];
int  i;

    while (fgets (line, MAXLINELEN, stdin))
    {
        i = 0;
        while ((line[i] == ' ') || (line[i] == '\t'))
            i++;

        if (line[i] == '#')
        {
            i++;
            while ((line[i] == ' ') || (line[i] == '\t'))
                i++;

            if ((strncmp(&line[i],"include ",8) == 0)
             || (strncmp(&line[i],"include\t",8) == 0))
            {
                i += 8;
                while ((line[i] != '/') && (line[i] != '>') && line[i])
                    i++;

                if (line[i] == '<')
                {
                    i++;
                    while (line[i] != '>')
                    {
                        if (line[i] == '/')
                            line[i] == '\\';
                        i++;
                    }
                }
            }
        }

        fputs (line, stdout);
    }

    return 0;
}
