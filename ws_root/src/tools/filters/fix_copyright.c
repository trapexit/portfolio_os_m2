/* @(#) fix_copyright.c 96/02/20 1.4 */

/*
 * fix_copyright: copy stdin to stdout, inserting a copyright
 * message along the way.
 *
 * Watches the stream for any line containing the start of
 * an ID string, then figures out from that line what text
 * to put around the copyright message; thus we are not
 * dependent on knowning what comment characters to use.
 *
 * Only the first such line triggers message insertion.
 *
 * If we see a copyright message go by, we stop looking
 * for a place to insert our message. Note that we will still
 * get multiple copyright messages if a file contains a copyright
 * message somewhere after its ID string.
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>

static char *copyright_message[] =
{
    "Copyright (c) 1993-1996, an unpublished work by The 3DO Company.",
    "All rights reserved. This material contains confidential",
    "information that is the property of The 3DO Company. Any",
    "unauthorized duplication, disclosure or use is prohibited.",
    ""
};

#define	COPYR_LINES	((sizeof copyright_message) / (sizeof copyright_message[0]))

static char copyright_match[] = "opyright";
static char id_head[] = "@(#)";
static char id_tail[] = "@(#)";
static char id_eoc[] = "*/";


#ifndef	MAXLINELEN
#define	MAXLINELEN 1024
#endif

static char textbuf[MAXLINELEN];


int main(void)
{
char *head = 0;
char *tail;
int   searching = 1;
int   line;

    while (fgets(textbuf, MAXLINELEN, stdin))
    {
        if (searching)
        {
            if (strstr(textbuf, copyright_match))
                searching = 0;
        }

        if (searching && (head = strstr(textbuf, id_head)))
        {
            if (tail = strstr(head + sizeof id_head - 1, id_tail))
            {
                tail += sizeof id_tail - 1;
            }
            else if (tail = strstr(head + sizeof id_head - 1, id_eoc))
            {
                /* do not advance over end of comment! */
            }
            else if (tail = strchr(head + sizeof id_head - 1, '"'))
            {
                /* do not advance over end of string! */
            }
            else
            {
                tail = "\n";
            }

            *head = 0;
            for (line = 0; line < COPYR_LINES; ++line)
                printf("%s%s%s", textbuf, copyright_message[line], tail);

            *head = id_head[0];

            searching = 0;
            fputs(textbuf, stdout);

            *head = 0;
            printf("%sDistributed with Portfolio V%d.%d%s",textbuf, OS_VERSION, OS_REVISION, tail);
            *head = id_head[0];
        }
        else
        {
            fputs(textbuf, stdout);
        }
    }
    return 0;
}
