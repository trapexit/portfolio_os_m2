/* @(#) info_loop.c 96/10/30 1.2 */

#include <kernel/types.h>
#include <kernel/list.h>
#include <kernel/task.h>
#include <ui/requester.h>
#include <graphics/frame2d/spriteobj.h>
#include <stdio.h>
#include <string.h>
#include "req.h"
#include "eventmgr.h"
#include "controls.h"
#include "framebuf.h"
#include "highlight.h"
#include "animlists.h"
#include "overlays.h"
#include "sound.h"
#include "eventloops.h"


/*****************************************************************************/


Err InfoLoop(StorageReq *req, const List *infoPairs)
{
Control   okButton;
AnimList  animList;
Highlight highlight;
Overlay   overlay;
Item      eventMsg;
Event     event;
int32     sigs;
Corner    corners[NUM_HIGHLIGHT_CORNERS];
bool      selected;
Err       result;
int16     buttonWidth;
int16     buttonHeight;
int16     buttonX;
int16     buttonY;
bool      addedControls;
int16     startX;
int16     startY;

    startX = req->sr_ListView.lv_X + (req->sr_ListView.lv_Width / 2),
    startY = req->sr_ListView.lv_Y + (req->sr_ListView.lv_Height / 2),

    result = PrepOverlay(req, &overlay, OVL_INFO,
                         startX, startY, infoPairs);
    if (result < 0)
        return result;

    PrepAnimList(req, &animList);
    animList.al.an.n_Priority = 200;
    InsertAnimNode(&req->sr_AnimList, (AnimNode *)&animList);

    overlay.ov.an.n_Priority = 0;
    InsertAnimNode(&animList, (AnimNode *)&overlay);

    buttonWidth     = Spr_GetWidth(req->sr_ButtonImages[0]);
    buttonHeight    = Spr_GetHeight(req->sr_ButtonImages[0]);
    buttonX         = overlay.ov_X + overlay.ov_Width - 1 - buttonWidth - 4;
    buttonY         = overlay.ov_Y + overlay.ov_Height - 1 - buttonHeight;

    selected        = FALSE;
    addedControls   = FALSE;

    while (TRUE)
    {
        sigs = WaitSignal(req->sr_RenderSig | MSGPORT(req->sr_EventPort)->mp_Signal);

        if (req->sr_RenderSig & sigs)
        {
            DoNextFrame(req, &req->sr_AnimList);

            if (overlay.ov_Stabilized)
            {
                if (!addedControls)
                {
                    PrepControl(req, &okButton, buttonX, buttonY, CTRL_TYPE_OVL_OK);
                    InsertAnimNode(&animList, (AnimNode *)&okButton);

                    PrepHighlight(req, &highlight);
                    highlight.hl.an.n_Priority = 200;
                    InsertAnimNode(&animList, (AnimNode *)&highlight);

                    GetControlCorners(&okButton, corners);
                    HighlightControl(req, &okButton);

                    MoveHighlightNow(req, &highlight, corners);
                    DisableHighlightNow(req, &highlight);
                    EnableHighlight(req, &highlight);

                    addedControls = TRUE;
                }
            }
        }

        eventMsg = GetMsg(req->sr_EventPort);
        if (eventMsg <= 0)
            continue;

        event = *(Event *)(MESSAGE(eventMsg)->msg_DataPtr);
        ReplyMsg(eventMsg,0,NULL,0);

        if (event.ev_Type == EVENT_TYPE_BUTTON_DOWN)
        {
            if (event.ev_Button == EVENT_BUTTON_SELECT)
            {
                SelectControl(req, &okButton);
                selected = TRUE;
            }
            else if (event.ev_Button == EVENT_BUTTON_STOP)
            {
                HighlightControl(req, &okButton);
                SelectControl(req, &okButton);
                selected = TRUE;
            }
        }
        else if (event.ev_Type == EVENT_TYPE_BUTTON_UP)
        {
            if ((event.ev_Button == EVENT_BUTTON_SELECT) || (event.ev_Button == EVENT_BUTTON_STOP))
            {
                UnselectControl(req, &okButton);

                if (selected)
                    break;
            }
        }
        else if (event.ev_Type == EVENT_TYPE_FSCHANGE)
        {
            BlinkFirstBox(req, &req->sr_Boxes);
            req->sr_FSChanged = TRUE;
        }
    }

    if (addedControls)
    {
        RemNode((Node *)&okButton);
        RemNode((Node *)&highlight);

        UnprepControl(req,   &okButton);
        UnprepHighlight(req, &highlight);
    }

    MoveOverlay(req, &overlay, startX, startY);

    while (!overlay.ov_Stabilized)
    {
        WaitSignal(req->sr_RenderSig);
        DoNextFrame(req, &req->sr_AnimList);
    }

    RemNode((Node *)&animList);
    UnprepAnimList(req, &animList);

    return 0;
}
