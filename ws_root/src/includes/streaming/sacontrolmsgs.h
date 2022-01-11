/****************************************************************************
**
**  @(#) sacontrolmsgs.h 96/03/04 1.7
**
*****************************************************************************/
#ifndef __SACONTROLMSGS_H__
#define __SACONTROLMSGS_H__

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

/***********************************************/
/* Opcode values as passed in control messages */
/***********************************************/
enum SAudioControlOpcode {
		kSAudioCtlOpLoadTemplates = 0,
		kSAudioCtlOpSetVol,	
		kSAudioCtlOpSetPan,	
		kSAudioCtlOpSetClockChan,	
		kSAudioCtlOpGetVol,	
		kSAudioCtlOpGetFreq,	
		kSAudioCtlOpGetPan,	
		kSAudioCtlOpGetClockChan,
		kSAudioCtlOpCloseChannel,
		kSAudioCtlOpFlushChannel,
		kSAudioCtlOpMute,
		kSAudioCtlOpUnMute
		};

/**************************************/
/* Control block 					  */
/**************************************/
typedef union SAudioCtlBlock {

		struct {					/* kSAudioCtlOpLoadTemplates */
			int32	*tagListPtr;	/* NULL terminated tag list pointer */
			} loadTemplates;

		struct {					/* kSAudioCtlOpSetVol, kSAudioCtlOpGetVol */
			uint32	channelNumber;	/* which channel to control */
			int32	value;
			} amplitude;

		struct {					/* kSAudioCtlOpSetPan, kSAudioCtlOpGetPan */
			uint32	channelNumber;	/* which channel to control */
			int32	value;
			} pan;

		struct {					/* kSAudioCtlOpMuteSAudioChannel */
			uint32	channelNumber;	/* channel to make clock channel */
			} mute;

		struct {					/* kSAudioCtlOpUnMuteSAudioChannel */
			uint32	channelNumber;	/* channel to make clock channel */
			} unMute;

		struct {					/* kSAudioCtlOpSetClockChannel */
			uint32	channelNumber;	/* channel to make clock channel */
			} clock;

		struct {					/* kSAudioCtlOpCloseChannel */
			uint32	channelNumber;	/* channel to close */
			} CloseSAudioChannel;

		struct {					/* kSAudioCtlOpFlushChannel */
			uint32	channelNumber;	/* channel to flush */
			} FlushSAudioChannel;
			
	} SAudioCtlBlock, *SAudioCtlBlockPtr;


#endif /* __SACONTROLMSGS_H__ */
