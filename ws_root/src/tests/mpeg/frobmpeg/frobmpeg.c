/* @(#) frobmpeg.c 96/05/23 1.5 */
/* file: dumpmpegstate.c */
/* dump state of the M2 MPEG Unit hardware */
/* 6/21/95 George Mitsuoka */
/* The 3DO Company Copyright 1995 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/interrupts.h>
#include <kernel/kernel.h>
#include <kernel/super.h>
#include <kernel/sysinfo.h>
#include <hardware/bda.h>
#include <hardware/PPCasm.h>

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned version:16, revision:16;
		}
		f;
	}
	TDeviceIDReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned reserved0:19,
					 vofRdEnable:1, vofWrEnable:1, vofReset_n:1,
					 vodEnable:1, vodReset_n:1,
					 motEnable:1, motReset_n:1,
					 mvdReset_n:1,
					 parserStep:1, parserEnable:1, parserReset_n:1,
					 vbdEnable:1, vbdReset_n:1;
		}
		f;
	}
	TConfigReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned reserved0:25,
					 stripBufferError:1, everythingDone:1,
					 outputFormatter:1, outputDMA:1, bitstreamError:1,
					 endOfPicture:1, videoBitstreamDMA:1;
		}
		f;
	}
	TInterruptReg;

typedef
	struct
	{
		unsigned reserved0:7, address:25;
		unsigned reserved1:15, length:17;
	}
	TDMAReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned reserved0:15,
					 vbdSnoopEnable:1, reserved1:9, bufferByteCount:7;
		}
		f;
	}
	TBufferStatusReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b0002:3, fullPelBackwardVector:1, b04:1, backwardRSize:3,
					 b0810:3, fullPelForwardVector:1, b12:1, forwardRSize:3,
					 b16:1, priorityMode:3, b2028:9, pictureCodingType:3;
		}
		f;
	}
	TParserConfigReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b00:1, mbRow:7, b08:1,
					 bitstreamError:1, errorState:6, evalBits:16;
		}
		f;
	}
	TParserStatus0Reg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b0002:3, blockNumber:3, macroBlockType:5, numBitsValid:5,
					 lastStartCode:8, b24:1, mbCol:7;
		}
		f;
	}
	TParserStatus1Reg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b0015:16, mbHeight:8, mbWidth:8;
		}
		f;
	}
	TImageSizeReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned reserved0:7, address:25;
		}
		f;
	}
	TRefBufAddrReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b0006:7,outputBufferAddress:13, b2027:8,
					 fullFrameStripBuffer:1, stripBufferEnable:1,
					 stripBufferSize:2;
		}
		f;
	}
	TOutputDMAConfigReg;

#define STRIPBUFFERSIZE16K	0
#define STRIPBUFFERSIZE32K	1
#define STRIPBUFFERSIZE64K	2
#define STRIPBUFFERSIZE128K	3
#define DEFAULT_STRIPBUFFERSIZE (1024 * 32)
#define DEFAULT_SBSIZECODE STRIPBUFFERSIZE32K

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b0002:3, vofSnoopEnable:1, b0406:3, ditherMode:1,
					 b0809:2, operaFormat:1, format:1, b12:1, resamplerMode:3,
					 b1618:3, enableCSC:1, bit2022:3, rowChunks:9;
		}
		f;
	}
	TOutputFormatConfigReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b00:1,hStart:7,b08:1,vStart:7,b16:1,hStop:7,
					 b24:1,vStop:7;
		}
		f;
	}
	TOutputFormatCropReg;

typedef
	union
	{
		uint32 i;
		struct
		{
			unsigned b0023:24, DSB:1, alphaFillValue:7;
		}
		f;
	}
	TOutputFormatAlphaFillReg;

#define MPG_REG_DEVICEID			0x0000L
#define MPG_REG_CONFIG				0x0004L
#define MPG_REG_INTENABLE			0x0008L
#define MPG_REG_INTSTATUS			0x000cL
#define MPG_REG_VBSCURADDR			0x0010L
#define MPG_REG_VBSCURLEN			0x0014L
#define MPG_REG_VBSNEXTADDR			0x0018L
#define MPG_REG_VBSNEXTLEN			0x001cL
#define MPG_REG_VBSCONFIG			0x0020L
#define MPG_REG_PARSERCONFIG		0x0024L
#define MPG_REG_PARSERSTATUS0		0x0028L
#define MPG_REG_PARSERSTATUS1		0x005cL
#define MPG_REG_IMAGESIZE			0x002cL
#define MPG_REG_REVPREDBUF			0x0054L
#define MPG_REG_FWDPREDBUF			0x0030L
#define MPG_REG_OUTDMACONFIG		0x0034L
#define MPG_REG_FORMATCONFIG		0x0038L
#define MPG_REG_FORMATSIZE			0x0050L
#define MPG_REG_FORMATCROP			0x0040L
#define MPG_REG_FORMATINPUT			0x003cL
#define MPG_REG_FORMATOUTPUT		0x0044L
#define MPG_REG_UPPERDITHERMTX		0x0048L
#define MPG_REG_LOWERDITHERMTX		0x004cL
#define MPG_REG_LFSR				0x0050L
#define MPG_REG_FORMATALPHAFILL		0x0060L
#define MPG_REG_INTRAQUANTTABLE		0x0200L
#define MPG_REG_NONINTRAQUANTTABLE	0x0240L

void M2WriteMPGRegister( uint32 regOffset, uint32 value );
uint32 M2ReadMPGRegister( uint32 regOffset, uint32 expectedValue );
int32 M2MPEGInit( void );
Err M2MPGDumpState( void );

vuint32 *MPEGUnitBaseAddr = 0L;

#define print(a)			printf a

#ifdef DEBUG_PRINT
#define DEBUG(a)			print(a)
#else
#define DEBUG(a)
#endif

Err superMain( uint32 argc, char **argv );

main( int argc, char **argv )
{
	CallBackSuper( superMain, (uint32) argc, (uint32) argv, 0L );
}

Err superMain( uint32 argc, char **argv )
{
	int32 i, stepCount = 0L, dumpState = 0L, addr, value;
	TConfigReg configReg;

	M2MPEGInit();

	for( i = 1L; i < argc; i++ )
	{
		if( strcmp( argv[i],"-s" ) == 0L )
		{
			if( ++i >= argc )
			{
				printf("missing step argument\n");
				return( 1 );
			}
			stepCount = atoi( argv[ i ] );
		}
		else if( strcmp( argv[i], "-d" ) == 0L )
		{
			dumpState = 1L;
		}
		else if( strcmp( argv[i], "-e" ) == 0L )
		{
			configReg.i = M2ReadMPGRegister( MPG_REG_CONFIG, 0L );
			configReg.f.parserEnable = 1;
			M2WriteMPGRegister( MPG_REG_CONFIG, configReg.i );
			printf("enabled parser\n");
		}
		else if( strcmp( argv[i], "-preset" ) == 0L )
		{
			configReg.i = M2ReadMPGRegister( MPG_REG_CONFIG, 0L );
			configReg.f.parserReset_n = 0;
			M2WriteMPGRegister( MPG_REG_CONFIG, configReg.i );
			configReg.f.parserReset_n = 1;
			M2WriteMPGRegister( MPG_REG_CONFIG, configReg.i );
			printf("reset parser\n");
		}
		else if( strcmp( argv[i], "-vbdreset" ) == 0L )
		{
			configReg.i = M2ReadMPGRegister( MPG_REG_CONFIG, 0L );
			configReg.f.vbdReset_n = 0;
			M2WriteMPGRegister( MPG_REG_CONFIG, configReg.i );
			configReg.f.vbdReset_n = 1;
			M2WriteMPGRegister( MPG_REG_CONFIG, configReg.i );
			printf("reset vbd\n");
			M2WriteMPGRegister( MPG_REG_VBSNEXTADDR, 0L );
			M2WriteMPGRegister( MPG_REG_VBSNEXTLEN, 1024L );
			printf("wrote nextaddr,len\n");
		}
		else if( strcmp( argv[i], "-wc" ) == 0L )
		{
			if( ++i >= argc )
			{
				printf("missing config reg value\n");
				exit( 1 );
			}
			value = atoi( argv[ i ] );
			printf("writing 0x%08lx to config reg\n",value);
			M2WriteMPGRegister( MPG_REG_CONFIG, value );
			value = M2ReadMPGRegister( MPG_REG_CONFIG, 0L );
			printf("config reg = 0x%08lx\n",value);
		}
		else if( strcmp( argv[i], "-w" ) == 0L )
		{
			if( ++i >= argc )
			{
				printf("missing address and value for write\n");
				exit( 1 );
			}
			addr = atoi( argv[ i ] );
			if( ++i >= argc )
			{
				printf("missing value for write\n");
				exit( 1 );
			}
			value = atoi( argv[ i ] );
			*((uint32 *) addr) = value;
			printf("%08lx = %08lx\n",addr,value);
		}
		else if( strcmp( argv[i], "-r" ) == 0L )
		{
			if( ++i >= argc )
			{
				printf("missing address for read\n");
				exit( 1 );
			}
			addr = atoi( argv[ i ] );
			value = *((uint32 *) addr);
			printf("%08lx = %08lx\n",addr,value);
		}
	}
	if( stepCount )
		printf("stepping parser %d time(s)\n",stepCount);
	for( i = 0L; i < stepCount; i++ )
	{
		configReg.i = M2ReadMPGRegister( MPG_REG_CONFIG, 0L );
		/* printf("read 0x%08lx\n",configReg.i); */
		configReg.f.parserStep = 1L;
		/* printf("writing 0x%08lx\n",configReg.i); */
		M2WriteMPGRegister( MPG_REG_CONFIG, configReg.i );
		printf("%d\n",i+1);
		if( dumpState )
			M2MPGDumpState();
	}
	if( argc == 1 )
		M2MPGDumpState();

	return( 0 );
}

int32 gM2MPEGVersionInfo;

/*	write an M2 MPEG Unit register. regOffset is the byte offset
	of the register from the base of the MPEG Unit address space.
	value is a 32 bit value to write */

void M2WriteMPGRegister( uint32 regOffset, uint32 value )
{
	vuint32 *dest;

	/* regOffset is a byte offset */
	dest = (vuint32 *) ((uint8 *) MPEGUnitBaseAddr + regOffset);

	DEBUG(("MPGWrite 0x%08lx = 0x%08lx\n",dest,value));
	BDA_WRITE( (uint32) dest, value );
}

uint32 M2ReadMPGRegister( uint32 regOffset, uint32 expectedValue )
{
	vuint32 *src;
	uint32 result;

	TOUCH( expectedValue );

	/* regOffset is a byte offset */
	src = (vuint32 *) ((uint8 *) MPEGUnitBaseAddr + regOffset);

	result = BDA_READ( (uint32) src );
	DEBUG(("MPGRead 0x%08lx = %08lx\n",src,result));
	return( result );
}

int32 M2MPEGInit( void )
{
	BDAInfo bdaInfo;

	print(("M2MPEGInit\n"));

	SuperQuerySysInfo( SYSINFO_TAG_BDA, &bdaInfo, sizeof(bdaInfo));
	MPEGUnitBaseAddr = (vuint32 *) bdaInfo.bda_MPEGBase;

	print(("SysInfo says MPEGUnitBaseAddr = 0x%08lx\n",MPEGUnitBaseAddr));
	print(("             PowerBusBase     = 0x%08lx\n",bdaInfo.bda_PBBase));
	print(("             MemControlBase   = 0x%08lx\n",bdaInfo.bda_MCBase));
	print(("             VDUBase          = 0x%08lx\n",bdaInfo.bda_VDUBase));
	print(("             TEBase           = 0x%08lx\n",bdaInfo.bda_TEBase));
	print(("             DSPBase          = 0x%08lx\n",bdaInfo.bda_DSPBase));
	print(("             ControlPortBase  = 0x%08lx\n",bdaInfo.bda_CPBase));
	print(("             MPEGUnitBase     = 0x%08lx\n",bdaInfo.bda_MPEGBase));

	/* get version info */
	gM2MPEGVersionInfo = M2ReadMPGRegister( MPG_REG_DEVICEID, 0L );

	print(("M2 MPEG Unit Version %d, revision %d\n",
				 gM2MPEGVersionInfo >> 16, gM2MPEGVersionInfo & 0xffffL));
	return( 0L );
}

/* print out state */

Err M2MPGDumpState( void )
{
	TDeviceIDReg idReg;
	TConfigReg configReg;
	TInterruptReg interruptReg;
	uint32 tempReg;
	TBufferStatusReg bufferStatusReg;
	TParserConfigReg parserConfigReg;
	TParserStatus0Reg parserStatus0Reg;
	TParserStatus1Reg parserStatus1Reg;
	TImageSizeReg imageSizeReg;
	TOutputDMAConfigReg outputDMAConfigReg;
	TOutputFormatConfigReg outputFormatConfigReg;
	TOutputFormatCropReg outputFormatCropReg;
	TOutputFormatAlphaFillReg outputFormatAlphaFillReg;

	print(("Dumping MPEG Unit State:\n"));
	idReg.i = M2ReadMPGRegister( MPG_REG_DEVICEID, 0x000000001L );
	print(("\tVersion = 0x%08x Revision = 0x%08x\n",
			idReg.f.version,idReg.f.revision ));

	print(("\n\tConfiguration Register\n"));
	configReg.i = M2ReadMPGRegister( MPG_REG_CONFIG, 0L );
	print(("\t\tvofRdEnable   = %d\n",configReg.f.vofRdEnable ));
	print(("\t\tvofWrEnable   = %d\n",configReg.f.vofWrEnable ));
	print(("\t\tvofReset_n    = %d\n",configReg.f.vofReset_n ));
	print(("\t\tvodEnable     = %d\n",configReg.f.vodEnable ));
	print(("\t\tvodReset_n    = %d\n",configReg.f.vodReset_n ));
	print(("\t\tmotEnable     = %d\n",configReg.f.motEnable ));
	print(("\t\tmotReset_n    = %d\n",configReg.f.motReset_n ));
	print(("\t\tmvdReset_n    = %d\n",configReg.f.mvdReset_n ));
	print(("\t\tparserStep    = %d\n",configReg.f.parserStep ));
	print(("\t\tparserEnable  = %d\n",configReg.f.parserEnable ));
	print(("\t\tparserReset_n = %d\n",configReg.f.parserReset_n ));
	print(("\t\tvbdEnable     = %d\n",configReg.f.vbdEnable ));
	print(("\t\tvbdReset_n    = %d\n",configReg.f.vbdReset_n ));

	print(("\n\tInterrupt Enable Register\n"));
	interruptReg.i = M2ReadMPGRegister( MPG_REG_INTENABLE, 0L );
	print(("\t\tstripBufferError  = %d\n",
				 interruptReg.f.stripBufferError ));
	print(("\t\teverythingDone    = %d\n",interruptReg.f.everythingDone ));
	print(("\t\toutputFormatter   = %d\n",interruptReg.f.outputFormatter ));
	print(("\t\toutputDMA         = %d\n",interruptReg.f.outputDMA ));
	print(("\t\tbitstreamError    = %d\n",interruptReg.f.bitstreamError ));
	print(("\t\tendOfPicture      = %d\n",interruptReg.f.endOfPicture ));
	print(("\t\tvideoBitstreamDMA = %d\n",interruptReg.f.videoBitstreamDMA ));

	print(("\n\tInterrupt Status Register\n"));
	interruptReg.i = M2ReadMPGRegister( MPG_REG_INTSTATUS, 0L );
	print(("\t\tstripBufferError  = %d\n",interruptReg.f.stripBufferError ));
	print(("\t\teverythingDone    = %d\n",interruptReg.f.everythingDone ));
	print(("\t\toutputFormatter   = %d\n",interruptReg.f.outputFormatter ));
	print(("\t\toutputDMA         = %d\n",interruptReg.f.outputDMA ));
	print(("\t\tbitstreamError    = %d\n",interruptReg.f.bitstreamError ));
	print(("\t\tendOfPicture      = %d\n",interruptReg.f.endOfPicture ));
	print(("\t\tvideoBitstreamDMA = %d\n",interruptReg.f.videoBitstreamDMA ));

	print(("\n\tVideo Bitstream DMA Registers\n"));
	tempReg = M2ReadMPGRegister( MPG_REG_VBSCURADDR, 0L );
	print(("\t\tCurrent Address = 0x%08lx\n",tempReg ));
	tempReg = M2ReadMPGRegister( MPG_REG_VBSCURLEN, 0L );
	print(("\t\tCurrent Length  = 0x%08lx\n",tempReg ));
	tempReg = M2ReadMPGRegister( MPG_REG_VBSNEXTADDR, 0L );
	print(("\t\tNext Address    = 0x%08lx\n",tempReg ));
	tempReg = M2ReadMPGRegister( MPG_REG_VBSNEXTLEN, 0L );
	print(("\t\tNext Length     = 0x%08lx\n",tempReg ));

	print(("\n\tVideo Bitstream DMA Config/Status Register\n"));
	bufferStatusReg.i = M2ReadMPGRegister( MPG_REG_VBSCONFIG, 0L );
	print(("\t\tvbdSnoopEnable  = %d\n",bufferStatusReg.f.vbdSnoopEnable ));
	print(("\t\tbufferByteCount = %d\n",bufferStatusReg.f.bufferByteCount ));

	print(("\n\tParser Configuration Register\n"));
	parserConfigReg.i = M2ReadMPGRegister( MPG_REG_PARSERCONFIG, 0L );
	print(("\t\tfullPelBackwardVector = %d\n",
			parserConfigReg.f.fullPelBackwardVector ));
	print(("\t\tbackwardRSize         = %d\n",
			parserConfigReg.f.backwardRSize ));
	print(("\t\tfullPelForwardVector  = %d\n",
			parserConfigReg.f.fullPelForwardVector ));
	print(("\t\tforwardRSize          = %d\n",
			parserConfigReg.f.forwardRSize ));
	print(("\t\tpriorityMode          = %d\n",
			parserConfigReg.f.priorityMode ));
	print(("\t\tpictureCodingType     = %d\n",
			parserConfigReg.f.pictureCodingType ));

	print(("\n\tParser Status Registers\n"));
	parserStatus0Reg.i = M2ReadMPGRegister( MPG_REG_PARSERSTATUS0, 0L );
	print(("\t\tmbRow          = %d\n",parserStatus0Reg.f.mbRow ));
	print(("\t\tbitstreamError = %d\n",parserStatus0Reg.f.bitstreamError ));
	print(("\t\terrorState     = %d\n",parserStatus0Reg.f.errorState ));
	print(("\t\tevalBits       = %d\n",parserStatus0Reg.f.evalBits ));
	parserStatus1Reg.i = M2ReadMPGRegister( MPG_REG_PARSERSTATUS1, 0L );
	print(("\t\tblockNumber    = %d\n",parserStatus1Reg.f.blockNumber ));
	print(("\t\tmacroBlockType = %d\n",parserStatus1Reg.f.macroBlockType ));
	print(("\t\tnumBitsValid   = %d\n",parserStatus1Reg.f.numBitsValid ));
	print(("\t\tlastStartCode  = %d\n",parserStatus1Reg.f.lastStartCode ));
	print(("\t\tmbCol          = %d\n",parserStatus1Reg.f.mbCol ));

	print(("\n\tImage Size Register\n"));
	imageSizeReg.i = M2ReadMPGRegister( MPG_REG_IMAGESIZE, 0L );
	print(("\t\tmbHeight = %d\n",imageSizeReg.f.mbHeight ));
	print(("\t\tmbWidth  = %d\n",imageSizeReg.f.mbWidth));

	print(("\n\tPrediction Buffer Address Registers\n"));
	tempReg = M2ReadMPGRegister( MPG_REG_REVPREDBUF, 0L );
	print(("\t\tReverse = 0x%08lx\n",tempReg ));
	tempReg = M2ReadMPGRegister( MPG_REG_FWDPREDBUF, 0L );
	print(("\t\tForward = 0x%08lx\n",tempReg ));

	print(("\n\tOutput DMA Configuration Register\n"));
	outputDMAConfigReg.i = M2ReadMPGRegister( MPG_REG_OUTDMACONFIG, 0L );
	print(("\t\toutputBufferAddress  = 0x%08lx\n",
			outputDMAConfigReg.f.outputBufferAddress << 12 ));
	print(("\t\tfullFrameStripBuffer = %d\n",
			outputDMAConfigReg.f.fullFrameStripBuffer ));
	print(("\t\tstripBufferEnable    = %d\n",
			outputDMAConfigReg.f.stripBufferEnable ));
	print(("\t\tstripBufferSize      = %d\n",
			outputDMAConfigReg.f.stripBufferSize ));

	print(("\n\tOutput Formatter Configuration Register\n"));
	outputFormatConfigReg.i = M2ReadMPGRegister( MPG_REG_FORMATCONFIG, 0L );
	print(("\t\tvofSnoopEnable = %d\n",outputFormatConfigReg.f.vofSnoopEnable ));
	print(("\t\tditherMode     = %d\n",outputFormatConfigReg.f.ditherMode ));
	print(("\t\toperaFormat    = %d\n",outputFormatConfigReg.f.operaFormat ));
	print(("\t\tformat         = %d\n",outputFormatConfigReg.f.format ));
	print(("\t\tresamplerMode  = %d\n",outputFormatConfigReg.f.resamplerMode ));
	print(("\t\tenableCSC      = %d\n",outputFormatConfigReg.f.enableCSC ));
	print(("\t\trowChunks      = %d\n",outputFormatConfigReg.f.rowChunks ));

	print(("\n\tOutput Formatter Image Size Register\n"));
	imageSizeReg.i = M2ReadMPGRegister( MPG_REG_FORMATSIZE, 0L );
	print(("\t\tmbHeight = %d\n",imageSizeReg.f.mbHeight ));
	print(("\t\tmbWidth  = %d\n",imageSizeReg.f.mbWidth ));

	print(("\n\tOutput Formatter Cropping Control Register\n"));
	outputFormatCropReg.i =  M2ReadMPGRegister( MPG_REG_FORMATCROP, 0L );
	print(("\t\thStart = %d\n",outputFormatCropReg.f.hStart ));
	print(("\t\tvStart = %d\n",outputFormatCropReg.f.hStart ));
	print(("\t\thStop  = %d\n",outputFormatCropReg.f.hStop ));
	print(("\t\tvStop  = %d\n",outputFormatCropReg.f.vStop ));

	print(("\n\tOutput Formatter Address Registers\n"));
	tempReg = M2ReadMPGRegister( MPG_REG_FORMATINPUT, 0L );
	print(("\t\tinput  = 0x%08lx\n",tempReg ));
	tempReg = M2ReadMPGRegister( MPG_REG_FORMATOUTPUT, 0L );
	print(("\t\toutput = 0x%08lx\n",tempReg ));

	print(("\n\tDither Matrix Registers\n"));
	tempReg = M2ReadMPGRegister( MPG_REG_UPPERDITHERMTX, 0L );
	print(("\t\tupper = 0x%08lx\n",tempReg ));
	tempReg = M2ReadMPGRegister( MPG_REG_LOWERDITHERMTX, 0L );
	print(("\t\tlower = 0x%08lx\n",tempReg ));

	print(("\n\tLFSR Register\n"));
	tempReg = M2ReadMPGRegister( MPG_REG_LFSR, 0L );
	print(("\t\tlfsr = 0x%08lx\n",tempReg ));

	print(("\n\tOutput Formatter Alpha Fill Register\n"));
	outputFormatAlphaFillReg.i =
		M2ReadMPGRegister( MPG_REG_FORMATALPHAFILL, 0L );
	print(("\t\thDSB            = %d\n",outputFormatAlphaFillReg.f.DSB ));
	print(("\t\thalphaFillValue = %d\n",
			outputFormatAlphaFillReg.f.alphaFillValue ));
	return( 0L );
}

