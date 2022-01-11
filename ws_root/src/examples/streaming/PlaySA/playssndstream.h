/******************************************************************************
**
**  @(#) playssndstream.h 96/03/29 1.13
**
******************************************************************************/

#ifndef	_PLAYSTREAM_H_
#define	_PLAYSTREAM_H_

#include <kernel/types.h>

#include <streaming/datastream.h>
#include <streaming/preparestream.h>
#include <streaming/saudiosubscriber.h>


typedef struct Player	Player, *PlayerPtr;
typedef int32 (*PlaySSNDUserFn)(PlayerPtr ctx);

/**************************************/
/* Internal player context descriptor */
/**************************************/
struct Player {

	PlaySSNDUserFn	userFn;			/* user callback function pointer */
	void*			userCB;			/* Context for the user function */

	DSHeaderChunk	hdr;			/* Copy of stream header from stream
									 * file */

	DSDataBufPtr	bufferList;		/* ptr to linked list of buffers
									 * used by streamer */
	Item			acqMsgPort;		/* data acquisition's request msg port */
	DSStreamCBPtr	streamCBPtr;	/* ptr to stream thread's context */

	Item			messagePort;	/* port for receiving end of stream
									 * message */
	/* uint32		messagePortSignal; */
	Item			messageItem;	/* msg item for sending streamer requests */
	Item			endOfStreamMessageItem;
									/* msg item that is replied to as
									 * end of stream */
	DSDataType		datatype;		/* 'SNDS' or 'MPAU' */

	};


/*****************************/
/* Public routine prototypes */
/*****************************/

int32	PlaySSNDStream( char* streamFileName, PlaySSNDUserFn userFn,
						void* userCB, int32 callBackInterval );
void	DismantlePlayer( PlayerPtr ctx );

#endif	/* _PLAYSTREAM_H_ */
