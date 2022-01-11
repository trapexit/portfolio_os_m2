/* @(#) tbda_synth1.c 96/03/27 1.4 */
/*
** Play several waveforms using DSPPs DAC
**
** Author: Phil Burk
*/

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <stdio.h>
#include <stdlib.h>


#include <dspptouch/dspp_addresses.h>
#include <dspptouch/dspp_touch.h>
#include <dspptouch/touch_hardware.h>

void tPlaySynth( int32 NumVoices, int32 NumPerBatch );

#define SYNTH_VERSION  "0.4"

#ifndef ASIC_M2
#define ASIC_M2
#endif

#ifdef ASIC_M2

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		goto cleanup; \
	}

/* Triangle on left channel, sawtooth on right. */
int16 TriangleData[] = {
0x8000 ,
0x9000 ,
0xA000 ,
0xB000 ,
0xC000 ,
0xD000 ,
0xE000 ,
0xF000 ,
0x0000 ,
0x1000 ,
0x2000 ,
0x3000 ,
0x4000 ,
0x5000 ,
0x6000 ,
0x7000 ,
0x7FFF ,
0x6FFF ,
0x5FFF ,
0x4FFF ,
0x3FFF ,
0x2FFF ,
0x1FFF ,
0x0FFF ,
0xFFFF ,
0xEFFF ,
0xDFFF ,
0xCFFF ,
0xBFFF ,
0xAFFF ,
0x9FFF ,
0x8FFF ,
};

typedef struct SamplerBlock
{
/* Inputs */
	float    sb_PhaseInc;
	float    sb_Amplitude;
/* Outputs */
	float    sb_Output;
/* Internal */
	float    sb_Phase;
	float    sb_Previous;
	float    sb_Current;
	int16    *sb_Address;
	int32    sb_Count;
	int16    *sb_NextAddress;
	int32    sb_NextCount;
} SampleBlock;

#define MAX_NUM_SAMPLERS   (16)
#define MAX_SAMPLES_PER_BATCH  (8)

SampleBlock gSamplerBlocks[MAX_NUM_SAMPLERS];

void tPlaySynth( int32 NumVoices, int32 NumPerBatch )
{
	int32 i,j;
	int32 LeftData[MAX_SAMPLES_PER_BATCH], RightData[MAX_SAMPLES_PER_BATCH];
	register SampleBlock *sb;
	float   Temp, OutScalar;
	register float Phase;
	register float Previous, Current;
	int32 LoopCount;
	float Fundamental;


	Fundamental = (1.9999 / (MAX_NUM_SAMPLERS+1));

	OutScalar = 1.0 / NumVoices;

	for( i=0; i<NumVoices; i++ )
	{
		sb = &gSamplerBlocks[i];
		sb->sb_PhaseInc = Fundamental * (i+1);
		printf("Freq[%d] = %g\n", i, sb->sb_PhaseInc );
		sb->sb_Amplitude = 1.0;
		sb->sb_Phase = 0.0;
		sb->sb_Previous = 0.0;
		sb->sb_Current = 0.0;
		sb->sb_Address = &TriangleData[0];
		sb->sb_Count = (sizeof(TriangleData) / sizeof(int16));
		sb->sb_NextAddress = &TriangleData[0];
		sb->sb_NextCount = (sizeof(TriangleData) / sizeof(int16));
	}
	while(1)
	{
		if( (LoopCount++ & 0x3FFF) == 0 ) Yield();


/* Calculate N waveforms. */
		for( i=0; i<NumVoices; i++ )
		{
			sb = &gSamplerBlocks[i];
			Phase = sb->sb_Phase;
			Previous = sb->sb_Previous;
			Current = sb->sb_Current;

			for( j=0; j<NumPerBatch; j++ )
			{
				Phase += sb->sb_PhaseInc;
DBUG(("i = %d, PhaseInc = %g, Phase = %g\n", i, sb->sb_PhaseInc, Phase ));
				while( Phase > 1.0 )
				{
DBUG(("Address = 0x%x, Count = 0x%x\n", sb->sb_Address, sb->sb_Count ));
					Phase -= 1.0;
					Previous = Current;
					Current = *(sb->sb_Address++);
					sb->sb_Count -= 1;
					if( sb->sb_Count <= 0 )
					{
						sb->sb_Address = sb->sb_NextAddress;
						sb->sb_Count = sb->sb_NextCount;
					}
				}
DBUG(("Phase = %g, Previous = %g, Current = %g\n", Phase, Previous, Current ));
/* Interpolate sample. */
				Temp = Previous + ((Current - Previous) * Phase);
				sb->sb_Output = Temp * sb->sb_Amplitude;
DBUG(("Temp = %g, Output = %g\n", Temp, sb->sb_Output ));
				LeftData[j] += sb->sb_Output * OutScalar;
				RightData[j] += sb->sb_Output * OutScalar;
DBUG(("LeftData = %d, RightData = %d\n", LeftData[j], RightData[j] ));
			}

/* Update structure. */
			sb->sb_Phase = Phase;
			sb->sb_Previous = Previous;
			sb->sb_Current = Current;
		}
		for( j=0; j<NumPerBatch; j++ )
		{

/* Hang until room in FIFO */
			while( ReadHardware(&DSPX_DATA_MEMORY[DSPI_OUTPUT_STATUS]) >= DSPI_OUTPUT_FIFO_MAX_DEPTH );

/* Write two samples to DAC. */
#if (AF_BDA_PASS == 1)
/* DAC is in wrong slave mode. */
			WriteHardware(&DSPX_DATA_MEMORY[DSPI_OUTPUT0], LeftData[j]>>1);
			WriteHardware(&DSPX_DATA_MEMORY[DSPI_OUTPUT1], RightData[j]>>1);
#else
			WriteHardware(&DSPX_DATA_MEMORY[DSPI_OUTPUT0], LeftData[j]);
			WriteHardware(&DSPX_DATA_MEMORY[DSPI_OUTPUT1], RightData[j]);
#endif
			WriteHardware(&DSPX_DATA_MEMORY[DSPI_OUTPUT_CONTROL], 1);
			LeftData[j] = 0;
			RightData[j] = 0;
		}


	}
}

int main( int32 argc, char *argv[])
{
	int32 NumVoices, NumPerBatch ;

	printf("Synth Version = %s\n", SYNTH_VERSION );
	NumVoices = ( argc > 1 ) ? atoi(argv[1]) : 1 ;
	NumPerBatch = ( argc > 2 ) ? atoi(argv[2]) : 1 ;
	printf("NumVoices = %d, NumPerBatch = %d\n", NumVoices, NumPerBatch );

	dsphInitDSPP();

	dsphEnableADIO();
	tPlaySynth( NumVoices, NumPerBatch );
}

#endif  /* ASIC_M2 */
