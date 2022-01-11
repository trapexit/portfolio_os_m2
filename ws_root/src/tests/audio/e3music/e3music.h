#ifndef __E3MUSIC_H
#define __E3MUSIC_H


/****************************************************************************
**
**  @(#) e3music.h 95/05/09 1.5
**
****************************************************************************/
/******************************************
** Sound Manager for E3 flying demo
**
** Author: Phil Burk
** Copyright (c) 1995 3DO 
** All Rights Reserved
******************************************/

#include <kernel/types.h>
#include <audio/audio.h>
#include "sfx_tools.h"

#define PRT(x)   { printf x; }
#define ERR(x)   PRT(x)

/* Init and Term */
Err e3InitMusic( void );
Err e3TermMusic( void );

/* Start music which will play continuously. */
Err e3LoadMusic( char *MusicName );
Err e3StartMusic( int32 Amplitude );
Err e3SetMusicAmplitude( int32 Amplitude );
Err e3StopMusic( void );

/* Control sound of ultralight engine. */

Err e3LoadEngineSound( char *SampleName );
Err e3StartEngineSound( int32 Amplitude, int32 Frequency );
Err e3ControlEngineSound( int32 Amplitude, int32 Frequency );
Err e3StopEngineSound( void );

/* Control sound of Wind. */
Err e3StartWindSound( int32 Amplitude, int32 Frequency );
Err e3ControlWindSound( int32 Amplitude, int32 Frequency );
Err e3StopWindSound( void );

/* Trigger one shot sound. */
Err e3LoadCannonSound( char *SampleName );
Err e3StartCannonSound( int32 Amplitude, int32 Frequency );
Err e3ControlCannonSound( int32 Amplitude, int32 Frequency );
Err e3StopCannonSound( void );

/*****************************************************************************/


#endif /* __E3MUSIC_H */
