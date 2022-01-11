/* @(#) bg.c 96/09/12 1.5 */

#include <kernel/types.h>
#include <graphics/frame2d/spriteobj.h>
#include <stdio.h>
#include "req.h"
#include "utils.h"
#include "bg.h"



/*****************************************************************************/


static void DrawBackground(StorageReq *req, Background *bg, GState *gs);

static const AnimMethods bgMethods =
{
    (UnprepFunc)UnprepAnimNode,
    (StepFunc)StepAnimNode,
    (DrawFunc)DrawBackground,
    (DisableFunc)DisableAnimNode,
    (EnableFunc)EnableAnimNode
};


/*****************************************************************************/


void PrepBackground(StorageReq *req, Background *bg, SpriteObj **bgSlices, uint32 flags)
{
Point2 corner;
gfloat y;
uint32 i;

    PrepAnimNode(req, (AnimNode *)bg, &bgMethods, sizeof(Background));
    bg->bg.an.n_Priority = 0;

    bg->bg_Flags = flags;

    y = 0;
    for (i = 0; i < BG_NUMSLICES; i++)
    {
        bg->bg_Slices[i] = bgSlices[i];

        corner.x = 0.0;
        corner.y = y;

        Spr_SetPosition(bg->bg_Slices[i], &corner);
        y += Spr_GetHeight(bg->bg_Slices[i]);
    }

    if (flags & BG_DRAWBAR)
    {
        corner.x = 0;
        corner.y = 184;
        Spr_SetPosition(req->sr_BarImage, &corner);
    }
}


/*****************************************************************************/


static void DrawBackground(StorageReq *req, Background *bg, GState *gs)
{
uint32 i;

    for (i = 0; i < BG_NUMSLICES; i++)
        F2_Draw(gs, bg->bg_Slices[i]);

    if (bg->bg_Flags & BG_DRAWBAR)
        F2_Draw(gs, req->sr_BarImage);

    if ((req->sr_Prompt) && (bg->bg_Flags & BG_DRAWPROMPT))
        F2_Draw(gs, req->sr_Prompt);
}
