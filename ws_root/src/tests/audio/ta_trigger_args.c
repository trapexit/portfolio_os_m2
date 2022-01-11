/******************************************************************************
**
**  @(#) ta_trigger_args.c 95/12/14 1.4
**
**  Test bad args to ArmTrigger().
**
**  By: Bill Barton
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950516 WJB  Created.
**  950526 WJB  Changed for Arm/DisarmTrigger().
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <audio/audio.h>
#include <stdio.h>

Err testtrigger (void);

int main (int argc, char *argv[])
{
    Err errcode;

    TOUCH(argc);

    if ((errcode = OpenAudioFolio()) < 0) goto clean;

    errcode = testtrigger();

clean:
    if (errcode < 0) PrintError (NULL, 0, NULL, errcode);
    printf ("%s: done\n", argv[0]);
    CloseAudioFolio();
    return 0;
}


#define TEST_SUCCESS(call)                                                  \
    {                                                                       \
        const Err errcode = (call);                                         \
                                                                            \
        printf (#call ": ");                                                \
        if (errcode >= 0) {                                                 \
            printf ("PASS\n");                                              \
            npass++;                                                        \
        }                                                                   \
        else {                                                              \
            printf ("FAIL - expected success, got error $%08lx\n", errcode);\
            PrintfSysErr (errcode);                                         \
            nfail++;                                                        \
        }                                                                   \
    }

#define TEST_FAILURE(call,testerrcode)                                      \
    {                                                                       \
        const Err errcode = (call);                                         \
                                                                            \
        printf (#call ": ");                                                \
        if (errcode == (Err)(testerrcode)) {                                \
            printf ("PASS - got expected error $%08lx\n", errcode);         \
            PrintfSysErr (errcode);                                         \
            npass++;                                                        \
        }                                                                   \
        else if (errcode >= 0) {                                            \
            printf ("FAIL - expected failure, got success\n");              \
            nfail++;                                                        \
        }                                                                   \
        else {                                                              \
            printf ("FAIL - expected error $%08lx, got error $%08lx\n", testerrcode, errcode); \
            PrintfSysErr (errcode);                                         \
            nfail++;                                                        \
        }                                                                   \
    }


Err testtrigger (void)
{
    Item ins;
    Item cue1 = -1;
    Item cue2 = -1;
    Err errcode;
    int npass = 0, nfail = 0;

    if ((errcode = ins = LoadInstrument ("schmidt_trigger.dsp", 0, 100)) < 0) goto clean;
    if ((errcode = cue1 = CreateCue (NULL)) < 0) goto clean;
    if ((errcode = cue2 = CreateCue (NULL)) < 0) goto clean;

        /* succeed */
    printf ("\nSuccessful calls:\n");
    TEST_SUCCESS (ArmTrigger (ins, "Trigger", cue1, 0));
    TEST_SUCCESS (ArmTrigger (ins, NULL, cue2, 0));
    TEST_SUCCESS (ArmTrigger (ins, NULL, cue1, AF_F_TRIGGER_RESET));
    TEST_SUCCESS (ArmTrigger (ins, NULL, cue1, AF_F_TRIGGER_CONTINUOUS));
    TEST_SUCCESS (ArmTrigger (ins, NULL, cue1, AF_F_TRIGGER_CONTINUOUS | AF_F_TRIGGER_RESET));
    TEST_SUCCESS (DisarmTrigger (ins, NULL));
    TEST_SUCCESS (ArmTrigger (ins, NULL, cue1, 0));

        /* bad ins */
    printf ("\nBad Instrument:\n");
    TEST_FAILURE (ArmTrigger (cue2, NULL, cue1, 0), AF_ERR_BADITEM);
    TEST_FAILURE (ArmTrigger (0,    NULL, cue1, 0), AF_ERR_BADITEM);
    TEST_FAILURE (ArmTrigger (-1,   NULL, cue1, 0), AF_ERR_BADITEM);

        /* bad name */
    printf ("\nBad Trigger Name:\n");
    TEST_FAILURE (ArmTrigger (ins, "Trigger1", cue1, 0), AF_ERR_NAME_NOT_FOUND);
    TEST_FAILURE (ArmTrigger (ins, (void *)1, cue1, 0), AF_ERR_BADPTR);

        /* bad cue */
    printf ("\nBad Cue:\n");
    TEST_FAILURE (ArmTrigger (ins, "Trigger", ins, 0), AF_ERR_BADITEM);
    TEST_FAILURE (ArmTrigger (ins, "Trigger", -1, 0), AF_ERR_BADITEM);

        /* bad cue */
    printf ("\nBad Flags:\n");
    TEST_FAILURE (ArmTrigger (ins, "Trigger", cue1, 0x04), AF_ERR_OUTOFRANGE);
    TEST_FAILURE (ArmTrigger (ins, "Trigger", cue1, 0x80), AF_ERR_OUTOFRANGE);
    TEST_FAILURE (ArmTrigger (ins, "Trigger", cue1, 0x400), AF_ERR_OUTOFRANGE);
    TEST_FAILURE (ArmTrigger (ins, "Trigger", cue1, 0x40000), AF_ERR_OUTOFRANGE);
    TEST_FAILURE (ArmTrigger (ins, "Trigger", cue1, 0x8000000), AF_ERR_OUTOFRANGE);
    TEST_FAILURE (ArmTrigger (ins, "Trigger", cue1, 7), AF_ERR_OUTOFRANGE);

    printf ("\nSummary: %d PASS, %d FAIL\n", npass, nfail);

clean:
    DeleteCue (cue2);
    DeleteCue (cue1);
    UnloadInstrument (ins);

    return errcode;
}
