/* @(#) openfolio.c 96/06/05 1.1 */

#include <loader/loader3do.h>


/****************************************************************************/


Err OpenBlitterFolio(void)
{
    return ImportByName(FindCurrentModule(), "blitter");
}


