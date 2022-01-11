/* @(#) openfolio.c 96/04/23 1.9 */

#include <loader/loader3do.h>


/****************************************************************************/


int32 intlOpenFolio(void)
{
    return ImportByName(FindCurrentModule(), "international");
}
