/* @(#) openfolio.c 96/04/23 1.5 */

#include <loader/loader3do.h>


/****************************************************************************/


Err OpenRequesterFolio(void)
{
    return ImportByName(FindCurrentModule(), "requester");
}
