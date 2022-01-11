/* @(#) closefolio.c 96/04/23 1.8 */

#include <loader/loader3do.h>


/****************************************************************************/


Err CloseFontFolio(void)
{
    return UnimportByName(FindCurrentModule(), "font");
}
