
/******************************************************************************
**
**  @(#) EventUtility.c 96/10/08 1.14
**
**  Convenience interface routines for the event broker.
**
******************************************************************************/

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/msgport.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <misc/event.h>
#include <stdlib.h>
#include <string.h>


/****************************************************************************/


typedef struct
{
    ControlPadEventData pad_Data;
    bool                pad_Avail;
} PadInfo;

typedef struct
{
    MouseEventData mouse_Data;
    bool           mouse_Avail;
} MouseInfo;

static Item       msgPortItem;
static uint8      numPads;
static uint8      numMice;
static MouseInfo *mice;
static PadInfo   *pads;


/****************************************************************************/


Err InitEventUtility(uint8 numberPads, uint8 numberMice, bool isFocused)
{
Err    result;
Item   msgItem;
Item   brokerPortItem;
uint32 mouseSize;
uint32 padSize;
ConfigurationRequest config;

    numPads = numberPads;
    numMice = numberMice;

    if (numPads + numMice == 0)
        return EVENTUTILITY_ERR_BADCONTROLLERNUM;

    brokerPortItem = result = FindMsgPort(EventPortName);
    if (brokerPortItem >= 0)
    {
        msgPortItem = result = CreateMsgPort(CURRENTTASK->t.n_Name, 0, 0);
        if (msgPortItem >= 0)
        {
            msgItem = result = CreateMsg(NULL, 0, msgPortItem);
            if (msgItem >= 0)
            {
                padSize   = numPads * sizeof(PadInfo);
                mouseSize = numMice * sizeof(MouseInfo);

                pads = AllocMem(padSize + mouseSize, MEMTYPE_FILL | MEMTYPE_TRACKSIZE);
                if (pads)
                {
                    mice = (MouseInfo *)((uint32)pads + padSize);

                    memset(&config, 0, sizeof(config));
                    config.cr_Header.ebh_Flavor = EB_Configure;
                    config.cr_Category          = (isFocused ? LC_FocusListener : LC_Observer);
                    config.cr_TriggerMask[0]    = EVENTBIT0_ControlButtonUpdate |
                                                 EVENTBIT0_MouseUpdate |
                                                  EVENTBIT0_MouseMoved ;
                    config.cr_TriggerMask[1]    = EVENTBIT1_ControlSettingChanged;

                    result = SendMsg(brokerPortItem, msgItem, &config, sizeof(config));
                    if (result >= 0)
                    {
                        result = WaitPort(msgPortItem, msgItem);
                        if (result >= 0)
                        {
                            result = MESSAGE(msgItem)->msg_Result;
                            if (result >= 0)
                            {
                                DeleteMsg(msgItem);
                                return 0;
                            }
                        }
                    }
                    FreeMem(pads, TRACKED_SIZE);
                }
                else
                {
                    result = EVENTUTILITY_ERR_NOMEM;
                }
                DeleteMsg(msgItem);
            }
            DeleteMsgPort(msgPortItem);
        }
    }

    return result;
}


/****************************************************************************/


Err KillEventUtility(void)
{
    FreeMem(pads, TRACKED_SIZE);
    DeleteMsgPort(msgPortItem);

    pads        = NULL;
    mice        = NULL;
    msgPortItem = -1;

    return 0;
}


/****************************************************************************/


static Err ProcessNextEvent(bool wait)
{
Item               event;
uint8              pos;
EventBrokerHeader *hdr;
EventFrame        *frame;

    if (wait)
        event = WaitPort(msgPortItem, 0);
    else
        event = GetMsg(msgPortItem);

    if (event <= 0)
        return event;

    hdr = MESSAGE(event)->msg_DataPtr;
    if (hdr->ebh_Flavor == EB_EventRecord)
    {
        frame = (EventFrame *) (hdr + 1);
        while (frame->ef_ByteCount)
        {
            pos = frame->ef_GenericPosition;
            if (pos)
            {
                pos--;
                switch (frame->ef_EventNumber)
                {
                    case EVENTNUM_ControlButtonUpdate:
                    case EVENTNUM_ControlSettingChanged:
                    {
                        if (pos < numPads)
                        {
                            memcpy(&pads[pos].pad_Data, frame->ef_EventData, sizeof(ControlPadEventData));
                            pads[pos].pad_Avail = TRUE;
                        }
                        break;
                    }

                    case EVENTNUM_MouseUpdate:
                    case EVENTNUM_MouseMoved:
                    {
                        if (pos < numMice)
                        {
                            memcpy(&mice[pos].mouse_Data, frame->ef_EventData, sizeof(MouseEventData));
                            mice[pos].mouse_Avail = TRUE;
                        }
                        break;
                    }
                }
            }
            frame = (EventFrame *)(frame->ef_ByteCount + (uint32)frame);
        }
    }
    ReplyMsg(event, 0, NULL, 0);
    return 0;
}


/****************************************************************************/


Err GetControlPad(uint8 padNumber, bool wait, ControlPadEventData *data)
{
Err      result;
PadInfo *pad;

    if ((padNumber < 1) || (padNumber > numPads))
        return EVENTUTILITY_ERR_BADCONTROLLERNUM;

    if (!pads)
        return EVENTUTILITY_ERR_NOTINITED;

    pad = &pads[padNumber-1];
    while (TRUE)
    {
        if (pad->pad_Avail)
        {
            pad->pad_Avail = FALSE;
            *data = pad->pad_Data;
            return 1;
        }

        result = ProcessNextEvent(wait);
        if (result < 0)
            return result;

        if ((result == 0) && !wait && !pad->pad_Avail)
        {
            *data = pad->pad_Data;
            return 0;
        }
    }
}


/*****************************************************************************/


Err GetMouse(uint8 mouseNumber, bool wait, MouseEventData *data)
{
Err        result;
MouseInfo *mouse;

    if ((mouseNumber < 1) || (mouseNumber > numMice))
        return EVENTUTILITY_ERR_BADCONTROLLERNUM;

    if (!mice)
        return EVENTUTILITY_ERR_NOTINITED;

    mouse = &mice[mouseNumber-1];
    while (TRUE)
    {
        if (mouse->mouse_Avail)
        {
            mouse->mouse_Avail = FALSE;
            *data = mouse->mouse_Data;
            return 1;
        }

        result = ProcessNextEvent(wait);
        if (result < 0)
            return result;

        if ((result == 0) && !wait && !mouse->mouse_Avail)
        {
            *data = mouse->mouse_Data;
            return 0;
        }
    }
}
