/* @(#) closefolio.c 96/04/23 1.2 */

#include <audio/audio.h>        /* self */
#include <loader/loader3do.h>

Err CloseAudioFolio (void)
{
    return UnimportByName (FindCurrentModule(), "audio");
}
