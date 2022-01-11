#ifndef __EZ3DSOUND_H
#define __EZ3DSOUND_H


/****************************************************************************
**
**  @(#) ez3dsound.h 96/07/31 1.1
**
**  EZ3D Sound
**
****************************************************************************/


#ifndef __AUDIO_AUDIO_H
#include <audio/audio.h>    /* MixerSpec */
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __AUDIO_SOUND3D_H
#include <audio/sound3d.h>  /* default flags */
#endif

typedef struct EZ3DSoundParent
{
	Node      ezsp_Node;        /* contains name and links */
	List      ezsp_Sounds;      /* sounds created from parent */
	Item      ezsp_Sample;      /* sample used in sound, or zero. */
	Item      ezsp_InsTemplate; /* instrument template used to create. */
} EZ3DSoundParent;

typedef struct EZ3DSound
{
	MinNode          ezso_Node;
	EZ3DSoundParent *ezso_Parent;        /* points to parentof multiple sounds */
	uint32           ezso_MixerChannel;  /* channel index assigned at CreateSE */
	Item             ezso_Instrument;    /* instrument used to create sound */
	Sound3D         *ezso_Sound3D;       /* 3D processor */
	Item             ezso_FreqKnob;      /* for doppler shifting */
	float32          ezso_NominalFrequency; /* the frequency when the sound's at rest */
	float32          ezso_Gain;          /* relative amplitude of this sound */
	PolarPosition4D  ezso_LastTarget;    /* last specified position */
} EZ3DSound;

#define EZ3D_DEFAULT_FLAGS (S3D_F_PAN_DELAY | \
                            S3D_F_PAN_FILTER | \
                            S3D_F_DISTANCE_AMPLITUDE_SQUARE | \
                            S3D_F_OUTPUT_HEADPHONES | \
                            S3D_F_DOPPLER)

#define MAKE_EZ3DSND_ERR(svr,class,err) MakeErr(ER_USER,MakeErrId('E','3'),svr,ER_E_USER,class,err)
#define EZ3DSND_ERR_UNIMPLEMENTED  MAKE_EZ3DSND_ERR(ER_SEVERE,ER_C_STND,ER_NotSupported)
#define EZ3DSND_ERR_NOMEM          MAKE_EZ3DSND_ERR(ER_SEVERE,ER_C_STND,ER_NoMem)
#define EZ3DSND_ERR_NOSOUNDS       MAKE_EZ3DSND_ERR(ER_SEVERE,ER_C_NSTND,1)
#define EZ3DSND_ERR_NUM_FX_INPUTS  MAKE_EZ3DSND_ERR(ER_SEVERE,ER_C_NSTND,2)
#define EZ3DSND_ERR_NUM_FX_OUTPUTS MAKE_EZ3DSND_ERR(ER_SEVERE,ER_C_NSTND,3)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Err  CreateEZ3DSoundEnvironment( int32 maxSounds, float32 gain, float32 fxgain,
	const char *effectsPatch, const TagArg * );
void DeleteEZ3DSoundEnvironment( void );
Err  LoadEZ3DSound( EZ3DSound **ezsoPtr, const char *name, const TagArg *tags );
void UnloadEZ3DSound( EZ3DSound *ezso );
Err  StartEZ3DSound( EZ3DSound *ezso, float32 x, float32 y, float32 z, const TagArg *tags );
Err  SetEZ3DSoundLoudness( EZ3DSound *ezso, float32 loudness );
Err  MoveEZ3DSound( EZ3DSound *ezso, float32 x, float32 y, float32 z );
Err  StopEZ3DSound( EZ3DSound *ezso, const TagArg *tags );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define GetEZSoundInstrument(ezso) (ezso->ezso_Instrument)
#define CreateEZSoundKnob(ezso,name,tags) CreateKnob(GetEZSoundInstrument(ezso),name,tags)

#define ReleaseEZ3DSound( ezso, tags ) ReleaseInstrument(GetEZSoundInstrument(ezso),tags)


#endif
