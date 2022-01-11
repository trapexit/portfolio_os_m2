/* @(#) closefolio.c 96/06/05 1.1 */

#include <loader/loader3do.h>


/****************************************************************************/


Err CloseBlitterFolio(void)
{
    return UnimportByName(FindCurrentModule(), "blitter");
}
