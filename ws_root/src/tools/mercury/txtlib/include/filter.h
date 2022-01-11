/*
	File:		filter.h

	Contains:	This file contains definition for resampling filter 

	Written by:	Anthont Tai, 3DO 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<1+>	 5/30/95	TMA		Add headers for setting filter width functions.
		<1+>	 1/16/95	TMA		Update header information

	To Do:
*/

#ifndef _H_filter
#define _H_filter


/*
 * defines for different types of filter
 */
#define BOX_FILTER			0x00000001
#define TRIANGLE_FILTER			0x00000002
#define BELL_FILTER			0x00000004
#define B_SPLINE_FILTER			0x00000008
#define _FILTER					0x00000010
#define LANCZS3_FILTER			0x00000020
#define MICHELL_FILTER			0x00000040

#ifndef M_PI
#  define M_PI		3.14159265358979323846
#endif

/*
 * prototypes for filtering function 
 */

double SetFilterWidth(long type,double fwidth);
double GetFilterWidth(long type);
void ResetFilterWidth(void);

void preparezoom
	(
	BYTE *srcptr,
	long srcwidth,
	long srcheight,
	BYTE *dstptr,
	long dstwidth,
	long dstheight,
	long filtertype
	);

#endif
