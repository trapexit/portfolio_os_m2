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
//  .NAME AlCommand - Encapsulates the user defined interface to command history
//
//  .SECTION Description
//      This file contains the definitions required to define an user
//		command history command.
//
//	The command layer in Alias is a software layer that is in-between the user
//	interface and the main application.  A command is an object that represents
//	actions such as: fillet, square, patch or boundary.
//	
//	The user interface code builds commands that are sent through the layer to
//	be executed.  The user interface does not create geometry itself, it
//	creates commands that do it.
//
//	Commands can be saved somewhere and re-executed whenever the system feels
//	it is necessary.  As long as the data the command depends upon is valid,
//	the command remains valid.
//
//	The most practical application of re-executing a command is the generation
//	of data from a piece of source geometry.  When the original geometry changes,
//	the new model can be 'automatically' created by re-executing the command
//	history on the new data.
//
//	Command functions:
//		These are outlined in the AlUserCommand class.
//
//	A `constructor' DAG node is considered to be a DAG node 
//	which creates the result.  An example of a constructor is 
//	one of the original curves in a fillet operation.
//
//	A `target' DAG node is considered to be the result of a command operation.
//	An example is the fillet created by the fillet command.
//
//	The 'AlCommand' class is used to maintain a list of the current plugin
//	defined command history commands.
//	New commands are installed using the 'AlCommand::add' method.  This associates
//	a string with a function that allocates a new copy of a command.
//	AlCommand::remove is used to remove a command type from the database.
//
//	AlCommand::create is used to create an new copy of the command.
//	After a command has been created, its data fields can be accessed using
//	the 'AlCommand->userCommand' method.
//
//	A command has two components, the UI driver and the underlying geometry
//	manipulation code.
//	The UI creates a new command using AlCommand::create.  The private data
//	is accessed using the AlCommand->userCommand method (which is typecast
//	to the plugin's derived command).  From this, the command data structure's
//	fields can be filled in.  The UI uses the execute method to call the
//	userCommand's execute command.  This performs the actual command.
//	Finally the result is added to the Alias system using the 'install'
//	method.
//
//	The Alias messaging system will automatically call the various functions
//	in the command to maintain the dependencies and re-execute the command if
//	necessary.
//

#ifndef _AlCommand
#define _AlCommand

#include <AlStyle.h>
#include <AlObject.h>

class AlUserCommand;
typedef AlUserCommand *AlUserCommandAlloc();

enum AlSurfaceType
{ kSurfaceInvalid, kSurfaceSpline, kSurfacePolyset };

class AlCommand : public AlObject
{
	friend class			AlFriend;
	friend class			AlCommandFriend;

public:
							AlCommand();
	virtual					~AlCommand();		
	AlCommand*				asCommandPtr();
	virtual statusCode		deleteObject();
	virtual AlObjectType	type() const;

	statusCode				create( const char *name );

	statusCode				install();
	statusCode				uninstall();
	statusCode				modified();

	int						execute( boolean freeIfInvalid = TRUE );	
	int						undo( boolean freeIfInvalid = TRUE );
	
	AlSurfaceType			surfaceType() const;
	statusCode				setSurfaceType( AlSurfaceType );

	AlUserCommand*			userCommand();
	int						status() const;

public:
	static statusCode		setDebug( boolean on );
	static statusCode		add( AlUserCommandAlloc *, const char *name );
	static statusCode		remove( const char *name );

private:
	static AlUserCommandAlloc*	find( const char * );
	static AlList*			commandList;

    static void             initMessages();
    static void             finiMessages();
};

#endif	// _AlCommand

