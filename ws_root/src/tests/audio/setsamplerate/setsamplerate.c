/******************************************************************************
**
**  @(#) setsamplerate.c 96/06/04 1.8
**  $Id: ta_setsamplerate.c,v 1.1 1995/02/07 21:40:26 peabody Exp $
**
**  Set/Get audiofolio sample rate.
**  This program must be built in priveleged mode.
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
**  950207 WJB  Created.
**  950908 WJB  Converted to floating point.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <audio/audio.h>
#include <misc/frac16.h>
#include <stdlib.h>      /* strtof() */
#include <stdio.h>

int main (int argc, char *argv[])
{
    Err errcode;

    if ((errcode = OpenAudioFolio()) < 0) goto clean;

    {
        TagArg tags[] = {
            { AF_TAG_SAMPLE_RATE_FP },
            { AF_TAG_AMPLITUDE_FP },
            TAG_END
        };
        float32 curFreq, curAmp;

        if ((errcode = GetAudioFolioInfo (tags)) < 0) goto clean;
        curFreq = ConvertTagData_FP(tags[0].ta_Arg);
        curAmp = ConvertTagData_FP(tags[1].ta_Arg);
        printf ("Current sample rate: %f Hz\n", curFreq);
        printf ("Current amplitude: %f\n", curAmp);
    }

    if (argc > 1) {
        const float32 newFreq = strtof(argv[1], NULL);

        if ((errcode = SetAudioFolioInfoVA ( AF_TAG_SAMPLE_RATE_FP,
                                                ConvertFP_TagData(newFreq),
                                             TAG_END)) < 0) goto clean;
        printf ("New sample rate: %f Hz\n", newFreq);
    }

    if (argc > 2) {
        const float32 newAmp = strtof(argv[2], NULL);

        if ((errcode = SetAudioFolioInfoVA ( AF_TAG_AMPLITUDE_FP,
                                                ConvertFP_TagData(newAmp),
                                             TAG_END)) < 0) goto clean;
        printf ("New Amplitude: %f\n", newAmp);
    }
clean:
    if (errcode < 0) PrintError (NULL, NULL, NULL, errcode);
    CloseAudioFolio();
    return 0;
}
