
/******************************************************************************
**
**  @(#) weaver.h 96/11/20 1.3
**
******************************************************************************/
/*
 *	File:			Weaver.h
 *
 *	Contains:		definitions for Weaver.c
 *
 *	Written by:		Joe Buczek & friends
 *
 ******************************************************************************/

#ifndef __STREAMING_WEAVER_H
#define __STREAMING_WEAVER_H

#define	BASETIME_FLAG_STRING	"-b"	/* flag for beginning time for output stream */
#define	STREAMBLK_FLAG_STRING	"-s"	/* flag for size of stream blocks */
#define	MEDIABLK_FLAG_STRING	"-m"	/* flag for size of media blocks */
#define	OUTDATA_FLAG_STRING		"-o"	/* flag for output data file name */
#define	VERBOSE_FLAG_STRING		"-v"	/* flag to enable verbose output */
#define	IOBUFSIZE_FLAG_STRING	"-iobs"	/* flag for I/O block size specification */
#define	MAX_MARKERS_FLAG_STRING	"-mm"	/* flag for max number of markers to allow */
#define	MAX_EARLY_TIME_FLAG_STRING	"-mct"	/* flag for max amount of time to pull chunks back
											 *  to fill up "FILL" chunks
											 */
#endif	/* __STREAMING_WEAVER_H */
