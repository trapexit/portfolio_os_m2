/* @(#) load.h 96/07/31 1.2 */

#ifndef __LOAD_H
#define __LOAD_H


/*****************************************************************************/


#ifndef __LOADER_LOADER3DO_H
#include <loader/loader3do.h>
#endif


/*****************************************************************************/


Err  LoadModule(LoaderInfo **li, const char *name, bool currentDir, bool systemDir);
void UnloadModule(LoaderInfo *li);


/*****************************************************************************/


#endif /* __LOAD_H */
