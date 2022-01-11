/* @(#) dspp_simulator.c 96/06/19 1.23 */
/* $Id: dspp_simulator.c,v 1.12 1995/03/21 01:30:24 phil Exp phil $ */

/***************************************************************
** DSP Simulator for M2 Software development
**
** Author: Phil Burk
**
** Copyright (C) 1995 3DO
** All rights reserved.
**
****************************************************************
** To Do:
** X- Make debug switchable by writing to a DSPI address, TraceMask.
** X- debug _++, _--
** X- add Barrel Shifter operands
** O- debug BRP in tv_branches1.fth
** O- add ROTATE with Barrel Shift
** X- finish FDMA, test with SIMPLE write DMA
** X- add Codec
** X- add Timer
** X- add correct NumOps=4=0 interpretation for:
**        $ 102 _[A] _= [R4] $ 100 _[A] $ 101 _[A]  [R5] _>> _*+
** X- design memory allocation scheme for MEMTYPE_DMA
** X- resolve setting and checking of data memory, h.mem! h.mem@ ???
** O- Add pass 2 support:
** X-   number of DMA channels increased from 24 to 32
** X-   oscillator phase register is now shared between all channels
** X-   DMA Address and Count now uses a shadow register similar to NextAddress andNextCount
** X-   DMANext interrupt enables can be read all at once from a new register
** X-   DMANext interrupt is now only set when the Next Address and Count is read, not when count goes to zero
** X-   timer architecture simplified
** X-   DMALate interrupt removed
** O-   ADIO buffer moved into top of physical data memory
** X-   number of software interrupts increased from 2 to 16
** O-   address map for DSPI peripheral space completely modified

****************************************************************
** 950117 PLB Creation.  Basic MOVEs
** 950119 PLB Added arithmetics, register, branches.
** 950314 WJB Added #ifndef around ERR() definition.
** 950428 PLB Mask data in FIFO with 0x0000FFFF
** 950505 WJB Fixed some warnings.
** 950511 WJB Added pass 2 soft int support.
***************************************************************/

#ifdef SIMULATE_DSPP    /* { */

#define SIM_VERSION ("0.20")

	/* platform-specific includes */
#ifdef PF_HOST_MACINTOSH
	#include <StdLib.h>
	#include <StdIO.h>
	#include <String.h>
#endif

#ifdef PF_HOST_UNIX
	#include <ctype.h>
	#include <malloc.h>
	#include <math.h>
	#include <memory.h>
	#include <string.h>
	#include <stdlib.h>
	#include <stdio.h>
#else
	#include <dspptouch/touch_hardware.h>   /* prototypes for Read/WriteHardware() */
	#include <kernel/mem.h>
	#include <stdlib.h>
	#include "dspptouch_internal.h"
	#include <dspptouch/dspp_simulator.h>
	#include <dspptouch/dspp_addresses.h>
	#define HOST_3DO
#endif

	/* types 'n stuff normally defined by 3DO includes */
#ifndef HOST_3DO
	typedef long vint32;
	typedef unsigned long vuint32;
	typedef short int16;
	typedef unsigned short uint16;
	#ifndef TRUE
		#define TRUE  (1)
		#define FALSE (0)
	#endif
	#include "dspp_addresses.h"
#endif

/* platform independent includes (!!! ideally at any rate, but seeing as they live in
${SRC}/includes/dspptouch, these really aren't cross-platform) */

#ifndef AF_ASIC_M2
	#error "SIMULATE_DSPP" should only be used with "AF_ASIC_M2"
#endif


/***************************************************************
** Options
***************************************************************/
/* If defined, audio frame advances every DSPP_TICKS_PER_FRAME clocks. */
/* Otherwise it advances every time we advance Output FIFO */
/* #define DSPS_FRAME_BY_CLOCK */
/***************************************************************
** Constants
***************************************************************/

#define DSPS_MODE_READ  (1)
#define DSPS_MODE_WRITE (0)
#define DSPS_STACK_DEPTH  (4)
#define DSPS_MAX_OPERANDS (8)

/* Options are:
**   DSPIB_TRACE_FDMA
**   DSPIB_TRACE_EXEC
**   DSPIB_TRACE_VERBOSE
**   DSPIB_TRACE_RW       - trace Read/WriteHardware()
*/
#define TRACE_MASK_DEFAULT  (0)

/***************************************************************
** Data Structures that correspond to M2 Hardware
***************************************************************/
#define FDMA_FIFO_SIZE  (8)

typedef struct DSPI_FifoDma
{
	int32   fdma_CurrentAddress;
	int32   fdma_CurrentCount;
	int32   fdma_NextAddress;
	int32   fdma_NextCount;
	int32   fdma_PreviousValue;   /* Used for SQXD decompression. */
	int32   fdma_PreviousCurrent; /* Used when FIFO underflows. */
	int8    fdma_GoForever;       /* If true, reuse Next Addr,Cnt */
	int8    fdma_NextValid;       /* If true, Next Addr,Cnt are valid */
	int8    fdma_Reserved;
	int32   fdma_FIFO[FDMA_FIFO_SIZE]; /* 16 bit data, masked with 0xFFFF */
	int32   fdma_DMA_Ptr;         /* DMA pointer into FIFO */
	int32   fdma_DSPI_Ptr;        /* DSPI pointer into FIFO */
	int32   fdma_Depth;           /* Number of values in FIFO */
} DSPI_FifoDma;

#define DSPI_NUM_OUTPUTS   (8)
#define DSPI_OUTPUT_FIFO_SIZE   (DSPI_OUTPUT_FIFO_MAX_DEPTH*2)
#define DSPP_TICKS_PER_FRAME   (200)

typedef struct DSPP_Internal
{
	int32   dspi_DataMemory[0x300];
	DSPI_FifoDma dspi_FifoDma[DSPI_NUM_DMA_CHANNELS];
	int32   dspi_Frequency;     /* Only a single register. */
	int32   dspi_Phase;         /* Only a single register. */
	int32   dspi_LastOscCount;  /* Number of samples read by oscillator. */
	int32   dspi_Inputs[2];
	int32   dspi_Outputs[DSPI_NUM_OUTPUTS];
	int32   dspi_OutputFIFO[DSPI_OUTPUT_FIFO_SIZE];
	int32   dspi_OutputWritePtr;
	int32   dspi_OutputReadPtr;
	FILE   *dspi_OutputFile;
	int32   dspi_LastFrameClock;   /* DSPP clock value when last frame occured */
} DSPP_Internal;

typedef struct DSPP_Operand
{
	int32   dopr_Address; /* -1 or valid */
	int32   dopr_Value; /* -1 or valid */
} DSPP_Operand;

typedef struct DSPP_Brain
{
	int32   dbrn_Accumulator;   /* 20 bit */
	int32   dbrn_ProgramCounter;
	int32   dbrn_Clock;
	int32   dbrn_CommandReg;
	int32   dbrn_WriteBackAddress;  /* write back if non-negative */
	int32   dbrn_Stack[DSPS_STACK_DEPTH];
	int32   dbrn_StackPtr;
	int32   dbrn_GWilling;   /* If true, execute code */
	int32   dbrn_AudLock;    /* If true, reset every frame */
	int8    dbrn_CC_Carry;
	int8    dbrn_CC_Zero;
	int8    dbrn_CC_Negative;
	int8    dbrn_CC_Overflow;
	int8    dbrn_CC_Exact;
	int8    dbrn_Sleeping;     /* True if _SLEEP executing */
	int8    dbrn_Reserved0;
	int8    dbrn_Reserved1;
	DSPP_Operand  dbrn_Operands[DSPS_MAX_OPERANDS];    /* Parsed from instruction stream. */
	int32   dbrn_OpUseIndex;   /* Incremented as each operand is consumed by instruction. */
	int32   dbrn_RBase0;
	int32   dbrn_RBase4;
	int32   dbrn_RBase8;
	int32   dbrn_RBase12;
} DSPP_Brain;

typedef struct DSPP_Registers
{
	int32         dspr_CodeMemory[1024];
	DSPP_Internal dspr_Internal;
	DSPP_Brain    dspr_Brain;
	uint32        dspr_TraceMask;   /* Set bits to turn on debug. */
	uint32        dspr_ChannelEnable;
	uint32        dspr_ChannelDirection;
	uint32        dspr_Channel8Bit;
	uint32        dspr_ChannelSQXD;
	uint32        dspr_ChannelComplete;        /* DMA finished */
#if (AF_BDA_PASS > 1)
	uint32        dspr_ShadowCurrentAddress;   /* Hold  current address for atomic update. */
	uint32        dspr_ShadowCurrentCount;
#endif
	uint32        dspr_ShadowNextAddress;      /* Hold  next address for atomic update. */
	uint32        dspr_ShadowNextCount;
/* Interrupt registers. */
	uint32        dspr_MainInterruptPartial;   /* main DSPP interrupts, partly combinatorial */
	uint32        dspr_MainIntEnable;      /* main DSPP interrupts, partly combinatorial */
	uint32        dspr_DMANextInterrupt;   /* DMANext interrupts */
	uint32        dspr_DMANextIntEnable;   /* If true, DMANext interrupt enabled. */
	uint32        dspr_ConsumedInterrupt;  /* DMA&FIFO data consumed interrupts */
	uint32        dspr_ConsumedIntEnable;  /* If true, consumed interrupt enabled. */
	uint32        dspr_UnderOverInterrupt;
	uint32        dspr_UnderOverIntEnable;   /* If true, UnderOver interrupt enabled. */
/* Audio Timer */
	uint16        dspr_AudioDuration;       /* 16 bit count down value */
	uint32        dspr_AudioTime;
#if (AF_BDA_PASS == 1)
	uint32        dspr_DMALateInterrupt;
	uint32        dspr_DMALateIntEnable;   /* If true, DMALate interrupt enabled. */
	uint32        dspr_AudioFrameCounter;
	uint32        dspr_WakeupTime;
#endif /* AF_BDA_PASS */

} DSPP_Registers;


/***************************************************************
** Global Data
***************************************************************/
DSPP_Registers *DSPRegs = NULL;
int32 (*gIntHandlerFunc)( void );
int32 gIfServicingInterrupt = 0;

/***************************************************************
** Macros
***************************************************************/

#ifndef ABS
	#define ABS(x) (((x)<0)?-(x):(x))
#endif

/* #ifndef PRT */
#define PRT(x) { printf x; fflush(stdout); }
/* #endif */

/* #ifndef ERR */
#define ERR(x) PRT(x)
/* #endif */

#define TEST_VERBOSE (DSPRegs->dspr_TraceMask & DSPIB_TRACE_VERBOSE)
#define TEST_EXEC (DSPRegs->dspr_TraceMask & DSPIB_TRACE_EXEC)
#define TEST_FDMA (DSPRegs->dspr_TraceMask & DSPIB_TRACE_FDMA)
#define TEST_RW (DSPRegs->dspr_TraceMask & DSPIB_TRACE_RW)
#define TEST_OUTPUT (DSPRegs->dspr_TraceMask & DSPIB_TRACE_OUTPUT)

#ifdef DBUG
	#undef DBUG
#endif
#define DBUG(x) { if(TEST_VERBOSE) PRT(x); }

#define TRACE_VERBOSE(x) { if(TEST_VERBOSE) PRT(x); }
#define TRACE_EXEC(x) { if(TEST_EXEC || TEST_VERBOSE) PRT(x); }
#define TRACE_FDMA(x) { if(TEST_FDMA || TEST_VERBOSE) PRT(x); }
#define TRACE_RW(x) { if(TEST_RW || TEST_VERBOSE) PRT(x); }

#define UNIMPLEMENTED(msg) DBUG(("DSP: %s unimplemented!\n", msg))
#define ILLEGAL_ADDRESS(addr) ERR(("DSP: Illegal address = 0x%x\n", (int) (addr)))

#define DSP_BRAIN (DSPRegs->dspr_Brain)
#define DSP_CODE  (DSPRegs->dspr_CodeMemory)
#define DSP_PC    (DSP_BRAIN.dbrn_ProgramCounter)
#define DSP_CLOCK    (DSP_BRAIN.dbrn_Clock)
#define DSP_ACCUME    (DSP_BRAIN.dbrn_Accumulator)
#define DSP_COMMAND    (DSP_BRAIN.dbrn_CommandReg)
#define DSP_FDMA(n)    (DSPRegs->dspr_Internal.dspi_FifoDma[n])

#define DSPI_READ(ad)  (dspsAccessData(ad,0,DSPS_MODE_READ))
#define DSPI_WRITE(ad,v)  (dspsAccessData(ad,v,DSPS_MODE_WRITE))

#define CC_CARRY   (DSP_BRAIN.dbrn_CC_Carry)
#define CC_ZERO    (DSP_BRAIN.dbrn_CC_Zero)
#define CC_NEG     (DSP_BRAIN.dbrn_CC_Negative)
#define CC_OVER    (DSP_BRAIN.dbrn_CC_Overflow)
#define CC_EXACT   (DSP_BRAIN.dbrn_CC_Exact)

#define SIGN_EXTEND8(n) (((n) & 0x80) ? (((int32)n) | 0xFFFFFF00) : (((int32)n) & 0x000000FF))
#define SIGN_EXTEND16(n) (((n) & 0x8000) ? (((int32)n) | 0xFFFF0000) : (((int32)n) & 0x0000FFFF))
#define SIGN_EXTEND20(n) (((n) & 0x80000) ? (((int32)n) | 0xFFF00000) : (((int32)n) & 0x000FFFFF))

#define IN_RANGE(x,lo,hi) (((x)>=((int32)lo))&&((x)<=((int32)hi)))
#define READWRITE16(adr,val) (dspsReadWrite16( (uint32 *)adr, (uint32) val, ReadWrite))
#define READWRITE32(adr,val) (dspsReadWrite32( (uint32 *)adr, (uint32) val, ReadWrite))

#ifdef PF_HOST_UNIX
	#define RANDOM  random()
#else
	#define RANDOM  rand()
#endif

/* Define part numbers for FDMA to simplify PAss 1 versus Pass 2 code */
#define DSPI_FIFO_PART_CURRENT      (0)
#define DSPI_FIFO_PART_NEXT         (1)
#define DSPI_FIFO_PART_FREQUENCY    (2)
#define DSPI_FIFO_PART_PHASE        (3)
#define DSPI_FIFO_PART_DATA         (4)
#define DSPI_FIFO_PART_CONTROL      (5)

/***************************************************************
** Prototypes
***************************************************************/
int32 dspsAccessInternal( int32 Addr, int32 Val, int32 ReadWrite );
int32 dspsAccessData( int32 Addr, int32 Val, int32 ReadWrite );
static void dspsAdvanceAudioFrame( void );
void CheckCode( char *Msg );
static void dspsResetDSPP( void );


/***************************************************************
** Code
***************************************************************/



/**********************************************************************
** Display single value by drawing stars on terminal.
**********************************************************************/
void PlotLine( int32 Val, int32 Min, int32 Max, int32 MaxStars )
{
	int32 NumStars;
	int32 i;
	char StarBuffer[256];

	NumStars = ((Val-Min) * MaxStars) / (Max - Min);

	for( i=0; i<MaxStars; i++)
	{
		StarBuffer[i] = (i < NumStars) ? '*' : '.';
	}
	StarBuffer[MaxStars] = (char)0; /* NUL terminate */

	PRT(("0x%08x : %s", (int) Val, StarBuffer));
}

void PlotStereoPair( int32 LeftSample, int32 RightSample )
{
	PlotLine( SIGN_EXTEND16(LeftSample), -0x8000, 0x7FFF, 32 );
	PRT((" , "));
	PlotLine( SIGN_EXTEND16(RightSample), -0x8000, 0x7FFF, 32 );
	PRT(("\n"));
}

#define ADIO_FILENAME  ("sim.out.raw")
/**********************************************************************
** dspsControlAudioOutputFile - specify an output file for the adio
*/
static void dspsControlAudioOutputFile( int32 Val )
{
	if( Val == 0 )  /* Close it. */
	{
/* Check to see if file already opened. */
		if( DSPRegs->dspr_Internal.dspi_OutputFile != NULL )
		{
			fclose( DSPRegs->dspr_Internal.dspi_OutputFile );
			DSPRegs->dspr_Internal.dspi_OutputFile = NULL;
		}
	}
	else
	{
/* Check to see if file already opened. */
		if( DSPRegs->dspr_Internal.dspi_OutputFile == NULL )
		{
			DSPRegs->dspr_Internal.dspi_OutputFile = fopen( ADIO_FILENAME, "w" );
			if(DSPRegs->dspr_Internal.dspi_OutputFile == NULL)
			{
				ERR(("dspsControlAudioOutputFile: Could not open simulator output file!\n"));
				exit(-1);
			}
			PRT(("dspsControlAudioOutputFile: Simulator output written to %s\n", ADIO_FILENAME ));
		}
	}
}

/**********************************************************************
** dspsAudioFrameToDAC - output an audio frame to "DAC"
*/
static void dspsAudioFrameToDAC( void )
{
	int32 i,ri;
	int32 OutSamp;
	int8  OutByte;

/* Don't output last frame because of nasty wrap. */
	ri = DSPRegs->dspr_Internal.dspi_OutputReadPtr;
	if( ((DSPRegs->dspr_Internal.dspi_OutputWritePtr - ri) & (DSPI_OUTPUT_FIFO_MAX_DEPTH - 1)) == 1)
	{
		ERR(("dspsAudioFrameToDAC: Output FIFO underflow!\n"));
		return;
	}

/* Plot DAC output if trace enabled. */
	if( TEST_OUTPUT )
	{
		PlotStereoPair( DSPRegs->dspr_Internal.dspi_OutputFIFO[ri],
			DSPRegs->dspr_Internal.dspi_OutputFIFO[ri+1] );
	}

/* Write 2 samples to file if open. */
	if( DSPRegs->dspr_Internal.dspi_OutputFile )
	{
		for( i=0; i<2; i++ )
		{
			OutSamp = DSPRegs->dspr_Internal.dspi_OutputFIFO[ri+i];
			OutByte = (int8) (OutSamp >> 8);
			fwrite( (const void *) &OutByte, 1, 1, DSPRegs->dspr_Internal.dspi_OutputFile );
			OutByte = (int8) OutSamp;
			fwrite( (const void *) &OutByte, 1, 1, DSPRegs->dspr_Internal.dspi_OutputFile );
		}
	}

/* Advance reader. */
	DSPRegs->dspr_Internal.dspi_OutputReadPtr = ri + 2;
	if( DSPRegs->dspr_Internal.dspi_OutputReadPtr >= DSPI_OUTPUT_FIFO_MAX_DEPTH )
	{
		DSPRegs->dspr_Internal.dspi_OutputReadPtr = 0;
	}
}

/**********************************************************************
** dspsAudioFrameToFIFO - output an audio frame to FIFO
*/
static void dspsAudioFrameToFIFO( void )
{
	int32 i,wi;

	if( DSPRegs->dspr_Internal.dspi_OutputWritePtr == DSPRegs->dspr_Internal.dspi_OutputReadPtr )
	{
		UNIMPLEMENTED("dspsAudioFrameToFIFO: Overflow!");
	}

/* Write 2 samples to FIFO. */
	wi = DSPRegs->dspr_Internal.dspi_OutputWritePtr;
	for( i=0; i<2; i++ )
	{
		DSPRegs->dspr_Internal.dspi_OutputFIFO[wi+i] = DSPRegs->dspr_Internal.dspi_Outputs[i];
/* Echo to input. */
		DSPRegs->dspr_Internal.dspi_Inputs[i] = DSPRegs->dspr_Internal.dspi_Outputs[i];
	}

/* Advance writer. */
	DSPRegs->dspr_Internal.dspi_OutputWritePtr = wi + 2;
	if( DSPRegs->dspr_Internal.dspi_OutputWritePtr >= DSPI_OUTPUT_FIFO_MAX_DEPTH )
	{
		DSPRegs->dspr_Internal.dspi_OutputWritePtr = 0;
	}

#ifndef DSPS_FRAME_BY_CLOCK
	dspsAdvanceAudioFrame();
#endif
}

/**********************************************************************
** dspsCheckTimerInterrupt - set timer interrupt if wakeup time indicates
*/
static void dspsCheckTimerInterrupt( void )
{
#if (AF_BDA_PASS == 1)
	if( ((DSPRegs->dspr_AudioTime - DSPRegs->dspr_WakeupTime) & 0x8000000) == 0 )
	{
DBUG(("dspsCheckTimerInterrupt: set it.\n"));
		DSPRegs->dspr_MainInterruptPartial |= DSPX_F_INT_TIMER;
	}
#endif
}

/**********************************************************************
** dspsAdvanceAudioTimer
*/
static void dspsAdvanceAudioTimer( void )
{
#if (AF_BDA_PASS == 1)
DBUG(("dspsAdvanceAudioTimer: dspr_AudioFrameCounter = %d\n", (int) DSPRegs->dspr_AudioFrameCounter));
	if( (--DSPRegs->dspr_AudioFrameCounter) == 0 )
	{
		DSPRegs->dspr_AudioFrameCounter = DSPRegs->dspr_AudioDuration;
		DSPRegs->dspr_AudioTime++;
	}
#else
/* Advance time on each frame count. */
	DSPRegs->dspr_AudioTime++;
/* Interrupt on transition from 0 to 0xFFFF */
	if( (--DSPRegs->dspr_AudioDuration) == 0xFFFF )
	{
		DSPRegs->dspr_MainInterruptPartial |= DSPX_F_INT_TIMER;
	}
#endif
}


/**********************************************************************
** dspsAdvanceAudioFrame - advance DAC Frame
*/
static void dspsAdvanceAudioFrame( void )
{
DBUG(("dspsAdvanceAudioFrame: DSP_BRAIN.dbrn_AudLock = 0x%x\n", DSP_BRAIN.dbrn_AudLock));
	dspsAudioFrameToDAC();
	DSPRegs->dspr_Internal.dspi_LastFrameClock = DSP_CLOCK;
	dspsAdvanceAudioTimer();
	if( DSP_BRAIN.dbrn_AudLock )
	{
		dspsResetDSPP();
	}
}

/**********************************************************************
** dspsAdvancePC - advance PC and clock DSPP and ADIO
*/
static void dspsAdvancePC( void )
{
	DSP_PC++;
	DSP_CLOCK--;

#ifdef DSPS_FRAME_BY_CLOCK
/* Check to see if DAC frame occurs. */
DBUG(("dspsAdvancePC - DSP_CLOCK = 0x%x\n", DSP_CLOCK ));
	if( ((DSPRegs->dspr_Internal.dspi_LastFrameClock - DSP_CLOCK) & 0xFFF) > DSPP_TICKS_PER_FRAME )
	{
		dspsAdvanceAudioFrame();
	}
#endif /* DSPS_FRAME_BY_CLOCK */
}

/**********************************************************************
** dspsReadWrite16 - convenience routine to read or write a variable
*/
static uint32 dspsReadWrite16( uint32 *HostAddress, uint32 Value, int32 ReadWrite)
{
	if(ReadWrite == DSPS_MODE_READ)
	{
		return (*HostAddress & 0xFFFF);
	}
	else
	{
		*HostAddress = (uint32) (0xFFFF & Value);
		return 0;
	}
}

/**********************************************************************
** dspsReadWrite32 - convenience routine to read or write a variable
*/
static uint32 dspsReadWrite32( uint32 *HostAddress, uint32 Value, int32 ReadWrite)
{
	if(ReadWrite == DSPS_MODE_READ)
	{
		return (*HostAddress);
	}
	else
	{
		*HostAddress = (uint32) (Value);
		return 0;
	}
}

/**********************************************************************
** dspsSetRBase( int32 WhichRBase, int32 Address )
*/
static void dspsSetRBase( int32 WhichRBase, int32 Address )
{
	switch( WhichRBase )
	{
		case 4:
			DSP_BRAIN.dbrn_RBase4 = Address + 4 - WhichRBase;
			break;
		case 0:
			DSP_BRAIN.dbrn_RBase0 = Address;
			DSP_BRAIN.dbrn_RBase4 = Address + 4 - WhichRBase;
			/* NO BREAK */
		case 8:
			DSP_BRAIN.dbrn_RBase8 = Address + 8 - WhichRBase;
			/* NO BREAK */
		case 12:
			DSP_BRAIN.dbrn_RBase12 = Address + 12 - WhichRBase;
			break;
	}
	DBUG(("dspsSetRBase: RB0,4,8,12 = 0x%x, 0x%x, 0x%x, 0x%x\n",
		(int) DSP_BRAIN.dbrn_RBase0,
		(int) DSP_BRAIN.dbrn_RBase4,
		(int) DSP_BRAIN.dbrn_RBase8,
		(int) DSP_BRAIN.dbrn_RBase12 ));
}

/**********************************************************************
** dspsTranslateReg( int32 Reg )
*/
int32 dspsTranslateReg( int32 Reg )
{
	if( Reg < 4 ) return DSP_BRAIN.dbrn_RBase0 + Reg;
	if( Reg < 8 ) return DSP_BRAIN.dbrn_RBase4 + Reg - 4;
	if( Reg < 12 ) return DSP_BRAIN.dbrn_RBase8 + Reg - 8;
	return DSP_BRAIN.dbrn_RBase12 + Reg - 12;
}

/**********************************************************************
** dspsResetChannel - Reset a single DMA channel
*/
static void dspsResetChannel( int32 Channel )
{
	DSPI_FifoDma *fdma;

	fdma = &DSP_FDMA(Channel);

#if (AF_BDA_PASS == 1)
	DSPRegs->dspr_ConsumedInterrupt &= ~((1L)<<Channel);
#endif
	DSPRegs->dspr_ChannelComplete &= ~((1L)<<Channel);

/* Clear FIFOs */
	fdma->fdma_DMA_Ptr = 0;
	fdma->fdma_DSPI_Ptr = 0;
	fdma->fdma_Depth = 0;

}

/**********************************************************************
** dspsResetChannels - Reset any channel whose bit is set in Val
*/
static void dspsResetChannels( int32 Mask )
{
	int32 i;
	for( i=0; i<DSPI_NUM_DMA_CHANNELS; i++ )
	{
		if( Mask & ((1L)<<i) ) dspsResetChannel( i );
	}
}

/**********************************************************************
** dspsResetDSPP
*/
static void dspsResetDSPP( void )
{
DBUG(("dspsResetDSPP\n"));
	DSP_BRAIN.dbrn_StackPtr = 0;
	DSP_BRAIN.dbrn_AudLock = 0;
	DSP_BRAIN.dbrn_Sleeping = 0;
	DSPRegs->dspr_TraceMask = TRACE_MASK_DEFAULT;
#if (AF_BDA_PASS == 1)
	DSPRegs->dspr_AudioFrameCounter = 1;
#endif
	DSP_PC = 0;
	dspsSetRBase( 0, 0 );
}

/**********************************************************************
** Initialize Simulator
*/
int32 dspsInitSimulator( void )
{
	PRT(("Init DSPP Simulator - Pass %d, Sim version = %s\n", AF_BDA_PASS, SIM_VERSION ));

#ifdef HOST_3DO
	if( !IsUser() )
	{
		DSPRegs = (DSPP_Registers *) SuperAllocMem (sizeof(DSPP_Registers), MEMTYPE_FILL);
	}
	else
#else
	{
		DSPRegs = (DSPP_Registers *) calloc( sizeof(DSPP_Registers), 1);
	}
#endif
	if( DSPRegs == NULL )
	{
		ERR(("Could not allocate DSPRegs\n"));
		return -1;
	}
	dspsResetDSPP();
	dspsResetChannels(-1);

	return 0;
}



/**********************************************************************
** dspsPush - push value onto stack
*/
static void dspsPush( int32 Val )
{
	if( DSP_BRAIN.dbrn_StackPtr < DSPS_STACK_DEPTH )
	{
		DSP_BRAIN.dbrn_Stack[DSP_BRAIN.dbrn_StackPtr++] = Val;
	}
	else
	{
		ERR(("DSP: Stack overflow!\n"));
	}
}

/**********************************************************************
** dspsPop - pop value from stack
*/
int32 dspsPop( void )
{
	if( DSP_BRAIN.dbrn_StackPtr > 0 )
	{
		return DSP_BRAIN.dbrn_Stack[--DSP_BRAIN.dbrn_StackPtr];
	}
	else
	{
		ERR(("DSP: Stack underflow!\n"));
	}
	return -1;
}


/**********************************************************************
** dspsParseOperands( void )
*/
int32 dspsParseOperands( int32 NumOperands )
{
	int32 Addr, Val = 0xBAD;
	int32 OpIndex = 0;
	int32 Operand = 0;
	int32 NumRegs = 0;
	int32 i;

/* Clear Operand array. */
	DSP_BRAIN.dbrn_OpUseIndex = 0;
	for( i=0; i<DSPS_MAX_OPERANDS; i++ )
	{
		DSP_BRAIN.dbrn_Operands[i].dopr_Address = -1;
		DSP_BRAIN.dbrn_Operands[i].dopr_Value = -1;
	}

	while(OpIndex < NumOperands)
	{
		Operand = DSP_CODE[DSP_PC];
		dspsAdvancePC();
DBUG(("dspsParseOperands: Operand = 0x%x\n", (int) Operand ));

		if( Operand & 0x8000 )
		{

			if( (Operand & 0xC000) == 0xC000 )  /* Immediate value */
			{
				Val = Operand & 0x1FFF;
				if( Operand & 0x2000 )
				{
					Val = Val << 3;  /* Left Justify */
				}
				else
				{ /* Sign extend if right justified. */
					if( Val & 0x1000 ) Val |= 0xE000 ;
				}
				DSP_BRAIN.dbrn_Operands[OpIndex].dopr_Value = Val;
DBUG(("dspsParseOperands: Immediate Val = 0x%x\n", (int) Val ));
				OpIndex++;
			}
			else if( (Operand & 0xE000) == 0x8000 )  /* Address operand */
			{
				Addr = Operand & 0x03FF;
DBUG(("dspsParseOperands: Addr = 0x%x\n", (int) Addr ));
				if( Operand & 0x0400 ) /* Indirect? */
				{
					Addr = DSPI_READ( Addr );
DBUG(("dspsParseOperands: INDIRECT! New Addr = 0x%x\n", (int) Addr ));
				}
				DSP_BRAIN.dbrn_Operands[OpIndex].dopr_Address = Addr;
				if( Operand & 0x0800 ) /* WriteBack? */
				{
					DSP_BRAIN.dbrn_WriteBackAddress = Addr;
				}
				OpIndex++;
			}
			else if ( (Operand & 0xE000) == 0xA000 )  /* 1 or 2 register operand */
			{
				NumRegs = (Operand & 0x0400) ? 2 : 1;
			}
		}
		else
		{
			NumRegs = 3;
		}

		if( NumRegs > 0 )
		{
			int32 i, Shifter, RegDI;

		/* Shift successive register operands from a single operand word. */
			for( i=0; i<NumRegs; i++ )
			{
				Shifter = ((NumRegs - i) - 1) * 5;
				RegDI = (Operand >> Shifter) & 0x1F;
				Addr = dspsTranslateReg( RegDI & 0xF );
DBUG(("dspsParseOperands: Reg 0x%x => Address 0x%x\n", (int) (RegDI & 0xF), (int) Addr ));

				if( RegDI & 0x0010 ) /* Indirect? */
				{
					Addr = DSPI_READ( Addr );
DBUG(("dspsParseOperands: INDIRECT! New Addr = 0x%x\n", (int) Addr ));
				}

				if( (NumRegs < 3) && (i == 0) )
				{
				/* Check writeback for Reg1 */
					if( Operand & 0x0800 ) /* WriteBack? */
					{
						DSP_BRAIN.dbrn_WriteBackAddress = Addr;
					}
				}
				else if( (NumRegs < 3) && (i == 1) )
				{
				/* Check writeback for Reg2 */
					if( Operand & 0x1000 ) /* WriteBack? */
					{
						DSP_BRAIN.dbrn_WriteBackAddress = Addr;
					}
				}
				DSP_BRAIN.dbrn_Operands[OpIndex].dopr_Address = Addr;
				OpIndex++;
			}
			NumRegs = 0;
		}
	}

	return 0;
}

/**********************************************************************
** dspsReadNextOperand( void ) - read next operand, return value
*/
int32 dspsReadNextOperand( void )
{
	int32 Addr, Value, OpIndex;

	OpIndex = DSP_BRAIN.dbrn_OpUseIndex++;
	Value = DSP_BRAIN.dbrn_Operands[OpIndex].dopr_Value;
	if( Value < 0 )
	{
		Addr = DSP_BRAIN.dbrn_Operands[OpIndex].dopr_Address;
		DBUG(("dspsReadNextOperand: Address(%d) = 0x%4x\n", (int) OpIndex, (int) Addr ));
		Value = DSPI_READ( Addr );
	}
	DBUG(("dspsReadNextOperand: Value(%d) = 0x%4x\n", (int) OpIndex, (int) Value ));
	return Value;
}

/**********************************************************************
** dspsWriteNextOperand( void ) - read next operand, return value
*/
int32 dspsWriteNextOperand( int32 Value )
{
	int32 Addr;

	Addr = DSP_BRAIN.dbrn_Operands[DSP_BRAIN.dbrn_OpUseIndex++].dopr_Address;
	if( Addr < 0 )
	{
		ERR(("dspsWriteNextOperand: write to non address operand!\n"));
		return -1;
	}
	else
	{
		return DSPI_WRITE(Addr,Value);
	}
}

/**********************************************************************
** dspsExecArithmetic - execute the arithmetic instruction in command register
*/
static void dspsExecArithmetic( void )
{
	int32 NumOps, MultSelect, MuxA, MuxB, ALU_Code, BarrelCode;
	int32 MultOp1, MultOp2;  /* 16 bit Multiplier Operands */
	int32 MultResult=0, ALU_Result=0, ALU_OpA=0, ALU_OpB=0;   /* 20 bit */

/* Extract fields from operand */
	NumOps = (DSP_COMMAND >> 13) & 3;
	MultSelect = (DSP_COMMAND >> 12) & 1;
	MuxA = (DSP_COMMAND >> 10) & 3;
	MuxB = (DSP_COMMAND >> 8) & 3;
	ALU_Code = (DSP_COMMAND >> 4) & 0xF;
	BarrelCode = (DSP_COMMAND) & 0xF;

	DBUG(("dspsExecArithmetic: NumOps = %d, MultSelect = %d\n", (int) NumOps, (int) MultSelect ));
	DBUG(("dspsExecArithmetic: MuxA = %d, MuxB = %d\n", (int) MuxA, (int) MuxB ));
	DBUG(("dspsExecArithmetic: ALU_Code = %d, BarrelCode = %d\n", (int) ALU_Code, (int) BarrelCode ));

/* Get operands from instruction stream. */
/* Check for NumOps overflow. */
	if( (NumOps == 0) && ((MuxA==1)||(MuxA==2)||(MuxB==1)||(MuxB==2)) ) NumOps = 4;
/* Check for implicit operand for Barrel Shifter. */
	if( BarrelCode == 8 ) NumOps++;
	dspsParseOperands(NumOps);

/* Perform Multiply in case we need it. */
	if( (MuxA == 0x3) || (MuxB == 0x3) )
	{
		MultOp1 = dspsReadNextOperand();
		MultOp1 = SIGN_EXTEND16(MultOp1);
		MultOp2 = (MultSelect) ? dspsReadNextOperand() : DSP_ACCUME >> 4;
		MultOp2 = SIGN_EXTEND16(MultOp2);
		MultResult = (MultOp1 * MultOp2 )>>11;
	}

/* Determine input to ALU, Multiplexer*/
	switch(MuxA)
	{
		case 0: ALU_OpA = DSP_ACCUME; break;
		case 1: ALU_OpA = (dspsReadNextOperand() << 4);
			break;
		case 2: ALU_OpA = (dspsReadNextOperand() << 4);   /* Probably not right. */
			break;
		case 3: ALU_OpA = MultResult; break;
	}
	ALU_OpA &= 0x000FFFFF;   /* So we can easily detect CARRY */
	switch(MuxB)
	{
		case 0: ALU_OpB = DSP_ACCUME; break;
		case 1: ALU_OpB = (dspsReadNextOperand() << 4);
			break;
		case 2: ALU_OpB = (dspsReadNextOperand() << 4);
			break;  /* Probably not right. */
		case 3: ALU_OpB = MultResult; break;
	}
	ALU_OpB &= 0x000FFFFF;   /* So we can easily detect CARRY */

#define CARRY_CHECK_BIT  (0x00100000)
/* Perform ALU operation. */
	switch(ALU_Code)
	{
	case 0:
		ALU_Result = ALU_OpA;               /* _TRA */
		CC_OVER = 0;
		CC_CARRY = 0;
		break;
	case 1:
		ALU_Result = -ALU_OpB;              /* _NEG */
		CC_OVER = 0;
		CC_CARRY = 0;
		break;
	case 2:
		ALU_Result = ALU_OpA + ALU_OpB;      /* _+ */
		CC_OVER = ( ((ALU_OpA & 0x80000) == (ALU_OpB & 0x80000)) &&
		            ((ALU_OpA & 0x80000) != (ALU_Result & 0x80000)) );
		CC_CARRY = ((ALU_Result & CARRY_CHECK_BIT) != 0);
		break;
	case 3:
		ALU_Result = ALU_OpA + (((int32)CC_CARRY) << 4);    /* _+C */
		CC_OVER = 0;  /* %Q no overflow for +C ??? */
		CC_CARRY = ((ALU_Result & CARRY_CHECK_BIT) != 0);
		break;
	case 4:
		ALU_Result = ALU_OpA - ALU_OpB;      /* _- */
		CC_OVER = ( ((ALU_OpA & 0x80000) == ((~ALU_OpB) & 0x80000)) &&
		            ((ALU_OpA & 0x80000) != (ALU_Result & 0x80000)) );
		CC_CARRY = ((ALU_Result & CARRY_CHECK_BIT) != 0);
		break;
	case 5:
		ALU_Result = ALU_OpA - (((int32)CC_CARRY) << 4);    /* _-B */
		CC_OVER = 0;  /* %Q no overflow for -B ??? */
		CC_CARRY = ((ALU_Result & CARRY_CHECK_BIT) != 0);
		break;
	case 6:
		ALU_Result = ALU_OpA + 1;              /* _++ */
		CC_OVER = ( !(ALU_OpA & 0x80000) && (ALU_Result & 0x80000) );
		CC_CARRY = 0;
		break;
	case 7:
		ALU_Result = ALU_OpA - 1;              /* _-- */
		CC_OVER = ( (ALU_OpA & 0x80000) && !(ALU_Result & 0x80000) );
		CC_CARRY = 0;
		break;
	case 8:
		ALU_Result = ALU_OpA;                /* _TRL */
		CC_OVER = 0;
		CC_CARRY = 0;
		break;
	case 9:
		ALU_Result = ~ALU_OpA;               /* _NOT */
		CC_OVER = 0;
		CC_CARRY = 0;
		break;
	case 10:
		ALU_Result = ALU_OpA & ALU_OpB;       /* _AND */
		CC_OVER = 0;
		CC_CARRY = 0;
		break;
	case 11:
		ALU_Result = ~(ALU_OpA & ALU_OpB);    /* _NAND */
		CC_OVER = 0;
		CC_CARRY = 0;
		break;
	case 12:
		ALU_Result = ALU_OpA | ALU_OpB;       /* _OR */
		CC_OVER = 0;
		CC_CARRY = 0;
		break;
	case 13:
		ALU_Result = ~(ALU_OpA | ALU_OpB);    /* _NOR */
		CC_OVER = 0;
		CC_CARRY = 0;
		break;
	case 14:
		ALU_Result = ALU_OpA ^ ALU_OpB;       /* _XOR */
		CC_OVER = 0;
		CC_CARRY = 0;
		break;
	case 15:
		ALU_Result = ~(ALU_OpA ^ ALU_OpB);    /* _XNOR */
		CC_OVER = 0;
		CC_CARRY = 0;
		break;
	}

/* Set Condition Codes */
	CC_NEG   = ((ALU_Result & 0x00080000) != 0);
	CC_ZERO  = ((ALU_Result & 0x000FFFF0) == 0);
	CC_EXACT = ((ALU_Result & 0x0000000F) == 0);

/* Pass result through Barrel Shifter */
	{
		int32 ShiftAmount = 0;
		static int32 BarrelCodeToShift[8] = { 0, 1, 2, 3, 4, 5, 8, 16 };

		if( BarrelCode == 8 )
		{
		/* Fetch Barrel shift operand. */
			BarrelCode = dspsReadNextOperand();
		}

		if( BarrelCode & 0x8 )   /* Negative */
		{
			int32  ShiftIndex = ((~BarrelCode) + 1) & 0x7;
			ShiftAmount = BarrelCodeToShift[ShiftIndex];
			DBUG(("DSP: Right Shift by %d\n", (int) ShiftAmount))
			if( ALU_Code < 8 )
			{
				DSP_ACCUME = SIGN_EXTEND20(ALU_Result) >> ShiftAmount;   /* Arithmetic */
			}
			else
			{
				DSP_ACCUME = (ALU_Result & 0xFFFFF) >> ShiftAmount;  /* Logical */
			}
		}
		else
		{
			ShiftAmount = BarrelCodeToShift[BarrelCode];

			if( ShiftAmount == 16 )
			{
			/* Clip result.  Saturation arithmetic. */
				DBUG(("DSP: CLIP\n"));
				if( CC_OVER )
				{
					DSP_ACCUME = ( CC_NEG ) ? 0x7FFFF : 0xFFF80000 ;
				}
				else
				{
					DSP_ACCUME = SIGN_EXTEND20(ALU_Result);
				}
			}
			else
			{
				DBUG(("DSP: Left Shift by %d\n", (int) ShiftAmount))
				DSP_ACCUME = SIGN_EXTEND20(ALU_Result) << ShiftAmount;
			}
		}
	}

	TRACE_EXEC(("DSP: ACCUME = 0x%x, Codes = %c%c%c%c%c\n", (int) DSP_ACCUME,
		((CC_CARRY) ? 'C' : 'c'),
		((CC_ZERO)  ? 'Z' : 'z'),
		((CC_OVER)  ? 'V' : 'v'),
		((CC_NEG)   ? 'N' : 'n'),
		((CC_EXACT) ? 'X' : 'x')
		));

/* Check to see if there is an assignment. */
	if( DSP_BRAIN.dbrn_WriteBackAddress >= 0 )
	{
		DSPI_WRITE( DSP_BRAIN.dbrn_WriteBackAddress, DSP_ACCUME>>4 );
		DSP_BRAIN.dbrn_WriteBackAddress = -1;
	}
	else if(DSP_BRAIN.dbrn_OpUseIndex < NumOps)
	{
		dspsWriteNextOperand(DSP_ACCUME>>4);
	}
}

/**********************************************************************
** dspsSuperSpecial - execute the Super Special control instruction
*/
static void dspsSuperSpecial( void )
{
	int32 Select;

	Select = (DSP_COMMAND >> 7) & 7;

	switch(Select)
	{
		case 0:   /* NOP */
			break;

		case 1:   /* BAC */
			DSP_PC = DSP_ACCUME >> 4;
			break;

		case 2:   /* not used */
			break;
		case 3:   /* not used */
			break;

		case 4:   /* RTS */
			DSP_PC = dspsPop();
			break;

		case 5:   /* OP_MASK */
			UNIMPLEMENTED("OP_MASK");
			break;
		case 6:   /* not used */
			break;

		case 7:   /* SLEEP */
			DSP_PC--;
			DBUG(("SLEEP! New PC = 0x%x\n", (int) DSP_PC));
			DSP_BRAIN.dbrn_Sleeping = TRUE;
			break;
	}
}

/**********************************************************************
** dspsExecControl - execute the control instruction in command register
*/
static void dspsExecControl( void )
{
	int32 Mode, Select, Mask, Target;
	int32 Val, DstAddr, PtrAddr;

	Mode = (DSP_COMMAND >> 13) & 3;
	Select = (DSP_COMMAND >> 12) & 1;
	Mask = (DSP_COMMAND >> 10) & 3;
	Target = (DSP_COMMAND >> 0) & 0x3FF;

	DBUG(("DSP: Mode = %d, Select = %d, Mask = %d\n",
		(int) Mode, (int) Select, (int) Mask ));

	switch (Mode)
	{
	case 0: /* special */
		switch ( (DSP_COMMAND >> 10) & 7 )
		{
		case 0:
			dspsSuperSpecial();
			break;

		case 1:   /* Jump */
			DSP_PC = DSP_COMMAND & 0x3FF;
			break;

		case 2:   /* JSR */
			dspsPush(DSP_PC);
			DSP_PC = DSP_COMMAND & 0x3FF;
			break;

		case 3:   /* BFM */
			UNIMPLEMENTED("BFM");
			break;

		case 4:   /* MOVEREG */
			{
				int32 RegDI = DSP_COMMAND & 0x1F;
				DstAddr = dspsTranslateReg( RegDI & 0xF );
				if( RegDI & 0x0010 ) /* Indirect? */
				{
					DstAddr = DSPI_READ( DstAddr );
				}
				dspsParseOperands(1);
				Val = dspsReadNextOperand();
				DSPI_WRITE( DstAddr, Val );
			}
			break;

		case 5:   /* M2 RBASE */
			{
				int32 WhichRBase = (DSP_COMMAND & 3) << 2;
				int32 RegAddr = DSP_COMMAND & 0x3FC;
				dspsSetRBase( WhichRBase, RegAddr );
			}
			break;

		case 6:   /* MOVE direct */
			DstAddr = DSP_COMMAND & 0x3FF;
			dspsParseOperands(1);
			Val = dspsReadNextOperand();
			DSPI_WRITE( DstAddr, Val );
			break;

		case 7:   /* MOVE indirect */
			PtrAddr = DSP_COMMAND & 0x3FF;
			DstAddr = DSPI_READ( PtrAddr );
			dspsParseOperands(1);
			Val = dspsReadNextOperand();
			DSPI_WRITE( DstAddr, Val );
			break;
		}
		break;

/* Mode 01,10 branch instructions */
	case 1:
	case 2:
		{
			int32 Flag0, Flag1;
			int32 Mask0, Mask1;
			int32 Test;

		/* Select which bits to look at. */
			if( Select == 0)
			{
				Flag0 = CC_NEG;
				Flag1 = CC_OVER;
			}
			else
			{
				Flag0 = CC_CARRY;
				Flag1 = CC_ZERO;
			}
		/* Extract Mask bits. */
			Mask0 = ((Mask & 2) != 0);
			Mask1 = ((Mask & 1) != 0);

			Test = (Flag0 || !Mask0) && (Flag1 || !Mask1);
			if( Mode == 2 ) Test = !Test;

		/* Perform branch */
			if( Test ) DSP_PC = Target;
		}
		break;

/* Complex branch instructions. */
	case 3:
		{
			int32 Type, Test=0;
			Type = (Select << 2) | Mask;
			switch( Type )
			{
			case 0:    /* BLT (N&-V)|(-N&V) */
				Test = (CC_NEG && !CC_OVER) || (!CC_NEG && CC_OVER);
				break;
			case 1:    /* BLE ((N&-V)|(-N&V))|Z */
				Test = (CC_NEG && !CC_OVER) || (!CC_NEG && CC_OVER) || CC_ZERO;
				break;
			case 2:    /* BGE (N&V)|(-N&-V) */
				Test = (CC_NEG && CC_OVER) || (!CC_NEG && !CC_OVER);
				break;
			case 3:    /* BGT ((N&V)|(-N&-V))&-Z */
				Test = ((CC_NEG && CC_OVER) || (!CC_NEG && !CC_OVER)) && !CC_ZERO;
				break;
			case 4:    /* BHI C & -Z */
				Test = (CC_CARRY && !CC_ZERO);
				break;
			case 5:    /* BLS -C | Z */
				Test = (!CC_CARRY || CC_ZERO);
				break;
			case 6:    /* BXS X */
				Test = CC_EXACT;
				break;
			case 7:    /* BXC -X */
				Test = !CC_EXACT;
				break;
			}

			/* Perform branch */
			if( Test ) DSP_PC = Target;
			break;
		}
		break;
	}
}
/**********************************************************************
** dspsGenMainInt - calculate combinatorial interrupt
*/
static uint32 dspsGenMainInt( void )
{
	uint32 MainInt;

	dspsCheckTimerInterrupt();

	MainInt = DSPRegs->dspr_MainInterruptPartial;
DBUG(("dspsGenMainInt: NextInt = 0x%08x, NextIntEnable = 0x%08x\n",
		DSPRegs->dspr_DMANextInterrupt, DSPRegs->dspr_DMANextIntEnable ));
	if( DSPRegs->dspr_DMANextInterrupt & DSPRegs->dspr_DMANextIntEnable )
	{
		MainInt |= DSPX_F_INT_DMANEXT;
	}

	if( DSPRegs->dspr_ConsumedInterrupt & DSPRegs->dspr_ConsumedIntEnable )
	{
		MainInt |= DSPX_F_INT_CONSUMED;
	}

	return MainInt;
}
/**********************************************************************
** dspsFDMA_Check - check to see if FDMA needs service
** Scan all FIFOs and return highest numbered channel that needs service,
** and is enabled.
** or return -1;
*/
int32 dspsFDMA_Check( void )
{
	int32 i;
	int32 Indx = -1;
	DSPI_FifoDma *fdma;
	uint32 Mask;

	for( i=DSPI_NUM_DMA_CHANNELS-1; i>=0; i-- )
	{
		Mask = (1L)<<i;

		if( Mask & DSPRegs->dspr_ChannelEnable & ~DSPRegs->dspr_ChannelComplete )
		{
			fdma = &DSP_FDMA(i);
/* Check direction */
			if( DSPRegs->dspr_ChannelDirection & Mask )
			{
			/* Write direction */
TRACE_FDMA(("dspsFDMA_Check: Output FIFO, Chan = %d, Depth = %d\n",
	(int) i, (int) fdma->fdma_Depth ));
				if( fdma->fdma_Depth >= 4 )
				{
					Indx = i;
					break;
				}
			}
			else
			{
			/* Read direction */
TRACE_FDMA(("dspsFDMA_Check: Input FIFO, Chan = %d, Depth = %d\n",
	(int) i, (int) fdma->fdma_Depth ));
				if( fdma->fdma_Depth <= 4 )
				{
					Indx = i;
					break;
				}
			}
		}
	}
	return Indx;
}

/**********************************************************************
** dspsWriteDMAtoFIFO - Write into FIFO from DMA
*/
static void dspsWriteDMAtoFIFO( int32 Channel, int32 Value )
{
	DSPI_FifoDma *fdma;
	fdma = &DSP_FDMA(Channel);
TRACE_FDMA(("dspsWriteDMAtoFIFO: Value = 0x%4x, Depth = %d\n",
	(int) Value, (int) fdma->fdma_Depth ));

	fdma->fdma_FIFO[fdma->fdma_DMA_Ptr] = Value & 0xFFFF;

	if( fdma->fdma_Depth < DSPI_FIFO_MAX_DEPTH )
	{
		fdma->fdma_DMA_Ptr = (fdma->fdma_DMA_Ptr + 1) & 7;
		fdma->fdma_Depth += 1;
	}
	else
	{
		UNIMPLEMENTED("dspsWriteDMAtoFIFO: FIFO Overflow");
	}
}

/**********************************************************************
** dspsWriteDSPPtoFIFO - Write into FIFO from DSPP
*/
static void dspsWriteDSPPtoFIFO( int32 Channel, int32 Value )
{

	DSPI_FifoDma *fdma;
	fdma = &DSP_FDMA(Channel);
TRACE_FDMA(("dspsWriteDSPPtoFIFO: Value = 0x%4x, Depth = %d\n",
	(int) Value, (int) fdma->fdma_Depth ));

	fdma->fdma_FIFO[fdma->fdma_DSPI_Ptr] = Value  & 0xFFFF;

	if( fdma->fdma_Depth < DSPI_FIFO_MAX_DEPTH )
	{
		fdma->fdma_DSPI_Ptr = (fdma->fdma_DSPI_Ptr + 1) & 7;
		fdma->fdma_Depth += 1;
	}
	else
	{
		UNIMPLEMENTED("dspsWriteDSPPtoFIFO: FIFO Overflow");
	}
}

/**********************************************************************
** dspsReadFIFOtoDMA - Read from FIFO to DMA
*/
int32 dspsReadFIFOtoDMA( int32 Channel )
{
	DSPI_FifoDma *fdma;
	int32 ReadVal;

	fdma = &DSP_FDMA(Channel);
	ReadVal = fdma->fdma_FIFO[fdma->fdma_DMA_Ptr];

	if( fdma->fdma_Depth > 0 )
	{
		fdma->fdma_DMA_Ptr = (fdma->fdma_DMA_Ptr + 1) & 7;
		fdma->fdma_Depth -= 1;
	}
	else
	{
		UNIMPLEMENTED("dspsReadFIFOtoDMA: FIFO Underflow");
	}
	return ReadVal;
}

/**********************************************************************
** dspsReadFIFOtoDSPP - Read from FIFO to DSPP
*/
int32 dspsReadFIFOtoDSPP( int32 Channel )
{
	DSPI_FifoDma *fdma;
	int32 ReadVal;

	fdma = &DSP_FDMA(Channel);
	if( fdma->fdma_Depth > 0 )
	{
		ReadVal = fdma->fdma_FIFO[fdma->fdma_DSPI_Ptr];
TRACE_FDMA(("dspsReadFIFOtoDSPP: Channel = 0x%x, ReadVal = 0x%08x\n", Channel, ReadVal ));
TRACE_FDMA(("dspsReadFIFOtoDSPP:     Ptr = 0x%x, Depth = 0x%08x\n", fdma->fdma_DSPI_Ptr, fdma->fdma_Depth ));
		fdma->fdma_DSPI_Ptr = (fdma->fdma_DSPI_Ptr + 1) & 7;
		fdma->fdma_Depth -= 1;

		if(fdma->fdma_Depth == 0)
		{
/* Set Consumed interrupt. */
TRACE_FDMA(("dspsReadFIFOtoDSPP: FIFO Empty. ChannelComplete = 0x%x\n",
	(int) DSPRegs->dspr_ChannelComplete ))
			if( DSPRegs->dspr_ChannelComplete & ((1L)<<Channel) )
			{
				DSPRegs->dspr_ConsumedInterrupt |= (1L)<<Channel;
			}
		}
		fdma->fdma_PreviousCurrent = ReadVal;
	}
	else
	{
		DSPRegs->dspr_UnderOverInterrupt |= (1L)<<Channel; /* !!! Is this right? */
		ReadVal = fdma->fdma_PreviousCurrent;
TRACE_FDMA(("FIFO Underflowed. Channel = 0x%x, ReadVal = 0x%08x\n", Channel, ReadVal ));
	}
	return ReadVal;
}

/**********************************************************
** DecodeSQXD
*/
int32 DecodeSQXD( int8 CodedVal, int32 Prev )
{
	int32 Expanded;
	int32 Output;

/* Expand 8 to 16 bits */
	{
		int32 TempVal = SIGN_EXTEND8(CodedVal & 0xFE);
		Expanded = (TempVal * ABS(TempVal) ) << 1;
	}

/* Use Delta or Exact method depending on LSB */
	if( CodedVal & 1 )
	{	/* LSB == 1 so use delta form */
/* Shift delta to improve resolution for small deltas. */
		Expanded = Expanded >> 2;
		Output = Expanded + Prev;
	}
	else
	{
		Output = Expanded;
	}

	Output = Output & 0xFFFF;

	DBUG(("0x%x = DecodeSQXD( CodedVal = 0x%x, Prev = 0x%x)\n",
		(int) Output, (int) CodedVal, (int) Prev  ));
	return Output ;
}

/**********************************************************************
** dspsProcessNextDMA - Handle count==0, maybe use NextAddr,NextCnt
*/
static void dspsProcessNextDMA( int32 Channel )
{
	DSPI_FifoDma *fdma;

	fdma = &DSP_FDMA(Channel);

/* Get NextAddress if needed. */
	if( fdma->fdma_CurrentCount <= 0 )
	{
TRACE_FDMA(("dspsProcessNextDMA: count == 0 on channel %d,  fdma->fdma_NextValid = %d\n", Channel, fdma->fdma_NextValid ));
		if( fdma->fdma_NextValid )
		{
			fdma->fdma_CurrentAddress = fdma->fdma_NextAddress;
			fdma->fdma_CurrentCount = fdma->fdma_NextCount;
TRACE_FDMA(("dspsProcessNextDMA: new CurrentAddress, CurrentCount  = 0x%x,0x%x\n", fdma->fdma_CurrentAddress, fdma->fdma_CurrentCount ));
			if( !fdma->fdma_GoForever )
			{
				fdma->fdma_NextValid = 0;
			}
/* Set Completion bit. */
			DSPRegs->dspr_ChannelComplete &= ~((1L)<<Channel);
#if (AF_BDA_PASS != 1)
/* Set interrupt bit when next read. */
			DSPRegs->dspr_DMANextInterrupt |= ((1L)<<Channel);
#endif
		}
		else
		{
/* Disable the channel so we don't keep servicing it. */
#if (AF_BDA_PASS == 1)
			DSPRegs->dspr_ChannelComplete |= ((1L)<<Channel);
#else
			DSPRegs->dspr_ChannelEnable &= ~((1L)<<Channel);
#endif
		}
#if (AF_BDA_PASS == 1)
/* Set interrupt bit when count == 0. */
		DSPRegs->dspr_DMANextInterrupt |= ((1L)<<Channel);
#endif
DBUG(("dspsProcessNextDMA: DSPRegs->dspr_DMANextInterrupt = 0x%08x\n", DSPRegs->dspr_DMANextInterrupt ));
	}
}

/**********************************************************************
** dspsServiceInputFDMA - Service Read FIFO.
*/
static void dspsServiceInputFDMA( int32 Channel )
{
	DSPI_FifoDma *fdma;
	int8  *DataPtr;
	int32 LocalCount;   /* How many samples to process. */
	int32 If8Bit;
	int32 IfSQXD;
	int32 CurSample = 0;
	int32 i;

	fdma = &DSP_FDMA(Channel);

TRACE_FDMA(("dspsServiceInputFDMA: Channel = %2d, Address = 0x%8x, Count = 0x%8x\n",
		(int) Channel, (int) fdma->fdma_CurrentAddress, (int) fdma->fdma_CurrentCount ));

	if(fdma->fdma_CurrentCount == 0)
	{
		return;
	}


	DataPtr = (int8 *) fdma->fdma_CurrentAddress;

/* Move a maximum of 4 samples per service. */
	LocalCount = fdma->fdma_CurrentCount;
	if( LocalCount > 4 ) LocalCount = 4;

/* Advance samples based on type. */
	If8Bit =  ( (DSPRegs->dspr_Channel8Bit & ((1L)<<Channel)) != 0 );
	IfSQXD =  ( (DSPRegs->dspr_ChannelSQXD & ((1L)<<Channel)) != 0 );

	if( If8Bit )
	{
		int8 CurByte;

/* Fetch data from memory, {decompress}, and write to FIFO. */
		for( i=0; i < LocalCount; i++ )
		{
			CurByte = *DataPtr++;
			if( IfSQXD )
			{
				CurSample = DecodeSQXD( CurByte, fdma->fdma_PreviousValue );
				fdma->fdma_PreviousValue = CurSample;
			}
			else
			{
				CurSample = ((int32)CurByte) << 8;
			}
			dspsWriteDMAtoFIFO(Channel, CurSample);
		}
	}
	else
	{

TRACE_FDMA(("dspsServiceInputFDMA: LocalCount = %2d, Count = 0x%8x\n",
		(int) LocalCount, (int) fdma->fdma_CurrentCount ));

		for( i=0; i < LocalCount; i++ )
		{
/* Do two succesive byte transfers to avoid int16 access. */
			CurSample =  ((int32)(*DataPtr++)) & 0xFF;
			CurSample = (CurSample << 8) | (((int32)(*DataPtr++)) & 0xFF);
			dspsWriteDMAtoFIFO(Channel, CurSample);
		}
	}

/* Write Updated Address and Count back to stack. */
	fdma->fdma_CurrentAddress = (uint32) DataPtr;
	fdma->fdma_CurrentCount -= LocalCount;

	dspsProcessNextDMA( Channel );
}

/**********************************************************************
** dspsServiceOutputFDMA - Service Output FIFO.
*/
static void dspsServiceOutputFDMA( int32 Channel )
{
	DSPI_FifoDma *fdma;
	int8  *DataPtr;
	int32 LocalCount;   /* How many samples to process. */
	int32 CurSample = 0;
	int32 i;

	fdma = &DSP_FDMA(Channel);

	if(fdma->fdma_CurrentCount == 0) return; /* %Q DMANFW */


TRACE_FDMA(("dspsServiceOutputFDMA: Channel = %2d, Address = 0x%8x, Count = 0x%8x\n",
		(int) Channel, (int) fdma->fdma_CurrentAddress, (int) fdma->fdma_CurrentCount ));

	DataPtr = (int8 *) fdma->fdma_CurrentAddress;

/* Move a maximum of 4 samples per service. */
	LocalCount = fdma->fdma_CurrentCount;
	if( LocalCount > 4 ) LocalCount = 4;

TRACE_FDMA(("dspsServiceOutputFDMA: LocalCount = %2d, Count = 0x%8x\n",
		(int) LocalCount, (int) fdma->fdma_CurrentCount ));

	for( i=0; i < LocalCount; i++ )
	{
		CurSample = dspsReadFIFOtoDMA(Channel);
/* Do two succesive byte transfers to avoid int16 access. */
		*DataPtr++ = (int8) (CurSample >> 8);  /* Write out MS Byte */
		*DataPtr++ = (int8) CurSample;         /* Write out LS Byte */
	}

/* Write Updated Address and Count back to stack. */
	fdma->fdma_CurrentAddress = (uint32) DataPtr;
	fdma->fdma_CurrentCount -= LocalCount;

	dspsProcessNextDMA( Channel );
}

/**********************************************************************
** dspsRunFDMA - update FDMA as needed
** Scan all FIFOs and service highest numbered channel that needs service.
*/
static void dspsRunFDMA( void )
{
	int32 Channel;

	Channel = dspsFDMA_Check();

	if( Channel >= 0 )
	{

		TRACE_FDMA(("dspsRunFDMA: 0x%x needs service.\n", (int) Channel ));

		if( DSPRegs->dspr_ChannelDirection & ((1L)<<Channel) )
		{
			dspsServiceOutputFDMA( Channel );
		}
		else
		{
			dspsServiceInputFDMA( Channel );
		}
	}
}

/**********************************************************************
** dspsRunOscillator - cycle hardware oscillator
*/
static void dspsRunOscillator( int32 Channel )
{
	DSPI_FifoDma *fdma;
	int32 Cnt, i;

	fdma = &DSP_FDMA(Channel);

/* Add Phase Increment */
	DSPRegs->dspr_Internal.dspi_Phase += DSPRegs->dspr_Internal.dspi_Frequency;
/* Extract two high bits to advance FIFO */
	Cnt = (DSPRegs->dspr_Internal.dspi_Phase >> 15) & 3;
/* Clip to positive phase */
	DSPRegs->dspr_Internal.dspi_Phase &= 0x7FFF;

/* Advance FIFO if data present. */
	if( Cnt > fdma->fdma_Depth ) Cnt = fdma->fdma_Depth;
	for( i=0; i<Cnt; i++ )
	{
		dspsReadFIFOtoDSPP( Channel );
	}

/* Return count to program for counting samples. */
	DSPRegs->dspr_Internal.dspi_LastOscCount = Cnt;

TRACE_FDMA(("dspsRunOscillator: Freq = 0x%x, Phase = 0x%x, Cnt = 0x%x\n",
		(int) DSPRegs->dspr_Internal.dspi_Frequency,
		(int) DSPRegs->dspr_Internal.dspi_Phase,
		(int) Cnt ));
}

/**********************************************************************
** dspsAccessFDMA - access FIFO/Oscillator
** Addr = DSPI address, 0 -> 0x3FF
*/
int32 dspsAccessFDMA( int32 Addr, int32 Val, int32 ReadWrite )
{
	int32 Channel, Part;
	DSPI_FifoDma *fdma;
	int32 ReadVal = -1;

#if AF_BDA_PASS == 1
	if( IN_RANGE(Addr, DSPI_FIFO_OSC_BASE, DSPI_FIFO_PHASE(DSPI_NUM_DMA_CHANNELS) ) )
	{
/* Oscillator */
		Channel = (Addr - DSPI_FIFO_OSC_BASE) / DSPI_FIFO_OSC_SIZE;
		Part = Addr & 3;
	}
	else if( IN_RANGE(Addr, DSPI_FIFO_BASE, DSPI_FIFO_CONTROL(DSPI_NUM_DMA_CHANNELS) ) )
	{
/* Bump/Status */
		Channel = (Addr - DSPI_FIFO_BASE) / DSPI_FIFO_SIZE;
		Part = (Addr & 1) + 4;  /* Make it look like Pass 2 */
	}
	else
	{
		ERR(("dspsAccessFDMA: invalid address = 0x%x\n", Addr ));
		return -1;
	}
#define TEMP_OFFSET_DATA    (DSPI_FIFO_OFFSET_DATA + 4)
#define TEMP_OFFSET_CONTROL (DSPI_FIFO_OFFSET_CONTROL + 4)
#else
	Channel = (Addr - DSPI_FIFO_OSC_BASE) / DSPI_FIFO_OSC_SIZE;
	Part = Addr & (DSPI_FIFO_OSC_SIZE-1);
#define TEMP_OFFSET_DATA    DSPI_FIFO_OFFSET_DATA
#define TEMP_OFFSET_CONTROL DSPI_FIFO_OFFSET_CONTROL
#endif
	fdma = &DSP_FDMA(Channel);

	switch(Part)
	{

		case DSPI_FIFO_OSC_OFFSET_CURRENT:
			if( fdma->fdma_Depth == 0 )
			{
				ReadVal = fdma->fdma_PreviousCurrent;
			}
			else
			{
				ReadVal = fdma->fdma_FIFO[fdma->fdma_DSPI_Ptr];
			}
			break;

		case DSPI_FIFO_OSC_OFFSET_NEXT:
			if( fdma->fdma_Depth == 0 )
			{
				ReadVal = fdma->fdma_PreviousCurrent;
			}
			else if( fdma->fdma_Depth == 1 )
			{
				ReadVal = fdma->fdma_FIFO[fdma->fdma_DSPI_Ptr];
			}
			else
			{
				ReadVal = fdma->fdma_FIFO[(fdma->fdma_DSPI_Ptr+1)&7];
			}
			break;

		case DSPI_FIFO_OSC_OFFSET_FREQUENCY:
			if(ReadWrite == DSPS_MODE_READ)
			{
				ReadVal = DSPRegs->dspr_Internal.dspi_LastOscCount;
			}
			else
			{
				DSPRegs->dspr_Internal.dspi_Frequency =  Val;
				dspsRunOscillator( Channel );
			}
			break;

		case DSPI_FIFO_OSC_OFFSET_PHASE:
			ReadVal = READWRITE16( &(DSPRegs->dspr_Internal.dspi_Phase),
				(int) Val);
			break;

		case TEMP_OFFSET_DATA:
			if( ReadWrite == DSPS_MODE_READ )
			{
				ReadVal = dspsReadFIFOtoDSPP( Channel );
			}
			else
			{

				dspsWriteDSPPtoFIFO( Channel, Val );
			}
			break;

		case TEMP_OFFSET_CONTROL:
			if( ReadWrite == DSPS_MODE_READ )
			{
				ReadVal = fdma->fdma_Depth;
			}
			else
			{

				UNIMPLEMENTED("Write to FIFO STATUS");
			}
			break;

	}
	return ReadVal;

#undef TEMP_OFFSET_DATA
#undef TEMP_OFFSET_CONTROL
}

/**********************************************************************
** dspsAccessDMA_Stack - access Contents of DMA Stack
** Addr = DSPX address
*/
int32 dspsAccessDMA_Stack( int32 Addr, int32 Val, int32 ReadWrite )
{
	DSPI_FifoDma *fdma;
	int32 Channel;
	int32 Part;
	int32 ReadVal = -1;

/* Write Addresses and Counts */
	if( IN_RANGE(Addr, DSPX_DMA_STACK, (DSPX_DMA_STACK_CONTROL-1) ) )
	{
		Channel = (Addr - ((int32)DSPX_DMA_STACK)) / DSPX_DMA_STACK_CHANNEL_SIZE;
		fdma = &DSP_FDMA(Channel);
		Part = (Addr / sizeof(uint32)) & 3;
TRACE_FDMA(("dspsAccessDMA_Stack: [DSPX_DMA_STACK]:%c Channel = 0x%2x, Part=%d, Val = 0x%x\n",
	((ReadWrite==DSPS_MODE_READ)?'R':'W'), (int) Channel, (int) Part, (int) Val  ));
		switch(Part)
		{
#if (AF_BDA_PASS == 1)
		case DSPX_DMA_ADDRESS_OFFSET:
			ReadVal = READWRITE32( &(fdma->fdma_CurrentAddress), Val);
			break;
		case DSPX_DMA_COUNT_OFFSET:
			ReadVal = READWRITE32( &(fdma->fdma_CurrentCount), Val);
			break;
#else
		case DSPX_DMA_ADDRESS_OFFSET:
			if( ReadWrite == DSPS_MODE_READ )
			{
				ReadVal = fdma->fdma_CurrentAddress;
			}
			else
			{
				DSPRegs->dspr_ShadowCurrentAddress = Val;
			}
			break;

		case DSPX_DMA_COUNT_OFFSET:
			if( ReadWrite == DSPS_MODE_READ )
			{
				ReadVal = fdma->fdma_CurrentCount;
			}
			else
			{
				DSPRegs->dspr_ShadowCurrentCount = Val;
			}
			break;
#endif

		case DSPX_DMA_NEXT_ADDRESS_OFFSET:
			if( ReadWrite == DSPS_MODE_READ )
			{
				ReadVal = fdma->fdma_NextAddress;
			}
			else
			{
				DSPRegs->dspr_ShadowNextAddress = Val;
			}
			break;

		case DSPX_DMA_NEXT_COUNT_OFFSET:
			if( ReadWrite == DSPS_MODE_READ )
			{
				ReadVal = fdma->fdma_NextCount;
			}
			else
			{
				DSPRegs->dspr_ShadowNextCount = Val;
			}
			break;
		}
	}
/* DMA Control register */
#if (AF_BDA_PASS == 1)
	else if( IN_RANGE(Addr, DSPX_DMA_STACK_CONTROL,
		DSPX_DMA_STACK_CONTROL + (DSPI_NUM_DMA_CHANNELS * sizeof(uint32))  ) )
	{

		Channel = (Addr - (int32)DSPX_DMA_STACK_CONTROL) / sizeof(uint32);
#else
	else if( IN_RANGE(Addr, DSPX_DMA_STACK_CONTROL,
		DSPX_DMA_STACK_CONTROL + (DSPI_NUM_DMA_CHANNELS * DSPX_DMA_STACK_CONTROL_SIZE)  ) )
	{

		Channel = (Addr - (int32)DSPX_DMA_STACK_CONTROL) / DSPX_DMA_STACK_CONTROL_SIZE;
#endif
		fdma = &DSP_FDMA(Channel);
		if( ReadWrite == DSPS_MODE_READ )
		{
			ReadVal = (fdma->fdma_GoForever) ?  DSPX_F_DMA_GO_FOREVER : 0;
			ReadVal |= (fdma->fdma_NextValid) ?  DSPX_F_DMA_NEXTVALID : 0;
#if (AF_BDA_PASS == 1)
/* Only readable in Pass 1. */
			ReadVal |= ((DSPRegs->dspr_DMANextIntEnable & ((1L)<<Channel))) ?  DSPX_F_INT_DMANEXT_EN : 0;
#endif

TRACE_FDMA(("dspsAccessDMA_Stack: [DSPX_DMA_STACK_CONTROL] read 0x%x from Channel = 0x%2x\n", (int) ReadVal, (int) Channel ));
		}
		else
		{
TRACE_FDMA(("dspsAccessDMA_Stack: [DSPX_DMA_STACK_CONTROL] write 0x%x to Channel = 0x%2x\n", (int) Val, (int) Channel ));
			if( Val & DSPX_F_SHADOW_SET_NEXTVALID )
			{
				fdma->fdma_NextValid = ((Val & DSPX_F_DMA_NEXTVALID) != 0);
TRACE_FDMA(("dspsAccessDMA_Stack: Set NextValid for channel 0x%x to %d\n", (int) Channel, (int) fdma->fdma_NextValid ));
			}
			if( Val & DSPX_F_SHADOW_SET_FOREVER )
			{
				fdma->fdma_GoForever = (Val & DSPX_F_DMA_GO_FOREVER) != 0;
			}
			if( Val & DSPX_F_SHADOW_SET_DMANEXT )
			{
				if (Val & DSPX_F_INT_DMANEXT_EN)
				{
					DSPRegs->dspr_DMANextIntEnable |= ((1L)<<Channel);
				}
				else
				{
					DSPRegs->dspr_DMANextIntEnable &= ~((1L)<<Channel);
				}
			}


#if (AF_BDA_PASS == 1)
			if( Val & DSPX_F_SHADOW_SET_ADDRESS )
			{
				fdma->fdma_NextAddress = DSPRegs->dspr_ShadowNextAddress;
			}
			if( Val & DSPX_F_SHADOW_SET_COUNT )
			{
				fdma->fdma_NextCount = DSPRegs->dspr_ShadowNextCount;
			}
#else
			if( Val & DSPX_F_SHADOW_SET_ADDRESS_COUNT )
			{
				if( Addr & 8 )
				{
					fdma->fdma_NextAddress = DSPRegs->dspr_ShadowNextAddress;
					fdma->fdma_NextCount = DSPRegs->dspr_ShadowNextCount;
TRACE_FDMA(("dspsProcessNextDMA: new NextAddress, NextCount  = 0x%x,0x%x\n", fdma->fdma_CurrentAddress, fdma->fdma_CurrentCount ));
				}
				else
				{
					fdma->fdma_CurrentAddress = DSPRegs->dspr_ShadowCurrentAddress;
					fdma->fdma_CurrentCount = DSPRegs->dspr_ShadowCurrentCount;
TRACE_FDMA(("dspsProcessNextDMA: new CurrentAddress, CurrentCount  = 0x%x,0x%x\n", fdma->fdma_CurrentAddress, fdma->fdma_CurrentCount ));
				}
			}
#endif
		}
	}
	else
	{
		ILLEGAL_ADDRESS(Addr);
	}
	return ReadVal;
}

/**********************************************************************
** dspsAccessInternal - access memory internal to DSPP
** Addr = DSPI address, 0 -> 0x3FF
*/
int32 dspsAccessInternal( int32 Addr, int32 Val, int32 ReadWrite )
{
	int32 ReadVal = 0;

	TRACE_VERBOSE(("dspsAccessInternal: DSPI_Addr = 0x%x\n", (int)Addr ));
	if( IN_RANGE(Addr, 0, 0x2FF) ) /* !!! too high */
	{
		DBUG(("DSP access DATA memory: 0x%x, 0x%x, RW=%d\n",
			(int) Addr, (int) Val, (int) ReadWrite ));
		ReadVal = READWRITE16( &(DSPRegs->dspr_Internal.dspi_DataMemory[Addr]), (int) Val );
	}
#if AF_BDA_PASS == 1
	else if( IN_RANGE(Addr, DSPI_FIFO_OSC_BASE, DSPI_FIFO_CONTROL(DSPI_NUM_DMA_CHANNELS) ) )
#else
	else if( (Addr & 0x7) < DSPI_FIFO_OSC_NUM_REGS )
#endif
	{
		ReadVal = dspsAccessFDMA( Addr, Val, ReadWrite );
	}
	else if( IN_RANGE(Addr, DSPI_INPUT0, DSPI_INPUT1 ) )
	{
		if(ReadWrite == DSPS_MODE_READ)
		{
			int32 i;
			i = Addr - DSPI_INPUT0;
			ReadVal = DSPRegs->dspr_Internal.dspi_Inputs[i];
		}
	}
	else if( IN_RANGE(Addr, DSPI_OUTPUT0, DSPI_OUTPUT7 ) )
	{
		if(ReadWrite == DSPS_MODE_WRITE)
		{
			int32 i;
			i = Addr - DSPI_OUTPUT0;
			DSPRegs->dspr_Internal.dspi_Outputs[i] = Val;
		}
	}
	else
	{
		switch(Addr)
		{
		case DSPI_SIM_TRACE:
PRT(("dspsAccessInternal: access DSPI_SIM_TRACE %d\n", Val ));
			ReadVal = READWRITE16( &(DSPRegs->dspr_TraceMask), (int) Val);
			break;

		case DSPI_SIM_ADIO_FILE:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				dspsControlAudioOutputFile( Val );
			}
			break;

		case DSPI_OUTPUT_CONTROL:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				if( Val & 1) dspsAudioFrameToFIFO( );
			}
			break;

		case DSPI_OUTPUT_STATUS:
			if(ReadWrite == DSPS_MODE_READ)
			{
				int32 t;
				t = DSPRegs->dspr_Internal.dspi_OutputWritePtr -
					DSPRegs->dspr_Internal.dspi_OutputReadPtr;
				ReadVal = t & (DSPI_OUTPUT_FIFO_MAX_DEPTH - 1);
				DBUG(("Output Status = 0x%x\n", (int)ReadVal));
			}
			break;

		case DSPI_INPUT_CONTROL:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				UNIMPLEMENTED("DSPI_INPUT_CONTROL");
			}
			break;

		case DSPI_INPUT_STATUS:
			UNIMPLEMENTED("DSPI_INPUT_STATUS");
			ReadVal = 1;
			break;

		case DSPI_AUDLOCK:
			ReadVal = READWRITE16( &(DSP_BRAIN.dbrn_AudLock), (int) Val  );
			break;

		case DSPI_CLOCK:
			ReadVal = READWRITE16( &(DSP_CLOCK), (int) Val );
			break;

		case DSPI_NOISE:
			ReadVal = RANDOM & ((uint32)0x0000FFFF);
			DBUG(( "NOISE = 0x%x\n", (int) ReadVal ));
			break;

#if AF_BDA_PASS == 1
		case DSPI_CPU_INT0:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				DSPRegs->dspr_MainInterruptPartial |= DSPX_F_INT_SOFT0;
			}
			break;
		case DSPI_CPU_INT1:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				DSPRegs->dspr_MainInterruptPartial |= DSPX_F_INT_SOFT1;
			}
			break;
#else /* AF_BDA_PASS >= 2 */
		case DSPI_CPU_INT:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				DSPRegs->dspr_MainInterruptPartial |= (Val << DSPX_FLD_INT_SOFT_SHIFT) & DSPX_FLD_INT_SOFT_MASK;
			}
			break;
#endif /* AF_BDA_PASS */

		default:
			ILLEGAL_ADDRESS(Addr);
			ReadVal = -1;
			break;
		}
	}

	return ReadVal;
}

/**********************************************************************
** dspsAccessData - access memory internal to DSPP by DSPP
** Addr = DSPI address, 0 -> 0x3FF
*/
int32 dspsAccessData( int32 Addr, int32 Val, int32 ReadWrite )
{
	const int32 ReadVal = dspsAccessInternal( Addr, Val, ReadWrite );

	if( ReadWrite == DSPS_MODE_READ )
	{
		TRACE_EXEC(("DSP: READ from address 0x%4x, Value = 0x%4x\n", (int) Addr, (int) ReadVal ));
	}
	else
	{
		TRACE_EXEC(("DSP: WRITE to  address 0x%4x, Value = 0x%4x\n", (int) Addr, (int) Val ));
	}

	return ReadVal;
}

#define READ_OR_CLEAR(var) \
	if(ReadWrite == DSPS_MODE_WRITE) var &= ~Val; \
	else return var;

#define READ_OR_SET(var) \
	if(ReadWrite == DSPS_MODE_WRITE) var |= Val; \
	else return var;

/**********************************************************************
** dspsAccessHardware - read or write any DSPP address
*/
int32 dspsAccessHardware( int32 Addr, int32 Val, int32 ReadWrite )
{
	int32 Indx;

	if( IN_RANGE(Addr, DSPX_CODE_MEMORY_LOW, DSPX_CODE_MEMORY_HIGH) )
	{
		Indx = (Addr - (int32)DSPX_CODE_MEMORY_LOW) / 4;
		DBUG(("dspsAccessHardware: DSP Code[0x%x] = 0x%x\n", (int) Indx, (int) DSPRegs->dspr_CodeMemory[Indx] ));
		return READWRITE16( &(DSPRegs->dspr_CodeMemory[Indx]), Val );
	}
	else if( IN_RANGE(Addr, DSPX_DATA_MEMORY_LOW, DSPX_DATA_MEMORY_HIGH) )
	{
		Indx = (Addr - (int32)DSPX_DATA_MEMORY_LOW) / 4;
		return dspsAccessInternal( Indx, Val, ReadWrite );
	}
	else if( IN_RANGE(Addr, DSPX_DMA_STACK, DSPX_DMA_STACK + 0x0FFF ) )
	{
		return dspsAccessDMA_Stack( Addr, Val, ReadWrite );
	}
	else
	{
/* Decode miscellaneous DSPX addresses. */
		switch (Addr)
		{
		case (int32) DSPX_INTERRUPT_SET:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				DSPRegs->dspr_MainInterruptPartial |= (Val & ~DSPX_F_INT_ALL_DMA);
			}
			else
			{
				return dspsGenMainInt();
			}
			break;

		case (int32) DSPX_INTERRUPT_CLR:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				DSPRegs->dspr_MainInterruptPartial &= ~(Val & ~DSPX_F_INT_ALL_DMA);
			}
			else
			{
				return dspsGenMainInt();
			}
			break;

		case (int32) DSPX_INTERRUPT_ENABLE:
			READ_OR_SET(DSPRegs->dspr_MainIntEnable);
			break;
		case (int32) DSPX_INTERRUPT_DISABLE:
			READ_OR_CLEAR(DSPRegs->dspr_MainIntEnable);
			break;

		case (int32) DSPX_INT_DMANEXT_SET:
			TRACE_RW(("DSPX_INT_DMANEXT_SET\n"));
			READ_OR_SET(DSPRegs->dspr_DMANextInterrupt);
			break;
		case (int32) DSPX_INT_DMANEXT_CLR:
			TRACE_RW(("DSPX_INT_DMANEXT_CLR\n"));
			READ_OR_CLEAR(DSPRegs->dspr_DMANextInterrupt);
			break;
#if (AF_BDA_PASS > 1)
		case (int32) DSPX_INT_DMANEXT_ENABLE:
			READ_OR_SET(DSPRegs->dspr_DMANextIntEnable);
			break;
#endif

		case (int32) DSPX_INT_CONSUMED_SET:
			READ_OR_SET(DSPRegs->dspr_ConsumedInterrupt);
			break;
		case (int32) DSPX_INT_CONSUMED_CLR:
			READ_OR_CLEAR(DSPRegs->dspr_ConsumedInterrupt);
			break;
		case (int32) DSPX_INT_CONSUMED_ENABLE:
			READ_OR_SET(DSPRegs->dspr_ConsumedIntEnable);
			break;
		case (int32) DSPX_INT_CONSUMED_DISABLE:
			READ_OR_CLEAR(DSPRegs->dspr_ConsumedIntEnable);
			break;

#if (AF_BDA_PASS == 1)
		case (int32) DSPX_INT_DMALATE_SET:
			READ_OR_SET(DSPRegs->dspr_DMALateInterrupt);
			break;
		case (int32) DSPX_INT_DMALATE_CLR:
			READ_OR_CLEAR(DSPRegs->dspr_DMALateInterrupt);
			break;
		case (int32) DSPX_INT_DMALATE_ENABLE:
			READ_OR_SET(DSPRegs->dspr_DMALateIntEnable);
			break;
		case (int32) DSPX_INT_DMALATE_DISABLE:
			READ_OR_CLEAR(DSPRegs->dspr_DMALateIntEnable);
			break;
#endif /* AF_BDA_PASS == 1 */

		case (int32) DSPX_INT_UNDEROVER_SET:
			READ_OR_SET(DSPRegs->dspr_UnderOverInterrupt);
			break;
		case (int32) DSPX_INT_UNDEROVER_CLR:
			READ_OR_CLEAR(DSPRegs->dspr_UnderOverInterrupt);
			break;
		case (int32) DSPX_INT_UNDEROVER_ENABLE:
			READ_OR_SET(DSPRegs->dspr_UnderOverIntEnable);
			break;
		case (int32) DSPX_INT_UNDEROVER_DISABLE:
			READ_OR_CLEAR(DSPRegs->dspr_UnderOverIntEnable);
			break;

		case (int32) DSPX_CHANNEL_ENABLE:
			TRACE_RW(("DSPX_CHANNEL_ENABLE\n"));
			READ_OR_SET(DSPRegs->dspr_ChannelEnable);
			break;
		case (int32) DSPX_CHANNEL_DISABLE:
			TRACE_RW(("DSPX_CHANNEL_DISABLE\n"));
			READ_OR_CLEAR(DSPRegs->dspr_ChannelEnable);
			break;

		case (int32) DSPX_CHANNEL_DIRECTION_SET:
			READ_OR_SET(DSPRegs->dspr_ChannelDirection);
			break;
		case (int32) DSPX_CHANNEL_DIRECTION_CLR:
			READ_OR_CLEAR(DSPRegs->dspr_ChannelDirection);
			break;

		case (int32) DSPX_CHANNEL_8BIT_SET:
			READ_OR_SET(DSPRegs->dspr_Channel8Bit);
			break;
		case (int32) DSPX_CHANNEL_8BIT_CLR:
			READ_OR_CLEAR(DSPRegs->dspr_Channel8Bit);
			break;

		case (int32) DSPX_CHANNEL_SQXD_SET:
			READ_OR_SET(DSPRegs->dspr_ChannelSQXD);
			break;
		case (int32) DSPX_CHANNEL_SQXD_CLR:
			READ_OR_CLEAR(DSPRegs->dspr_ChannelSQXD);
			break;

		case (int32) DSPX_CHANNEL_RESET:
			TRACE_RW(("DSPX_CHANNEL_RESET\n"));
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				dspsResetChannels(Val);
			}
			else
			{
				ERR(("DSPX_CHANNEL_RESET is write only.\n"));
			}
			break;

		case (int32) DSPX_CHANNEL_STATUS:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				ERR(("DSPX_CHANNEL_STATUS is write only.\n"));
			}
			else
			{
				return (DSPRegs->dspr_ChannelComplete);
			}
			break;

		case (int32) DSPX_CONTROL:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				DSP_BRAIN.dbrn_GWilling = ((Val & DSPX_F_GWILLING) != 0);
DBUG(("Gwilling set to %d\n", DSP_BRAIN.dbrn_GWilling ));
			}
			else
			{
				return (DSP_BRAIN.dbrn_GWilling); /* %Q Incomplete, STEP */
			}
			break;

		case (int32) DSPX_RESET:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				if( Val & DSPX_F_RESET_DSPP) dspsResetDSPP();
				/* %Q reset FIFOs too */
			}
			break;


#if AF_BDA_PASS == 1
		case (int32) DSPX_AUDIO_DURATION:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				DSPRegs->dspr_AudioDuration = Val;
DBUG(("Write AudioDuration = 0x%x\n", (int) DSPRegs->dspr_AudioDuration ));
#if (AF_BDA_PASS == 1)
 /* Hardware doesn't do this but it speeds up simulation. */
				DSPRegs->dspr_AudioFrameCounter = (Val & 0xFFFF);
#endif
			}
			else
			{
DBUG(("Read AudioDuration = 0x%x\n", (int) DSPRegs->dspr_AudioDuration ));
				return (DSPRegs->dspr_AudioDuration);
			}
			break;

		case (int32) DSPX_AUDIO_TIME:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				DSPRegs->dspr_AudioTime = Val;
DBUG(("Write AudioTime = 0x%x\n", (int) DSPRegs->dspr_AudioTime ));
			}
			else
			{
DBUG(("Read AudioTime = 0x%x\n", (int) DSPRegs->dspr_AudioTime ));
				return (DSPRegs->dspr_AudioTime);
			}
			break;

		case (int32) DSPX_WAKEUP_TIME:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				DSPRegs->dspr_WakeupTime = Val;
DBUG(("Write WakeupTime = 0x%x\n", (int) DSPRegs->dspr_WakeupTime ));
			}
			else
			{
DBUG(("Read WakeupTime = 0x%x\n", (int) DSPRegs->dspr_WakeupTime ));
				return (DSPRegs->dspr_WakeupTime);
			}
			break;
#else /* AF_BDA_PASS */


		case (int32) DSPX_FRAME_DOWN_COUNTER:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				DSPRegs->dspr_AudioDuration = Val;
DBUG(("Write AudioDuration = 0x%x\n", (int) DSPRegs->dspr_AudioDuration ));
			}
			else
			{
DBUG(("Read AudioDuration = 0x%x\n", (int) DSPRegs->dspr_AudioDuration ));
				return (DSPRegs->dspr_AudioDuration);
			}
			break;

		case (int32) DSPX_FRAME_UP_COUNTER:
			if(ReadWrite == DSPS_MODE_WRITE)
			{
				DSPRegs->dspr_AudioTime = Val;
DBUG(("Write AudioTime = 0x%x\n", (int) DSPRegs->dspr_AudioTime ));
			}
			else
			{
DBUG(("Read AudioTime = 0x%x\n", (int) DSPRegs->dspr_AudioTime ));
				return (DSPRegs->dspr_AudioTime);
			}
			break;
#endif /* AF_BDA_PASS */

		case (int32) AUDIO_CONFIG:
			UNIMPLEMENTED("AUDIO_CONFIG");
			break;
		case (int32) AUDIN_CONFIG:
			UNIMPLEMENTED("AUDIN_CONFIG");
			break;
		case (int32) AUDOUT_CONFIG:
			UNIMPLEMENTED("AUDOUT_CONFIG");
			break;

		case (int32) DSPX_ACC:
			if(ReadWrite == DSPS_MODE_READ)
			{
				uint32 Bits = 0;
				Bits |= ((uint32) DSP_BRAIN.dbrn_Sleeping) << 30;
				Bits |= (DSP_ACCUME & 0xFFFFF);
				 /* %Q Incomplete */
				return (int32) Bits;
			}
			break;

		default:
			ERR(("Illegal address = 0x%x\n", (int) Addr));
			return -1;
		}
	}
	return 0;
}

void CheckCode( char *Msg )
{

	int i;
	for( i=0; i<0x40; i++ )
	{
		if( (DSP_CODE[i] & 0xFFFF0000) != 0)
		{
		ERR(("\nClobbered code =  0x%x at 0x%x\n", (int) DSP_CODE[i], (int) i ));
		ERR(("Msg = %s\n", Msg ));
		exit(-1);
		}
	}
}

/**********************************************************************
** dspsRunSimulator - run DSPP processor one instruction cycle
*/
static void dspsRunSimulator( void )
{
/* Run DMA engines. */
	dspsRunFDMA();
#ifdef PARANOID
	CheckCode("After dspsRunFDMA");
#endif

/* Don't execute DSPP unless GWILLING */
	TRACE_VERBOSE(("\ndspsRunSimulator: GWilling =  0x%x\n", (int) DSP_BRAIN.dbrn_GWilling ));
	if( DSP_BRAIN.dbrn_GWilling == 0 ) return;

/* Reset write back. */
	DSP_BRAIN.dbrn_WriteBackAddress = -1;

/* Load instruction. */
	DSP_COMMAND = DSP_CODE[DSP_PC];
	TRACE_EXEC(("\nDSP: Execute 0x%x at 0x%x\n", (int) DSP_COMMAND, (int) DSP_PC ));
	dspsAdvancePC();

/* Is it an arithmetic or control opcode? */
	if( DSP_COMMAND & 0x8000 )
	{
		dspsExecControl();
	}
	else
	{
		dspsExecArithmetic();
	}

/* Make sure we don't recurse endlessly. */
	if( !gIfServicingInterrupt )
	{
		uint32 MainInt = dspsGenMainInt();
/* Call interrupt handler if interrupts set. */
DBUG(("dspsRunSimulator: MainInt = 0x%08x, 0x%08x\n", MainInt, DSPRegs->dspr_MainIntEnable ));
		if( MainInt & DSPRegs->dspr_MainIntEnable )
		{
			gIfServicingInterrupt = TRUE;
		/*	DSPRegs->dspr_TraceMask = DSPIB_TRACE_FDMA | DSPIB_TRACE_RW; */
			(*gIntHandlerFunc)();
		/*	DSPRegs->dspr_TraceMask = 0; */
			gIfServicingInterrupt = FALSE;
		}
	}
}

void dspsInstallIntHandler( int32 (*HandlerFunc)( void ) )
{
	gIntHandlerFunc = HandlerFunc;
}

/**********************************************************************
** All access to hardware through these routines.
*/
void WriteHardware( vuint32 *Address, uint32 Val )
{
	if(DSPRegs == NULL)
	{
		if(dspsInitSimulator() < 0) return;
	}
	TRACE_RW(("WriteHardware( Addr = 0x%x, Val = 0x%x)\n", (int)Address, (int) Val));
	dspsAccessHardware( (int32) Address, Val, DSPS_MODE_WRITE );
	dspsRunSimulator();
}

uint32 ReadHardware( vuint32 *Address )
{
	int32 Val;

	if(DSPRegs == NULL)
	{
		if(dspsInitSimulator() < 0) return -1;
	}
	dspsRunSimulator();
	Val = dspsAccessHardware( (int32) Address, 0, DSPS_MODE_READ );
	TRACE_RW(("0x%x = ReadHardware( Address = 0x%x )\n", (int) Val, (int)Address));

	return Val;
}

#else

/* keep the compiler happy... */
extern int foo;

#endif /* } SIMULATE_DSPP */
