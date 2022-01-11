/* @(#) closefolio.c 96/04/23 1.4 */

#include <loader/loader3do.h>


/****************************************************************************/


Err CloseFSUtilsFolio(void)
{
    return UnimportByName(FindCurrentModule(), "fsutils");
}
