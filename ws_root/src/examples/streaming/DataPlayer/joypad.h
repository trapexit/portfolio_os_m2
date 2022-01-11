/****************************************************************************
**
**  @(#) joypad.h 96/02/29 1.1
**
*****************************************************************************/
#ifndef __JPIJOYPAD__
#define __JPIJOYPAD__

#include <kernel/types.h>
#include <misc/event.h>


/* as written, we allow 4 control pads */
#define	kDefaultCtrlPadCount	4

#define LONGWORD(aStruct)		( *((long*) &(aStruct)) )

/* a couple of handy defines for setting pad continuous flag */
#define		PADARROWS	(ControlDown | ControlUp | ControlRight | ControlLeft)
#define		PADBUTTONS	(ControlA | ControlB | ControlC | ControlStart | ControlX)
#define		PADSHIFT	(ControlRightShift | ControlLeftShift)
#define		PADALL		(PADARROWS | PADBUTTONS | PADSHIFT)

/* bitfield structure for the control pad buttons */
typedef struct 
{
	unsigned	int downArrow		: 1;	/* [  31] */
	unsigned	int upArrow			: 1;	/* [  30] */
	unsigned	int rightArrow		: 1;	/* [  29] */
	unsigned	int leftArrow		: 1;	/* [  28] */
	unsigned	int aBtn			: 1;	/* [  27] */
	unsigned	int bBtn			: 1;	/* [  26] */
	unsigned	int cBtn			: 1;	/* [  25] */
	unsigned	int	startBtn		: 1;	/* [  24] */
	unsigned	int	xBtn			: 1;	/* [  23] */
	unsigned	int	rightShift		: 1;	/* [  22] */
	unsigned	int	leftShift		: 1;	/* [  21] */
	unsigned	int					: 21;	/* [0-20] filler, leave set to 0 */
} JoyPadState;
typedef JoyPadState *JoyPadStatePtr;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern	int32	KillJoypad( void );
extern	int32	GetJoyPadContinuous(int32 padNum);
extern	int32	SetJoyPadContinuous(int32 continuousBtnFlags, int32 padNum);
extern	void	SetJoyPadLeftHanded(Boolean playLeftHanded, int32 padNum);
extern	Boolean	GetJoyPad(JoyPadState* joyState, int32 padNum);

extern	Boolean gPadInitialized;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
