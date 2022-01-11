#ifndef __DSPPTOUCH_DSPP_TOUCH_BULLDOG_H
#define __DSPPTOUCH_DSPP_TOUCH_BULLDOG_H


/******************************************************************************
**
**  @(#) dspp_touch_bulldog.h 95/12/04 1.20
**  $Id: dspp_touch_bulldog.h,v 1.4 1995/03/09 08:09:47 phil Exp phil $
**
**  DSP Includes for low level hardware driver for M2
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950308 PLB  Created from dspp_touch.h.
**  950308 WJB  Cleaned up.
**  950419 WJB  Added prototype for dsphTraceSimulator().
**  950419 WJB  Added dsphSetChannelDirection() prototype.
**  950424 WJB  Added dsphInitInstrumentation() prototype.
**  950509 WJB  Made dsphRead/WriteDataMem() be just macros unless PARANOID is defined.
**  950510 WJB  Made dsphTraceSimulator() a macro.
**  950809 WJB  Added dsphReadCodeMem().
**
**  Initials:
**
**  PLB: Phil Burk (phil)
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <dspptouch/dspp_addresses.h>   /* DSPX_ */
#include <dspptouch/touch_hardware.h>   /* Read/WriteHardware() */
#include <kernel/types.h>

/* Used with DMA control routines. */
#define DSPH_F_DMA_LOOP          (1)  /* goes forever, otherwise single play */
#define DSPH_F_DMA_INT_ENABLE    (2)  /* enable DMANext interrupt */
#define DSPH_F_DMA_INT_DISABLE   (4)  /* disable DMANext interrupt */

/* -------------------- Functions */

	/* New M2 functions */
int32 dsphGetChannelDirection( int32 Channel );
void  dsphEnableADIO( void );
void  dsphDisableADIO( void );
int32 dsphReadDataMem( int32 DSPI_Addr );
void  dsphWriteDataMem( int32 DSPI_Addr, int32 Value );
void  dsphSetChannelDecompression( int32 Channel, int32 IfSQXD );
void  dsphSetChannel8Bit( int32 Channel, int32 If8Bit );
void  dsphSetChannelDirection( int32 Channel, int32 Direction );

void dsphSetNextDMA (int32 DMAChan, AudioGrain *NextAddr, int32 NextCnt, uint32 Flags);
void dsphSetInitialDMA (int32 DMAChan, AudioGrain *Addr, int32 Cnt );

	/* read/write data memory (normally macros, functions when PARANOID is on) */
#ifdef PARANOID
	int32 dsphReadDataMem( int32 DSPI_Addr );
	void dsphWriteDataMem( int32 DSPI_Addr, int32 Value );
#else
	/* @@@ these macros must evaluate their args precisely once (some callers use postinc) */
	#define dsphReadDataMem(DSPI_Addr)          ( (int32) (int16) ReadHardware (DSPX_DATA_MEMORY + (DSPI_Addr)) )
	#define dsphWriteDataMem(DSPI_Addr,Value)   WriteHardware (DSPX_DATA_MEMORY + (DSPI_Addr), (Value))
#endif

    /* read code memory */
#define dsphReadCodeMem(DSPI_Addr) ReadHardware (DSPX_CODE_MEMORY + (DSPI_Addr))


	/* Simulator stuff */
#ifdef SIMULATE_DSPP
	#define dsphTraceSimulator(Mask) dsphWriteDataMem( DSPI_SIM_TRACE, (Mask) )
#endif



/*****************************************************************************/

#endif /* __DSPPTOUCH_DSPP_TOUCH_BULLDOG_H */
