/******************************************************************************
**
**  @(#) PortableSANMDefs.h 95/05/26 1.1
**
******************************************************************************/
/**	File:			portablesanmdefs.h
 *
 *	Contains:		defintions are needed to write out the streamed animation data files
 *
 ******************************************************************************/

#ifndef _PORTABLESANMDEFS_H_
#define _PORTABLESANMDEFS_H_

#include ":streaming:subschunkcommon.h"	/* found in {3DOINCLUDES}Streaming: folder */

typedef	struct StreamAnimCCB {		/*	subType = 'ACCB'			*/
	SUBS_CHUNK_COMMON;
	unsigned long	ccb_Flags;		/*	32 bits of CCB flags		*/
	struct			CCB *ccb_NextPtr;
	long			*ccb_CelData;
	void*			ccb_PLUTPtr;
	long 			ccb_X;
	long 			ccb_Y;
	long			ccb_hdx;
	long			ccb_hdy;
	long			ccb_vdx;
	long			ccb_vdy;
	long			ccb_ddx;
	long			ccb_ddy;
	unsigned long	ccb_PPMPC;
	unsigned long	ccb_PRE0;		/* Sprite Preamble Word 0 */
	unsigned long	ccb_PRE1;		/* Sprite Preamble Word 1 */
	long			ccb_Width;
	long			ccb_Height;
	} StreamAnimCCB, *StreamAnimCCBPtr;


typedef	struct StreamAnimFrame {	/* subType = 'FRME' */
	SUBS_CHUNK_COMMON;
	long		duration;		/* in audio ticks */

	/*	In here goes a PDAT (and maybe a PLUT chunk) */

	} StreamAnimFrame, *StreamAnimFramePtr;

/* Definitions for streamed animation blocks */
#ifndef SANM_CHUNK_TYPE
	#define	SANM_CHUNK_TYPE		('SANM')
	#define SANM_HEADER_TYPE	('AHDR')
	#define	AHDR_CHUNK_TYPE		('AHDR')
	#define	ACCB_CHUNK_TYPE		('ACCB')
	#define	FRME_CHUNK_TYPE		('FRME')
#endif

#endif	/* _PORTABLESANMDEFS_H_ */
