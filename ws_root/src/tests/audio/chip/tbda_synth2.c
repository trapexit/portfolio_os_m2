/* @(#) tbda_synth2.c 96/03/27 1.3 */
/*
** Play several waveformsto test CPU performance of software synthesis.
** Note that Phase goes from 1.0 to 0.0 instead of 0.0 to 1.0
**
** Author: Phil Burk
*/

#ifdef TARGET_3DO       /* @@@ must be defined in Makefile */
	#include <kernel/types.h>
	#include <kernel/debug.h>
	#include <kernel/operror.h>
	#include <stdio.h>
	#include <stdlib.h>
#else
	#include <stdio.h>
	#include <stdlib.h>
	#include <math.h>
	typedef long int32;
	typedef unsigned long uint32;
	typedef short int16;
	typedef signed char int8;
	typedef unsigned char uint8;
	#define TRUE (1)
	#define FALSE (0)
#endif

typedef int8  AudioGrain;
#define ABS(x) (((x)<0)?-(x):(x))
void tPlaySynth( int32 NumIterations, int32 NumVoices, int32 NumPerBatch );

#define SYNTH_VERSION  "2.0"

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

#if 0
static int16 SineWave[] =
{
     0x0000,  0x18f8,  0x30fb,  0x471c,
     0x5a82,  0x6a6d,  0x7641,  0x7d8a,
     0x7fff,  0x7d8a,  0x7641,  0x6a6d,
     0x5a82,  0x471c,  0x30fb,  0x18f8,
     0x0000,  0xe708,  0xcf05,  0xb8e4,
     0xa57e,  0x9593,  0x89bf,  0x8276,
     0x8000,  0x8276,  0x89bf,  0x9593,
     0xa57e,  0xb8e4,  0xcf05,  0xe708,
};
#endif

static int16 SineWaveCBXD[] =
{
     0x004a,  0x4947,  0x4541,  0x3931,
     0x21df,  0xcfc7,  0xbfbd,  0xb94a,
     0x00b6,  0xb7b9,  0x8ec1,  0xc5cf,
     0xdf21,  0x3139,  0x8e45,  0x4749,
};

static int32 LookupDecompression[256];

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
	AudioGrain    *sb_Address;
	int32    sb_PreviousSample;  /* For CBXD algorithm. */
	int32    sb_Count;
	AudioGrain    *sb_NextAddress;
	int32    sb_NextCount;
	int32    sb_Flags;
} SampleBlock;

#define SB_F_CBXD     (0x0001)

#define MAX_NUM_SAMPLERS   (16)
#define MAX_SAMPLES_PER_BATCH  (8)

SampleBlock gSamplerBlocks[MAX_NUM_SAMPLERS];

void tPlaySynth( int32 NumIterations, int32 NumVoices, int32 NumPerBatch )
{
	int32 i,j;
	int32 LeftData[MAX_SAMPLES_PER_BATCH], RightData[MAX_SAMPLES_PER_BATCH];
	register SampleBlock *sb;
	register float Phase, Previous, Current;
	register int32 SampleCount;
	register AudioGrain *SampleAddress;
	int32   PreviousSample;
	float   Temp, OutScalar;
	float   Fundamental;

	Fundamental = (1.9999 / (MAX_NUM_SAMPLERS+1));

	OutScalar = 1.0 / NumVoices;

	for( i=0; i<NumVoices; i++ )
	{
		sb = &gSamplerBlocks[i];
		sb->sb_PhaseInc = Fundamental * (i+1);
		DBUG(("Freq[%d] = %g\n", i, sb->sb_PhaseInc ));
		sb->sb_Amplitude = 1.0;
		sb->sb_Phase = 1.0;
		sb->sb_Previous = 0.0;
		sb->sb_Current = 0.0;
		sb->sb_Address = (AudioGrain *) &SineWaveCBXD[0];
		sb->sb_Count = (sizeof(SineWaveCBXD) / sizeof(int8));
		sb->sb_NextAddress = sb->sb_Address;
		sb->sb_NextCount = sb->sb_Count;
		sb->sb_Flags = SB_F_CBXD;
	}
	while(NumIterations--)
	{
/* Calculate N waveforms. */
		for( i=0; i<NumVoices; i++ )
		{
			sb = &gSamplerBlocks[i];
			Phase = sb->sb_Phase;
			Previous = sb->sb_Previous;
			Current = sb->sb_Current;
			SampleCount = sb->sb_Count;
			SampleAddress = sb->sb_Address;
			PreviousSample = sb->sb_PreviousSample;

			for( j=0; j<NumPerBatch; j++ )
			{
				Phase -= sb->sb_PhaseInc;
DBUG(("--------\nVoice = %d, PhaseInc = %g, Phase = %g\n", i, sb->sb_PhaseInc, Phase ));
				while( Phase < 0.0 )
				{
DBUG(("Address = 0x%x, Count = 0x%x\n", SampleAddress, SampleCount ));
					Phase += 1.0;
					Previous = Current;
					if( sb->sb_Flags & SB_F_CBXD )
					{
						uint32 Temp = *(((uint8 *)SampleAddress++));
						int32 Cubed = LookupDecompression[Temp];
						if ( Temp & 1 ) Cubed += PreviousSample;
						PreviousSample = Cubed;
DBUG(("Temp = 0x%02x, Cubed = 0x%08x\n", Temp, Cubed ));
						Current = Cubed;
					}
					else
					{
						Current = *(((int16 *)SampleAddress)++);    /* !!! illegal C - can't ++ the result of an expression, in this case ((int16 *)SampleAddress) */
					}
					if( --SampleCount <= 0 )
					{
						SampleAddress = sb->sb_NextAddress;
						SampleCount = sb->sb_NextCount;
					}
				}
DBUG(("Phase = %g, Previous = %g, Current = %g\n", Phase, Previous, Current ));
/* Interpolate sample. */
				Temp = Current + ((Previous - Current) * Phase);
				sb->sb_Output = Temp * sb->sb_Amplitude;
DBUG(("Temp = %g, Output = %g\n", Temp, sb->sb_Output ));
				LeftData[j] += sb->sb_Output * OutScalar;
				RightData[j] += sb->sb_Output * OutScalar;
DBUG(("LeftData = %d, RightData = %d\n", LeftData[j], RightData[j] ));
			}

/* Update structure. */
			sb->sb_PreviousSample = PreviousSample;
			sb->sb_Count = SampleCount;
			sb->sb_Address = SampleAddress;
			sb->sb_Phase = Phase;
			sb->sb_Previous = Previous;
			sb->sb_Current = Current;
		}

	}
}

void tFillDecompressionTable( void )
{
	int32 i;
	int32  Temp;
	for( i=0; i<256; i++)
	{
		Temp = i;
		if( Temp & 0x80 ) Temp |= 0xFFFFFF00;  /* Sign extend. */
		LookupDecompression[i] = (Temp * Temp * Temp) >> 6;
		DBUG(("LookupDecompression[0x%x] = 0x%08x\n", i, LookupDecompression[i] ));
	}
}

int main( int32 argc, char *argv[])
{
	int32 NumSamples, NumVoices, NumPerBatch ;

	printf("Synth Version = %s\n", SYNTH_VERSION );
	NumSamples = ( argc > 1 ) ? atoi(argv[1]) : 44100 ;
	NumVoices = ( argc > 2 ) ? atoi(argv[2]) : 8 ;
	NumPerBatch = ( argc > 3 ) ? atoi(argv[3]) : 8 ;
	printf("NumSamples = %d, NumVoices = %d, NumPerBatch = %d\n",
		NumSamples, NumVoices, NumPerBatch );

	tFillDecompressionTable();
	tPlaySynth( NumSamples/NumPerBatch, NumVoices, NumPerBatch );
	PRT(("%s done.\n", argv[0] ));
	return 0;
}

