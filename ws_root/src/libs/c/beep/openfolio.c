/* @(#) openfolio.c 96/01/16 1.39 */

#include <beep/beep.h>        /* self */
#include <loader/loader3do.h>

Err OpenBeepFolio (void)
{
    return ImportByName (FindCurrentModule(), "beep");
}
