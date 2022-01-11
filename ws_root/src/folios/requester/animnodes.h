/* @(#) animnodes.h 96/09/07 1.2 */

#ifndef __ANIMNODES_H
#define __ANIMNODES_H


/*****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __GRAPHICS_CLT_GSTATE_H
#include <graphics/clt/gstate.h>
#endif

#ifndef __TRANSITIONS_H
#include "transitions.h"
#endif


/*****************************************************************************/


#define NOMINAL_PRIORITY 100

typedef Err (* UnprepFunc)(struct StorageReq *req, struct AnimNode *self);
typedef Err (* StepFunc)(struct StorageReq *req, struct AnimNode *self);
typedef Err (* DrawFunc)(struct StorageReq *req, struct AnimNode *self, GState *gs);
typedef Err (* DisableFunc)(struct StorageReq *req, struct AnimNode *self);
typedef Err (* EnableFunc)(struct StorageReq *req, struct AnimNode *self);

typedef struct
{
    UnprepFunc  an_UnprepFunc;
    StepFunc    am_StepFunc;
    DrawFunc    am_DrawFunc;
    DisableFunc am_DisableFunc;
    EnableFunc  am_EnableFunc;
} AnimMethods;

typedef struct AnimNode
{
    NamelessNode       an;
    const AnimMethods *an_Methods;
    TransitionInfo     an_DisabledLevel;
} AnimNode;

#define an_Disabled an.n_Type


/*****************************************************************************/


void PrepAnimNode(struct StorageReq *req, AnimNode *an, const AnimMethods *methods, uint32 objSize);
void UnprepAnimNode(struct StorageReq *req, AnimNode *an);
void StepAnimNode(StorageReq *req, AnimNode *an);
void DisableAnimNode(struct StorageReq *req, AnimNode *an);
void EnableAnimNode(struct StorageReq *req, AnimNode *an);


/*****************************************************************************/


#endif /* __ANIMNODES_H */
