#ifndef _PLANE_PHYSICS_H
#define _PLANE_PHYSICS_H
/******************************************************************************
**
**  @(#) plane_physics.h 95/07/07 1.1
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
#include "stdio.h"
#include "math.h"

typedef struct PlaneCharacteristics
{
	float32    plch_ViscousDragCoefficient;
	float32    plch_TurbulentDragCoefficient;
	float32    plch_LiftCoefficient;
	float32    plch_ZeroLiftAngle;     /* AngleToWind at which there is no lift. */
	float32    plch_Mass;              /* Kilograms */
	float32    plch_MaxThrust;
} PlaneCharacteristics;

typedef struct PlaneStatus
{
	float32    plst_XPos;
	float32    plst_YPos;
	float32    plst_Altitude;
	float32    plst_Yaw;         /* Compass angle. 0 is East along X axis. */
	float32    plst_YawDelta;    /* Rotational velocity */
	float32    plst_Pitch;       /* Angle of nose to horizon. */
	float32    plst_PitchDelta;  /* Rotational velocity */
	float32    plst_Roll;        /* Angle about central axis. */
	float32    plst_RollDelta;   /* Rotational velocity */
	float32    plst_Throttle;
	float32    plst_HorizontalVelocity;
	float32    plst_VerticalVelocity;
	float32    plst_AirVelocity;   /* Derived from H & V velocity */
} PlaneStatus;

int32	ppInitPlaneStatus( PlaneStatus *plst );
int32	ppCalcPlaneCharacteristics( PlaneCharacteristics *plch, float32 maxGs, float32 mass, float32 maxVel );
int32	ppDoPlanePhysics( PlaneStatus *plst, PlaneCharacteristics *plch, float32 elapsedTime );
int32	ppControlPlane( PlaneStatus *plst, PlaneCharacteristics *plch, uint32 ButtonBits, float32 elapsedTime );
int32	ppReportPlane( PlaneStatus *plst );

#endif /* _PLANE_PHYSICS_H */
