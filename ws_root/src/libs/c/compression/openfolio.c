/* @(#) openfolio.c 96/04/23 1.9 */

#include <loader/loader3do.h>


/****************************************************************************/


Err OpenCompressionFolio(void)
{
    return ImportByName(FindCurrentModule(), "compression");
}


