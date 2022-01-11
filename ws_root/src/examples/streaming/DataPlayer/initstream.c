/****************************************************************************
**
**  @(#) initstream.c 96/03/27 1.4
**
*****************************************************************************/

#include <kernel/debug.h>
#include <string.h>			/* for memset() */

#include <streaming/datastream.h>
#include <streaming/datastreamlib.h>
#include <streaming/datasubscriber.h>
#include <streaming/dsstreamdefs.h>
#include <streaming/preparestream.h>
#include <streaming/saudiosubscriber.h>
#include <streaming/satemplatedefs.h>
#include <streaming/sacontrolmsgs.h>

#include <streaming/datasubscriber.h>

#include "initstream.h"	


#define kDATASubPriority		-1		/* data subscriber thread delta priority */


/* ----------------------- private prototypes -----------------------  */
static Err		MakeDefaultStreamHeader(DSHeaderChunkPtr streamHdrPtr, uint32 subscriberMask);


/*
 * StartStreamFromHeader
 *	Initialize streaming according to the data found in the stream header.  The
 *	 subscriberMask parameter defines which subscribers to instantiate, and so can
 *	 be used to override the stream header if you only want SOME of the data in
 *	 the stream to be played.  It can also be used to define which subscribers to
 *	 instantiate for a stream with no header, because we manufacture a stream header
 *	 block which specifies ALL subscribers when we find a headerless stream.
 */
Err 
StartStreamFromHeader(StreamBlockPtr ctx, char *streamFile, uint32 subscriberMask)
{
	Err					err;
	DSHeaderChunk		streamHdr;
	DSHeaderSubsPtr		subsPtr;
	int32				subsNdx;
	
	/* clear out the context block as we assume that it doesn't have any valid data in it */
	memset(ctx, 0, sizeof(StreamBlock));
	
	/* try to load a header from the stream, use our defaults if nothing is found */
	if ( kDSHeaderNotFound == (err = FindAndLoadStreamHeader(&streamHdr, streamFile)) )
		err = MakeDefaultStreamHeader(&streamHdr, subscriberMask);
	if ( 0 != err )
		goto ERROR_EXIT;
	
	/* make sure we understand the header version*/
	if ( streamHdr.headerVersion != DS_STREAM_VERSION )
		return kDSVersionErr;
		
	/* allocate memory for the stream buffers */
	if ( NULL == (ctx->dataBufferList = CreateBufferList(streamHdr.streamBuffers, streamHdr.streamBlockSize)) )
	{
		PRNT(("Unable to allocate stream buffers\n"));
		err = kDSNoMemErr;
		goto ERROR_EXIT;
	}

	/* create the client's message port and item */
	err = (ctx->messagePort = NewMsgPort(&ctx->messagePortSignal));
	CHECK_OS_ERR(err, ("Unable to create message port\n"), ERROR_EXIT);

	err = (ctx->messageItem = CreateMsgItem(ctx->messagePort));
	CHECK_OS_ERR(err, ("Unable to create message item\n"), ERROR_EXIT);
		
	err = (ctx->endOfStreamMessageItem = CreateMsgItem(ctx->messagePort));
	CHECK_OS_ERR(err, ("Unable to create EOS message item\n"), ERROR_EXIT);
		
	err = (ctx->acqMsgPort = NewDataAcq(streamFile, streamHdr.dataAcqDeltaPri));
	CHECK_OS_ERR(err, ("Unable to create data acq\n"), ERROR_EXIT);

	err = NewDataStream(&ctx->streamCBPtr, 			/* output: stream control block ptr */
					ctx->dataBufferList, 			/* pointer to buffer list */
					streamHdr.streamBlockSize, 		/* size of each buffer */
					streamHdr.streamerDeltaPri,		/* streamer thread relative priority */
					streamHdr.numSubsMsgs);			/* number of subscriber messages */
	CHECK_OS_ERR(err, ("Failed in NewDataStream()\n"), ERROR_EXIT);

	err = DSConnect(ctx->messageItem, NULL, ctx->streamCBPtr, ctx->acqMsgPort);
	CHECK_OS_ERR(err, ("Failed in DSConnect()\n"), ERROR_EXIT);
	
	for ( subsNdx = 0; 
			subsPtr = &streamHdr.subscriberList[subsNdx],
			subsPtr->subscriberType != 0;
			subsNdx++ ) 
	{
		switch ( subsPtr->subscriberType ) 
		{
			case DATA_SUB_CHUNK_TYPE:
				if ( true == FlagIsSet(subscriberMask, kDataSubFlag) )
				{
					err = NewDataSubscriber(&ctx->dataCBPtr, ctx->streamCBPtr,
											subsPtr->deltaPriority, ctx->messageItem);
					CHECK_OS_ERR(err, ("Failed in NewDataSubscriber()\n"), ERROR_EXIT);
					SetFlag(ctx->subscriberFlags, kDataSubFlag);
				}
				else
					PRNT(("Stream %s has DATA ('%.4s') data but we're not handling it\n", streamFile, (char*)&subsPtr->subscriberType));
				break;

			case SNDS_CHUNK_TYPE:
				if ( true == FlagIsSet(subscriberMask, kAudioSubFlag) )
				{
					err = NewSAudioSubscriber(ctx->streamCBPtr, subsPtr->deltaPriority, ctx->messageItem);
					CHECK_OS_ERR(err, ("Failed in NewSAudioSubscriber()\n"), ERROR_EXIT);
					SetFlag(ctx->subscriberFlags, kAudioSubFlag);
				}
				else
					PRNT(("Stream %s has audio ('%.4s') data but we're not handling it\n", streamFile, (char*)&subsPtr->subscriberType));
				break;

			case CTRL_CHUNK_TYPE:
				/* The ControlSubscriber is obsolete */
				break;

			default:
				PRNT(("StartStreamFromHeader(), Stream \"%s\" has unknown subscriber '%.4s' listed in stream header\n",
							streamFile, (char*) &subsPtr->subscriberType));
				break;
		}
	}

ERROR_EXIT:
	
	/* shut things down if we found an error */
	if ( err < 0)
		ShutDownStreaming(ctx);
		
	return err;
}



/*
 * fill in a stream header with default values
 */
static Err
MakeDefaultStreamHeader(DSHeaderChunkPtr streamHdrPtr, uint32 subscriberMask)
{
	DSHeaderSubsPtr		subsPtr;

	PRNT(("\nWARNING: creating a 'default' stream header with 'SNDS' and 'DATA'\n  subscribers in it.\n\n"));

	memset(streamHdrPtr, 0, sizeof(DSHeaderChunk));

	/* use the default header values */
	streamHdrPtr->chunkType			= HEADER_CHUNK_TYPE;
	streamHdrPtr->headerVersion		= DS_STREAM_VERSION;

	streamHdrPtr->streamBlockSize	= kDefaultBlockSize;
	streamHdrPtr->streamBuffers		= kDefaultNumBuffers;
	streamHdrPtr->streamerDeltaPri	= kStreamerDeltaPri;
	streamHdrPtr->dataAcqDeltaPri	= kDataAcqDeltaPri;
	streamHdrPtr->numSubsMsgs		= kNumSubsMsgs;

	streamHdrPtr->audioClockChan	= kAudioClockChan;
	streamHdrPtr->enableAudioChan	= kEnableAudioChanMask;
	
	/* we initialized the "preloadInstList" fields to zero so we won't preload ANY instruments */

	/* build the subscriber list.  always include the control subscriber, add
	 *  the others based on the user's wishes
	 */
	subsPtr = streamHdrPtr->subscriberList;
	if ( true == FlagIsSet(subscriberMask, kAudioSubFlag) )
	{
		subsPtr->subscriberType	= SNDS_CHUNK_TYPE;
		subsPtr->deltaPriority	= kSoundsPriority;
		subsPtr++;
	}
	if ( true == FlagIsSet(subscriberMask, kDataSubFlag) )
	{
		subsPtr->subscriberType = DATA_SUB_CHUNK_TYPE;
		subsPtr->deltaPriority	= kDATASubPriority;
	}

	return 0;
}


/*
 * ShutDownStreaming
 *	Shut down streaming (or those parts we started anyway).
 */
void
ShutDownStreaming(StreamBlockPtr ctx) 
{
	DSStopStream(ctx->messageItem, NULL, ctx->streamCBPtr, SOPT_FLUSH);

	/* Disconnecting the data acq will start decoupling it from the streamer.
	 * Disposing it will wait until it's done decoupling. */
	if ( NULL != ctx->streamCBPtr ) 
		DSConnect(ctx->messageItem, NULL, ctx->streamCBPtr, 0);
	if ( 0 != ctx->acqMsgPort ) 
		DisposeDataAcq(ctx->acqMsgPort);
	/*  tossing the streamer will shut down all subscribers... */
	if ( NULL != ctx->streamCBPtr ) 
		DisposeDataStream(ctx->messageItem, ctx->streamCBPtr);
	
	if ( NULL != ctx->dataBufferList )
		FreeMem(ctx->dataBufferList, TRACKED_SIZE);
	if ( 0 != ctx->messagePort )	
		DeleteMsgPort(ctx->messagePort);
	if ( 0 != ctx->messageItem )
		DeleteMsg(ctx->messageItem);
	if ( 0 != ctx->endOfStreamMessageItem )
		DeleteMsg(ctx->endOfStreamMessageItem);

	ctx->streamCBPtr = NULL;
	ctx->dataBufferList = NULL;
	ctx->acqMsgPort = ctx->endOfStreamMessageItem = ctx->messagePort =
		ctx->messageItem = 0;
}


