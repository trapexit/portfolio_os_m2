#ifndef __BEEP_BEEP_H
#define __BEEP_BEEP_H


/****************************************************************************
**
**  @(#) beep.h 96/05/21 1.9
**
**  Beep Folio Includes
**
****************************************************************************/

#ifndef EXTERNAL_RELEASE
/* History
** 960214 PLB Creation
*/
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

#ifndef __KERNEL_TAGS_H
#include <kernel/tags.h>
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __AUDIO_AUDIO_SIGNALS_H
#include <audio/audio_signals.h>
#endif


/**********************************************************************/
/**************************** Constants  ******************************/
/**********************************************************************/

/* This unique number matches number assigned in kernel/nodes.h. */
#define BEEPNODE   (NST_BEEP)
#define BEEPFOLIONAME "beep"

/* Make flags unequal so we don't accidentally pass the wrong one. */
#define BEEP_F_CHAN_CONFIG_SQS2     (1)
#define BEEP_F_CHAN_CONFIG_8BIT     (2)
#define BEEP_F_IF_GO_FOREVER        (4)

#define BEEP_ID_KEY_MASK              (0xF)
#define BEEP_ID_KEY_SHIFT             (28)
#define BEEP_ID_VALID_KEY             (0xB)
#define BEEP_ID_SIGTYPE_MASK          (0xF)
#define BEEP_ID_SIGTYPE_SHIFT         (24)
#define BEEP_ID_CALCRATE_MASK         (0x3)
#define BEEP_ID_CALCRATE_SHIFT        (22)
#define BEEP_ID_MACHINE_MASK          (0x3F)
#define BEEP_ID_MACHINE_SHIFT         (16)
#define BEEP_ID_INDEX_MASK            (0xFF)
#define BEEP_ID_INDEX_SHIFT           (0)

/*
** Item type numbers for Audio Folio
*/
#define BEEP_MACHINE_NODE    (1)

enum beep_folio_tags
{
	BEEP_TAG_MACHINE = TAG_ITEM_LAST+1
};

/**********************************************************************/
/************************** Error Returns *****************************/
/**********************************************************************/

#define MAKE_BEEP_ERR(svr,class,err) MakeErr(ER_FOLI,ER_BEEP,svr,ER_E_SSTM,class,err)

/* Standard errors returned from audiofolio */
#define BEEP_ERR_BADITEM          MAKE_BEEP_ERR(ER_SEVERE,ER_C_STND,ER_BadItem)
#define BEEP_ERR_BADPRIV          MAKE_BEEP_ERR(ER_SEVERE,ER_C_STND,ER_NotPrivileged)
#define BEEP_ERR_BADPTR           MAKE_BEEP_ERR(ER_SEVERE,ER_C_STND,ER_BadPtr)
#define BEEP_ERR_BADTAG           MAKE_BEEP_ERR(ER_SEVERE,ER_C_STND,ER_BadTagArg)
#define BEEP_ERR_BADTAGVAL        MAKE_BEEP_ERR(ER_SEVERE,ER_C_STND,ER_BadTagArgVal)
#define BEEP_ERR_NOMEM            MAKE_BEEP_ERR(ER_SEVERE,ER_C_STND,ER_NoMem)
#define BEEP_ERR_NOSIGNAL         MAKE_BEEP_ERR(ER_SEVERE,ER_C_STND,ER_NoSignals)
#define BEEP_ERR_NOTFOUND         MAKE_BEEP_ERR(ER_SEVERE,ER_C_STND,ER_NotFound)
#define BEEP_ERR_NOTOWNER         MAKE_BEEP_ERR(ER_SEVERE,ER_C_STND,ER_NotOwner)
#define BEEP_ERR_UNIMPLEMENTED    MAKE_BEEP_ERR(ER_SEVERE,ER_C_STND,ER_NotSupported)

#ifndef EXTERNAL_RELEASE
/* !!! some of these probably don't need to be known publicly */
#endif
/* Beep specific errors. */
#define BEEP_ERR_BASE (0)

/* Illegal security violation. */
#define BEEP_ERR_SECURITY              MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+1)

/* Invalid signal type. */
#define BEEP_ERR_BAD_SIGNAL_TYPE       MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+2)

/* Invalid Parameter. */
#define BEEP_ERR_INVALID_PARAM         MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+3)

/* Invalid ChannelConfiguration. */
#define BEEP_ERR_INVALID_CONFIGURATION MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+4)

/* No machine loaded. */
#define BEEP_ERR_NO_MACHINE            MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+5)

/* Task has not opened Beep folio. */
#define BEEP_ERR_FOLIO_NOT_OPEN        MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+6)

/* ChannelNum out of range. */
#define BEEP_ERR_CHANNEL_RANGE         MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+7)

/* VoiceNum out of range. */
#define BEEP_ERR_VOICE_RANGE           MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+8)

/* Can't open file. */
#define BEEP_ERR_OPEN_FILE             MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+9)

/* Badly formatted file. */
#define BEEP_ERR_BAD_FILE              MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+10)

/* File could not be read. */
#define BEEP_ERR_READ_FAILED           MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+11)

/* A beep machine is already loaded. One at atime only. */
#define BEEP_ERR_ALREADY_LOADED        MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+12)

/* A flag parameter is illegal for this routine. */
#define BEEP_ERR_ILLEGAL_FLAG          MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+13)

/* Name passed to Beep folio is too long. */
#define BEEP_ERR_NAME_TOO_LONG         MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+14)

/* Illegal Beep Machine file information. */
#define BEEP_ERR_ILLEGAL_MACHINE       MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+15)

/* DSP is already in use. */
#define BEEP_ERR_DSP_BUSY              MAKE_BEEP_ERR(ER_SEVERE,ER_C_NSTND,BEEP_ERR_BASE+16)

/**********************************************************************/
/************************** Macros and Functions **********************/
/**********************************************************************/

#define UnloadBeepMachine(bm) DeleteItem(bm)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
Err OpenBeepFolio( void );
Err CloseBeepFolio( void );

Err ConfigureBeepChannel( uint32 ChannelNum, uint32 Flags );
Err SetBeepParameter( uint32 ParameterID, float32 Value );
Err SetBeepVoiceParameter( uint32 ChannelNum, uint32 ParameterID, float32 Value );
Err SetBeepChannelData( uint32 ChannelNum, const void *Addr, int32 NumSamples );
Err SetBeepChannelDataNext( uint32 ChannelNum, const void *Addr, int32 NumSamples,
	uint32 Flags, int32 Signal );
Err StartBeepChannel( uint32 ChannelNum );
Err StopBeepChannel( uint32 ChannelNum );

Item LoadBeepMachine( const char *MachineName );
Err  UnloadBeepMachine( Item BeepMachine );

uint32 GetBeepTime( void );

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __BEEP_BEEP_H */
