/* @(#) daemon.h 96/10/08 1.2 */

#ifndef __DAEMON_H
#define __DAEMON_H


/*****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************/


typedef enum
{
    DAEMON_IMPORTBYNAME,
    DAEMON_UNIMPORTBYNAME,
    DAEMON_OPENMODULE,
    DAEMON_OPENMODULETREE,
    DAEMON_CLOSEMODULE
} DaemonOp;


void InitImportedModules(LoaderInfo *li, Item clientTask);
Err  AskDaemon(DaemonOp op, uint32 arg1, void *arg2);
Err  PingDaemon(void);
Err  StartLoaderDaemon(void);


/*****************************************************************************/


#endif /* __DAEMON_H */
