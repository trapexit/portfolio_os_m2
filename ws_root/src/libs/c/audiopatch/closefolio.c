/* @(#) closefolio.c 96/04/23 1.2 */

#include <audio/patch.h>            /* self */
#include <loader/loader3do.h>

Err CloseAudioPatchFolio(void)
{
    return UnimportByName(FindCurrentModule(), "audiopatch");
}
