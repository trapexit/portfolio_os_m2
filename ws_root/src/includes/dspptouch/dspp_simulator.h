#ifndef __DSPPTOUCH_DSPP_SIMULATOR_H
#define __DSPPTOUCH_DSPP_SIMULATOR_H


/******************************************************************************
**
**  @(#) dspp_simulator.h 95/05/10 1.5
**
**  DSPP Simulator
**
**  By: Phil Burk and Bill Barton
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950419 WJB  Created.
**  950510 WJB  Moved dsphTraceSimulator() back to dspp_touch_bulldog.h.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <kernel/types.h>


#ifdef SIMULATE_DSPP /* { */

/* -------------------- Functions */

void dspsInstallIntHandler( int32 (*HandlerFunc)( void ) );

#endif   /* } */


/*****************************************************************************/

#endif /* __DSPPTOUCH_DSPP_SIMULATOR_H */
