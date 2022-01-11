
/******************************************************************************
**
**  @(#) plane_physics.c 96/04/15 1.8
**  $Id: plane.c,v 1.7 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/*
** Physical model of plane.
**
** Author: phil Burk
** Copyright 1995 3DO
** All Rights Reserved
*/

#include <kernel/types.h>
#include <kernel/mem.h>
#include <misc/event.h>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

#include "plane_physics.h"

#define PRT(x)  { printf x ; }
#define DBUG(x)  /* PRT(x) */

/*******************************************************************/
/*************** Plane Control *************************************/
/*******************************************************************/

#define PI                       (3.14159265)
#define PIx2                     (PI*2.0)
#define GRAVITY_ACCELERATION     (9.8) /* meters/second**2 */

/* Vertical Crash Velocity in meters/second. This corresponds roughly to 5 miles/hour */
#define CRASH_METERS_PER_SECOND  (-20.0)

/* Cockpit altitude relative to base of wheels. */
#define COCKPIT_ALTITUDE         (2.0)
#define ZERO_LIFT_ANGLE          (-0.2)
#define VELOCITY_FRACTION_FOR_LIFT  (0.5)

#define THROTTLE_FRACTION_PER_SECOND (0.3)
#define YAW_INCR_PER_SECOND      (0.03 * PIx2)
#define PITCH_INCR_PER_SECOND    (0.16 * PI)
#define ROLL_INCR_PER_SECOND     (0.20 * PI)

#define MAX_GROUND_PITCH         (0.15)
#define PITCH_DECAY_FACTOR       (0.96)
#define ROLL_DECAY_FACTOR        (0.985)

#define YAW_DELTA_FILTER_COEFF   (0.90)
#define PITCH_DELTA_FILTER_COEFF (0.95)
#define ROLL_DELTA_FILTER_COEFF  (0.95)

/****************************************************************/
int32	ppInitPlaneStatus( PlaneStatus *plst )
{
	memset( plst, 0, sizeof(PlaneStatus) );
	plst->plst_Altitude = COCKPIT_ALTITUDE;
	return 0;
}

/****************************************************************/
int32	ppCalcPlaneCharacteristics( PlaneCharacteristics *plch, float32 maxGs, float32 mass, float32 maxVel )
{
	plch->plch_Mass = mass;
	plch->plch_MaxThrust = maxGs * mass * GRAVITY_ACCELERATION;

/* Based on equation:   drag(maxvel) = maxthrust */
	plch->plch_ViscousDragCoefficient = plch->plch_MaxThrust /
		(maxVel * (1.0 + (maxVel/2.0)));
	plch->plch_TurbulentDragCoefficient = plch->plch_ViscousDragCoefficient / 2.0;

/* Base lift coefficient on having sufficient lift to fly at X% max velocity. */
	plch->plch_ZeroLiftAngle = ZERO_LIFT_ANGLE;
	plch->plch_LiftCoefficient = (GRAVITY_ACCELERATION * plch->plch_Mass) / 
		((maxVel * VELOCITY_FRACTION_FOR_LIFT) * sinf(0.0 - plch->plch_ZeroLiftAngle));

	return 0;
}

/****************************************************************/
int32	ppControlPlane( PlaneStatus *plst, PlaneCharacteristics *plch, uint32 ButtonBits, float32 elapsedTime )
{
	float32 TempDelta;

/* Apply buttons to motion. */
/* YAW --------------------------------------------------- YAW */
	if( ButtonBits & ControlLeft )
	{
		TempDelta = YAW_INCR_PER_SECOND * elapsedTime;
	}
	else if( ButtonBits & ControlRight )
	{
		TempDelta = -(YAW_INCR_PER_SECOND * elapsedTime);
	}
	else
	{
		TempDelta = 0.0;
	}
/* Low pass filter YawDelta and apply to Yaw */
	plst->plst_YawDelta = (YAW_DELTA_FILTER_COEFF * plst->plst_YawDelta)
		+ ((1.0 - YAW_DELTA_FILTER_COEFF) * TempDelta );
	plst->plst_Yaw += plst->plst_YawDelta;
/* Wrap Yaw */
	while( plst->plst_Yaw > PIx2 ) plst->plst_Yaw -= PIx2;
	while( plst->plst_Yaw < 0.0 ) plst->plst_Yaw += PIx2;


/* PITCH ----------------------------------------------- PITCH */
	TempDelta = 0.0;
	if( ButtonBits & ControlUp )
	{
		TempDelta = -(PITCH_INCR_PER_SECOND * elapsedTime);
	}
	else if( ButtonBits & ControlDown )
	{
		TempDelta = PITCH_INCR_PER_SECOND * elapsedTime;
	}
/* Low pass filter PitchDelta and apply to Pitch */
	plst->plst_PitchDelta = (PITCH_DELTA_FILTER_COEFF * plst->plst_PitchDelta)
	+ ((1.0 - PITCH_DELTA_FILTER_COEFF) * TempDelta );
	plst->plst_Pitch += plst->plst_PitchDelta;

/* Don't allow plane to fully change pitch until airborn. */
	if( plst->plst_Altitude < (1.0 + COCKPIT_ALTITUDE) )
	{
		if(plst->plst_Pitch > MAX_GROUND_PITCH) plst->plst_Pitch = MAX_GROUND_PITCH;
		if(plst->plst_Pitch < 0.0) plst->plst_Pitch = 0.0;
	}
/* Return plane to level when hand off stick. */
	plst->plst_Pitch = plst->plst_Pitch * PITCH_DECAY_FACTOR;

/* ROLL ------------------------------------------------ ROLL */
	if( ButtonBits & ControlLeftShift )
	{
		TempDelta = -(ROLL_INCR_PER_SECOND * elapsedTime);
	}
	else if( ButtonBits & ControlRightShift )
	{
		TempDelta = ROLL_INCR_PER_SECOND * elapsedTime;
	}
	else
	{
		TempDelta = 0.0;
	}
/* Low pass filter RollDelta and apply to Roll */
	plst->plst_RollDelta = (ROLL_DELTA_FILTER_COEFF * plst->plst_RollDelta)
		+ ((1.0 - ROLL_DELTA_FILTER_COEFF) * TempDelta );
	plst->plst_Roll += plst->plst_RollDelta;
/* Wrap Roll */
	while( plst->plst_Roll > PI ) plst->plst_Roll -= PIx2;
	while( plst->plst_Roll < -PI ) plst->plst_Roll += PIx2;
/* Decay back to flat. */
	plst->plst_Roll = plst->plst_Roll * ROLL_DECAY_FACTOR;

/* Throttle or brake. */
	if( ButtonBits & ControlA )
	{
		plst->plst_Throttle += THROTTLE_FRACTION_PER_SECOND * plch->plch_MaxThrust * elapsedTime;
		if( plst->plst_Throttle > plch->plch_MaxThrust ) plst->plst_Throttle = plch->plch_MaxThrust;
	}
	if( ButtonBits & ControlB )
	{
		plst->plst_Throttle -= THROTTLE_FRACTION_PER_SECOND * plch->plch_MaxThrust * elapsedTime;
		if( plst->plst_Throttle < 0.0 ) plst->plst_Throttle = 0.0;
/* Brake hard on ground. */
		if( plst->plst_Altitude < 1.0 )
		{
			plst->plst_HorizontalVelocity *= 0.9;
		}
	}
	return 0;
}

/****************************************************************/
int32	ppDoPlanePhysics( PlaneStatus *plst, PlaneCharacteristics *plch, float32 elapsedTime )
{
	float32 LiftForce;
	float32 DragForce;
	float32 ThrustForce;
	float32 GravityForce;
	float32 VerticalComponent;   /* Positive Force is up. */
	float32 HorizontalComponent; /* Positive Force is forward. */
	float32 VerticalForce;   /* Positive Force is up. */
	float32 HorizontalForce; /* Positive Force is forward. */
	float32 CosPitch;
	float32 SinPitch;
	float32 AngleOfTravel;   /* Angle to horizon plane is actually travelling. */
	float32 CosTravel;
	float32 SinTravel;
	float32 AngleToWind;   /* Angle to plane direction of travel. */
	int32 Result = 0;

DBUG(("--------------------------------------------\n" ));
/* Calculate Pitch sin/cos once */
	CosPitch = cosf( plst->plst_Pitch );
	SinPitch = sinf( plst->plst_Pitch );

/* Calculate AirSpeed using Pythagorean theorem. */
	plst->plst_AirVelocity = sqrtf(
		(plst->plst_HorizontalVelocity * plst->plst_HorizontalVelocity) +
		(plst->plst_VerticalVelocity * plst->plst_VerticalVelocity) );

/* Calculate angle of travel to horizon. */
	if( (plst->plst_HorizontalVelocity < 1.0) && (plst->plst_HorizontalVelocity > -1.0) )
	{
		AngleOfTravel = 0.0;
	}
	else
	{
		AngleOfTravel = atan2f( plst->plst_VerticalVelocity, plst->plst_HorizontalVelocity );
	}

	CosTravel = cosf( AngleOfTravel );
	SinTravel = sinf( AngleOfTravel );
	AngleToWind = plst->plst_Pitch - AngleOfTravel;
DBUG(("Pitch = %g, AngleOfTravel = %g, AngleToWind = %g\n", plst->plst_Pitch, AngleOfTravel, AngleToWind ));

/* Calculate DRAG forces along angle of travel. */
/* Drag = R(v) = Av + Bvv = (A + Bv)v */
	DragForce = ((plch->plch_TurbulentDragCoefficient * plst->plst_AirVelocity) +
		plch->plch_ViscousDragCoefficient) * plst->plst_AirVelocity;
/* Project Force along Vertical and Horizontal axes */
	VerticalComponent = -1.0 * SinTravel * DragForce;
	HorizontalComponent = -1.0 * CosTravel * DragForce;
	VerticalForce = VerticalComponent;
	HorizontalForce = HorizontalComponent;

/* Simplistic model. THRUST=Throttle along axis of plane. */
	ThrustForce = plst->plst_Throttle;
	VerticalComponent = SinPitch * ThrustForce;
	HorizontalComponent = CosPitch * ThrustForce;
	VerticalForce += VerticalComponent;
	HorizontalForce += HorizontalComponent;
DBUG(("Thrust = %g, Drag = %g\n", ThrustForce, DragForce ));

/* Calculate LIFT Normal to plane and project. */
	LiftForce = plch->plch_LiftCoefficient * plst->plst_AirVelocity *
		sinf(AngleToWind - plch->plch_ZeroLiftAngle);
	VerticalComponent = CosPitch * LiftForce;
	HorizontalComponent = -1.0 * (SinPitch * LiftForce);
	VerticalForce += VerticalComponent;
	HorizontalForce += HorizontalComponent;
	
/* Apply GRAVITY only to Vertical Force */
	GravityForce = plch->plch_Mass * GRAVITY_ACCELERATION;
	VerticalComponent = -1.0 * GravityForce;
	HorizontalComponent = 0.0;
	VerticalForce += VerticalComponent;
	HorizontalForce += HorizontalComponent;
DBUG(("Gravity = %g, Lift = %g\n", GravityForce, LiftForce ));

/* Change velocities based on forces. */
	plst->plst_HorizontalVelocity += (HorizontalForce / plch->plch_Mass) * elapsedTime;
	plst->plst_VerticalVelocity += (VerticalForce / plch->plch_Mass) * elapsedTime;
DBUG(("HVel = %g, VVel = %g\n", plst->plst_HorizontalVelocity, plst->plst_VerticalVelocity ));

/* Calculate new position based on velocities. */
	plst->plst_XPos += cosf( plst->plst_Yaw ) * plst->plst_HorizontalVelocity * elapsedTime;
	plst->plst_YPos += sinf( plst->plst_Yaw ) * plst->plst_HorizontalVelocity * elapsedTime;
	plst->plst_Altitude += plst->plst_VerticalVelocity * elapsedTime;

/* Check for Crash */
	if( plst->plst_Altitude < COCKPIT_ALTITUDE )
	{
		if( plst->plst_VerticalVelocity < CRASH_METERS_PER_SECOND )
		{
			Result = -1;
		}
		plst->plst_Altitude = COCKPIT_ALTITUDE;
		plst->plst_VerticalVelocity = 0.0;
	}
	return Result;
}

/****************************************************************/
int32	ppReportPlane( PlaneStatus *plst )
{
	PRT(("X = %f, Y = %g, Z = %g\n",
		plst->plst_XPos,
		plst->plst_YPos,
		plst->plst_Altitude ));
	PRT(("Yaw = %f, Pitch = %g, Roll = %g\n",
		plst->plst_Yaw,
		plst->plst_Pitch,
		plst->plst_Roll ));
	return 0;
}
