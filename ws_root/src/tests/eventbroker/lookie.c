
/******************************************************************************
**
**  @(#) lookie.c 96/02/21 1.12
**
******************************************************************************/

/**
|||	AUTODOC -class examples -group EventBroker -name lookie
|||	Connects to the event broker and reports any events that occur.
|||
|||	  Format
|||
|||	    lookie [focus | hybrid]
|||
|||	  Description
|||
|||	    Connects to the event broker and requests to be informed about every
|||	    event. Whenever any event occurs, the important data from the event is
|||	    displayed in the Debugger Terminal window.
|||
|||	  Arguments
|||
|||	    [focus]
|||	        Makes lookie a focus listener.
|||
|||	    [hybrid]
|||	        Makes lookie a hybrid listener.
|||
|||	  Associated Files
|||
|||	    lookie.c
|||
|||	  Location
|||
|||	    Examples/EventBroker
|||
**/

#include <kernel/types.h>
#include <kernel/msgport.h>
#include <kernel/operror.h>
#include <kernel/device.h>
#include <kernel/item.h>
#include <kernel/task.h>
#include <misc/event.h>
#include <stdio.h>
#include <string.h>


/*****************************************************************************/


/* display information about a control pad event */
static void DumpControlPad(const EventFrame *frame, const char *action)
{
ControlPadEventData *cped;

    cped = (ControlPadEventData *)frame->ef_EventData;

    printf("  ControlPad %s 0x%08X pod %d position %d generic %d\n",
           action,
           cped->cped_ButtonBits,
           frame->ef_PodNumber,
           frame->ef_PodPosition,
           frame->ef_GenericPosition);

    if (cped->cped_AnalogValid) {
      printf("    stick (%d,%d) shift (%d, %d) shuttle %d\n",
	     cped->cped_StickX,
	     cped->cped_StickY,
	     cped->cped_LeftShift,
	     cped->cped_RightShift,
	     cped->cped_Shuttle);
    }

}


/*****************************************************************************/


/* dump information about a mouse event */
static void DumpMouse(const EventFrame *frame, const char *action)
{
MouseEventData *med;

    med = (MouseEventData *) frame->ef_EventData;

    printf("  Mouse %s 0x%08X at (%d,%d)\n",
           action,
           med->med_ButtonBits,
           med->med_HorizPosition,
           med->med_VertPosition);

    printf("    pod %d position %d generic %d\n",
          frame->ef_PodNumber,
          frame->ef_PodPosition,
          frame->ef_GenericPosition);
}


/*****************************************************************************/


/* dump information about a light gun event */
static void DumpLightGun(const EventFrame *frame, const char *action)
{
LightGunEventData *lged;

    lged = (LightGunEventData *) frame->ef_EventData;

    printf("  LightGun %s 0x%08X pod %d position %d generic %d\n",
	   action,
	   lged->lged_ButtonBits,
	   frame->ef_PodNumber,
	   frame->ef_PodPosition,
           frame->ef_GenericPosition);

    printf("    counter %d line-hits %d\n",
	   lged->lged_Counter,
	   lged->lged_LinePulseCount);
}


/*****************************************************************************/


/* dump information about a joystick event */
static void DumpJoystick(const EventFrame *frame, const char *action)
{
StickEventData *sed;

    sed = (StickEventData *) frame->ef_EventData;

    printf("  Joystick %s 0x%08X pod %d position %d generic %d\n",
	   action,
	   sed->stk_ButtonBits,
	   frame->ef_PodNumber,
	   frame->ef_PodPosition,
           frame->ef_GenericPosition);

    printf("    position(%5d,%5d,%5d)\n",
           sed->stk_HorizPosition,
           sed->stk_VertPosition,
           sed->stk_DepthPosition);
}


/*****************************************************************************/


/* dump information about an IR controller event */
static void DumpIR(const EventFrame *frame, const char *action)
{
IRControllerEventData *ired;

    ired = (IRControllerEventData *) frame->ef_EventData;

    printf("  IR key %s, key-code 0x%08X generic-IR code 0x%08X pod %d position %d generic %d\n",
	   action,
	   ired->ir_KeyCode,
	   ired->ir_GenericCode,
	   frame->ef_PodNumber,
	   frame->ef_PodPosition,
           frame->ef_GenericPosition);
}



/*****************************************************************************/


/* dump information about a filesystem state event */
static void DumpFilesystem(const EventFrame *frame, const char *action)
{
FilesystemEventData *fsed;

    fsed = (FilesystemEventData *) frame->ef_EventData;

    printf("  Filesystem '%s' %s\n", fsed->fsed_Name, action);
}


/*****************************************************************************/


/* dump information about a timing frame */
static void DumpTiming(const EventFrame *frame)
{
DetailedEventTimingData *detd;

    detd = (DetailedEventTimingData *) frame->ef_EventData;

    printf("  VBL %Ld ticks (0x%X,0x%X) scanline %d %sidentical\n",
	   detd->cpih_VBL,
	   detd->cpih_TimerTicks.tt_Hi,
	   detd->cpih_TimerTicks.tt_Lo,
	   detd->cpih_ScanLine,
	   detd->cpih_IsIdentical ? "" : "not ");
}


/*****************************************************************************/


/* dump data on an event record */
static void DumpEventRecord(const EventBrokerHeader *hdr,
                            const EventFrame *frame)
{
    printf("Lookie got an event record at 0x%08X time %Ld:\n", hdr,
           frame->ef_SystemTimeStamp);

    while (frame->ef_ByteCount != 0)
    {
        printf(" Frame of %d bytes:", frame->ef_ByteCount);

        switch (frame->ef_EventNumber)
        {
            case EVENTNUM_ControlButtonPressed:
                DumpControlPad(frame,"buttons pressed");
                break;

            case EVENTNUM_ControlButtonReleased:
                DumpControlPad(frame,"buttons release");
                break;

            case EVENTNUM_ControlButtonUpdate:
                DumpControlPad(frame,"button update");
                break;

            case EVENTNUM_ControlButtonArrived:
                DumpControlPad(frame,"button arrival");
                break;

            case EVENTNUM_ControlSettingChanged:
                DumpControlPad(frame,"settings changed");
                break;

            case EVENTNUM_MouseButtonPressed:
                DumpMouse(frame,"button pressed");
                break;

            case EVENTNUM_MouseButtonReleased:
                DumpMouse(frame,"button released");
                break;

            case EVENTNUM_MouseMoved:
                DumpMouse(frame,"moved");
                break;

            case EVENTNUM_MouseUpdate:
                DumpMouse(frame,"data update");
                break;

	    case EVENTNUM_LightGunButtonPressed:
                DumpLightGun(frame, "buttons pressed");
                break;

            case EVENTNUM_LightGunButtonReleased:
                DumpLightGun(frame, "buttons released");
                break;

            case EVENTNUM_LightGunUpdate:
                DumpLightGun(frame, "update");
                break;

            case EVENTNUM_LightGunFireTracking:
                DumpLightGun(frame, "fire-tracking");
                break;

            case EVENTNUM_StickButtonPressed:
                DumpJoystick(frame, "buttons pressed");
                break;

            case EVENTNUM_StickButtonReleased:
                DumpJoystick(frame, "buttons released");
                break;

            case EVENTNUM_StickUpdate:
                DumpJoystick(frame, "update");
                break;

            case EVENTNUM_StickMoved:
                DumpJoystick(frame, "moved");
                break;

            case EVENTNUM_IRKeyPressed:
                DumpIR(frame, "pressed");
                break;

            case EVENTNUM_IRKeyReleased:
                DumpIR(frame, "released");
                break;

            case EVENTNUM_EventQueueOverflow:
                printf("  Event queue overflowed, some events lost\n");
                break;

            case EVENTNUM_ControlPortChange:
                printf("  The Control Port configuration has changed\n");
                break;

            case EVENTNUM_GivingFocus:
                printf("  We have been given focus\n");
                break;

            case EVENTNUM_LosingFocus:
                printf("  We are losing focus\n");
                break;

            case EVENTNUM_DeviceChanged:
                printf("  Device on-line or off-line\n");
                break;

            case EVENTNUM_FilesystemMounted:
                DumpFilesystem(frame, "mounted");
                break;

            case EVENTNUM_FilesystemOffline:
                DumpFilesystem(frame, "off-line");
                break;

            case EVENTNUM_FilesystemDismounted:
                DumpFilesystem(frame, "dismounted");
                break;

            case EVENTNUM_DetailedEventTiming:
		DumpTiming(frame);
                break;

            default:
                printf("  Event %d\n", frame->ef_EventNumber);
                break;
        }
        frame = (EventFrame *) (frame->ef_ByteCount + (char *) frame);
    }

    printf(" End of record\n\n");
}


/*****************************************************************************/


/* dump information on an event broker event message */
static void DumpEvent(const EventBrokerHeader *hdr)
{
    switch (hdr->ebh_Flavor)
    {
        case EB_EventRecord: DumpEventRecord(hdr, (EventFrame *) (hdr + 1));
                             break;

        default            : printf("Lookie got event-message type %d\n", hdr->ebh_Flavor);
                             break;
    }
}


/*****************************************************************************/


int main(int32 argc, char **argv)
{
Item                  ebPortItem;
Item                  msgPortItem;
Item                  msgItem;
Item                  eventItem;
int32                 sigs;
Message              *event;
MsgPort              *msgPort;
ConfigurationRequest  config;
EventBrokerHeader    *msgHeader;
Err                   err;
bool                  quit;

    ebPortItem = FindMsgPort(EventPortName);
    if (ebPortItem >= 0)
    {
        msgPortItem = CreateMsgPort(NULL, 0, 0);
        if (msgPortItem >= 0)
        {
            msgItem = CreateMsg(NULL,0,msgPortItem);
            if (msgItem >= 0)
            {
                memset(&config,0,sizeof(config));
                config.cr_Header.ebh_Flavor = EB_Configure;
                config.cr_Category          = LC_Observer;

                if (argc >= 2)
                {
                    if (strcmp(argv[1], "focus") == 0)
                        config.cr_Category = LC_FocusListener;
                    else if (strcmp(argv[1], "hybrid") == 0)
                        config.cr_Category = LC_FocusUI;
                    else
                        printf("Listener category not 'focus' or 'hybrid'\n");
               }

               config.cr_TriggerMask[0] = EVENTBIT0_ControlButtonPressed |
                                          EVENTBIT0_ControlButtonReleased |
                                          EVENTBIT0_ControlButtonUpdate |
                                          EVENTBIT0_MouseButtonPressed |
                                          EVENTBIT0_MouseButtonReleased |
                                          EVENTBIT0_MouseUpdate |
                                          EVENTBIT0_MouseMoved |
                                          EVENTBIT0_GivingFocus |
                                          EVENTBIT0_LosingFocus |
                                          EVENTBIT0_LightGunButtonPressed |
                                          EVENTBIT0_LightGunButtonReleased |
                                          EVENTBIT0_LightGunUpdate |
                                          EVENTBIT0_LightGunFireTracking |
                                          EVENTBIT0_StickButtonPressed |
                                          EVENTBIT0_StickButtonReleased |
                                          EVENTBIT0_StickUpdate |
                                          EVENTBIT0_StickMoved |
                                          EVENTBIT0_IRKeyPressed |
                                          EVENTBIT0_IRKeyReleased;

               config.cr_TriggerMask[1] = EVENTBIT1_ControlSettingChanged;

               config.cr_TriggerMask[2] = EVENTBIT2_ControlPortChange |
                                          EVENTBIT2_FilesystemMounted |
                                          EVENTBIT2_FilesystemOffline |
                                          EVENTBIT2_DeviceChanged |
                                          EVENTBIT2_FilesystemDismounted;

               config.cr_CaptureMask[2] = EVENTBIT2_DetailedEventTiming;

               err = SendMsg(ebPortItem, msgItem, &config, sizeof(config));
               if (err >= 0)
               {
                    msgPort = (MsgPort *)LookupItem(msgPortItem);
                    quit    = FALSE;
                    while (!quit)
                    {
                        sigs = WaitSignal(msgPort->mp_Signal);
                        if (sigs < 0)
                        {
                            printf("WaitSignal() failed: ");
                            PrintfSysErr(sigs);
                            break;
                        }

                        while (TRUE)
                        {
                            eventItem = GetMsg(msgPortItem);
                            if (eventItem < 0)
                            {
                                printf("GetMsg() failed: ");
                                PrintfSysErr(eventItem);
                                quit = TRUE;
                                break;
                            }

                            if (eventItem == 0)
                                break;

                            event     = (Message *) LookupItem(eventItem);
                            msgHeader = (EventBrokerHeader *) event->msg_DataPtr;
                            if (eventItem == msgItem)
                            {
                                if ((int32) event->msg_Result < 0)
                                {
                                    printf("Lookie says broker refused configuration request: ");
                                    PrintfSysErr(event->msg_Result);
                                    quit = TRUE;
                                    break;
                                }
                                printf("Lookie says broker has accepted event-config request\n");
                            }
                            else
                            {
                                DumpEvent(msgHeader);
                                ReplyMsg(eventItem, 0, NULL, 0);
                            }
                        }
                    }
                }
                else
                {
                    printf("Cannot send message: ");
                    PrintfSysErr(err);
                }
                DeleteMsg(msgItem);
            }
            else
            {
                printf("Cannot create message: ");
                PrintfSysErr(msgItem);
            }
            DeleteMsgPort(msgPortItem);
        }
        else
        {
            printf("Cannot create event-listener port: ");
            PrintfSysErr(msgPortItem);
        }
    }
    else
    {
        printf("Can't find Event Broker port: ");
        PrintfSysErr(ebPortItem);
    }

    return 0;
}
