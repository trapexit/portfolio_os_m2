/*******************************************************
**
** @(#) beep_internal.h 96/06/19 1.12
**
** Beep folio includes.
**
** Author: Phil Burk
** Copyright 1995 3DO
** All Rights Reserved
*******************************************************/

#include <beep/beep.h>
#include <kernel/types.h>
#include <kernel/debug.h>       /* print_vinfo() */
#include <kernel/tags.h>        /* tag iteration */
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <dspptouch/dspp_addresses.h>


/**********************************************************************/
/**************************** Debug Support  **************************/
/**********************************************************************/

#define	PRT(x)	{ printf x; }
#ifndef BUILD_STRINGS
	#define ERR(x)  /* PRT(x) */
#else
	#define ERR(x)  PRT(x)
#endif

#define ID_FORM MAKE_ID('F','O','R','M')
#define ID_BEEP MAKE_ID('B','E','E','P')
#define ID_INFO MAKE_ID('I','N','F','O')
#define ID_VCDO MAKE_ID('V','C','D','O')
#define ID_CODE MAKE_ID('C','O','D','E')
#define ID_INIT MAKE_ID('I','N','I','T')

/* INFO chunk structure. */
typedef struct BeepMachineInfo
{
	uint32        bminfo_MachineID;
	uint32        bminfo_SiliconVersion;
	uint32        bminfo_NumChannelsAssigned;
} BeepMachineInfo;

typedef struct BeepMachineInit
{
	uint32     bmin_ParamID;
	uint8      bmin_FirstVoice; /* First voice to be set. */
	uint8      bmin_NumVoices;  /* Number of voices to be set. */
	int16      bmin_Value;      /* Set DSPP to this raw integer value. */
} BeepMachineInit;

typedef struct BeepMachine
{
	ItemNode      bm_Node;
	BeepMachineInfo bm_Info;
	uint32        bm_NumCodeWords;
	uint16       *bm_CodeImage;
	uint32        bm_NumVoices;
	uint16       *bm_VoiceDataOffsets;
	BeepMachineInit    *bm_Initializer;
} BeepMachine;

typedef struct BeepFolio
{
	Folio        bf_Folio;
	Item         bf_BeepModule;     /* beep folio's module item */
	BeepMachine *bf_Machine;
	Item         bf_FIRQ;
	Item         bf_TasksToSignal[DSPI_MAX_DMA_CHANNELS];
	int32        bf_Signals[DSPI_MAX_DMA_CHANNELS];
} BeepFolio;

extern  BeepFolio   gBeepBase;
#define BB_FIELD(x) gBeepBase.x
extern int16 gSilence[8];

#define CUR_BEEP_MACHINE (BB_FIELD(bf_Machine))

#define IS_BEEP_OPEN (IsItemOpened(CURRENTTASKITEM, BB_FIELD(bf_BeepModule)) >= 0)
#define CHECK_CHANNEL_RANGE(chan) if(chan >= CUR_BEEP_MACHINE->bm_Info.bminfo_NumChannelsAssigned) return BEEP_ERR_CHANNEL_RANGE;
#define CHECK_VOICE_RANGE(chan) if(chan >= CUR_BEEP_MACHINE->bm_NumVoices) return BEEP_ERR_VOICE_RANGE;
#define CHECK_MACHINE_LOADED    if( CUR_BEEP_MACHINE == NULL ) return BEEP_ERR_NO_MACHINE;
#define CHECK_VALID_RAM(addr,len)   if( !IsMemReadable ( (char *) addr, len ) ) return BEEP_ERR_SECURITY;

Err  swiSetBeepParameter( uint32 ParameterID, float32 Value );
Err  swiSetBeepVoiceParameter( uint32 ChannelNum, uint32 ParameterID, float32 Value );

Err  swiConfigureBeepChannel( uint32 ChannelNum, uint32 Flags );
Err  swiSetBeepChannelData( uint32 ChannelNum, void *Addr, int32 NumSamples );
Err  swiSetBeepChannelDataNext( uint32 ChannelNum, void *Addr, int32 NumSamples , uint32 Flags, int32 Signal);
Err  swiHackBeepChannelDataNext( uint32 *params );
Err  SetBeepChannelDataNext( uint32 ChannelNum, void *Addr, int32 NumSamples, uint32 Flags, int32 Signal );
Err  swiStartBeepChannel( uint32 ChannelNum );
Err  swiStopBeepChannel( uint32 ChannelNum );
Err  swiMonitorBeepChannel( uint32 ChannelNum, int32 Signal );
uint32 swiGetBeepTime( void );

Err lowSetBeepVoiceParameter( uint32 voiceNum, uint32 ParameterID, int16 intValue  );

/* Private SWI */
Err  HackBeepChannelDataNext( uint32 *params );

Err  beepInitDSP( void );
Err  beepTermDSP( void );
Err  beepInitInterrupt( void );
Err  beepTermInterrupt( void );
Item internalCreateBeepMachine ( BeepMachine *bm, TagArg *args);
Err  internalDeleteBeepMachine ( BeepMachine *bm );
