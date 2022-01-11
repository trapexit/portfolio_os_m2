/******************************************************************************
**
**  @(#) satemplatedefs.h 96/06/04 1.1
**
******************************************************************************/
#ifndef __STREAMING_SATEMPLATEDEFS_H
#define __STREAMING_SATEMPLATEDEFS_H

/**********************************************/
/* Flags for pre-loading instrument templates */
/**********************************************/                                    

#define	SA_COMPRESSION_SHIFT	24
#define	SA_CHANNELS_SHIFT		16
#define	SA_RATE_SHIFT			8
#define	SA_SIZE_SHIFT			0

/* Sample rate values */
#define	SA_44KHz			1
#define	SA_22KHz			2

/* Sample size values */
#define	SA_16Bit			1
#define	SA_8Bit				2

/* Sample channel values */
#define	SA_STEREO			2
#define	SA_MONO				1

/* Sample compression type values */
#define	SA_COMP_NONE		1
#define	SA_COMP_SDX2		2
#define	SA_COMP_ADPCM4		4

/* Macro to create instrument tags from human usable parameters. 
 * The tag values created by this macro are used by the 
 * GetTemplateItem() routine.
 */
#define	MAKE_SA_TAG( rate, bits, chans, compression )	\
	((long) ( (rate << SA_RATE_SHIFT ) 					\
	| (bits << SA_SIZE_SHIFT) 							\
	| (chans << SA_CHANNELS_SHIFT)						\
	| (compression << SA_COMPRESSION_SHIFT) ) )


enum InstrumentTags {
	/* The following are Opera instrument tags and are kept for compatibility only. */
	SA_44K_16B_S = (MAKE_SA_TAG( SA_44KHz, SA_16Bit, SA_STEREO, SA_COMP_NONE )), 
	SA_44K_16B_M = (MAKE_SA_TAG( SA_44KHz, SA_16Bit, SA_MONO, SA_COMP_NONE )),
	SA_44K_8B_S = (MAKE_SA_TAG( SA_44KHz, SA_8Bit, SA_STEREO, SA_COMP_NONE )),
	SA_44K_8B_M = (MAKE_SA_TAG( SA_44KHz, SA_8Bit, SA_MONO, SA_COMP_NONE )),
	SA_22K_16B_S = (MAKE_SA_TAG( SA_22KHz, SA_16Bit, SA_STEREO, SA_COMP_NONE )),
	SA_22K_16B_M = (MAKE_SA_TAG( SA_22KHz, SA_16Bit, SA_MONO, SA_COMP_NONE )),
	SA_22K_8B_S = (MAKE_SA_TAG( SA_22KHz, SA_8Bit, SA_STEREO, SA_COMP_NONE )),
	SA_22K_8B_M = (MAKE_SA_TAG( SA_22KHz, SA_8Bit, SA_MONO, SA_COMP_NONE )),
	SA_44K_16B_S_SDX2 = (MAKE_SA_TAG( SA_44KHz, SA_16Bit, SA_STEREO, SA_COMP_SDX2 )),
	SA_44K_16B_M_SDX2 = (MAKE_SA_TAG( SA_44KHz, SA_16Bit, SA_MONO, SA_COMP_SDX2 )),
	SA_22K_16B_S_SDX2 = (MAKE_SA_TAG( SA_22KHz, SA_16Bit, SA_STEREO, SA_COMP_SDX2 )),
	SA_22K_16B_M_SDX2 = (MAKE_SA_TAG( SA_22KHz, SA_16Bit, SA_MONO, SA_COMP_SDX2 )),
	SA_44K_16B_M_ADPCM4 = (MAKE_SA_TAG( SA_44KHz, SA_16Bit, SA_MONO, SA_COMP_ADPCM4 )),
	SA_22K_16B_M_ADPCM4 = (MAKE_SA_TAG( SA_22KHz, SA_16Bit, SA_MONO, SA_COMP_ADPCM4 )),

	/* Compressed M2 instrument tags */
	SA_44K_16B_S_SQD2 = 0,
	SA_44K_16B_M_SQD2,
	SA_22K_16B_S_SQD2,
	SA_22K_16B_M_SQD2,
	
	/* The new consistent ID type for 4:1 compression */
	SA_44K_16B_M_ADP4,
	SA_22K_16B_M_ADP4,
	
	/* Compressed instrument tags for M2 hardware 2:1 */
	SA_44K_16B_S_SQS2, /* Currently Unsupported */
	SA_44K_16B_M_SQS2,
	SA_22K_16B_S_SQS2, /* Currently Unsupported */
	SA_22K_16B_M_SQS2,
	
	SA_44K_16B_S_CBD2,
	SA_44K_16B_M_CBD2,
	SA_22K_16B_S_CBD2,
	SA_22K_16B_M_CBD2
};

#endif /* __STREAMING_SATEMPLATEDEFS_H */

