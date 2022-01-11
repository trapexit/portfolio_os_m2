/*
//-
//	Copyright (C) 1995, Alias|Wavefront
//
//  These coded instructions,  statements and  computer programs contain
//  unpublished information proprietary to Alias|Wavefront  and are
//  protected by the Canadian and US Federal copyright law. They may not
//  be disclosed to third parties  or copied  or duplicated, in whole or
//  in part,  without the prior written consent of Alias|Wavefront
//
//  Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+
*/

/*
//
//  .NAME AlFunction - Class for creating the OpenAlias interface to Alias
//
//  .SECTION Description
//		This class provides a means to interface the OpenAlias application
//		to the Alias user interface.
//
*/

#ifndef _AlFunction
#define _AlFunction

#include <AlStyle.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void AlMouseButtonFunction( int input, Screencoord x, Screencoord y );

#define kModifierControl    0001
#define kModifierShift      0002
#define kModifierAlt        0004
#define kModifierButton1    0010
#define kModifierButton2    0011
#define kModifierButton3    0012
#define kModifierButton4    0014
#define kModifierButton5    0020

/*
// Short cuts for the common ones
*/
#define kButton1    kModifierButton1
#define kButton2    kModifierButton2
#define kButton3    kModifierButton3

typedef enum 
	{ kInputInvalid, kInputAbort, kInputKeyboard, kInputButton, kInputOther }
AlInputType;

typedef enum
	{ kCoordinateInvalid, kCoordinateAbsolute, kCoordinateRelative }
AlCoordinateType;

typedef enum
	{ kBehaviourInvalid, kBehaviourContinuous, kBehaviourMomentary }
AlBehaviourType;

typedef enum
	{ kFilterNone, kFilterLinear, kFilterAngular }
AlFilterType;

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class AlMomentaryFunction;
class AlContinuousFunction;
class AlCamera;

class FuncLink;

class AlFunction
{
	friend class AlFunctionHandle;

public:
	virtual ~AlFunction();
	statusCode deleteObject();

	virtual AlMomentaryFunction*	asMomentaryFunctionPtr();
	virtual AlContinuousFunction*	asContinuousFunctionPtr();

	const char *name();

protected:
	AlFunction();
	void *my_pointer;
};

class AlMomentaryFunction : public AlFunction
{
public:
	virtual AlMomentaryFunction*	asMomentaryFunctionPtr();

	statusCode			create( const char *command, void (*action)( void ));
	statusCode			create( void (*action)(void) );
};
class AlContinuousFunction : public AlFunction
{
public:
	virtual AlContinuousFunction*	asContinuousFunctionPtr();

	statusCode				create( void (*init)( void ),
									AlMouseButtonFunction *down,
									AlMouseButtonFunction *move,
									AlMouseButtonFunction *up,
									void (*cleanup)( void ),
									boolean manipulatesPickList = FALSE );

	statusCode				create( const char *,	
									void (*init)( void ),
									AlMouseButtonFunction *down,
									AlMouseButtonFunction *move,
									AlMouseButtonFunction *up,
									void (*cleanup)( void ),
									boolean manipulatesPickList = FALSE );

	statusCode				setPreInitFunction( void (*preInit)() );
	statusCode				setPostCleanupFunction( void (*postCleanup)() );

	statusCode				setPrompt( const char *staticPrompt, char *inputBuffer, AlFilterType );
	statusCode				setPrompt( const char *(*outputStringFunc)(), char *inputBuffer, AlFilterType );

	statusCode				setBehaviour( AlBehaviourType type );
	statusCode				setMouseCoordinateType( AlCoordinateType type);
	AlBehaviourType			behaviour() const;

	static statusCode		createGoButton( void (*pressed)( void ) );
	static statusCode		clearGoButton( boolean do_redraw );

public:
	static AlCoordinateType	keyboardCoordinateMode();
	static AlInputType		translateInput( int event, int &button );
	static int				inputModifierMask();	// see kModifier_xxxx
};

#endif /* __cplusplus */

#endif	/* _AlFunction */
