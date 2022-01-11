/* @(#) closefolio.c 96/04/23 1.8 */

#include <loader/loader3do.h>


/****************************************************************************/


int32 CloseJStringFolio(void)
{
    return UnimportByName(FindCurrentModule(), "jstring");
}
