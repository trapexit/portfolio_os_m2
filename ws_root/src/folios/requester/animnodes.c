/* @(#) animnodes.c 96/09/07 1.2 */

#include <kernel/types.h>
#include <string.h>
#include "req.h"
#include "animnodes.h"


/*****************************************************************************/


void PrepAnimNode(StorageReq *req, AnimNode *an, const AnimMethods *methods, uint32 objSize)
{
    TOUCH(req);

    memset(an, 0, objSize);
    an->an_Methods    = methods;
    an->an.n_Priority = NOMINAL_PRIORITY;
    an->an.n_Size     = objSize;
    PrepTransition(&an->an_DisabledLevel, TT_NONE, 0, 0, 0, NULL);
}


/*****************************************************************************/


void UnprepAnimNode(StorageReq *req, AnimNode *an)
{
    TOUCH(req);

    if (an)
    {
        UnprepTransition(&an->an_DisabledLevel);
        memset(an, 0, an->an.n_Size);
    }
}


/*****************************************************************************/


void StepAnimNode(StorageReq *req, AnimNode *an)
{
    StepTransition(&an->an_DisabledLevel, &req->sr_CurrentTime);
}


/*****************************************************************************/


void DisableAnimNode(StorageReq *req, AnimNode *an)
{
    if (an)
    {
        if (!an->an_Disabled)
        {
            an->an_Disabled = TRUE;

            PrepTransition(&an->an_DisabledLevel, TT_LINEAR,
                           an->an_DisabledLevel.ti_Current, 0xff, 1020,
                           &req->sr_CurrentTime);
        }
    }
}


/*****************************************************************************/


void EnableAnimNode(StorageReq *req, AnimNode *an)
{
    if (an)
    {
        if (an->an_Disabled)
        {
            an->an_Disabled = FALSE;

            PrepTransition(&an->an_DisabledLevel, TT_LINEAR,
                           an->an_DisabledLevel.ti_Current, 0x0, 1020,
                           &req->sr_CurrentTime);
        }
    }
}
