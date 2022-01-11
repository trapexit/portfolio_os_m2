/* @(#) closefolio.c 96/04/23 1.2 */

#include <loader/loader3do.h>


/****************************************************************************/


Err CloseSaveGameFolio(void)
{
    return UnimportByName(FindCurrentModule(), "savegame");
}

