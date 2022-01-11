/******************************************************************************
**
**  @(#) ta_sample_mem.c 96/08/08 1.11
**
**  Test sample free mem
**
******************************************************************************/

#include <audio/audio.h>
#include <audio/parse_aiff.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <stdio.h>
#include <string.h>

void testsample (bool wait);

int main (int argc, char *argv[])
{
    Err errcode;

    TOUCH(argv);

    printf ("ta_sample_mem: start\n");

  #ifdef MEMDEBUG
    if ((errcode = CreateMemDebug ( NULL )) < 0) goto clean;

    if ((errcode = ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
                                     MEMDEBUGF_FREE_PATTERNS |
                                     MEMDEBUGF_PAD_COOKIES |
                                     MEMDEBUGF_CHECK_ALLOC_FAILURES |
                                     MEMDEBUGF_KEEP_TASK_DATA)) < 0) goto clean;
  #endif

    if ((errcode = OpenAudioFolio()) < 0) goto clean;

    testsample(argc > 1);

clean:
    if (errcode < 0) PrintError (NULL, NULL, NULL, errcode);

    CloseAudioFolio();

  #ifdef MEMDEBUG
    DumpMemDebugVA (
        DUMPMEMDEBUG_TAG_SUPER, TRUE,
        TAG_END);
    DeleteMemDebug();
  #endif

    printf ("ta_sample_mem: end\n");

    return 0;
}

#define NUMWORDS 4

void testsample (bool wait)
{
    Item sample = -1;
    uint32 *sampData = NULL;
    Err errcode;
    int32 sig;
    int i;

    if ((errcode = sig = AllocSignal(0)) <= 0) goto clean;

#if 1
    if (!(sampData = AllocMem (NUMWORDS * sizeof (uint32), MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE | MEMTYPE_FILL))) {
        errcode = -1;
        goto clean;
    }
    for (i=0; i<NUMWORDS; i++) sampData[i] = i;
#endif

#if 1
    if ((errcode = sample = CreateSampleVA (
        TAG_ITEM_NAME,          "test.sample",
        AF_TAG_AUTO_FREE_DATA,  TRUE,
        AF_TAG_ADDRESS,         sampData,
        AF_TAG_NUMBYTES,        NUMWORDS * sizeof (uint32),
      /*
        AF_TAG_DELAY_LINE,      NUMWORDS * sizeof (uint32),
        AF_TAG_DELAY_LINE_TEMPLATE, NUMWORDS * sizeof (uint32),
        AF_TAG_SET_FLAGS,       AF_SAMPF_CLEARONSTOP,
        AF_TAG_CLEAR_FLAGS,     AF_SAMPF_CLEARONSTOP,
      */
      /*
        AF_TAG_SUSTAINBEGIN,    0,
        AF_TAG_SUSTAINEND,      3,
        AF_TAG_ADDRESS,         sampData,
        AF_TAG_NUMBYTES,        NUMWORDS * sizeof (uint32),
        AF_TAG_SUSTAINBEGIN,    1,
        AF_TAG_SUSTAINEND,      2,
        AF_TAG_RELEASEBEGIN,    3,
        AF_TAG_RELEASEEND,      5,
        AF_TAG_LOWNOTE,         24,
        AF_TAG_HIGHNOTE,        64,
        AF_TAG_LOWVELOCITY,     72,
        AF_TAG_HIGHVELOCITY,    127,
      */
      /*
        AF_TAG_DELAY_LINE,      NUMWORDS * sizeof (uint32),
        TAG_JUMP, 1)) < 0) goto clean;
      */
        TAG_END)) < 0) goto clean;
#endif

#if 0
    if ((errcode = sample = CreateDelayLine (NUMWORDS * sizeof (uint32), 1, TRUE)) < 0) goto clean;
#endif

#if 0
    if ((errcode = sample = CreateDelayLineTemplate (NUMWORDS * sizeof (uint32), 1, TRUE)) < 0) goto clean;
#endif

#if 0
    if ((errcode = sample = LoadSystemSample ("sinewave.aiff")) < 0) goto clean;
#endif

    DebugSample (sample);

#if 0
    {
        TagArg tags[] = {
            { AF_TAG_ADDRESS },
            TAG_END
        };

        if ((errcode = GetAudioItemInfo (sample, tags)) < 0) goto clean;
        sampData = (uint32 *)tags[0].ta_Arg;
    }
#endif

    printf ("samp=0x%x data=0x%08x:", sample, sampData);
    if (sampData) for (i=0; i<NUMWORDS; i++) {
        printf (" %08x", sampData[i]);
    }
    printf ("\n");

#if 0
    {
        const Err errcode = SetAudioItemInfo (sample, (TagArg *)1);

        if (errcode < 0) {
            PrintError (NULL, "modify sample", NULL, errcode);
        }
        DebugSample (sample);
    }
#endif

#if 0
    {
        const Err errcode = SetAudioItemInfoVA (sample,
          /*
            AF_TAG_SUPPRESS_WRITE_BACK_D_CACHE, TRUE,
            AF_TAG_ADDRESS,         sampData,
            AF_TAG_NUMBYTES,        NUMWORDS * sizeof (uint32),
            AF_TAG_FRAMES,          NUMWORDS * sizeof (uint32) / sizeof (uint16),
            AF_TAG_DETUNE,          100,
            AF_TAG_BASENOTE,        AF_A440_PITCH,
            AF_TAG_DETUNE,          0,
            AF_TAG_BASENOTE,        AF_A440_PITCH-1,
            AF_TAG_SAMPLE_RATE_FP,  ConvertFP_TagData(22050),

            AF_TAG_SUSTAINBEGIN,    0,
            AF_TAG_SUSTAINEND,      2,
            AF_TAG_RELEASEBEGIN,    2,
            AF_TAG_RELEASEEND,      5,
            AF_TAG_NUMBITS,         16,
            AF_TAG_CHANNELS,        1,
            AF_TAG_COMPRESSIONRATIO, 4,
            AF_TAG_COMPRESSIONTYPE, ID_ADP4,
            AF_TAG_NUMBYTES,        13,
            AF_TAG_FRAMES,          7,
            AF_TAG_CHANNELS,        2,
            TAG_ITEM_NAME,          "foo.sample",
            AF_TAG_RELEASEBEGIN,    3,
            AF_TAG_RELEASEEND,      3,
            AF_TAG_AUTO_FREE_DATA,  FALSE,
          */
            AF_TAG_SUSTAINBEGIN,    0,
            AF_TAG_SUSTAINEND,      2,
            AF_TAG_RELEASEBEGIN,    2,
            AF_TAG_RELEASEEND,      5,
            TAG_JUMP, NULL);

        if (errcode < 0) {
            PrintError (NULL, "modify sample", NULL, errcode);
        }
        DebugSample (sample);
    }
#endif

#if 0
    {
        TagArg tags[] = {
            { AF_TAG_ADDRESS },
            { AF_TAG_FRAMES },
            { AF_TAG_NUMBYTES },
            { AF_TAG_CHANNELS },
            { AF_TAG_WIDTH  },
            { AF_TAG_NUMBITS },
            { AF_TAG_COMPRESSIONTYPE },
            { AF_TAG_COMPRESSIONRATIO },
            { AF_TAG_SUSTAINBEGIN },
            { AF_TAG_SUSTAINEND },
            { AF_TAG_RELEASEBEGIN },
            { AF_TAG_RELEASEEND },
            { AF_TAG_BASENOTE },
            { AF_TAG_DETUNE },
            { AF_TAG_SAMPLE_RATE_FP },
            { AF_TAG_BASEFREQ_FP },
            { AF_TAG_LOWNOTE },
            { AF_TAG_HIGHNOTE },
            { AF_TAG_LOWVELOCITY },
            { AF_TAG_HIGHVELOCITY },
            { AF_TAG_SET_FLAGS },
          #if 0
            { AF_TAG_JUMP, (TagData)1 },
          #endif
            TAG_END
        };
        Err errcode;

        if ((errcode = GetAudioItemInfo (sample, tags)) < 0)
            PrintError (NULL, "get sample info", NULL, errcode);
        else {
            DumpTagList (tags, "ta_sample_mem");
            printf ("sample rate = %g Hz\n", ConvertTagData_FP(GetTagArg(tags, AF_TAG_SAMPLE_RATE_FP, ConvertFP_TagData(0))));
            printf ("base freq   = %g Hz\n", ConvertTagData_FP(GetTagArg(tags, AF_TAG_BASEFREQ_FP, ConvertFP_TagData(0))));
        }
    }
#endif

#if 0
    {
        const Item delayins = LoadInstrument ("delay_f1.dsp", 0, 100);
        const Item attachment = CreateAttachment (delayins, sample, NULL);

        if (attachment < 0) {
            PrintError (NULL, "create attachment", NULL, attachment);
        }

        DeleteAttachment (attachment);
        UnloadInstrument (delayins);
    }
#endif

    if (wait) {
        printf ("ta_sample_mem(0x%x) signal=0x%x\n", CURRENTTASKITEM, sig);
        WaitSignal (sig);
    }

clean:
    if (errcode < 0) PrintError (NULL, "test sample", NULL, errcode);

    DeleteSample (sample);

    if (sampData) {
        printf ("after free:");
        for (i=0; i<NUMWORDS; i++) {
            printf (" %08x", sampData[i]);
        }
        printf ("\n");
    }

    if (wait) {
        printf ("ta_sample_mem(0x%x) signal=0x%x\n", CURRENTTASKITEM, sig);
        WaitSignal (sig);
    }
}

/* need to fake out loader */
void foo (void)
{
    StartInstrument (0, NULL);
}
