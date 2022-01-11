/*
//-
//	Copyright (C) 1995, Alias|Wavefront
//
//	These coded instructions,  statements and  computer programs contain
//	unpublished information proprietary to Alias|Wavefront  and are
// 	rotected by the Canadian and US Federal copyright law. They may not
//	be disclosed to third parties  or copied  or duplicated, in whole or
//	in part,  without the prior written consent of Alias|Wavefront
//
//	Unpublished-rights reserved under the Copyright Laws of the United States.
//
/+
*/

/*
//
//	.NAME AlMessage - Encapsulates the handling of object messages
//
//	.SECTION Description
//		This file contains the definitions required to handle object messages
//		for Alias objects.
//		A message handler is a user supplied function that is automatically
//		called when an event occurs.
//
//		A message handler is installed using the AlUniverse::addMessageHandler()
//		function.  Below is a sample message handler to handle the deleting of
//		DagNodes:
//
//%@ void myDagNodeDeleteHandler( int msgtype, AlDagNode *dagnode )
//%@ {
//%@%@ do_something_before_its_deleted( dagnode );
//%@ }
//
//		To install the handler into the universe, the following function call
//		would be made in the application's initialization code:
//
//		AlMessage::addMessageHandler( kMessageDagNodeDeleted, myDagNodeDeleteHandler );
//	.br
//		If either Alias or the application deletes a dag node by using
//		AlDagNode.deleteObject(), the message handler will be called with a
//		pointer to the dagnode before the dagnode is removed.
//
//		
//		In addition to being able receive standard messages from inside
//		Alias, it is possible to define custom message types which extend
//		the default set.  Of course, Alias has no prior knowledge of these
//		new types, so hooks cannot be added into existing code to send new
//		messages, but plugins can define messages which can then be received
//		by either the same plugin, or different plugins altogether.  This 
//		allows a collaborative paradigm between plugins, such as sharing of
//		code or data.
//
//		To create a custom message type, call AlMessage::addMessageType.
//		You must supply a name for this type, which is then used by others
//		to find the type.  You are returned an AlMessageTypeHandle, a receipt
//		which the creator of the message type can then use to define
//		specific properties of the message type.  In particular, the owner
//		can lock a message type against the addition of handlers, lock the
//		type against the sending of messages, and set prologue and epilogue
//		functions which get called automatically before and after the sending
//		of a message.  See the documentation for the individual member 
//		functions for more details.
//
//		Note that while a method is supplied to send a message, this method
//		applies only to user-defined messages.  Predefined Alias messages 
//		cannot be sent.
//
*/

#ifndef _AlMessage
#define _AlMessage

#include <AlStyle.h>

typedef enum {
	kMessageFirstMessage = 0,
	kMessageInvalid = 0,

	kMessageDagNodeDeleted,
	kMessageDagNodeInstanced,
	kMessageDeleteAll,
	kMessageDagNodeModified,
	kMessageDagNodeModifiedGeometry,
	kMessageDagNodeModifiedConstraint,
	kMessageDagNodePreReplaceGeometry,
	kMessageDagNodeReplaceGeometry,
	kMessageDagNodeApplyTransformation,
	kMessageDagNodeVisible,
	kMessageCosDeleted,
	kMessageCosModified,
	kMessageCosVisible,
	kMessageAttributesDelete,	
	kMessagePreUpdate,
	kMessageUpdate,
	kMessagePostUpdate,
	kMessageAnimPlayback,
	kMessageListModifiedNodes,
	kMessagePreRefresh,
	kMessageRefresh,
	kMessageCommandInstall,
	kMessageCommandFree,
	kMessagePickListModified,
	kMessageTrimSurface,
	kMessageUntrimSurface,
	kMessagePlotRefresh,
	kMessageUniverseCreated,
	kMessageUniverseDeleted,
	kMessageUniverseMerged,

	kMessageQuit,

	kMessagePreRetrieve,
	kMessagePostRetrieve,
	kMessagePreStore,
	kMessagePostStore,

	kMessageLastMessage /* this is not a valid message, just an ending marker.*/
} AlMessageType;

#ifdef __cplusplus
class AlDagNode;
class AlTM;
class AlCurveOnSurface;
class AlAttributes;
class AlSurface;
class AlFriend;
class AlNotifyDagNode;
class AlObject;
#endif

/*
//
//	Prototypes for the call back functions.  See AlUniverse::addMessageHandler
//	for a summary of which prototypes belong to which messages.
//
*/
typedef void (AlCallbackVoid)          ( AlMessageType );
typedef void (AlCallbackInt)           ( AlMessageType, int );
typedef void (AlCallbackStringInt)	   ( AlMessageType, const char*, int );
typedef void (AlCallbackString)		   ( AlMessageType, const char* );
typedef void (AlCallbackCurveOnSurface)( AlMessageType, AlCurveOnSurface* );
typedef void (AlCallbackAttributes)    ( AlMessageType, AlAttributes* );
typedef int  (AlCallbackUpdate)        ( AlMessageType, int );
typedef void (AlCallbackSurface)       ( AlMessageType, AlSurface* );
typedef void (AlCallbackOneDagNode)    ( AlMessageType, AlDagNode* );
typedef void (AlCallbackTwoDagNodes)   ( AlMessageType, AlDagNode*, AlDagNode*);
typedef void (AlCallbackDagNodeAndMatrix)( AlMessageType, AlDagNode*, AlTM* );

#ifdef __cplusplus
typedef void (AlCallbackPickListModified)( AlMessageType, const AlNotifyDagNode*, AlObject *);
#endif

#ifdef __cplusplus
class AlMessageTypeHandle
{
friend						class AlMessage;
public:
							AlMessageTypeHandle( void );
							AlMessageTypeHandle( const AlMessageTypeHandle& );
							~AlMessageTypeHandle();

	AlMessageTypeHandle&	operator =( const AlMessageTypeHandle& );

	boolean					isValid( void ) const;
	int						type( void ) const;

	statusCode				setPrologue( int (*)( int, void * ) );
	statusCode				setEpilogue( int (*)( int, void * ) );

	statusCode				addLock( boolean& );
	statusCode				setAddLock( boolean );

	statusCode				sendLock( boolean& );
	statusCode				setSendLock( boolean );

private:
							AlMessageTypeHandle( void* );

	void 					*opaque_pointer;
};
#endif

#ifndef __cplusplus

/* C version */
typedef enum {
	AlMessage_kImmediate,
    AlMessage_kQueue,
    AlMessage_kIdleQueue
} AlPriorityType;

#else

class AlMessage
{
	friend class AlFriend;
public:

	enum AlPriorityType {
		kImmediate,
		kQueue,
		kIdleQueue
	};

public:
	static statusCode 	addMessageHandler( int, void * );
	static statusCode 	removeMessageHandler( int, void * );

	static AlMessageTypeHandle 	addMessageType( const char * );
	static int					getMessageType( const char * );
	static statusCode			sendMessage( int, void*, AlPriorityType = kImmediate );

protected:
	static void*		setPostFunction( AlMessageType, void* );
	static void			startMessageHandlers( void );
};

#endif	/* __cplusplus */

#endif	/*  _AlMessage */

