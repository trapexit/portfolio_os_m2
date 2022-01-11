/* @(#) openfolio.c 96/04/23 1.2 */

#include <loader/loader3do.h>


/****************************************************************************/


Err OpenDateFolio(void)
{
    return ImportByName(FindCurrentModule(), "date");
}
