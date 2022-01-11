/* @(#) boxes.c 96/10/02 1.5 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/font.h>
#include <stdio.h>
#include "req.h"
#include "utils.h"
#include "msgstrings.h"
#include "hierarchy.h"
#include "boxes.h"


/*****************************************************************************/


#define SCROLL_SPEED            (10)


/*****************************************************************************/


static void StepBoxes(StorageReq *req, Boxes *boxes);
static void DrawBoxes(StorageReq *req, Boxes *boxes, GState *gs);


static const AnimMethods boxesMethods =
{
    (UnprepFunc)UnprepBoxes,
    (StepFunc)StepBoxes,
    (DrawFunc)DrawBoxes,
    (DisableFunc)DisableAnimNode,
    (EnableFunc)EnableAnimNode
};


/*****************************************************************************/


void PrepBoxes(struct StorageReq *req, Boxes *boxes, int16 x, int16 y, int16 width, int16 height)
{
    PrepAnimNode(req, (AnimNode *)boxes, &boxesMethods, sizeof(Control));

    boxes->bx_New         = 0;
    boxes->bx_X           = x;
    boxes->bx_Y           = y;
    boxes->bx_Width       = width;
    boxes->bx_Height      = height;
    boxes->bx_TotalHeight = 0;
    boxes->bx_ViewOffset  = 0;
    boxes->bx_NumBoxes    = 0;
    boxes->bx_StartBox    = 0;

    PrepTransition(&boxes->bx_CurrentTotalHeight,   TT_NONE, 0, 0, 0, NULL);
    PrepTransition(&boxes->bx_CurrentViewOffset,    TT_NONE, 0, 0, 0, NULL);
    PrepTransition(&boxes->bx_FrameLevel,           TT_NONE, 0, 0, 0, NULL);
    PrepTransition(&boxes->bx_BlinkLevel,           TT_NONE, 0, 0, 0, NULL);
}


/*****************************************************************************/


void UnprepBoxes(StorageReq *req, Boxes *boxes)
{
    UnprepTransition(&boxes->bx_CurrentTotalHeight);
    UnprepTransition(&boxes->bx_CurrentViewOffset);
    UnprepTransition(&boxes->bx_FrameLevel);
    UnprepTransition(&boxes->bx_BlinkLevel);
    UnprepAnimNode(req, (AnimNode *)boxes);
}


/*****************************************************************************/


static void StepBoxes(StorageReq *req, Boxes *boxes)
{
    StepAnimNode(req, (AnimNode *)boxes);
    StepTransition(&boxes->bx_CurrentTotalHeight, &req->sr_CurrentTime);
    StepTransition(&boxes->bx_CurrentViewOffset, &req->sr_CurrentTime);
    StepTransition(&boxes->bx_FrameLevel, &req->sr_CurrentTime);
    StepTransition(&boxes->bx_BlinkLevel, &req->sr_CurrentTime);

    if ((boxes->bx_New != 0)
     && (boxes->bx_FrameLevel.ti_End == boxes->bx_FrameLevel.ti_Current))
    {
        boxes->bx_New = 0;
    }
}


/*****************************************************************************/


static void DrawBoxes(StorageReq *req, Boxes *boxes, GState *gs)
{
Point2          corner;
SpriteObj      *spr;
uint32          y;
uint32          i;
Point2          point;
int16           textX;
int16           textY;
int16           textWidth;
int16           deltaX;
int16           deltaY;
HierarchyEntry *he;
bool            draw;

    SetOutClipping(gs, boxes->bx_X, boxes->bx_Y, (boxes->bx_X + boxes->bx_Width - 1), (boxes->bx_Y + boxes->bx_Height - 1));

    spr     = req->sr_BoxImages[0];
    y       = boxes->bx_Y + boxes->bx_Height - 1 - Spr_GetHeight(spr);
    textY   = y + Spr_GetHeight(spr) - req->sr_SampleChar.cd_Descent - 2;
    he      = (HierarchyEntry *)FirstNode(&req->sr_Hierarchy.h_Entries);

    for (i = 0; i < boxes->bx_StartBox; i++)                              /* Advance list to startbox */
        he = (HierarchyEntry *)NextNode(he);

    for (i = boxes->bx_StartBox; (i < (boxes->bx_StartBox+3)) && (i < boxes->bx_NumBoxes); i++)
    {
        spr = req->sr_BoxImages[0];                                     /* This box isn't doing anything */

        switch (boxes->bx_New)
        {
            case 1:                                                     /* Building a new box */
                    if ((i+1) >= boxes->bx_NumBoxes)
                        continue;
                    if ((i+2) >= boxes->bx_NumBoxes)
                        spr = req->sr_BoxImages[boxes->bx_FrameLevel.ti_Current];
                    break;
            case 2:                                                     /* Tearing down an existing box */
                    if ((i+1) >= boxes->bx_NumBoxes)
                        spr = req->sr_BoxImages[boxes->bx_FrameLevel.ti_Current];
                    break;
        }

        corner.x = boxes->bx_X;
        corner.y = y;
        Spr_SetPosition(spr, &corner);

        draw = FALSE;
        if (i || (((boxes->bx_BlinkLevel.ti_Current / 33) & 1) == 0))
            draw = TRUE;

        if (draw)
            F2_Draw(gs, spr);

        if (IsNode(&req->sr_Hierarchy.h_Entries, he))
        {
            if (he->he_Label && (textY >= boxes->bx_Y))
            {
                GetTextPosition(he->he_Label, &point);
                textWidth = he->he_Extent.se_TopRight.x - he->he_Extent.se_TopLeft.x + 1;
                textX     = boxes->bx_X + (((int32)boxes->bx_Width - (int32)textWidth) / 2);
                if (textX < 0)
                    textX = 0;

                deltaX  = textX - point.x;
                deltaY  = textY - point.y;
                MoveText(he->he_Label, deltaX, deltaY);

                if (draw)
                    DrawText(gs, he->he_Label);
            }
            he = (HierarchyEntry *)NextNode(he);
        }

        y     -= 42;
        textY -= 42;
    }

    ClearClipping(gs);
}



/*****************************************************************************/


void GetBoxCorners(StorageReq *req, Boxes *boxes, uint32 boxNum, Corner *corners)
{
uint32 i;
int32  y;

    if (boxNum < boxes->bx_StartBox)                                    /* Move the directory list down one */
        boxes->bx_StartBox = boxNum;
    else
        if (boxNum >= (boxes->bx_StartBox+3))                           /* Move the directory list up one */
            boxes->bx_StartBox = boxNum-2;

    y = boxes->bx_Y + boxes->bx_Height - 1 - Spr_GetHeight(req->sr_BoxImages[0]) + 37;
    for (i = boxes->bx_StartBox; i < boxNum; i++)
        y -= 42;

    corners[0].cr_X = boxes->bx_X;
    corners[0].cr_Y = y + 7;

    corners[1].cr_X = boxes->bx_X + boxes->bx_Width - 1;
    corners[1].cr_Y = y + 7;

    corners[2].cr_X = boxes->bx_X + boxes->bx_Width - 1;
    corners[2].cr_Y = y + 47;

    corners[3].cr_X = boxes->bx_X;
    corners[3].cr_Y = corners[2].cr_Y;
}


/*****************************************************************************/


void MakeBoxVisible(StorageReq *req, Boxes *boxes, uint32 boxNum)
{
    TOUCH(req);
    TOUCH(boxes);
    TOUCH(boxNum);
}


/*****************************************************************************/


static void CalcTotalHeight(Boxes *boxes)
{
    boxes->bx_TotalHeight = 100;
}


/*****************************************************************************/


void AddBoxes(StorageReq *req, Boxes *boxes, uint32 numBoxes)
{
    boxes->bx_NumBoxes += numBoxes;
    boxes->bx_New = 1;
    if ((boxes->bx_NumBoxes - boxes->bx_StartBox) > 3)
        boxes->bx_StartBox = boxes->bx_NumBoxes - 3;

    CalcTotalHeight(boxes);

    PrepTransition(&boxes->bx_FrameLevel, TT_LINEAR,
                   0,
                   NUM_BOX_IMAGES-1,
                   SCROLL_SPEED, &req->sr_CurrentTime);

    PrepTransition(&boxes->bx_CurrentTotalHeight, TT_LINEAR,
                   boxes->bx_CurrentTotalHeight.ti_Current,
                   boxes->bx_TotalHeight,
                   SCROLL_SPEED, &req->sr_CurrentTime);

}


/*****************************************************************************/


void RemoveBoxes(StorageReq *req, Boxes *boxes, uint32 numBoxes)
{
    boxes->bx_NumBoxes -= numBoxes;
    boxes->bx_New = 2;

    if (boxes->bx_NumBoxes > 3)
        boxes->bx_StartBox = boxes->bx_NumBoxes - 3;
	else
	    boxes->bx_StartBox = 0;

    CalcTotalHeight(boxes);

    PrepTransition(&boxes->bx_FrameLevel, TT_LINEAR,
                   NUM_BOX_IMAGES-1,
                   0,
                   SCROLL_SPEED, &req->sr_CurrentTime);

    PrepTransition(&boxes->bx_CurrentTotalHeight, TT_LINEAR,
                   boxes->bx_CurrentTotalHeight.ti_Current,
                   boxes->bx_TotalHeight,
                   SCROLL_SPEED, &req->sr_CurrentTime);

}


/*****************************************************************************/


void BlinkFirstBox(StorageReq *req, Boxes *boxes)
{
    TOUCH(req);
    PrepTransition(&boxes->bx_BlinkLevel, TT_LINEAR,
                   200, 0, 100, &req->sr_CurrentTime);
}
