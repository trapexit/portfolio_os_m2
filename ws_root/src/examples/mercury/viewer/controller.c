/*
 * @(#) controller.c 96/05/13 1.3
 *
 * Copyright (c) 1996, The 3DO Company.  All rights reserved
 */

#define DEBUG
#undef  TRACE

#include "controller.h"

/*
 *
 */
void Controller_Construct(CONTROLLER *controls) {

    controls->err = InitEventUtility( 1, 0, LC_ISFOCUSED );  
	controls->err = 0;

}

/*
 *
 */
void Controller_Update(CONTROLLER *controls) {

	controls->err = GetControlPad (1, FALSE, &controls->cped);

}

/*
 *
 */
void Controller_CollectEvents(CONTROLLER *controls) {

	uint32		buttonsDelta;

	controls->previousButtons = controls->currentButtons;

	Controller_Update(controls);
	
	controls->currentButtons = controls->cped.cped_ButtonBits;

	buttonsDelta = (controls->previousButtons ^ controls->currentButtons);
	controls->pressButtons = (buttonsDelta & controls->currentButtons);
	controls->releaseButtons = (buttonsDelta & controls->previousButtons);

	#if 0
	printf(" Collected controls: (Current %x) (Press %x) (Release %x)\n", 
			controls->currentButtons, controls->pressButtons, controls->releaseButtons);
	#endif

}

#if 0
/*
 *
 */
void Controller_UpdateCharacter( CONTROLLER *controls, AppData* appData,
                                 Matrix* matrix, Point3 *point )
{

	Vector3			angle;		/* rotation angles */
	Point3			pos;		/* position */
	
	uint32			buttons = controls->currentButtons;

#define	PadAngle	(2.0f)
#define	PadPos		(0.75f)

	/*
	 *
	 */
	Vec3_Set(&angle, 0, 0, 0);
	Pt3_Set(&pos, 0, 0, 0);

	/*
	 * Simple control logic
	 */
	if ( buttons & ControlA ) {
		if ( buttons & ControlUp )
			pos.z = -PadPos;
		if ( buttons & ControlDown )
			pos.z = PadPos;
		if ( buttons & ControlLeft )
			pos.x = -PadPos;
		if ( buttons & ControlRight )
			pos.x = PadPos;
			
	} else if ( controls->currentButtons & ControlB ) {
		if ( buttons & ControlUp )
			angle.x = -PadAngle;
		if ( buttons & ControlDown )
			angle.x = PadAngle;
		if ( buttons & ControlLeft )
			angle.y = PadAngle;
		if ( buttons & ControlRight )
			angle.y = -PadAngle;
			
	} else if ( controls->currentButtons & ControlC ) {
		if ( buttons & ControlUp )
			angle.x = -PadAngle;
		if ( buttons & ControlDown )
			angle.x = PadAngle;
		if ( buttons & ControlLeft )
			angle.y = PadAngle;
		if ( buttons & ControlRight )
			angle.y = -PadAngle;
			
	} else {
		if ( buttons & ControlUp )
			pos.y = PadPos;
		if ( buttons & ControlDown )
			pos.y = -PadPos;
		if ( buttons & ControlLeft )
			pos.x = -PadPos;
		if ( buttons & ControlRight )
			pos.x = PadPos;
	}
	
	if (controls->currentButtons & ControlLeftShift) {
		angle.z = -PadAngle;
	}
	if (controls->currentButtons & ControlRightShift) {
		angle.z = PadAngle;
	}
	
	if ( controls->currentButtons & ControlC ) {
		Char_Move( character, -point->x, -point->y, -point->z );
		Char_Yaw( character, angle.y );
		Char_Pitch( character, angle.x );
		Char_Roll( character, angle.z );
		Char_Move( character, point->x, point->y, point->z );
	} else {
		Char_Move( character, pos.x, pos.y, pos.z );
		Char_Yaw( character, angle.y );
		Char_Pitch( character, angle.x );
		Char_Roll( character, angle.z );
	}

}
#endif

/* End of File */
