/* @(#) eventloops.h 96/10/30 1.6 */

#ifndef __EVENTLOOPS_H
#define __EVENTLOOPS_H


/*****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __UI_REQUESTER_H
#include <ui/requester.h>
#endif

#ifndef __IOSERVER_H
#include "ioserver.h"
#endif

#ifndef __CONTROLS_H
#include "controls.h"
#endif

#ifndef __DIRSCAN_H
#include "dirscan.h"
#endif

/*****************************************************************************/


typedef struct
{
    MinNode ip;
    char    ip_Label[20];
    char    ip_Info[50];
} InfoPair;


/*****************************************************************************/


Err MainLoop(StorageReq *req);
Err IOLoop(StorageReq *req, const ServerPacket *packet, const Control *ctrl);
Err CopyLoop(StorageReq *req, const CopyStats *stats, const Control *copyControl);
Err DeleteLoop(StorageReq *req, const EntryNode *en, const Control *deleteControl);
Err InfoLoop(StorageReq *req, const List *infoPairs);
Err TextLoop(StorageReq *req, char *string, uint32 maxchars);


/*****************************************************************************/


#endif /* __EVENTLOOPS_H */
