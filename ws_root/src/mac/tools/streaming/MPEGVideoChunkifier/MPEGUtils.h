/******************************************************************************
**
**  @(#) MPEGUtils.h 96/03/29 1.1
**
******************************************************************************/

/****************************************************************************
*                                                                           *
*       File:           MPEGUtils.h                                         *
*                       Version 1.0     	                                *
*                       Definitons for MPEG chunkifier       	            *
*       Date:           02-16-1995                                          *
*       Written by:     Philippe Cassereau                                  *
*                                                                           *
****************************************************************************/

#ifndef __MPEGUTILS__
#define __MPEGUTILS__

#include <stdio.h>
#include ":streaming:dsstreamdefs.h"

enum {
	NOTPICT = -1,	/* picture types */
	IFRAME = 0x08,
	PFRAME = 0x10,
	BFRAME = 0x18
};

char *strstrn(char *ptr1, long len1, char *ptr2, long len2);
long outbits(long val, long whichin, char *ptr, long howmany, long startbit, FILE *outfile);
long markbits(long *userbitcount );
long getUserbitcount(long *userbitcount);
long getbitcount(void);

#endif /* __MPEGUTILS__ */
