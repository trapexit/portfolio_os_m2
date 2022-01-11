//-
//	Copyright (C) 1995, Alias|Wavefront
//
//  These coded instructions,  statements and  computer programs contain
//  unpublished information proprietary to Alias|Wavefront  and are
//  rotected by the Canadian and US Federal copyright law. They may not
//  be disclosed to third parties  or copied  or duplicated, in whole or
//  in part,  without the prior written consent of Alias|Wavefront
//
//  Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+

//
//  .NAME AlUserCommand - Encapsulates the user defined interface to command history
//
//  .SECTION Description
//      This file contains the definitions required to define an user
//		command history command.
//

#ifndef _AlUserCommand
#define _AlUserCommand

#include <AlStyle.h>

class AlObject;
class AlNotifyDagNode;
class AlCommand;
class AlDagNode;
class AlCurveOnSurface;
class AlContact;
class AlCommandRef;

class ofstream;

class AlOutput
{
public:
	virtual statusCode output( const void *data, int size );
	virtual statusCode declareObject( AlObject *obj );

protected:
			AlOutput() {};
	virtual ~AlOutput() {};
};

class AlInput
{
public:
	virtual statusCode	input( void *data, int size );
	virtual int			inputRemaining() const;

	virtual AlObject*	resolveObject( AlObject *obj );

protected:
			AlInput() {};
	virtual ~AlInput() {};
};

class AlUserCommandPrivate;

class AlUserCommand
{
	friend class AlCommandFriend;

public:
	// overloaded user functions
	// do not call these directly or the command history may not be correctly
	// maintained
	//
	virtual				~AlUserCommand();
	virtual int			isValid();
	virtual int			execute();
	virtual int			declareReferences();
	virtual int			instanceDag( AlDagNode *oldDag, AlDagNode *newDag );
	virtual int			undo();
	virtual int			dagModified( AlDagNode *dag );
	virtual int			geometryModified( AlDagNode *dag );
	virtual int			curveOnSurfaceModified( AlCurveOnSurface *surf );
	virtual int  		listModifiedDagNodes( const AlNotifyDagNode *dagMod, AlObject *obj );
	virtual int			debug( const char *prefix );
	virtual void *		asDerivedPtr();

	virtual statusCode	storeWire( AlOutput *output );
	virtual statusCode	storeSDL( ofstream &outputSDL );
	virtual statusCode 	retrieveWire( AlInput *input );
	virtual statusCode	resolveWire( AlInput *input );

	// for your own use
	virtual int			type();	

public:
// general utility functions
	const char *		name() const;
	AlCommand *			command() const;

// declare utility functions
	AlCommandRef*		firstConstructorRef();
	statusCode			addConstructorRef( AlDagNode * );
	statusCode			addConstructorRef( AlCurveOnSurface *);
	statusCode			addConstructorContact( AlDagNode *, AlContact * );

	AlCommandRef*		firstTargetRef();
	statusCode			addTargetRef( AlDagNode * );
	statusCode			addTargetRef( AlCurveOnSurface * );
	statusCode			addTargetContact( AlDagNode *, AlContact * );
	statusCode			deleteAllReferences();

	boolean				isDagNodeAConstructor( AlDagNode * ) const;
	boolean				isDagNodeATarget( AlDagNode *, boolean includeCOS = FALSE ) const;
	boolean				isCurveOnSurfaceAConstructor( AlCurveOnSurface *) const;
	boolean				isCurveOnSurfaceATarget( AlCurveOnSurface *) const;

protected:
	// This constructor is not used in the base class (since it is pure).
	// Defined your 'userCmdConstructor' to call the constructor for your
	// derived class.
	AlUserCommand();

private:
	// don't touch this
	AlUserCommandPrivate *privateData;
};

#endif	// _AlUserCommand
