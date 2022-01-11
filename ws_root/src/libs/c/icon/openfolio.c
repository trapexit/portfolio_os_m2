/* @(#) openfolio.c 96/04/23 1.7 */

#include <loader/loader3do.h>


/****************************************************************************/


Err OpenIconFolio(void)
{
    return ImportByName(FindCurrentModule(), "icon");
}
