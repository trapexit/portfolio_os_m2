/* @(#) animlists.c 96/09/07 1.2 */

#include <kernel/types.h>
#include "req.h"
#include "animlists.h"


/*****************************************************************************/


static const AnimMethods animListMethods =
{
    (UnprepFunc)UnprepAnimList,
    (StepFunc)StepAnimList,
    (DrawFunc)DrawAnimList,
    (DisableFunc)DisableAnimList,
    (EnableFunc)EnableAnimList
};


/*****************************************************************************/


void PrepAnimList(StorageReq *req, AnimList *animList)
{
    PrepAnimNode(req, (AnimNode *)animList, &animListMethods, sizeof(AnimList));
    PrepList(&animList->al_Nest);
}


/*****************************************************************************/


void UnprepAnimList(StorageReq *req, AnimList *animList)
{
AnimNode *an;

    while (an = (AnimNode *)RemHead(&animList->al_Nest))
    {
        if (an->an_Methods->an_UnprepFunc)
            (* an->an_Methods->an_UnprepFunc)(req, an);
    }

    UnprepAnimNode(req, (AnimNode *)animList);
}


/*****************************************************************************/


void StepAnimList(StorageReq *req, AnimList *animList)
{
AnimNode *an;

    ScanList(&animList->al_Nest, an, AnimNode)
    {
        if (an->an_Methods->am_StepFunc)
            (* an->an_Methods->am_StepFunc)(req, an);
    }
}


/*****************************************************************************/


void DrawAnimList(StorageReq *req, AnimList *animList, GState *gs)
{
AnimNode *an;

    ScanListB(&animList->al_Nest, an, AnimNode)
    {
        if (an->an_Methods->am_DrawFunc)
            (* an->an_Methods->am_DrawFunc)(req, an, gs);
    }
}


/*****************************************************************************/


void DisableAnimList(StorageReq *req, AnimList *animList)
{
AnimNode *an;

    ScanList(&animList->al_Nest, an, AnimNode)
    {
        if (an->an_Methods->am_DisableFunc)
            (* an->an_Methods->am_DisableFunc)(req, an);
    }
}


/*****************************************************************************/


void EnableAnimList(StorageReq *req, AnimList *animList)
{
AnimNode *an;

    ScanList(&animList->al_Nest, an, AnimNode)
    {
        if (an->an_Methods->am_EnableFunc)
            (* an->an_Methods->am_EnableFunc)(req, an);
    }
}


/*****************************************************************************/


void InsertAnimNode(AnimList *animList, AnimNode *an)
{
    InsertNodeFromHead(&animList->al_Nest, (Node *)an);
}
