/* @(#) openfolio.c 96/04/23 1.10 */

#include <loader/loader3do.h>


/****************************************************************************/


int32 OpenJStringFolio(void)
{
    return ImportByName(FindCurrentModule(), "jstring");
}
