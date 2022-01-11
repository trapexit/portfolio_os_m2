/* @(#) tbda_wave.c 96/03/27 1.6 */
/*
** Play a waveform using DSPPs DAC
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

void tPlayWave( int32 LeftShift, int32 IfReverse, int32 ForceData );

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
uint16 Triangle[] = {
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

int16 SineWave[] =
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

void tPlayWave( int32 LeftShift, int32 IfReverse, int32 ForceData )
{
	int32 WaveIndex = 0;
/*	int32 OuterLoopCnt = 0; */
	int32 InnerLoopCnt = 0;

	while(1)
	{
/* Hang until room in FIFO */
		while( ReadHardware(&DSPX_DATA_MEMORY[DSPI_OUTPUT_STATUS]) >= DSPI_OUTPUT_FIFO_MAX_DEPTH )
		{
			InnerLoopCnt++;
		}
/* Write two samples to DAC. */
		if( ForceData == 0 )
		{
			if(IfReverse)
			{
				WriteHardware(&DSPX_DATA_MEMORY[DSPI_OUTPUT0], Triangle[WaveIndex]>>LeftShift);
				WriteHardware(&DSPX_DATA_MEMORY[DSPI_OUTPUT1], SineWave[WaveIndex]>>LeftShift); /* SINE */
			}
			else
			{
				WriteHardware(&DSPX_DATA_MEMORY[DSPI_OUTPUT0], SineWave[WaveIndex]>>LeftShift); /* SINE */
				WriteHardware(&DSPX_DATA_MEMORY[DSPI_OUTPUT1], Triangle[WaveIndex]>>LeftShift);
			}
		}
		else
		{
			WriteHardware(&DSPX_DATA_MEMORY[DSPI_OUTPUT0], ForceData);
			WriteHardware(&DSPX_DATA_MEMORY[DSPI_OUTPUT1], ForceData);
		}

		WriteHardware(&DSPX_DATA_MEMORY[DSPI_OUTPUT_CONTROL], 1);
		WaveIndex++;
		if( WaveIndex >=  (sizeof(Triangle)/sizeof(int16)) )
		{
			WaveIndex = 0;
		}
/* Report every few seconds. */
#if 0
		if( (OuterLoopCnt++ & 0x3FFFF) == 0)
		{
			PRT(("Average counts waiting = %d\n", InnerLoopCnt/OuterLoopCnt));
			OuterLoopCnt = 0;
			InnerLoopCnt = 0;
		}
#endif
	}
}

void PrintHelp( void )
{
	PRT(("tbda_wave <-sLEFTSHIFT> <-r> <-dData>\n"));
}
int main( int32 argc, char *argv[])
{

	int32 LeftShift = 0, IfReverse = 0, i;
	int32 ForceData = 0;
	char c, *s;

	for( i=1; i<argc; i++ )
	{
		s = argv[i];

		if( *s++ == '-' )
		{
			c = *s++;
			switch(c)
			{
			case 's':
				LeftShift = atoi(s);
				break;
			case 'r':
				IfReverse = TRUE;
				break;
			case 'd':
				ForceData = atoi(s);
				break;

			case '?':
			default:
				PrintHelp();
				exit(1);
				break;
			}
		}
		else
		{
			PrintHelp();
			exit(1);
		}
	}

	PRT(("LeftShift = %d\n", LeftShift));
	PRT(("ForceData = 0x%04x\n", ForceData));
	PRT(("If Reverse Left/Right = %d\n", IfReverse));

	dsphInitDSPP();

	dsphEnableADIO();
	tPlayWave( LeftShift, IfReverse, ForceData );
}

#endif  /* ASIC_M2 */
