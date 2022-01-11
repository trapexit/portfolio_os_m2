/******************************************************************************
**
**  @(#) MPEGStreamInfo.c 96/03/29 1.1
**
******************************************************************************/

/****************************************************************************
*																			*
*	File:		MPEGStreamInfo.c											*
*				Version 1.0													*
*				MPEG chunkifier 											*
*	Date:		09-13-1995													*
*	Written by:	Shahriar Vaghar 											*
*																			*
****************************************************************************/

#include <stdio.h>

#ifndef __MPEGUTILS__
#include "MPEGUtils.h"
#endif

#ifndef __TMPEGSTREAMINFO__
#include "MPEGStreamInfo.h"
#endif

/************************ Class TMPEGStreamInfo *********************/

TMPEGStreamInfo::TMPEGStreamInfo()
{
	fFrameCount = 0;
	fCurrentPictureType = 0;
	fCurrentTemporalRef = 0;
	fRefFrameDistance = 0;
	fGOPFrameCount = 0;
}

TMPEGStreamInfo::~TMPEGStreamInfo()
{
	/* Does Nothing. */
}

void
TMPEGStreamInfo::PictureType(int aType)
{
	/* Note: TemporalRef must be called before PictureType. */
	
	if (aType == IFRAME || aType == PFRAME) {
	
		if ((fCurrentTemporalRef - fGOPFrameCount) < 0)
			fGOPFrameCount = 0;
		
		fRefFrameDistance = fCurrentTemporalRef - fGOPFrameCount + 1;
	}
	
	fCurrentPictureType = aType;
}

void
TMPEGStreamInfo::Dump()
{
	printf("%d    Type = %d    Ref. = %d    Ref. Dist. = %d    GOP count = %d\n",
		   fFrameCount, fCurrentPictureType, fCurrentTemporalRef, fRefFrameDistance, fGOPFrameCount);
}

