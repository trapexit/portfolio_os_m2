/*
 *
 *
 */
#include "controller.h"

/*
 *
 */
Controller* Controller_Construct(void)
{
	Controller	*controls;

    controls = AllocMem(sizeof(Controller), MEMTYPE_NORMAL);
	assert( controls );

	controls->currentButtons = 0;
    controls->	previousButtons = 0;
	controls->pressButtons = 0;
	controls->releaseButtons = 0;

	/*
	 * This is should be called one time
	 */
	/*    InitEventUtility( 1, 0, LC_ISFOCUSED ); */

	return controls;
}

/*
 *
 */
void Controller_Destruct(Controller *controls)
{
	assert( controls );
    FreeMem( controls, sizeof(Controller) );
}

/*
 *
 */
bool Controller_Update(Controller *controls) {

	return GetControlPad (1, FALSE, &controls->cped);

}

/*
 *
 */
void Controller_CollectEvents(Controller *controls) {

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

/*
 *
 */
void Controller_PlayWithMatrix(Controller *controls, Matrix *object, Vector3D *point )
{

	Vector3D		angle;
	Vector3D		pos;

	uint32			buttons = controls->currentButtons;

#define	kPadAngle	(2.0f * DEG_TO_RAD)
#define	kPadPos		(2.0f)

	/*
	 *
	 */
	Vector3D_Zero(&angle);
	Vector3D_Zero(&pos);

	/*
	 * Simple control logic
	 */
	if ( buttons & ControlA ) {
		if ( buttons & ControlUp )
			pos.z = -kPadPos;
		if ( buttons & ControlDown )
			pos.z = kPadPos;
		if ( buttons & ControlLeft )
			pos.x = -kPadPos;
		if ( buttons & ControlRight )
			pos.x = kPadPos;

	} else if ( controls->currentButtons & ControlB ) {
		if ( buttons & ControlUp )
			angle.x = -kPadAngle;
		if ( buttons & ControlDown )
			angle.x = kPadAngle;
		if ( buttons & ControlLeft )
			angle.y = kPadAngle;
		if ( buttons & ControlRight )
			angle.y = -kPadAngle;

	} else if ( controls->currentButtons & ControlC ) {
		if ( buttons & ControlUp )
			angle.x = -kPadAngle;
		if ( buttons & ControlDown )
			angle.x = kPadAngle;
		if ( buttons & ControlLeft )
			angle.y = kPadAngle;
		if ( buttons & ControlRight )
			angle.y = -kPadAngle;

	} else {
		if ( buttons & ControlUp )
			pos.y = kPadPos;
		if ( buttons & ControlDown )
			pos.y = -kPadPos;
		if ( buttons & ControlLeft )
			pos.x = -kPadPos;
		if ( buttons & ControlRight )
			pos.x = kPadPos;
	}

	if (controls->currentButtons & ControlLeftShift) {
		angle.z = -kPadAngle;
	}
	if (controls->currentButtons & ControlRightShift) {
		angle.z = kPadAngle;
	}

	if ( controls->currentButtons & ControlC ) {

		Vector3D_Negate( point );
		Matrix_MoveByVector( object, point );
		Matrix_TurnYLocal( object, angle.y );
		Matrix_TurnXLocal( object, angle.x );
		Matrix_TurnZLocal( object, angle.z );
		Vector3D_Negate( point );
		Matrix_MoveByVector( object, point );

	} else {

		Matrix_TurnYLocal( object, angle.y );
		Matrix_TurnXLocal( object, angle.x );
		Matrix_TurnZLocal( object, angle.z );
		Matrix_MoveByVector( object, &pos );
	}
}

/*  End of File */
