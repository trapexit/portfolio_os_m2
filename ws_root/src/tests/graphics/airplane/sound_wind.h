#ifndef _SOUND_WIND_H
#define _SOUND_WIND_H

/* @(#) sound_wind.h 96/04/15 1.2 */
/******************************************
** Wind sound based on filtered noise.
** Does not use samples.
**
** Author: Phil Burk
** Copyright (c) 1995 3DO 
** All Rights Reserved
**
*******************************************/

#include <kernel/types.h>
#include <kernel/kernel.h>

Err LoadWindSound( void );
Err ControlWindSound( float32 fractionalVelocity );
Err UnloadWindSound( void );

#endif /* _SOUND_WIND_H */
