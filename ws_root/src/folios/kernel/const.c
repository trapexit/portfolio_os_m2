/* @(#) const.c 96/06/30 1.36 */

#include <kernel/types.h>


/* sleazy little routine to circumvent the compiler's checking of
 * "const"
 */
void CopyToConst(uint32 *dst, uint32 src)
{
    *dst = src;
}
