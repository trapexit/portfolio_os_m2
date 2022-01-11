/* @(#) closefolio.c 96/01/16 1.1 */

#include <beep/beep.h>        /* self */
#include <loader/loader3do.h>

Err CloseBeepFolio (void)
{
    return UnimportByName (FindCurrentModule(), "beep");
}
