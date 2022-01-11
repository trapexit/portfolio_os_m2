#ifndef __DSPPTOUCH_DSPP_ADDRESSES_H
#define __DSPPTOUCH_DSPP_ADDRESSES_H

/******************************************************************************
**
**  @(#) dspp_addresses.h 96/10/16 1.33
**  $Id: dspp_addresses.h,v 1.35 1995/03/17 23:36:03 peabody Exp phil $
**
**  DSPP Addresses for M2 Pass 1 and Pass 2 ASICs.
**
**  By: Phil Burk
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950410 WJB  Restored header.
**  950411 WJB  Reduced M2 data memory to 0x2e0 words.
**              Set DSPP_BULLDOG_CLOCK_RATE to 66MHz.
**  950420 WJB  Renamed DSPI_ANVIL_EMUL_ things as DSPI_ things.
**  950424 WJB  Moved magic opera EO_ constants from dspp_touch.h.
**  950428 WJB  Renamed DSPI_CPU_INT0 to DSPI_CPU_INT.
**  950501 WJB  Added DSPI_NUM_TRIGGERS.
**  950503 WJB  Added AF_BDA_PASS variants for DSPX_F_INT defines.
**  950505 WJB  Added madam.h and clio.h includes for AF_ASIC_OPERA mode.
**              Minor tweaks to recover from the great split.
**  950508 WJB  Made M2 DSPI_NUM_DMA_CHANNELS BDA pass-variant.
**  951204 WJB  Removed opera stuff. Tidied up formatting.
**  951207 PLB  Made run time switchable between Pass 1 and Pass 2
**  961016 PLB  Set DSPP_BULLDOG_CLOCK_RATE to 66.666667MHz.
**
**  Initials:
**
**  PLB: Phil Burk (phil)
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#ifndef PF_HOST_UNIX
	#include <kernel/types.h>
#endif


    /* DSPP 1.15 signal ranges */
#define DSPP_MAX_SIGNED    ((int32) 0x00007FFF)
#define DSPP_MIN_SIGNED    ((int32) 0xFFFF8000)
#define DSPP_MAX_UNSIGNED  ((int32) 0x0000FFFF)
#define DSPP_MIN_UNSIGNED  ((int32) 0x00000000)

/* -------------------- DSPI space (as seen from the DSPP) */

	/* Memory Segments */
#define DSPI_CODE_MEMORY_BASE           (0x000)
#define DSPI_CODE_MEMORY_SIZE           (0x400)

#define DSPI_DATA_MEMORY_BASE           (0x000)
#define DSPI_DATA_MEMORY_SIZE           (0x2e0)

#ifdef SIMULATE_DSPP
	#define DSPI_SIM_TRACE              (0x03FE)    /* Fake register for simulator */
	#define DSPIB_TRACE_VERBOSE         (0x01)      /* !!! DSPI_F_TRACE_VERBOSE??? this is a flag, not a bit number */
	#define DSPIB_TRACE_EXEC            (0x02)
	#define DSPIB_TRACE_FDMA            (0x04)
	#define DSPIB_TRACE_RW              (0x08)
	#define DSPIB_TRACE_OUTPUT          (0x10)
	/* Writing a 1 to this location will open the simulator adio file. */
	/* Writing a 0 to this location will close the simulator adio file. */
	#define DSPI_SIM_ADIO_FILE          (0x03EC)    /* Fake register for simulator */
#endif   /* SIMULATE_DSPP */

#define DSPI_FIFO_MAX_DEPTH             (0x08)      /* max depth of a FIFO */


/* -------------------- DSPX space (addressable by CPU) */

#define DSPP_BASE                       (0x00060000)

	/* Code Memory - can get address using &DSPX_CODE_MEMORY[offset] or DSPX_CODE_MEMORY + offset */
#define DSPX_CODE_MEMORY                DSPX_CODE_MEMORY_LOW
#define DSPX_CODE_MEMORY_LOW            ((vuint32 *) (DSPP_BASE + 0x0000))
#define DSPX_CODE_MEMORY_HIGH           ( &DSPX_CODE_MEMORY [ DSPI_CODE_MEMORY_SIZE-1 ] )

	/*
		Data Memory - can get address using &DSPX_DATA_MEMORY[offset] or
		DSPX_DATA_MEMORY + offset where offset is a DSPI_ value.

		@@@ Note: This range covers the entire addressible data space, not
			merely the space defined by DSPI_DATA_MEMORY_SIZE.
	*/
#define DSPX_DATA_MEMORY                DSPX_DATA_MEMORY_LOW
#define DSPX_DATA_MEMORY_LOW            ((vuint32 *) (DSPP_BASE + 0x1000))
#define DSPX_DATA_MEMORY_HIGH           ((vuint32 *) (DSPP_BASE + 0X1FFC))

	/* Registers */
#define DSPX_INTERRUPT_SET              ((vuint32 *) (DSPP_BASE + 0x4000))
#define DSPX_INTERRUPT_CLR              ((vuint32 *) (DSPP_BASE + 0x4004))
#define DSPX_INTERRUPT_ENABLE           ((vuint32 *) (DSPP_BASE + 0x4008))
#define DSPX_INTERRUPT_DISABLE          ((vuint32 *) (DSPP_BASE + 0x400C))

	#define DSPX_F_INT_TIMER            (0x00000100)
	#define DSPX_F_INT_INPUT_UNDER      (0x00000080)
	#define DSPX_F_INT_INPUT_OVER       (0x00000040)
	#define DSPX_F_INT_OUTPUT_UNDER     (0x00000020)
	#define DSPX_F_INT_OUTPUT_OVER      (0x00000010)
	#define DSPX_F_INT_UNDEROVER        (0x00000008)
	#define DSPX_F_INT_CONSUMED         (0x00000002)
	#define DSPX_F_INT_DMANEXT          (0x00000001)

#define DSPX_INT_DMANEXT_SET            ((vuint32 *) (DSPP_BASE + 0x4010))
#define DSPX_INT_DMANEXT_CLR            ((vuint32 *) (DSPP_BASE + 0x4014))
#define DSPX_INT_DMANEXT_ENABLE         ((vuint32 *) (DSPP_BASE + 0x4018)) /* BDA2+ */
#define DSPX_INT_CONSUMED_SET           ((vuint32 *) (DSPP_BASE + 0x4020))
#define DSPX_INT_CONSUMED_CLR           ((vuint32 *) (DSPP_BASE + 0x4024))
#define DSPX_INT_CONSUMED_ENABLE        ((vuint32 *) (DSPP_BASE + 0x4028))
#define DSPX_INT_CONSUMED_DISABLE       ((vuint32 *) (DSPP_BASE + 0x402C))

#define DSPX_DMA_STACK_ADDRESS          ((vuint32 *) (DSPP_BASE + 0x5000))

#define DSPX_DMA_STACK                  ((vuint32 *) (DSPP_BASE + 0x5000))      /* !!! should probably be (DMARegisters *) instead */
	/* STACK OFFSETS FOR EACH CHANNEL (!!! remove?) */
#define DSPX_DMA_STACK_CHANNEL_SIZE     (0x10)
	/* WORD OFFSETS IN STACK FOR EACH CHANNEL (!!! remove?) */
#define DSPX_DMA_ADDRESS_OFFSET         (0x00)
#define DSPX_DMA_COUNT_OFFSET           (0x01)
#define DSPX_DMA_NEXT_ADDRESS_OFFSET    (0x02)
#define DSPX_DMA_NEXT_COUNT_OFFSET      (0x03)

typedef char AudioGrain;    /* Define granularity of audio DMA */

#define DSPX_DMA_STACK_CONTROL          ((vuint32 *) (DSPP_BASE + 0x5200))
	#define DSPX_F_DMA_NEXTVALID        (0x0001)
	#define DSPX_F_DMA_GO_FOREVER       (0x0002)
	#define DSPX_F_INT_DMANEXT_EN       (0x0004)
	#define DSPX_F_SHADOW_SET_DMANEXT   (0x00040000)
	#define DSPX_F_SHADOW_SET_FOREVER   (0x00020000)
	#define DSPX_F_SHADOW_SET_NEXTVALID (0x00010000)

#define DSPX_CHANNEL_ENABLE             ((vuint32 *) (DSPP_BASE + 0x6000))
#define DSPX_CHANNEL_DISABLE            ((vuint32 *) (DSPP_BASE + 0x6004))
#define DSPX_CHANNEL_DIRECTION_SET      ((vuint32 *) (DSPP_BASE + 0x6008))
#define DSPX_CHANNEL_DIRECTION_CLR      ((vuint32 *) (DSPP_BASE + 0x600C))
	#define DSPX_DIR_RAM_TO_DSPP        (0)
	#define DSPX_DIR_DSPP_TO_RAM        (1)
#define DSPX_CHANNEL_8BIT_SET           ((vuint32 *) (DSPP_BASE + 0x6010))
#define DSPX_CHANNEL_8BIT_CLR           ((vuint32 *) (DSPP_BASE + 0x6014))
#define DSPX_CHANNEL_SQXD_SET           ((vuint32 *) (DSPP_BASE + 0x6018))
#define DSPX_CHANNEL_SQXD_CLR           ((vuint32 *) (DSPP_BASE + 0x601C))
#define DSPX_CHANNEL_RESET              ((vuint32 *) (DSPP_BASE + 0x6030))
#define DSPX_CHANNEL_STATUS             ((vuint32 *) (DSPP_BASE + 0x603C))

#define AUDIO_CONFIG                    ((vuint32 *) (DSPP_BASE + 0x6050))
#define AUDIN_CONFIG                    ((vuint32 *) (DSPP_BASE + 0x6060))
#define AUDOUT_CONFIG                   ((vuint32 *) (DSPP_BASE + 0x6068))
#define DSPX_CONTROL                    ((vuint32 *) (DSPP_BASE + 0x6070))
	#define DSPX_F_GWILLING             (0x0001)
	#define DSPX_F_STEP_CYCLE           (0x0002)
	#define DSPX_F_STEP_PC              (0x0004)
	#define DSPX_F_SNOOP                (0x0008)
#define DSPX_RESET                      ((vuint32 *) (DSPP_BASE + 0x6074))
	#define DSPX_F_RESET_DSPP           (0x0001)
	#define DSPX_F_RESET_INPUT          (0x0002)
	#define DSPX_F_RESET_OUTPUT         (0x0004)

#define DSPX_NMEM                       ((vuint32 *) (DSPP_BASE + 0x7000))
#define DSPX_DMEMRD                     ((vuint32 *) (DSPP_BASE + 0x7008))
#define DSPX_DMEMWR                     ((vuint32 *) (DSPP_BASE + 0x7010))
#define DSPX_RBASE                      ((vuint32 *) (DSPP_BASE + 0x7018))
#define DSPX_ACC                        ((vuint32 *) (DSPP_BASE + 0x701C))


#define DSPI_OUTPUT_FIFO_MAX_DEPTH           (8)

	/* Internal DSPP I/O Registers */
#define DSPI_INPUT0                     (0x02F0)
#define DSPI_INPUT1                     (0x02F1)
#define DSPI_OUTPUT0                    (0x02E0)
#define DSPI_OUTPUT1                    (0x02E1)
#define DSPI_OUTPUT2                    (0x02E2)
#define DSPI_OUTPUT3                    (0x02E3)
#define DSPI_OUTPUT4                    (0x02E4)
#define DSPI_OUTPUT5                    (0x02E5)
#define DSPI_OUTPUT6                    (0x02E6)
#define DSPI_OUTPUT7                    (0x02E7)
#define DSPI_INPUT_CONTROL              (0x03D6)
#define DSPI_OUTPUT_CONTROL             (0x03D7)
#define DSPI_INPUT_STATUS               (0x03DE)
#define DSPI_OUTPUT_STATUS              (0x03DF)
#define DSPI_PC                         (0x03EE)

#define DSPI_CPU_INT                    (0x03E6)

#define DSPI_AUDLOCK                    (0x03F6)
#define DSPI_CLOCK                      (0x03F7)
#define DSPI_NOISE                      (0x03FF)

#define DSPI_NUM_DMA_CHANNELS           (32)
#define DSPI_MAX_DMA_CHANNELS           (32)

	/* DSPI_FIFO_OSC registers */
#define DSPI_FIFO_OSC_BASE              (0x0300)

enum {                                  /* offsets in DSPI_FIFO_OSC for each channel */
	DSPI_FIFO_OSC_OFFSET_CURRENT,       /* read current value w/o removing from FIFO */
	DSPI_FIFO_OSC_OFFSET_NEXT,          /* read next value w/o removing from FIFO */
	DSPI_FIFO_OSC_OFFSET_FREQUENCY,
	DSPI_FIFO_OSC_OFFSET_PHASE,
	DSPI_FIFO_OFFSET_DATA,              /* R/W data, advance FIFO pointer */
	DSPI_FIFO_OFFSET_CONTROL,           /* R: status, W: control */
	DSPI_FIFO_OSC_NUM_REGS
};

#define DSPI_FIFO_OSC_SIZE              (8)

/* convenience macros to get to each FIFO register */
#define DSPI_FIFO_OSC(channel)          (dsphGetFIFOPartAddress(channel, 0 ))
#define DSPI_FIFO_CURRENT(channel)      (dsphGetFIFOPartAddress(channel, DSPI_FIFO_OSC_OFFSET_CURRENT))
#define DSPI_FIFO_NEXT(channel)         (dsphGetFIFOPartAddress(channel, DSPI_FIFO_OSC_OFFSET_NEXT))
#define DSPI_FIFO_FREQUENCY(channel)    (dsphGetFIFOPartAddress(channel, DSPI_FIFO_OSC_OFFSET_FREQUENCY))
#define DSPI_FIFO_PHASE(channel)        (dsphGetFIFOPartAddress(channel, DSPI_FIFO_OSC_OFFSET_PHASE))
#define DSPI_FIFO_DATA(channel)         (dsphGetFIFOPartAddress(channel, DSPI_FIFO_OFFSET_DATA))
#define DSPI_FIFO_CONTROL(channel)      (dsphGetFIFOPartAddress(channel, DSPI_FIFO_OFFSET_CONTROL))

	/* DSPI_FIFO registers */
#define DSPI_FIFO_BASE_P1                  (0x0380)
enum {                                  /* offsets in DSPI_FIFO for each channel */
	DSPI_FIFO_OFFSET_DATA_P1,              /* R/W data, advance FIFO pointer */
	DSPI_FIFO_OFFSET_CONTROL_P1,           /* R: status, W: control */
	DSPI_FIFO_SIZE_P1
};

	/* convenience macros to get to control registers */
#define DSPX_DMA_STACK_CONTROL_SIZE     (0x10)
#define DSPX_DMA_CONTROL_CURRENT_OFFSET (0x00)
#define DSPX_DMA_CONTROL_NEXT_OFFSET    (0x08)
#define DSPX_DMA_CONTROL_CURRENT(channel) ((vuint32 *) ( ((uint32)DSPX_DMA_STACK_CONTROL) + \
/**/                                                     ((channel)*DSPX_DMA_STACK_CONTROL_SIZE) + \
/**/                                                     DSPX_DMA_CONTROL_CURRENT_OFFSET ) )
#define DSPX_DMA_CONTROL_NEXT(channel)  (((vuint32 *) ( ((uint32)DSPX_DMA_STACK_CONTROL) + \
/**/                                                     ((channel)*DSPX_DMA_STACK_CONTROL_SIZE) + \
/**/                                                     DSPX_DMA_CONTROL_NEXT_OFFSET )) )

#define DSPX_INT_UNDEROVER_SET          ((vuint32 *) (DSPP_BASE + (0x4030)))
#define DSPX_INT_UNDEROVER_CLR          ((vuint32 *) (DSPP_BASE + (0x4034)))
#define DSPX_INT_UNDEROVER_ENABLE       ((vuint32 *) (DSPP_BASE + (0x4038)))
#define DSPX_INT_UNDEROVER_DISABLE      ((vuint32 *) (DSPP_BASE + (0x403C)))


#define DSPX_F_INT_ALL_DMA              (DSPX_F_INT_DMANEXT    | \
/**/                                     DSPX_F_INT_CONSUMED   | \
/**/                                     DSPX_F_INT_UNDEROVER )
/* unused: 0xfffff800 */

#define DSPX_FLD_INT_SOFT_WIDTH         16          /* width of the field and the number of interrupts */
#define DSPX_FLD_INT_SOFT_SHIFT         16
#define DSPX_FLD_INT_SOFT_MASK          (0xffff0000)

#define DSPX_F_SHADOW_SET_ADDRESS_COUNT (0x80000000)
#define DSPX_FRAME_DOWN_COUNTER         ((vuint32 *) (DSPP_BASE + 0x6040))
#define DSPX_FRAME_UP_COUNTER           ((vuint32 *) (DSPP_BASE + 0x6044))


/* -------------------- Misc Stuff */

	/* Clock rates in Hz (!!! perhaps not the best location for this) */
#define DSPP_BULLDOG_CLOCK_RATE         66666667    /* !!! This probably is wrong. maybe should come from SysInfo? */


/*****************************************************************************/

#endif /* __DSPPTOUCH_DSPP_ADDRESSES_H */
