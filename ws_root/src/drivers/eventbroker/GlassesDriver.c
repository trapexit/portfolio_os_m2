/* @(#) GlassesDriver.c 96/07/08 1.10 */

/* Control Port driverlet code for 3DO Stereo Glasses. */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/operror.h>
#include <misc/event.h>
#include <misc/poddriver.h>
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


extern PodInterface *interfaceStruct;

/*
  3DO Stereo Glasses driverlet.  Output only.

  Private data assignments:  0 is the audio control settings, 1 is the
  left eye control, 2 is the right eye control.
*/

Err driverlet (PodInterface *interfaceStruct)
{
  Pod *pod;
  uint32 outBits, flipBits;
  Err barf;
  uint32 oddField, eye;
  pod = interfaceStruct->pi_Pod;
  switch (interfaceStruct->pi_Command) {
  case PD_InitDriver:
    DBUG(("Glasses driverlet init\n"));
    break;
  case PD_InitPod:
    DBUG(("Glasses pod init\n"));
    pod->pod_PrivateData[0] = AUDIO_Channels_Stereo;
    pod->pod_PrivateData[1] =
      pod->pod_PrivateData[2] =
	pod->pod_PrivateData[3] = 0;
    pod->pod_Flags = POD_IsGlassesCtlr + POD_IsAudioCtlr;
    pod->pod_Blipvert = TRUE;
    break;
  case PD_ReconnectPod:
    pod->pod_Blipvert = TRUE;
    break;
  case PD_ParsePodInput:
    break;
  case PD_ProcessCommand:
    interfaceStruct->pi_CommandOutLen = 0;
    barf = FALSE;
    switch (interfaceStruct->pi_CommandIn[0]) {
    case GENERIC_AudioCtlr:
      if (interfaceStruct->pi_CommandIn[1] == GENERIC_AUDIO_SetChannels) {
/*
  Nasty sleaze - simply sets output to command data byte, no
  mapping or verification.
*/
	pod->pod_PrivateData[0] = interfaceStruct->pi_CommandIn[1];
      } else {
	barf = TRUE;
      }
      break;
    case GENERIC_GlassesCtlr:
      if (interfaceStruct->pi_CommandIn[1] == GENERIC_GLASSES_SetView) {
	pod->pod_PrivateData[1] = interfaceStruct->pi_CommandIn[2];
	pod->pod_PrivateData[2] = interfaceStruct->pi_CommandIn[3];
      } else {
	barf = TRUE;
      }
      break;
    default:
      barf = TRUE;
    }
    if (barf) {
      return MAKEEB(ER_SEVERE,ER_C_STND,ER_NotSupported);
    }
    pod->pod_Blipvert = TRUE; /* output bits changed, must send */
    break;
  case PD_ConstructPodOutput:
/*
  FIXME - the following is no longer accurate, and needs to be cleaned up
  prior to 2.0 release.  dplatt

  The VBL field in the interface structure is the output field and line
  information which was captured most recently.  If bit 11 indicates
  that the capture occurred during the even field, then the output
  buffer now being built is for the _odd_ field... and vice versa.
*/
    outBits = pod->pod_PrivateData[0] & 0x00000003;
    oddField = interfaceStruct->pi_VBL & 0x00000800; /* test bit 11 */
    flipBits = 0;
    eye = pod->pod_PrivateData[1]; /* do left eye */
    if (eye == GLASSES_Opaque ||
	(eye == GLASSES_OnEvenField && !oddField) ||
	(eye == GLASSES_OnOddField && oddField)) {
      outBits |= 0x00000008; /* opaque left lens */
    }
    if (eye == GLASSES_OnEvenField || eye == GLASSES_OnOddField) {
      flipBits |= 0x00000008; /* toggle left lens */
    }
    eye = pod->pod_PrivateData[2]; /* do right eye */
    if (eye == GLASSES_Opaque ||
	(eye == GLASSES_OnEvenField && !oddField) ||
	(eye == GLASSES_OnOddField && oddField)) {
      outBits |= 0x00000004; /* opaque right lens */
    }
    if (eye == GLASSES_OnEvenField || eye == GLASSES_OnOddField) {
      flipBits |= 0x00000004; /* toggle right lens */
    }
    (*interfaceStruct->pi_PackBits)(outBits,
				    4,
				    FALSE,
				    interfaceStruct->pi_ControlPortBuffers,
				    MB_OUTPUT_SEGMENT);
    if (eye == GLASSES_OnEvenField || eye == GLASSES_OnOddField) {
      flipBits |= 0x00000004; /* toggle left lens */
    }
    if (flipBits) {
      (*interfaceStruct->pi_PackBits)(flipBits,
				      4,
				      FALSE,
				      interfaceStruct->pi_ControlPortBuffers,
				      MB_FLIPBITS_SEGMENT);
      pod->pod_Flipvert = TRUE;
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
