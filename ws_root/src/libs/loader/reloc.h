/* @(#) reloc.h 96/07/31 1.1 */

#ifndef __RELOC_H
#define __RELOC_H


/*****************************************************************************/


#ifndef __LOADER_LOADER3DO_H
#include <loader/loader3do.h>
#endif


/*****************************************************************************/


Err RelocateModule(LoaderInfo *li, bool minimizeRelocs);


/*****************************************************************************/


#endif /* __RELOC_H */
