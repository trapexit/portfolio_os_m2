/* @(#) animlists.h 96/09/07 1.2 */

#ifndef __ANIMLISTS_H
#define __ANIMLISTS_H


/*****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif

#ifndef __GRAPHICS_CLT_GSTATE_H
#include <graphics/clt/gstate.h>
#endif

#ifndef __ANIMNODES_H
#include "animnodes.h"
#endif


/*****************************************************************************/


typedef struct
{
    AnimNode al;
    List     al_Nest;
} AnimList;


/*****************************************************************************/


void PrepAnimList(struct StorageReq *req, AnimList *animList);
void UnprepAnimList(struct StorageReq *req, AnimList *animList);
void StepAnimList(struct StorageReq *req, AnimList *animList);
void DrawAnimList(struct StorageReq *req, AnimList *animList, GState *gs);
void DisableAnimList(struct StorageReq *req, AnimList *animList);
void EnableAnimList(struct StorageReq *req, AnimList *animList);
void InsertAnimNode(AnimList *aanimList, AnimNode *an);


/*****************************************************************************/


#endif /* __ANIMLISTS_H */
