/******************************************************************************
**
**  @(#) loadins.c 96/08/28 1.33
**
**  Load an instrument (used to test DSPP disassembly)
**
**  Note: Define PORT1_3 when compiling under 1.3.
**
**  By: Bill Barton
**
**  Copyright (c) 1994, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  941216 WJB  Created.
**  950206 WJB  Added -keep option.
**  950207 WJB  Added 1.3 support.
**  950210 WJB  Added dsp resource printing.
**  950215 WJB  Spiffed up UnloadInstrument() calls.
**  950215 WJB  Spiffed up UnloadInstrument() calls some more.
**  950307 WJB  Added switches to create instruments w/ AF_TAG_CALCRATE_DIVIDE tag.
**  950309 WJB  Added -delay option.
**  950328 WJB  Replaced Sleep() (lib3do) call with WaitTime() (clib).
**              Updated usage text.
**  950601 WJB  Changed -keep option to accept a shut-down signal.
**  951120 WJB  Use LoadScoreTemplate() instead of LoadInsTemplate().
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <audio/audio.h>
#include <audio/dspp_template.h>
#include <audio/score.h>    /* LoadScoreTemplate() */
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/time.h>
#include <stdio.h>
#include <stdlib.h>         /* calloc() */
#include <string.h>         /* strcmp() */

#if 0
NEWMEM
#define printavail(desc) \
    do { \
        MemInfo meminfo; \
        \
        AvailMem (&meminfo, MEMTYPE_ANY); \
        printf ("%s: sys=%lu:%lu task=%lu:%lu total=%lu\n", desc, meminfo, meminfo.minfo_SysFree + meminfo.minfo_TaskFree); \
    } while(0)
#else
#define printavail(desc)
#endif

#ifndef ER_USER
  #define ER_USER    Make6Bit('U')  /* for user code */
#endif
#define MakeUErr(svr,class,err) MakeErr(ER_USER,0,svr,ER_E_USER,class,err)

Err loadinstruments (int argc, const char * const argv[]);

int main (int argc, char *argv[])
{
    Err errcode;

    if (argc < 2) {
        printf ("usage: %s [-keep | -delay] [-rev | -sloppy] [-hang]\n", argv[0]);
        printf ("       { [-1 | -2 | -8] [-template | -instrument] [-dump | -nodump] [-audin | -noaudin]\n");
        printf ("         [ <dsp instrument name> | <patch file name> | <template item number> ] }\n");
        printf ("  -delay      Wait 5 seconds before unloading instruments.\n");
        printf ("  -hang       Wait for signal between freeing instruments and exiting.\n");
        printf ("  -keep       Wait for signal before unloading instruments.\n");
        printf ("  -sloppy     Exit without deleting created items.\n");
        printf ("  -rev        Unload instruments in same order as loaded. Normally unloads in reverse order.\n");
        printf ("  -1          Select full rate execution for subsequent instruments.\n");
        printf ("  -2          Select half rate execution for subsequent instruments.\n");
        printf ("  -8          Select 1/8th rate execution for subsequent instruments.\n");
        printf ("  -template   Only create Templates, not instruments.\n");
        printf ("  -instrument Create Templates and Instruments (default).\n");
        printf ("  -dump       Enable dumping DSPPTemplate.\n");
        printf ("  -nodump     Disable dumping DSPPTemplate (default).\n");
        printf ("  -audin      Enable audio input.\n");
        printf ("  -noaudin    Disable audio input (default).\n");
        return 0;
    }

    printavail("start");
  #ifdef MEMDEBUG
    if ((errcode = CreateMemDebug ( NULL )) < 0) goto clean;

    if ((errcode = ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
                         MEMDEBUGF_FREE_PATTERNS |
                         MEMDEBUGF_PAD_COOKIES |
                         MEMDEBUGF_CHECK_ALLOC_FAILURES |
                         MEMDEBUGF_KEEP_TASK_DATA)) < 0) goto clean;
  #endif

    if ((errcode = OpenAudioFolio()) < 0) goto clean;

    errcode = loadinstruments (argc, (const char * const *)argv);

clean:
    if (errcode) PrintError (NULL, NULL, NULL, errcode);
    CloseAudioFolio();

  #ifdef MEMDEBUG
    DumpMemDebugVA (
        DUMPMEMDEBUG_TAG_SUPER, TRUE,
        TAG_END);
    DeleteMemDebug();
  #endif
    printavail("end");

    return 0;
}

struct instab {
    Item tempitem;
    Item insitem;
    bool deleteTemplate;
};
static void enableaudin (bool enable);
static Err loadins (struct instab *instab, const char *insDesc, int32 ratediv, bool instr, bool dump);
static Err FindInsTemplate (struct instab *instab, const char *insDesc);
static Err unloadins (struct instab *instab);
static void printdspavail (const char *desc);
static void PrintInsResources (const char *desc, Item item);

static Err loadinstruments (int argc, const char * const argv[])
{
    const int32 quitsig = AllocSignal (0);
    struct instab *instab;
    bool keep = FALSE;
    bool revfree = FALSE;
    bool nofree = FALSE;
    bool delay = FALSE;
    bool hang = FALSE;
    bool instr = TRUE;
    bool dump = FALSE;
    int32 ratediv = 1;
    int numins = 0;
    Err errcode = 0;
    int i;

    if ((instab = (struct instab *)calloc (argc, sizeof *instab)) == NULL) {
        errcode = MakeUErr (ER_SEVERE, ER_C_STND, ER_NoMem);
        goto clean;
    }

    printdspavail ("start");
    for (i=1; i<argc; i++) {
        if (!strcasecmp (argv[i], "-keep")) keep = TRUE;
        else if (!strcasecmp (argv[i], "-rev")) revfree = TRUE;
        else if (!strcasecmp (argv[i], "-sloppy")) nofree = TRUE;
        else if (!strcasecmp (argv[i], "-1")) ratediv = 1;
        else if (!strcasecmp (argv[i], "-2")) ratediv = 2;
        else if (!strcasecmp (argv[i], "-8")) ratediv = 8;
        else if (!strcasecmp (argv[i], "-delay")) delay = TRUE;
        else if (!strcasecmp (argv[i], "-hang")) hang = TRUE;
        else if (!strcasecmp (argv[i], "-template")) instr = FALSE;
        else if (!strcasecmp (argv[i], "-instrument")) instr = TRUE;
        else if (!strcasecmp (argv[i], "-dump")) dump = TRUE;
        else if (!strcasecmp (argv[i], "-nodump")) dump = FALSE;
        else if (!strcasecmp (argv[i], "-audin")) enableaudin (TRUE);
        else if (!strcasecmp (argv[i], "-noaudin")) enableaudin (FALSE);
        else {
            const char * const insDesc = argv[i];
            Err errcode;        /* local to avoid propogating this error code to outer scope */

            printf ("\n%s: load instrument %s\n", argv[0], insDesc);

            if ((errcode = loadins (&instab[numins], insDesc, ratediv, instr, dump)) < 0) {
                PrintError (NULL, "load instrument", insDesc, errcode);
            }
            else {
                numins++;
            }
            printdspavail (insDesc);
            TOUCH(errcode);     /* silence warning for not using errcode */
        }
    }

    if (keep) {
        printf ("loadins(0x%05lx): Instruments loaded. Waiting for unload signal 0x%08lx\n", CURRENTTASKITEM, quitsig);
        WaitSignal (quitsig);
    }
    else if (delay) {               /* sleep for 5 seconds */
        Item timerio = CreateTimerIOReq();
        WaitTime (timerio,5,0);
        DeleteTimerIOReq (timerio);
    }

    if (!nofree) {
        if (revfree) for (i=0; i<numins; i++) {
            Err errcode;        /* local to avoid propogating this error code to outer scope */

            if ((errcode = unloadins (&instab[i])) < 0) {
                PrintError (NULL, "unload instrument", NULL, errcode);
            }
            TOUCH(errcode);     /* silence warning for not using errcode */
        }
        else for (i=numins; i--;) {
            Err errcode;        /* local to avoid propogating this error code to outer scope */

            if ((errcode = unloadins (&instab[i])) < 0) {
                PrintError (NULL, "unload instrument", NULL, errcode);
            }
            TOUCH(errcode);     /* silence warning for not using errcode */
        }
    }
    if (hang) {
        printf ("loadins(0x%05lx): instruments unloaded. Waiting for quit signal 0x%08lx\n", CURRENTTASKITEM, quitsig);
        WaitSignal (quitsig);
    }
    printdspavail ("end");

clean:
    if (instab) free (instab);
    return errcode;
}

static void enableaudin (bool enable)
{
    Err errcode;

    if ((errcode = EnableAudioInput (enable, NULL)) < 0) {
        PrintError (NULL, enable ? "enable audio input" : "disable audio input", NULL, errcode);
    }
}

static Err loadins (struct instab *instab, const char *insDesc, int32 ratediv, bool instr, bool dump)
{
    Err errcode;

    if ((errcode = FindInsTemplate (instab, insDesc)) < 0) goto clean;

    if (dump) {
        const DSPPTemplate * const dtmp = dsppLookupTemplate (instab->tempitem);

        dsppDumpTemplate (dtmp, insDesc);
        printf ("\n");
    }

    PrintInsResources (insDesc, instab->tempitem);

    if (instr) {
        if ((errcode = instab->insitem = CreateInstrumentVA (instab->tempitem,
                                                             AF_TAG_CALCRATE_DIVIDE, ratediv,
                                                             TAG_END)) < 0) goto clean;
        PrintInsResources (insDesc, instab->insitem);
    }

    return 0;

clean:
    unloadins (instab);
    return errcode;
}

static Err FindInsTemplate (struct instab *instab, const char *insDesc)
{
    Err errcode;

        /* is it an item number? */
    {
        const char *t;

        instab->tempitem = strtol (insDesc, &t, 0);
        if (!*t && CheckItem(instab->tempitem, AUDIONODE, AUDIO_TEMPLATE_NODE)) return 0;
    }

        /* must be a name otherwise */
    if ((errcode = instab->tempitem = LoadScoreTemplate (insDesc)) < 0) goto clean;
    instab->deleteTemplate = TRUE;
    return 0;

clean:
    return errcode;
}

static Err unloadins (struct instab *instab)
{
    Err errcode = 0;

    if (instab->insitem) {
        errcode = DeleteInstrument (instab->insitem);
        instab->insitem = -1;
    }
    if (instab->deleteTemplate && instab->tempitem) {
        errcode = UnloadScoreTemplate (instab->tempitem);
        instab->tempitem = -1;
    }

    return errcode;
}


static const char * const rsrcdesc [AF_RESOURCE_TYPE_MANY] = {  /* @@@ depends on AF_RESOURCE_TYPE_ order */
    "Ticks",
    "Code",
    "Data",
    "FIFOs",
    "Triggers",
};

static void printdspavail (const char *desc)
{
    int32 i;
    Err errcode;

    printf ("%s: avail", desc);
    for (i=0; i<AF_RESOURCE_TYPE_MANY; i++) {
        AudioResourceInfo rinfo;

        if ((errcode = GetAudioResourceInfo (&rinfo, sizeof rinfo, i)) >= 0) {
            printf (" %s=%d,%d", rsrcdesc[i], rinfo.rinfo_Free, rinfo.rinfo_LargestFreeSpan);
        }
        else {
            printf (" %s=error 0x%x", rsrcdesc[i], errcode);
        }
    }
    printf ("\n");
}

static void PrintInsResources (const char *desc, Item item)
{
    const ItemNode * const n = LookupItem (item);

    if (n) {
        printf ("%s (item 0x%x, type %d) ", desc, item, n->n_Type);
        DumpInstrumentResourceInfo (item, "usage");
    }
}
