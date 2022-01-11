/* @(#) ControlPadDriver.c 96/07/18 1.17 */

/* Control Port driverlet code for 3DO Control Pads. */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <misc/event.h>
#include <misc/poddriver.h>
#include <stdio.h>
#include <string.h>


extern int32 debugFlag;

#undef DEBUG
#undef DEBUG2

#ifdef DEBUG
# define DBUG(x)  printf x
#else
# define DBUG(x) /* x */
#endif

#ifdef DEBUG2
# define DBUG2(x)  printf x
#else
# define DBUG2(x) /* x */
#endif

#define DBUG0(x) if (debugFlag) { printf x ; }

/*
  3DO Control Pad driverlet.  Handles the old-style 8-bit control pad
  (at least for now), and the standard and two extended-form control
  pads.

  Private data assignments:  0 is the current field's standard bit
  settings, 1 is the previous field's, 2 is the output data, 3 the current
  field's analog encodings, 4 is the previous field's analog encoding;
*/

static void StampFrame(EventFrame *thisFrame, Pod *thisPod) {
  thisFrame->ef_PodNumber = thisPod->pod_Number;
  thisFrame->ef_PodPosition = thisPod->pod_Position;
  thisFrame->ef_GenericPosition =
    thisPod->pod_GenericNumber[GENERIC_ControlPad];
  DBUG(("Stamped event pod %d position %d generic %d\n",thisFrame->ef_PodNumber, thisFrame->ef_PodPosition, thisFrame->ef_GenericPosition));
}

static void FillAnalog(ControlPadEventData *cped, Pod *pod) {
  if (pod->pod_Type == PODID_3DO_M2_CONTROL_PAD) {
    cped->cped_AnalogValid = TRUE;
    cped->cped_StickX = (uint8) (pod->pod_PrivateData[3] >> 24);
    cped->cped_StickY = (uint8) (pod->pod_PrivateData[3] >> 16);
    cped->cped_RightShift = (uint8) (pod->pod_PrivateData[3] >> 12) & 0x0F;
    cped->cped_LeftShift = (uint8) (pod->pod_PrivateData[3] >> 8) & 0x0F;
    cped->cped_Shuttle = (uint8) pod->pod_PrivateData[3];
  }
}

Err ControlPadDriver(PodInterface *interfaceStruct)
{
  Pod *pod;
  uint32 decodedBits = 0, oldBits;
  uint32 buttonsDown, buttonsUp;
  uint32 events, events1;
  uint8 *base;
  EventFrame *thisFrame;
  PodStateTable *pst;
  uint8 genericNum;
  uint32 analogBits;
  ControlPadEventData *cped;
  pod = interfaceStruct->pi_Pod;
  DBUG(("Control Pad driver, function %d\n", interfaceStruct->pi_Command));
  switch (interfaceStruct->pi_Command) {
  case PD_InitDriver:
    DBUG(("Control Pad driverlet init\n"));
    break;
  case PD_InitPod:
    DBUG(("Control Pad pod init\n"));
    pod->pod_PrivateData[0] =
      pod->pod_PrivateData[1] =
	pod->pod_PrivateData[3] =
	  pod->pod_PrivateData[4] = 0;
    pod->pod_PrivateData[2] = AUDIO_Channels_Stereo;
    pod->pod_Flags = POD_IsControlPad + POD_IsAudioCtlr +
      POD_ShortFramesOK + POD_MultipleFramesOK + POD_EventFilterOK +
	POD_StateFilterOK;
    if (pod->pod_Type == PODID_3DO_M2_CONTROL_PAD) {
      pod->pod_Flags |= POD_FastClockOK;
      base = interfaceStruct->pi_ControlPortBuffers->
	mb_Segment[MB_INPUT_SEGMENT].bs_SegmentBase + pod->pod_InputByteOffset;
      if (!(base[2] & 0x01)) {
	pod->pod_Flags &= ~POD_IsAudioCtlr;
      }
    }
    pod->pod_Blipvert = TRUE; /* output bits changed, must send */
    break;
  case PD_ReconnectPod:
    pod->pod_Blipvert = TRUE; /* output bits changed, must send */
    break;
  case PD_ParsePodInput:
    DBUG(("Control Pad parse data\n"));
    oldBits = pod->pod_PrivateData[1] = pod->pod_PrivateData[0];
    base = interfaceStruct->pi_ControlPortBuffers->
	mb_Segment[MB_INPUT_SEGMENT].bs_SegmentBase + pod->pod_InputByteOffset;
    analogBits = 0;
    switch (pod->pod_Type) {
    case PODID_3DO_OPERA_CONTROL_PAD:
      decodedBits = (((uint32) base[0] << 8) + base[1]) << 19;
      break;
    case PODID_3DO_M2_CONTROL_PAD:
      decodedBits =
	(((((uint32) base[0] << 8) + base[1]) << 8) +
	 base[2]) << 8;                               /* 101DURLABCDEFSXRL */
      decodedBits = ((decodedBits << 3) & 0xFE000000) /* DURLABC        */ |
	            ((decodedBits << 6) & 0x01E00000) /*        SXRL    */ |
		    ((decodedBits >> 1) & 0x001C0000) /*            DEF */;
      analogBits = ((((base[3] << 8) | base[4]) << 8) | base[5]) << 8 | base[6];
      break;
    case PODID_3DO_SILLY_CONTROL_PAD:
      decodedBits = ((((((uint32) base[0] << 8)
		       + base[1]) << 8) + base[2]) << 11)
	& (unsigned long) 0xFFFC0000; /* only pass 14 buttons */
      break;
    }
    pod->pod_PrivateData[0] = decodedBits;
    pod->pod_PrivateData[3] = analogBits;
    buttonsDown = decodedBits & ~ oldBits;
    buttonsUp   = oldBits & ~ decodedBits;
    events = EVENTBIT0_ControlButtonArrived;
    if (buttonsDown) {
      events |= EVENTBIT0_ControlButtonPressed +
	EVENTBIT0_ControlButtonUpdate;
    }
    if (buttonsUp) {
      events |= EVENTBIT0_ControlButtonReleased +
	EVENTBIT0_ControlButtonUpdate;
    }
    pod->pod_EventsReady[0] = events;
    if (analogBits != pod->pod_PrivateData[4]) {
      pod->pod_PrivateData[4] = analogBits;
      pod->pod_EventsReady[1] = EVENTBIT1_ControlSettingChanged;
    }
    break;
  case PD_AppendEventFrames:
    DBUG(("Control Pad event-frame append\n"));
    DBUG(("Ready 0x%x look for 0x%x\n", pod->pod_EventsReady[0], interfaceStruct->pi_TriggerMask[0] | interfaceStruct->pi_CaptureMask[0]));
    events = (interfaceStruct->pi_TriggerMask[0] |
	      interfaceStruct->pi_CaptureMask[0]) & pod->pod_EventsReady[0];
    events1 = (interfaceStruct->pi_TriggerMask[1] |
	      interfaceStruct->pi_CaptureMask[1]) & pod->pod_EventsReady[1];
    DBUG(("Combined mask 0x%x\n", events));
    if (events == 0 && events1 == 0 &&
	! interfaceStruct->pi_RecoverFromLostEvents) {
      break;
    }
    if (events & EVENTBIT0_ControlButtonPressed) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_ControlButtonPressed,
					 sizeof (ControlPadEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	StampFrame(thisFrame, pod);
	cped = (ControlPadEventData *) thisFrame->ef_EventData;
	FillAnalog(cped, pod);
	cped->cped_ButtonBits = pod->pod_PrivateData[0] &
	  ~ pod->pod_PrivateData[1];
	DBUG(("Append a pad-button-pressed frame\n"));
      }
    }
    if (events & EVENTBIT0_ControlButtonReleased) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_ControlButtonReleased,
					 sizeof (ControlPadEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	StampFrame(thisFrame, pod);
	cped = (ControlPadEventData *) thisFrame->ef_EventData;
	FillAnalog(cped, pod);
	cped->cped_ButtonBits = pod->pod_PrivateData[1] &
	  ~ pod->pod_PrivateData[0];
	DBUG(("Append a pad-button-released frame\n"));
      }
    }
    if (interfaceStruct->pi_RecoverFromLostEvents ||
	(events & EVENTBIT0_ControlButtonUpdate) != 0) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_ControlButtonUpdate,
					 sizeof (ControlPadEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	StampFrame(thisFrame, pod);
	cped = (ControlPadEventData *) thisFrame->ef_EventData;
	FillAnalog(cped, pod);
	cped->cped_ButtonBits = pod->pod_PrivateData[0];
	DBUG(("Append a pad-button-update frame\n"));
      }
    }
    if (events & EVENTBIT0_ControlButtonArrived) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_ControlButtonArrived,
					 sizeof (ControlPadEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	StampFrame(thisFrame, pod);
	cped = (ControlPadEventData *) thisFrame->ef_EventData;
	FillAnalog(cped, pod);
	cped->cped_ButtonBits = pod->pod_PrivateData[0];
	DBUG(("Append a pad-button-arrival frame\n"));
      }
    }
    if (events1 & EVENTBIT1_ControlSettingChanged) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_ControlSettingChanged,
					 sizeof (ControlPadEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	StampFrame(thisFrame, pod);
	cped = (ControlPadEventData *) thisFrame->ef_EventData;
	FillAnalog(cped, pod);
	cped->cped_ButtonBits = pod->pod_PrivateData[0];
	DBUG(("Append a pad-button-arrival frame\n"));
      }
    }
    break;
  case PD_ProcessCommand:
    interfaceStruct->pi_CommandOutLen = 0;
    if (interfaceStruct->pi_CommandIn[0] != GENERIC_AudioCtlr ||
	interfaceStruct->pi_CommandIn[1] != GENERIC_AUDIO_SetChannels) {
      return MAKEEB(ER_SEVERE,ER_C_STND,ER_NotSupported);
    }
/*
  Nasty sleaze - simply sets output to command data byte, no
  mapping or verification.
*/
    pod->pod_PrivateData[2] = interfaceStruct->pi_CommandIn[2];
    pod->pod_Blipvert = TRUE; /* output bits changed, must send */
    break;
  case PD_ConstructPodOutput:
    DBUG(("Build %d output bits for a control pad\n", pod->pod_BitsOut));
    switch (pod->pod_Type) {
    case PODID_3DO_OPERA_CONTROL_PAD:
      (*interfaceStruct->pi_PackBits)(pod->pod_PrivateData[2],
				      2,
				      FALSE,
				      interfaceStruct->pi_ControlPortBuffers,
				      MB_OUTPUT_SEGMENT);
      break;
    case PODID_3DO_M2_CONTROL_PAD:
      (*interfaceStruct->pi_PackBits)(pod->pod_PrivateData[2] << 6,
				      8,
				      FALSE,
				      interfaceStruct->pi_ControlPortBuffers,
				      MB_OUTPUT_SEGMENT);
      break;
    case PODID_3DO_SILLY_CONTROL_PAD:
      (*interfaceStruct->pi_PackBits)((pod->pod_PrivateData[2] << 6) | 0x20,
				      8,
				      FALSE,
				      interfaceStruct->pi_ControlPortBuffers,
				      MB_OUTPUT_SEGMENT);
      break;
    }
    break;
  case PD_UpdatePodStateTable:
    pst = interfaceStruct->pi_PodStateTable;
    genericNum = pod->pod_GenericNumber[GENERIC_ControlPad];
    if (pst && pst->pst_ControlPadTable.gt_HowMany > genericNum) {
      pst->pst_ControlPadTable.gt_EventSpecificData[genericNum].cped_ButtonBits =
	pod->pod_PrivateData[0];
      FillAnalog(&pst->pst_ControlPadTable.gt_EventSpecificData[genericNum], pod);
      pst->pst_ControlPadTable.gt_ValidityTimeStamps[genericNum] =
	interfaceStruct->pi_VBL;
    }
    break;
  default:
    break;
  }
  return 0;
}
