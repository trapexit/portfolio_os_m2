/* @(#) ansi_ctype.c 96/03/29 1.3 */

#include <kernel/types.h>
#include <ctype.h>


/* IL (illegal) is 0, but enhances readability.  */

#define IL 0
#define _UX (__U+__X)
#define _LX (__L+__X)
#define _CS (__C+__S)           /* control and space (e.g. tab) */


static unsigned const char chartypes[] = {
        __C,                     /*   nul        */
        __C,                     /*   \001       */
        __C,                     /*   \002       */
        __C,                     /*   \003       */
        __C,                     /*   \004       */
        __C,                     /*   \005       */
        __C,                     /*   \006       */
        __C,                     /*   bell       */
        __C,                     /*   backspace  */
        __C+__S,                 /*   tab        */
        __C+__S,                 /*   newline    */
        __C+__S,                 /*   vtab       */
        __C+__S,                 /*   formfeed   */
        __C+__S,                 /*   return     */
        __C,                     /*   \016       */
        __C,                     /*   \017       */
        __C,                     /*   \020       */
        __C,                     /*   \021       */
        __C,                     /*   \022       */
        __C,                     /*   \023       */
        __C,                     /*   \024       */
        __C,                     /*   \025       */
        __C,                     /*   \026       */
        __C,                     /*   \027       */
        __C,                     /*   \030       */
        __C,                     /*   \031       */
        __C,                     /*   \032       */
        __C,                     /*   \033       */
        __C,                     /*   \034       */
        __C,                     /*   \035       */
        __C,                     /*   \036       */
        __C,                     /*   \037       */
        __B+__S,                /*   space      */
        __P,                    /*   !          */
        __P,                    /*   "          */
        __P,                    /*   #          */
        __P,                    /*   $          */
        __P,                    /*   %          */
        __P,                    /*   &          */
        __P,                    /*   '          */
        __P,                    /*   (          */
        __P,                    /*   )          */
        __P,                    /*   *          */
        __P,                    /*   +          */
        __P,                    /*   ,          */
        __P,                    /*   -          */
        __P,                    /*   .          */
        __P,                    /*   /          */
        __N,                    /*   0          */
        __N,                    /*   1          */
        __N,                    /*   2          */
        __N,                    /*   3          */
        __N,                    /*   4          */
        __N,                    /*   5          */
        __N,                    /*   6          */
        __N,                    /*   7          */
        __N,                    /*   8          */
        __N,                    /*   9          */
        __P,                    /*   :          */
        __P,                    /*   ;          */
        __P,                    /*   <          */
        __P,                    /*   =          */
        __P,                    /*   >          */
        __P,                    /*   ?          */
        __P,                    /*   @          */
        __U+__X,                /*   A          */
        __U+__X,                /*   B          */
        __U+__X,                /*   C          */
        __U+__X,                /*   D          */
        __U+__X,                /*   E          */
        __U+__X,                /*   F          */
        __U,                    /*   G          */
        __U,                    /*   H          */
        __U,                    /*   I          */
        __U,                    /*   J          */
        __U,                    /*   K          */
        __U,                    /*   L          */
        __U,                    /*   M          */
        __U,                    /*   N          */
        __U,                    /*   O          */
        __U,                    /*   P          */
        __U,                    /*   Q          */
        __U,                    /*   R          */
        __U,                    /*   S          */
        __U,                    /*   T          */
        __U,                    /*   U          */
        __U,                    /*   V          */
        __U,                    /*   W          */
        __U,                    /*   X          */
        __U,                    /*   Y          */
        __U,                    /*   Z          */
        __P,                    /*   [          */
        __P,                    /*   \          */
        __P,                    /*   ]          */
        __P,                    /*   ^          */
        __P,                    /*   _          */
        __P,                    /*   `          */
        __L+__X,                /*   a          */
        __L+__X,                /*   b          */
        __L+__X,                /*   c          */
        __L+__X,                /*   d          */
        __L+__X,                /*   e          */
        __L+__X,                /*   f          */
        __L,                    /*   g          */
        __L,                    /*   h          */
        __L,                    /*   i          */
        __L,                    /*   j          */
        __L,                    /*   k          */
        __L,                    /*   l          */
        __L,                    /*   m          */
        __L,                    /*   n          */
        __L,                    /*   o          */
        __L,                    /*   p          */
        __L,                    /*   q          */
        __L,                    /*   r          */
        __L,                    /*   s          */
        __L,                    /*   t          */
        __L,                    /*   u          */
        __L,                    /*   v          */
        __L,                    /*   w          */
        __L,                    /*   x          */
        __L,                    /*   y          */
        __L,                    /*   z          */
        __P,                    /*   {          */
        __P,                    /*   |          */
        __P,                    /*   }          */
        __P,                    /*   ~          */
        __C,                     /*   \177       */
	IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,
	IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,
	IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,
	IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,
	IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,
	IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,
	IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,
	IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,IL,
};

unsigned const char *__ctype = chartypes;


int tolower(int c)
{
    return (isupper(c) ? c + ('a' - 'A') : c);
}

int toupper(int c)
{
    return (islower(c) && c != 0xdf ? c + ('A' - 'a') : c);
}
