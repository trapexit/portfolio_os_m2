#ifndef __DSPPTOUCH_TOUCH_HARDWARE_H
#define __DSPPTOUCH_TOUCH_HARDWARE_H


/******************************************************************************
**
**  @(#) touch_hardware.h 96/02/28 1.13
**  $Id: touch_hardware.h,v 1.7 1995/02/23 10:23:40 phil Exp phil $
**
**  Touch Hardware, fake it with a PRT, or call DSPP simulator.
**
**  By:  Phil Burk
**
**  Copyright (c) 1992, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  940907 PLB  Change PutHard to WriteHardware
**  950131 WJB  Cleaned up includes.
**  950505 WJB  Restructured compile time variants.
**              Removed dependency on audio_internal.h.
**
**  Initials:
**
**  PLB: Phil Burk (phil)
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <kernel/types.h>


/* -------------------- Functions / Macros */
/* #define FAKETOUCH */
#if defined(SIMULATE_DSPP)  /* DSPP simulator */

		/* These calls are implemented in the simulator */
	void WriteHardware( vuint32 *Address, uint32 Val );
	uint32 ReadHardware( vuint32 *Address );

#elif defined(FAKETOUCH)    /* Fake mode: just print out all hardware accesses. */

	#include <stdio.h>      /* printf() */

	extern int32 verbose;

	#define WriteHardware(addr,val) \
		{ if(verbose) printf ("WriteHardware: %9x to %9x\n", (uint32) addr, val); }

		/* @@@ this one always returns 0 */
	#define ReadHardware(addr) \
		( verbose ? (printf ("ReadHardware: %9x\n", (uint32) addr), 0) : 0 )

#else                       /* Real mode: Actually poke hardware. */

    /* @@@ these macros must evaluate their args precisely once (some callers use postinc) */
	#define WriteHardware(addr,val) do { *((vuint32 *)(addr)) = (val); } while (0)
	#define ReadHardware(addr) *((vuint32 *)(addr))

#endif


/*****************************************************************************/

#endif  /* __DSPPTOUCH_TOUCH_HARDWARE_H */
