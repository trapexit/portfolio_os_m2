/* @(#) closefolio.c 96/04/23 1.5 */

#include <loader/loader3do.h>


/****************************************************************************/


Err CloseRequesterFolio(void)
{
    return UnimportByName(FindCurrentModule(), "requester");
}
