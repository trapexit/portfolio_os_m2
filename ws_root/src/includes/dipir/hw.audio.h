#ifndef __DIPIR_HW_AUDIO_H
#define __DIPIR_HW_AUDIO_H 1


/******************************************************************************
**
**  @(#) hw.audio.h 96/03/28 1.7
**
**  Definitions related to the HWResource_Audio structure.
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

typedef struct HWResource_Audio
{
	uint32		audio_Addr;		/* Address */
	uint32		audio_SharedConfig;	/* Shared config reg value */
	uint32		audio_InputConfig;	/* Input config reg value */
	uint32		audio_OutputConfig;	/* Output config reg value */
} HWResource_Audio;

/*
 * Audio IO Setup DAC/ADC
 */

/* Pack bits into configuration fields. */
#define MakeAudioConfig(ClkMaster,PosWsLength,FrameClkRate,BitClkRate) \
        ((((ClkMaster) & 1)<<31) | \
         (((PosWsLength) & 0x1FF)<<16) | \
         (((FrameClkRate) & 0x1FF)<<7) | \
          ((BitClkRate) & 0xF))

#define MakeAudinConfig(PosEdge,Ch0high,Ch01st,WordStart,SubFrameLength) \
        ((((PosEdge) & 0x1)<<30) | \
         (((Ch0high) & 0x1)<<29) | \
         (((Ch01st) & 0x1)<<28) | \
         (((WordStart) & 0xFF)<<16) | \
         (((SubFrameLength) & 0xFF)<<8))

#define MakeAudoutConfig(PosEdge,Ch0high,Chan248,WordStart,SubFrameLength) \
        ((((PosEdge) & 0x1)<<30) | \
         (((Ch0high) & 0x1)<<29) | \
         (((Chan248) & 0x3)<<27) | \
         (((WordStart) & 0xFF)<<16) | \
         (((SubFrameLength) & 0xFF)<<8))

/* Select AUDIO configuration based on BDA pass 1 or 2 and card DevCard or
 * MultiPlayer.
 */
#define DSP_SAMPLE_RATE   (44100)

#ifdef BUILD_BDA1_1
/* BDA 1.1 Dev Card uses Crystal 4216 Codec in serial mode 2 */
	#define B11DC_BITS_PER_FRAME		(256)
	#define B11DC_XTAL_FREQ			(22579200)
	#define B11DC_DATA_OUTPUT_EDGE		(0)
	#define B11DC_SYNC_TO_DATA_OFFSET	(0)
	#define B11DC_SUB_FRAME_LENGTH		(32)
	#define B11DC_AUDWS_LENGTH		(2)
	#define B11DC_BIT_CLOCK_DIVIDER		(( (B11DC_XTAL_FREQ / DSP_SAMPLE_RATE) / B11DC_BITS_PER_FRAME) / 2)
#endif

/* BDA 2.0 must have these settings for all machines. */
#define B20_BITS_PER_FRAME		(32)
#define B20_SUB_FRAME_LENGTH		(16)

/* BDA 2.0 Dev Card uses Crystal 4216 Codec in serial mode 4 */
#define B20DC_XTAL_FREQ			(22579200/2)
#define B20DC_DATA_OUTPUT_EDGE		(1)
#define B20DC_DATA_INPUT_EDGE		(0)
#define B20DC_SYNC_TO_DATA_OFFSET	(1)
#define B20DC_AUDWS_LENGTH		(2)
#define B20DC_BIT_CLOCK_DIVIDER	(( (B20DC_XTAL_FREQ / DSP_SAMPLE_RATE) / B20_BITS_PER_FRAME) / 2)

/* Multiplayer uses Asahi DAC */
#define B20MP_XTAL_FREQ			(16934400)
#define B20MP_DATA_OUTPUT_EDGE		(0)
#define B20MP_DATA_INPUT_EDGE		(1)
#define B20MP_SYNC_TO_DATA_OFFSET	(0)
#define B20MP_AUDWS_LENGTH		(16)
#define B20MP_BIT_CLOCK_DIVIDER	(( (B20MP_XTAL_FREQ / DSP_SAMPLE_RATE) / B20_BITS_PER_FRAME) / 2)

#endif /* __DIPIR_HW_AUDIO_H */
