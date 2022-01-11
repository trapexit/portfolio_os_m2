/****************************************************************************
**
**  @(#) initstream.h 96/03/27 1.5
**
*****************************************************************************/

#ifndef _INIT_STREAM_
#	define	_INIT_STREAM_

#include <streaming/datastream.h>
#include <streaming/saudiosubscriber.h>
#include <streaming/datasubscriber.h>


/* handy macros */
#ifndef SetFlag
#define SetFlag(val, flag)		((val)|=(flag))
#define ClearFlag(val, flag)	((val)&=~(flag))
#define FlagIsSet(val, flag)	((bool)(((val)&(flag))!=0))
#define FlagIsClear(val, flag)	((bool)(((val)&(flag))==0))
#endif


#ifndef PRINT_LEVEL
#define PRINT_LEVEL 0  /* default value, can be preset by the #includer */
#endif

#if defined(DEBUG) || (PRINT_LEVEL >= 2)
#define CHECK_OS_ERR(errCode, printfArgs, exitLabel) { if ( (errCode) < 0 ) \
						{ (void)printf printfArgs; PrintfSysErr(errCode); goto exitLabel;} }
#else
#define	CHECK_OS_ERR(errCode, printfArgs, exitLabel) {if ( (errCode) < 0 ) goto exitLabel;}
#endif

/* bits to define which subscribers to instantiate.   */
enum
{
	kControlSubFlag		= (1 << 0),
	kAudioSubFlag		= (1 << 1),
	kDataSubFlag		= (1 << 2),
	kAllSubFlags		= (0x7FFFFFFF)
};


typedef struct StreamBlock
{
	DSDataBufPtr		dataBufferList;			/* list of buffers to use for streaming */
	DSStreamCBPtr		streamCBPtr;			/* Stream Context Block */
	Item				acqMsgPort;				/* data acquisition request msg port */
	
	Item				messagePort;			/* port for communicating with the streamer */
	Item				messageItem;
	uint32				messagePortSignal;		/* port's signal */
	Item				endOfStreamMessageItem;	/* item to get end of stream notification */
	
	DataContextPtr		dataCBPtr;				/* data subscriber context */
	uint32				subscriberFlags;		/* bit field, one bit set for each subscriber created */
} StreamBlock, *StreamBlockPtr;


#ifdef __cplusplus 
  extern "C" {
#endif


extern	Err		StartStreamFromHeader(StreamBlockPtr ctx, char *streamFile, uint32 subscriberMask);
extern	void	ShutDownStreaming(StreamBlockPtr ctx);

#ifdef __cplusplus
  }
#endif

#endif /* _INIT_STREAM_ defined */
