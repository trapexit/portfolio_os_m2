/******************************************************************************
**
**  @(#) ta_trigger_recv.c 95/09/06 1.8
**
**  Receive trigger signals from an instrument created by another task.
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
**  950515 WJB  Created.
**  950526 WJB  Changed for Arm/DisarmTrigger().
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <audio/audio.h>
#include <kernel/kernel.h>
#include <kernel/time.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>         /* atoi() */
#include <string.h>

Err testtrigger (Item insitem, const char *trigname, bool reset, int trigcount);
Err oneshottest (Item trigins, const char *trigname, Item trigcue, int32 quitsig, bool reset, int trigcount);
Err continuoustest (Item trigins, const char *trigname, Item trigcue, int32 quitsig, bool reset);
void printf_trigmsg (const char *fmt, ...);

int main (int argc, char *argv[])
{
    Item trigins = 0;
    const char *trigname = NULL;
    int trigcount = 1;
    bool reset = FALSE;
    Err errcode;

    if (argc < 2) {
        printf ("usage: %s [-continuous] [-reset] [-<trigcount>] <instrument item #> [trigger name]\n", argv[0]);
        return 0;
    }

    {
        int i;
        int nargs = 0;

        for (i=1; i<argc; i++) {
            const char *arg = argv[i];

            if (arg[0] == '-') {
                if (!strcasecmp (arg, "-continuous")) trigcount = -1;
                else if (!strcasecmp (arg, "-reset")) reset = TRUE;
                else if (isdigit(arg[1])) {
                    trigcount = atoi(&arg[1]);
                    if (trigcount <= 0) {
                        printf ("%s: Trigger count '%d' out of range\n", argv[0], trigcount);
                        return 0;
                    }
                }
                else {
                    printf ("%s: Unknown switch '%s'\n", argv[0], arg);
                    return 0;
                }
            }
            else {
                switch (nargs) {
                    case 0:
                        trigins = strtoul (arg, NULL, 0);
                        break;

                    case 1:
                        trigname = arg;
                        break;

                    default:
                        printf ("%s: Too many arguments\n", argv[0]);
                        return 0;
                }
                nargs++;
            }
        }
    }

    if ((errcode = OpenAudioFolio()) < 0) goto clean;

    errcode = testtrigger (trigins, trigname, reset, trigcount);

clean:
    if (errcode < 0) PrintError (NULL, 0, NULL, errcode);
    printf ("%s: done\n", argv[0]);
    CloseAudioFolio();
    return 0;
}

Err testtrigger (Item trigins, const char *trigname, bool reset, int trigcount)
{
    int32 quitsig;
    Item trigcue = -1;
    Err errcode;

    if ((errcode = quitsig = AllocSignal (0)) <= 0) goto clean;
    if ((errcode = trigcue = CreateCue(NULL)) < 0) goto clean;

    printf_trigmsg ("quit signal = 0x%08lx\n", quitsig);

    errcode = (trigcount > 0)
        ? oneshottest (trigins, trigname, trigcue, quitsig, reset, trigcount)
        : continuoustest (trigins, trigname, trigcue, quitsig, reset);

clean:
    DeleteCue (trigcue);
    if (quitsig > 0) FreeSignal (quitsig);
    return errcode;
}

Err oneshottest (Item trigins, const char *trigname, Item trigcue, int32 quitsig, bool reset, int trigcount)
{
    const int32 cuesig = GetCueSignal (trigcue);
    const int32 waitsigs = cuesig | quitsig;
    int nrecvdtrigs = 0;
    Err errcode;

    printf_trigmsg ("Arm trigger '%s' on instrument 0x%05lx %d times with cue 0x%05lx\n", trigname ? trigname : "(default)", trigins, trigcount, trigcue);

    while (nrecvdtrigs < trigcount) {
        if ((errcode = ArmTrigger (trigins, trigname, trigcue, reset ? AF_F_TRIGGER_RESET : 0)) < 0) goto clean;
        reset = FALSE;

        {
            const int32 recvsigs = WaitSignal (waitsigs);

            if (recvsigs & quitsig) {
                printf_trigmsg ("Got quit signal\n");
                break;
            }

            if (recvsigs & cuesig) {
                nrecvdtrigs++;
                printf_trigmsg ("received cue 0x%05lx (%d)\n", trigcue, nrecvdtrigs);
            }
        }
    }

    errcode = DisarmTrigger (trigins, trigname);

clean:
    return errcode;
}

Err continuoustest (Item trigins, const char *trigname, Item trigcue, int32 quitsig, bool reset)
{
    const int32 cuesig = GetCueSignal (trigcue);
    const int32 waitsigs = cuesig | quitsig;
    int32 recvsigs;
    Err errcode;

    printf_trigmsg ("Arm trigger '%s' on instrument 0x%05lx continuously with cue 0x%05lx\n", trigname ? trigname : "(default)", trigins, trigcue);
    if ((errcode = ArmTrigger (trigins, trigname, trigcue, (reset ? AF_F_TRIGGER_RESET : 0) | AF_F_TRIGGER_CONTINUOUS)) < 0) goto clean;

    while (!((recvsigs = WaitSignal (waitsigs)) & quitsig)) {
        if (recvsigs & cuesig) printf_trigmsg ("received cue 0x%05lx\n", trigcue);
    }

    errcode = DisarmTrigger (trigins, trigname);

clean:
    return errcode;
}

void printf_trigmsg (const char *fmt, ...)
{
    {
        TimeVal time;
        int32 hour, min, sec, msec, t;

        SampleSystemTimeTV (&time);

        msec = time.tv_usec / 1000;

        t = time.tv_sec;
        sec = t % 60;
        min = (t /= 60) % 60;
        hour = t / 60;

        printf ("%d:%02d:%02d.%03d ta_trigger_recv(0x%05lx): ", hour, min, sec, msec, CURRENTTASKITEM);
    }

    {
        va_list ap;

        va_start (ap,fmt);
        vprintf (fmt, ap);
        va_end (ap);
    }
}
