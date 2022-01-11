/******************************************************************************
**
**  @(#) loadatag.c 95/11/27 1.9
**
**  Test ATAG loader
**
******************************************************************************/

#include <audio/atag.h>
#include <audio/audio.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* -------------------- main() */

void readatag (const char *filename, bool dump);
Err DumpItem (Item item, const char *banner);

int main (int argc, char *argv[])
{
    Err errcode;
    bool dump = FALSE;
    int i;

    if (argc < 2) {
        printf ("usage: %s [-dump] [-nodump] <.atag file>...\n", argv[0]);
        return 0;
    }

#ifdef MEMDEBUG
    if ((errcode = CreateMemDebug ( NULL )) < 0) goto clean;

    if ((errcode = ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
                                     MEMDEBUGF_FREE_PATTERNS |
                                     MEMDEBUGF_PAD_COOKIES |
                                     MEMDEBUGF_CHECK_ALLOC_FAILURES |
                                     MEMDEBUGF_KEEP_TASK_DATA)) < 0) goto clean;
#endif

    if ((errcode = OpenAudioFolio()) < 0) goto clean;

    for (i=1; i<argc; i++) {
        const char * const arg = argv[i];

        if (arg[0] == '-') {
            if (!strcasecmp (arg, "-dump")) dump = TRUE;
            else if (!strcasecmp (arg, "-nodump")) dump = FALSE;
            else printf ("unknown switch %s\n", arg);
        }
        else readatag (argv[i], dump);
    }

clean:
    if (errcode < 0) PrintError (NULL, NULL, NULL, errcode);
    CloseAudioFolio();
#ifdef MEMDEBUG
    DumpMemDebug(NULL);
    DeleteMemDebug();
#endif
    return 0;
}

void readatag (const char *filename, bool dump)
{
    Item atagitem;
    Err errcode;

    if ((errcode = atagitem = LoadATAG (filename)) < 0) goto clean;
    if (dump) DumpItem (atagitem, filename);
    else printf ("%s: 0x%x\n", filename, atagitem);

clean:
    if (errcode < 0) PrintError (NULL, "read ATAG file", filename, errcode);
    DeleteItem (atagitem);
}

Err DumpItem (Item item, const char *banner)
{
    char b[128];
    Err errcode;

    if (banner) printf ("\n%s:\n", banner);
    sprintf (b, "items -full 0x%x", item);
    if ((errcode = system (b)) < 0) goto clean;

    return 0;

clean:
    return errcode;
}
