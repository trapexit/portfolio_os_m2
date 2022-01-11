/*
 *		Audio Replay - music.h - Replay music stream (.STREAM) with triggers
 *
 *
 *			PROGRAM HISTORY:
 *
 *		001		n/s		.h file...
 *
 */

/******************************************************************************/

#ifdef MACINTOSH
#include <misc:event.h>										/* This is used for handling the controller */
#include <kernel:mem.h>										/* for FreeMemTrack */
#include <kernel:types.h>
#include <kernel:debug.h>									/* for print macro: CHECK_NEG */
#include <kernel:msgport.h>									/* GetMsg */
#include <audio:audio.h>									/* Sound is what this is all about */
#include <audio:parse_aiff.h>								/* This is needed for LoadSample */
#else
#include <misc/event.h>										/* This is used for handling the controller */
#include <kernel/mem.h>										/* for FreeMemTrack */
#include <kernel/types.h>
#include <kernel/debug.h>									/* for print macro/ CHECK_NEG */
#include <kernel/msgport.h>									/* GetMsg */
#include <audio/audio.h>									/* Sound is what this is all about */
#include <audio/parse_aiff.h>								/* This is needed for LoadSample */
#endif

#include <stdio.h>											/* Every good 'C' program needs this... */
#include <stdlib.h>											/* for exit() */
#include <string.h>


/*
 *
 */
#define PRIO_SFX			(50)			/* Priority of the Sound Effects */

/*
 *
 */
#define kSFX_Hit				(0)
#define kSFX_Block				(1)
#define kSFX_WeaponWeaponHit	(2)
#define kSFX_Forcefield			(3)
#define kSFX_Blood			(4)

#define kSFX_Count		(5)

#define kSFX_Hit_Filename  			"hit.aif"
#define kSFX_Block_Filename			"block.aif"
#define kSFX_WeaponWeaponHit_Filename "weapon.aif"
#define kSFX_Forcefield_Filename	"force.aif"
#define kSFX_Blood_Filename	"blood.aif"

/*
 * PROTOTYPES
 */
int32 InitSoundFX(void);											/* Initialize the sound effects routines */
int32 Position3DSound(int32 channel, int32 x, int32 y, int32 z);	/* Positions the 3D sound */
void PlaySoundFX(int32 noisename, int32 x, int32 y, int32 z);		/* Play sound effect with pseudo-3D sound */
void LoopSoundFX(int32 noisename);									/* Play looping sound effect with pseudo-3D sound */

/*
 * End of File
 */
