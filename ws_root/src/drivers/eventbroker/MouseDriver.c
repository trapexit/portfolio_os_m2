/* @(#) MouseDriver.c 96/07/08 1.13 */

/* Control Port driverlet code for 3DO mice. */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/operror.h>
#include <misc/event.h>
#include <misc/poddriver.h>
#include <stdlib.h>
#include <string.h>

/* #define DEBUG */

#ifdef DEBUG
# define DBUG(x)  kprintf x
#else
# define DBUG(x) /* x */
#endif

#ifdef DEBUG2
# define DBUG2(x)  kprintf x
#else
# define DBUG2(x) /* x */
#endif

#define DBUG0(x) kprintf x

/*
  3DO mouse driverlet.

  Private data assignments:  0 is the current field's standard bit
  settings, 1 is the previous field's, 2 is the current X offset,
  3 is the current Y offset, 4 is the output data, 5 through 7 are
  unused.
*/

static void StampFrame(EventFrame *thisFrame, Pod *thisPod) {
  thisFrame->ef_PodNumber = thisPod->pod_Number;
  thisFrame->ef_PodPosition = thisPod->pod_Position;
  thisFrame->ef_GenericPosition =
    thisPod->pod_GenericNumber[GENERIC_Mouse];
  DBUG2(("Stamped event pod %d position %d generic %d\n",thisFrame->ef_PodNumber, thisFrame->ef_PodPosition, thisFrame->ef_GenericPosition));
}

static void FillFrame(EventFrame *thisFrame, Pod *thisPod, uint32 buttons)
{
  MouseEventData *med;
  StampFrame(thisFrame, thisPod);
  med = (MouseEventData *) &thisFrame->ef_EventData[0];
  med->med_ButtonBits = buttons;
  med->med_HorizPosition = thisPod->pod_PrivateData[2];
  med->med_VertPosition = thisPod->pod_PrivateData[3];
}

Err driverlet(PodInterface *interfaceStruct)
{
  Pod *pod;
  uint32 rawBits, decodedBits, oldBits;
  uint32 buttonsDown, buttonsUp;
  uint32 events;
  uint8 *base;
  int32 xIncrement, yIncrement;
  EventFrame *thisFrame;
  PodStateTable *pst;
  uint8 genericNum;
  pod = interfaceStruct->pi_Pod;
  switch (interfaceStruct->pi_Command) {
  case PD_InitDriver:
    DBUG(("Mouse driverlet init\n"));
    break;
  case PD_InitPod:
    DBUG(("Mouse pod init\n"));
    pod->pod_PrivateData[0] =
      pod->pod_PrivateData[1] =
	pod->pod_PrivateData[2] =
	  pod->pod_PrivateData[3] =
	    pod->pod_PrivateData[4] = 0;
    pod->pod_Flags = POD_IsMouse +
      POD_ShortFramesOK + POD_MultipleFramesOK + POD_StateFilterOK +
	POD_FastClockOK;
    break;
  case PD_ParsePodInput:
    DBUG(("Mouse parse data\n"));
    oldBits = pod->pod_PrivateData[1] = pod->pod_PrivateData[0];
    base = interfaceStruct->pi_ControlPortBuffers->
      mb_Segment[MB_INPUT_SEGMENT].bs_SegmentBase + pod->pod_InputByteOffset;
    rawBits = (((uint32) base[0]) << 24) +
	  (((uint32) base[1]) << 16) +
	  (((uint32) base[2]) << 8) +
	  (((uint32) base[3]));
    DBUG(("Mouse bits: %x\n", rawBits));
    decodedBits = (rawBits << 8) & 0xF0000000;
    yIncrement = (rawBits >> 10) & 0x000003FF;
    if (yIncrement & 0x00000200) {
      yIncrement |= 0xFFFFFC00;
    }
    xIncrement = rawBits & 0x000003FF;
    if (xIncrement & 0x00000200) {
      xIncrement |= 0xFFFFFC00;
    }
    DBUG(("X increment 0x%x Y increment 0x%x\n", xIncrement, yIncrement));
    pod->pod_PrivateData[0] = decodedBits;
    pod->pod_PrivateData[2] += xIncrement;
    pod->pod_PrivateData[3] += yIncrement;
    buttonsDown = decodedBits & ~ oldBits;
    buttonsUp   = oldBits & ~ decodedBits;
    events = buttonsDown ?
      (buttonsUp ?
       (EVENTBIT0_MouseButtonPressed + EVENTBIT0_MouseButtonReleased +
	EVENTBIT0_MouseUpdate + EVENTBIT0_MouseDataArrived) :
       (EVENTBIT0_MouseButtonPressed +
	EVENTBIT0_MouseUpdate + EVENTBIT0_MouseDataArrived)) :
	  (buttonsUp ?
	   (EVENTBIT0_MouseButtonReleased +
	    EVENTBIT0_MouseUpdate + EVENTBIT0_MouseDataArrived) :
	   (EVENTBIT0_MouseDataArrived));
    if (xIncrement || yIncrement) {
      events |= EVENTBIT0_MouseMoved;
      pod->pod_Flags &= ~POD_EventFilterOK;
    } else {
      pod->pod_Flags |= POD_EventFilterOK;
    }
    pod->pod_EventsReady[0] = events;
    DBUG2(("Pod down 0x%x up 0x%x events 0x%x\n", buttonsDown, buttonsUp, events));
    break;
  case PD_AppendEventFrames:
    DBUG(("Mouse event-frame append\n"));
    DBUG(("Ready 0x%x look for 0x%x\n", pod->pod_EventsReady[0], interfaceStruct->pi_TriggerMask[0] | interfaceStruct->pi_CaptureMask[0]));
    events = (interfaceStruct->pi_TriggerMask[0] |
	      interfaceStruct->pi_CaptureMask[0]) & pod->pod_EventsReady[0];
    DBUG(("Combined mask 0x%x\n", events));
    if (events == 0 && ! interfaceStruct->pi_RecoverFromLostEvents) {
      break;
    }
    if (events & EVENTBIT0_MouseButtonPressed) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_MouseButtonPressed,
					 sizeof (MouseEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	FillFrame(thisFrame, pod, pod->pod_PrivateData[0] &
		  ~ pod->pod_PrivateData[1]);
	DBUG(("Append a mouse-button-pressed frame\n"));
      }
    }
    if (events & EVENTBIT0_MouseButtonReleased) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_MouseButtonReleased,
					 sizeof (MouseEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	FillFrame(thisFrame, pod, pod->pod_PrivateData[1] &
		  ~ pod->pod_PrivateData[0]);
	DBUG(("Append a mouse-button-released frame\n"));
      }
    }
    if (interfaceStruct->pi_RecoverFromLostEvents ||
	(events & EVENTBIT0_MouseUpdate) != 0) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_MouseUpdate,
					 sizeof (MouseEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	FillFrame(thisFrame, pod, pod->pod_PrivateData[0]);
	DBUG(("Append a mouse-button-update frame\n"));
      }
    }
    if (events & EVENTBIT0_MouseMoved) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_MouseMoved,
					 sizeof (MouseEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	FillFrame(thisFrame, pod, pod->pod_PrivateData[0]);
	DBUG(("Append a mouse-data-arrival frame\n"));
      }
    }
    if (events & EVENTBIT0_MouseDataArrived) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_MouseDataArrived,
					 sizeof (MouseEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	FillFrame(thisFrame, pod, pod->pod_PrivateData[0]);
	DBUG(("Append a mouse-data-arrival frame\n"));
      }
    }
    break;
  case PD_UpdatePodStateTable:
    pst = interfaceStruct->pi_PodStateTable;
    genericNum = pod->pod_GenericNumber[GENERIC_Mouse];
    if (pst && pst->pst_MouseTable.gt_HowMany > genericNum) {
      pst->pst_MouseTable.gt_EventSpecificData[genericNum].med_ButtonBits =
	pod->pod_PrivateData[0];
      pst->pst_MouseTable.gt_EventSpecificData[genericNum].med_HorizPosition =
	pod->pod_PrivateData[2];
      pst->pst_MouseTable.gt_EventSpecificData[genericNum].med_VertPosition =
	pod->pod_PrivateData[3];
      pst->pst_MouseTable.gt_ValidityTimeStamps[genericNum] =
	interfaceStruct->pi_VBL;
    }
    break;
  default:
    break;
  }
  return 0;
}


Err main(int32 argc, char **argv)
{
  PodDriver *theDriver = (PodDriver *) argc;

  TOUCH(argv);

  theDriver->pd_DriverEntry = driverlet;
  return 0;
}

