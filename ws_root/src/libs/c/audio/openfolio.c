/* @(#) openfolio.c 96/04/23 1.40 */

#include <audio/audio.h>        /* self */
#include <loader/loader3do.h>

Err OpenAudioFolio (void)
{
    return ImportByName (FindCurrentModule(), "audio");
}
