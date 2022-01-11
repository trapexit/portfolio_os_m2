/* @(#) openfolio.c 96/04/23 1.4 */

#include <loader/loader3do.h>


/****************************************************************************/


Err OpenFSUtilsFolio(void)
{
    return ImportByName(FindCurrentModule(), "fsutils");
}
