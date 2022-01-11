/******************************************************************************
**
**  @(#) JoyPad.c 95/12/13 1.3
**	Routines to read the control pad.
**
******************************************************************************/

#include <kernel/debug.h>
#include <misc/event.h>
#include "JoyPad.h"

/* private static variables */
		Boolean				gPadInitialized = false;
static	ControlPadEventData	gCPadData;
static	Boolean				gLeftHandedPad[kDefaultCtrlPadCount] = {0,0,0,0};
static	int32				gJoyContinuousBtns[kDefaultCtrlPadCount] = {0,0,0,0};
static	uint32				gLastJoySettings[kDefaultCtrlPadCount] = {0,0,0,0};


/*
 * InitJoypad
 *
 *  Initialize the event manager's simple joypad interface for our use.
 */
static int32
InitJoypad(int32 numControlPads )
{
	int32	errorCode;
	
	if ( gPadInitialized )
		return 0;
		
	errorCode = InitEventUtility(numControlPads, 0, LC_FocusListener);
	CHECK_NEG("InitJoypad: error in InitEventUtility", errorCode);
	
	gPadInitialized = true;
	return errorCode;
}


/*
 * ReadJoyPad
 *
 *  Read the specified pad, return the current button state.
 */
static int32
ReadJoyPad(uint32 *buttons, int32 padNum)
{
	int32	errorCode;
	errorCode = GetControlPad (padNum, FALSE, &gCPadData);
	CHECK_NEG("ReadJoyPad: error in GetControlPad", errorCode);
	
	*buttons = gCPadData.cped_ButtonBits;
	return errorCode;
}


/*
 * KillJoypad
 *
 *  Kill the event manager's simple joypad interface.
 */
int32
KillJoypad( void )
{ 
	int32	errorCode = 0;
	
	if ( gPadInitialized )
	{
		errorCode = KillEventUtility();
		CHECK_NEG("TermJoypad: error in KillEventUtility", errorCode);
		gPadInitialized = FALSE;
	}

	return errorCode;
}


/*
 * SetJoyPadContinuous
 *
 * set which joypad buttons may be held down continuously.  return the old setting
 *  in case the user wants to restore it later.
 */
int32 
SetJoyPadContinuous(int32 continuousBtnFlags, int32 padNum)
{
	int32	oldJoyBits;
	
	oldJoyBits = gJoyContinuousBtns[padNum - 1];
	gJoyContinuousBtns[padNum - 1] = continuousBtnFlags;
	return oldJoyBits;
}


/*
 * GetJoyPadContinuous
 *
 * return which joypad buttons may be held down continuously.  
 */
int32 
GetJoyPadContinuous(int32 padNum)
{
	return gJoyContinuousBtns[padNum - 1];
}


/*
 * SetJoyPadLeftHanded
 *
 * allow the user to play with the standard pad upsidedown, or in a left
 *  handed orientation.  
 */
void 
SetJoyPadLeftHanded(Boolean playLeftHanded, int32 padNum)
{
	gLeftHandedPad[padNum - 1] = playLeftHanded;
}


/*
 * GetJoyPadState
 *
 * Return the state of the joypad switches that have made a transition
 * from off to on since the last call.
 */
static 
uint32 GetJoyPadState(int32 padNum)
{
	uint32	currentJoy;				/* current joystick settings */
	uint32	resultJoy;				/* settings returned to caller */

	/* read the current joypad buttons settings */
	if ( ReadJoyPad(&currentJoy, padNum) < 0 )
		currentJoy = 0;
	resultJoy = currentJoy ^ gLastJoySettings[padNum - 1];
	resultJoy &= currentJoy;

	/* mask out any buttons which aren't allowed to be held down continuously, */
	/*  remember what we're returning */
	resultJoy |= (currentJoy & gJoyContinuousBtns[padNum - 1]);
	gLastJoySettings[padNum - 1] = currentJoy;

	return resultJoy;
}


/*
 * GetJoyPad
 *
 * read the joypad and report the state of the buttons.  continuously
 *  pressed buttons are only reported if their bits are set in the 
 *  global ÔgJoyContinuousBtnsÕ.
 * if the user has requested lefthanded play, map the switches.
 *
 * Returns true if the START button is depressed.
 */
Boolean 
GetJoyPad(JoyPadStatePtr joyStatePtr, int32 padNum)
{
	uint32	joyBits;
	
	if ( gPadInitialized == false )
	{
		InitJoypad(kDefaultCtrlPadCount);
	}
		
#ifdef	DEBUG
	if ( (padNum < 1) || (padNum > kDefaultCtrlPadCount) )
	{
		PERR(("ERROR: %d is an invalid pad number. Must be between 1 and %d\n", 
					padNum, kDefaultCtrlPadCount));
		return false;
	}
#endif

	joyBits = GetJoyPadState(padNum);
	
	/*!!!!! is this useful??? 												*/
	/* in the 'left handed' position, the pad is upsidedown so we need 		*/
	/*  to remap the arrows, and the A & C buttons, the start and X buttons */
	/*  and the left/right shift buttons									*/
	/*!!!!! is this useful???												*/
	if ( gLeftHandedPad[padNum - 1] )
	{
		LONGWORD(*joyStatePtr) = 0L;
		joyStatePtr->leftShift	= (joyBits & ControlRightShift)	? 1 : 0;
		joyStatePtr->rightShift	= (joyBits & ControlLeftShift)	? 1 : 0;
	
		joyStatePtr->xBtn		= (joyBits & ControlStart)		? 1 : 0;
		joyStatePtr->startBtn	= (joyBits & ControlX)			? 1 : 0;
		
		joyStatePtr->aBtn		= (joyBits & ControlC)			? 1 : 0;
		joyStatePtr->bBtn		= (joyBits & ControlB)			? 1 : 0;
		joyStatePtr->cBtn		= (joyBits & ControlA)			? 1 : 0;
		joyStatePtr->upArrow	= (joyBits & ControlDown)		? 1 : 0;
		joyStatePtr->downArrow	= (joyBits & ControlUp)			? 1 : 0;
		joyStatePtr->leftArrow	= (joyBits & ControlRight)		? 1 : 0;
		joyStatePtr->rightArrow	= (joyBits & ControlLeft)		? 1 : 0;
	}
	else
	{
		LONGWORD(*joyStatePtr) = joyBits;

#define	LONG_HAND	0
#if	LONG_HAND
		joyStatePtr->leftShift	= (joyBits & ControlLeftShift)	? 1 : 0;
		joyStatePtr->rightShift	= (joyBits & ControlRightShift)	? 1 : 0;
	
		joyStatePtr->xBtn		= (joyBits & ControlX)			? 1 : 0;
		joyStatePtr->startBtn	= (joyBits & ControlStart)		? 1 : 0;
		
		joyStatePtr->aBtn		= (joyBits & ControlA)			? 1 : 0;
		joyStatePtr->bBtn		= (joyBits & ControlB)			? 1 : 0;
		joyStatePtr->cBtn		= (joyBits & ControlC)			? 1 : 0;
		joyStatePtr->upArrow	= (joyBits & ControlUp)			? 1 : 0;
		joyStatePtr->downArrow	= (joyBits & ControlDown)		? 1 : 0;
		joyStatePtr->leftArrow	= (joyBits & ControlLeft)		? 1 : 0;
		joyStatePtr->rightArrow	= (joyBits & ControlRight)		? 1 : 0;
#endif
	}
	
	return ( joyStatePtr->startBtn );
}

