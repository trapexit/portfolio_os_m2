/* @(#) event.c 96/03/08 1.1 */

/*
 * This code is mostly copied from the event library,
 * but this version deals with DeviceChanged events
 * in addition to control port events.
 */

/* Event convenience utilities. */

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


#define MAXPAD   255

static int32 NumControlPads;

static struct pad {
  int32               pad_Avail;
  ControlPadEventData pad_Data;
} *pads;

static bool DeviceChanged;

static Item msgPortItem;
static Item brokerPortItem;
static Item msgItem;

static Err ChewOneEvent(bool wait);

/* #define DEBUG */

#ifdef DEBUG
# define DBUG(x)  printf x
#else
# define DBUG(x) /* x */
#endif

Err InitEventStuff (int32 numControlPads, int32 isFocused)
{
  Err err;
  
  static ConfigurationRequest config;

  int32 padSize;

  DeviceChanged = FALSE;

  brokerPortItem = FindMsgPort(EventPortName);

  if (brokerPortItem < 0) {
    err =  brokerPortItem;
    goto bail;
  }

  msgPortItem = CreateMsgPort(CURRENTTASK->t.n_Name, 0, 0);

  if (msgPortItem < 0) {
    err = msgPortItem;
    goto bail;
  }

  msgItem = CreateMsg(NULL, 0, msgPortItem);

  if (msgItem < 0) {
    err = msgItem;
    goto bail;
  }

  if (numControlPads > MAXPAD) {
    NumControlPads = MAXPAD;
  } else if (numControlPads == 0) {
    NumControlPads = 1;
  } else {
    NumControlPads = numControlPads;
  }

  padSize = (NumControlPads + 1) * sizeof (struct pad);

  pads = AllocMem(padSize, MEMTYPE_FILL | MEMTYPE_TRACKSIZE);

  if (!pads) {
    err = -1; /* need better */
    goto bail;
  }

  DBUG(("Pads structure allocated at 0x%x\n", pads));
    
  memset(&config, 0, sizeof config);
  config.cr_Header.ebh_Flavor = EB_Configure;
  config.cr_Category = (isFocused == LC_ISFOCUSED) ?
		       LC_FocusListener : LC_Observer;
  config.cr_QueueMax = 0; /* let the Broker set it */;
  config.cr_TriggerMask[0] =
    EVENTBIT0_ControlButtonUpdate;
  config.cr_TriggerMask[2] =
    EVENTBIT2_DeviceChanged;

  err = SendMsg(brokerPortItem, msgItem, &config, sizeof config);

  if (err < 0) {
    goto bail;
  }

  return 0;

 bail:
  KillEventUtility();
  return err;
}

Err GetEventStuff(int32 padNumber, bool wait, ControlPadEventData *data, uint32 *deviceChanged)
{
  Err err;
  if (padNumber < 1 || padNumber > NumControlPads || !pads) {
    return -1;
  }
  DBUG(("Get pad %d, wait flag %d\n", padNumber, wait));
  while (TRUE) {
    *deviceChanged = DeviceChanged;
    if (DeviceChanged) {
      DeviceChanged = FALSE;
      return 1;
    }
    if (pads[padNumber].pad_Avail) {
      pads[padNumber].pad_Avail = FALSE;
      *data = pads[padNumber].pad_Data;
      return 1;
    }
    err = ChewOneEvent(wait);
    if (err < 0) {
      return err;
    }
    if (err == 0 && !wait && !pads[padNumber].pad_Avail) {
      *data = pads[padNumber].pad_Data;
      return 0;
    }
  }
}

static Err ChewOneEvent(bool wait)
{
  int32 eventItem;
  int32 generic;
  Message *event;
  EventBrokerHeader *hdr;
  EventFrame *frame;
  do {
    eventItem = GetMsg(msgPortItem);
    if (eventItem < 0) {
      return eventItem;
    }
    if (eventItem == 0) {
      if (!wait) {
	return 0;
      }
      eventItem = WaitPort(msgPortItem, 0);
      if (eventItem < 0)
          return eventItem;
    }
    if (eventItem != 0) {
      DBUG(("Got message 0x%x\n"));
      event = (Message *) LookupItem(eventItem);
      if (eventItem == msgItem) {
	if ((int32) event->msg_Result < 0) {
	  return ((int32) event->msg_Result);
	}
	eventItem = 0; /* keep going */
      } else {
	hdr = (EventBrokerHeader *) event->msg_DataPtr;
	switch (hdr->ebh_Flavor) {
	case EB_EventRecord:
	  frame = (EventFrame *) (hdr + 1); /* Gawd that's ugly! */
	  while (TRUE) {
	    if (frame->ef_ByteCount == 0) {
	      break;
	    }
	    generic = frame->ef_GenericPosition;
	    switch (frame->ef_EventNumber) {
	    case EVENTNUM_ControlButtonUpdate:
	      if (generic >= 1 && generic <= NumControlPads) {
		memcpy(&pads[generic].pad_Data, frame->ef_EventData,
		       sizeof (ControlPadEventData));
		pads[generic].pad_Avail = TRUE;
	      }
	      break;
	    case EVENTNUM_DeviceChanged:
	      DeviceChanged = TRUE;
	      break;
	    default:
	      DBUG(("Event type %d\n", frame->ef_EventNumber));
	      break;
	    }
	    frame = (EventFrame *) (frame->ef_ByteCount + (char *) frame);
	  }
	  break;
	default:
	  break;
	}
	ReplyMsg(eventItem, 0, NULL, 0);
      }
    }
  } while (eventItem == 0);
  return 1;
}

Err KillEventUtility(void)
{
  FreeMem(pads, TRACKED_SIZE);
  pads = NULL;

  DeleteMsg(msgItem);
  msgItem = -1;

  DeleteMsgPort(msgPortItem);
  msgPortItem = -1;

  return 0;
}  
