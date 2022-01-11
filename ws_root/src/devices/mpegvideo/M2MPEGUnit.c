/* @(#) M2MPEGUnit.c 96/12/11 1.40 */

#include <stdio.h>
#include <kernel/types.h>
#include <kernel/cache.h>
#include <kernel/debug.h>
#include <kernel/interrupts.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/super.h>
#include <kernel/sysinfo.h>
#include <hardware/PPCasm.h>
#include <hardware/bda.h>

#include "M2MPEGUnit.h"
#include "MPEGVideoParse.h"
#include "mpVideoDriver.h"
#include "videoDeviceContext.h"

vuint32 *MPEGUnitBaseAddr = 0L;
uint8 *gRAMBase = 0L;
int32 gPictureNumber = 0L;

volatile uint8  gDum;

#ifdef DEBUG_ANALYZER_TRACING_ON
#define RBASE (0x40000000)
#define mark(a)  _dcbf((void *)(RBASE|a)); gDum =* (volatile uint8*)(RBASE|a);
#else
#define mark(a)
#endif

#ifdef DEBUG_PRINT_UNIT
  #define DBGUNIT(a)	PRNT(a)
#else
  #define DBGUNIT(a)	(void)0
#endif

#ifdef M2MPG_TRACE
#undef mark
#define TRACE_SIZE 1024
uint32 gTraceLog[ TRACE_SIZE ];
int32 gTraceIndex = 0;
#define mark(a) \
{ gTraceLog[ gTraceIndex++ ] = a;\
  gTraceIndex %= TRACE_SIZE;\
  gTraceLog [ gTraceIndex ] = 0xffffffffL; }
#endif

uint8 gDiagLevel = 1;

#ifdef DEBUG
  #define print(a)			Superkprintf a
#else
  #define print(a)
#endif

#define prinfo(a) if( gDiagLevel > 8 ) print(a)

static void FlushCacheLines( uint8 *address, int32 length )
{
/*	int32 i; */

	TOUCH( address );
	TOUCH( length );

	FlushDCacheAll(0);
}

#ifdef ASIC_OPERA
struct M2RegInfo
{
	int32 regNum;
	char *regName;
}	M2MPGRegisters[] =
	{	{	MPG_REG_DEVICEID,			"Device ID           "	},
		{	MPG_REG_CONFIG,				"Config              "	},
		{	MPG_REG_INTENABLE,			"Interrupt Enable    "	},
		{	MPG_REG_INTSTATUS,			"Interrupt Status    "	},
		{	MPG_REG_VBSCURADDR,			"VBS Current Address "	},
		{	MPG_REG_VBSCURLEN,			"VBS Current Length  "	},
		{	MPG_REG_VBSNEXTADDR,		"VBS Next Address    "	},
		{	MPG_REG_VBSNEXTLEN,			"VBS Next Length     "	},
		{	MPG_REG_VBSCONFIG,			"VBS Config          "	},
		{	MPG_REG_PARSERCONFIG,		"Parser Config       "	},
		{	MPG_REG_PARSERSTATUS0,		"Parser Status       "	},
		{	MPG_REG_PARSERSTATUS1,		"Parser Status 1     "	},
		{	MPG_REG_IMAGESIZE,			"Image Size          "	},
		{	MPG_REG_REVPREDBUF,			"Reverse Pred Addr   "	},
		{	MPG_REG_FWDPREDBUF,			"Forward Pred Addr   "	},
		{	MPG_REG_OUTDMACONFIG,		"Output DMA Config   "	},
		{	MPG_REG_FORMATCONFIG,		"Format Config       "	},
		{	MPG_REG_FORMATSIZE,			"Format Size         "	},
		{	MPG_REG_FORMATCROP,			"Format Crop         "	},
		{	MPG_REG_FORMATINPUT,		"Format Input Addr   "	},
		{	MPG_REG_FORMATOUTPUT,		"Format Output Addr  "	},
		{	MPG_REG_UPPERDITHERMTX,		"Upper Dither Matrix "	},
		{	MPG_REG_LOWERDITHERMTX,		"Lower Dither Matrix "	},
		{	MPG_REG_LFSR,				"LFSR                "	},
		{	MPG_REG_FORMATALPHAFILL,	"Alpha Fill          "	},
		{	MPG_REG_INTRAQUANTTABLE,	"Intra Quant Table   "	},
		{	MPG_REG_NONINTRAQUANTTABLE,	"NonIntra Quant Table"	}
	};
#endif


int32 gM2MPEGVersionInfo;

/*	write an M2 MPEG Unit register. regOffset is the byte offset
	of the register from the base of the MPEG Unit address space.
	value is a 32 bit value to write */

void M2WriteMPGRegister( uint32 regOffset, uint32 value )
{
	vuint32 *dest;

	/* regOffset is a byte offset */
	dest = (vuint32 *) ((uint8 *) MPEGUnitBaseAddr + regOffset);

	DBGUNIT(("MPGWriteMpgRegister 0x%08lx = 0x%08lx\n",dest,value));
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
	DBGUNIT(("MPGReadMPGRegister 0x%08lx = %08lx\n",src,result));
	return( result );
}

int32 M2MPGInterruptHandler( void );
Item gMPEGUnitSemaphore = -1;
Item gMPEGInterruptHandler = -1;
Item gMPEGVBDInterruptHandler = -1;

int32 M2MPEGInit( void )
{
BDAInfo bdaInfo;
Err     result;

    gRAMBase = (uint8 *) (KernelBase->kb_MemRegion)->mr_MemBase;

    /* create a semaphore to keep multiple devices from accessing the HW simultaneously */
    gMPEGUnitSemaphore = result = CreateSemaphore("MPEG Unit",0);
    if (gMPEGUnitSemaphore >= 0)
    {
        gMPEGInterruptHandler = result = CreateFIRQ("MPEG", 0, M2MPGInterruptHandler, INT_BDA_MPG );
        if (gMPEGInterruptHandler >= 0)
        {
            gMPEGVBDInterruptHandler = result = CreateFIRQ("MPEG", 0, M2MPGInterruptHandler, INT_BDA_MPGVBD );
            if (gMPEGVBDInterruptHandler >= 0)
            {
                SuperQuerySysInfo( SYSINFO_TAG_BDA, &bdaInfo, sizeof(bdaInfo));
                MPEGUnitBaseAddr = (vuint32 *) bdaInfo.bda_MPEGBase;

                /* get version info */
                gM2MPEGVersionInfo = M2ReadMPGRegister( MPG_REG_DEVICEID, M2MPGPASS1 );

                EnableInterrupt(INT_BDA_MPGVBD);
                EnableInterrupt(INT_BDA_MPG);

                return 0;
            }
            DeleteFIRQ(gMPEGInterruptHandler);
        }
        DeleteSemaphore(gMPEGUnitSemaphore);
    }

    return result;
}

int32 M2MPEGDelete(void)
{
    SuperDeleteItem(gMPEGUnitSemaphore);
    SuperDeleteItem(gMPEGInterruptHandler);
    SuperDeleteItem(gMPEGVBDInterruptHandler);
    return 0;
}

int32 M2MPEGChangeOwner( Item newOwner)
{
	SetItemOwner( gMPEGUnitSemaphore, newOwner );
	SetItemOwner( gMPEGInterruptHandler, newOwner );
	SetItemOwner( gMPEGVBDInterruptHandler, newOwner );
	return 0;
}

/* make sure all of the decode parameters are legal */

Err M2MPGValidateParameters( tVideoDeviceContext *theUnit )
{
	Err status = 0L;

	/* check for legal depth on current hardware */
	if( theUnit->operaMode )
		if( gM2MPEGVersionInfo != M2MPGPASS1 )
		{
			print(("Opera format pixels not supported on this hardware\n"));
			status = BADIOARG;
		}
	switch( theUnit->depth )
	{
		case 16:
			theUnit->pixelSize = 2L;
			break;
		case 24:
			theUnit->pixelSize = 4L;
			break;
		default:
			print(("Illegal pixel depth specification: %ld\n",theUnit->depth));
			status = BADIOARG;
	}
	/* check dimensions */
	if( (theUnit->xSize % 16 ) || (theUnit->ySize % 16) )
	{
		print(("Illegal picture dimensions, must be multiples of 16\n"));
		status = BADIOARG;
	}
	/* check cropping */
	if( (theUnit->hStart % 16) || (theUnit->vStart % 16) )
	{
		print(("Illegal crop values, must be multiples of 16\n"));
		status = BADIOARG;
	}
	else
	{
		/* convert to macroblock units */
		theUnit->hStop = (theUnit->hStart + theUnit->xSize) / 16 - 1L;
		theUnit->vStop = (theUnit->vStart + theUnit->ySize) / 16 - 1L;
		theUnit->hStart /= 16L;
		theUnit->vStart /= 16L;
	}
	/* check resampling */
	if( theUnit->resampleMode )
	{
		if( gM2MPEGVersionInfo != M2MPGPASS1 )
		{
			print(("Resampling not supported on this hardware\n"));
			status = BADIOARG;
		}
		if( theUnit->operaMode && (theUnit->depth != 24L) )
		{
			print(("Opera resampling only supported in 24 bit mode\n"));
			status = BADIOARG;
		}
		if( (theUnit->resampleMode == kCODEC_NTSC_RESAMPLE) &&
			(theUnit->xSize != 320L ) )
		{
			print(("NTSC resampling requires xSize = 320\n"));
			status = BADIOARG;
		}
		if( (theUnit->resampleMode == kCODEC_PAL_RESAMPLE) &&
			(theUnit->xSize != 384L ) )
		{
			print(("PAL resampling requires xSize = 384\n"));
			status = BADIOARG;
		}
	}
	return( status );
}

/* handle MPEG Unit interrupts */

#define DUMMYDMASIZE 128

tVideoDeviceContext *gCurrentUnit = 0L;
uint8 gM2MPGDMAEnable = 1L, gVerticalCroppingOnBPicture = 0L;
enum { no = 0, yes, handleIt } gBufferEndsOnSequenceEnd = no;
TParserStatus1Reg gSavedParserStatus1;
uint8 *curDMAEndAddr, *gSavedDMAAddr;
TBufferStatusReg gSavedVBSConfig;
uint8 gM2MPGDummyBuffer[ DUMMYDMASIZE ];

TInterruptReg gFakeInterrupt;

gPrintFlag = 0L;

int32 M2MPGInterruptHandler( void )
{
	uint32 					signals;
	TInterruptReg 			intrStatus;
	tVideoDeviceContext *	theUnit = gCurrentUnit;

	mark(16);
	DBGUNIT(("M2MPGInterrupt! gCurrentUnit = %08lx\n", theUnit));
	/* get current context */

	/* read interrupt status register */
	intrStatus.i =  M2ReadMPGRegister( MPG_REG_INTSTATUS, gFakeInterrupt.i );
	if( gPrintFlag )
		print(("intrStatus.i = 0x%08lx\n", intrStatus.i));

	/* clear the interrupt(s) */
	M2WriteMPGRegister( MPG_REG_INTSTATUS, intrStatus.i );

	/* check individual interrupts */
	signals = 0L;
	if( intrStatus.f.stripBufferError )
	{
		mark(17);
		print(("MPEG strip buffer error\n"));
		signals |= theUnit->signalStripBufferError;
	}
	if( intrStatus.f.everythingDone )
	{
		if( gM2MPGDMAEnable )
		{
			/* done with picture */
			if( gBufferEndsOnSequenceEnd == yes )
			{
				/* the buffer ends with a sequence end code */
				/* but we've gotten an all done before a vbs interrupt */
				/* this means that there was another non-slice start code */
				/* before the sequence end code, so we don't have to */
				/* handle this as a special case */
				gBufferEndsOnSequenceEnd = no;
			}
			mark(18);
			gSavedDMAAddr = 0L;
			gM2MPGDMAEnable = 0L;
			signals |= theUnit->signalEverythingDone;
		}
		else
		{
			mark(50);		/* spurious interrupt */
		}
	}
	if( intrStatus.f.outputFormatter )
	{
		mark(19);
		signals |= theUnit->signalOutputFormatter;

		/* if vertical cropping is on, and this is a B picture,
		   this interrupt is the same as an all done */
		if( gVerticalCroppingOnBPicture )
		{
			gSavedDMAAddr = 0L;
			gM2MPGDMAEnable = 0L;
			signals |= theUnit->signalEverythingDone;
		}
	}
	if( intrStatus.f.outputDMA )
	{
		mark(20);
		signals |= theUnit->signalOutputDMA;
	}
	if( intrStatus.f.bitstreamError )
	{
		mark(21);
		print(("MPEG bitstream error\n"));
		signals |= theUnit->signalBitstreamError;
	}
	if( intrStatus.f.endOfPicture )
	{
		if( gM2MPGDMAEnable )
		{
			mark(22);
			signals |= theUnit->signalEndOfPicture;
		}
		else
		{
			mark(39);		/* spurious interrupt */
		}
	}
	if( intrStatus.f.videoBitstreamDMA )
	{
		mark(23);
		if( M2ReadMPGRegister( MPG_REG_VBSCURLEN, (uint32) 0L ) != 0L )
		{
			mark(24); 	/* spurious interrupt! */
		}
		else if( gBufferEndsOnSequenceEnd == yes)
		{
			/* yes, we've hit the end of the buffer before a non-slice */
			/* start code was detected, so handle special case */
			gBufferEndsOnSequenceEnd = handleIt;

			/* provide a dummy buffer to force the start code detection */
			M2MPGContinue( gM2MPGDummyBuffer, DUMMYDMASIZE );
		}
		else if( gM2MPGDMAEnable )
		{
			mark(25);
			signals |= theUnit->signalVideoBitstreamDMA;
		}
		else if( gVerticalCroppingOnBPicture )
		{
			/* done with the picture */
			gSavedDMAAddr = curDMAEndAddr;
		}
		else
		{
			/* currently emptying internal hardware buffers */
			mark(26);
			gSavedDMAAddr = curDMAEndAddr;
			gSavedParserStatus1.i =
				M2ReadMPGRegister( MPG_REG_PARSERSTATUS1, 0L );
			gSavedVBSConfig.i = M2ReadMPGRegister( MPG_REG_VBSCONFIG, 0L );

			/* provide a dummy buffer */
			M2MPGContinue( gM2MPGDummyBuffer,  DUMMYDMASIZE );
		}
	}
	mark(27);
	/* signal the waiting task */
	if( gPrintFlag )
	{
		print(("intrStatus.i = 0x%08lx\n",intrStatus.i));
		print(("signals = 0x%08lx\n",signals));
	}
	SuperInternalSignal( theUnit->task, signals );

	return( 0L );
}

#define INTRAQUANTTABLE		0
#define NONINTRAQUANTTABLE	1

void M2MPGWriteTable( int32 whichTable, uint8 *table )
{
	uint32 i,values,regOffset;

	DBGUNIT(("loading quant table\n"));

	/* load the quant tables differently depending revision */
	if( gM2MPEGVersionInfo != M2MPGPASS1 )
	{
		/* load quant tables 4 values at a time */
		if( whichTable == INTRAQUANTTABLE )
			regOffset = MPG_REG_INTRAQUANTTABLE;
		else
			regOffset = MPG_REG_NONINTRAQUANTTABLE;

		for( i = 0L; i < 64L; i += 4L )
		{
			values = ((uint32) table[ i ] << 24) |
					 ((uint32) table[ i+1 ] << 16) |
					 ((uint32) table[ i+2 ] << 8) |
					 ((uint32) table[ i+3 ] << 0);

			M2WriteMPGRegister( regOffset, values );
			regOffset += 4L;
		}
	}
	else	/* pass 1 silicon method */
	{
		/* load quant tables one value at a time */
		if( whichTable == INTRAQUANTTABLE )
			regOffset = MPG_REG_INTRAQUANTTABLE;
		else
			regOffset = MPG_REG_INTRAQUANTTABLE + 64 * 4;

		for( i = 0L; i < 64L; i++ )
		{
			M2WriteMPGRegister( regOffset, (uint32) table[ i ] );
			regOffset += 4;
		}
	}
}

Err M2MPGEndDMA( uint8 *buf, int32 *count, uint8 **addr )
{
	int32 i;
	uint32 code = START_CODE_PREFIX,totalByteCount;
	TConfigReg configReg;
	TParserStatus0Reg parserStatus0Reg;
	TInterruptReg interruptReg;

	/* special case if the sequence end code ends the dma */
	if( gBufferEndsOnSequenceEnd == handleIt )
	{
		*buf++ = (SEQUENCE_END_CODE >> 24) & 0xffL;
		*buf++ = (SEQUENCE_END_CODE >> 16) & 0xffL;
		*buf++ = (SEQUENCE_END_CODE >> 8) & 0xffL;
		*buf   = (SEQUENCE_END_CODE) & 0xffL;
		*count = 4L;
		*addr = 0L;
		gBufferEndsOnSequenceEnd = no;
		return( 0L );
	}
	/* normal case */

	/* recover any data buffered in the hardware */
	mark(28);
	if( gSavedDMAAddr )
	{
		mark(29);
		*addr = gSavedDMAAddr;
	}
	else
	{
		mark(30);
		*addr = (uint8 *) ((uint32) gRAMBase |
				(uint32) M2ReadMPGRegister( MPG_REG_VBSCURADDR, 0L ));
		gSavedParserStatus1.i = M2ReadMPGRegister( MPG_REG_PARSERSTATUS1, 0L );
		gSavedVBSConfig.i = M2ReadMPGRegister( MPG_REG_VBSCONFIG, 0L );
	}

	/* if we're vertically cropping a B picture, don't read out */
	/* the part, it could be anywhere */
	if( gVerticalCroppingOnBPicture )
	{
		/* disable all interrupts */
		interruptReg.i = 0L;
		interruptReg.f.videoBitstreamDMA = 1;
		M2WriteMPGRegister( MPG_REG_INTENABLE, interruptReg.i );

		*count = 0L;
		gM2MPGDMAEnable = 1L;
		return( 0L );
	}

	/* recover start code */
	code |= gSavedParserStatus1.f.lastStartCode;
	for( i = 0L; i < 4; i++ )
	{
		buf[ i ] = ((uint8 *) &code)[ i ];
	}

	/* empty the parser buffer */
	i = gSavedParserStatus1.f.numBitsValid;

	/* if this is pass 1 silicon, add one to the count */
	if( gM2MPEGVersionInfo == M2MPGPASS1 )
		i += 1;
	else
		i -= 3;

	if( i % 8 )
	{
		mark(31);
		print(("Ack!: M2MPGEndDMA numBitsValid = %ld\n",i));
	}

	totalByteCount = i / 8 + gSavedVBSConfig.f.bufferByteCount;

	/* add 4 to account for the start code */
	totalByteCount += 4;

	/* add 2 to account for undocumented hw buffering (Thanks, guys!) */
	totalByteCount += 2;

	/* disable all but bitstream dma interrupt */
	interruptReg.i = 0L;
	interruptReg.f.videoBitstreamDMA = 1;
	M2WriteMPGRegister( MPG_REG_INTENABLE, interruptReg.i );

	/* single stepping the parser advances eval bits by 8 bits */
	configReg.i = M2ReadMPGRegister( MPG_REG_CONFIG, 0L );
	configReg.f.parserStep = 1;
	for( i = 4L; i < totalByteCount; i++ )
	{
		parserStatus0Reg.i = M2ReadMPGRegister( MPG_REG_PARSERSTATUS0, 0L );
		code = parserStatus0Reg.f.evalBits >> 8;
		buf[ i ] = (uint8) code;
		M2WriteMPGRegister( MPG_REG_CONFIG, configReg.i );
	}
	*count = i;

	DBGUNIT(("recovered %ld buffered bytes\n",*count));

	/* enable DMA chaining */
	gM2MPGDMAEnable = 1L;

	return( 0L );
}

Err M2MPGStartSlice(tVideoDeviceContext *theUnit, MPEGStreamInfo *si,
					uint8 *vbsBuf, int32 vbsLen)
{
	TConfigReg configReg;
	TInterruptReg interruptReg;
	TImageSizeReg sizeReg;
	TOutputDMAConfigReg outputDMAConfigReg;
	TOutputFormatConfigReg outputFormatConfigReg;
	TOutputFormatCropReg outputFormatCropReg;
	TOutputFormatAlphaFillReg outputFormatAlphaFillReg;
	TParserConfigReg parserConfigReg;
	uint32 *formatAddr = NULL,*forwardAddr = NULL,*backAddr = NULL;
	uint32 temp;

	mark(33);

	DBGUNIT(("M2MPGStartSlice\n"));
	gPictureNumber = si->pictureNumber;

	/* lock the MPEG Unit semaphore to prevent simultaneous access */
	temp = LockSemaphore( gMPEGUnitSemaphore, SEM_WAIT );
	if( temp < 1 )
	{
		PERR(("MPEG Unit semaphore lock failed\n"));
		return( temp );
	}
	/* write driver type register to ensure compatibility */
	M2WriteMPGRegister( MPG_REG_DRIVERTYPE, DRIVERTYPE_MPEG1 );

	/* disable all interrupts */
	M2WriteMPGRegister( MPG_REG_INTENABLE, 0L );

	/* reset the MPEG Unit */
	configReg.i = 0L;
	M2WriteMPGRegister( MPG_REG_CONFIG, configReg.i );
	configReg.f.parserReset_n = 1;
	configReg.f.parserEnable = 1;
	M2WriteMPGRegister( MPG_REG_CONFIG, configReg.i );
	configReg.f.parserReset_n = 0;
	configReg.f.parserEnable = 0;
	M2WriteMPGRegister( MPG_REG_CONFIG, configReg.i );

	/* load the quantization matrices */
	M2MPGWriteTable( INTRAQUANTTABLE, si->intraQuantMatrix );
	M2MPGWriteTable( NONINTRAQUANTTABLE, si->nonIntraQuantMatrix );

	/* size register */
	DBGUNIT(("vSize = %ld, hSize = %ld\n", SEQ_HEIGHT(si), SEQ_WIDTH(si)));
	sizeReg.i = 0L;
    sizeReg.f.mbHeight = (SEQ_HEIGHT(si) + 15) >> 4;
    sizeReg.f.mbWidth = (SEQ_WIDTH(si) + 15) >> 4;
	DBGUNIT(("sizeReg.f.mbHeight = %ld, sizeReg.f.mbWidth = %ld\n",
		   sizeReg.f.mbHeight, sizeReg.f.mbWidth));
	DBGUNIT(("sizeReg.i = 0x%08lx\n", sizeReg.i));
	M2WriteMPGRegister( MPG_REG_IMAGESIZE, sizeReg.i );

	/* set up reference picture and output addresses */
	outputDMAConfigReg.i = 0L;
	switch(PICT_TYPE(si))
	{
		case 1: /* I picture */
			mark(34);
		case 2: /* P picture */
			mark(35);
		    if (si->parseIFrameOnly) {
				/* Do same as a B picture. Decode to the strip buffer. */
				outputDMAConfigReg.i = (uint32) theUnit->stripBuffer->address;
				formatAddr = theUnit->stripBuffer->address;
			} else {
				outputDMAConfigReg.i = (uint32)
                                       theUnit->refPic[theUnit->nextRef];
				theUnit->nextRef ^= 1;
				forwardAddr = theUnit->refPic[theUnit->nextRef];
				formatAddr = forwardAddr;
			}
			break;
		case 3: /* B picture */
			mark(36);
			outputDMAConfigReg.i = (uint32) theUnit->stripBuffer->address;
			forwardAddr = theUnit->refPic[ theUnit->nextRef ];
			backAddr = theUnit->refPic[ theUnit->nextRef ^ 1L ];
			formatAddr = theUnit->stripBuffer->address;
			break;
		default:
			mark(37);
			print(("BAD PICTURE TYPE\n"));
			break;
	}
    /* only write the addresses necessary for the picture type */
    switch (PICT_TYPE(si))
    {
        case 3: /* reverse prediction for B pictures only */
			DBGUNIT(("MPEG_REG_REVPREDBUF = 0x%08lx\n",backAddr));
            M2WriteMPGRegister( MPG_REG_REVPREDBUF, (uint32) backAddr );
			outputDMAConfigReg.f.stripBufferEnable = 1;
			outputDMAConfigReg.f.stripBufferSize = theUnit->stripBuffer->sizeCode;
        case 2: /* forward prediction for B or P pictures */
			DBGUNIT(("MPEG_REG_FWDPREDBUF = 0x%08lx\n",forwardAddr));
            M2WriteMPGRegister( MPG_REG_FWDPREDBUF, (uint32) forwardAddr );
        case 1: /* decoder output for B, P, and I pictures */
			DBGUNIT(("MPEG_REG_OUTDMACONFIG = 0x%08lx\n",outputDMAConfigReg.i));
		    if (si->parseIFrameOnly) {
				/* In this mode we are decoding into the strip buffer.
				 * So it should be enabled.
				 */
				outputDMAConfigReg.f.stripBufferEnable = 1;
				outputDMAConfigReg.f.stripBufferSize = theUnit->stripBuffer->sizeCode;
			}
            M2WriteMPGRegister( MPG_REG_OUTDMACONFIG, outputDMAConfigReg.i );
    }
	/* set up the output formatter */
	outputFormatConfigReg.i = 0L;
	outputFormatConfigReg.f.operaFormat = theUnit->operaMode;
	if( (theUnit->depth == 16L) || (theUnit->depth == -16L) )
		outputFormatConfigReg.f.format = 1;
	outputFormatConfigReg.f.resamplerMode = theUnit->resampleMode;
	outputFormatConfigReg.f.enableCSC = theUnit->outputRGB;
	outputFormatConfigReg.f.rowChunks =
		(unsigned) (theUnit->xSize * theUnit->pixelSize) >> 5;
	M2WriteMPGRegister( MPG_REG_FORMATCONFIG, outputFormatConfigReg.i );

	/* the input to the formatter has been set up above by picture type */
	M2WriteMPGRegister( MPG_REG_FORMATINPUT, (uint32) formatAddr );

	/* use the same size values for the output formatter input */
	M2WriteMPGRegister( MPG_REG_FORMATSIZE, sizeReg.i );

	/* output a picture to the user's buffer */
	DBGUNIT(("MPEG_REG_FORMATOUTPUT = 0x%08lx\n",theUnit->destBuffer));
	M2WriteMPGRegister( MPG_REG_FORMATOUTPUT, (uint32) theUnit->destBuffer );

	/* crop the picture to the right dimensions */
	outputFormatCropReg.i = 0L;
	outputFormatCropReg.f.hStart = (unsigned) theUnit->hStart;
	outputFormatCropReg.f.vStart = (unsigned) theUnit->vStart;
	temp = ((SEQ_WIDTH(si) + 15) >> 4) - 1;
	if( theUnit->hStop > temp )
		outputFormatCropReg.f.hStop = temp;
	else
		outputFormatCropReg.f.hStop = (unsigned) theUnit->hStop;
	temp = ((SEQ_HEIGHT(si) + 15) >> 4) - 1;
	if( theUnit->vStop > temp )
		outputFormatCropReg.f.vStop = temp;
	else
		outputFormatCropReg.f.vStop = (unsigned) theUnit->vStop;
	DBGUNIT(("outputFormatCropReg = 0x%08lx\n",outputFormatCropReg.i));
    M2WriteMPGRegister( MPG_REG_FORMATCROP, outputFormatCropReg.i );

	/* if this is a B picture and we're cropping, need to handle special */
	if( (PICT_TYPE(si) == 3) &&
		((outputFormatCropReg.f.vStop+1) < sizeReg.f.mbHeight) )
		gVerticalCroppingOnBPicture = 1L;
	else
		gVerticalCroppingOnBPicture = 0L;


    /* set up the parser configuration register */
    parserConfigReg.i = 0L;
    parserConfigReg.f.fullPelBackwardVector = si->fullPelBackwardVector;
    parserConfigReg.f.backwardRSize = si->backwardFCode - 1;
    parserConfigReg.f.fullPelForwardVector = si->fullPelForwardVector;
    parserConfigReg.f.forwardRSize = si->forwardFCode - 1;
    parserConfigReg.f.pictureCodingType = PICT_TYPE(si);
	DBGUNIT(("parserConfigReg = 0x%08lx\n",parserConfigReg.i));
    M2WriteMPGRegister( MPG_REG_PARSERCONFIG, parserConfigReg.i );

	/* dither matrix */
	if( (theUnit->depth == 16L) || (theUnit->depth == -16L) )
	{
		M2WriteMPGRegister( MPG_REG_UPPERDITHERMTX, theUnit->dither[ 0 ] );
		M2WriteMPGRegister( MPG_REG_LOWERDITHERMTX, theUnit->dither[ 1 ] );
	}
	else
	{
		M2WriteMPGRegister( MPG_REG_UPPERDITHERMTX, 0L );
		M2WriteMPGRegister( MPG_REG_LOWERDITHERMTX, 0L );
	}
	/* alpha fill register */
	outputFormatAlphaFillReg.i = 0L;
	outputFormatAlphaFillReg.f.DSB = 1; /* theUnit->DSB; */
	outputFormatAlphaFillReg.f.alphaFillValue = theUnit->alphaFill;
	M2WriteMPGRegister( MPG_REG_FORMATALPHAFILL, outputFormatAlphaFillReg.i );

	/* clear interrupt status reg */
	interruptReg.i = 0L;
	interruptReg.f.stripBufferError = interruptReg.f.everythingDone =
	interruptReg.f.outputFormatter = interruptReg.f.outputDMA =
	interruptReg.f.bitstreamError = interruptReg.f.endOfPicture =
	interruptReg.f.videoBitstreamDMA = 1;

	M2WriteMPGRegister( MPG_REG_INTSTATUS, interruptReg.i );

    /* enable the interrupts */
	M2WriteMPGRegister( MPG_REG_INTENABLE, interruptReg.i );

	/* clear bitstream length just in case some junk was left over */
	M2WriteMPGRegister( MPG_REG_VBSNEXTLEN, (uint32) 0L );
	M2WriteMPGRegister( MPG_REG_VBSCURLEN, (uint32) 0L );

	/* enable bitstream dma chaining */
	gM2MPGDMAEnable = 1L;

    /* fire it up */
	gCurrentUnit = theUnit;			/* set context for interrupt handler */
	configReg.i = 0L;
	configReg.f.vofRdEnable = configReg.f.vofWrEnable =
	configReg.f.vofReset_n = configReg.f.vodEnable =
	configReg.f.vodReset_n = configReg.f.motEnable =
	configReg.f.motReset_n = configReg.f.mvdReset_n =
	configReg.f.parserStep = configReg.f.parserEnable =
	configReg.f.parserReset_n = configReg.f.vbdEnable =
	configReg.f.vbdReset_n = 1;

    M2WriteMPGRegister( MPG_REG_CONFIG, configReg.i);

	/* must set up input bitstream DMA registers after enable, thanks Jim */
	mark(38);
	FlushCacheLines( vbsBuf, vbsLen );
	M2WriteMPGRegister( MPG_REG_VBSNEXTADDR, (uint32) vbsBuf );
	M2WriteMPGRegister( MPG_REG_VBSNEXTLEN, (uint32) vbsLen );
	curDMAEndAddr = vbsBuf + vbsLen;

	return( 0L );
}

Err M2MPGEndSlice()
{
	return( UnlockSemaphore( gMPEGUnitSemaphore ) );
}

Err M2MPGContinue( uint8 *vbsBuf, int32 vbsLen )
{
	int32 i, limit;
	/* code must be static in case start code crosses buffer boundary */
	static uint32 code = 0xffffffffL;

	/* special case sequence end */
	/* this is ugly: if there is a sequence end code */
	/* near the end of the buffer, the hardware won't detect it */
	/* until the next buffer is sent. This may cause pictures to */
	/* be delivered to late to be displayed, e.g. for VideoCD stills */
	/* so we handle this case specially. */
	if( vbsLen < 8 )
		limit = vbsLen;			/* carry over code from last dma */
	else
	{
		limit = 8;
		code = 0xffffffffL;		/* clear code */
	}
	for( i = vbsLen - limit; i < vbsLen; i++ )
	{
		code = (code << 8) | vbsBuf[ i ];

		if( code == SEQUENCE_END_CODE )
		{
			gBufferEndsOnSequenceEnd = yes;
			break;
		}
	}
	/* write input bitstream DMA registers */
	mark(32);
	FlushCacheLines( vbsBuf, vbsLen );
	M2WriteMPGRegister( MPG_REG_VBSNEXTADDR, (uint32) vbsBuf );
	M2WriteMPGRegister( MPG_REG_VBSNEXTLEN, (uint32) vbsLen );
	curDMAEndAddr = vbsBuf + vbsLen;

	return( 0L );
}

/* print out state */

#ifndef DEBUG

Err M2MPGDumpState( void) { return 0; }

#else

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

	if( gDiagLevel == 1 )
	{
		tempReg = M2ReadMPGRegister( MPG_REG_VBSNEXTADDR, 0L );
		print(("pic: %ld buf: 0x%08lx ",gPictureNumber, tempReg));

		tempReg = M2ReadMPGRegister( MPG_REG_VBSCURADDR, 0L );
		bufferStatusReg.i = M2ReadMPGRegister( MPG_REG_VBSCONFIG, 0L );
		tempReg -= bufferStatusReg.f.bufferByteCount;

		parserStatus0Reg.i = M2ReadMPGRegister( MPG_REG_PARSERSTATUS0, 0L );
		parserStatus1Reg.i = M2ReadMPGRegister( MPG_REG_PARSERSTATUS1, 0L );
		tempReg -= (parserStatus1Reg.f.numBitsValid + 7) / 8;
		tempReg -= 2;
		print(("addr: 0x%08lx ",tempReg));
		print(("ps0: 0x%08lx ",parserStatus0Reg.i));
		print(("ps1: 0x%08lx\n",parserStatus1Reg.i));

		return( 0 );
	}
	if( gDiagLevel > 4 )
	{
		print(("Dumping MPEG Unit State:\n"));
		print(("\n\tPicture Number = %ld\n\n",gPictureNumber));
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
	}
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

	if( gDiagLevel > 4 )
	{
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
	}
	print(("\n\tParser Status Registers\n"));
	parserStatus0Reg.i = M2ReadMPGRegister( MPG_REG_PARSERSTATUS0, 0L );
	print(("\t\tmbRow          = %d\n",parserStatus0Reg.f.mbRow ));
	print(("\t\tbitstreamError = %d\n",parserStatus0Reg.f.bitstreamError ));
	print(("\t\terrorState     = %d\n",parserStatus0Reg.f.errorState ));
	print(("\t\tevalBits       = 0x%04x\n",parserStatus0Reg.f.evalBits ));
	parserStatus1Reg.i = M2ReadMPGRegister( MPG_REG_PARSERSTATUS1, 0L );
	print(("\t\tblockNumber    = %d\n",parserStatus1Reg.f.blockNumber ));
	print(("\t\tmacroBlockType = %d\n",parserStatus1Reg.f.macroBlockType ));
	print(("\t\tnumBitsValid   = %d\n",parserStatus1Reg.f.numBitsValid ));
	print(("\t\tlastStartCode  = %d\n",parserStatus1Reg.f.lastStartCode ));
	print(("\t\tmbCol          = %d\n",parserStatus1Reg.f.mbCol ));

	if( gDiagLevel > 4 )
	{
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
		print(("\t\tvofSnoopEnable = %d\n",outputFormatConfigReg.f.vofSnoopEnable));
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
	}
#ifdef NEVER
	/* can't read quant tables!!!!! (hw engineers, sheesh!) */
	print(("\n\tQuant Tables"));
	for( i = 0; i < 64; i++ )
	{
		if( (i % 8) == 0 )
			print(("\n"));
		tempReg = M2ReadMPGRegister( MPG_REG_INTRAQUANTTABLE + 4*i, 0L );
		print(("%08lx ",tempReg));
	}
#endif
	return( 0L );
}

#endif /* DEBUG */
