/*
 *
 *
 */
#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#ifdef MACINTOSH
#include <kernel:types.h>
#include <kernel:mem.h>
#include <graphics:graphics.h>
#include <graphics:view.h>
#include <graphics:clt:gstate.h>
#include <misc:event.h>
#else
#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/clt/gstate.h>
#include <misc/event.h>
#endif
#include <stdio.h>
#include <assert.h>
#include "matrix.h"

/*
 *
 */
typedef struct {

	ControlPadEventData	cped;
	uint32				currentButtons;
	uint32				previousButtons;
	uint32				pressButtons;
	uint32				releaseButtons;

} Controller, *pController;

/*
 *
 */
Controller* Controller_Construct(void);
void Controller_Destruct(Controller *controls);

bool Controller_Update(Controller *controls);
void Controller_CollectEvents(Controller *controls);

void Controller_PlayWithMatrix(Controller *controls, Matrix *object, Vector3D *point );

#endif







