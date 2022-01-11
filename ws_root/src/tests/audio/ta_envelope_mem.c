/******************************************************************************
**
**  @(#) ta_envelope_mem.c 96/08/09 1.11
**
**  Test envelope free mem
**
******************************************************************************/

#include <audio/audio.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <stdio.h>
#include <string.h>

void testenvelope (bool wait);

int main (int argc, char *argv[])
{
    Err errcode;

    TOUCH(argv);

    printf ("ta_envelope_mem: start\n");

  #ifdef MEMDEBUG
    if ((errcode = CreateMemDebug ( NULL )) < 0) goto clean;

    if ((errcode = ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
                                     MEMDEBUGF_FREE_PATTERNS |
                                     MEMDEBUGF_PAD_COOKIES |
                                     MEMDEBUGF_CHECK_ALLOC_FAILURES |
                                     MEMDEBUGF_KEEP_TASK_DATA)) < 0) goto clean;
  #endif

    if ((errcode = OpenAudioFolio()) < 0) goto clean;

    testenvelope(argc > 1);

clean:
    if (errcode < 0) PrintError (NULL, NULL, NULL, errcode);

    CloseAudioFolio();

  #ifdef MEMDEBUG
    DumpMemDebugVA(
        DUMPMEMDEBUG_TAG_SUPER, TRUE,
        TAG_END);
    DeleteMemDebug();
  #endif

    printf ("ta_envelope_mem: end\n");

    return 0;
}

static const EnvelopeSegment envDataTemplate[] = {
    { 0.0, 1.0 },
    { 1.0, 1.0 },
    { -1.0, 1.0 },
    { 0.0, 0.0 }
};
#define NUMPOINTS (sizeof envDataTemplate / sizeof envDataTemplate[0])

void testenvelope (bool wait)
{
    EnvelopeSegment *envData = NULL;
    Item envelope = -1;
    Err errcode;
    int32 sig;
    int i;

    if ((errcode = sig = AllocSignal(0)) <= 0) goto clean;

    if (!(envData = AllocMem (sizeof envDataTemplate, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE))) {
        errcode = -1;
        goto clean;
    }
    memcpy (envData, envDataTemplate, sizeof envDataTemplate);

    if ((errcode = envelope = CreateEnvelopeVA (envData, NUMPOINTS,
        TAG_ITEM_NAME,          "test.envelope",
        AF_TAG_AUTO_FREE_DATA,  TRUE,
        AF_TAG_SET_FLAGS,       AF_ENVF_FATLADYSINGS,
        AF_TAG_SUSTAINBEGIN,    1,
        AF_TAG_SUSTAINEND,      1,
        AF_TAG_RELEASEBEGIN,    2,
        AF_TAG_RELEASEEND,      2,
        AF_TAG_RELEASEJUMP,     2,
        AF_TAG_BASENOTE,        48,
        AF_TAG_NOTESPEROCTAVE,  -24,
        TAG_END)) < 0) goto clean;

    printf ("env=0x%x data=0x%08x:\n", envelope, envData);
    for (i=0; i<NUMPOINTS; i++) {
        printf ("  { %x, %x }\n", ConvertFP_TagData(envData[i].envs_Value), ConvertFP_TagData(envData[i].envs_Duration));
    }

#if 0
    {
        const Err errcode = SetAudioItemInfo (envelope, (TagArg *)1);

        if (errcode < 0) {
            PrintError (NULL, "modify envelope", NULL, errcode);
        }
    }
#endif

#if 0
    {
        const Err errcode = SetAudioItemInfoVA (envelope,
            AF_TAG_RELEASEBEGIN,    3,
            AF_TAG_RELEASEEND,      3,
          /*
            TAG_ITEM_NAME,          "foo.envelope",
            AF_TAG_AUTO_FREE_DATA,  FALSE,
            AF_TAG_FRAMES,          NUMPOINTS-1,
            AF_TAG_ADDRESS,         envData,
            TAG_JUMP, 1,
          */
            TAG_END);

        if (errcode < 0) {
            PrintError (NULL, "modify envelope", NULL, errcode);
        }
    }
#endif

#if 1
    {
        TagArg tags[] = {
            { AF_TAG_ADDRESS },
            { AF_TAG_FRAMES },
            { AF_TAG_TYPE },
            { AF_TAG_SUSTAINBEGIN },
            { AF_TAG_SUSTAINEND },
            { AF_TAG_SUSTAINTIME_FP },
            { AF_TAG_RELEASEBEGIN },
            { AF_TAG_RELEASEEND },
            { AF_TAG_RELEASEJUMP },
            { AF_TAG_RELEASETIME_FP },
            { AF_TAG_BASENOTE },
            { AF_TAG_NOTESPEROCTAVE },
            { AF_TAG_SET_FLAGS },
          #if 0
            { TAG_JUMP, (TagData)1 },
          #endif
            TAG_END
        };
        Err errcode;

      #if 0
        if ((errcode = GetAudioItemInfo (envelope, (TagArg *)1)) < 0)
      #endif
        if ((errcode = GetAudioItemInfo (envelope, tags)) < 0)
            PrintError (NULL, "get envelope info", NULL, errcode);
        else {
            DumpTagList (tags, "ta_envelope_mem");
        }
    }
#endif

    if (wait) {
        printf ("ta_envelope_mem(0x%x) signal=0x%x\n", CURRENTTASKITEM, sig);
        WaitSignal (sig);
    }

clean:
    if (errcode < 0) PrintError (NULL, "test envelope", NULL, errcode);

    DeleteEnvelope (envelope);

    printf ("after free:\n");
    for (i=0; i<NUMPOINTS; i++) {
        printf ("  { %x, %x }\n", ConvertFP_TagData(envData[i].envs_Value), ConvertFP_TagData(envData[i].envs_Duration));
    }

    if (wait) {
        printf ("ta_envelope_mem(0x%x) signal=0x%x\n", CURRENTTASKITEM, sig);
        WaitSignal (sig);
    }
}

void foo (void)
{
    StartInstrument (0, NULL);
}
