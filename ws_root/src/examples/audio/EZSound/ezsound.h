#ifndef __EZSOUND_H
#define __EZSOUND_H


/****************************************************************************
**
**  @(#) ezsound.h 96/08/07 1.7
**
**  EZ Sound
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

typedef struct EZSoundParent
{
	Node      ezsp_Node;        /* contains name and links */
	List      ezsp_Sounds;      /* sounds created from parent */
	Item      ezsp_Sample;      /* sample used in sound, or zero. */
	Item      ezsp_InsTemplate; /* instrument template used to create. */
} EZSoundParent;

typedef struct EZSound
{
	MinNode   ezso_Node;
	EZSoundParent  *ezso_Parent;  /* points to parentof multiple sounds */
	uint32    ezso_MixerChannel;  /* channel index assigned at CreateSE */
	Item      ezso_Instrument;    /* instrument used to create sound */
} EZSound;

#define MAKE_EZSND_ERR(svr,class,err) MakeErr(ER_USER,MakeErrId('E','Z'),svr,ER_E_USER,class,err)
#define EZSND_ERR_UNIMPLEMENTED  MAKE_EZSND_ERR(ER_SEVERE,ER_C_STND,ER_NotSupported)
#define EZSND_ERR_NOMEM          MAKE_EZSND_ERR(ER_SEVERE,ER_C_STND,ER_NoMem)
#define EZSND_ERR_NOSOUNDS       MAKE_EZSND_ERR(ER_SEVERE,ER_C_NSTND,1)
#define EZSND_ERR_NUM_FX_INPUTS  MAKE_EZSND_ERR(ER_SEVERE,ER_C_NSTND,2)
#define EZSND_ERR_NUM_FX_OUTPUTS MAKE_EZSND_ERR(ER_SEVERE,ER_C_NSTND,3)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Err  CreateEZSoundEnvironment( int32 maxSounds, float32 gain,
	const char *effectsPatch, const TagArg * );
void DeleteEZSoundEnvironment( void );
Err  LoadEZSound( EZSound **ezsoPtr, const char *name, const TagArg *tags );
Err  SetEZSoundMix( EZSound *ezso, float32 amplitude, float32 pan, float32 fxsend );
void UnloadEZSound( EZSound *ezso );
Err  StartEZSound( EZSound *ezso, const TagArg *tags );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define GetEZSoundInstrument(ezso) (ezso->ezso_Instrument)
#define CreateEZSoundKnob(ezso,name,tags) CreateKnob(GetEZSoundInstrument(ezso),name,tags)

#define ReleaseEZSound( ezso, tags ) ReleaseInstrument(GetEZSoundInstrument(ezso),tags)
#define StopEZSound( ezso, tags ) StopInstrument(GetEZSoundInstrument(ezso),tags)

#endif
