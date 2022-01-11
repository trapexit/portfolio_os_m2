/* @(#) movingtext.c 96/10/30 1.6 */

#include <kernel/types.h>
#include <graphics/frame2d/spriteobj.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "req.h"
#include "utils.h"
#include "bg.h"
#include "msgstrings.h"
#include "movingtext.h"


/*****************************************************************************/


static void StepMovingText(StorageReq *req, MovingText *mt);
static void DrawMovingText(StorageReq *req, MovingText *mt, GState *gs);


static const AnimMethods movingTextMethods =
{
    (UnprepFunc)UnprepMovingText,
    (StepFunc)StepMovingText,
    (DrawFunc)DrawMovingText,
    (DisableFunc)DisableAnimNode,
    (EnableFunc)EnableAnimNode
};


/*****************************************************************************/


Err PrepMovingText(StorageReq *req, MovingText *mt, TextTypes type, const char *label)
{
FontTextArray fta;
int16         xSpeed, ySpeed;
int16         destX, destY;
StringExtent  se;
Err           result;

    PrepAnimNode(req, (AnimNode *)mt, &movingTextMethods, sizeof(MovingText));

    mt->mt.an.n_Priority++;
    mt->mt_Type    = type;
    mt->mt_Stabilized = FALSE;

    fta.fta_StructSize        = sizeof(FontTextArray);
    fta.fta_Pen.pen_X          = req->sr_ListView.lv_X + 4;
    fta.fta_Pen.pen_Y          = req->sr_ListView.lv_Y + req->sr_ListView.lv_SelectionY + req->sr_SampleChar.cd_Ascent;
    fta.fta_Pen.pen_FgColor  = TEXT_COLOR_VIEWLIST_NORMAL;
    fta.fta_Pen.pen_BgColor  = 0;
    fta.fta_Pen.pen_XScale    = 1.0;
    fta.fta_Pen.pen_YScale    = 1.0;
    fta.fta_Pen.pen_Flags = 0;
    fta.fta_Pen.pen_reserved = 0;
    fta.fta_Clip.min.x        = 0.0;
    fta.fta_Clip.min.y        = 0.0;
    fta.fta_Clip.max.x        = BG_WIDTH;
    fta.fta_Clip.max.y        = BG_HEIGHT;
    fta.fta_String            = (char *)label;
    fta.fta_NumChars            = strlen(label);

    result = GetStringExtent(&se, req->sr_Font, &fta.fta_Pen, fta.fta_String, fta.fta_NumChars);
    if (result < 0)
        return result;

    mt->mt_Width = se.se_TopRight.x - se.se_TopLeft.x + 1;

    if (type == TEXT_DELETED)
    {
        destX = fta.fta_Pen.pen_X;
        destY = BG_HEIGHT + 120;

        PrepTransition(&mt->mt_CurrentX, TT_NONE, fta.fta_Pen.pen_X, destX, 0, &req->sr_CurrentTime);
        PrepTransition(&mt->mt_CurrentY, TT_ACCELERATED, fta.fta_Pen.pen_Y, destY, 10, &req->sr_CurrentTime);
    }
    else if (type == TEXT_COPIED)
    {
        destX = req->sr_Suitcase.ctrl_X + (((int32)req->sr_Suitcase.ctrl_Width - (int32)(se.se_TopRight.x - se.se_TopLeft.x + 1)) / 2);
        destY = req->sr_Suitcase.ctrl_Y + req->sr_Suitcase.ctrl_Height - req->sr_SampleChar.cd_Descent - 2;

        if (destX < 10)
            destX = 10;

        xSpeed = abs(fta.fta_Pen.pen_X - destX) * 3;
        ySpeed = abs(fta.fta_Pen.pen_Y - destY) * 3;

        PrepTransition(&mt->mt_CurrentX, TT_LINEAR, fta.fta_Pen.pen_X, destX, xSpeed, &req->sr_CurrentTime);
        PrepTransition(&mt->mt_CurrentY, TT_LINEAR, fta.fta_Pen.pen_Y, destY, ySpeed, &req->sr_CurrentTime);
    }

    PrepTransition(&mt->mt_CurrentAngle, TT_LINEAR, 0, 360, 360*3, &req->sr_CurrentTime);

    mt->mt_DestX = destX;
    mt->mt_DestY = destY;

    return CreateTextState(&mt->mt_Label, req->sr_Font, &fta, 1);
}


/*****************************************************************************/


void UnprepMovingText(StorageReq *req, MovingText *mt)
{
    UnprepTransition(&mt->mt_CurrentX);
    UnprepTransition(&mt->mt_CurrentY);
    UnprepTransition(&mt->mt_CurrentAngle);
    DeleteTextState(mt->mt_Label);
    UnprepAnimNode(req, (AnimNode *)mt);
}


/*****************************************************************************/


static void StepMovingText(StorageReq *req, MovingText *mt)
{
int16  newX, oldX;
int16  newY, oldY;
int16  oldAngle;
gfloat currentAngle;
Point2 point;

    StepAnimNode(req, (AnimNode *)mt);

    if (!mt->mt_Stabilized)
    {
        oldX     = mt->mt_CurrentX.ti_Current;
        oldY     = mt->mt_CurrentY.ti_Current;
        oldAngle = mt->mt_CurrentAngle.ti_Current;

        StepTransition(&mt->mt_CurrentX, &req->sr_CurrentTime);
        StepTransition(&mt->mt_CurrentY, &req->sr_CurrentTime);
        StepTransition(&mt->mt_CurrentAngle, &req->sr_CurrentTime);

        newX = mt->mt_CurrentX.ti_Current;
        newY = mt->mt_CurrentY.ti_Current;

        if ((newX != oldX) || (newY != oldY) || (mt->mt_CurrentAngle.ti_Current != oldAngle))
        {
            GetTextAngle(mt->mt_Label, &currentAngle);
            if (mt->mt_Type == TEXT_DELETED)
                RotateText(mt->mt_Label, mt->mt_CurrentAngle.ti_Current - currentAngle, newX + mt->mt_Width, newY);
            else
                RotateText(mt->mt_Label, mt->mt_CurrentAngle.ti_Current - currentAngle, newX, newY);

            GetTextPosition(mt->mt_Label, &point);
            MoveText(mt->mt_Label, newX - point.x, newY - point.y);

            if ((newX == mt->mt_DestX) && (newY == mt->mt_DestY) && ((mt->mt_CurrentAngle.ti_Current % 360) == 0))
            {
                mt->mt_Stabilized = TRUE;
            }
        }
    }
}


/*****************************************************************************/


static void DrawMovingText(StorageReq *req, MovingText *mt, GState *gs)
{
    TOUCH(req);

    DrawText(gs, mt->mt_Label);
}


/*****************************************************************************/


void ReturnMovingText(StorageReq *req, MovingText *mt)
{
int16 xSpeed;
int16 ySpeed;

    mt->mt_DestX      = req->sr_ListView.lv_X + 4;
    mt->mt_DestY      = req->sr_ListView.lv_Y + req->sr_ListView.lv_SelectionY + req->sr_SampleChar.cd_Ascent;
    mt->mt_Stabilized = FALSE;

    xSpeed = abs(mt->mt_CurrentX.ti_Current - mt->mt_DestX) * 4;
    ySpeed = abs(mt->mt_CurrentY.ti_Current - mt->mt_DestY) * 4;

    PrepTransition(&mt->mt_CurrentX, TT_LINEAR, mt->mt_CurrentX.ti_Current, mt->mt_DestX, xSpeed, &req->sr_CurrentTime);
    PrepTransition(&mt->mt_CurrentY, TT_LINEAR, mt->mt_CurrentY.ti_Current, mt->mt_DestY, ySpeed, &req->sr_CurrentTime);
}
