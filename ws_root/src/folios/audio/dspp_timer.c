/* @(#) dspp_timer.c 96/08/20 1.7 */
/****************************************************************
**
** Low Level Timer for AudioFolio
**
** Hardware Abstraction Layer
**
** By:  Phil Burk
**
** Copyright (c) 1995, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/

#if 0
--- How it Works ---

Pass 1 emulation of Pass 2 ----
For Pass 1,we attempt to emulate Pass 2 hardware.
The DSPX_AUDIO_DURATION is set to 1 so that DSPX_AUDIO_TIME increments
once for every frame.  This makes it act like the DSPX_FRAME_UP_COUNTER
in Pass 2.

We then use the DSPX_WAKEUP_TIME to generate interrupts.  In order to emulate
Pass 2, we don't schedule a wakeup more than 0xFFFF frames in the future.

Calculation of audio time based on frame count ---

Audio time advances more slowly than the frame count.  To convert
from frames to audio time we divide the number of frames elapsed
by the AudioDuration.  A problem occurs when the frame count wraps
around 32 bits.  The frame count will wrap every:

       (0xFFFFFFFF / 44100.0) / 3600 = 26 minutes

By checking the elapsed frames every second or so, we can advance the
AudioTime accurately.  We need to keep track of remainder frames
to avoid drift because of roundoff errors.

#endif

#include <dspptouch/dspp_touch.h>
#include <dspptouch/touch_hardware.h>       /* Read/WriteHardware() */
#include <kernel/semaphore.h>               /* LockSemaphore() */
#include <kernel/kernel.h>

#include "audio_folio_modes.h"
#include "audio_internal.h"

/* Macros for debugging. */
#define DBUG(x)  /* PRT(x) */

uint32 dsppGetCurrentFrameCount( void )
{
	return ReadHardware(DSPX_FRAME_UP_COUNTER);
}

/******************************************************************
** Schedule a wakeup interrupt at a given frame count.
*/
void dsppSetWakeupFrame( uint32 WakeupFrame )
{
	int32 FrameAdvance;
	uint32 currentFrameCount;

	currentFrameCount = dsppGetCurrentFrameCount();
	FrameAdvance = WakeupFrame - currentFrameCount;

/* The hardware cannot schedule more than 64K frames in the future
** because it uses a 16 bit down counter. */
#define MAXIMUM_FRAME_ADVANCE     (0xFFFF)
#define MINIMUM_FRAME_ADVANCE     (0x0001)
	if( FrameAdvance > MAXIMUM_FRAME_ADVANCE )
	{
		FrameAdvance = MAXIMUM_FRAME_ADVANCE;
	}
	else if( FrameAdvance < MINIMUM_FRAME_ADVANCE )
	{
		FrameAdvance = MINIMUM_FRAME_ADVANCE;
	}
DBUG(("dsppSetWakeupFrame: Next event in 0x%x frames. Next signal in 0x%x frames.\n", WakeupFrame - currentFrameCount, FrameAdvance));

/* Set down counter to highest value before we clear interrupt so we don't
** get a spurious interrupt before setting the correct value. */
	WriteHardware( DSPX_FRAME_DOWN_COUNTER, MAXIMUM_FRAME_ADVANCE );
	WriteHardware( DSPX_INTERRUPT_CLR, DSPX_F_INT_TIMER );
	WriteHardware( DSPX_FRAME_DOWN_COUNTER, FrameAdvance );

/* Request timer interrupt. */
	WriteHardware( DSPX_INTERRUPT_ENABLE, DSPX_F_INT_TIMER );
}
