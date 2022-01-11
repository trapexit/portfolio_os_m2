/******************************************************************************
**
**  @(#) dumpsiginfo.c 96/03/27 1.4
**
**  Dump signal information
**
******************************************************************************/

#include <audio/audio.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int main (void)
{
    static const char * const sigDesc[AF_SIGNAL_TYPE_MANY] = {  /* @@@ depends on AF_SIGNAL_TYPE_ order */
        "Signed",
        "Unsigned",
        "Osc Freq",
        "LFO Freq",
        "Sample Rate",
        "Whole",
    };
    Err errcode;
    int32 i;

    if ((errcode = OpenAudioFolio()) < 0) {
        PrintError (NULL, "open audio folio", NULL, errcode);
        goto clean;
    }

    printf ("Signal                Min           Max      Prec\n");
    /*       xxxxxxxxxxx  -mmmmm.ddddd  -mmmmm.ddddd   m.ddddde-xx */
    for (i=0; i<AF_SIGNAL_TYPE_MANY; i++) {
        AudioSignalInfo info;

        if ((errcode = GetAudioSignalInfo (&info, sizeof info, i)) >= 0) {
            printf ("%-11s %13.5f %13.5f ", sigDesc[i], info.sinfo_Min, info.sinfo_Max);
            printf (fabsf(info.sinfo_Precision) < 0.1 ? "%13.5e\n" : "%9.5f\n", info.sinfo_Precision);
        }
        else {
            printf ("%-11s error 0x%x\n", sigDesc[i], errcode);
        }
    }

clean:
    CloseAudioFolio();
    TOUCH(errcode);     /* silence warning for not using errcode */
    return 0;
}
