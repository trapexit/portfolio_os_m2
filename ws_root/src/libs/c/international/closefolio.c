/* @(#) closefolio.c 96/04/23 1.7 */

#include <loader/loader3do.h>


/****************************************************************************/


int32 intlCloseFolio(void)
{
    return UnimportByName(FindCurrentModule(), "international");
}
