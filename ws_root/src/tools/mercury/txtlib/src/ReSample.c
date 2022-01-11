/*
	File:		ReSample.c

	Contains:	This module contains functions which resample the images 

	Written by:	Anthony Tai 

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

#include "qGlobal.h"

#include <stdlib.h>
#include "ReSample.h"
#include "filter.h"


/****************************************************************************************
 *	ReSample32
 *
 *	ReSample 32 bit images
 ****************************************************************************************/
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
	)
{
	switch (type)
	{
		case POINT_SAMPLE:
			PointSample(src,srcwidth, srcheight, dst, dstwidth, dstheight, channel);
			break;
		case AVERAGE_SAMPLE:
			SinhSample(src,srcwidth, srcheight, dst, dstwidth, dstheight, channel, BOX_FILTER);
			break;
		case WEIGHT_SAMPLE:
			SinhSample(src,srcwidth, srcheight, dst, dstwidth, dstheight, channel, TRIANGLE_FILTER);
			break;
		case SINH_SAMPLE:
			SinhSample(src,srcwidth, srcheight, dst, dstwidth, dstheight, channel, B_SPLINE_FILTER);
			break;
		case LANCZS3_SAMPLE:
			SinhSample(src,srcwidth, srcheight, dst, dstwidth, dstheight, channel, LANCZS3_FILTER);
			break;
		case MICHELL_SAMPLE:
			SinhSample(src,srcwidth, srcheight, dst, dstwidth, dstheight, channel, MICHELL_FILTER);
			break;
		case USER_SAMPLE:
			/*UserSample(src,srcwidth, srcheight, dst, dstwidth, dstheight, channel);*/
			break;
		default:
			SinhSample(src,srcwidth, srcheight, dst, dstwidth, dstheight, channel, _FILTER);
	}
}

/****************************************************************************************
 *	PointSample
 *
 *	Point Sample 32 bit images
 ****************************************************************************************/
void PointSample
	(
	BYTE *src,
	long srcwidth,
	long srcheight,
	BYTE *dst,
	long dstwidth,
	long dstheight,
	long channel
	)
{
	long *xindex, *yindex;
	double scalex, scaley;
	long x, y, c, offset;

	scalex = (double)srcwidth / (double)dstwidth;
	scaley = (double)srcheight / (double)dstheight;
	xindex = (long *)malloc(dstwidth * sizeof(long));
	yindex = (long *)malloc(dstheight * sizeof(long));
	for (y = 0; y < dstheight; y++)
		yindex[y] = (long)(scaley * y);
	for (x = 0; x < dstwidth; x++)
		xindex[x] = (long)(scalex * x);
	for (y = 0; y < dstheight; y++)
	{
		for (x = 0; x < dstwidth; x++)
		{
			offset = srcwidth * yindex[y] + xindex[x];
			offset = offset * channel;
			for (c = 0; c < channel; c++)
			{
				*dst = *(src + offset + c);
				dst++;
			}
		}
	}
}

/****************************************************************************************
 *	SplitChannels
 *
 *	split Channels
 ****************************************************************************************/
void SplitChannels
	(
	BYTE **ptr,
	BYTE *src,
	long channel,
	long width,
	long height
	)
{
	long c, x, y;
	
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			for (c = 0; c < channel; c++)
			{
				*ptr[c] = *src;
				ptr[c]++;
				src++;
			}
		}
	}
}

/****************************************************************************************
 *	MergeChannels
 *
 *	Merge Channels
 ****************************************************************************************/
void MergeChannels
	(
	BYTE **ptr,
	BYTE *dst,
	long channel,
	long width,
	long height
	)
{
	long c, x, y;
	
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			for (c = 0; c < channel; c++)
			{
				*dst = *ptr[c];
				ptr[c]++;
				dst++;
			}
		}
	}
}

/****************************************************************************************
 *	SinhSample
 *
 *	Sinh Sample 32 bit images
 ****************************************************************************************/
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
	)
{
	BYTE *srcptr, *dstptr, *ptr, *ptr1;
	long i, j, srcsize, dstsize;

	srcsize = srcwidth * srcheight;
	srcptr = (BYTE *)malloc (srcsize);
	dstsize = dstwidth * dstheight;
	dstptr = (BYTE *)malloc (dstsize);
	for (i = 0; i < channel; i++)
	{
		ptr = srcptr;
		ptr1 = src + i;
		for (j = 0; j < srcsize; j++)
		{
			*ptr = *ptr1;
			ptr++;
			ptr1 += channel;
		}
	
		/* using the algorithm from GemIII */
		preparezoom(srcptr, srcwidth, srcheight, dstptr, dstwidth, dstheight, filtertype);
		
		
		ptr = dstptr;
		ptr1 = dst + i;
		for (j = 0; j < dstsize; j++)
		{
			*ptr1 = *ptr;
			ptr++;
			ptr1 += channel;
		}
	}
	free(srcptr);
	free(dstptr);
}

/****************************************************************************************
 *	UserSample
 *
 *	User needs to provide each level of detail. If the texture is indexed, a color
 *	palette is also needed.
 ****************************************************************************************/
void UserSample
	(
	BYTE **src,
 	long srcwidth,				/* finest level of detail in width */
	long srcheight,				/* finest level of detail in height */
	long lod,					/* number of lod */
	long type					/* indexed or literal */
	)
{
	BYTE **mySrc;
	long width, height, myLOD;
	
	mySrc = src;
	width = srcwidth;
	height = srcheight;
	myLOD = lod;
	
	if (type == TYPE_LITERAL)
	{
	
	}
	else if (type == TYPE_INDEXED)
	{
	
	}
}
