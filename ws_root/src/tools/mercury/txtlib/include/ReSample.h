/*
	File:		ReSample.h

	Contains:	This file contains prototypes for image resampling. 

	Written by:	Anthony Tai, 3DO 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	 3/26/95	TMA		Updated comments.
		<1+>	 1/16/95	TMA		Update headers

	To Do:
*/


#ifndef _H_ReSample
#define _H_ReSample

#define USER_SAMPLE			0x00000001L
#define POINT_SAMPLE		0x00000002L
#define AVERAGE_SAMPLE		0x00000004L
#define WEIGHT_SAMPLE		0x00000008L
#define SINH_SAMPLE			0x00000010L
#define LANCZS3_SAMPLE		0x00000020L
#define MICHELL_SAMPLE		0x00000040L
#define TYPE_LITERAL		0x00000001L
#define TYPE_INDEXED		0x00000002L

void ReSampleImage
	(
	BYTE *src,
	long srcwidth,
	long srcheight,
	BYTE *dst,
	long dstwidth,
	long dstheight,
	long channel,
	long type
	);

void PointSample
	(
	BYTE *src,
	long srcwidth,
	long srcheight,
	BYTE *dst,
	long dstwidth,
	long dstheight,
	long channel
	);

void SinhSample
	(
	BYTE *src,
	long srcwidth,
	long srcheight,
	BYTE *dst,
	long dstwidth,
	long dstheight,
	long channel,
	long filtertype
	);

void SplitChannels
	(
	BYTE **ptr,
	BYTE *src,
	long channel,
	long width,
	long height
	);
	
void MergeChannels
	(
	BYTE **ptr,
	BYTE *dst,
	long channel,
	long width,
	long height
	);

void UserSample
	(
	BYTE **src,					/* array of pointers for each level of detail */
	long srcwidth,				/* finest level of detail in width */
	long srcheight,				/* finest level of detail in height */
	long lod,					/* number of lod */
	long type					/* indexed or literal */
	);

#endif
