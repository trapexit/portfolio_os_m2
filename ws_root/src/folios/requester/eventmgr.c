/* @(#) eventmgr.c 96/10/08 1.5 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/msgport.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <kernel/debug.h>
#include <kernel/task.h>
#include <misc/event.h>
#include <ui/requester.h>
#include <stdio.h>
#include <string.h>
#include "eventmgr.h"


/*****************************************************************************/


struct EventMgr
{
    Item   em_Daemon;
    Item   em_MsgPort;
    uint32 em_NumRepeats;
};


/*****************************************************************************/


#define REPEAT_DELAY    600000
#define REPEAT_INTERVAL 100000

/*****************************************************************************/

/*
   About DEVCHG_DELAY, a hack in 3-part harmony...
	Normally, we want to know about filesystem mount and dismount events, which
	the event broker can tell us about just fine.  When a filesystem is mounted
	or dismounted, we send a client event so that the directory hierarchy and
	file list can be rebuilt if a storage card is inserted or removed.  The 
	mount/dismount notification from the broker is timely; it comes in after
	the mount or dismount activity is fully completed.  One glitch that makes
	this whole concept a mess, however, happens when an unformatted storage card
	is inserted; it won't mount because it isn't formatted.  To detect this
	sort of event, we also have to ask broker to tell us about DeviceChanged
	events.  The DeviceChanged will come in immediately upon card insert/remove,
	before any mount/dismount activity occurs.  If a DeviceChanged is going to
	be followed immediately by a mount/dismount, we need to ignore the DeviceChanged
	(our attempts to query the filesystem on a DeviceChange event can actually
	interfere with the dismount process!)  If we get a DeviceChanged that
	isn't followed by a mount/dismount, we need to handle the DeviceChanged as
	if it were a mount/dismount.  That's the background to all this.
	
	To implement the basic concept (respond to a DeviceChanged only if it isn't
	quickly followed by a mount or dismount), we respond to a DeviceChanged by
	setting a timer that will signal us after DEVCHG_DELAY microseconds.  When
	we get a mount or dismount, we cancel the devchg timer and send the client
	an event based on the mount/dismount.  If we don't receive a mount or 
	dismount within DEVCHG_DELAY microseconds, we send the client an event
	based on the timer expiring.
	
	I initially thought about 1/10 of a second might be a good delay, but it 
	looks like more than a second is needed.  I suspect this might be because of
	debugging printfs currently in the acro filesystem; it might be possible
	to shorten this delay a bit when the printfs are removed.
*/

#define DEVCHG_DELAY_SECS	1
#define DEVCHG_DELAY_USECS	600000


/*****************************************************************************/


typedef struct
{
    uint32 es_ButtonState;
    uint32 es_PreviousPadBits;

    uint32 es_PreviousStickBits;
    uint32 es_PreviousStickX;
    uint32 es_PreviousStickY;
	
	uint32 es_RepeatDelay;
	uint32 es_RepeatInterval;
} EventState;

typedef struct
{
    uint32 m_Button;
    uint32 m_Flag;
} ButtonMap;


/*****************************************************************************/


static const ButtonMap controlPadEventMap[] =
{
    {ControlA,          EVENT_BUTTON_SELECT},
    {ControlB,          EVENT_BUTTON_NOP},
    {ControlC,          EVENT_BUTTON_INFO},
    {ControlD,          EVENT_BUTTON_NOP},
    {ControlE,          EVENT_BUTTON_NOP},
    {ControlF,          EVENT_BUTTON_NOP},
    {ControlX,          EVENT_BUTTON_STOP},
    {ControlRightShift, EVENT_BUTTON_SHIFT},
    {ControlLeftShift,  EVENT_BUTTON_SHIFT},
    {0,0}
};

static const ButtonMap stickEventMap[] =
{
    {StickFire,       EVENT_BUTTON_SELECT},
    {StickA,          EVENT_BUTTON_SELECT},
    {StickB,          EVENT_BUTTON_NOP},
    {StickC,          EVENT_BUTTON_INFO},
    {StickStop,       EVENT_BUTTON_STOP},
    {StickRightShift, EVENT_BUTTON_SHIFT},
    {StickLeftShift,  EVENT_BUTTON_SHIFT},
    {0,0}
};

#if 0
static const ButtonMap mouseEventMap[] =
{
    {MouseLeft,   EVENT_BUTTON_SELECT},
    {MouseMiddle, EVENT_BUTTON_NOP},
    {MouseRight,  EVENT_BUTTON_INFO},
    {MouseShift,  EVENT_BUTTON_NOP},
    {0,0}
};
#endif


/*****************************************************************************/


static Item CreateTimerSignalIOReq(int32 signal)
{
Err 	err;
Item	ioreq	= 0;
Item	device	= 0;
List *	list	= NULL;

/*	
	Open the timer device and create an IOReq for it.  We don't use the 
	convenience routines from the OS libs because we need to tie a specific 
	signal to the IOReq and there's no convenience routine to do that.
	
	Note:  We don't need a timer device that supports all the commands listed
	in the CreateDeviceStackList call, but if you make the call asking only
	for TIMERCMD_DELAY_USEC it returns an empty list.  An OS bug, I suspect.
*/

    err = CreateDeviceStackListVA(&list,
		"cmds", DDF_EQ, DDF_INT, 10,
			TIMERCMD_GETTIME_VBL, 
			TIMERCMD_SETTIME_VBL,
			TIMERCMD_DELAY_VBL, 
			TIMERCMD_DELAYUNTIL_VBL,
			TIMERCMD_METRONOME_VBL,
			TIMERCMD_GETTIME_USEC, 
			TIMERCMD_SETTIME_USEC,
			TIMERCMD_DELAY_USEC, 
			TIMERCMD_DELAYUNTIL_USEC,
			TIMERCMD_METRONOME_USEC,
		NULL);
			
    if (err < 0) {
		goto CLEANUP;
	}
	
    if (IsEmptyList(list)) {
		err = MAKEKERR(ER_SEVER,ER_C_STND,ER_NoHardware);
		goto CLEANUP;
    }
	
	device = OpenDeviceStack((DeviceStack *)FirstNode(list));
    
	if (device < 0) {
        err = device;
		goto CLEANUP;
	}
	
    ioreq = CreateItemVA(MKNODEID(KERNELNODE, IOREQNODE), 
				CREATEIOREQ_TAG_DEVICE, device,
				(signal > 0) ? CREATEIOREQ_TAG_SIGNAL : TAG_NOP, signal,
				TAG_END);
    
	if (ioreq < 0) {
		err = ioreq;
		goto CLEANUP;
	}

	err = 0;

CLEANUP:

	if (list) {	
		DeleteDeviceStackList(list);
	}
	
	if (err < 0) {
		if (ioreq > 0) {
			DeleteIOReq(ioreq);
		}
		if (device > 0) {
			CloseDeviceStack(device);
		}
		ioreq = err;
	}

    return ioreq;
}


/*****************************************************************************/


static Err SignalTime(Item ioreq, uint32 seconds, uint32 micros)
{
IOInfo  ioInfo;
TimeVal tv;

    memset(&ioInfo,0,sizeof(IOInfo));

    tv.tv_Seconds              = seconds;
    tv.tv_Microseconds         = micros;
    ioInfo.ioi_Command         = TIMERCMD_DELAY_USEC;
    ioInfo.ioi_Send.iob_Buffer = &tv;
    ioInfo.ioi_Send.iob_Len    = sizeof(TimeVal);

    return SendIO(ioreq, &ioInfo);
}


/*****************************************************************************/


static uint32 EventMap(uint32 *deltaBits, const ButtonMap *map)
{
uint32 i;

    i = 0;
    while (map[i].m_Button)
    {
        if (*deltaBits & map[i].m_Button)
        {
            *deltaBits &= ~(map[i].m_Button);
            return map[i].m_Flag;
        }
        i++;
    }

    return EVENT_BUTTON_NOP;
}


/*****************************************************************************/


static Item SendClientEvent(Item clientPort, Event *event)
{
Item msg;

    msg = GetMsg(CURRENTTASK->t_DefaultMsgPort);
    if (msg <= 0)
        msg = CreateBufferedMsg(NULL,0,CURRENTTASK->t_DefaultMsgPort,sizeof(Event));

    if (msg >= 0)
    {
        if (SendMsg(clientPort, msg, event, sizeof(Event)) < 0)
        {
             SendMsg(CURRENTTASK->t_DefaultMsgPort, msg, NULL, 0);
        }
    }

    return msg;
}


/*****************************************************************************/


static void ProcessFrame(EventState *state, EventFrame *frame,
                         Item clientPort, Event *event, Item devchgTimerIO, int32 devchgTimerSignal)
{
ControlPadEventData *cpd;
StickEventData      *sd;
uint32               updateBits;
uint32               newBits;
uint32               goneBits;
uint32               oldRepeatInterval;
uint32               oldRepeatDelay;
uint32               horizPos;
uint32               vertPos;

    cpd = (ControlPadEventData *)&frame->ef_EventData[0];
    sd  = (StickEventData *)&frame->ef_EventData[0];

    event->ev_Type = EVENT_TYPE_NOP;
    event->ev_X    = 0;
    event->ev_Y    = 0;

    oldRepeatDelay  	  	= state->es_RepeatDelay;
    oldRepeatInterval 		= state->es_RepeatInterval;
    state->es_RepeatDelay   = 0;
    state->es_RepeatInterval= 0;

    switch (frame->ef_EventNumber)
    {
        case EVENTNUM_ControlButtonUpdate:

              updateBits             = cpd->cped_ButtonBits & (ControlUp | ControlDown | ControlLeft | ControlRight | ControlA | ControlB | ControlC | ControlD | ControlE | ControlF | ControlX | ControlLeftShift | ControlRightShift);
              newBits                = updateBits & (~state->es_PreviousPadBits);
              goneBits               = (~updateBits) & state->es_PreviousPadBits;
              state->es_PreviousPadBits = updateBits;

              /* send messages for any buttons that were released */
              event->ev_Type = EVENT_TYPE_BUTTON_UP;
              while (TRUE)
              {
                  event->ev_Button = EventMap(&goneBits, controlPadEventMap);
                  if (event->ev_Button == EVENT_BUTTON_NOP)
                      break;

                  event->ev_ButtonState &= ~(1 << event->ev_Button);
                  state->es_ButtonState  = event->ev_ButtonState;
                  SendClientEvent(clientPort, event);
              }

              /* send messages for any buttons that were pressed */
              event->ev_Type = EVENT_TYPE_BUTTON_DOWN;
              while (TRUE)
              {
                  event->ev_Button = EventMap(&newBits, controlPadEventMap);
                  if (event->ev_Button == EVENT_BUTTON_NOP)
                      break;

                  event->ev_ButtonState |= (1 << event->ev_Button);
                  state->es_ButtonState  = event->ev_ButtonState;
                  SendClientEvent(clientPort, event);
              }

              /* send messages for any directional buttons */
              if (newBits & (ControlUp | ControlDown | ControlLeft | ControlRight))
              {
                  if (newBits & ControlUp)
                      event->ev_Y = -1;
                  else if (newBits & ControlDown)
                      event->ev_Y = 1;

                  if (newBits & ControlLeft)
                      event->ev_X = -1;
                  else if (newBits & ControlRight)
                      event->ev_X = 1;

                  event->ev_Type        = EVENT_TYPE_RELATIVE_MOVE;
                  event->ev_ButtonState = state->es_ButtonState;

                  SendClientEvent(clientPort, event);
                  state->es_RepeatDelay    = REPEAT_DELAY;
                  state->es_RepeatInterval = REPEAT_INTERVAL;
              }
              break;

        case EVENTNUM_StickUpdate:

              updateBits             = sd->stk_ButtonBits & (StickFire | StickA | StickB | StickC | StickStop | StickLeftShift | StickRightShift);
              newBits                = updateBits & (~state->es_PreviousStickBits);
              goneBits               = (~updateBits) & state->es_PreviousStickBits;
              state->es_PreviousStickBits = updateBits;

              /* send messages for any buttons that were released */
              event->ev_Type = EVENT_TYPE_BUTTON_UP;
              while (TRUE)
              {
                  event->ev_Button = EventMap(&goneBits, stickEventMap);
                  if (event->ev_Button == EVENT_BUTTON_NOP)
                      break;

                  event->ev_ButtonState &= ~(1 << event->ev_Button);
                  state->es_ButtonState  = event->ev_ButtonState;
                  SendClientEvent(clientPort, event);
              }

              /* send messages for any buttons that were pressed */
              event->ev_Type = EVENT_TYPE_BUTTON_DOWN;
              while (TRUE)
              {
                  event->ev_Button = EventMap(&newBits, stickEventMap);
                  if (event->ev_Button == EVENT_BUTTON_NOP)
                      break;

                  event->ev_ButtonState |= (1 << event->ev_Button);
                  state->es_ButtonState  = event->ev_ButtonState;
                  SendClientEvent(clientPort, event);
              }
              break;

        case EVENTNUM_StickMoved:

              horizPos = (sd->stk_HorizPosition + 255) / 256;
              vertPos  = (sd->stk_VertPosition + 255) / 256;

              if (horizPos != state->es_PreviousStickX)
              {
                  if (horizPos < 2)
                      event->ev_X = -1;
                  else if (horizPos > 2)
                      event->ev_X = 1;

                  state->es_PreviousStickX = sd->stk_HorizPosition;
              }

              if (vertPos != state->es_PreviousStickY)
              {
                  if (vertPos < 2)
                      event->ev_Y = -1;
                  else if (vertPos > 2)
                      event->ev_Y = 1;

                  state->es_PreviousStickY = sd->stk_VertPosition;
              }

              if (event->ev_X || event->ev_Y)
              {
                  event->ev_Type        = EVENT_TYPE_RELATIVE_MOVE;
                  event->ev_ButtonState = state->es_ButtonState;

                  SendClientEvent(clientPort, event);

                  if (oldRepeatInterval)
                  {
                      state->es_RepeatDelay    = oldRepeatDelay;
                      state->es_RepeatInterval = REPEAT_INTERVAL;
                  }
                  else
                  {
                      state->es_RepeatDelay    = REPEAT_DELAY;
                      state->es_RepeatInterval = REPEAT_INTERVAL;
                  }
              }
              break;

        case EVENTNUM_DeviceChanged: 
        case EVENTNUM_FilesystemOffline: 
		
			  SignalTime(devchgTimerIO, DEVCHG_DELAY_SECS, DEVCHG_DELAY_USECS);
			  break;
			  
        case EVENTNUM_FilesystemMounted:
        case EVENTNUM_FilesystemDismounted:
		
			  AbortIO(devchgTimerIO);
			  WaitIO(devchgTimerIO);
			  ClearCurrentSignals(devchgTimerSignal);
			  
			  event->ev_Type = EVENT_TYPE_FSCHANGE;
			  SendClientEvent(clientPort, event);
			  break;
    }
}


/*****************************************************************************/


static void EventMgrDaemon(Item clientPort)
{
Item                 startupMsg;
EventState           state;
Err             	 result;
ConfigurationRequest config;
Item                 configMsg;
Item                 eventMsg;
EventBrokerHeader   *hdr;
EventFrame          *frame;
Event                event;
Item                 daemonPort;
Item                 repeatTimerIO;
Item				 devchgTimerIO;
int32                sigs;
int32                daemonSignal;
int32                repeatMetronomeSignal;
int32				 repeatTimerSignal;
int32				 devchgTimerSignal;
Item                 lastRepeatMsg;

    /* pull the startup message out of the port */
    startupMsg = WaitPort(CURRENTTASK->t_DefaultMsgPort, 0);

    devchgTimerSignal	  = AllocSignal(0);
	repeatTimerSignal	  = AllocSignal(0);
    repeatMetronomeSignal = AllocSignal(0);
	
    memset(&state,0,sizeof(state));

	devchgTimerIO		= -1;
    repeatTimerIO      	= -1;
    daemonSignal       	= 0;
    lastRepeatMsg      	= -1;

    daemonPort = result = CreateMsgPort(NULL,0,0);
    if (daemonPort >= 0)
    {
        daemonSignal = MSGPORT(daemonPort)->mp_Signal;

        repeatTimerIO = result = CreateTimerSignalIOReq(repeatTimerSignal);
        if (repeatTimerIO >= 0)
        {
	        devchgTimerIO = result = CreateTimerSignalIOReq(devchgTimerSignal);
			if (devchgTimerIO >= 0)
			{
				configMsg = result = CreateMsg(NULL, 0, daemonPort);
				if (configMsg >= 0)
				{
					memset(&config, 0, sizeof(config));
					config.cr_Header.ebh_Flavor = EB_Configure;
					config.cr_Category          = LC_FocusListener;
					config.cr_QueueMax          = 0;
					config.cr_TriggerMask[0]    = EVENTBIT0_ControlButtonUpdate;
					config.cr_TriggerMask[2]    = EVENTBIT2_FilesystemMounted |
												  EVENTBIT2_FilesystemOffline |
												  EVENTBIT2_FilesystemDismounted |
												  EVENTBIT2_DeviceChanged;
	
					result = SendMsg(FindMsgPort(EventPortName), configMsg, &config, sizeof(config));
					if (result >= 0)
					{
						WaitPort(daemonPort, configMsg);
						result = MESSAGE(configMsg)->msg_Result;
					}
					DeleteMsg(configMsg);
				}
			}
        }
    }

    ReplyMsg(startupMsg,result,NULL,0);
    if (result < 0)
        return;

    /* we're in business, start processing incoming messages */

    memset(&state, 0, sizeof(state));

    while (TRUE)
    {
        sigs = WaitSignal(daemonSignal | devchgTimerSignal | repeatTimerSignal | repeatMetronomeSignal);
		
		if (sigs & devchgTimerSignal)
		{
			event.ev_Type = EVENT_TYPE_FSCHANGE;
			SendClientEvent(clientPort, &event);
		}
		
        if (sigs & repeatTimerSignal)
        {
            StartMetronome(repeatTimerIO, 0, state.es_RepeatInterval, repeatMetronomeSignal);
            event.ev_Qualifiers |= EVENT_QUAL_REPEAT;
            lastRepeatMsg = SendClientEvent(clientPort, &event);
        }
        else if (sigs & repeatMetronomeSignal)
        {
            /* only send a repeated event if the previous one has been returned */
            if (MESSAGE(lastRepeatMsg)->msg_Holder == CURRENTTASK->t_DefaultMsgPort)
                lastRepeatMsg = SendClientEvent(clientPort, &event);
        }

        eventMsg = GetMsg(daemonPort);
        if (eventMsg > 0)
        {
            do
            {
                hdr = (EventBrokerHeader *)MESSAGE(eventMsg)->msg_DataPtr;
                if (hdr->ebh_Flavor == EB_EventRecord)
                {
                    frame = (EventFrame *)&hdr[1];
                    while (frame->ef_ByteCount)
                    {
                        ProcessFrame(&state, frame, clientPort, &event, devchgTimerIO, devchgTimerSignal);
                        frame = (EventFrame *) (frame->ef_ByteCount + (char *) frame);
                    }
                }
                ReplyMsg(eventMsg,0,NULL,0);

                eventMsg = GetMsg(daemonPort);
            }
            while (eventMsg > 0);

            AbortIO(repeatTimerIO);
            WaitIO(repeatTimerIO);
            ClearCurrentSignals(repeatTimerSignal);

            if (state.es_RepeatDelay)
                SignalTime(repeatTimerIO, 0, state.es_RepeatDelay);
        }
    }
}


/*****************************************************************************/


Err ConnectEventMgr(EventMgr **mgr, Item msgPort)
{
Item thread;
Item port;
Item msg;
Err  result;

    *mgr = AllocMem(sizeof(EventMgr), MEMTYPE_FILL);
    if (*mgr)
    {
        port = result = CreateMsgPort(NULL, 0, 0);
        if (port >= 0)
        {
            msg = result = CreateSmallMsg(NULL, 0, port);
            if (msg >= 0)
            {
                thread = result = CreateThreadVA(EventMgrDaemon, "EventMgr Daemon", 150, 4096,
                                      CREATETASK_TAG_ARGC,           msgPort,
                                      CREATETASK_TAG_DEFAULTMSGPORT, 0,
                                      TAG_END);

                if (thread >= 0)
                {
                    result = SendSmallMsg(THREAD(thread)->t_DefaultMsgPort, msg, 0, 0);
                    if (result >= 0)
                    {
                        WaitPort(port,msg);
                        result = MESSAGE(msg)->msg_Result;
                    }

                    if (result < 0)
                    {
                        DeleteThread(thread);
                    }
                    else
                    {
                        (*mgr)->em_Daemon  = thread;
                        (*mgr)->em_MsgPort = msgPort;
                    }
                }
                DeleteMsg(msg);
            }
            DeleteMsgPort(port);
        }

        if (result < 0)
            FreeMem(*mgr, sizeof(EventMgr));
    }
    else
    {
        result = REQ_ERR_NOMEM;
    }

    return result;
}


/*****************************************************************************/


void DisconnectEventMgr(EventMgr *mgr)
{
    if (mgr)
    {
        DeleteThread(mgr->em_Daemon);
        FreeMem(mgr, sizeof(EventMgr));
    }
}
