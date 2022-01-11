/* @(#) dspp_touch_bulldog.c 96/12/05 1.55 */
/* $Id: dspp_touch_bulldog.c,v 1.14 1995/03/16 18:48:15 peabody Exp phil $ */
/*******************************************************************
**
** Interface to DSPP for Bulldog
**
** by Phil Burk
**
** Copyright (c) 1994 3DO
**
*******************************************************************/

/*
** 940901 PLB Removed AUDOUT setup code.  Now done in ROM.
**            Split from dspp_touch.c.
** 950223 WJB Changed DSPX_CODE_MEMORY_SIZE to DSPI_CODE_MEMORY_SIZE.
** 950419 WJB Surrounded dsphTraceSimulator() w/ #ifdef SIMULATE_DSPP.
** 950420 WJB Added warning messages for as yet unimplemented dsphGetAudioFrameCount() and dsphGetAudioCyclesUsed().
** 950424 WJB Implemented dsphGetAudioFrameCount() and dsphGetAudioCyclesUsed().
** 950426 WJB Moved TicksUsed to m2_head.dsp.
** 950427 WJB Connected dsphGetAudioCyclesUsed() for opera compatibility mode to DSPGetTotalRsrcUsed(DRSC_TICKS).
** 950509 WJB Made dsphRead/WriteDataMem() be just macros unless PARANOID is defined.
** 950510 WJB Made dsphTraceSimulator() a macro.
** 950719 PLB Turn off snooping.
** 950816 PLB Don't read DMA Address in dsphDisableDMA() cuz it tickles
**            a hardware bug in Pass 1 BDA and is useless anyway.
** 960617 PLB Clear entire FIFO by filling with zero.
** 961030 PLB Leave ADIO clocks running after folio terminates so that Asahi Codecs
**            do not get confused and make horrible noises.  Fixes CR11215,6,7 (maybe).
** 961204 PLB Align ZeroData to full clear FIFO using DMA. Fixes TRAC#11128.
*/

#include <dspptouch/dspp_instructions.h>
#include <dspptouch/dspp_touch.h>
#include <dspptouch/touch_hardware.h>

#include <dipir/hw.audio.h>         /* HWResource_Audio */
#include <kernel/sysinfo.h>         /* SYSINFO_TAG_AUDIN */
#include <kernel/super.h>           /* SuperSetSysInfo() */
#include <string.h>                 /* strncmp() */

#include "dspptouch_internal.h"


/* -------------------- Debugging */

#define DEBUG_Config        0       /* turn on debugging in dsphConfiguerADIO() */

#define DBUG(x) /* PRT(x) */
#define DBUGDMA(x) DBUG(x)
#define NDBUG(x) /* */

#define CHECK_AUDIO_CLOCK   (0)


/* -------------------- Local Data */

static int16 gZeroData[16];  /* Used as source for DMA. */
static int16 *gZeroDataAligned;


/* -------------------- Local functions */

static void dsphInitDMA( void );
static void dsphInitInterruptRegs( void );
static void dsphClearAudOut( void );


/* -------------------- Init/Term */

static void dsphConfigureADIO( void );

/********************************************************************
**
**  Initialise DSPP
**
**  Return: version number of BDA for matching with client
**
********************************************************************/
int32	dsphInitDSPP( void )
{
	int32 i;
#ifdef SIMULATE_DSPP
	int32 SaveTrace;
#endif

/* Align ZeroData to 8 byte boundary so that DMA can fill FIFO
** and clear it completely.  TRAC#11128
*/
	{
		uint32 addr;
		addr = (uint32) gZeroData;
		addr = (addr + 7) & ~7;  /* Align to 8 byte boundary. */
		gZeroDataAligned = (int16 *) addr;
	}
/*
** Halt BEFORE Reset because otherwise processor may continue
** running after Reset and then get halted mid-instruction.
** This could leave the DSP in a confused state which could cause
** the DSP to not run properly after a DIPIR restart.
*/
	dsphHalt();
	dsphReset();

	dsphConfigureADIO( );
	dsphInitDMA();
	dsphInitInterruptRegs();

#ifdef SIMULATE_DSPP
	SaveTrace = dsphReadDataMem( DSPI_SIM_TRACE );
	dsphWriteDataMem( DSPI_SIM_TRACE, 0 );
#endif  /* SIMULATE_DSPP */

/* Initialize Data memory for consistent boot behavior. */
	for (i=0; i<DSPI_DATA_MEMORY_SIZE; i++)
	{
		dsphWriteDataMem( i, 0 );
	}

/* Initialize Code memory for consistent boot behavior. */
	for (i=0; i<DSPI_CODE_MEMORY_SIZE; i++)
	{
		dsphWriteCodeMem( i, DSPN_OPCODE_SLEEP );
	}

#ifdef SIMULATE_DSPP
	dsphWriteDataMem( DSPI_SIM_TRACE, SaveTrace );
#endif  /* SIMULATE_DSPP */

/* Setup reasonable Timer values to clear interrupt. */
	WriteHardware( DSPX_FRAME_DOWN_COUNTER, 0xFFFF );
	WriteHardware( DSPX_FRAME_UP_COUNTER, 0 );

	dsphWriteDataMem( DSPI_CLOCK, 0 );

	return(2);
}

/*******************************************************************/
int32	dsphTermDSPP( void )
{

/* Mute output to prevent noise after this point. */
	SuperSetSysInfo( SYSINFO_TAG_AUDIOMUTE, (void *) SYSINFO_AUDIOMUTE_QUIET, 0 );
/* Stop execution of processor. */
	dsphHalt();
/* Just reset DSP. Leave ADIO running. */
	WriteHardware(DSPX_RESET,DSPX_F_RESET_DSPP);
/* Clear DAC FIFO so that we do not get 44100/8 Hz noise. */
	dsphClearAudOut();
/* Disable DMA and interrupts. */
	dsphInitDMA();
	dsphInitInterruptRegs();

	return(0);
}


/********************************************************************
** Audio IO Setup DAC/ADC
********************************************************************/
static uint32 gAudoutConfigImage;
static uint32 gAudinConfigImage;
static uint32 gAudioConfigImage;

/*****************************************************************************/
static void dsphSetAudioConfigImages( void )
{

	uint32  onboardAudinConfig = 0;
	uint32  pcmciaAudinConfig = 0;
	uint32  ifUsePCMCIA = FALSE;

/* Ask ROM to tell us how to configure CODEC */
	HWResource         HWR;

/* Start at beginning. */
	HWR.hwr_InsertID = 0;
	while( (NextHWResource( &HWR )) >= 0)
	{
#if 0
		PRT(("Name = %s\n", HWR.hwr_Name));
		PRT(("DeviceSpecific[0] = 0x%x\n", HWR.hwr_DeviceSpecific[0]));
		PRT(("DeviceSpecific[1] = 0x%x\n", HWR.hwr_DeviceSpecific[1]));
		PRT(("DeviceSpecific[2] = 0x%x\n", HWR.hwr_DeviceSpecific[2]));
		PRT(("Channel = 0x%x\n", HWR.hwr_Channel));
		PRT(("Slot = 0x%x\n", HWR.hwr_Slot));
		PRT(("InsertID = 0x%x\n", HWR.hwr_InsertID));
#endif
/* FIXME - why doesn't this work? if(MatchDeviceName("AUDIO", HWR.hwr_Name, 1)) */
		if( strncmp("AUDIO",HWR.hwr_Name,5) == 0 )
		{
			HWResource_Audio  *hwaudio = (HWResource_Audio *) &HWR.hwr_DeviceSpecific[0];
			DBUG(("audio_SharedConfig = 0x%x\n", hwaudio->audio_SharedConfig ));
			DBUG(("audio_InputConfig = 0x%x\n", hwaudio->audio_InputConfig ));
			DBUG(("audio_OutputConfig = 0x%x\n", hwaudio->audio_OutputConfig ));

/* Use PCMCIA ADC if available. Use last one found. */
			if( HWR.hwr_Channel == CHANNEL_ONBOARD )
			{
				gAudioConfigImage = hwaudio->audio_SharedConfig;
				gAudoutConfigImage = hwaudio->audio_OutputConfig;

				onboardAudinConfig = hwaudio->audio_InputConfig;
			}
			else if( HWR.hwr_Channel == CHANNEL_PCMCIA )
			{
				pcmciaAudinConfig = hwaudio->audio_InputConfig;
				ifUsePCMCIA = TRUE;
			}
			break;
		}
	}

	if( ifUsePCMCIA )
	{
		int slotCode = 0;
		switch(HWR.hwr_Slot)
		{
			case 5: slotCode = SYSINFO_AUDIN_PCCARD_5; break;
			case 6: slotCode = SYSINFO_AUDIN_PCCARD_6; break;
			case 7: slotCode = SYSINFO_AUDIN_PCCARD_7; break;
			default:
				ERR(("dsphSetAudioConfigImages got bad slot = %d\n", HWR.hwr_Slot));
				break;
		}
		SuperSetSysInfo( SYSINFO_TAG_AUDIN, (void *) slotCode, 0 );
		gAudinConfigImage = pcmciaAudinConfig;
	}
	else
	{
		SuperSetSysInfo( SYSINFO_TAG_AUDIN, (void *) SYSINFO_AUDIN_MOTHERBOARD, 0 );
		gAudinConfigImage = onboardAudinConfig;
	}
}

/*****************************************************************************/
static void dsphConfigureADIO( void )
{
	dsphSetAudioConfigImages();
	WriteHardware(AUDIO_CONFIG, gAudioConfigImage );
	WriteHardware(AUDIN_CONFIG, gAudinConfigImage );
	WriteHardware(AUDOUT_CONFIG, gAudoutConfigImage );

/* Now that ADIO is running, unmute DAC. */
	SuperSetSysInfo( SYSINFO_TAG_AUDIOMUTE, (void *) SYSINFO_AUDIOMUTE_NOTQUIET, 0 );

#if DEBUG_Config
	printf( "dsphConfigureADIO from ROM: AUDIO = 0x%x, AUDIN = 0x%x, AUDOUT = 0x%x\n",
		gAudioConfigImage, gAudinConfigImage, gAudoutConfigImage );
#endif
}

#if CHECK_AUDIO_CLOCK
/*****************************************************************************/
/* Measure AudioFrameClockRate with CPU to make sure it is running.
*/
static void dsphCheckAudioFrameRate( void )
{
	int32  i;
	uint32 prevCnt, newCnt, minCnt, maxCnt;
	uint32 intState;
	SystemInfo SI;
	int32  Result;
	uint32 clockSpeedMHz = 66;

/* Base min and max allowable cnt on CPU clock speed. */
	Result = SuperQuerySysInfo( SYSINFO_TAG_SYSTEM, &SI , sizeof(SI) );
        if( Result == SYSINFO_SUCCESS )
	{
		clockSpeedMHz = SI.si_CPUClkSpeed / 1000000;
	}
/*
** Based on measurements at 66 MHz, the expected value is roughly
** clockSpeedMHz*1.0 (by coincidence).  We should allow a 2X margin
** of error.
*/
	minCnt = clockSpeedMHz / 2;
	maxCnt = clockSpeedMHz * 2;
	DBUG(("clockSpeedMHz = %d, min = %d, max = %d\n", clockSpeedMHz, minCnt, maxCnt));

/* Disable interrupts for more accurate timing. */
	intState = Disable();

/* What is current value? */
	prevCnt = newCnt = ReadHardware( DSPX_FRAME_UP_COUNTER );

/* Wait for first rising edge. */
	for( i=0; i<maxCnt; i++ )
	{
		newCnt = ReadHardware( DSPX_FRAME_UP_COUNTER );
		if (newCnt != prevCnt) break;
	}

/* Did we see transition? */
	if (newCnt != prevCnt)
	{
		prevCnt = newCnt;
/* Wait for next rising edge. */
		for( i=0; i<maxCnt; i++ )
		{
			newCnt = ReadHardware( DSPX_FRAME_UP_COUNTER );
			if (newCnt != prevCnt) break;
		}
	}

/* Restore Interrupts. */
	Enable(intState);

/* Is measured count out of range? */
	if( (i<minCnt) || (i>maxCnt) )
	{
		PRT(("ERROR - Audio Frame Clock not working.  Check LCCD connection.\n"));
	}
	else
	{
		PRT(("    Audio Frame Clock cycle took %d loops.\n", i ));
	}
}
#endif

/*****************************************************************************/
static void dsphOutputPair( int32 LeftVal, int32 RightVal )
{
	dsphWriteDataMem(DSPI_OUTPUT0, LeftVal);
	dsphWriteDataMem(DSPI_OUTPUT1, RightVal);
	dsphWriteDataMem(DSPI_OUTPUT_CONTROL, 1);
}

/*****************************************************************************/
static void dsphResetAudOut( void )
{
	WriteHardware( DSPX_RESET, DSPX_F_RESET_OUTPUT );
	WriteHardware( DSPX_RESET, DSPX_F_RESET_OUTPUT ); /* Twice to allow sync. */
	WriteHardware( DSPX_RESET, 0 );                  /* Unreset */
}

static void dsphResetAudIn( void )
{
	WriteHardware( DSPX_RESET, DSPX_F_RESET_INPUT );
	WriteHardware( DSPX_RESET, DSPX_F_RESET_INPUT ); /* Twice to allow sync. */
	WriteHardware( DSPX_RESET, 0 );                  /* Unreset */
}

static void dsphClearAudOut( void )
{
	int32 i;
	for( i=0; i<DSPI_OUTPUT_FIFO_MAX_DEPTH; i++ )
	{
		dsphOutputPair( 0, 0 );
	}
}

/* !!! Do we have a potential race condition if IN clicks over before
** OUT is enabled.  Would we then be out of sync.
** !!! Maybe we should clear the ADC FIFO here???
*/
void dsphEnableAudOut( void )
{
	WriteHardware(AUDOUT_CONFIG, gAudoutConfigImage | 0x80000000 );
	WriteHardware(DSPX_INTERRUPT_CLR, DSPX_F_INT_OUTPUT_UNDER );
}
#define DSPH_EnableAudIn WriteHardware(AUDIN_CONFIG, gAudinConfigImage | 0x80000000 );

/* Start ADIO cleanly. */
void dsphEnableADIO( void )
{
/* Can only send one frame when in RESET mode because
** can't advance frame pointer.
*/
	dsphResetAudOut();
	dsphClearAudOut();
	dsphResetAudOut();
	dsphClearAudOut();
	dsphResetAudIn();
	dsphEnableAudOut();
	DSPH_EnableAudIn;

#if CHECK_AUDIO_CLOCK
	dsphCheckAudioFrameRate();
#endif
}

/* Stop ADIO */
void dsphDisableADIO( void )
{
	WriteHardware(AUDOUT_CONFIG, gAudoutConfigImage & ~0x80000000 );
	WriteHardware(AUDIN_CONFIG, gAudinConfigImage & ~0x80000000 );
	dsphResetAudOut();
	dsphResetAudIn();
}


/* -------------------- Misc master controls */

/*******************************************************************/
void dsphReset( void )
{
	WriteHardware(DSPX_RESET,-1);  /* -1 resets everything. */
}

/*******************************************************************/
void dsphHalt( void )
{
	WriteHardware( DSPX_CONTROL, 0 );
}

/*******************************************************************/
void dsphStart( void )
{
	WriteHardware( DSPX_CONTROL, DSPX_F_GWILLING );
}


/* -------------------- FIFOs and DMA */
void dsphClearInputFIFO( int32 DMAChan )
{
	uint32 Mask = (1L << DMAChan);

/* Force to RAM->DSP mode */
	dsphSetChannelDirection( DMAChan, DSPX_DIR_RAM_TO_DSPP );
	WriteHardware( DSPX_CHANNEL_RESET, Mask );

/* Clear FIFO RAM by DMAing zero samples. */
DBUG(("gZeroData = 0x%x, gZeroDataAligned = 0x%x\n", gZeroData, gZeroDataAligned ));
	dsphSetInitialDMA( DMAChan, (AudioGrain *) gZeroDataAligned, DSPI_FIFO_MAX_DEPTH );
	dsphEnableDMA( DMAChan );
/*
** WARNING - Do NOT clear a FIFO by writing to the FIFO_BUMP_CURR
** register from the PPC.  That can mess up other channels if
** the DSP is using the Oscillator hardware.
*/
}

/*******************************************************************
** Reset FIFO
*******************************************************************/
void dsphResetFIFO( int32 DMAChan )
{
	uint32 Mask;
	Mask = (1L << DMAChan);
/* Completely clear FIFO before starting. */
	WriteHardware( DSPX_CHANNEL_RESET, Mask );
	dsphDisableChannelInterrupt( DMAChan );

/* Clear interrupts that may have been set previously. */
	WriteHardware( DSPX_INT_DMANEXT_CLR, Mask );
	WriteHardware( DSPX_INT_CONSUMED_CLR, Mask );
	WriteHardware( DSPX_INT_UNDEROVER_CLR, Mask );
}

/*******************************************************************/
void dsphResetAllFIFOs( void )
{
	int32 i;
	for( i=0; i<DSPI_NUM_DMA_CHANNELS; i++ ) dsphResetFIFO(i);
}

/*******************************************************************/
int32 dsphGetChannelDirection( int32 Channel )
{
	int32 Mask;
	Mask = 1UL << Channel;
	return (ReadHardware( DSPX_CHANNEL_DIRECTION_SET ) & Mask) ? OUTPUT_FIFO : INPUT_FIFO;
}

/*******************************************************************/
void dsphSetChannelDirection( int32 Channel, int32 Direction )
{
	int32 Mask;

	Mask = 1UL << Channel;
DBUG(("dsphSetChannelDirection( 0x%x, %d )\n", Channel, Direction ));
	if( Direction == DSPX_DIR_RAM_TO_DSPP )
	{
		WriteHardware( DSPX_CHANNEL_DIRECTION_CLR, Mask );
	}
	else
	{
		WriteHardware( DSPX_CHANNEL_DIRECTION_SET, Mask );
	}
}

/*******************************************************************/
void dsphSetChannel8Bit( int32 Channel, int32 If8Bit )
{
	int32 Mask;

	Mask = 1UL << Channel;
DBUG(("dsphSetChannelDecompression( 0x%x, %d )\n", Channel, If8Bit ));
	if( If8Bit )
	{
		WriteHardware( DSPX_CHANNEL_8BIT_SET, Mask );
	}
	else
	{
		WriteHardware( DSPX_CHANNEL_8BIT_CLR, Mask );
	}
}

/*******************************************************************/
void dsphSetChannelDecompression( int32 Channel, int32 IfSQXD )
{
	int32 Mask;

	Mask = 1UL << Channel;
DBUG(("dsphSetChannelDecompression( 0x%x, %d )\n", Channel, IfSQXD ));
	if( IfSQXD )
	{
		WriteHardware( DSPX_CHANNEL_SQXD_SET, Mask );
		WriteHardware( DSPX_CHANNEL_8BIT_SET, Mask );
	}
	else
	{
		WriteHardware( DSPX_CHANNEL_SQXD_CLR, Mask );
		WriteHardware( DSPX_CHANNEL_8BIT_CLR, Mask );
	}
}

/*******************************************************************/
static void dsphInitDMA( void )
{
	WriteHardware( DSPX_CHANNEL_DISABLE, -1 );
	WriteHardware( DSPX_CHANNEL_DIRECTION_CLR, -1 );
	WriteHardware( DSPX_CHANNEL_8BIT_CLR, -1 );
	WriteHardware( DSPX_CHANNEL_SQXD_CLR, -1 );

	dsphResetAllFIFOs();
}

static void dsphInitInterruptRegs( void )
{
	WriteHardware( DSPX_INTERRUPT_CLR, ~0 );
	WriteHardware( DSPX_INTERRUPT_DISABLE, ~0 );
	WriteHardware( DSPX_INT_DMANEXT_CLR, ~0 );
	WriteHardware( DSPX_INT_CONSUMED_CLR, ~0 );
	WriteHardware( DSPX_INT_CONSUMED_DISABLE, ~0 );
	WriteHardware( DSPX_INT_UNDEROVER_CLR, ~0 );
	WriteHardware( DSPX_INT_UNDEROVER_DISABLE, ~0 );
}

/***************************************************************/
void dsphEnableChannelInterrupt( int32 DMAChan )
{
	int32 Mask;
DBUG(( "dsphEnableChannelInterrupt(0x%x)\n", DMAChan ));

#if 0
/* Check to see if DMANEXT already set. ONLY when debugging. */
	if( ReadHardware(DSPX_INT_DMANEXT_SET) & (1<<DMAChan) )
	{
		PRT(("DMANext already set for channel %d\n", DMAChan ));
	}
#endif

	Mask = DSPX_F_INT_DMANEXT_EN | DSPX_F_SHADOW_SET_DMANEXT;
/* Set shadow control register. */
	WriteHardware( DSPX_DMA_CONTROL_CURRENT(DMAChan), Mask );
}

/***************************************************************/
void dsphDisableChannelInterrupt( int32 DMAChan )
{
/* Set shadow control register. */
	int32 Mask = 0 | DSPX_F_SHADOW_SET_DMANEXT;
	WriteHardware( DSPX_DMA_CONTROL_CURRENT(DMAChan), Mask );
}

int32 dsphGetFIFOPartAddress( int32 channel, int32 part )
{
	return DSPI_FIFO_OSC_BASE + ((uint32)(channel) * DSPI_FIFO_OSC_SIZE) + part;
}

/* -------------------- Data Memory read/write support */
/* (only functions in paranoid mode - otherwise are just macros) */

#ifdef PARANOID /* { */

/*****************************************************************/
/* DSPI_Addr is relative to base of DSPI data memory. */
int32 dsphReadDataMem( int32 DSPI_Addr )
{
DBUG(("dsphReadDataMem( DSPI_Addr = 0x%x )\n", DSPI_Addr ));

	if( (DSPI_Addr < 0) ||
	    (DSPI_Addr >= DSPI_DATA_MEMORY_SIZE))
	{
		ERR(("dsphReadDataMem: Data address out of range = 0x%x\n", DSPI_Addr));
		return -1;
	}

	return (int32) (int16) ReadHardware (DSPX_DATA_MEMORY + DSPI_Addr);
}

/*******************************************************************
** Write to parameter memory that can be read by DSPP
*******************************************************************/
void dsphWriteDataMem( int32 DSPI_Addr, int32 Value )
{
	if( (DSPI_Addr < 0) ||
	    (DSPI_Addr >= DSPI_DATA_MEMORY_SIZE))
	{
		ERR(("dsphWriteDataMem: Data address out of range = 0x%x\n", DSPI_Addr));
		return;
	}

	WriteHardware (DSPX_DATA_MEMORY + DSPI_Addr, Value);
}

#endif  /* } PARANOID */

