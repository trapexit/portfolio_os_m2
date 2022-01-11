/******************************************************************************
**
**  @(#) PlayEZFlixStream.h 96/03/20 1.12
**  definitions for high level stream playback function
**
******************************************************************************/

#ifndef	_PLAYEZFLIXSTREAM_H_
#define	_PLAYEZFLIXSTREAM_H_

#include <kernel/types.h>

#include <streaming/preparestream.h>
#include <streaming/datastream.h>
#include <streaming/ezflixsubscriber.h>


typedef struct Player	Player, *PlayerPtr;
typedef int32 (*PlayEZFlixUserFn)(PlayerPtr ctx);

enum { cntFrameBuffers = 2 };

typedef struct ScreenInfo {
	Item		bitmapItem[cntFrameBuffers];		/* one Bitmap Item per display buffer */
	Bitmap		*bitmap[cntFrameBuffers];			/* bitmap[i]->bm_Buffer points to the first pixel in the buffer */
	void		*allocatedBuffer[cntFrameBuffers];	/* for deallocation; may include padding on each end */
	Item		viewItem;							/* the View Item to display */
	uint32		renderSignal;						/* this signal tells us when a bitmap has reached the display */
	} ScreenInfo;

/**************************************/
/* Internal player context descriptor */
/**************************************/
struct Player {

	PlayEZFlixUserFn	userFn;				/* user callback function pointer */
	void				*userContext;		/* for use by user callback function */

	DSHeaderChunk		hdr;				/* Copy of stream header from stream file */

	DSDataBufPtr		bufferList;			/* ptr to linked list of buffers used by streamer */
	Item				acqMsgPort;			/* data acquisition thread's request message port */
	DSStreamCBPtr		streamCBPtr;		/* ptr to stream thread's context */

	ScreenInfo			*screenInfoPtr;		/* for drawing to the screen */

	Item				messagePort;		/* port for receiving end of stream message */
	Item				messageItem;		/* msg item for sending streamer requests */
	Item				endOfStreamMessageItem; /* msg item that is replied to as end of stream */

	EZFlixContextPtr	contextPtr;			/* EZFlix subscriber context ptr */
	EZFlixRecPtr		channelPtr;			/* EZFlix subscriber channel ptr */

	Boolean				streamHasAudio;		/* Audio present in the stream? */
	};


/*****************************/
/* Public routine prototypes */
/*****************************/

int32	PlayEZFlixStream( ScreenInfo *screenInfoPtr, 
				char* streamFileName, 
				PlayEZFlixUserFn userFn, 
				void *userContext );
void	DismantlePlayer( PlayerPtr ctx );

#endif	/* _PLAYEZFLIXSTREAM_H_ */
