/* @(#) closefolio.c 96/04/23 1.3 */

#include <audio/patchfile.h>        /* self */
#include <loader/loader3do.h>

Err CloseAudioPatchFileFolio(void)
{
    return UnimportByName(FindCurrentModule(), "audiopatchfile");
}
