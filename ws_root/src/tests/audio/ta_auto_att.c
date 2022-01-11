/******************************************************************************
**
**  @(#) ta_auto_att.c 96/08/15 1.9
**
**  Automatic attachment tester
**
******************************************************************************/

#include <audio/audio.h>
#include <audio/handy_macros.h>
#include <audio/patch.h>
#include <audio/parse_aiff.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <stdio.h>
#include <string.h>

static bool TestMostThings (void);
static bool TestDefaultHooks (void);

    /* attachment verification */
static bool CheckExpectedResult (const char *testDesc, Item result, Err expectedResult);
static bool CheckAttachmentParameters (const char *testDesc, Item att, Item master, Item slave, const char *hookName, int32 startAt, uint32 attFlags);
static bool CheckAttachmentAtHook (const char *testDesc, Item att, Item master, const char *hookName);

    /* test objects */
#define SAMPLE_NUM_FRAMES   512
#define SAMPLE_FRAME_SIZE   2
#define SAMPLE_NUM_BYTES    (SAMPLE_NUM_FRAMES * SAMPLE_FRAME_SIZE)

#define ENVELOPE_NUM_FRAMES 4

static Item CreateTestEnvelope (bool fatLadySings);
static Item CreateTestSample (void);

    /* formatting */
static void FormatItem (char *buf, size_t bufSize, Item);
static void FormatString (char *buf, size_t bufSize, const char *);

    /* misc */
#define MakeUserErr(er) MakeErr(ER_USER,0,ER_SEVERE,ER_E_USER,ER_C_STND,(er))


int main (void)
{
    bool pass = TRUE;
    Err errcode;

  #ifdef MEMDEBUG
    if ((errcode = CreateMemDebug ( NULL )) < 0) goto clean;

    if ((errcode = ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
                                     MEMDEBUGF_FREE_PATTERNS |
                                     MEMDEBUGF_PAD_COOKIES |
                                     MEMDEBUGF_CHECK_ALLOC_FAILURES |
                                     MEMDEBUGF_KEEP_TASK_DATA)) < 0) goto clean;
  #endif

    if ((errcode = OpenAudioFolio()) < 0) goto clean;

    printf ("\n%s: automatic attachment creation test\n", CURRENTTASK->t.n_Name);

    if (!TestMostThings()) pass = FALSE;
    if (!TestDefaultHooks()) pass = FALSE;

    printf ("\n%s: %s\n", CURRENTTASK->t.n_Name, pass ? "PASSED" : "FAILED");

clean:
    if (errcode < 0) PrintError (NULL, NULL, NULL, errcode);

    CloseAudioFolio();

  #ifdef MEMDEBUG
    DumpMemDebugVA (
        DUMPMEMDEBUG_TAG_SUPER, TRUE,
        TAG_END);
    DeleteMemDebug();
  #endif

    return 0;
}


/* -------------------- test items and hooks */

static Item CreateTestPatch1 (void);
static bool TestCreateAttachmentGeneral (Item master, Item slave, const char *hookName, int32 startAt, uint32 attFlags, Err expectedResult);

#define TestCreateAttachmentBasic(master,slave,hookName,expected) \
    TestCreateAttachmentGeneral ((master), (slave), (hookName), 0, 0, (expected))

#define TestCreateAttachmentFlags(master,slave,hookName,attFlags,expected) \
    TestCreateAttachmentGeneral ((master), (slave), (hookName), 0, attFlags, (expected))

#define TestCreateAttachmentStartAt(master,slave,hookName,startAt,expected) \
    TestCreateAttachmentGeneral ((master), (slave), (hookName), (startAt), 0, (expected))

static bool TestCreateAttachmentFatLadySings (Item master, Item slave, const char *hookName, uint32 expectedAttFlags);

static bool TestMostThings (void)
{
    Item patch_tmpl;
    Item patch_ins;
    Item sample = -1;
    Item emptySample = -1;
    Item envelope = -1;
    Item envelopeFLS = -1;
    Item delayLine = -1;
    Item delayLineTemplate = -1;
    Item semaphore = -1;
    Err errcode;
    bool pass = FALSE;      /* initial state until set up is done */

        /* set up test items */
    if ((errcode = patch_tmpl = CreateTestPatch1()) < 0) {
        PrintError (NULL, "create TestPatch1 patch", NULL, errcode);
        goto clean;
    }
    if ((errcode = patch_ins = CreateInstrument (patch_tmpl, NULL)) < 0) {
        PrintError (NULL, "create TestPatch1 instrument", NULL, errcode);
        goto clean;
    }
    if ((errcode = sample = CreateTestSample()) < 0) {
        PrintError (NULL, "create test sample", NULL, errcode);
        goto clean;
    }
    if ((errcode = emptySample = CreateSample (NULL)) < 0) {
        PrintError (NULL, "create empty sample", NULL, errcode);
        goto clean;
    }
    if ((errcode = delayLine = CreateDelayLine (SAMPLE_NUM_BYTES, 1, TRUE)) < 0) {
        PrintError (NULL, "create delay line", NULL, errcode);
        goto clean;
    }
    if ((errcode = delayLineTemplate = CreateDelayLineTemplate (SAMPLE_NUM_BYTES, 1, TRUE)) < 0) {
        PrintError (NULL, "create delay line template", NULL, errcode);
        goto clean;
    }
    if ((errcode = envelope = CreateTestEnvelope (FALSE)) < 0) {
        PrintError (NULL, "create envelope", NULL, errcode);
        goto clean;
    }
    if ((errcode = envelopeFLS = CreateTestEnvelope (TRUE)) < 0) {
        PrintError (NULL, "create envelope", NULL, errcode);
        goto clean;
    }
    if ((errcode = semaphore = CreateSemaphore ("TestCreateAttachmentGeneral", 0)) < 0) {
        PrintError (NULL, "create semaphore", NULL, errcode);
        goto clean;
    }

        /* set up complete. from here on, pass unless something fails */
    pass = TRUE;

        /* Test slave and hook combinations */
    {
        Item masterItems[2];
        Item slaveItems[4];
        const char * const hookNames[5] = {
            "InFIFO",
            "OutFIFO",
            "Env1",
            "FooKnob",
            "Dumbo",
        };
        static const Err resultMatrix[2][4][5] = {
            /* template */
            {
            /*    InFIFO                 OutFIFO                Env1                   FooKnob                Dumbo */
                { 0,                     AF_ERR_SECURITY,       AF_ERR_NAME_NOT_FOUND, AF_ERR_BAD_PORT_TYPE,  AF_ERR_NAME_NOT_FOUND },  /* sample */
                { 0,                     0,                     AF_ERR_NAME_NOT_FOUND, AF_ERR_BAD_PORT_TYPE,  AF_ERR_NAME_NOT_FOUND },  /* delay line */
                { 0,                     0,                     AF_ERR_NAME_NOT_FOUND, AF_ERR_BAD_PORT_TYPE,  AF_ERR_NAME_NOT_FOUND },  /* delay line template */
                { AF_ERR_NAME_NOT_FOUND, AF_ERR_NAME_NOT_FOUND, 0,                     AF_ERR_NAME_NOT_FOUND, AF_ERR_NAME_NOT_FOUND },  /* envelope */
            },

            /* instrument */
            {
            /*    InFIFO                 OutFIFO                Env1                   FooKnob                Dumbo */
                { 0,                     AF_ERR_SECURITY,       AF_ERR_NAME_NOT_FOUND, AF_ERR_BAD_PORT_TYPE,  AF_ERR_NAME_NOT_FOUND },  /* sample */
                { 0,                     0,                     AF_ERR_NAME_NOT_FOUND, AF_ERR_BAD_PORT_TYPE,  AF_ERR_NAME_NOT_FOUND },  /* delay line */
                { AF_ERR_BADITEM,        AF_ERR_BADITEM,        AF_ERR_BADITEM,        AF_ERR_BADITEM,        AF_ERR_BADITEM },         /* delay line template (!!! might change, depends on order of checks) */
                { AF_ERR_NAME_NOT_FOUND, AF_ERR_NAME_NOT_FOUND, 0,                     AF_ERR_NAME_NOT_FOUND, AF_ERR_NAME_NOT_FOUND },  /* envelope */
            },
        };
        int iMaster, iSlave, iHook;

        {
            char itemDesc[64];

            printf ("\n------- Test slave and hook combinations\n");

            FormatItem (itemDesc, sizeof itemDesc, patch_tmpl);
            printf ("template:        %s\n", itemDesc);
            FormatItem (itemDesc, sizeof itemDesc, patch_ins);
            printf ("instrument:      %s\n", itemDesc);

            FormatItem (itemDesc, sizeof itemDesc, sample);
            printf ("sample:          %s\n", itemDesc);
            FormatItem (itemDesc, sizeof itemDesc, delayLine);
            printf ("delay line:      %s\n", itemDesc);
            FormatItem (itemDesc, sizeof itemDesc, delayLineTemplate);
            printf ("delay line tmpl: %s\n", itemDesc);
            FormatItem (itemDesc, sizeof itemDesc, envelope);
            printf ("envelope:        %s\n", itemDesc);

            printf ("\n");
        }

        masterItems[0] = patch_tmpl;
        masterItems[1] = patch_ins;
        slaveItems[0] = sample;
        slaveItems[1] = delayLine;
        slaveItems[2] = delayLineTemplate;
        slaveItems[3] = envelope;

        for (iMaster = 0; iMaster < sizeof masterItems / sizeof masterItems[0]; iMaster++) {
            for (iSlave = 0; iSlave < sizeof slaveItems / sizeof slaveItems[0]; iSlave++) {
                for (iHook = 0; iHook < sizeof hookNames / sizeof hookNames[0]; iHook++) {
                    if (!TestCreateAttachmentBasic (masterItems[iMaster], slaveItems[iSlave], hookNames[iHook], resultMatrix[iMaster][iSlave][iHook])) pass = FALSE;
                }
            }
        }
    }

        /* Test invalid items and hook name pointers */
    {
        printf ("\n------- Test invalid items and hook name pointers\n\n");

            /* bad master item */
        if (!TestCreateAttachmentBasic (sample,     sample, "InFIFO", AF_ERR_BADITEM)) pass = FALSE;
        if (!TestCreateAttachmentBasic (semaphore,  sample, "InFIFO", AF_ERR_BADITEM)) pass = FALSE;
        if (!TestCreateAttachmentBasic ((Item)0,    sample, "InFIFO", AF_ERR_BADITEM)) pass = FALSE;
        if (!TestCreateAttachmentBasic ((Item)-128, sample, "InFIFO", AF_ERR_BADITEM)) pass = FALSE;

            /* bad slave item */
        if (!TestCreateAttachmentBasic (patch_tmpl, patch_ins,  "InFIFO", AF_ERR_BADITEM)) pass = FALSE;
        if (!TestCreateAttachmentBasic (patch_tmpl, semaphore,  "InFIFO", AF_ERR_BADITEM)) pass = FALSE;
        if (!TestCreateAttachmentBasic (patch_tmpl, (Item)0,    "InFIFO", AF_ERR_BADITEM)) pass = FALSE;
        if (!TestCreateAttachmentBasic (patch_tmpl, (Item)-129, "InFIFO", AF_ERR_BADITEM)) pass = FALSE;

            /* bad hook name pointer */
        if (!TestCreateAttachmentBasic (patch_tmpl, sample, (char *)1, AF_ERR_BADPTR)) pass = FALSE;
    }

        /* test attachment flags */
    {
        int i;

        printf ("\n------- Test attachment flags\n\n");

        for (i=0; i<32; i++) {
            const uint32 testFlag = ((uint32)1) << i;

            if (!TestCreateAttachmentFlags (patch_tmpl, sample, "InFIFO", testFlag, (testFlag & AF_ATTF_LEGALFLAGS) ? 0 : AF_ERR_BADTAGVAL)) pass = FALSE;
        }
    }

        /*
            test start at bounds

            start at validation is common for both templates and instruments,
            so only need to test against one
        */
    {
        printf ("\n------- Test 'start at' bounds\n\n");

        if (!TestCreateAttachmentStartAt (patch_tmpl, sample,            "InFIFO", -1,                  AF_ERR_BADTAGVAL)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, sample,            "InFIFO", 0,                   0)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, sample,            "InFIFO", SAMPLE_NUM_FRAMES/2, 0)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, sample,            "InFIFO", SAMPLE_NUM_FRAMES-1, 0)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, sample,            "InFIFO", SAMPLE_NUM_FRAMES,   AF_ERR_BADTAGVAL)) pass = FALSE;

        if (!TestCreateAttachmentStartAt (patch_tmpl, emptySample,       "InFIFO", -1,                  AF_ERR_BADTAGVAL)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, emptySample,       "InFIFO", 0,                   0)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, emptySample,       "InFIFO", SAMPLE_NUM_FRAMES/2, 0)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, emptySample,       "InFIFO", SAMPLE_NUM_FRAMES-1, 0)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, emptySample,       "InFIFO", SAMPLE_NUM_FRAMES,   0)) pass = FALSE;

        if (!TestCreateAttachmentStartAt (patch_tmpl, delayLine,         "InFIFO", -1,                  AF_ERR_BADTAGVAL)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, delayLine,         "InFIFO", 0,                   0)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, delayLine,         "InFIFO", SAMPLE_NUM_FRAMES/2, 0)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, delayLine,         "InFIFO", SAMPLE_NUM_FRAMES-1, 0)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, delayLine,         "InFIFO", SAMPLE_NUM_FRAMES,   AF_ERR_BADTAGVAL)) pass = FALSE;

        if (!TestCreateAttachmentStartAt (patch_tmpl, delayLineTemplate, "InFIFO", -1,                  AF_ERR_BADTAGVAL)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, delayLineTemplate, "InFIFO", 0,                   0)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, delayLineTemplate, "InFIFO", SAMPLE_NUM_FRAMES/2, 0)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, delayLineTemplate, "InFIFO", SAMPLE_NUM_FRAMES-1, 0)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, delayLineTemplate, "InFIFO", SAMPLE_NUM_FRAMES,   AF_ERR_BADTAGVAL)) pass = FALSE;

        if (!TestCreateAttachmentStartAt (patch_tmpl, envelope,          "Env1", -1,                    AF_ERR_BADTAGVAL)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, envelope,          "Env1", 0,                     0)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, envelope,          "Env1", ENVELOPE_NUM_FRAMES/2, 0)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, envelope,          "Env1", ENVELOPE_NUM_FRAMES-1, 0)) pass = FALSE;
        if (!TestCreateAttachmentStartAt (patch_tmpl, envelope,          "Env1", ENVELOPE_NUM_FRAMES,   AF_ERR_BADTAGVAL)) pass = FALSE;
    }

        /* test AF_ENVF_FATLADYSINGS propagation */
    {
        {
            char itemDesc[64];

            printf ("\n------- Test AF_ENVF_FATLADYSINGS propagation\n");

            FormatItem (itemDesc, sizeof itemDesc, patch_tmpl);
            printf ("template:   %s\n", itemDesc);
            FormatItem (itemDesc, sizeof itemDesc, patch_ins);
            printf ("instrument: %s\n", itemDesc);
            FormatItem (itemDesc, sizeof itemDesc, envelopeFLS);
            printf ("envelope:   %s\n", itemDesc);
            printf ("\n");
        }

        if (!TestCreateAttachmentFatLadySings (patch_tmpl, envelopeFLS, "Env1", AF_ATTF_FATLADYSINGS)) pass = FALSE;
        if (!TestCreateAttachmentFatLadySings (patch_ins,  envelopeFLS, "Env1", AF_ATTF_FATLADYSINGS)) pass = FALSE;
    }

clean:
    DeleteItem (patch_tmpl);
    DeleteItem (sample);
    DeleteItem (emptySample);
    DeleteItem (delayLine);
    DeleteItem (delayLineTemplate);
    DeleteItem (envelope);
    DeleteItem (envelopeFLS);
    DeleteItem (semaphore);
    TOUCH(errcode);
    return pass;
}


/*
    Patch for testing most things:
        - 2 envelope hooks (no default)
        - 2 FIFOs (1 in, 1 out)
        - 1 knob
        - 1 output
*/
static Item CreateTestPatch1 (void)
{
    const Item tmpl_sampler = LoadInsTemplate ("sampler_16_v1.dsp", NULL);
    const Item tmpl_delay = LoadInsTemplate ("delay_f1.dsp", NULL);
    const Item tmpl_envelope = LoadInsTemplate ("envelope.dsp", NULL);
    PatchCmdBuilder *pb = NULL;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

    AddTemplateToPatch (pb, "env1", tmpl_envelope);
    AddTemplateToPatch (pb, "env2", tmpl_envelope);
    AddTemplateToPatch (pb, "samp", tmpl_sampler);
    AddTemplateToPatch (pb, "delay", tmpl_delay);

    DefinePatchKnob (pb, "FooKnob", 1, AF_SIGNAL_TYPE_GENERIC_SIGNED, 0);
    DefinePatchPort (pb, "Output", 1, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);
    ConnectPatchPorts (pb, NULL, "FooKnob", 0, NULL, "Output", 0);

    ExposePatchPort (pb, "Env1", "env1", "Env");
    ExposePatchPort (pb, "Env2", "env2", "Env");
    ExposePatchPort (pb, "InFIFO", "samp", "InFIFO");
    ExposePatchPort (pb, "OutFIFO", "delay", "OutFIFO");

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

    if ((result = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME, "TestPatch1",
        TAG_END)) < 0) goto clean;

clean:
    DeletePatchCmdBuilder (pb);
    UnloadInsTemplate (tmpl_sampler);
    UnloadInsTemplate (tmpl_delay);
    UnloadInsTemplate (tmpl_envelope);

    return result;
}


/*
    General purpose create and validate attachment

    Arguments
        master
            Master Item

        slave
            Slave Item

        hookName
            Hook name

        startAt
            Start At value

        attFlags
            Attachment flags

        expectedResult
            >=0 if success is expected. Expected error code if failure is expected.

    Results
        TRUE if test passed; FALSE if test failed.
*/
static bool TestCreateAttachmentGeneral (Item master, Item slave, const char *hookName, int32 startAt, uint32 attFlags, Err expectedResult)
{
    const Item att = CreateAttachmentVA (master, slave,
                        AF_TAG_NAME,      hookName,
                        AF_TAG_START_AT,  startAt,
                        AF_TAG_SET_FLAGS, attFlags,
                        TAG_END);
    char testDesc[256];
    bool pass = TRUE;

    {
        char hookDesc[64];

        FormatString (hookDesc, sizeof hookDesc, hookName);
        sprintf (testDesc, "master=0x%x slave=0x%x hook=%s startat=%d flags=0x%x", master, slave, hookDesc, startAt, attFlags);
    }

        /* did we get the result we expected? */
    if (!CheckExpectedResult (testDesc, att, expectedResult)) pass = FALSE;

        /* check the attachment that was created (even if it wasn't supposed to be created) */
    if (att >= 0) {
            /* does it match the parameters? */
        if (!CheckAttachmentParameters (testDesc, att, master, slave, hookName, startAt, attFlags)) pass = FALSE;
    }

    DeleteItem (att);
    return pass;
}


/*
    Test attachment creation for propagation of AF_ENVF_FATLADYSINGS -> AF_ATTF_FATLADYSINGS

    Arguments
        master
            Master Item

        slave
            Slave Item

        hookName
            Hook name

        expectedAttFlags
            Expected attachment flags value to compare attachment against.

    Results
        TRUE if test passed; FALSE if test failed.
*/
static bool TestCreateAttachmentFatLadySings (Item master, Item slave, const char *hookName, uint32 expectedAttFlags)
{
    const Item att = CreateAttachmentVA (master, slave,
                        AF_TAG_NAME,      hookName,
                        TAG_END);
    char testDesc[256];
    bool pass = TRUE;

    {
        char hookDesc[64];

        FormatString (hookDesc, sizeof hookDesc, hookName);
        sprintf (testDesc, "master=0x%x slave=0x%x hook=%s expected flags=0x%x", master, slave, hookDesc, expectedAttFlags);
    }

        /* did we get the result we expected? */
    if (!CheckExpectedResult (testDesc, att, 0)) pass = FALSE;

        /* check the attachment that was created (even if it wasn't supposed to be created) */
    if (att >= 0) {
            /* does it match the parameters? */
        if (!CheckAttachmentParameters (testDesc, att, master, slave, hookName, 0, expectedAttFlags)) pass = FALSE;
    }

    DeleteItem (att);
    return pass;
}


/* -------------------- test default hooks */

static bool TestDefaultHook (Item (*patchfn)(void), const char *patchDesc, Item slave, const char *hookName, Err expectedResult);

static Item CreateDefaultFIFOPatch0 (void);
static Item CreateDefaultFIFOPatch1In (void);
static Item CreateDefaultFIFOPatch1Out (void);
static Item CreateDefaultFIFOPatch2 (void);
static Item CreateDefaultEnvHookPatch0 (void);
static Item CreateDefaultEnvHookPatch1 (void);

static bool TestCreateAttachmentDefaultHooks (Item master, Item slave, const char *hookName, Err expectedResult);

static bool TestDefaultHooks (void)
{
    Item delayLine;
    Item envelope = -1;
    Err errcode;
    bool pass = FALSE;      /* initial state until set up is done */

    if ((errcode = delayLine = CreateDelayLine (SAMPLE_NUM_BYTES, 1, TRUE)) < 0) {
        PrintError (NULL, "create delay line", NULL, errcode);
        goto clean;
    }
    if ((errcode = envelope = CreateTestEnvelope(FALSE)) < 0) {
        PrintError (NULL, "create envelope", NULL, errcode);
        goto clean;
    }

        /* set up complete. from here on, pass unless something fails */
    pass = TRUE;

    {
        char itemDesc[64];

        printf ("\n------- Test default hooks\n");

        FormatItem (itemDesc, sizeof itemDesc, delayLine);
        printf ("delay line: %s\n", itemDesc);
        FormatItem (itemDesc, sizeof itemDesc, envelope);
        printf ("envelope:   %s\n", itemDesc);
        printf ("\n");
    }

        /* test matrix */
    if (!TestDefaultHook (CreateDefaultFIFOPatch0,    "DefaultFIFOPatch0",    delayLine, NULL,      AF_ERR_BAD_NAME)) pass = FALSE;
    if (!TestDefaultHook (CreateDefaultFIFOPatch1In,  "DefaultFIFOPatch1In",  delayLine, "InFIFO",  0)) pass = FALSE;
    if (!TestDefaultHook (CreateDefaultFIFOPatch1Out, "DefaultFIFOPatch1Out", delayLine, "OutFIFO", 0)) pass = FALSE;
    if (!TestDefaultHook (CreateDefaultFIFOPatch2,    "DefaultFIFOPatch2",    delayLine, NULL,      AF_ERR_BAD_NAME)) pass = FALSE;
    if (!TestDefaultHook (CreateDefaultEnvHookPatch0, "DefaultEnvHookPatch0", envelope,  NULL,      AF_ERR_NAME_NOT_FOUND)) pass = FALSE;
    if (!TestDefaultHook (CreateDefaultEnvHookPatch1, "DefaultEnvHookPatch1", envelope,  "Env",     0)) pass = FALSE;

clean:
    DeleteItem (delayLine);
    DeleteItem (envelope);
    return pass;
}

/*
    Perform attachment test on a patch created by the specified function.

    Arguments
        patchfn
            Function to call to create patch template.

        patchDesc
            Name of patch (for error generation)

        slave
            Slave Item to attach

        hookName
            Actual name of default hook, or NULL if there isn't a default hook.

        expectedResult
            >=0 if success is expected or negative error code expected.
*/
static bool TestDefaultHook (Item (*patchfn)(void), const char *patchDesc, Item slave, const char *hookName, Err expectedResult)
{
    Item patch_tmpl;
    Item patch_ins;
    Err errcode;
    bool pass = FALSE;

        /* set up */
    if ((errcode = patch_tmpl = patchfn()) < 0) {
        PrintError (NULL, "create test patch", patchDesc, errcode);
        goto clean;
    }
    if ((errcode = patch_ins = CreateInstrument (patch_tmpl, NULL)) < 0) {
        PrintError (NULL, "create test instrument", patchDesc, errcode);
        goto clean;
    }

        /* set up done */
    pass = TRUE;

        /* test attachments */
    if (!TestCreateAttachmentDefaultHooks (patch_tmpl, slave, hookName, expectedResult)) pass = FALSE;
    if (!TestCreateAttachmentDefaultHooks (patch_ins, slave, hookName, expectedResult)) pass = FALSE;

clean:
    DeleteItem (patch_tmpl);
    return pass;
}

/*
    Default FIFO Test patch: 0 FIFOs
*/
static Item CreateDefaultFIFOPatch0 (void)
{
    PatchCmdBuilder *pb = NULL;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

    if ((result = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME, "DefaultFIFOPatch0",
        TAG_END)) < 0) goto clean;

clean:
    DeletePatchCmdBuilder (pb);

    return result;
}

/*
    Default FIFO Test patch: 1 Out FIFO
*/
static Item CreateDefaultFIFOPatch1Out (void)
{
    const Item tmpl_delay = LoadInsTemplate ("delay_f1.dsp", NULL);
    PatchCmdBuilder *pb = NULL;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

    AddTemplateToPatch (pb, "delay", tmpl_delay);

    ExposePatchPort (pb, "OutFIFO", "delay", "OutFIFO");

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

    if ((result = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME, "DefaultFIFOPatch1Out",
        TAG_END)) < 0) goto clean;

clean:
    DeletePatchCmdBuilder (pb);
    UnloadInsTemplate (tmpl_delay);

    return result;
}

/*
    Default FIFO Test patch: 1 in FIFO
*/
static Item CreateDefaultFIFOPatch1In (void)
{
    const Item tmpl_sampler = LoadInsTemplate ("sampler_16_v1.dsp", NULL);
    PatchCmdBuilder *pb = NULL;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

    AddTemplateToPatch (pb, "samp", tmpl_sampler);

    ExposePatchPort (pb, "InFIFO", "samp", "InFIFO");

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

    if ((result = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME, "DefaultFIFOPatch1In",
        TAG_END)) < 0) goto clean;

clean:
    DeletePatchCmdBuilder (pb);
    UnloadInsTemplate (tmpl_sampler);

    return result;
}

/*
    Default FIFO Test patch: 2 FIFOs
*/
static Item CreateDefaultFIFOPatch2 (void)
{
    const Item tmpl_sampler = LoadInsTemplate ("sampler_16_v1.dsp", NULL);
    const Item tmpl_delay = LoadInsTemplate ("delay_f1.dsp", NULL);
    PatchCmdBuilder *pb = NULL;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

    AddTemplateToPatch (pb, "samp", tmpl_sampler);
    AddTemplateToPatch (pb, "delay", tmpl_delay);

    ExposePatchPort (pb, "InFIFO", "samp", "InFIFO");
    ExposePatchPort (pb, "OutFIFO", "delay", "OutFIFO");

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

    if ((result = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME, "DefaultFIFOPatch2",
        TAG_END)) < 0) goto clean;

clean:
    DeletePatchCmdBuilder (pb);
    UnloadInsTemplate (tmpl_sampler);
    UnloadInsTemplate (tmpl_delay);

    return result;
}


/*
    Default envelope hook test patch: Env1, Env2 (no default)
*/
static Item CreateDefaultEnvHookPatch0 (void)
{
    const Item tmpl_envelope = LoadInsTemplate ("envelope.dsp", NULL);
    PatchCmdBuilder *pb = NULL;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

    AddTemplateToPatch (pb, "env1", tmpl_envelope);
    AddTemplateToPatch (pb, "env2", tmpl_envelope);

    ExposePatchPort (pb, "Env1", "env1", "Env");
    ExposePatchPort (pb, "Env2", "env2", "Env");

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

    if ((result = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME, "DefaultEnvHookPatch0",
        TAG_END)) < 0) goto clean;

clean:
    DeletePatchCmdBuilder (pb);
    UnloadInsTemplate (tmpl_envelope);

    return result;
}

/*
    Default envelope hook test patch: Env1, Env (default is 2nd)
*/
static Item CreateDefaultEnvHookPatch1 (void)
{
    const Item tmpl_envelope = LoadInsTemplate ("envelope.dsp", NULL);
    PatchCmdBuilder *pb = NULL;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

    AddTemplateToPatch (pb, "env1", tmpl_envelope);
    AddTemplateToPatch (pb, "env2", tmpl_envelope);

    ExposePatchPort (pb, "Env1", "env1", "Env");
    ExposePatchPort (pb, "Env", "env2", "Env");

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

    if ((result = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME, "DefaultEnvHookPatch1",
        TAG_END)) < 0) goto clean;

clean:
    DeletePatchCmdBuilder (pb);
    UnloadInsTemplate (tmpl_envelope);

    return result;
}


/*
    Create and validate attachment

    Arguments
        master
            Master Item

        slave
            Slave Item

        hookName
            The actual name of the default hook. NULL if there is no default hook.

        expectedResult
            >=0 if success is expected. Expected error code if failure is expected.

    Results
        TRUE if test passed; FALSE if test failed.
*/
static bool TestCreateAttachmentDefaultHooks (Item master, Item slave, const char *hookName, Err expectedResult)
{
    const Item att = CreateAttachment (master, slave, NULL);
    char testDesc[256];
    bool pass = TRUE;

    {
        char hookDesc[64];

        FormatString (hookDesc, sizeof hookDesc, hookName);
        sprintf (testDesc, "master=0x%x slave=0x%x default hook=%s", master, slave, hookDesc);
    }

        /* did we get the result we expected? */
    if (!CheckExpectedResult (testDesc, att, expectedResult)) pass = FALSE;

        /* check the attachment that was created (even if it wasn't supposed to be created) */
    if (att >= 0) {
            /* does it match the parameters? */
        if (!CheckAttachmentParameters (testDesc, att, master, slave, NULL, 0, 0)) pass = FALSE;

            /* if there is in fact a default hook, is the attachment actually there? */
        if (hookName) {
            if (!CheckAttachmentAtHook (testDesc, att, master, hookName)) pass = FALSE;
        }
    }

    DeleteItem (att);
    return pass;
}


/* -------------------- Attachment verification */

static bool CompareHookNames (const char *name1, const char *name2);
static bool IsItemInArray (const Item *items, int32 numItems, Item matchItem);

/*
    Check the result against the expected result.

    Arguments
        testDesc
            String containting description of test. Printed only when test fails.

        result
            Result of operation to be tested

        expectedResult
            0 if operation was expected to succeed. Specific error code of failure
            if operation was expected to fail.

    Results
        TRUE if expectations were met (both result and expectedResult are >= 0 OR
        result == expectedResult); FALSE otherwise.
*/
static bool CheckExpectedResult (const char *testDesc, Item result, Err expectedResult)
{
        /* did we get something other than expected results? */
    if (expectedResult >= 0 ? result >= 0 : result == expectedResult) return TRUE;

    printf ("### FAIL: %s\n", testDesc);
    if (expectedResult >= 0) {
        printf ("###       Expected success\n");
    }
    else {
        printf ("###       Expected error code 0x%x - ", expectedResult);
        PrintfSysErr (expectedResult);
    }
    if (result >= 0) {
        printf ("###       Got success 0x%x\n", result);
    }
    else {
        printf ("###       Got error code 0x%x - ", result);
        PrintfSysErr (result);
    }
    printf ("\n");

    return FALSE;
}

/*
    Check attachment parameters.

    Arguments
        testDesc
            String containting description of test. Printed only when test fails.

        att
            Attachment Item to test. Assumed to be valid.

        master, slave, hookName, startAt, attFlags
            Attachment parameters to test. hookName is considered equal if both are NULL
            or the strings match case insensitively.

    Results
        TRUE if parameters match; FALSE otherwise.
*/
static bool CheckAttachmentParameters (const char *testDesc, Item att, Item master, Item slave, const char *hookName, int32 startAt, uint32 attFlags)
{
    TagArg tags[] = {
        { AF_TAG_MASTER },
        { AF_TAG_SLAVE },
        { AF_TAG_NAME },
        { AF_TAG_START_AT },
        { AF_TAG_SET_FLAGS },
        TAG_END
    };
    Err errcode;

    if ((errcode = GetAudioItemInfo (att, tags)) < 0) {
        printf ("### FAIL: %s\n", testDesc);
        printf ("###       Unable to get attachment info\n");
        printf ("###       "); PrintfSysErr (errcode);
        printf ("\n");
        return FALSE;
    }

    if (
        master != (Item)tags[0].ta_Arg ||
        slave != (Item)tags[1].ta_Arg ||
        !CompareHookNames (hookName, (char *)tags[2].ta_Arg) ||
        startAt != (int32)tags[3].ta_Arg ||
        attFlags != (uint32)tags[4].ta_Arg)
    {
        char hookDesc[64];

        FormatString (hookDesc, sizeof hookDesc, (char *)tags[2].ta_Arg);

        printf ("### FAIL: %s\n", testDesc);
        printf ("###       Attachment doesn't match request. Got:\n");
        printf ("###       master=0x%x slave=0x%x hook=%s startat=%d flags=0x%x\n\n", master, slave, hookDesc, (int32)tags[3].ta_Arg, (uint32)tags[4].ta_Arg);
        return FALSE;
    }

    return TRUE;
}

/*
    Compare hook names.

    They are considered to match if both pointers are the same, or the strings match
    case insensitively.

    Arguments
        name1, name2

    Results
        TRUE if match, FALSE otherwise.
*/
static bool CompareHookNames (const char *name1, const char *name2)
{
    return
        name1 == name2 ||
        IsMemReadable (name1, 1) && IsMemReadable (name2, 1) && !strcasecmp (name1, name2);
}

/*
    Make sure attachment is at the specified hook. Useful for checking the
    results of attaching to a default hook.

    Arguments
        testDesc
            String containting description of test. Printed only when test fails.

        att
            Attachment Item to test. Assumed to be valid.

        master, hookName
            Master Item and name of hook at which to search for att. hookName must
            not be NULL.

    Results
        TRUE if found; FALSE otherwise.
*/
static bool CheckAttachmentAtHook (const char *testDesc, Item att, Item master, const char *hookName)
{
    Item foundAtts[32];     /* @@@ this is a magic number, but shouldn't be a problem,
                            ** unless the attachments are allowed to accumulate. */
    int32 numFoundAtts;
    Err errcode;

    if ((errcode = numFoundAtts = GetAttachments (foundAtts, sizeof foundAtts / sizeof foundAtts[0], master, hookName)) < 0) {
        printf ("### FAIL: %s\n", testDesc);
        printf ("###       Unable to get attachment list\n");
        printf ("###       "); PrintfSysErr (errcode);
        printf ("\n\n");
        return FALSE;
    }

    if (!IsItemInArray (foundAtts, MIN (numFoundAtts, sizeof foundAtts / sizeof foundAtts[0]), att)) {
        printf ("### FAIL: %s\n", testDesc);
        printf ("###       Attachment not in hook's attachment list\n\n");
        return FALSE;
    }

    return TRUE;
}


/*
    Determine whether an Item is in an array of items.

    Arguments
        items
            Array of Items to search

        numItems
            Number of elements in Item array.

        matchItem
            Item number to match.

    Results
        TRUE if matchItem is in items, FALSE otherwise.
*/
static bool IsItemInArray (const Item *items, int32 numItems, Item matchItem)
{
    int32 i;

    for (i=0; i<numItems; i++) {
        if (items[i] == matchItem) return TRUE;
    }
    return FALSE;
}


/* -------------------- Test objects */

static Item CreateTestEnvelope (bool fatLadySings)
{
    static const EnvelopeSegment envpoints[ENVELOPE_NUM_FRAMES] = {
        { 0.0000, 0.010 },
        { 0.9999, 0.020 },  /* attack */
        { 0.5000, 0.220 },  /* decay, sustain */
        { 0.0000, 0.000 },  /* release */
    };

    return CreateEnvelopeVA (envpoints, ENVELOPE_NUM_FRAMES,
        TAG_ITEM_NAME,       "AttachmentTest",
        AF_TAG_SUSTAINBEGIN, 2,
        AF_TAG_SUSTAINEND,   2,
        AF_TAG_SET_FLAGS,    fatLadySings ? AF_ENVF_FATLADYSINGS : 0,
        TAG_END);
}

static Item CreateTestSample (void)
{
    void *data;
    Item sample = -1;
    Err errcode;

    if (!(data = AllocMem (SAMPLE_NUM_BYTES, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE))) {
        errcode = MakeUserErr(ER_NoMem);
        goto clean;
    }

    if ((errcode = sample = CreateSampleVA (
        TAG_ITEM_NAME,         "AttachmentTest",
        AF_TAG_ADDRESS,        data,
        AF_TAG_NUMBYTES,       SAMPLE_NUM_BYTES,
        AF_TAG_AUTO_FREE_DATA, TRUE,
        TAG_END)) < 0) goto clean;

    data = NULL;
    TOUCH(data);

    return sample;

clean:
    DeleteItem (sample);
    FreeMem (data, TRACKED_SIZE);
    return errcode;
}


/* -------------------- Misc formatting */

static void FormatItem (char *buf, size_t bufSize, Item item)
{
    const ItemNode * const n = LookupItem (item);

    TOUCH(bufSize);     /* !!! really need an snprintf() */

    buf += sprintf (buf, "0x%x (", item);
    if (!n) {
        buf += sprintf (buf, "bad item");
    }
    else {
        if (n->n_SubsysType != AUDIONODE) {
            buf += sprintf (buf, "non-audio item");
        }
        else {
            buf += sprintf (buf, "type %d", n->n_Type);
        }

        if (n->n_Name) {
            buf += sprintf (buf, ", '%s'", n->n_Name);
        }
    }
    sprintf (buf, ")");
}

static void FormatString (char *buf, size_t bufSize, const char *s)
{
    TOUCH(bufSize);     /* !!! really need an snprintf() */

    if (s == NULL) {
        sprintf (buf, "NULL");
    }
    else if (IsMemReadable (s,1)) {
        sprintf (buf, "'%s'", s);
    }
    else {
        sprintf (buf, "0x%x (bad pointer)", s);
    }
}
