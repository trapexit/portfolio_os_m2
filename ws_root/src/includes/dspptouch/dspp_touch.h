#ifndef __DSPPTOUCH_DSPP_TOUCH_H
#define __DSPPTOUCH_DSPP_TOUCH_H


/******************************************************************************
**
**  @(#) dspp_touch.h 96/07/03 1.23
**  $Id: dspp_touch.h,v 1.13 1995/03/09 00:49:09 peabody Exp phil $
**
**  DSP Includes for low level hardware driver
**  This is included by clients of the library.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950113 WJB  Added this header.
**  950113 WJB  Added note about redundant DSPP opcode definitions.
**  950130 WJB  Removed redundant opcode definitions.
**  950208 WJB  Moved available resource definitions to dspp_resources.h.
**  950301 WJB  Added some triple bangs.
**  950307 PLB  Moved device specific code to dspp_touch_{anvil,bulldog}.h
**  950308 WJB  Cleaned up.
**  950420 WJB  Moved dsph...() functions from dspp.h.
**  950420 WJB  Removed EI_ constants. No longer used.
**  950424 WJB  Moved dsphWriteEIMem() prototype to dspp_touch_anvil.h to balance dspp_touch_bulldog.h.
**  950424 WJB  Moved magic opera EO_ constants to dspp_addresses.h.
**  950509 WJB  Made dsphWriteCodeMem() a macro unless PARANOID is defined.
**  950803 WJB  Turned Read/Write16() into macros.
**  960603 WJB  Added Own/DisownDSPP().
**
**  Initials:
**
**  PLB: Phil Burk (phil)
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <dspptouch/dspp_addresses.h>       /* DSPP addresses (DSPX_) */
#include <kernel/types.h>
#include <dspptouch/dspp_touch_bulldog.h>


/* -------------------- Constants */

#define INPUT_FIFO  (0)
#define OUTPUT_FIFO (1)


/* -------------------- Globals */

/* -------------------- Functions */

    /* exclusive access */
Item OwnDSPP (Err busyErrCode);
#define DisownDSPP(dsppLock) DeleteItem(dsppLock)

    /* misc */
int32  dsphInitDSPP( void );
int32  dsphTermDSPP( void );
int8  *dsphReadChannelAddress( int32 Channel );
uint32 dsphConvertChannelToInterrupt( int32 DMAChan );
void   dsphDisableDMA( int32 DMAChan );
void   dsphEnableDMA( int32 DMAChan );
void   dsphHalt( void );
void   dsphReset( void );
void   dsphResetAllFIFOs( void );
void   dsphClearInputFIFO( int32 DMAChan );
void   dsphResetFIFO( int32 DMAChan );
void   dsphSetDMANext (int32 DMAChan, AudioGrain *NextAddr, int32 NextCnt);
void   dsphSetFullDMA (int32 DMAChan, AudioGrain *Addr, int32 Cnt, AudioGrain *NextAddr, int32 NextCnt );
void   dsphStart( void );
void   dsphDisableChannelInterrupt( int32 DMNAChan );
void   dsphEnableChannelInterrupt( int32 DMNAChan );
void   dsphDownloadCode( const uint16 *Image, int32 Entry, int32 CodeSize);
int32  dsphGetFIFOPartAddress( int32 channel, int32 part );

	/* Write code memory (normally macro, function when PARANOID is on) */
#ifdef PARANOID
	void dsphWriteCodeMem( int32 CodeAddr, int32 Value );
#else
    /* @@@ this macro must evaluate its args precisely once (some callers use postinc) */
	#define dsphWriteCodeMem(DSPI_Addr,Value) WriteHardware (DSPX_CODE_MEMORY + (DSPI_Addr), (Value))
#endif


/*****************************************************************************/

#endif /* __DSPPTOUCH_DSPP_TOUCH_H */
