/* @(#) eventmgr.h 96/09/29 1.3 */

#ifndef __EVENTMGR_H
#define __EVENTMGR_H


/*****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __UI_REQUESTER_H
#include <ui/requester.h>
#endif


/*****************************************************************************/


typedef struct EventMgr EventMgr;


typedef enum EventTypes											/* types of events */
{
    EVENT_TYPE_NOP,
    EVENT_TYPE_RELATIVE_MOVE,
    EVENT_TYPE_ABSOLUTE_MOVE,
    EVENT_TYPE_BUTTON_DOWN,
    EVENT_TYPE_BUTTON_UP,
    EVENT_TYPE_FSCHANGE
} EventTypes;

typedef enum ButtonEvents										/* class of button events */
{
    EVENT_BUTTON_NOP,
    EVENT_BUTTON_SELECT,
    EVENT_BUTTON_STOP,
    EVENT_BUTTON_INFO,
	EVENT_BUTTON_SHIFT
} ButtonEvents;

#define EVENT_BUTTONSTATE_A             (1 << 1)                                        /* button state flags */
#define EVENT_BUTTONSTATE_B             (1 << 2)
#define EVENT_BUTTONSTATE_C             (1 << 3)
#define EVENT_BUTTONSTATE_1             (1 << 4)
#define EVENT_BUTTONSTATE_2             (1 << 5)
#define EVENT_BUTTONSTATE_3             (1 << 6)
#define EVENT_BUTTONSTATE_SELECT        (1 << 7)
#define EVENT_BUTTONSTATE_LS            (1 << 8)
#define EVENT_BUTTONSTATE_RS            (1 << 9)

#define EVENT_QUAL_REPEAT               (1 << 0)                                        /* event qualifiers */


typedef struct Event										/* event structure */
{
    EventTypes      ev_Type;                                                                                /* type of event that occured */
    uint32          ev_VBlankCount;                                                                 /* Vertical blank count */
    uint32          ev_ButtonState;
    uint32          ev_Qualifiers;

    union
    {
        ButtonEvents    ev_button;

        struct
        {
            int32   ev_x;
            int32   ev_y;
        } ev_Coordinates;
    } ev_EventInfo;
} Event;

#define ev_X      ev_EventInfo.ev_Coordinates.ev_x
#define ev_Y      ev_EventInfo.ev_Coordinates.ev_y
#define ev_Button ev_EventInfo.ev_button


/*****************************************************************************/


Err ConnectEventMgr(EventMgr **mgr, Item msgPort);
void DisconnectEventMgr(EventMgr *mgr);


/*****************************************************************************/


#endif /* __EVENTMGR_H */
