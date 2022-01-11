/*
**	File:		view.h	
**
**	Contains:	Header file for "Mercury" data viewer 
**
**	Written by:	Ravindar Reddy
**
**	Copyright:	© 1996 by The 3DO Company. All rights reserved.
**	          	This material constitutes confidential and proprietary
**	          	information of the 3DO Company and shall not be used by
**	          	any Person or for any purpose except as expressly
**	          	authorized in writing by the 3DO Company.
**
**	Last Modified:	 96/04/17	version		1.3	
*/

#ifndef view_INC
#define view_INC	1

typedef struct TimeInfo
{
	gfloat minTime;
	gfloat maxTime;
	gfloat accumTime;
	gfloat totalTime;
	TimerTicks sTime;
	TimerTicks eTime;
} TimeInfo;

typedef struct PerfInfo 
{
	/* constants */
	uint32 disableAnim;
	uint32 printInfo;
	uint32 numFrames;
	uint32 scene3DTriCount;
	uint32 scene2DTriCount;

	/* collected data */
	uint32 total3DTriCount;
	uint32 total2DTriCount;

	TimeInfo fInfo;
	TimeInfo mInfo;
	TimeInfo aInfo;
} PerfInfo;

/* functin prototypes */
void
InitPerfInfo(
	PerfInfo *p,
	uint32 tri_count
	);

void 
AnalyzeTime(
	PerfInfo *p
	);

void 
MoveCameraInit(
	CloseData* cd 
	);

void 
MoveCamera(
	CloseData *pclosedata
	);

BSDF *
GameCodeInit(
	CloseData   **pclosedata, 
	GState *gs, 
	int argc, 
	char **argv
	);

uint32
GameCode(
	BSDF *sdf,
	CloseData *cd
	);

BSDF *
MoveObjectsInit(
    int argc,
    char **argv
    );

#endif
