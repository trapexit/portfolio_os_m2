/* @(#) copy_loop.c 96/10/30 1.1 */

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
#include "dirscan.h"
#include "framebuf.h"
#include "highlight.h"
#include "animlists.h"
#include "overlays.h"
#include "boxes.h"
#include "sound.h"
#include "eventloops.h"


/*****************************************************************************/


Err CopyLoop(StorageReq *req, const CopyStats *stats, const Control *copyControl)
{
Control   		okButton;
Control   		cancelButton;
Control   		anim;
AnimList  		animList;
Highlight 		highlight;
Overlay   		overlay;
Item      		eventMsg;
Event     		event;
int32     		sigs;
Corner    		corners[NUM_HIGHLIGHT_CORNERS];
bool      		ok;
bool      		selected;
Err       		result;
int16     		buttonWidth;
int16     		buttonHeight;
int16     		buttonX;
int16     		buttonY;
bool      		addedControls;
int16     		animX;
int16     		animY;
int16     		timer;

	result = PrepOverlay(req, &overlay, OVL_CONFIRMCOPY,
						 copyControl->ctrl_X + (copyControl->ctrl_Width / 2),
						 copyControl->ctrl_Y + (copyControl->ctrl_Height / 2),
						 req->sr_MovePending ? MSG_MOVE : MSG_COPY,
						 stats->dirCount, 
						 stats->fileCount, 
						 stats->fileBytes);
	
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

    ok              = TRUE;
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
                if (addedControls)
                {
                    timer--;
                    if (timer == 0)
                    {
                        timer = 75;
                        if (anim.ctrl_Highlighted)
                            UnhighlightControl(req, &anim);
                        else
                            HighlightControl(req, &anim);
                    }
                }
                else
                {
                    PlaySound(req, SOUND_QUESTION);

                    PrepControl(req, &anim, 0, 0, CTRL_TYPE_OVL_QUESTION);
                    animX = overlay.ov_X + overlay.ov_Width - anim.ctrl_Width - 6;
                    animY = overlay.ov_Y + 21;
                    PositionControl(req, &anim, animX, animY);
                    InsertAnimNode(&animList, (AnimNode *)&anim);

                    PrepControl(req, &cancelButton, buttonX, buttonY, CTRL_TYPE_OVL_CANCEL);
                    InsertAnimNode(&animList, (AnimNode *)&cancelButton);

                    PrepControl(req, &okButton, buttonX - buttonWidth, buttonY, req->sr_MovePending ? CTRL_TYPE_OVL_MOVE : CTRL_TYPE_OVL_COPY);
                    InsertAnimNode(&animList, (AnimNode *)&okButton);

                    PrepHighlight(req, &highlight);
                    highlight.hl.an.n_Priority = 200;
                    InsertAnimNode(&animList, (AnimNode *)&highlight);

                    if (ok)
                    {
                        GetControlCorners(&okButton, corners);
                        HighlightControl(req, &okButton);
                    }
                    else
                    {
                        GetControlCorners(&cancelButton, corners);
                        HighlightControl(req, &cancelButton);
                    }

                    MoveHighlightNow(req, &highlight, corners);
                    DisableHighlightNow(req, &highlight);
                    EnableHighlight(req, &highlight);

                    HighlightControl(req, &anim);

                    addedControls = TRUE;
                    timer = 75;
                }
            }
        }

        eventMsg = GetMsg(req->sr_EventPort);
        if (eventMsg <= 0)
            continue;

        event = *(Event *)(MESSAGE(eventMsg)->msg_DataPtr);
        ReplyMsg(eventMsg,0,NULL,0);

        if (addedControls && event.ev_Type == EVENT_TYPE_BUTTON_DOWN)
        {
            if (event.ev_Button == EVENT_BUTTON_SELECT)
            {
                if (ok)
                    SelectControl(req, &okButton);
                else
                    SelectControl(req, &cancelButton);

                selected = TRUE;
            }
            else if (event.ev_Button == EVENT_BUTTON_STOP)
            {
                if (ok)
                {
                    UnselectControl(req, &okButton);
                    UnhighlightControl(req, &okButton);
                    GetControlCorners(&cancelButton, corners);
                    MoveHighlight(req, &highlight, corners);
                    HighlightControl(req, &cancelButton);
                }

                SelectControl(req, &cancelButton);
                ok     = FALSE;
                selected = TRUE;
            }
        }
        else if (addedControls && event.ev_Type == EVENT_TYPE_BUTTON_UP)
        {
            if ((event.ev_Button == EVENT_BUTTON_SELECT) || (event.ev_Button == EVENT_BUTTON_STOP))
            {
                if (ok)
                    UnselectControl(req, &okButton);
                else
                    UnselectControl(req, &cancelButton);

                if (selected)
                    break;
            }
        }
        else if (addedControls && event.ev_Type == EVENT_TYPE_RELATIVE_MOVE)
        {
            if (event.ev_X > 0)
            {
                if (ok)
                {
                    UnselectControl(req, &okButton);
                    UnhighlightControl(req, &okButton);
                    GetControlCorners(&cancelButton, corners);
                    MoveHighlight(req, &highlight, corners);
                    HighlightControl(req, &cancelButton);
                    ok     = FALSE;
                    selected = FALSE;
                }
            }
            else if (event.ev_X < 0)
            {
                if (!ok)
                {
                    UnselectControl(req, &cancelButton);
                    UnhighlightControl(req, &cancelButton);
                    GetControlCorners(&okButton, corners);
                    MoveHighlight(req, &highlight, corners);
                    HighlightControl(req, &okButton);
                    ok     = TRUE;
                    selected = FALSE;
                }
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
        RemNode((Node *)&anim);
        RemNode((Node *)&okButton);
        RemNode((Node *)&cancelButton);
        RemNode((Node *)&highlight);

        UnprepControl(req,   &anim);
        UnprepControl(req,   &okButton);
        UnprepControl(req,   &cancelButton);
        UnprepHighlight(req, &highlight);
    }

    MoveOverlay(req, &overlay, copyControl->ctrl_X + (copyControl->ctrl_Width / 2),
                               copyControl->ctrl_Y + (copyControl->ctrl_Height / 2));

    while (!overlay.ov_Stabilized)
    {
        WaitSignal(req->sr_RenderSig);
        DoNextFrame(req, &req->sr_AnimList);
    }

    RemNode((Node *)&animList);
    UnprepAnimList(req, &animList);

    if (ok)
        return 0;

    return -1;
}

/*****************************************************************************/

