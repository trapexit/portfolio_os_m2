/*
 * @(#) controller.h 96/05/16 1.4
 *
 * Copyright (c) 1996, The 3DO Company.  All rights reserved.
 */
#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#ifdef MACINTOSH
#include <kernel:types.h>
#include <kernel:mem.h>
#include <misc:event.h>
#else
#include <kernel/types.h>
#include <kernel/mem.h>
#include <misc/event.h>
#endif
#include <stdio.h>
#include <assert.h>

/*
 *
 */
typedef struct {

	ControlPadEventData	cped;
	int32				err;
	uint32				currentButtons;
	uint32				previousButtons;
	uint32				pressButtons;
	uint32				releaseButtons;

} CONTROLLER;

/*
 *
 */
void Controller_Construct(CONTROLLER *controls);
void Controller_Update(CONTROLLER *controls);
void Controller_CollectEvents(CONTROLLER *controls);
#if 0
void Controller_UpdateCharacter( CONTROLLER *controls, Character *character, Point3 *point );
#endif

#endif
