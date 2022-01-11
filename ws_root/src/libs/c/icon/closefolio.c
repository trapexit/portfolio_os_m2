/* @(#) closefolio.c 96/04/23 1.5 */

#include <loader/loader3do.h>


/****************************************************************************/


Err CloseIconFolio(void)
{
    return UnimportByName(FindCurrentModule(), "icon");
}
