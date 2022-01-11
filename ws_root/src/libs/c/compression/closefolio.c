/* @(#) closefolio.c 96/04/23 1.7 */

#include <loader/loader3do.h>


/****************************************************************************/


Err CloseCompressionFolio(void)
{
    return UnimportByName(FindCurrentModule(), "compression");
}
