/* @(#) io_loop.c 96/10/30 1.5 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/io.h>
#include <kernel/list.h>
#include <kernel/time.h>
#include <kernel/task.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/fsutils.h>
#include <ui/icon.h>
#include <ui/requester.h>
#include <graphics/font.h>
#include <graphics/frame2d/spriteobj.h>
#include <stdio.h>
#include <string.h>
#include "req.h"
#include "eventmgr.h"
#include "dirscan.h"
#include "framebuf.h"
#include "msgstrings.h"
#include "utils.h"
#include "highlight.h"
#include "bg.h"
#include "animlists.h"
#include "overlays.h"
#include "controls.h"
#include "ioserver.h"
#include "sound.h"
#include "eventloops.h"


/*****************************************************************************/


typedef struct
{
    AnimList  od_AnimList;
    Control   od_Anim;
    Control   od_Button;
    Highlight od_Highlight;
    Overlay   od_Overlay;
    bool      od_OverlayDisplayed;
    bool      od_AddedControls;
} OverlayData;


/*****************************************************************************/


static void RemoveOverlay(StorageReq *req, OverlayData *od, Control *ctrl)
{
    if (od->od_OverlayDisplayed)
    {
        if (od->od_AddedControls)
        {
            RemNode((Node *)&od->od_Anim);
            RemNode((Node *)&od->od_Button);
            RemNode((Node *)&od->od_Highlight);

            UnprepControl(req,   &od->od_Anim);
            UnprepControl(req,   &od->od_Button);
            UnprepHighlight(req, &od->od_Highlight);

            od->od_AddedControls = FALSE;
        }

        if (ctrl)
        {
            MoveOverlay(req, &od->od_Overlay, ctrl->ctrl_X + (ctrl->ctrl_Width / 2),
                                              ctrl->ctrl_Y + (ctrl->ctrl_Height / 2));
        }
        else
        {
            MoveOverlay(req, &od->od_Overlay, BG_WIDTH / 2, BG_HEIGHT / 2);
        }

        while (!od->od_Overlay.ov_Stabilized)
        {
            WaitSignal(req->sr_RenderSig);
            DoNextFrame(req, &req->sr_AnimList);
        }

        RemNode((Node *)&od->od_AnimList);
        UnprepAnimList(req, &od->od_AnimList);

        od->od_OverlayDisplayed = FALSE;
    }
}


/*****************************************************************************/


Err IOLoop(StorageReq *req, const ServerPacket *packet, const Control *ctrl)
{
uint32        sigs;
TimeVal       startTime;
TimeVal       tv;
bool          recoverable;
bool          exiting;
bool          doOverlay;
bool          selected;
Err           statusReply;
Item          statusMsg;
ServerStatus *status;
Event         event;
Item          eventMsg;
Err           result;
Corner        corners[NUM_HIGHLIGHT_CORNERS];
OverlayTypes  ovlType;
ControlTypes  animType;
int16         buttonWidth;
int16         buttonHeight;
int16         buttonX;
int16         buttonY;
int16         timer;
OverlayData   od;
const char   *name;
Sounds        sound;

    result = StartIOService(req->sr_IOServer, packet);
    if (result < 0)
        return result;

    SampleSystemTimeTV(&startTime);

    statusMsg   = -1;
    statusReply = 0;
    recoverable = FALSE;
    exiting     = FALSE;
    ovlType     = -1;
    animType    = -1;
    doOverlay   = FALSE;
    selected    = FALSE;
    name        = NULL;
    sound       = -1;

    od.od_OverlayDisplayed = FALSE;
    od.od_AddedControls    = FALSE;

    while (TRUE)
    {
        sigs = WaitSignal(req->sr_RenderSig
                          | MSGPORT(req->sr_IOStatusPort)->mp_Signal
                          | MSGPORT(req->sr_EventPort)->mp_Signal);

        if (req->sr_RenderSig & sigs)
        {
            DoNextFrame(req, &req->sr_AnimList);

            if (od.od_OverlayDisplayed)
            {
                if (od.od_Overlay.ov_Stabilized)
                {
                    if (od.od_AddedControls)
                    {
                        if (sound >= -1)
                            PlaySound(req, sound);

                        timer--;
                        if (timer == 0)
                        {
                            timer = 75;
                            if (od.od_Anim.ctrl_Highlighted)
                            {
                                if (!od.od_Anim.ctrl_Looping)
                                    UnhighlightControl(req, &od.od_Anim);
                            }
                            else
                            {
                                HighlightControl(req, &od.od_Anim);
                            }
                        }
                    }
                    else
                    {
                        od.od_AddedControls = TRUE;

                        PrepHighlight(req, &od.od_Highlight);
                        od.od_Highlight.hl.an.n_Priority = 200;
                        InsertAnimNode(&od.od_AnimList, (AnimNode *)&od.od_Highlight);

                        if (ovlType == OVL_WORKING)
                            PrepControl(req, &od.od_Button, 0, 0, CTRL_TYPE_OVL_STOP);
                        else
                            PrepControl(req, &od.od_Button, 0, 0, CTRL_TYPE_OVL_CANCEL);

                        buttonWidth  = od.od_Button.ctrl_Width;
                        buttonHeight = od.od_Button.ctrl_Height;
                        buttonX      = od.od_Overlay.ov_X + od.od_Overlay.ov_Width - 1 - buttonWidth - 4;
                        buttonY      = od.od_Overlay.ov_Y + od.od_Overlay.ov_Height - 1 - buttonHeight;

                        PositionControl(req, &od.od_Button, buttonX, buttonY);
                        InsertAnimNode(&od.od_AnimList, (AnimNode *)&od.od_Button);

                        {
                            int16 animX;
                            int16 animY;

                            PrepControl(req, &od.od_Anim, 0, 0, animType);

                            animX = od.od_Overlay.ov_X + od.od_Overlay.ov_Width - od.od_Anim.ctrl_Width - 26;
                            if (animType == CTRL_TYPE_OVL_FOLDER)
                                animY = od.od_Overlay.ov_Y + 19;
                            else
                                animY = od.od_Overlay.ov_Y + 20;

                            PositionControl(req, &od.od_Anim, animX, animY);
                            InsertAnimNode(&od.od_AnimList, (AnimNode *)&od.od_Anim);
                        }

                        GetControlCorners(&od.od_Button, corners);
                        MoveHighlightNow(req, &od.od_Highlight, corners);
                        DisableHighlightNow(req, &od.od_Highlight);
                        EnableHighlight(req, &od.od_Highlight);

                        timer = 75;
                    }
                }
            }
        }

        if (statusMsg <= 0)
        {
            statusMsg = GetMsg(req->sr_IOStatusPort);
            if (statusMsg > 0)
            {
                status = (ServerStatus *)MESSAGE(statusMsg)->msg_DataPtr;
                switch (status->ss_State)
                {
                    case SS_DONE           : RemoveOverlay(req, &od, ctrl);
                                             result = status->ss_Info.ss_ErrorCode;
                                             ReplyMsg(statusMsg, 0, NULL, 0);
                                             return result;

                    case SS_DIRSCANNED     : req->sr_DirScanner = status->ss_Info.ss_DirScanner;
                                             req->sr_DirEntries = &req->sr_DirScanner->ds_Entries;
                                             ReplyMsg(statusMsg, 0, NULL, 0);
                                             statusMsg = -1;
                                             break;

                    case SS_ERROR          : recoverable = FALSE;
                                             statusReply = status->ss_Info.ss_ErrorCode;
                                             doOverlay   = TRUE;
                                             animType    = CTRL_TYPE_OVL_ERROR;
                                             ovlType     = OVL_ERROR;
                                             name        = NULL;
                                             sound       = SOUND_ERROR;

#ifdef BUILD_STRINGS
                                             printf("I/O error: ");
                                             PrintfSysErr(statusReply);
#endif
                                             break;

                    case SS_NEED_MEDIA     : recoverable = TRUE;
                                             statusReply = FILE_ERR_OFFLINE;
                                             doOverlay   = TRUE;
                                             animType    = CTRL_TYPE_OVL_HANDINSERT;
                                             ovlType     = OVL_MEDIAREQUEST;
                                             name        = status->ss_Info.ss_MediaSought;
                                             sound       = SOUND_QUESTION;
                                             break;

                    case SS_MEDIA_PROTECTED: recoverable = TRUE;
                                             statusReply = FILE_ERR_READONLY;
                                             doOverlay   = TRUE;
                                             animType    = CTRL_TYPE_OVL_LOCK;
                                             ovlType     = OVL_MEDIAPROTECTED;
                                             name        = status->ss_Info.ss_MediaProtected;
                                             sound       = SOUND_QUESTION;
                                             break;

                    case SS_MEDIA_FULL     : recoverable = FALSE;
                                             statusReply = FILE_ERR_NOSPACE;
                                             doOverlay   = TRUE;
                                             animType    = CTRL_TYPE_OVL_FOLDER;
                                             ovlType     = OVL_MEDIAFULL;
                                             name        = status->ss_Info.ss_MediaFull;
                                             sound       = SOUND_ERROR;
                                             break;

                    case SS_DUPLICATE_FILE : recoverable = FALSE;
                                             statusReply = FILE_ERR_DUPLICATEFILE;
                                             doOverlay   = TRUE;
                                             animType    = CTRL_TYPE_OVL_ERROR;
                                             ovlType     = OVL_DUPLICATE;
                                             name        = FindFinalComponent(status->ss_Info.ss_DuplicateFile);
                                             sound       = SOUND_ERROR;
                                             break;
											 
                    case SS_HIERARCHY_COPY : recoverable = FALSE;
                                             statusReply = FILE_ERR_DUPLICATEFILE;
                                             doOverlay   = TRUE;
                                             animType    = CTRL_TYPE_OVL_ERROR;
                                             ovlType     = OVL_HIERARCHYCOPY;
                                             name        = FindFinalComponent(status->ss_Info.ss_HierarchyName);
                                             sound       = SOUND_ERROR;
                                             break;
											 
                }
            }
        }

        if (!od.od_OverlayDisplayed && !exiting && !doOverlay)
        {
            SampleSystemTimeTV(&tv);
            SubTimes(&startTime, &tv, &tv);
            if (tv.tv_Seconds >= 1)
            {
                doOverlay = TRUE;
                ovlType   = OVL_WORKING;
                animType  = CTRL_TYPE_OVL_WORKING;
            }
        }

        if (doOverlay)
        {
            RemoveOverlay(req, &od, ctrl);

            PrepAnimList(req, &od.od_AnimList);
            od.od_AnimList.al.an.n_Priority = 200;
            InsertAnimNode(&req->sr_AnimList, (AnimNode *)&od.od_AnimList);

            if (ctrl)
            {
                result = PrepOverlay(req, &od.od_Overlay, ovlType,
                                     ctrl->ctrl_X + (ctrl->ctrl_Width / 2),
                                     ctrl->ctrl_Y + (ctrl->ctrl_Height / 2),
                                     name);
            }
            else
            {
                result = PrepOverlay(req, &od.od_Overlay, ovlType, BG_WIDTH / 2, BG_HEIGHT / 2, name);
            }

            if (result < 0)
            {
                StopIOService(req->sr_IOServer);
                if (statusMsg > 0)
                {
                    ReplyMsg(statusMsg, statusReply, NULL, 0);
                    statusMsg = -1;
                }
                exiting = TRUE;
            }
            else
            {
                od.od_Overlay.ov.an.n_Priority = 0;
                InsertAnimNode(&od.od_AnimList, (AnimNode *)&od.od_Overlay);

                od.od_OverlayDisplayed = TRUE;
                doOverlay              = FALSE;
            }
        }

        while (TRUE)
        {
            eventMsg = GetMsg(req->sr_EventPort);
            if (eventMsg <= 0)
                break;

            event = *(Event *)(MESSAGE(eventMsg)->msg_DataPtr);
            ReplyMsg(eventMsg, 0, NULL, 0);

            if (od.od_AddedControls && event.ev_Type == EVENT_TYPE_BUTTON_DOWN)
            {
                if ((event.ev_Button == EVENT_BUTTON_SELECT)
                 || (event.ev_Button == EVENT_BUTTON_STOP))
                {
                    if (od.od_OverlayDisplayed)
                    {
                        SelectControl(req, &od.od_Button);
                        selected = TRUE;
                    }
                }
            }
            else if (od.od_AddedControls && event.ev_Type == EVENT_TYPE_BUTTON_UP)
            {
                if ((event.ev_Button == EVENT_BUTTON_SELECT) ||
                    (event.ev_Button == EVENT_BUTTON_STOP))
                {
                    if (od.od_OverlayDisplayed)
                    {
                        if (selected)
                        {
                            UnselectControl(req, &od.od_Button);

                            StopIOService(req->sr_IOServer);
                            if (statusMsg > 0)
                            {
                                ReplyMsg(statusMsg, statusReply, NULL, 0);
                                statusMsg = -1;
                            }
                            exiting = TRUE;
                        }
                    }
                }
            }
            else if (event.ev_Type == EVENT_TYPE_FSCHANGE)
            {
                BlinkFirstBox(req, &req->sr_Boxes);
                req->sr_FSChanged = TRUE;

                if (statusMsg > 0)
                {
                    if (recoverable)
                    {
                        RemoveOverlay(req, &od, ctrl);
                        ReplyMsg(statusMsg, 0, NULL, 0);
                        statusMsg = -1;
                        sound     = -1;
                        SampleSystemTimeTV(&startTime);
                    }
                }
            }
        }
    }
}
