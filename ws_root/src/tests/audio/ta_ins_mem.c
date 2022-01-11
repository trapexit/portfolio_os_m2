/******************************************************************************
**
**  @(#) ta_ins_mem.c 96/08/27 1.1
**
**  Test instrument
**
******************************************************************************/

#include <audio/audio.h>
#include <audio/score.h>
#include <kernel/mem.h>
#include <stdio.h>
#include <stdlib.h>

static void TestInstrument (const char *insDesc, bool wait);
static Item FindInsTemplate (const char *insDesc);

int main (int argc, char *argv[])
{
    Err errcode;

    TOUCH(argv);

    printf ("ta_ins_mem: start\n");

  #ifdef MEMDEBUG
    if ((errcode = CreateMemDebug ( NULL )) < 0) goto clean;

    if ((errcode = ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
                                     MEMDEBUGF_FREE_PATTERNS |
                                     MEMDEBUGF_PAD_COOKIES |
                                     MEMDEBUGF_CHECK_ALLOC_FAILURES |
                                     MEMDEBUGF_KEEP_TASK_DATA)) < 0) goto clean;
  #endif

    if ((errcode = OpenAudioFolio()) < 0) goto clean;

    TestInstrument (argc > 1 ? argv[1] : "sawtooth.dsp", argc > 2);

clean:
    if (errcode < 0) PrintError (NULL, NULL, NULL, errcode);

    CloseAudioFolio();

  #ifdef MEMDEBUG
    DumpMemDebugVA (
        DUMPMEMDEBUG_TAG_SUPER, TRUE,
        TAG_END);
    DeleteMemDebug();
  #endif

    printf ("ta_ins_mem: end\n");

    return 0;
}

static void TestInstrument (const char *insDesc, bool wait)
{
    Item insTemplate = -1;
    Item instrument = -1;
    Err errcode;
    int32 sig;

    if ((errcode = sig = AllocSignal(0)) <= 0) goto clean;

    if ((errcode = insTemplate = FindInsTemplate (insDesc)) < 0) goto clean;

#if 1
    if ((errcode = instrument = CreateItemVA (MKNODEID(AUDIONODE,AUDIO_INSTRUMENT_NODE),
  /*
    if ((errcode = instrument = CreateInstrumentVA (insTemplate,
  */
        AF_TAG_TEMPLATE,        insTemplate,
        TAG_ITEM_NAME,          "test.instrument",
      /*
        AF_TAG_CALCRATE_DIVIDE, 2,
        AF_TAG_SET_FLAGS,       AF_INSF_AUTOABANDON,
        AF_TAG_PRIORITY,        200,
        AF_TAG_SPECIAL,         0,
        TAG_ITEM_NAME,          0x1,
        TAG_JUMP,               1,
        AF_TAG_ADDRESS,         0,
      */
        TAG_END)) < 0) goto clean;
#endif

    printf ("ins=0x%x tmpl=0x%x\n", instrument, insTemplate);

#if 0       /* !!! fix me */
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
            DumpTagList (tags, "ta_ins_mem");
            printf ("sample rate = %g Hz\n", ConvertTagData_FP(GetTagArg(tags, AF_TAG_SAMPLE_RATE_FP, ConvertFP_TagData(0))));
            printf ("base freq   = %g Hz\n", ConvertTagData_FP(GetTagArg(tags, AF_TAG_BASEFREQ_FP, ConvertFP_TagData(0))));
        }
    }
#endif

    if (wait) {
        printf ("ta_ins_mem(0x%x) signal=0x%x\n", CURRENTTASKITEM, sig);
        WaitSignal (sig);
    }

clean:
    if (errcode < 0) PrintError (NULL, "test instrument", insDesc, errcode);

    if ((errcode = DeleteItem (instrument)) < 0) {
        PrintError (NULL, "delete instrument", insDesc, errcode);
    }
    if ((errcode = DeleteItem (insTemplate)) < 0) {
        PrintError (NULL, "delete ins template", insDesc, errcode);
    }

    if (wait) {
        printf ("ta_ins_mem(0x%x) signal=0x%x\n", CURRENTTASKITEM, sig);
        WaitSignal (sig);
    }
}

static Item FindInsTemplate (const char *insDesc)
{
        /* is it an item number? */
    {
        const char *t;
        const Item insTemplate = strtol (insDesc, &t, 0);

        if (!*t && CheckItem (insTemplate, AUDIONODE, AUDIO_TEMPLATE_NODE)) return insTemplate;
    }

        /* must be a name otherwise */
    return LoadScoreTemplate (insDesc);
}
