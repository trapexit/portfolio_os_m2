/******************************************************************************
**
**  @(#) ta_attach_mem.c 96/08/09 1.8
**
**  Test attachment auto delete slave
**
******************************************************************************/

#include <audio/audio.h>
#include <audio/patch.h>
#include <audio/parse_aiff.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <stdio.h>
#include <string.h>

static Item maketestpatch (void);
static Item maketestenvelope (void);
void testattachment (bool wait);
static void dumpitem (Item item, const char *desc);

int main (int argc, char *argv[])
{
    Err errcode;

    TOUCH(argv);

    printf ("ta_attachment_mem: start\n");

  #ifdef MEMDEBUG
    if ((errcode = CreateMemDebug ( NULL )) < 0) goto clean;

    if ((errcode = ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
                                     MEMDEBUGF_FREE_PATTERNS |
                                     MEMDEBUGF_PAD_COOKIES |
                                     MEMDEBUGF_CHECK_ALLOC_FAILURES |
                                     MEMDEBUGF_KEEP_TASK_DATA)) < 0) goto clean;
  #endif

    if ((errcode = OpenAudioFolio()) < 0) goto clean;

    testattachment(argc > 1);

clean:
    if (errcode < 0) PrintError (NULL, NULL, NULL, errcode);

    CloseAudioFolio();

  #ifdef MEMDEBUG
    DumpMemDebugVA (
        DUMPMEMDEBUG_TAG_SUPER, TRUE,
        TAG_END);
    DeleteMemDebug();
  #endif

    printf ("ta_attachment_mem: end\n");

    return 0;
}

void testattachment (bool wait)
{
    Item sample = -1;
    Item envelope = -1;
    Item insTemplate = -1;
    Item instrument1 = -1;
    Item instrument2 = -1;
    Item instrument3 = -1;
    Item instrument4 = -1;
    Item tmpl_samp_att1 = -1;
    Item tmpl_samp_att2 = -1;
    Item tmpl_env_att1 = -1;
    Item tmpl_env_att2 = -1;
    Item ins3_samp_att1 = -1;
    Item ins3_samp_att2 = -1;
    Item ins4_samp_att1 = -1;
    Item ins4_samp_att2 = -1;
    Item ins3_env_att1 = -1;
    Item ins3_env_att2 = -1;
    Item ins4_env_att1 = -1;
    Item ins4_env_att2 = -1;
    Err errcode;
    int32 sig;

    if ((errcode = sig = AllocSignal(0)) <= 0) goto clean;

    if ((errcode = sample = LoadSystemSample ("sinewave.aiff")) < 0) goto clean;
    if ((errcode = envelope = maketestenvelope()) < 0) goto clean;
    if ((errcode = insTemplate = maketestpatch()) < 0) goto clean;
/*  if ((errcode = insTemplate = LoadInsTemplate ("sampler_16_v1.dsp", NULL)) < 0) goto clean; */

#if 0
    if ((errcode = instrument1 = CreateInstrument (insTemplate, NULL)) < 0) goto clean;
    if ((errcode = instrument2 = CreateInstrument (insTemplate, NULL)) < 0) goto clean;
#endif

#if 1
    dumpitem (tmpl_samp_att1 = CreateAttachmentVA (insTemplate, sample,
        TAG_ITEM_NAME,              "template sample att 1",
        AF_TAG_NAME,                "InFIFO",
        AF_TAG_SET_FLAGS,           AF_ATTF_NOAUTOSTART,
        AF_TAG_START_AT,            15,
        AF_TAG_AUTO_DELETE_SLAVE,   TRUE,
        TAG_END), "tmpl_samp_att1");
#endif

#if 1
    dumpitem (tmpl_samp_att2 = CreateAttachmentVA (insTemplate, sample,
        TAG_ITEM_NAME,              "template sample att 2",
        AF_TAG_NAME,                "InFIFO",
        AF_TAG_SET_FLAGS,           AF_ATTF_NOAUTOSTART,
        AF_TAG_START_AT,            5,
        AF_TAG_AUTO_DELETE_SLAVE,   TRUE,
        TAG_END), "tmpl_samp_att2");
#endif

#if 1
    dumpitem (tmpl_env_att1 = CreateAttachmentVA (insTemplate, envelope,
        TAG_ITEM_NAME,              "template env att 1",
        AF_TAG_NAME,                "Env",
        AF_TAG_SET_FLAGS,           AF_ATTF_NOAUTOSTART,
        AF_TAG_START_AT,            0,
        AF_TAG_AUTO_DELETE_SLAVE,   TRUE,
        TAG_END), "tmpl_env_att1");
#endif

#if 1
    dumpitem (tmpl_env_att2 = CreateAttachmentVA (insTemplate, envelope,
        TAG_ITEM_NAME,              "template env att 2",
        AF_TAG_NAME,                "Env",
        AF_TAG_SET_FLAGS,           AF_ATTF_NOAUTOSTART,
        AF_TAG_START_AT,            1,
/*      AF_TAG_AUTO_DELETE_SLAVE,   TRUE, */
        TAG_END), "tmpl_env_att2");
#endif

#if 1
    dumpitem (instrument3 = CreateInstrument (insTemplate, NULL), "instrument3");
    dumpitem (instrument4 = CreateInstrument (insTemplate, NULL), "instrument4");
#endif

#if 1
    dumpitem (ins3_samp_att1 = CreateAttachmentVA (instrument3, sample,
        TAG_ITEM_NAME,              "ins3 sample att 1",
        AF_TAG_NAME,                "InFIFO",
        AF_TAG_SET_FLAGS,           AF_ATTF_NOAUTOSTART,
        AF_TAG_START_AT,            6,
        AF_TAG_AUTO_DELETE_SLAVE,   TRUE,
        TAG_END), "ins3_samp_att1");
#endif

#if 0
    dumpitem (ins3_samp_att2 = CreateAttachmentVA (instrument3, sample,
        TAG_ITEM_NAME,              "ins1 sample att 2",
        AF_TAG_NAME,                "InFIFO",
        AF_TAG_SET_FLAGS,           AF_ATTF_NOAUTOSTART,
        AF_TAG_START_AT,            7,
/*      AF_TAG_AUTO_DELETE_SLAVE,   TRUE, */
        TAG_END), "ins3_samp_att2");
#endif

#if 0
    dumpitem (ins4_samp_att1 = CreateAttachmentVA (instrument4, sample,
        TAG_ITEM_NAME,              "ins2 sample att 1",
        AF_TAG_NAME,                "InFIFO",
        AF_TAG_SET_FLAGS,           AF_ATTF_NOAUTOSTART,
        AF_TAG_START_AT,            6,
/*      AF_TAG_AUTO_DELETE_SLAVE,   TRUE, */
        TAG_END), "ins4_samp_att1");
#endif

#if 0
    dumpitem (ins4_samp_att2 = CreateAttachmentVA (instrument4, sample,
        TAG_ITEM_NAME,              "ins2 sample att 2",
        AF_TAG_NAME,                "InFIFO",
        AF_TAG_SET_FLAGS,           AF_ATTF_NOAUTOSTART,
        AF_TAG_START_AT,            7,
/*      AF_TAG_AUTO_DELETE_SLAVE,   TRUE, */
        TAG_END), "ins4_samp_att2");
#endif

#if 0
    dumpitem (ins3_env_att1 = CreateAttachmentVA (instrument3, envelope,
        TAG_ITEM_NAME,              "ins1 env att 1",
        AF_TAG_NAME,                "Envx",
        AF_TAG_SET_FLAGS,           AF_ATTF_NOAUTOSTART,
        AF_TAG_START_AT,            1,
/*      AF_TAG_AUTO_DELETE_SLAVE,   TRUE, */
        TAG_END), "ins3_env_att1");
#endif

#if 0
    dumpitem (ins3_env_att2 = CreateAttachmentVA (instrument3, envelope,
        TAG_ITEM_NAME,              "ins1 env att 2",
        AF_TAG_NAME,                "Env",
        AF_TAG_SET_FLAGS,           AF_ATTF_NOAUTOSTART,
        AF_TAG_START_AT,            2,
/*      AF_TAG_AUTO_DELETE_SLAVE,   TRUE, */
        TAG_END), "ins3_env_att2");
#endif

#if 0
    if ((errcode = ins4_env_att1 = CreateAttachmentVA (instrument4, envelope,
        TAG_ITEM_NAME,              "ins2 env att 1",
        AF_TAG_NAME,                "Env",
        AF_TAG_SET_FLAGS,           AF_ATTF_NOAUTOSTART,
        AF_TAG_START_AT,            0,
/*      AF_TAG_AUTO_DELETE_SLAVE,   TRUE, */
        TAG_END), "ins4_env_att1");
#endif

#if 0
    dumpitem (ins4_env_att2 = CreateAttachmentVA (instrument4, envelope,
        TAG_ITEM_NAME,              "ins2 env att 2",
        AF_TAG_NAME,                "Env",
        AF_TAG_SET_FLAGS,           AF_ATTF_NOAUTOSTART,
        AF_TAG_START_AT,            1,
/*      AF_TAG_AUTO_DELETE_SLAVE,   TRUE, */
        TAG_END), "ins4_env_att2");
#endif

#if 0
    {
        const Err errcode = SetAudioItemInfo (tmpl_samp_att1, (TagArg *)1);

        if (errcode < 0) {
            PrintError (NULL, "modify attachment", NULL, errcode);
        }
    }
#endif

#if 0
    {
        static const TagArg tags2[] = {
            { AF_TAG_START_AT, (TagData)13 },
            TAG_END
        };

        const Err errcode = SetAudioItemInfoVA (tmpl_samp_att1,
            AF_TAG_CLEAR_FLAGS,     AF_ATTF_NOAUTOSTART,
          /*
            TAG_ITEM_NAME,          "foo.attachment",
            AF_TAG_AUTO_FREE_DATA,  FALSE,
            TAG_END);
            TAG_JUMP, (TagArg *)1);
          */
            TAG_JUMP, tags2);

        if (errcode < 0) {
            PrintError (NULL, "modify attachment", NULL, errcode);
        }
    }
#endif

#if 1
    {
        static TagArg tags2[] = {
            { AF_TAG_START_AT },
            { AF_TAG_SET_FLAGS },
          #if 0
            { AF_TAG_CLEAR_FLAGS },
          #endif
            TAG_END
        };
        static TagArg tags1[] = {
            { AF_TAG_MASTER },
            { AF_TAG_SLAVE },
            { TAG_NOP },
            { AF_TAG_NAME },
            { TAG_JUMP, tags2 },
          #if 0
            { TAG_JUMP, (TagData)1 },
            TAG_END
          #endif
        };
        Err errcode;

      #if 0
        if ((errcode = GetAudioItemInfo (tmpl_samp_att1, (TagArg *)1)) < 0)
      #endif
        if ((errcode = GetAudioItemInfo (tmpl_samp_att1, tags1)) < 0)
            PrintError (NULL, "get attachment info", NULL, errcode);
        else {
            DumpTagList (tags1, "ta_attach_mem");
        }
    }
#endif

    if (wait) {
        printf ("ta_attachment_mem(0x%x) signal=0x%x\n", CURRENTTASKITEM, sig);
        WaitSignal (sig);
    }

clean:
    if (errcode < 0) PrintError (NULL, "test attachment", NULL, errcode);

#if 0
    DeleteAttachment (tmpl_samp_att1);
    DeleteAttachment (tmpl_samp_att2);
    DeleteAttachment (tmpl_env_att1);
    DeleteAttachment (tmpl_env_att2);
    DeleteAttachment (ins3_samp_att1);
    DeleteAttachment (ins3_samp_att2);
    DeleteAttachment (ins4_samp_att1);
    DeleteAttachment (ins4_samp_att2);
    DeleteAttachment (ins3_env_att1);
    DeleteAttachment (ins3_env_att2);
    DeleteAttachment (ins4_env_att1);
    DeleteAttachment (ins4_env_att2);
    DeleteInstrument (instrument1);
    DeleteInstrument (instrument2);
    DeleteInstrument (instrument3);
    DeleteInstrument (instrument4);
    DeleteItem (sample);
    DeleteItem (envelope);
#endif
    DeleteItem (insTemplate);

    if (wait) {
        printf ("ta_attachment_mem(0x%x) signal=0x%x\n", CURRENTTASKITEM, sig);
        WaitSignal (sig);
    }

        /* quiet the compiler */
    TOUCH(insTemplate);
    TOUCH(instrument1);
    TOUCH(instrument2);
    TOUCH(instrument3);
    TOUCH(instrument4);
    TOUCH(tmpl_samp_att1);
    TOUCH(tmpl_samp_att2);
    TOUCH(tmpl_env_att1);
    TOUCH(tmpl_env_att2);
    TOUCH(ins3_samp_att1);
    TOUCH(ins3_samp_att2);
    TOUCH(ins4_samp_att1);
    TOUCH(ins4_samp_att2);
    TOUCH(ins3_env_att1);
    TOUCH(ins3_env_att2);
    TOUCH(ins4_env_att1);
    TOUCH(ins4_env_att2);
    TOUCH(sample);
    TOUCH(envelope);
    TOUCH(dumpitem);
}

static Item maketestpatch (void)
{
    PatchCmdBuilder *pb = NULL;
    Item tmpl_env;
    Item tmpl_sampler = -1;
    Item result;

    if ((result = tmpl_env = LoadInsTemplate ("envelope.dsp", NULL)) < 0) goto clean;
    if ((result = tmpl_sampler = LoadInsTemplate ("sampler_16_v1.dsp", NULL)) < 0) goto clean;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

    AddTemplateToPatch (pb, "env", tmpl_env);
    AddTemplateToPatch (pb, "sampler", tmpl_sampler);
    ExposePatchPort (pb, "Env", "env", "Env");
    ExposePatchPort (pb, "InFIFO", "sampler", "InFIFO");

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;
    result = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME,  "ta_attach_mem.patch",
        TAG_END);

clean:
    DeletePatchCmdBuilder (pb);
    UnloadInsTemplate (tmpl_sampler);
    UnloadInsTemplate (tmpl_env);
    return result;
}

static Item maketestenvelope (void)
{
    static const EnvelopeSegment envDataTemplate[] = {
        { 0.0, 1.0 },
        { 1.0, 1.0 },
        { -1.0, 1.0 },
        { 0.0, 0.0 }
    };
    #define NUMPOINTS (sizeof envDataTemplate / sizeof envDataTemplate[0])
    EnvelopeSegment *envData;
    Err errcode;
    Item envelope;

    if (!(envData = AllocMem (sizeof envDataTemplate, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE))) {
        errcode = -1;
        goto clean;
    }
    memcpy (envData, envDataTemplate, sizeof envDataTemplate);

    if ((errcode = envelope = CreateEnvelopeVA (envData, NUMPOINTS,
        TAG_ITEM_NAME,          "test.envelope",
        AF_TAG_AUTO_FREE_DATA,  TRUE,
        TAG_END)) < 0) goto clean;

    return envelope;

clean:
    FreeMem (envData, TRACKED_SIZE);
    return errcode;
}

static void dumpitem (Item att, const char *desc)
{
    if (att >= 0)
        printf ("%s: 0x%x\n", desc, att);
    else
        PrintError (NULL, "create", desc, att);
}
