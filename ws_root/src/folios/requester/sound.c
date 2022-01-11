/* @(#) sound.c 96/09/07 1.3 */

#include <kernel/types.h>
#include <kernel/operror.h>
#include <audio/audio.h>
#include <audio/patchfile.h>
#include "sound.h"


/*****************************************************************************/


Err LoadSounds(StorageReq *req)
{
    TOUCH(req);
    return 0;
}


/*****************************************************************************/


void UnloadSounds(StorageReq *req)
{
    TOUCH(req);
}


/*****************************************************************************/


void PlaySound(StorageReq *req, Sounds sound)
{
    TOUCH(req);
    TOUCH(sound);
}


/*****************************************************************************/


void StopSound(StorageReq *req)
{
    TOUCH(req);
}


/*****************************************************************************/


static Item internalLoadPatchTemplate (const char *fileName);

/* FIXME: remove this unused stuff */
Err Beep(void)
{
Err     result;
Item    oscIns;
Item    sleepCue;
Item    tmplt;
float32 ticksPerSecond;

	result = GetAudioClockRate(AF_GLOBAL_CLOCK, &ticksPerSecond);
	if (result >= 0)
	{
            sleepCue = result = CreateCue(NULL);
            if (sleepCue >= 0)
            {
                tmplt = result = internalLoadPatchTemplate("/remote/system.m2/requester/yes.patch");
                if (tmplt >= 0)
                {
                    oscIns = result = CreateInstrument(tmplt, NULL);
                    if (oscIns >= 0)
                    {
                        result = StartInstrument(oscIns, NULL);
                        if (result >= 0)
                        {
                            SleepUntilTime(sleepCue, GetAudioTime() + (ticksPerSecond));
                            StopInstrument(oscIns, NULL);
                        }
                    }
                    UnloadPatchTemplate(tmplt);
                }
                DeleteCue(sleepCue);
            }
        }

    return result;
}

static Item internalLoadPatchTemplate (const char *fileName)
{
    Item result;

    if ((result = OpenAudioPatchFileFolio()) >= 0) {
        result = LoadPatchTemplate (fileName);
        CloseAudioPatchFileFolio();
    }

    return result;
}
