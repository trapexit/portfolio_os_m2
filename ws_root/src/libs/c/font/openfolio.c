/* @(#) openfolio.c 96/04/23 1.8 */

#include <loader/loader3do.h>


/****************************************************************************/


Err OpenFontFolio(void)
{
    return ImportByName(FindCurrentModule(), "font");
}


