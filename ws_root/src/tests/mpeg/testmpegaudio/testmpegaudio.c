/******************************************************************************
**
**  @(#) testmpegaudio.c 96/04/08 1.5
**
******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <misc/mpaudiodecode.h>
#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/device.h>
#include <kernel/devicecmd.h>
#include <kernel/item.h>
#include <kernel/io.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/time.h>
#include <audio/audio.h>
#include <audio/soundspooler.h>
#include <file/fileio.h>
#include <file/filesystem.h>
#include <misc/event.h>
#include "testmpegaudio.h"

/*#define DEBUG_PRINT 1*/
/* #define PERFTEST 1 */
/*#define SOUNDFILE 1*/
/*#define COMPARE 1*/
#if PERFTEST
#undef DEBUG_PRINT
#undef SOUNDFILE
#undef COMPARE
#endif
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#ifdef DEBUG_PRINT
#define	DBUG(x)	PRT(x)
#else
#define	DBUG(x)
#endif

/* Macros to simplify error checking. */
#define CHECKRESULT(val,name,stage) \
    { \
	if (val < 0) { \
	    Result = val; \
	    PrintError(0,name,NULL,Result); \
	    mpCleanup(); \
	} \
	CleanupMask |= stage; \
    }
#define CHECKPOINTER(val,name,stage) \
    { \
        if (val == NULL) { \
	    PrintError(0,name,NULL,NULL); \
	    mpCleanup(); \
	} \
	CleanupMask |= stage; \
    }

#define DEFAULT_BSFILENAME   "envogue.mpa"
#define DEFAULT_AUDFILENAME  "envogue.dec"
#define DEFAULT_COMPFILENAME "envogue.comp"
#define NUMAUDBUFS           (8L)
#define AUDBUFSIZE           (1152L * 4L)
#define TOTALAUDSIZE         (NUMAUDBUFS * AUDBUFSIZE)

Err MPAGetCompressedBfr( const void *unit, const uint8 **buf, int32 *len, uint32 *pts, uint32 *ptsIsValidFlag );
Err MPACompressedBfrRead( const void *unit, uint8 *buf );
Err spoolDecompressedData( void );
Err NextDecompressedBfr( const uint32 **buf );

/* this is a program for testing the mpeg audio decoder. */
uint8       *bsBuffer;
uint32       *compBuffer;
#ifdef COMPARE
int32        log_diff[17];
RawFile      *theCompStream;
#endif
int32        bsSize;
int32        fileSize;
int32        read_count;
int32        write_count;
TimerTicks   start_time;
TimerTicks   end_time;
TimerTicks   total_time;
int32        playback_start;
RawFile      *theMPEGStream;
#ifdef SOUNDFILE
RawFile      *theAudioStream;
#endif
FileInfo     streamInfo;
TimeVal      tv;
Err          err;
Item         OutputIns;
Item         SamplerIns;
int32        Result;
int32        CleanupMask;
int32        SignalMask;
SoundSpooler *sspl;
uint32       *audBuffer[NUMAUDBUFS];
int32        BufSignal[NUMAUDBUFS];
int32        NextSignals, CurSignals;
int32        BufIndex;
ControlPadEventData cped;
uint32       pts;
uint32        ptsIsValid;
AudioHeader  header;
uint32       *decompressedBuf; 


int main( int argc, char *argv[] )
{
  char	*bsfilename;
#ifdef SOUNDFILE
  char	*audfilename;
#endif
#ifdef COMPARE
  char	*compfilename;
#endif
  int32 arg, i;
  MPACallbackFns    CBFns;
  MPADecoderContext *ctx;
  Err				status;

#ifdef MEMDEBUG
	CreateMemDebug( NULL );
#endif

  /* initialize some count stuff */
  bsfilename = DEFAULT_BSFILENAME;
#ifdef SOUNDFILE
  audfilename = DEFAULT_AUDFILENAME;
#endif
#ifdef COMPARE
  compfilename = DEFAULT_COMPFILENAME;
  for (i=0; i<17; i++) {
    log_diff[i] = 0;
  }
#endif
  read_count = 0;
  write_count = 0;
  playback_start = 0;
  BufIndex = 0;
  CleanupMask = 0;
  SignalMask = 0;
  CurSignals = 0;
  OutputIns = 0;
  SamplerIns = 0;
  Result = -1;
  sspl = NULL;

  memset( BufSignal, 0, sizeof(BufSignal) );

  CBFns.GetCompressedBfrFn = (MPAGetCompressedBfrFn)MPAGetCompressedBfr;
  CBFns.CompressedBfrReadFn = (MPACompressedBfrReadFn)MPACompressedBfrRead;

#ifdef DEBUG_AUDIO
  printf("MPAudioDecode(): CBFns @ 0x%x, size is %ld\n", &CBFns, sizeof(CBFns) );
  printf("MPAudioDecode(): MPAGetCompressedBfr @ 0x%x\n", CBFns.GetCompressedBfr );
  printf("MPAudioDecode(): MPACompressedBfrRead @ 0x%x\n", CBFns.CompressedBfrReadFn );
#endif

  /* clear audio buffers */
  memset (audBuffer, 0, sizeof (audBuffer));
  DBUG(("audBuffer array @ %x, size is %d\n", audBuffer, sizeof(audBuffer)));

  /* parse command line arguments */
  for( arg = 1; arg < argc; arg++ ) {
    if( (strcmp(argv[ arg ], "-f") == 0L) && (argc > arg+1) )
      bsfilename = argv[ ++arg ];
#ifdef SOUNDFILE
    else if( (strcmp(argv[ arg ], "-o") == 0L) && (argc > arg+1) )
      audfilename = argv[ ++arg ];
#endif
#ifdef COMPARE
    else if( (strcmp(argv[ arg ], "-c") == 0L) && (argc > arg+1) )
      compfilename = argv[ ++arg ];
#endif
    else {
      PRT(("usage: %s \n", argv[0]));
      PRT(("    [-f filename]       - MPEG source file (DEFAULT %s).\n", DEFAULT_BSFILENAME));
#ifdef SOUNDFILE
      PRT(("    [-o filename]       - Audio output file (DEFAULT %s).\n", DEFAULT_AUDFILENAME));
#endif
#ifdef COMPARE
      PRT(("    [-c filename]       - Audio compare file (DEFAULT %s).\n", DEFAULT_COMPFILENAME));
#endif
      exit(0);
    }
  }
  DBUG(("Bitstream filename is %s\n", bsfilename));
#ifdef SOUNDFILE
  DBUG(("Audio filename is %s\n", audfilename));
#endif
#ifdef COMPARE
  DBUG(("Audio compare filename is %s\n", compfilename));
#endif

  /* open the bitstream file */
  Result = OpenRawFile(&theMPEGStream, bsfilename, FILEOPEN_READ);
  CHECKRESULT (Result, "open bitstream file", CLNUP_BSFILE);
  DBUG(("opened file %s\n", bsfilename));

  /* get the size of the file */
  Result = GetRawFileInfo(theMPEGStream, &streamInfo, sizeof(FileInfo));
  CHECKRESULT (Result, "get file info", 0);
  fileSize = streamInfo.fi_ByteCount;
  DBUG(("file is %d bytes long\n", fileSize));

  /* Allocate a bitstream buffer of the same size*/
  bsBuffer = (uint8 *) AllocMem( fileSize, MEMTYPE_NORMAL );
  CHECKPOINTER (bsBuffer, "alloc bitstream mem", CLNUP_BSBUF);
  DBUG(("Alloc'd bsBuffer (%ld bytes) at %08lx\n", fileSize, bsBuffer));

  /* read in the entire stream */
  bsSize = ReadRawFile( theMPEGStream, (uint32 *) bsBuffer, fileSize);
  CHECKRESULT (bsSize, "reading file", 0);
  if (fileSize != bsSize) {
    ERR(("Sizes from GetRawFileInfo (%d) and ReadRawFile (%d) don't match\n", fileSize, bsSize));
    mpCleanup();
  }
  DBUG(("%ld bytes of stream has been read into memory\n", bsSize));

#ifndef PERFTEST
#ifdef SOUNDFILE
  /* open an audio dump file */
  Result = OpenRawFile(&theAudioStream, audfilename, FILEOPEN_WRITE_NEW);
  CHECKRESULT (Result, "open audio output file", CLNUP_AOUTFILE);
  DBUG(("opened file %s\n", audfilename));
#endif
#ifdef COMPARE
  /* open an audio input comparison file */
  Result = OpenRawFile(&theCompStream, compfilename, FILEOPEN_READ);
  CHECKRESULT (Result, "open audio comparison file", CLNUP_ACMPFILE);
  DBUG(("opened file %s\n", compfilename));

  /* Allocate a compare buffer of the appropriate size*/
  compBuffer = (uint32 *) AllocMem( AUDBUFSIZE, MEMTYPE_NORMAL );
  CHECKPOINTER (compBuffer, "alloc compare mem", CLNUP_CMPBUF);
  DBUG(("Alloc'd compBuffer (%ld bytes) at %08lx\n", AUDBUFSIZE, compBuffer));
#endif
#ifndef SOUNDFILE
#ifndef COMPARE
  /* Initialize the EventBroker. */
  Result = InitEventUtility(1, 0, LC_ISFOCUSED);
  CHECKRESULT (Result, "init EventUtility", CLNUP_EVENT);

  /* Initialize audio, return if error. */
  Result = OpenAudioFolio();
  CHECKRESULT (Result, "open audio folio", CLNUP_AUDFOLIO);

  /* Use directout instead of mixer. */
  OutputIns = LoadInstrument("line_out.dsp",  0, 100);
  CHECKRESULT(OutputIns,"LoadInstrument", CLNUP_OUTINS);
  Result = StartInstrument( OutputIns, NULL );
  CHECKRESULT(Result,"StartInstrument: OutputIns", 0);

  /* Load fixed rate stereo sample player instrument */
  SamplerIns = LoadInstrument("sampler_16_f2.dsp",  0, 100);
  CHECKRESULT(SamplerIns,"LoadInstrument", CLNUP_SAMPINS);

  /* Connect Sampler Instrument to DirectOut */
  Result = ConnectInstrumentParts (SamplerIns, "Output", 0, OutputIns, "Input", 0);
  CHECKRESULT(Result,"ConnectInstruments", 0);
  Result = ConnectInstrumentParts (SamplerIns, "Output", 1, OutputIns, "Input", 1);
  CHECKRESULT(Result,"ConnectInstruments", 0);

  /* Create SoundSpooler data structure. */
  sspl = ssplCreateSoundSpooler( NUMAUDBUFS, SamplerIns );
  CHECKPOINTER (sspl, "ssplCreateSoundSpooler", CLNUP_SPOOL);
  DBUG(("sound spooler address @ %x\n", sspl));
#endif
#endif
#endif

  /* Allocate sound buffers */
  for( i=0; i<NUMAUDBUFS; i++ ) {
    audBuffer[i] = (uint32 *) AllocMem( AUDBUFSIZE, MEMTYPE_NORMAL );
    CHECKPOINTER (audBuffer[i], "Alloc audio mem", CLNUP_AUDBUF);
    DBUG(("Allocated sound buffer %d @ %x\n", i, audBuffer[i]));
  }

#ifdef DEBUG_AUDIO
  printf("Calling CreateMPAudioDecoder()\n");
#endif

	status = CreateMPAudioDecoder( &ctx, CBFns );
	FAIL_NEG( "CreateMPAudioDecoder", status );

	PRT(("Beginning decode\n"));
	/*	mpBreakPoint();*/

	/* Sample the start time */
	SampleSystemTimeTT(&start_time);

#ifdef DEBUG
	ConvertTimerTicksToTimeVal(&start_time, &tv);
	DBUG(("start_time = %d.%06d seconds\n", tv.tv_Seconds, tv.tv_Microseconds));
#endif

	while ( bsSize > 0 )
	{
		status = NextDecompressedBfr( &decompressedBuf );
		CHECK_NEG("NextDecompressedBfr", status);

		/* start decoding */
		status = MPAudioDecode ( NULL, ctx, &pts, &ptsIsValid,
			decompressedBuf, &header );
		CHECK_NEG( "MPAudioDecode", status );

		if( status >= 0 )
		{
			status = spoolDecompressedData();
			CHECK_NEG("spoolDecompressedData", status );
		} /* if( status ) */
		else if( status == MPAEOFErr )
			break;
	}

	DBUG(("done parsing all the frames that we can\n"));
#ifndef PERFTEST
#ifndef SOUNDFILE
#ifndef COMPARE
	/* wait until done playing */
	DBUG(("Wait for buffers to finish playing\n"));
	while ((CurSignals | BufSignal[BufIndex]) != SignalMask) {
		DBUG(("WaitSignal(0x%x)\n", SignalMask ));
		NextSignals = WaitSignal( SignalMask );
		DBUG(("WaitSignal() got 0x%x\n", NextSignals ));
		CurSignals |= NextSignals;

		/* Tell sound spooler that the buffer(s) have completed. */
		Result = ssplProcessSignals( sspl, NextSignals, NULL );
		CHECKRESULT(Result,"ssplProcessSignals", 0);
      CurSignals |= NextSignals;
	}
#endif
#endif
#endif
	/* Sample the end time */
	SampleSystemTimeTT(&end_time);

	/* calculate the difference between the start and end times */
	SubTimerTicks(&start_time, &end_time, &total_time);

	/* convert the difference to TimeVal format */
	ConvertTimerTicksToTimeVal(&total_time, &tv);
	PRT(("total_time = %d.%06d seconds\n", tv.tv_Seconds, tv.tv_Microseconds));

	if (write_count)
		PRT(("CPU usage = %f\n", (((float)tv.tv_Seconds +
			((float)tv.tv_Microseconds/100000.0)) * 44100.0)/
			((float) write_count)));
#ifdef COMPARE
  {
  uint32 i;

	for (i=0; i<17; i++)
	{
		PRT(("# of diffs with MSB %d:		%d\n", i-1, log_diff[i]));
	}
  }
#endif

FAILED:


#ifdef MEMDEBUG
	DumpMemDebug( NULL );
	DeleteMemDebug( );
#endif

#ifdef DEBUG_AUDIO
  printf("Calling DeleteMPAudioDecoder()\n");
#endif

  status = DeleteMPAudioDecoder( ctx );
  CHECK_NEG( "DeleteMPAudioDecoder", status );

  mpCleanup();

  return (0);

} /* main() */


Err NextDecompressedBfr( const uint32 **buf )
{

	*buf = audBuffer[BufIndex];

#ifndef PERFTEST
#ifndef SOUNDFILE
#ifndef COMPARE
	if (playback_start)
	{
		if (CurSignals & BufSignal[BufIndex])
		{
			/* buffer available, clear the signal bit */
			CurSignals &= ~BufSignal[BufIndex];
			DBUG(("Buffer %d is available\n", BufIndex));
		}
		else
		{
			DBUG(("Wait for buffer %d to become available\n", BufIndex));
			DBUG(("WaitSignal(0x%x)\n", SignalMask ));
			NextSignals = WaitSignal( SignalMask );
			DBUG(("WaitSignal() got 0x%x\n", NextSignals ));

			/* Tell sound spooler that the buffer(s) have completed. */
			Result = ssplProcessSignals( sspl, NextSignals, NULL );
			CHECKRESULT(Result,"ssplProcessSignals", 0);
			CurSignals |= NextSignals;
			/* check to make sure your buffer is one of those freed up */
			if (CurSignals & BufSignal[BufIndex])
			{
				/* buffer available, clear the signal bit */
				CurSignals &= ~BufSignal[BufIndex];
				DBUG(("Buffer %d is available\n", BufIndex));
			}
			else
			{
				/* houston, we have a problem */
				DBUG(("spooler is screwing up the order of operation\n"));
				mpCleanup();
			} /* if (CurSignals & BufSignal[BufIndex]) */ 

		} /* if (CurSignals & BufSignal[BufIndex]) */
	} /* if (playback_start) */
#endif
#endif
#endif

  return 0;
}

int32 MPAGetCompressedBfr( const void *unit, const uint8 **buf, int32 *len, uint32 *pts, uint32 *ptsIsValidFlag )
{
  TOUCH(unit);					/* used in real app */
  TOUCH(pts);					/* used in real app */
  TOUCH(ptsIsValidFlag);		/* used in real app */

#ifdef DEBUG_AUDIO
  printf("MPAGetCompressedBfr(): entered\n");
  printf("MPAGetCompressedBfr(): len @  0x%x = %ld \n", len, *len );
  printf("MPAGetCompressedBfr(): buf @ 0x%x = %ld\n", buf, *buf);
#endif

  if (read_count) {
    *len = 0;
    *buf = NULL;
    bsSize = 0;
	return MPAEOFErr;
  }

  else
  {
    *len = bsSize;
    *buf = (uint8 *) bsBuffer;
    DBUG(("first call to MPAGetCompressedBfr\n"));
  }

  read_count++;

  return 0;
}


Err MPACompressedBfrRead( const void *theUnit, uint8 *buf )
{
  TOUCH(theUnit); 			/* used in real app */
  TOUCH(buf); 				/* used in real app */

#ifdef DEBUG_AUDIO
  printf("MPACompressedBfrRead(): entered\n");
  printf("MPACompressedBfrRead(): read_count= %d\n", read_count);
#endif

  if (read_count) DBUG(("done parsing this buffer\n"));
  return 0;
}

Err spoolDecompressedData( void )
{

#ifdef DEBUG_AUDIO
	printf("spoolDecompressedData(): entered\n");
#endif

#ifdef COMPARE
	int32 i, diff;
#endif

#ifndef PERFTEST
#ifndef SOUNDFILE
#ifndef COMPARE
	if (!playback_start)
	{
		DBUG(("not playing, done buffer %d beginning @ %x\n", BufIndex, audBuffer[BufIndex]));
		/* mpBreakPoint();*/
		/* Dispatch buffers full of sound to spooler. ssplSpoolData returns *
		 * a signal which can be checked to see when the data has completed *
		 * it playback. If it returns 0, there were no buffers available.	 */
		Result = ssplSpoolData( sspl, (uint8 *)audBuffer[BufIndex], AUDBUFSIZE, NULL );
		CHECKRESULT (Result, "spool data", 0);
		if (!Result)
		{
			ERR(("Out of buffers\n"));
			mpCleanup();
		}
		else
		{
			DBUG(("buffer %d mask is %x\n", BufIndex, Result));
		} /* if-else (!Result) */

		BufSignal[BufIndex] = Result;
		SignalMask |= BufSignal[BufIndex];
		DBUG(("signalmask is now %x\n", SignalMask));
		BufIndex++;
		if(BufIndex >= NUMAUDBUFS)
		{
			DBUG(("we have %d buffers decoded, now we will start spooler\n", NUMAUDBUFS));
			BufIndex = 0;
			playback_start = 1;
#if DEBUG_PRINT
			ssplDumpSoundSpooler (sspl);
#endif
			/* Start Spooler instrument. Will begin playing any queued buffers. */
			Result = ssplStartSpoolerTagsVA (sspl,
				AF_TAG_AMPLITUDE_FP, ConvertFP_TagData(1.0),
				TAG_END);
			CHECKRESULT(Result,"ssplStartSpooler", CLNUP_STOPSPOOL);
		} /* if (BufIndex >= NUMAUDBUFS) */
	} /* if (!playback_start) */
	else
	{ /* playback started */
		/* ssplSpoolData will return positive signals as long as
		 * it accepted the data. */
		Result = ssplSpoolData( sspl, (uint8 *)audBuffer[BufIndex], AUDBUFSIZE, NULL );
		CHECKRESULT (Result, "spool data", 0);
		if (!Result)
		{
			ERR(("We waited for one buffer, got none\n"));
			mpCleanup();
		} /* if (!Result) */


#endif /* COMPARE */
#endif /* SOUNDFILE */
#endif /* PERFTEST */

#ifdef SOUNDFILE
	/* write buffer into audio file */
	Result = WriteRawFile( theAudioStream, (int16 *)audBuffer[BufIndex], AUDBUFSIZE);
	CHECKRESULT (Result, "writing audio file", 0);
	if (Result != AUDBUFSIZE)
	{
		ERR(("Couldn't write all bytes.	Tried %d, wrote %d\n", AUDBUFSIZE, Result));
		mpCleanup();
	} /* if (Result != AUDBUFSIZE) */
	DBUG(("%ld bytes of audio has been written into a file\n", Result));
#endif /* SOUNDFILE */

#ifdef COMPARE
	/* read data into audio compare buffer */
	Result = ReadRawFile( theCompStream, (int16 *) compBuffer, AUDBUFSIZE);
	CHECKRESULT (Result, "reading compare file", 0);
	/* compare data in buffer with data in files */
	for (i=0; i<(Result>>1); i++)
	{ /* work with int16's */
		diff = *(audBuffer[BufIndex]+i) - *(compBuffer+i);
		if (diff < 0) diff = -diff;
		if (diff == 0x0) log_diff[0]++;
		else if (diff < 0x2) log_diff[1]++;
		else if (diff < 0x4) log_diff[2]++;
		else if (diff < 0x8) log_diff[3]++;
		else if (diff < 0x10) log_diff[4]++;
		else if (diff < 0x20) log_diff[5]++;
		else if (diff < 0x40) log_diff[6]++;
		else if (diff < 0x80) log_diff[7]++;
		else if (diff < 0x100) log_diff[8]++;
		else if (diff < 0x200) log_diff[9]++;
		else if (diff < 0x400) log_diff[10]++;
		else if (diff < 0x800) log_diff[11]++;
		else if (diff < 0x1000) log_diff[12]++;
		else if (diff < 0x2000) log_diff[13]++;
		else if (diff < 0x4000) log_diff[14]++;
		else if (diff < 0x8000) log_diff[15]++;
		else
		{
			PRT(("diff of %x found at %d offset in frame %d\n", diff, i, write_count/1152));
			PRT(("generated %x, compared to %x\n", *(audBuffer[BufIndex]+i), *(compBuffer+i)));
			log_diff[16]++;
		} /* else */
	} /* for */
	/* check if we're done */
	if (Result != AUDBUFSIZE)
	{
		PRT(("End of file reached\n"));
		for (i=0; i<17; i++)
		{
			PRT(("# of diffs with MSB %d:		%d\n", i-1, log_diff[i]));
		} /* for (i=0; i<17; i++) */
		mpCleanup();
	} /* if (Result != AUDBUFSIZE) */
	DBUG(("%ld bytes of audio data compared\n", Result));
#endif /* COMPARE */

	BufIndex++;
	if(BufIndex >= NUMAUDBUFS)
		BufIndex = 0;

#ifndef PERFTEST
#ifndef SOUNDFILE
#ifndef COMPARE
	} /* if-else (playback_start) */
#endif
#endif
#endif

	write_count += (BYTES_PER_MPEG_AUDIO_FRAME >>2); /* divide by 4 bytes/sample */

	return 0;
}

void mpCleanup ()
{
	int32 i;

	if (CleanupMask & CLNUP_STOPSPOOL)
	{
		ssplAbort( sspl, NULL );
	}
	if (CleanupMask & CLNUP_SPOOL)
	{
		if (sspl) ssplDeleteSoundSpooler( sspl );
	}
	if (CleanupMask & CLNUP_AUDBUF)
	{
		for (i=0; i<NUMAUDBUFS; i++)
		{
			if (audBuffer[i]) FreeMem (audBuffer[i], AUDBUFSIZE);
		}
	}
	if (CleanupMask & CLNUP_SAMPINS)
	{
		DisconnectInstrumentParts (OutputIns, "Input", 0);
		DisconnectInstrumentParts (OutputIns, "Input", 1);
		UnloadInstrument( SamplerIns );
	}
	if (CleanupMask & CLNUP_OUTINS)
	{
		UnloadInstrument( OutputIns );
	}
	if (CleanupMask & CLNUP_AUDFOLIO)
	{
		CloseAudioFolio();
	}
	if (CleanupMask & CLNUP_EVENT)
	{
		KillEventUtility();
	}
	if (CleanupMask & CLNUP_BSBUF)
	{
		if (bsBuffer) FreeMem (bsBuffer, bsSize);
	}
	if (CleanupMask & CLNUP_CMPBUF)
	{
		if (compBuffer) FreeMem (compBuffer, AUDBUFSIZE);
	}
	if (CleanupMask & CLNUP_BSFILE)
	{
		CloseRawFile(theMPEGStream);
	}
#ifdef SOUNDFILE
	if (CleanupMask & CLNUP_AOUTFILE)
	{
		CloseRawFile(theAudioStream);
	}
#endif
#ifdef COMPARE
	if (CleanupMask & CLNUP_ACMPFILE)
	{
		CloseRawFile(theCompStream);
	}
#endif

	PRT(("All done.\n"));

	exit(0);
}

void mpBreakPoint (void)
{
	int32 Joy;

	DBUG(("Entered breakpoint\n"));
	do
	{
		/* Read current state of Control Pad. */
		Result = GetControlPad (1, FALSE, &cped);
		CHECKRESULT (Result, "GetControlPad()", 0);
		Joy = cped.cped_ButtonBits;
	} while ((Joy & ControlStart) != 0);

	do
	{
		/* Read current state of Control Pad. */
		Result = GetControlPad (1, FALSE, &cped);
		CHECKRESULT (Result, "GetControlPad()", 0);
		Joy = cped.cped_ButtonBits;
	} while ((Joy & ControlStart) == 0);

	return;
}
