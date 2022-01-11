#ifndef __DSPPTOUCH_DSPP_TOUCH_ANVIL_H
#define __DSPPTOUCH_DSPP_TOUCH_ANVIL_H


/******************************************************************************
**
**  @(#) dspp_touch_anvil.h 95/05/10 1.11
**  $Id: dspp_touch_anvil.h,v 1.3 1995/03/09 00:49:09 peabody Exp phil $
**
**  DSP Includes for low level hardware driver for Opera/Anvil
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950308 PLB  Created from dspp_touch.h.
**  950308 WJB  Cleaned up.
**  950419 WJB  Moved DSPP_InitIMemAccess() here to balance dspp_touch_bulldog.h.
**  950424 WJB  Moved dsphWriteEIMem() prototype here to balance dspp_touch_bulldog.h.
**              Added dsph_InitInstrumentation() stub.
**  950510 WJB  Simplified dsphReadEOMem() API.
**
**  Initials:
**
**  PLB: Phil Burk (phil)
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <dspptouch/dspptouch_modes.h>
#include <kernel/types.h>


#ifdef AF_ASIC_OPERA  /* { */


/* -------------------- Defines */

#define DSPP_GWILL	0x00000001

	/* !!! retire these 2 */
#define NUM_AUDIO_INPUT_DMAS (13)       /* !!! use DSPI_ANVIL_NUM_INPUT_DMAS instead */
#define NUM_AUDIO_OUTPUT_DMAS (4)       /* !!! use DSPI_ANVIL_NUM_OUTPUT_DMAS instead */

#define DDR0_CHANNEL (DSPI_ANVIL_NUM_INPUT_DMAS + DSPI_ANVIL_NUM_FOREIGN_DMAS)
#define DRD0_CHANNEL (0)


/* -------------------- Functions */

	/* Macros to determine DMA Type */
#define IsDMATypeInput(chan) ((chan >= DRD0_CHANNEL) && (chan < (DRD0_CHANNEL + NUM_AUDIO_INPUT_DMAS)))
#define IsDMATypeOutput(chan) ((chan >= DDR0_CHANNEL) && (chan < (DDR0_CHANNEL + NUM_AUDIO_OUTPUT_DMAS)))

	/* Opera functions */
void   dsphWriteEIMem( int32 DSPPAddr, int32 Value );
int32  dsphReadEOMem( int32 ReadAddr );

#endif   /* } AF_ASIC_OPERA */


/*****************************************************************************/

#endif /* __DSPPTOUCH_DSPP_TOUCH_ANVIL_H */
