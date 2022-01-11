/******************************************************************************
**
**  @(#) ta_tuning_mem.c 96/08/09 1.6
**
**  Test tuning free mem
**
******************************************************************************/

#include <audio/audio.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <stdio.h>

#define NUMINTERVALS (5)
#define NOTESPEROCTAVE NUMINTERVALS
#define BASENOTE (AF_A440_PITCH)
#define BASEFREQ (440.0)  /* A440 */

void testtuning (bool wait);

int main (int argc, char *argv[])
{
    Err errcode;

    TOUCH(argv);

    printf ("ta_tuning_mem: start\n");

  #ifdef MEMDEBUG
    if ((errcode = CreateMemDebug ( NULL )) < 0) goto clean;

    if ((errcode = ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
                                     MEMDEBUGF_FREE_PATTERNS |
                                     MEMDEBUGF_PAD_COOKIES |
                                     MEMDEBUGF_CHECK_ALLOC_FAILURES |
                                     MEMDEBUGF_KEEP_TASK_DATA)) < 0) goto clean;
  #endif

    if ((errcode = OpenAudioFolio()) < 0) goto clean;

    testtuning(argc > 1);

clean:
    if (errcode < 0) PrintError (NULL, NULL, NULL, errcode);

    CloseAudioFolio();

  #ifdef MEMDEBUG
    DumpMemDebugVA(
        DUMPMEMDEBUG_TAG_SUPER, TRUE,
        TAG_END);
    DeleteMemDebug();
  #endif

    printf ("ta_tuning_mem: end\n");

    return 0;
}

void testtuning (bool wait)
{
    float32 *tuningTable = NULL;
    Item tuning = -1;
    Err errcode;
    int32 sig;
    int i;

    if ((errcode = sig = AllocSignal(0)) <= 0) goto clean;

    if (!(tuningTable = AllocMem (NUMINTERVALS * sizeof (float32), MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE | MEMTYPE_FILL))) {
        errcode = -1;
        goto clean;
    }

    tuningTable[0] = BASEFREQ;   /* 1:1 */
    tuningTable[1] = BASEFREQ * 8.0 / 7.0;
    tuningTable[2] = BASEFREQ * 5.0 / 4.0;
    tuningTable[3] = BASEFREQ * 3.0 / 2.0;
    tuningTable[4] = BASEFREQ * 7.0 / 4.0;

    if ((errcode = tuning = CreateItemVA (MKNODEID(AUDIONODE,AUDIO_TUNING_NODE),
        AF_TAG_ADDRESS,         tuningTable,
        AF_TAG_FRAMES,          NUMINTERVALS,
        AF_TAG_NOTESPEROCTAVE,  NOTESPEROCTAVE,
        AF_TAG_BASENOTE,        BASENOTE,
        AF_TAG_AUTO_FREE_DATA,  TRUE,
        TAG_ITEM_NAME,          "test.tuning",
        TAG_END)) < 0) goto clean;

    printf ("tuning=0x%x data=0x%08x:", tuning, tuningTable);
    for (i=0; i<NUMINTERVALS; i++) {
        printf (" 0x%08x", ((uint32 *)tuningTable)[i]);
    }
    printf ("\n");

#if 0
    {
        const Err errcode = SetAudioItemInfo (tuning, (TagArg *)1);

        if (errcode < 0) {
            PrintError (NULL, "modify tuning", NULL, errcode);
        }
    }
#endif

#if 1
    {
        const Err errcode = SetAudioItemInfoVA (tuning,
            AF_TAG_BASENOTE,        BASENOTE+5,
          /*
            AF_TAG_ADDRESS,         tuningTable,
            TAG_ITEM_NAME,          "foo.tuning",
            AF_TAG_FRAMES,          NUMINTERVALS-1,
            AF_TAG_NOTESPEROCTAVE,  NOTESPEROCTAVE-1,
            AF_TAG_AUTO_FREE_DATA,  TRUE,
            TAG_JUMP, 1,
          */
            TAG_END);

        if (errcode < 0) {
            PrintError (NULL, "modify tuning", NULL, errcode);
        }
    }
#endif

    if (wait) {
        printf ("ta_tuning_mem(0x%x) signal=0x%x\n", CURRENTTASKITEM, sig);
        WaitSignal (sig);
    }

clean:
    if (errcode < 0) PrintError (NULL, "test tuning", NULL, errcode);

    DeleteTuning (tuning);

    printf ("after free:");
    for (i=0; i<NUMINTERVALS; i++) {
        printf (" 0x%08x", ((uint32 *)tuningTable)[i]);
    }
    printf ("\n");

    if (wait) {
        printf ("ta_tuning_mem(0x%x) signal=0x%x\n", CURRENTTASKITEM, sig);
        WaitSignal (sig);
    }
}
