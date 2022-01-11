/* @(#) controls.c 96/10/30 1.5 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/font.h>
#include <stdio.h>
#include <string.h>
#include "req.h"
#include "msgstrings.h"
#include "sound.h"
#include "controls.h"


/*****************************************************************************/


static void StepControl(StorageReq *req, Control *ctrl);
static void DrawControl(StorageReq *req, Control *ctrl, GState *gs);


static const AnimMethods controlMethods =
{
    (UnprepFunc)UnprepControl,
    (StepFunc)StepControl,
    (DrawFunc)DrawControl,
    (DisableFunc)DisableControl,
    (EnableFunc)EnableControl
};


/*****************************************************************************/


void PrepControl(StorageReq *req, Control *ctrl, int16 x, int16 y, ControlTypes type)
{
StringExtent  se;
FontTextArray fta;
char         *label;
gfloat        textX;
gfloat        textY;
int32         textWidth;
bool          onTop;
SpriteObj    *disabledFrame;

    PrepAnimNode(req, (AnimNode *)ctrl, &controlMethods, sizeof(Control));
    PrepTransition(&ctrl->ctrl_FrameLevel, TT_NONE, 0, 0, 0, NULL);

    ctrl->ctrl_Type             = type;
    ctrl->ctrl_Selected         = FALSE;
    ctrl->ctrl_Disabled         = FALSE;
    ctrl->ctrl_Highlighted      = FALSE;
    ctrl->ctrl_AnimateSelection = FALSE;
    ctrl->ctrl_Looping          = FALSE;
    ctrl->ctrl_PingPong         = FALSE;

    onTop                       = FALSE;
    label                       = NULL;
    disabledFrame               = NULL;

    switch (type)
    {
        case CTRL_TYPE_OK:
                            ctrl->ctrl_Frames           = req->sr_OKImages;
                            ctrl->ctrl_NumFrames        = NUM_OK_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            label                       = MSG_OK;
                            break;

        case CTRL_TYPE_LOAD:
                            ctrl->ctrl_Frames           = req->sr_LoadImages;
                            ctrl->ctrl_NumFrames        = NUM_LOAD_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            label                       = MSG_LOAD;
                            break;

        case CTRL_TYPE_SAVE:
                            ctrl->ctrl_Frames           = req->sr_SaveImages;
                            ctrl->ctrl_NumFrames        = NUM_SAVE_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            label                       = MSG_SAVE;
                            break;

        case CTRL_TYPE_CANCEL:
                            ctrl->ctrl_Frames           = req->sr_CancelImages;
                            ctrl->ctrl_NumFrames        = NUM_CANCEL_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            label                       = MSG_CANCEL;
                            break;

        case CTRL_TYPE_EXIT:
                            ctrl->ctrl_Frames           = req->sr_CancelImages;
                            ctrl->ctrl_NumFrames        = NUM_CANCEL_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            label                       = MSG_EXIT;
                            break;

        case CTRL_TYPE_QUIT:
                            ctrl->ctrl_Frames           = req->sr_CancelImages;
                            ctrl->ctrl_NumFrames        = NUM_CANCEL_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            label                       = MSG_QUIT;
                            break;

        case CTRL_TYPE_DELETE:
                            ctrl->ctrl_Frames           = req->sr_DeleteImages;
                            ctrl->ctrl_NumFrames        = NUM_DELETE_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 8;
                            label                       = MSG_DELETE;
                            break;

        case CTRL_TYPE_COPY:
                            ctrl->ctrl_Frames           = req->sr_CopyImages;
                            ctrl->ctrl_NumFrames        = NUM_COPY_IMAGES - 1;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            label                       = MSG_COPY;
                            disabledFrame               = req->sr_CopyImages[NUM_COPY_IMAGES - 1];
                            break;

        case CTRL_TYPE_MOVE:
                            ctrl->ctrl_Frames           = req->sr_MoveImages;
                            ctrl->ctrl_NumFrames        = NUM_MOVE_IMAGES - 1;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            label                       = MSG_MOVE;
                            disabledFrame               = req->sr_MoveImages[NUM_MOVE_IMAGES - 1];
                            break;

        case CTRL_TYPE_END_COPY:
        case CTRL_TYPE_END_MOVE:
                            ctrl->ctrl_Frames           = req->sr_SuitcaseImages;
                            ctrl->ctrl_NumFrames        = NUM_SUITCASE_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            break;

        case CTRL_TYPE_RENAME:
                            ctrl->ctrl_Frames           = req->sr_RenameImages;
                            ctrl->ctrl_NumFrames        = NUM_RENAME_IMAGES - 1;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            label                       = MSG_RENAME;
                            disabledFrame               = req->sr_RenameImages[NUM_RENAME_IMAGES - 1];
                            break;

        case CTRL_TYPE_CREATEDIR:
                            ctrl->ctrl_Frames           = req->sr_CreateDirImages;
                            ctrl->ctrl_NumFrames        = NUM_CREATEDIR_IMAGES - 1;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            label                       = MSG_CREATEDIR;
                            disabledFrame               = req->sr_CreateDirImages[NUM_CREATEDIR_IMAGES - 1];
                            break;

        case CTRL_TYPE_FORMAT:
                            ctrl->ctrl_Frames           = req->sr_RenameImages;
                            ctrl->ctrl_NumFrames        = NUM_RENAME_IMAGES - 1;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            label                       = MSG_FORMAT;
                            disabledFrame               = req->sr_RenameImages[NUM_RENAME_IMAGES - 1];
                            break;

        case CTRL_TYPE_OVL_COPY:
                            ctrl->ctrl_Frames           = req->sr_ButtonImages;
                            ctrl->ctrl_NumFrames        = NUM_BUTTON_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            ctrl->ctrl_AnimateSelection = TRUE;
                            label                       = MSG_COPY;
                            onTop                       = TRUE;
                            break;

        case CTRL_TYPE_OVL_MOVE:
                            ctrl->ctrl_Frames           = req->sr_ButtonImages;
                            ctrl->ctrl_NumFrames        = NUM_BUTTON_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            ctrl->ctrl_AnimateSelection = TRUE;
                            label                       = MSG_MOVE;
                            onTop                       = TRUE;
                            break;

        case CTRL_TYPE_OVL_DELETE:
                            ctrl->ctrl_Frames           = req->sr_ButtonImages;
                            ctrl->ctrl_NumFrames        = NUM_BUTTON_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            ctrl->ctrl_AnimateSelection = TRUE;
                            label                       = MSG_OVL_DELETE;
                            onTop                       = TRUE;
                            break;

        case CTRL_TYPE_OVL_FORMAT:
                            ctrl->ctrl_Frames           = req->sr_ButtonImages;
                            ctrl->ctrl_NumFrames        = NUM_BUTTON_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            ctrl->ctrl_AnimateSelection = TRUE;
                            label                       = MSG_OVL_FORMAT;
                            onTop                       = TRUE;
                            break;

        case CTRL_TYPE_OVL_CANCEL:
                            ctrl->ctrl_Frames           = req->sr_ButtonImages;
                            ctrl->ctrl_NumFrames        = NUM_BUTTON_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            ctrl->ctrl_AnimateSelection = TRUE;
                            label                       = MSG_OVL_CANCEL;
                            onTop                       = TRUE;
                            break;

        case CTRL_TYPE_OVL_STOP:
                            ctrl->ctrl_Frames           = req->sr_ButtonImages;
                            ctrl->ctrl_NumFrames        = NUM_BUTTON_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            ctrl->ctrl_AnimateSelection = TRUE;
                            label                       = MSG_OVL_STOP;
                            onTop                       = TRUE;
                            break;

        case CTRL_TYPE_OVL_OK:
                            ctrl->ctrl_Frames           = req->sr_ButtonImages;
                            ctrl->ctrl_NumFrames        = NUM_BUTTON_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            ctrl->ctrl_AnimateSelection = TRUE;
                            label                       = MSG_OVL_OK;
                            onTop                       = TRUE;
                            break;

        case CTRL_TYPE_OVL_LOCK:
                            ctrl->ctrl_Frames           = req->sr_LockImages;
                            ctrl->ctrl_NumFrames        = NUM_LOCK_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            break;

        case CTRL_TYPE_OVL_QUESTION:
                            ctrl->ctrl_Frames           = req->sr_QuestionImages;
                            ctrl->ctrl_NumFrames        = NUM_QUESTION_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 18;
                            break;

        case CTRL_TYPE_OVL_FOLDER:
                            ctrl->ctrl_Frames           = req->sr_FolderImages;
                            ctrl->ctrl_NumFrames        = NUM_FOLDER_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            break;

        case CTRL_TYPE_OVL_WORKING:
                            ctrl->ctrl_Frames           = req->sr_WorkingImages;
                            ctrl->ctrl_NumFrames        = NUM_WORKING_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            ctrl->ctrl_Looping          = TRUE;
                            break;

        case CTRL_TYPE_OVL_HANDINSERT:
                            ctrl->ctrl_Frames           = req->sr_HandInsertImages;
                            ctrl->ctrl_NumFrames        = NUM_HANDINSERT_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            break;

        case CTRL_TYPE_OVL_ERROR:
                            ctrl->ctrl_Frames           = req->sr_StopImages;
                            ctrl->ctrl_NumFrames        = NUM_STOP_IMAGES;
                            ctrl->ctrl_FramesPerSecond  = 12;
                            break;

#ifdef BUILD_PARANOIA
        default:
                            printf("ERROR: illegal control type specified to PrepControl\n");
                            return;
#endif
    }

    ctrl->ctrl_X                = x;
    ctrl->ctrl_Y                = y;
    ctrl->ctrl_Width            = Spr_GetWidth(ctrl->ctrl_Frames[0]),
    ctrl->ctrl_Height           = Spr_GetHeight(ctrl->ctrl_Frames[0]),
    ctrl->ctrl_Label            = NULL;
    ctrl->ctrl_DisabledFrame    = disabledFrame;

    ctrl->ctrl_TotalX           = ctrl->ctrl_X;
    ctrl->ctrl_TotalY           = ctrl->ctrl_Y;
    ctrl->ctrl_TotalWidth       = ctrl->ctrl_Width;
    ctrl->ctrl_TotalHeight      = ctrl->ctrl_Height;

    if (!label)
        return;

    if (onTop)
        ctrl->ctrl_FgColor = TEXT_COLOR_CONTROL_BUTTON;
    else
        ctrl->ctrl_FgColor = TEXT_COLOR_CONTROL_LABEL_NORMAL;

    fta.fta_StructSize          = sizeof(fta);                      /* The size of this structure */
    fta.fta_Pen.pen_X           = 0;
    fta.fta_Pen.pen_Y           = req->sr_SampleChar.cd_Ascent;
    fta.fta_Pen.pen_FgColor     = ctrl->ctrl_FgColor;               /* Set the foreground color */
    fta.fta_Pen.pen_BgColor     = 0;                                /* The background color is 0 (black) */
    fta.fta_Pen.pen_XScale      = 1.0;                              /* We want a 1:1 aspect ratio */
    fta.fta_Pen.pen_YScale      = 1.0;
    fta.fta_Pen.pen_Flags       = 0;
    fta.fta_Pen.pen_reserved    = 0;
    fta.fta_Clip.min.x          = 0.0;                              /* Set upper left clipping corner to 0,0 */
    fta.fta_Clip.min.y          = 0.0;
    fta.fta_Clip.max.x          = BG_WIDTH;                         /* Set lower right clipping corner to 640x240 */
    fta.fta_Clip.max.y          = BG_HEIGHT;
    fta.fta_String              = (char *)label;                    /* This is the strings to print */
    fta.fta_NumChars            = strlen(label);                    /* This is the amount of characters in above string */

    if (GetStringExtent(&se, req->sr_Font, &fta.fta_Pen, fta.fta_String, fta.fta_NumChars) < 0)
        return;

    textWidth   = se.se_TopRight.x - se.se_TopLeft.x + 1;
    textX       = ctrl->ctrl_X + (((int32)ctrl->ctrl_Width - (int32)textWidth) / 2);

    if (onTop)
        textY   = ctrl->ctrl_Y + ((ctrl->ctrl_Height - req->sr_SampleChar.cd_CharHeight) / 2);
    else
        textY   = ctrl->ctrl_Y + ctrl->ctrl_Height - 2;

    if (CreateTextState(&ctrl->ctrl_Label, req->sr_Font, &fta, 1) < 0)
        return;

    MoveText(ctrl->ctrl_Label, textX, textY);

    if (textX < ctrl->ctrl_X)
    {
        ctrl->ctrl_TotalX    = textX;
        ctrl->ctrl_TotalWidth = textWidth;
    }

    if (!onTop)
        ctrl->ctrl_TotalHeight += req->sr_SampleChar.cd_CharHeight - 2;
}


/*****************************************************************************/


void UnprepControl(StorageReq *req, Control *ctrl)
{
    UnprepTransition(&ctrl->ctrl_FrameLevel);
    DeleteTextState(ctrl->ctrl_Label);
    UnprepAnimNode(req, (AnimNode *)ctrl);
}


/*****************************************************************************/


static void StepControl(StorageReq *req, Control *ctrl)
{
    StepAnimNode(req, (AnimNode *)ctrl);
    StepTransition(&ctrl->ctrl_FrameLevel, &req->sr_CurrentTime);
}


/*****************************************************************************/


static void DrawControl(StorageReq *req, Control *ctrl, GState *gs)
{
SpriteObj *spr;
Point2     corner;
uint32     not;

    TOUCH(req);

    spr = ctrl->ctrl_Frames[ctrl->ctrl_FrameLevel.ti_Current];

#if 0
    if (ctrl->ctrl.an_DisabledLevel.ti_Current && ctrl->ctrl_DisabledFrame)
    {
        spr = ctrl->ctrl_DisabledFrame;
    }
    else
#endif
    {
        not = 255 - (ctrl->ctrl.an_DisabledLevel.ti_Current / 2);
        Spr_SetTextureAttr(spr, TXA_BlendColorSSB0, (not << 16) | (not << 8) | not);
    }

    corner.x = ctrl->ctrl_X;
    corner.y = ctrl->ctrl_Y;
    Spr_SetPosition(spr, &corner);
    F2_Draw(gs, spr);

    if (ctrl->ctrl_Label)
    {
        if (ctrl->ctrl_Disabled)
            SetTextColor(ctrl->ctrl_Label, TEXT_COLOR_CONTROL_LABEL_DISABLED, 0);
        else
            SetTextColor(ctrl->ctrl_Label, ctrl->ctrl_FgColor, 0);

        DrawText(gs, ctrl->ctrl_Label);
    }
}


/*****************************************************************************/


void SelectControl(StorageReq *req, Control *ctrl)
{
    TOUCH(req);

    if (ctrl->ctrl_Disabled)
    {
        PlaySound(req, SOUND_UNAVAILABLE);
    }
    else if (!ctrl->ctrl_Selected)
    {
        ctrl->ctrl_Selected = TRUE;

        if (ctrl->ctrl_AnimateSelection)
        {
            PrepTransition(&ctrl->ctrl_FrameLevel, TT_LINEAR,
                            ctrl->ctrl_FrameLevel.ti_Current,
                            ctrl->ctrl_NumFrames - 1,
                            ctrl->ctrl_FramesPerSecond, &req->sr_CurrentTime);
        }
    }
}


/*****************************************************************************/


void UnselectControl(StorageReq *req, Control *ctrl)
{
    TOUCH(req);

    if (ctrl->ctrl_Selected)
    {
        ctrl->ctrl_Selected = FALSE;

        if (ctrl->ctrl_AnimateSelection)
        {
            PrepTransition(&ctrl->ctrl_FrameLevel, TT_LINEAR,
                           ctrl->ctrl_FrameLevel.ti_Current,
                           0,
                           ctrl->ctrl_FramesPerSecond, &req->sr_CurrentTime);
        }
    }
}


/*****************************************************************************/


void HighlightControl(StorageReq *req, Control *ctrl)
{
    TOUCH(req);

    if (ctrl)
    {
        if (!ctrl->ctrl_Highlighted)
        {
            ctrl->ctrl_Highlighted = TRUE;

            if (!ctrl->ctrl_AnimateSelection)
            {
                if (ctrl->ctrl_Looping)
                {
                    PrepTransition(&ctrl->ctrl_FrameLevel, TT_LOOPING,
                                   ctrl->ctrl_FrameLevel.ti_Current,
                                   ctrl->ctrl_NumFrames - 1,
                                   ctrl->ctrl_FramesPerSecond, &req->sr_CurrentTime);
                }
                else
                {
                    if (ctrl->ctrl_PingPong)
                    {
                        PrepTransition(&ctrl->ctrl_FrameLevel, TT_PINGPONG,
                                       ctrl->ctrl_FrameLevel.ti_Current,
                                       ctrl->ctrl_NumFrames - 1,
                                       ctrl->ctrl_FramesPerSecond, &req->sr_CurrentTime);
                    }
                    else
                    {
                        PrepTransition(&ctrl->ctrl_FrameLevel, TT_LINEAR,
                                       ctrl->ctrl_FrameLevel.ti_Current,
                                       ctrl->ctrl_NumFrames - 1,
                                       ctrl->ctrl_FramesPerSecond, &req->sr_CurrentTime);
                    }
                }
            }
        }
    }
}


/*****************************************************************************/


void UnhighlightControl(StorageReq *req, Control *ctrl)
{
    TOUCH(req);

    if (ctrl)
    {
        if (ctrl->ctrl_Highlighted)
        {
            ctrl->ctrl_Highlighted = FALSE;

            if (!ctrl->ctrl_AnimateSelection)
            {
                PrepTransition(&ctrl->ctrl_FrameLevel, TT_LINEAR,
                               ctrl->ctrl_FrameLevel.ti_Current,
                               0,
                               ctrl->ctrl_FramesPerSecond, &req->sr_CurrentTime);
            }
        }
    }
}


/*****************************************************************************/


void EnableControl(StorageReq *req, Control *ctrl)
{
    if (ctrl)                                                   /* If there's really a control here */
    {
        if (ctrl->ctrl_Disabled)                                /* And it's disabled */
        {
            ctrl->ctrl_Disabled = FALSE;                        /* Tell it it's enabled */
            EnableAnimNode(req, (AnimNode *)ctrl);              /* Make sure it really is */
        }
    }
}


/*****************************************************************************/


void DisableControl(StorageReq *req, Control *ctrl)
{
    if (ctrl)                                                   /* If there's really a control here */
    {
        if (!ctrl->ctrl_Disabled)                               /* And it's enabled */
        {
            ctrl->ctrl_Disabled = TRUE;                         /* Tell it it's disabled */
            DisableAnimNode(req, (AnimNode *)ctrl);             /* Make sure it really is */
            UnselectControl(req, ctrl);                         /* You can't select something which is disabled */
            UnhighlightControl(req, ctrl);                      /* An likewise you won't highlight it */
        }
    }
}


/*****************************************************************************/


void GetControlCorners(const Control *ctrl, Corner *corners)
{
    corners[0].cr_X = ctrl->ctrl_TotalX;
    corners[0].cr_Y = ctrl->ctrl_TotalY;

    corners[1].cr_X = ctrl->ctrl_TotalX + ctrl->ctrl_TotalWidth - 1;
    corners[1].cr_Y = ctrl->ctrl_TotalY;

    corners[2].cr_X = ctrl->ctrl_TotalX + ctrl->ctrl_TotalWidth - 1;
    corners[2].cr_Y = ctrl->ctrl_TotalY + ctrl->ctrl_TotalHeight - 1;

    corners[3].cr_X = ctrl->ctrl_TotalX;
    corners[3].cr_Y = ctrl->ctrl_TotalY + ctrl->ctrl_TotalHeight - 1;
}


/*****************************************************************************/


void PositionControl(StorageReq *req, Control *ctrl, int16 x, int16 y)
{
int16 deltaX;
int16 deltaY;

    TOUCH(req);

    deltaX       = x - ctrl->ctrl_X;
    deltaY       = y - ctrl->ctrl_Y;
    ctrl->ctrl_X = x;
    ctrl->ctrl_Y = y;

    ctrl->ctrl_TotalX += deltaX;
    ctrl->ctrl_TotalY += deltaY;

    if (ctrl->ctrl_Label)
        MoveText(ctrl->ctrl_Label, deltaX, deltaY);
}
