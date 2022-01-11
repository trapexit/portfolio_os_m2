/*******************************************************************************************
 **
 **  @(#) EZFlixCompress.h 95/10/01 1.2
 **
 *	File:			EZFlixCompress.h
 *
 *	Contains:		Internal interface to EZFlix compressor
 *
 *	Written by:		Greg Wallace
 *
 *******************************************************************************************/

#ifndef __EZFlixCompress__
#define __EZFlixCompress__

#include "EZFlixXPlat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Portable Pixel map header */
typedef struct PPMHeader
{
	unsigned short	PPMWidth;	/* width in file */
	unsigned short	PPMHeight;	/* height in file */
	
	unsigned short	width;
	unsigned short	height;
	unsigned short	depth;
	RGBTriple *		pixels;
} PPMHeader;
		
/* Our Params record */
typedef struct EZFlixParams 
{
	PPMHeader		pixelMapHeader;		/* Internal image buffer.  Images to be compressed	*/
										/* are converted from their native pixel format and	*/
										/* depth to a standard 32-bit (8,8,8) RGB image in	*/
										/* this buffer before being passed to the lower		*/
										/* level of the compressor.							*/
	Boolean			verbose;
	unsigned char*	tempBuf;			/* temp buffer pointer */
	long			tempBufWidth;
	long			tempBufHeight;		/* dimensions used to allocate tempBuf */
	unsigned char*	uDecBase;
	unsigned char*	vDecBase;
	long 			compCount;
	long			quality;			/* QT Quality setting */
} EZFlixParams, *EZFlixParamsPtr;

/* Comporessor Internal Functions */

extern long GWcompress_frame( EZFlixParamsPtr params, 
								unsigned long *compressed, 
								unsigned long* resultingSize );

#ifdef __cplusplus
}
#endif

#endif /* __EZFlixCompress__ */

