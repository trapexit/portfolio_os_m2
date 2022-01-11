#ifndef __E3MUSIC_RECV_H
#define __E3MUSIC_RECV_H


/****************************************************************************
**
**  @(#) e3music_recv.h 95/05/09 1.6
**
****************************************************************************/
/******************************************
** Remote Sound Communications for E3 flying demo
**
** Author: Phil Burk
** Copyright (c) 1995 3DO 
** All Rights Reserved
******************************************/

#include <kernel/types.h>
#include <audio/audio.h>
#include "e3music.h"

/* Sample selection and sound tuning options. */
#define E3_MUSIC_FILENAME   "$pbsamples/e3song.22.aiff"

#define E3_CANNON_FILENAME  "$pbsamples/machinegun.aiff"

/* #define E3_ENGINE_FILENAME  "$pbsamples/hotrod.aiff" */
#define E3_ENGINE_FILENAME  "$pbsamples/M2EngLoop3Jalopy.44m.aiff"
/* #define E3_ENGINE_FILENAME  "$pbsamples/M2EngLoop2.44m.aiff" */


#define MAX_MUSIC_AMPLITUDE    (0x4000)

#define MAX_ENGINE_AMPLITUDE   (0x4000)
#define MAX_ENGINE_FREQUENCY   (0xF000)

#define MAX_WIND_AMPLITUDE     (0x1000)
#define MAX_WIND_FREQUENCY     (0x1400)

#define CANNON_AMPLITUDE       (0x6000)
#define CANNON_FREQUENCY       (0x8000)


Err e3InitDemoSound( void );
e3HandleMusicCommand ( E3MusicState *e3ms );

/*****************************************************************************/


#endif /* __E3MUSIC_RECV_H */
