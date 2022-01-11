/* @(#) SplitterDriver.c 96/07/08 1.10 */

/* Control Port driverlet code for 3DO Splitters. */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <misc/event.h>
#include <misc/poddriver.h>
#include <stdio.h>
#include <string.h>

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

#define DBUG0(x) printf x

/*
  3DO Splitter driverlet.

  Private data assignments:

  0 is the state machine.
  1 is the output data (32 bits worth).

*/

Err SplitterDriver(PodInterface *interfaceStruct)
{
  Pod *pod, *successor;
  uint32 rawBits;
  uint32 bitsIn, bitsOut;
  uint32 misconfig, unstable;
  uint8 *base;
  pod = interfaceStruct->pi_Pod;
  DBUG(("Splitter driver, function %d\n", interfaceStruct->pi_Command));
  switch (interfaceStruct->pi_Command) {
  case PD_InitDriver:
    DBUG(("Splitter driverlet init\n"));
    break;
  case PD_InitPod:
    DBUG(("Splitter pod init\n"));
    memset(pod->pod_PrivateData, 0, sizeof pod->pod_PrivateData);
    pod->pod_Flags = POD_ShortFramesOK | POD_MultipleFramesOK |
      POD_FastClockOK;
    break;
  case PD_ParsePodInput:
    DBUG(("Splitter parse data\n"));
    base = interfaceStruct->pi_ControlPortBuffers->
	mb_Segment[MB_INPUT_SEGMENT].bs_SegmentBase + pod->pod_InputByteOffset;
    rawBits = ((((uint32) base[1] << 8) + (uint32) base[2]) << 8) + (uint32) base[3];
    switch (pod->pod_PrivateData[0]) {
    case SplitterUncertain:
      DBUG(("Uncertain, sending zeroes\n"));
      pod->pod_Flags = POD_ShortFramesOK | POD_MultipleFramesOK; /* filter off */
      pod->pod_PrivateData[1] = LATCHBIT;
      pod->pod_PrivateData[0] = SplitterCooldown;
      pod->pod_Blipvert = TRUE;
      break;
    case SplitterCooldown:
      DBUG(("Cooldown, sending zeroes\n"));
      pod->pod_Flags = POD_ShortFramesOK | POD_MultipleFramesOK; /* no filter */
      pod->pod_PrivateData[1] = LATCHBIT;
      if (rawBits == 0) {
	pod->pod_PrivateData[0] = SplitterConfiguring;
      }
      pod->pod_Blipvert = TRUE;
      break;
    case SplitterConfiguring:
      DBUG(("Configuring\n"));
      pod->pod_Flags = POD_ShortFramesOK | POD_MultipleFramesOK; /* no filter */
      pod->pod_Blipvert = TRUE;
      pod->pod_PrivateData[1] = LATCHBIT;
      if (rawBits != 0) {
	pod->pod_PrivateData[0] = SplitterCooldown;
	break;
      }
      bitsIn = bitsOut = 0;
      successor = (Pod *) NextNode(pod);
      misconfig = FALSE;
      unstable = FALSE;
      while (NextNode(successor) != NULL) { /* scan to end of list */
	DBUG(("Port 1 type 0x%x\n", successor->pod_Type));
	if (successor->pod_Type == 0xFE /* air */ ||
	    successor->pod_Type == 0x56 /* splitter 1 */ ||
	    successor->pod_Type == 0x57 /* splitter 2 */) {
	  misconfig = TRUE;
	  DBUG0(("Misconfigured!"));
	  break;
	}
	if (pod->pod_LoginLogoutPhase != POD_Online) {
	  DBUG(("Not yet online\n"));
	  unstable = TRUE;
	  break;
	}
	bitsIn += successor->pod_BitsIn;
	bitsOut += successor->pod_BitsOut;
	successor = (Pod *) NextNode(successor);
      }
      if (misconfig) {
	pod->pod_PrivateData[1] = 0;
	pod->pod_PrivateData[0] = SplitterUncertain;
	break;
      }
      if (unstable) {
	pod->pod_PrivateData[1] = 0;
	break;
      }
      DBUG(("Port 1 bits-in %d bits-out %d\n", bitsIn, bitsOut));
      bitsIn += 8; /* account for air frame */
      bitsOut += pod->pod_BitsOut;
      pod->pod_PrivateData[1] = ((bitsIn / 8) << 11) |
	(bitsOut / 2) | LATCHBIT;
      DBUG(("Write output bits 0x%x\n", pod->pod_PrivateData[1]));
      pod->pod_PrivateData[0] = SplitterConfigured;
      pod->pod_Flags = POD_StateFilterOK | POD_EventFilterOK |
	POD_ShortFramesOK | POD_MultipleFramesOK;
      break;
    case SplitterConfigured:
      DBUG(("Configured!\n"));
      if (rawBits == 0) {
	pod->pod_Blipvert = TRUE;
      } else if (rawBits != (pod->pod_PrivateData[1] & 0x00FFFFFF)) {
	DBUG0(("Bad data readback (0x%x != 0x%x, burping!\n",
	       rawBits, pod->pod_PrivateData[1]));
#ifdef NOTDEF
	pod->pod_PrivateData[1] = LATCHBIT;
	pod->pod_PrivateData[0] = SplitterUncertain;
#endif
	pod->pod_Flags = POD_ShortFramesOK | POD_MultipleFramesOK; /* turn off filtering */
	pod->pod_Blipvert = TRUE;
      }
      break;
    }
    break;
  case PD_PrecheckPodInput:
    base = interfaceStruct->pi_ControlPortBuffers->
	mb_Segment[MB_INPUT_SEGMENT].bs_SegmentBase + pod->pod_InputByteOffset;
    rawBits = ((((uint32) base[1] << 8) + (uint32) base[2]) << 8) + (uint32) base[3];
    if (pod->pod_PrivateData[0] == SplitterConfigured &&
	rawBits != (pod->pod_PrivateData[1] & 0x00FFFFFF)) {
      /*
	 This splitter has been configured, but the configuration
	 readback indicates that the splitter did not get valid control
	 data.  Tell the 'Broker not to trust the data on the port yet,
	 as the misconfiguration could result in bogus data arriving.
      */
      DBUG(("Precheck veto!\n"));
      return -1; /* TBD, return a real error code */
    }
    break;
  case PD_AppendEventFrames:
    break;
  case PD_ProcessCommand:
    interfaceStruct->pi_CommandOutLen = 0;
    break;
  case PD_ConstructPodOutput:
    DBUG(("Build output for a Splitter: 0x%x\n", pod->pod_PrivateData[1]));
    (*interfaceStruct->pi_PackBits)(pod->pod_PrivateData[1],
				    32,
				    FALSE,
				    interfaceStruct->pi_ControlPortBuffers,
				    MB_OUTPUT_SEGMENT);
    break;
  default:
    break;
  }
  return 0;
}

Err EndOfSplitterDriver(PodInterface *interfaceStruct)
{
  Pod *pod;
  pod = interfaceStruct->pi_Pod;
  DBUG(("End-of-splitter driver, function %d\n", interfaceStruct->pi_Command));
  switch (interfaceStruct->pi_Command) {
  case PD_InitPod:
  case PD_ReconnectPod:
    pod->pod_Flags = POD_ShortFramesOK + POD_MultipleFramesOK +
      POD_EventFilterOK + POD_StateFilterOK + POD_FastClockOK;
    pod->pod_Blipvert = TRUE; /* output bits changed, must send */
    break;
  default:
    break;
  }
  return 0;
}

