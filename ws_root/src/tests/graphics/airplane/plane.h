#ifndef _PLANE_H
#define _PLANE_H
/******************************************************************************
**
**  @(#) plane.h 96/04/15 1.3
**
******************************************************************************/


/*
** Airplane demo
**
** Author: phil Burk
** Copyright 1995 3DO
** All Rights Reserved
*/

#include <kernel/types.h>
#include "stdio.h"
#include "math.h"

#include "plane_physics.h"
#include "terrain.h"
#include "sound_wind.h"
#include "plane_graphics.h"

#define USE_SOUND             (1)
#define IF_DISPLAY_COCKPIT    (1)
#define IF_DISPLAY_FRAME_RATE (1)

/* Plane characteristics. ******************************************************/
/* Max Velocity in meters/second. */
#define kPlaneMaxVel             (600.0)
#define kPlaneMass               (900.0)

/* Planes engine can only generate a fraction of thrust needed to climb straight up. */
#define kPlaneMaxGs              (2.0)

#define SOME_POWER_OF_2  (64)
#define kNumRows         (SOME_POWER_OF_2+1)
#define kNumCols         (SOME_POWER_OF_2+1)
#define kNumPoints       (kNumRows*kNumCols)
#define kTransInc        (0.05)
#define kRotInc          (0.5)

/* Macro to simplify error checking. */
#define CALL_CHECK(_exp,msg) \
	DBUG(("%s\n",msg)); \
	do \
	{ \
		if ((Result = (Err) (_exp)) < 0) \
		{ \
			PrintError(0,"\\failure in",msg,Result); \
			goto cleanup; \
		} \
	} while(0)

#endif
