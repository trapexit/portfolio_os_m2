/******************************************************************************
**
**  @(#) playvideostream.h 96/08/30 1.14
**
******************************************************************************/

#ifndef	_PLAYSTREAM_H_
#define	_PLAYSTREAM_H_

#include <kernel/types.h>
#include <graphics/graphics.h>
#include <graphics/bitmap.h>

#include <streaming/datastream.h>
#include <streaming/saudiosubscriber.h>
#include <streaming/mpegvideosubscriber.h>
#include <streaming/mpegbufferqueues.h>
#include <streaming/preparestream.h>


typedef struct Player	Player, *PlayerPtr;

/* kUserFunctionPeriod defines how often the user function gets called to check for such things
 * as control pad input. This is in units of audio ticks (approx 240 Hz). 60 Hz polling is nice. */
#define kUserFunctionPeriod (240/60)

typedef int32 (*PlayVideoUserFn)(PlayerPtr ctx);


enum { cntFrameBuffers = 2 };

typedef struct ScreenInfo {
	Item		bitmapItem[cntFrameBuffers];		/* one Bitmap Item per display buffer */
	Bitmap		*bitmap[cntFrameBuffers];			/* bitmap[i]->bm_Buffer points to the first pixel in the buffer */
	void		*allocatedBuffer[cntFrameBuffers];	/* for deallocation; may include padding on each end */
	Item		viewItem;							/* the View Item to display */
	uint32		renderSignal;						/* this signal tells us when the old Bitmap is no longer visible */
	uint32		displaySignal;						/* this signal tells us when the new Bitmap has become visible */
	bool		displayedFirstFrameYet;				/* did we display the first frame yet? */
	} ScreenInfo;


/**************************************/
/* Internal player context descriptor */
/**************************************/
struct Player {
	PlayVideoUserFn			userFn;					/* user callback function pointer */
	void					*userContext;			/* for use by user callback function */
	Item					userCue;				/* Cue for periodic wake ups to check user function */
	uint32					userCueSignal;


	DSHeaderChunk			hdr;					/* Copy of stream header from stream file */

	DSDataBufPtr			bufferList;				/* ptr to linked list of buffers used by streamer */
	Item					acqMsgPort;				/* data acquisition request msg port */
	DSStreamCBPtr			streamCBPtr;			/* ptr to stream thread's context */

	ScreenInfo				*screenInfoPtr;			/* for drawing to the screen */

	Item					messagePort;			/* port for receiving end of stream message */
	uint32					messagePortSignal;

	Item					messageItem;			/* msg item for sending streamer requests */
	Item					endOfStreamMessageItem; /* msg item that is replied to as end of stream */

	Item					frameCompletionPort;	/* The port for recieving frame completion messages */
	uint32					frameCompletionPortSignal;

	Item					lastFrameMsg;			/* a reference to the last message we recieved.  This is
													 * used to reply to the message once a new frame has been
													 * put up on the screen */

	Item					mpegVideoSubMsgPort;	/* MPEGVideo subscriber request message port */
	Item					synchCue;				/* Audio Cue Item for synchronization timing */
	uint32					synchCueSignal;
	MPEGBufferPtr			lastMPEGBufferPtr;		/* pointer to the last mpeg buffer we recieved */

	int32					bitDepth;				/* Bitdepth of playback */
	bool					fLoop;					/* looping playback? */
	DSDataType				datatype;
	};


/*****************************/
/* Public routine prototypes */
/*****************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


Err		PlayVideoStream(ScreenInfo *screenInfoPtr, uint32 bitDepth,
			char *streamFileName, PlayVideoUserFn userFn, void *userContext,
			bool fLoop);
void	DismantlePlayer(PlayerPtr ctx);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* _PLAYSTREAM_H_ */
