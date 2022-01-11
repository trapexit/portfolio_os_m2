/* @(#) openfolio.c 96/04/23 1.2 */

#include <audio/patch.h>            /* self */
#include <loader/loader3do.h>

Err OpenAudioPatchFolio(void)
{
    return ImportByName(FindCurrentModule(), "audiopatch");
}
