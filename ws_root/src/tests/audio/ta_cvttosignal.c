/******************************************************************************
**
**  @(#) ta_cvttosignal.c 96/03/28 1.2
**
**  Test ConvertGenericAudioSignal()
**
******************************************************************************/

#include <audio/audio.h>
#include <stdio.h>
#include <stdlib.h>     /* strtof(), strtol() */

int main (int argc, char *argv[])
{
    Err errcode = 0;

    if (argc < 4) {
        printf ("usage: %s <signal type> <rate divide> <generic value>\n", argv[0]);
        goto clean;
    }

    if ((errcode = OpenAudioFolio()) < 0) {
        PrintError (NULL, "open audio folio", NULL, errcode);
        goto clean;
    }

    {
        const int32 signalType = strtol(argv[1], NULL, 0);
        const int32 rateDivide = strtol(argv[2], NULL, 0);
        const float32 genericValue = strtof(argv[3], NULL);

        printf ("ConvertGenericToAudioSignal (%d,%d,%f) -> %f\n",
            signalType, rateDivide, genericValue,
            ConvertGenericToAudioSignal (signalType, rateDivide, genericValue));
    }

clean:
    CloseAudioFolio();
    TOUCH(errcode);
    return 0;
}
