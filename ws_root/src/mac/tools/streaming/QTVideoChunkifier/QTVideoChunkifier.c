/******************************************************************************
**
**  @(#) QTVideoChunkifier.c 96/04/09 1.13
**
******************************************************************************/

/*------------------------------------------------------------------------------
NAME
      QTVideoChunkifier -- MPW tool to convert Cinepak and EZFlix compressed 
	QuickTime movie	to a 3DO DataStream chunk file.

SYNOPSIS
      QTVideoChunkifier [flags] [movieFile(s)É]

------------------------------------------------------------------------------*/

/**
|||	AUTODOC -public -class Streaming_Tools -group EZFlix -name QTVideoChunkifier
|||	Chunkifies an EZFlix-compressed QuickTime movie.
|||	
|||	  Format -preformatted
|||	
|||	    QTVideoChunkifier [options] <moviefile>
|||	
|||	  Description
|||	
|||	    Converts a EZFlix-compressed QuickTime movie to a 3DO chunkified
|||	    format. For convenience, creates a video-only stream that can be
|||	    played by the Data Streamer without weaving (though typically there
|||	    will be reason to weave eventually, to add audio, etc.) 
|||	
|||	  Arguments
|||	
|||	    -b <size>
|||	        Stream block size in bytes of the output video-only stream. Only 
|||	        meaningful if the video-only stream is to be played as is. If later
|||	        processed by the Weaver, will be over-ridden by the Weaver-specified
|||	        stream blocksize. Must be a multiple of 2048 bytes.  Default: 32768
|||	
|||	    -c <channel>
|||	        Logical channel number.  Default: 0
|||	
|||	    -d <outputDirectory>
|||	        Direct the output files to a directory.
|||	
|||	    -l
|||	        Causes a GOTO chunk to be output at the end of the stream, which
|||	        will make the streamed movie loop back to its first frame.
|||	        NOTE: This feature is currently disabled.
|||	
|||	    -m <rate>
|||	        Specifies the rate, in markers per second, at which key-frame
|||	        markers will be output. (All EZFlix frames are key frames.)
|||	        Default: 1
|||	
|||	    -o <outputfilename>
|||	        Specifies the output filename. If no name is specified, the output
|||	        filename will be the input filename with .EZFL appended to the end.
|||	
|||	    -opera
|||	        Output an Opera format stream.
|||	
|||	    -s
|||	        Specifies the start time.
|||	
|||	    -v
|||	        Enables verbose diagnostic output.
|||	
|||	  Caveats
|||	
|||	    QTVideoChunkifier ignores the input file's audio and outputs video
|||	    chunks only.
|||	
|||	  Example
|||	
|||	    QTVideoChunkifier video.MooV
**/

#include	<Types.h>
#include 	<ctype.h>
#include 	<fcntl.h>
#include 	<string.h>
#include 	<stdio.h>
#include 	<stddef.h>
#include 	<time.h>
#include	<ErrMgr.h>
#include	<CursorCtl.h>
#include	<Strings.h>
#include	<Errors.h>
#include	<Memory.h>
#include	<FixMath.h>
#include	<Packages.h>
#include	<QuickDraw.h>
#include	<TextUtils.h>
#include	<Movies.h>
#include	<GestaltEqu.h>

#include	"Films.h"
#include	"StdLib.h"
#include	"VQDecoder.h"
#include	"EZFlixStream.h"
#include	"EZFlixEncoder.h"
#include	"EZFlixDefines.h"

#define	PROGRAM_VERSION_STRING	"2.3b1"

#define	kPhysBlockSize			(1024 * 1024)
#define	kMinSoundChunkSize		(sizeof(StreamSoundChunk) + 1024)
#define	kMaxTrackCount			50

/* Define Audio ticks (44100/184) in double format.
 */
#define	AUDIO_TICKS_FIXED	(double) (44100.0 / 184.0)

#define 	COMPRESSION_TYPE_NONE		'NONE'

/******************************************/
/* Command line settable global variables */
/******************************************/
long		gPhysBlockSize		= kPhysBlockSize; /* default to a huge blocking factor */
Boolean		gWriteFillerChunks	= false; 		/* default to no blocking - no filler */
Boolean		gSupportCinepak		= false; 		/* default to no Cinepak support */
Boolean		gEnableMarkerOutput	= false;		/* default is no marker output file (set by -m) */
double		gMarkerRate 		= 1.0; 			/* default markers to 1 second intervals */
Boolean		gLoopIt				= false;		/* default for movie looping == FALSE */
Boolean		gVerbose			= false;		/* default for verbose output == FALSE */
Boolean		gFlagKeyFrames		= false;		/* default to flag key frames ala MovieToStream_Shuttle */
Boolean		gOpera				= false;		/* default to M2 format stream */
Ptr			gEmptyPtr;
long		gStartTime			= 0;			/* default for a one clip movie */
long		gChannel 			= 0; 			/* default channel == 0 */
char*		gToolName; 							/* "QTVideoChunkifier" */

QDGlobals	qd;									/* QuickDraw Globals */

/* Macro used to make chatty printf's go away unless user
 * specifies the "-v" verbose option.
 */
#define	VFPRINTF	if (gVerbose) fprintf


/*
	OpenMovie opens a movie file and returns the movie
*/

Movie OpenMovie(Str255 MovieName)
{
	OSErr		Oops;
	FSSpec		Spec;
	short		resRefNum;
	Movie		theMovie;
	short		resID;
	Str255		theStr;
	Boolean		dataRefWasChanged;
	
	if ((Oops = FSMakeFSSpec(0,0,MovieName,&Spec)) != noErr)	{
		fprintf(stderr,"### %d Occurred while FSMakeFSSpec for %P\n", Oops, MovieName);
		return 0;
	}
	if ((Oops = OpenMovieFile(&Spec,&resRefNum,0)) != noErr)	{
		fprintf(stderr,"### %d Occurred while opening %P\n", Oops, MovieName);
		return 0;
	}
	resID = 0;
	if ((Oops = NewMovieFromFile(&theMovie,resRefNum,&resID,theStr,0,&dataRefWasChanged)) != noErr)	
	{
		if (Oops == noCodecErr)
		{
			fprintf(stderr,"### %s -- Error: Codec not installed.  \n", gToolName);
			fprintf(stderr,"###\t\tPlease install EZFlix in the extensions folder and reboot.\n");
		}
		else fprintf(stderr,"### %d Occurred while opening movie in %P\n", Oops, theStr);
	}
	Oops = CloseMovieFile(resRefNum);
	
	return theMovie;
}

long 
AlignSample(Ptr srcData, Ptr dstData)
{
	Ptr 			sd = srcData;
	Ptr 			dd = dstData;
	long			frameType = *(unsigned char *)sd;
	Ptr				frameEnd = sd + (*(long *)sd & 0x00FFFFFF);
	long			tileCount = ((FrameHeaderPtr)sd)->frameTileCount;
	long			tileType,atomType,atomSize,desiredSize,size;
	Ptr				tileStart,tileEnd;
	short			t;
				
	DebugStr("\pAlignSample");
	*(FrameHeader *)dd = *(FrameHeader *)sd;
	dd += sizeof(FrameHeader);
	*((long *)dd)++ = 0xFE000006;
	*((short *)dd)++ = 0x0000;
	
	sd += sizeof(FrameHeader);
	
	/* Loop through all tiles and expand every code book */
	
	for (t = 0; t < tileCount; t++) 
	{
		/* parse atoms within frame looking for tiles */
		
		do {
			tileType = *(unsigned char *)sd;
			if (tileType == kTileKeyType8 || tileType == kTileInterType8)
				break;
			sd += *(long *)sd & 0x00FFFFFF;
		} while (true);
		
		/* skip tile header and get length of tile atom */
		
		tileStart = dd;
		tileEnd = sd + (*(long *)sd & 0x00FFFFFF);
		*(TileHeader *)dd = *(TileHeader *)sd;		/* Copy the frame header */
		dd += sizeof(TileHeader);
		sd += sizeof(TileHeader);
		
		/* parse atoms within tile looking for codebooks */
		
		do {
			atomSize = *(long *)sd & 0x00FFFFFF;
			atomType = *(unsigned char *)sd;
			desiredSize = (atomSize + 3) & 0x00FFFFFC;			
			BlockMove(sd,dd,desiredSize);
			*(long *)dd = (atomType << 24) | desiredSize;
			dd += desiredSize;
			sd += *(long *)sd & 0x00FFFFFF;
			
		} while (sd < tileEnd);
		*(long *)tileStart = (tileType << 24) | (dd - tileStart);
	}
	size = (dd - dstData);
	*(long *)srcData = (frameType << 24) | size;
	return size;
}

char *
CurrTime(char *timeStr)
{
	unsigned long	currTime;
	
	GetDateTime(&currTime);
	IUTimeString(currTime, true, (unsigned char*)timeStr);
	return p2cstr((unsigned char*)timeStr);
}


/*	Write out a chunk of type 'FILL' for the
 *	remaining block data. This is done when no
 *	other block is small enough to fit in the
 *	remaining space.
 */
WriteFillerChunk (short filmFile, long remainingBlockSize)
{
	FillerChunk		fill;
	long			writeSize;
	OSErr			Oops;

	/* if we are not writing filler chunks, then we are done. */
	if (! gWriteFillerChunks) return;
	
	if (remainingBlockSize < sizeof(FillerChunk)) {
		fprintf(stderr,"### %d Too small for filler chunk --- wrote zeros.\n", remainingBlockSize);
		writeSize = remainingBlockSize;
		Oops = FSWrite(filmFile,&writeSize,gEmptyPtr);
		return;
	}
		
	fill.chunkType = kFillerChunkType;
	fill.chunkSize = remainingBlockSize;
	
	writeSize = sizeof(FillerChunk);
	if ((Oops = FSWrite(filmFile,&writeSize,&fill)) != noErr)	{
		fprintf(stderr,"### %d Occurred during WriteFillerChunk\n", Oops);
	}

	writeSize = remainingBlockSize-sizeof(FillerChunk);
	Oops = FSWrite(filmFile,&writeSize,gEmptyPtr);
}


/*	Write out a chunk of type 'CTRL' for the
 *	marker chunk. This is done in order to
 *	cause the Data Acq to seek back to the top
 *	of this data stream.
 */
WriteMarkerChunk (short filmFile, long *remainingBlockSize, long offset)
{
#define DISABLE_CTRL_CHUNKS_TBD		1
#if DISABLE_CTRL_CHUNKS_TBD
	/* NO-OP the obsolete code for now. Later, switch to STRM GOTO chunks. */
	(void)filmFile;
	(void)remainingBlockSize;
	(void)offset;
	fprintf(stderr,"### Writing GOTO chunks is disabled until it gets updated to STRM GOTO chunks.\n");

#else
	#error "CTRL chunk code must be converted to generate STRM chunks"
	xxx CtrlMarkerChunk	mark;						/* OBSOLETE! REVISIT! */
	long				writeSize;
	OSErr				Oops;

	if (*remainingBlockSize < sizeof(CtrlMarkerChunk)) {/* OBSOLETE! REVISIT! */
		fprintf(stderr,"### remainingBlockSize less than CtrlMarkerChunk!!!\n");
		return;
	}
		
	mark.chunkType		= xxx CTRL_CHUNK_TYPE xxx;	/* OBSOLETE! REVISIT! */
	mark.chunkSize		= sizeof(CtrlMarkerChunk);	/* OBSOLETE! REVISIT! */
	mark.time			= gStartTime;
	mark.channel		= gChannel;
	mark.subChunkType	= GOTO_CHUNK_TYPE;
	mark.marker			= offset;					/* OBSOLETE! REVISIT! */
	
	writeSize = sizeof(mark);
	if ((Oops = FSWrite(filmFile,&writeSize,&mark)) != noErr)	{
		fprintf(stderr,"### %d Occurred during WriteMarkerChunk\n", Oops);
	}
	
	*remainingBlockSize -= sizeof(CtrlMarkerChunk);
#endif
}



/* Convert seconds to Film's time units.
 */
TimeValue
SecsToMediaTime (long numSeconds, FilmHeaderPtr pFilmH)
{
	return (pFilmH->scale * numSeconds);
}



/* Convert the data if the format is not Twos Complement.
 * The Audio Folio needs samples in Twos Complement format.
 */
CvtToTwosComplement (StreamSoundHeaderPtr pSSndH, unsigned char *soundData, long size)
{
unsigned char	*c;
long			i;

	if ((pSSndH->sampleDesc.sampleSize == 8) && (pSSndH->sampleDesc.dataFormat == 'raw ')) {
		c = soundData;
		for (i = 0; i < size; i++) {
			*c++ = (*c ^ 0x80);
		}
	}
}


long
CvtSamplesToBytes(long numberOfSamples, StreamSoundHeaderPtr pSSndH)
{
long	numBytes;

	numBytes = numberOfSamples * (pSSndH->sampleDesc.sampleSize >> 3) * pSSndH->sampleDesc.numChannels;
	return ((numBytes+3) & 0xFFFFFFFC);

}


TimeValue
CvtToSoundTime(	TimeValue				mediaTime,
				StreamSoundHeaderPtr	pSSndH,
				FilmHeaderPtr			pFilmH )
{
TimeValue	conversionValue;


	/* Calculate the value to use as a divisor to covert to
	 * the video media time. NOTE: the result is 16.16 fixed.
	 */
	conversionValue = (uint32)pSSndH->sampleDesc.sampleRate / pFilmH->scale;
	
	/* Convert soundTime to a 16.16 fixed value and then
	 * divide by the conversionValue. The result will be
	 * an integer value in Film scale time units.
	 */
	return (mediaTime * (conversionValue>>16));
}


TimeValue
CvtToMediaTime (TimeValue				soundTime,
				StreamSoundHeaderPtr	pSSndH,
				FilmHeaderPtr			pFilmH)
{
double		conversionValue, theResult;

	/* Calculate the value to use as a divisor to covert to
	 * the video media time. NOTE: the result is floating point number.
	 */
	conversionValue = (double)pSSndH->sampleDesc.sampleRate / (double)pFilmH->scale;
	
	/* The result will be
	 * an integer value in Film scale time units.
	 */
	theResult = soundTime / conversionValue;
	return ((long) (theResult + 0.5) );			/* round up */
}

/*	3DO time is in 240ths of a second. This is the resolution of the audio
 *	timer, and the audio timer is the common timebase for the Data Streamer.
 */
TimeValue
CvtTo3DOTime(	TimeValue		timeIn,
				long			scale)
{
double		conversionValue, theResult;

	/* Calculate the value to use as a divisor to covert to
	 * 3DO time. NOTE: the result is floating point number.
	 */
	conversionValue = (double)AUDIO_TICKS_FIXED / (double)scale;
	
	/* The result will be an integer
	 * value in 3DO time units.
	 */
	theResult = (double)timeIn * conversionValue;
	return ((long) (theResult + 0.5) );			/* round up */
}

/*	Calculate the number of sound samples required to fill the
 *	remainingBlockSize or the soundChunkTime, whichever is less.
 */
long
CalcNumSamples(	TimeValue				soundChunkTime,
				long					remainingBlockSize,
				StreamSoundHeaderPtr	pSSndH)
{
TimeValue	remBlkTime, soundSampleTime;

	/* Round remainingBlockSize and soundChunkTime down to nearest LongWord */
	remainingBlockSize = (remainingBlockSize & 0xFFFFFFFC) - sizeof(StreamSoundChunk);
	soundChunkTime &= 0xFFFFFFFC;
	
	/* Determine the number of sound samples that will fit in the remaining space. */
	remBlkTime = remainingBlockSize / (pSSndH->sampleDesc.sampleSize >> 3) / pSSndH->sampleDesc.numChannels;
	soundSampleTime = soundChunkTime / (pSSndH->sampleDesc.sampleSize >> 3) / pSSndH->sampleDesc.numChannels;
	
	/* If there are more samples requested than block size left, return the
	 * number of sound samples in a block.
	 */
	return (remBlkTime < soundSampleTime) ? remBlkTime : soundSampleTime;
}


/*	Convert the sample count to a byte count for decrementing the physical
 *	block count.
 */
long
SampleToBytes( TimeValue numberOfSamples, StreamSoundHeaderPtr	pSSndH )
{
	return (numberOfSamples * (pSSndH->sampleDesc.sampleSize >> 3) * pSSndH->sampleDesc.numChannels);
}



OSErr	
DumpMovie(Movie theMovie, Str255 filmFileName, Str255 keyFilePath)
{ 
	Track		t;
	Media		m,SoundMedia[kMaxTrackCount],VideoMedia[kMaxTrackCount];
	long		VideoMediaTrackOffset[kMaxTrackCount];
	long		SoundMediaCount,VideoMediaCount;
	long		n, numberOfTracks,x;
	OSType		mediaType;
	Str255		originalName;
	OSType		originalManufacturer;
	OSErr		Oops = noErr;
	
	short				sampleFlags;
	TimeValue			mediatime,sampleTime,durationPerSample,numberOfSamples;
	TimeValue			frameCount, movieDuration, movieTimeScale, mediaTimeScale;
	long				size,writeSize,headerSize,remainingBlockSize;
	ImageDescription	**IDesc = nil;
	SoundDescription	**SDesc = nil;
	long				currentSoundTime,currentVideoTime, soundTimeInMediaUnits;
	Boolean				isFlixFormat;		/* if doing EZFlix instead of Cinepak */
	FilmHeader			FilmH;
	FilmFrame			FilmF;
	EZFlixHeader		ezFlixHeader;
	EZFlixM2Header		ezFlixM2Header;
	StreamSoundHeader	SSndH;
	StreamSoundChunk	SSndC;
	Handle				sampledata	= nil;
	Handle				sounddata	= nil;
	short				FilmFile;
	Ptr					AlignedData	= nil;
	Boolean				soundLeft = true;
	
	FILE*				keyFile		= nil;
	TimeValue			lastMarkerTime = 0;
	const char			writePermission[2] = "w";

	Oops = Create(filmFileName,0,'CYBD','FILM');
	if ( (Oops != noErr) && (Oops != dupFNErr))	
	{
		fprintf(stderr,"### %d Occurred during Create of %P\n", Oops, filmFileName);
		goto CLEAN_UP;
	}

	if (( Oops = FSOpen(filmFileName,0,&FilmFile)) != noErr ) 
	{
		fprintf(stderr,"### %d Occurred during Open of %P\n", Oops, filmFileName);
		goto CLEAN_UP;
	}

	/* Reset the file position to the beginning if file exists. */
	if ( Oops == dupFNErr )
	{
		if (( Oops = SetFPos( FilmFile, fsFromStart,0 )) != noErr ) 
		{
			fprintf(stderr,"### %d Occurred during SetFPos of %P\n", Oops, filmFileName);
			goto CLEAN_UP;
		} /* if noErr != SetFPos */

		if (( Oops = SetEOF ( FilmFile, (long) 0 )) != noErr )
		{
			fprintf(stderr,"### %d Occurred during SetEOF of %P\n", Oops, filmFileName);
			goto CLEAN_UP;
		} /* if noErr != SetEOF */
		
	} /* if Oops == dupFNErr */

	/* Open output marker text file if marker output is enabled
	 */
	if ( gEnableMarkerOutput )
		{
		p2cstr( keyFilePath );
		keyFile = fopen( (char *)keyFilePath,writePermission );
		if ( keyFile == NULL )
			{
			fprintf(stderr,"### Error opening marker file: %s\n", keyFilePath );
			goto CLEAN_UP;
			}
		}

	SpinCursor(32);

/*	Get the length of the movie in movie time scale */
	movieDuration = GetMovieDuration(theMovie);
	
	
/*	Find the video media and the sound media for this movie */

	for (n = 0; n < kMaxTrackCount; n++)	
	{
		VideoMediaTrackOffset[n] = 0;
		SoundMedia[n] = VideoMedia[n] = nil;
	}
	
	SoundMediaCount = 0;
	VideoMediaCount = 0;
	
	SpinCursor(32);

/* make sure it doesn't have too many tracks for us to handle */

	if ( (numberOfTracks = GetMovieTrackCount(theMovie)) >= kMaxTrackCount )
	{
		fprintf(stderr,"#### Can't convert this movie, it has more than %d tracks.  Sorry\n", kMaxTrackCount);
		goto CLEAN_UP;
	}


/* and grab a reference to each */

	VFPRINTF(stderr,"\tcataloging movie tracks...\n");
	IDesc = (ImageDescription **)NewHandle(sizeof(ImageDescription));
	SDesc = (SoundDescription **)NewHandle(sizeof(SoundDescription));
	for (n = 1; n <= numberOfTracks; n++)
	{
		t = GetMovieIndTrack(theMovie,n);
		m = GetTrackMedia(t);
		GetMediaHandlerDescription(m,&mediaType,originalName,&originalManufacturer);
		if (mediaType == VideoMediaType)	
		{
			VFPRINTF(stderr,"\t  video track: %P\n", originalName);
			VideoMedia[VideoMediaCount] = m;
			VideoMediaTrackOffset[VideoMediaCount] = GetTrackOffset(t);
			VideoMediaCount++;

			/* make sure that it is a compact video or EZFlix track! */
			GetMediaSampleDescription(m,1,(SampleDescriptionHandle)IDesc);
			
			/* We can always support EZFlix */
			if ((*IDesc)->cType != kEZFlixTrackType)
			{
				if (! gSupportCinepak)
				{
					fprintf(stderr,"\t\t#### This track is not compressed with EZFlix.\n");
					fprintf(stderr,"\t\t#### Recompress the movie and try again.\n");
					goto CLEAN_UP;
				}
				else if ((*IDesc)->cType != kCompactVideoTrackType)
				{
					fprintf(stderr,"\t\t#### This track is not compressed with Compact Video or EZFlix.\n");
					fprintf(stderr,"\t\t#### Recompress the movie and try again.\n");
					goto CLEAN_UP;
				}
			}
		} 
		else if (mediaType == SoundMediaType) 
		{
			VFPRINTF(stderr,"\t  audio track: %P\n", originalName);
			SoundMedia[SoundMediaCount] = m;
			SoundMediaCount++;

			GetMediaSampleDescription(m,1,(SampleDescriptionHandle)SDesc);
			VFPRINTF(stderr,"\t  SDesc dataFormat: '%.4s'\n", &((*SDesc)->dataFormat) );
			VFPRINTF(stderr,"\t  SDesc numChannels: %d\n", (*SDesc)->numChannels );
			VFPRINTF(stderr,"\t  SDesc sampleSize: %d\n", (*SDesc)->sampleSize );
			VFPRINTF(stderr,"\t  SDesc packetSize: %d\n", (*SDesc)->packetSize );
			VFPRINTF(stderr,"\t  SDesc sampleRate: %d\n", ( ((uint32)(*SDesc)->sampleRate) >> 16) );
			
		}
	}
			

/*	Create the film header ----------------------------------------------------------- */

	SpinCursor(32);

	/* Start video time at the beginning.
	 */
	currentVideoTime = 0;

	/* Cycle through each video track and determine the number
	 * of frames in each track's media.
	 */
	frameCount = 0;

	/* Do a little check to see if the movie time scale matches
	 * each of the media time scales.
	 */
	movieTimeScale = GetMovieTimeScale(theMovie);
		
	for (n = 0; n < VideoMediaCount; n++)
	{
		frameCount += GetMediaSampleCount(VideoMedia[n]);
		
		if (movieTimeScale != GetMediaTimeScale(VideoMedia[n]))
		{
			fprintf(stderr,"\t ###Movie time scale: %ld does not match track %d time scale: %ld\n",
							GetMovieTimeScale(theMovie), n, GetMediaTimeScale(VideoMedia[n]) );
		}
	}
	
	GetMediaSampleDescription(VideoMedia[0],1,(SampleDescriptionHandle)IDesc);
	mediaTimeScale = GetMediaTimeScale(VideoMedia[0]);
	
	if ((*IDesc)->cType == kCompactVideoTrackType)
		isFlixFormat = false;
	else
		isFlixFormat = true;
	if (! isFlixFormat)
	{
		FilmH.chunkType		= kVideoChunkType;
		FilmH.chunkSize		= sizeof(FilmHeader);
		FilmH.subChunkType	= kVideoHeaderType;
		FilmH.time			= gStartTime;
		FilmH.channel	= gChannel;
		FilmH.version	= 0;
		FilmH.cType		= (*IDesc)->cType;
		FilmH.height	= (*IDesc)->height;
		FilmH.width		= (*IDesc)->width;
		FilmH.scale		= movieTimeScale / ( movieDuration / frameCount );
		FilmH.count		= frameCount;
	}

	else /* isFlixFormat */
	{
		/* Get decompression parameters from the EZFlix encoder and copy
		 * them into the header chunk.
		 */
		EZFlixEncoderParams params;
		GetEZFlixEncoderParams(&params);
			
		if (gOpera)
		{
			if ((*IDesc)->spatialQuality != kFixedTwoBitQuality)
			{
				fprintf( stderr, "### %s - This movie was not compressed at an\n", gToolName);
				fprintf( stderr, "\t\tOpera-compatible quality level.  Please recompress with\n");
				fprintf( stderr, "\t\tthe quality slider set to 50%.\n");
				return EXIT_FAILURE;
			}
			
			ezFlixHeader.chunkType 				= EZFLIX_CHUNK_TYPE;
			ezFlixHeader.chunkSize 				= sizeof(EZFlixHeader);
			ezFlixHeader.subChunkType 			= EZFLIX_HDR_CHUNK_SUBTYPE;
			ezFlixHeader.time					= gStartTime;
			ezFlixHeader.channel				= gChannel;
			ezFlixHeader.version				= kEZFlixFormatVersion;
			ezFlixHeader.cType		= (*IDesc)->cType;
			ezFlixHeader.height		= (*IDesc)->height;
			ezFlixHeader.width		= (*IDesc)->width;
			ezFlixHeader.scale		= movieTimeScale / ( movieDuration / frameCount );
			ezFlixHeader.count		= frameCount;
			
			ezFlixHeader.yquant[0]		= params.yquant[0];
			ezFlixHeader.yquant[1]		= params.yquant[1];
			ezFlixHeader.yquant[2]		= params.yquant[2];
			ezFlixHeader.yquant[3]		= params.yquant[3];
			ezFlixHeader.uvquant[0]		= params.uvquant[0];
			ezFlixHeader.uvquant[1]		= params.uvquant[1];
			ezFlixHeader.uvquant[2]		= params.uvquant[2];
			ezFlixHeader.uvquant[3]		= params.uvquant[3];
			ezFlixHeader.min			= params.min;
			ezFlixHeader.max			= params.max;
			ezFlixHeader.invgamma		= params.invgamma;
			ezFlixHeader.codecVersion	= params.version;
			ezFlixHeader.unused = 0;
		}
		else /* M2 Version of the header */
		{
			ezFlixM2Header.chunkType 				= EZFLIX_CHUNK_TYPE;
			ezFlixM2Header.chunkSize 				= sizeof(EZFlixM2Header);
			ezFlixM2Header.subChunkType 			= EZFLIX_HDR_CHUNK_SUBTYPE;
			ezFlixM2Header.time						= gStartTime;
			ezFlixM2Header.channel					= gChannel;
			ezFlixM2Header.version					= kEZFlixM2FormatVersion;
			ezFlixM2Header.cType					= (*IDesc)->cType;
			ezFlixM2Header.height					= (*IDesc)->height;
			ezFlixM2Header.width					= (*IDesc)->width;
			ezFlixM2Header.scale					= movieTimeScale / ( movieDuration / frameCount );
			ezFlixM2Header.count					= frameCount;
			
			ezFlixM2Header.codecVersion	= params.version;
			ezFlixM2Header.min			= params.min;
			ezFlixM2Header.max			= params.max;
			ezFlixM2Header.invgamma		= params.invgamma;
			ezFlixM2Header.unused = 0;
		}
	}
	
	VFPRINTF(stderr,"\t  videoScale: %ld\n", mediaTimeScale );
	VFPRINTF(stderr,"\t  video frame count: %d\n",frameCount);

	movieDuration = movieDuration / ( movieTimeScale / mediaTimeScale);
	VFPRINTF(stderr,"\t  movieDuration: %d\n",movieDuration);

	SpinCursor(32);

/*	Write the Film Header to disk ----------------------------------------------------------- */

	if (! isFlixFormat)
	{
		headerSize = sizeof(FilmHeader);
		if ((Oops = FSWrite(FilmFile,&headerSize,&FilmH)) != noErr)	{
			fprintf(stderr,"### %d Occurred during FSWrite of %P\n", Oops, filmFileName);
			goto CLEAN_UP;
		}
	} 
	else if (gOpera)
	{
		headerSize = sizeof(EZFlixHeader);
		if ((Oops = FSWrite(FilmFile,&headerSize,&ezFlixHeader)) != noErr)	{
			fprintf(stderr,"### %d Occurred during FSWrite of %P\n", Oops, filmFileName);
			goto CLEAN_UP;
		}
	}
	else
	{
		headerSize = sizeof(EZFlixM2Header);
		if ((Oops = FSWrite(FilmFile,&headerSize,&ezFlixM2Header)) != noErr)	{
			fprintf(stderr,"### %d Occurred during FSWrite of %P\n", Oops, filmFileName);
			goto CLEAN_UP;
		}
	}

	DisposeHandle((Handle)IDesc);
	IDesc = nil;
	
/*	Create the Streamed Sound Header ----------------------------------------------------------- */
/*	At this point, we don't do sound. */

	currentSoundTime 		= 0;
	soundTimeInMediaUnits	= 0;
	
	SoundMedia[0] = nil;
	
	/* Make a dummy sound header to use in the conversion routines.
	 */
	SSndH.channel = gChannel;
	SSndH.sampleDesc.numChannels	= 1;
	SSndH.sampleDesc.sampleSize	= 8;
	SSndH.sampleDesc.sampleRate	= 44100;
	soundLeft			= false;	

	DisposeHandle((Handle)SDesc);
	SDesc = nil;
	
	SpinCursor(32);

/*	Write the Streamed Sound Header to disk ----------------------------------------------- */

	if (SoundMedia[0]) 
	{
		writeSize = sizeof(StreamSoundHeader);
		if ((Oops = FSWrite(FilmFile,&writeSize,&SSndH)) != noErr)	{
			fprintf(stderr,"### %d Occurred during FSWrite of %P\n", Oops, filmFileName);
			goto CLEAN_UP;
		}
	}
	
/*	Copy all the audio samples and video frames as chunks --------------------------------- */

	VFPRINTF(stderr,"\tconverting media...\n");
	sampledata = NewHandle(0);
	AlignedData = malloc(gPhysBlockSize);
	if (!AlignedData) {
		return memFullErr;
	}
	
	/* Make the remainingBlockSize allow for the headers we just wrote out.
	 */
	remainingBlockSize = gPhysBlockSize;
	if (VideoMedia[0]) remainingBlockSize -= headerSize;
	if (SoundMedia[0]) remainingBlockSize -= sizeof(StreamSoundHeader);

	/* Write out one second of sound followed by one second of video.
	 */
	while ( frameCount > 0) {

		SpinCursor(32);
		
		if (SoundMedia[0] && soundLeft)
		{
		TimeValue	mediaTimePlusOneSec, soundChunkTime;
		long		numReqSamples;
			
			/* If the amount of written audio data is not yet one second ahead
			 * of the amount of written video data, write another chunk of audio
			 * data to get the audio stream one second ahead.
			 */
			soundTimeInMediaUnits	= CvtToMediaTime (currentSoundTime, &SSndH, &FilmH);
			mediaTimePlusOneSec		= currentVideoTime + SecsToMediaTime(1, &FilmH);
			soundChunkTime			= CvtToSoundTime (mediaTimePlusOneSec-soundTimeInMediaUnits,
														&SSndH,&FilmH);
			
			if (mediaTimePlusOneSec >= soundTimeInMediaUnits)
			{
				VFPRINTF(stderr,"remainingBlockSize = %d  %h\n", remainingBlockSize, remainingBlockSize);

				if (kMinSoundChunkSize > remainingBlockSize)
				{
					WriteFillerChunk (FilmFile, remainingBlockSize);
					remainingBlockSize = gPhysBlockSize;
				}
	
				VFPRINTF(stderr,"soundChunkTime = %d  %h\n", soundChunkTime, soundChunkTime);
				numReqSamples = CalcNumSamples(soundChunkTime, remainingBlockSize, &SSndH);
				
				VFPRINTF(stderr,"CalcNumSamples returned numReqSamples = %d\n", numReqSamples);
	
				Oops = GetMediaSample(SoundMedia[0],sounddata,0,&size,currentSoundTime,&sampleTime,
									  0,0,0,numReqSamples,&numberOfSamples,&sampleFlags);
				if (Oops)
				{
					fprintf(stderr,"### %d Occurred during GetMediaSample for sound\n", Oops);
					goto CLEAN_UP;
				}
				
				/* Check the size of each sample.  Make sure that the */
				/* size of each sample will fit into the physical block size. */
				if ( size > gPhysBlockSize )
				{
					fprintf(stderr,"### Error : Sample size (%d) is greater than the physical blocksize (%d).\n",
						size, gPhysBlockSize );
					fprintf(stderr, "### Error : Increase the blocksize and re-run the program.\n\n");
					Oops = maxSizeToGrowTooSmall;
					goto CLEAN_UP;
				}

				size = (size + 3) & 0xFFFFFFFC;	/* round up to nearest longword */

				/* If the size of sound sample is too big to fit into the reamaingBlockSize,
				 * write out a filler chunk.
				 */
				 if (size > remainingBlockSize)
				{
					WriteFillerChunk (FilmFile, remainingBlockSize);
					remainingBlockSize = gPhysBlockSize;
				}
		
				VFPRINTF(stderr,"currentSoundTime %3d, sampleTime %6d, Size %5d\n",
								currentSoundTime,     sampleTime,     size);
	
	
				/* Write out the stream sound chunk containing data to the end of
				 * this physical block.
				 */
				SSndC.chunkType		= kSoundChunkType;
				SSndC.chunkSize		= sizeof(StreamSoundChunk) + size;
				SSndC.subChunkType	= kSoundSampleType;
				SSndC.channel 		= gChannel + 1;	/* Use a different channel for the audio data */
				SSndC.time			= CvtTo3DOTime( currentSoundTime, SSndH.sampleDesc.sampleRate ) + gStartTime;
				SSndC.actualSampleSize	= size;
				/* SSndC.numSamples	= numberOfSamples; ¥¥¥ might need this someday... */
			
				writeSize = sizeof(StreamSoundChunk);
				if ((Oops = FSWrite(FilmFile,&writeSize,&SSndC)) != noErr)	{
					fprintf(stderr,"### %d Occurred while writing StreamSoundChunk to FilmFile\n", Oops);
					goto CLEAN_UP;
				}
	
				/* Convert the data if the format is not Twos Complement.
				 * The Audio Folio needs samples in Twos Complement format.
				 */
				CvtToTwosComplement (&SSndH, (unsigned char*)*sounddata, size);
			
	
				if ((Oops = FSWrite(FilmFile,&size,*sounddata)) != noErr)	{
					fprintf(stderr,"### %d Occurred while writing sounddata to FilmFile\n", Oops);
					goto CLEAN_UP;
				}
	
				/* Update the current Sound Time and 
				 * decrement the remainingBlockSize.
				 */
				currentSoundTime += numberOfSamples;
				remainingBlockSize -= sizeof(StreamSoundChunk) + size;
				
				if (remainingBlockSize == 0)
					remainingBlockSize = gPhysBlockSize;

				/* If the currentSoundTime is greater than or equal to the number of sound samples,
				   this is the last buffer of sound so clear the flag.
				 */
				if ( currentSoundTime >= SSndH.sampleDesc.sampleCount ) {
					soundLeft = false;
				}
			}
		}
		/* No sound so bump the soundChunkTime by one second to keep the video writing.
		 */
		else {
			currentSoundTime += 44100;
			soundTimeInMediaUnits = CvtToMediaTime (currentSoundTime, &SSndH, &FilmH);
			VFPRINTF(stderr,"soundTimeInMediaUnits = %d  %h\n", soundTimeInMediaUnits, soundTimeInMediaUnits);

		}

/*	LOOP UNTIL WE HAVE WRITTEN ONE SECOND OF VIDEO ¥¥¥ */
		do {

			SpinCursor(32);
			
			/* Get the next video chunk from any media based on the current time
			 * There may be more than one piece of media that has this time in it
			 */
			for (x = VideoMediaCount-1; x >= 0; x--) {
				if (currentVideoTime >= VideoMediaTrackOffset[x]) break;
			}
			
			VFPRINTF(stderr,"remainingBlockSize = %d  %h\n", remainingBlockSize, remainingBlockSize);

			mediatime = currentVideoTime - VideoMediaTrackOffset[x];
			m = VideoMedia[x];
			
			Oops = GetMediaSample(m,sampledata,0,&size,mediatime,&sampleTime,&durationPerSample,0,0,1,0,&sampleFlags);
			if (Oops) {
				fprintf(stderr,"### %d Occurred during GetMediaSample\n", Oops);
				fprintf(stderr,"### Filling to end of block and closing...\n");
				if (gLoopIt)
					WriteMarkerChunk (FilmFile, &remainingBlockSize, 0);
				if (remainingBlockSize)
					WriteFillerChunk (FilmFile, remainingBlockSize);
				goto CLEAN_UP;
			}
			
			/* Check the size of each sample.  Make sure that the */
			/* size of each sample will fit into the physical block size. */
			if ( size > gPhysBlockSize )
			{
				fprintf(stderr,"### Error : Sample size (%d) is greater than the physical blocksize (%d).\n",
					size, gPhysBlockSize );
				fprintf(stderr, "### Error : Increase the blocksize and re-run the program.\n\n");
				Oops = maxSizeToGrowTooSmall;
				goto CLEAN_UP;
			}
				
			/* copy the sample into an aligned working buffer */
			if (! isFlixFormat)
				size = AlignSample(*sampledata,AlignedData);
			else
			{
				memcpy(AlignedData, *sampledata, size);
			}
			
			/* put some fill at the end */
			if ( (sizeof(FilmFrame) + size) > remainingBlockSize)
			{
				WriteFillerChunk (FilmFile, remainingBlockSize);
				remainingBlockSize = gPhysBlockSize;
			}
	
			/* Adjust sampleTime to represent movie based time.
			 */
			sampleTime = mediatime + VideoMediaTrackOffset[x];
			
			if (! isFlixFormat)
			{
				FilmF.chunkType = kVideoChunkType;
				FilmF.frameSize	= size;
				FilmF.chunkSize = sizeof(FilmFrame) + size;
			}
			else if (gOpera)
			{
				FlixFrameHeader* qtFlixData = (FlixFrameHeader*) AlignedData;
				FilmF.chunkType = EZFLIX_CHUNK_TYPE;
				/* To make an Opera frame we must strip off the Flix Frame Header */
				/*	that the QT component puts on */
				FilmF.frameSize	= size - (long)(qtFlixData->chunk1Offset<<2);
				FilmF.chunkSize = offsetof(EZFlixFrame, frameData) + FilmF.frameSize;
			}
			else
			{
				FilmF.chunkType = EZFLIX_CHUNK_TYPE;
				/* To make an M2 Flix frame we just strip the height and width */
				FilmF.frameSize	= size;
				FilmF.chunkSize = offsetof(EZFlixM2Frame, frameData) + FilmF.frameSize;
			}
			
			/* Stuff in the fields of SubsChunkCommon */
			if ( gFlagKeyFrames )
			{
				FilmF.channel = gChannel;

				/* distinguish between key frames and difference frames */
				if ( sampleFlags != mediaSampleNotSync )
				{
					/*  FilmF.chunkFlags	= kKeyChunkFlag;  */ /* Nothing to do now for the shuttle stuff */
					if (isFlixFormat)
						FilmF.subChunkType	= kEZFlixKeyFrameType;
					else
						FilmF.subChunkType	= kVideoKeyFrameType;
				}
				else
				{
					/* FilmF.chunkFlags		= kDependentChunkFlag;  */
					if (isFlixFormat)
						FilmF.subChunkType	= kEZFlixDifferenceFrameType;
					else
						FilmF.subChunkType	= kVideoDifferenceFrameType;
				}
			}
			else
			{
				FilmF.channel 			= gChannel;
				if (isFlixFormat)
					FilmF.subChunkType	= kEZFlixFrameType;
				else
					FilmF.subChunkType	= kVideoFrameType;
			}
			FilmF.time			= CvtTo3DOTime( sampleTime, mediaTimeScale );
			FilmF.duration		= CvtTo3DOTime( durationPerSample, mediaTimeScale ); /* Bug #3616 */
			
			VFPRINTF(stderr,"Media Time %3d, sampleTime %6d, Duration %3d, Size %5d",
					mediatime, FilmF.time, FilmF.duration, size);
	
			/* Note the occurances of key frames...per marker rate */

			if ( gEnableMarkerOutput && (sampleFlags != mediaSampleNotSync) )
			{
				if (((FilmF.time - lastMarkerTime) >= (long) (gMarkerRate * AUDIO_TICKS_FIXED)) ||
						( FilmF.time == 0 && gStartTime != 0))
				{
					fprintf( keyFile, "markertime %d\n", FilmF.time + gStartTime );
					VFPRINTF( stderr,"\t\t\tmarkertime %d, lastMarkerTime <%d>\n", FilmF.time, lastMarkerTime );

					/* Update lastMarkerTime */
					lastMarkerTime = FilmF.time;
				}
			}
		
			/* If the remaining amount of this phisical block is less than
			 * the size of a filler chunk than bump up the chunk size.
			 * If we are not outputting fill, then this won't be a problem
			 */
			if (gWriteFillerChunks && (remainingBlockSize - FilmF.chunkSize) < sizeof(FillerChunk) )
				FilmF.chunkSize = remainingBlockSize;

			/* Write out the Film Frame Chunk.
			 */
			writeSize = sizeof(FilmFrame);
			if ((Oops = FSWrite(FilmFile, &writeSize, &FilmF)) != noErr)
			{
				fprintf(stderr,"### %d Occurred while writing Film Frame Chunk to FilmFile\n", Oops);
				goto CLEAN_UP;
			}
	
			/* Write out the Film Frame Data.
			 */
			if (! isFlixFormat)
			{
				writeSize = FilmF.chunkSize - sizeof(FilmFrame);
				if ((Oops = FSWrite(FilmFile, &writeSize, AlignedData)) != noErr)
				{
					fprintf(stderr,"### %d Occurred while writing Film Frame Data to FilmFile\n", Oops);
					goto CLEAN_UP;
				}
			}
			/* Write out the EZFlix Frame Data.
			 */
			else if (gOpera)
			{
				FlixFrameHeader* qtFlixData;
				
				/* Rearrange the order of the U, V, and Y portions of the frame */
				qtFlixData = (FlixFrameHeader*) AlignedData;
				writeSize = FilmF.chunkSize - sizeof(FilmFrame)
								- (long)(qtFlixData->chunk3Offset<<2)
								+ (long)(qtFlixData->chunk1Offset<<2); 		/* amount of Y data */
				if ((Oops = FSWrite(FilmFile, &writeSize, (char*)qtFlixData 
									+ (long)(qtFlixData->chunk3Offset<<2))) != noErr)
				{
					fprintf(stderr,"### %d Occurred while writing EZFlix Frame Data to FilmFile\n", Oops);
					goto CLEAN_UP;
				}
				writeSize = FilmF.chunkSize - sizeof(FilmFrame) - writeSize; /* remaining amount to write */
				if ((Oops = FSWrite(FilmFile, &writeSize, (long*)qtFlixData + (qtFlixData->chunk1Offset))) != noErr)
				{
					fprintf(stderr,"### %d Occurred while writing EZFlix Frame Data to FilmFile\n", Oops);
					goto CLEAN_UP;
				}
			}
			else /* M2 EZFlix frame output */
			{
				FlixFrameHeader* qtFlixData = (FlixFrameHeader*) AlignedData;
				
				/* write the FlixFrameHeader and the compressed data that follows */
				writeSize = FilmF.chunkSize - sizeof(FilmFrame); /* remaining amount to write */
				if ((Oops = FSWrite(FilmFile, &writeSize, qtFlixData)) != noErr)
				{
					fprintf(stderr,"### %d Occurred while writing EZFlix Frame Data to FilmFile\n", Oops);
					goto CLEAN_UP;
				}
			}
		
			SpinCursor(32);

			/* Update the current video time to just after this frame
			 * and decrement the remainingBlockSize.
			 */
			currentVideoTime = sampleTime + durationPerSample;
			remainingBlockSize -= FilmF.chunkSize;
				
			if (remainingBlockSize == 0)
				remainingBlockSize = gPhysBlockSize;

			frameCount--;
			
		} while (( frameCount > 0 ) &&
				( currentVideoTime < soundTimeInMediaUnits ));

	} /* while ( frameCount > 0 )  */
	
	/* Write out a MRKR control chunk to send this stream back to the top */
	if (gLoopIt)
		WriteMarkerChunk (FilmFile, &remainingBlockSize, 0);
	
	/* Make sure we have a file containing complete phys. blocks */
	if (remainingBlockSize)
		WriteFillerChunk (FilmFile, remainingBlockSize);

CLEAN_UP:

	/* Close the marker output file if we opened one */
	if ( gEnableMarkerOutput && keyFile != nil )
		fclose( keyFile );

	if ( sampledata != nil )
		DisposeHandle(sampledata);
		
	if ( sounddata != nil )
		DisposeHandle(sounddata);
		
	if ( IDesc != nil )
		DisposeHandle((Handle)IDesc);
		
	if ( SDesc != nil )
		DisposeHandle((Handle)SDesc);
		
	if ( AlignedData != nil )
		free(AlignedData);
	
	/* Close the output file if we opened one. */
	if ( FilmFile )
		FSClose(FilmFile);

	return Oops;
}


/*
 * MakeDestPath - make the movie destination name by pulling the source file name from 
 *		it's full path and adding the name to the destination folder path.  
 *
 *		sourceFile	- Src Disk:Folder1:Folder2:source movie
 *		destDir		- Dest Disk:Films:
 *	gives:
 *					- Dest Disk:Films:source movie.FILM
 */
void	
MakeDestPath(StringPtr resultingPath, StringPtr sourceFile, StringPtr destDir, StringPtr newFileExt)
{
	short	srcLastColon,
			destDirLen,
			fileNameLen;

	/* first, find the last colon in the source file name */
	for ( srcLastColon = StrLength(sourceFile); srcLastColon > 0 && sourceFile[srcLastColon] != ':'; 
			srcLastColon-- )
		;

	/* do we have a dest path?  if not, copy the path of the original */
	if (StrLength(destDir) == 0 && srcLastColon > 0)
	{
		BlockMove(sourceFile, destDir, srcLastColon + 1);
		destDir[0] = srcLastColon;
	}
	
	/* make sure the dest path is a valid directory (last char = ':') */
	destDirLen = *destDir;
	if ( destDir[destDirLen] != ':' )
	{
		destDir[++destDirLen] = ':';
		*destDir = destDirLen;
	}

	/* construct the dest file */
	BlockMove(destDir, resultingPath, destDirLen + 1);
	fileNameLen = sourceFile[0] - srcLastColon;	
	BlockMove(&sourceFile[srcLastColon + 1], &resultingPath[destDirLen + 1], fileNameLen);
	resultingPath[0] += fileNameLen;

	/* and finally, append the file extension */
	BlockMove(&newFileExt[1], &resultingPath[resultingPath[0] + 1], StrLength(newFileExt));
	resultingPath[0] += StrLength(newFileExt);
}


/*
 * Usage - output command line options and flags
 */
static void Usage( char *toolName )
	{
	fprintf( stderr, "# Version %s\n", PROGRAM_VERSION_STRING );
	fprintf( stderr, "usage: %s flags movieFile(s)É \n", toolName );
	fprintf( stderr, "		[-b <size>] blocking size\n" );
	fprintf( stderr, "		[-c <channel>]\n" );
	fprintf( stderr, "		[-d <outputDirectory>]\n" );
	/*  fprintf( stderr, "		[-k] flag key frames, ala MovieToStream_Shuttle\n");  */
	fprintf( stderr, "		[-l] loop to first frame\n" );
	fprintf( stderr, "		[-m <rate>] marker rate (sec)\n" );
	fprintf( stderr, "		[-o <outputfilename>]\n" );
	fprintf( stderr, "		[-opera] output an Opera compatible stream\n" );
	fprintf( stderr, "		[-s <start time>] audio ticks (0)\n");
	fprintf( stderr, "		[-v] toggle verbose diagnostic output\n" );
	fprintf( stderr, "		[movieFile(s)É].\n" );
	}

/* Do the conversion */
main(int argc, char *argv[])
{
	OSErr	status;
	long	parms;
	long	files;
	Movie	theMovie;
	Str255	newFileName = "\p";
	Str255	newFilePath = "\p";
	Str255	destFilmDir = "\p";
	Str255	keyFilePath = "\p";		
	Str255	timeStr;
	char	*srcMoviePath;
	char	*pEnd;

	char	*outFilePath	= NULL;
	char	*newFilePathPtr	= (char*)newFilePath;
	Str255	FileNum			= "\0";
	
	long	result;

	status = files = 0;
	gToolName = argv[0];

	/* So we can spin cursor */
	InitCursorCtl(NULL);

	/* Check to see if QuickTime is installed */
	if ( ( status = Gestalt( gestaltQuickTime, &result ) ) != noErr )
	{
		fprintf(stderr, "QuickTimeª is not installed.\n");
		fprintf(stderr, "Please place the QuickTimeª extension in your system folder and restart your Machintosh.\n");
		exit(status);
	}
	
	for (parms = 1; parms < argc; parms++) 
	{
		/* not an option? must be a file name.  move it down so all file names are sequential */
		if (*argv[parms] != '-') 
		{
			argv[++files] = argv[parms];
		} 
		else if ( (tolower(*(argv[parms]+1)) == 'd') && (strlen(argv[parms]) == 2) ) 
		{
			/* bump to the next param & grab the output directory, as long as we have a next param */
			/*  of course */
			++parms;
			if ( parms == argc )
			{
				fprintf(stderr,"### %s - invalid output directory.\n", argv[0]);
				Usage( argv[0] );
				return EXIT_FAILURE;
			}
			
			strcpy((char*)destFilmDir, argv[parms]);
			c2pstr((char*)destFilmDir);
		} 
		else if ( (tolower(*(argv[parms]+1)) == 'b') && (strlen(argv[parms]) == 2) ) 
		{
			/* bump to the next param & grab the blocking size, as long as we have a next param */
			/*  of course */
			long physBlockSize;
			++parms;
			if ( parms == argc )
			{
				fprintf(stderr,"### %s - invalid physical block size.\n", argv[0]);
				Usage( argv[0] );
				return EXIT_FAILURE;
			}
			
			physBlockSize = strtol(argv[parms], &pEnd, 10);
			if (pEnd[0] != 0) {
				fprintf(stderr,"### %s - invalid block size: %s\n", argv[0], argv[parms]);
				return EXIT_FAILURE;
			}

			/* Make sure the blocksize is a multiple of 2048 bytes. */
			if ( (physBlockSize % 2048) != 0 )
			{
				fprintf( stderr, "### %s -  Block size must be multiple of 2K bytes.\n", argv[0] );
				return EXIT_FAILURE;
			}
			gPhysBlockSize = physBlockSize;
			gWriteFillerChunks = true;
		} 
		else if ( (tolower(*(argv[parms]+1)) == 'm') && (strlen(argv[parms]) == 2) ) 
		{
			/* bump to the next param & grab the marker rate, as long as we have a next param */
			/*  of course */
			++parms;
			if ( parms == argc )
			{
				fprintf(stderr,"### %s - invalid marker rate.\n", argv[0]);
				Usage( argv[0] );
				return EXIT_FAILURE;
			}
			gEnableMarkerOutput = true;
			gMarkerRate = strtod(argv[parms], &pEnd);
			if (pEnd[0] != 0) {
				fprintf(stderr,"### %s - invalid marker rate: %s\n", argv[0], argv[parms]);
				return EXIT_FAILURE;
			}
		}
		else if ( (tolower(*(argv[parms]+1)) == 's') && (strlen(argv[parms]) == 2) ) 
		{
			/* bump to the next param & grab the start time, as long as we have a next param */
			/*  of course */
			++parms;
			if ( parms == argc )
			{
				fprintf(stderr,"### %s - invalid start time.\n", argv[0]);
				Usage( argv[0] );
				return EXIT_FAILURE;
			}
			gStartTime = strtol(argv[parms], &pEnd, 10);
			if (pEnd[0] != 0) {
				fprintf(stderr,"### %s - invalid start time: %s\n", argv[0], argv[parms]);
				return EXIT_FAILURE;
			}
		}
		/* Key frame tags are no longer supported */
#if 0
		else if ( (tolower(argv[parms][1]) == 'k') && (strlen(argv[parms]) == 2) ) 
		{
			/* flagging key frames, ala MovieToStream_Shuttle */
			gFlagKeyFrames = true;
		}
#endif	
		else if ( (tolower(*(argv[parms]+1)) == 'c') && (strlen(argv[parms]) == 2) ) 
		{
			/* bump to the next param & grab the channel, as long as we have a next param */
			/*  of course */
			++parms;
			if ( parms == argc )
			{
				fprintf(stderr,"### %s - invalid channel.\n", argv[0]);
				Usage( argv[0] );
				return EXIT_FAILURE;
			}
			gChannel = strtol(argv[parms], &pEnd, 10);
			if (pEnd[0] != 0) {
				fprintf(stderr,"### %s - invalid channel number: %s\n", argv[0], argv[parms]);
				return EXIT_FAILURE;
			}
		}

		else if (tolower(*(argv[parms]+1)) == 'l')
		{
			gLoopIt = true;
		} 
		else if (tolower(*(argv[parms]+1)) == 'v')
		{
			gVerbose = true;
		} 

		else if ( tolower(*(argv[parms]+1)) == 'o' )
		{
			if (parms + 1 > argc)
			{
				Usage( argv[0] );
				return EXIT_FAILURE;
			}
			else if (strlen(argv[parms]) == 2)
			{
				/* bump to the next param and grab the output filename. */
				++parms;
			
				outFilePath  = argv[parms];	/* save pointer to file name */
			}
			else if (strlen(argv[parms]) == 6)
			{					/* -Opera */
				if ( tolower(argv[parms][2]) == 'p' &&
					 tolower(argv[parms][3]) == 'e' &&
					 tolower(argv[parms][4]) == 'r' &&
					 tolower(argv[parms][5]) == 'a' )
				{ 
					gOpera = true;
				}
			}
			else
			{
				Usage( argv[0] );
				return EXIT_FAILURE;
			}
		}
		else 
		{
			fprintf(stderr,"### %s - \"%s\" is not an option.\n", argv[0], argv[parms]);
			Usage( argv[0] );
			return EXIT_FAILURE;
		}
	}

	/* Spin cursor before we start processing the files. */
	SpinCursor(32);	

	/* process the files */
	if ( files == 0 ) 
	{
		fprintf(stderr,"### No files specified\n");
		Usage( argv[0] );
	} 
	else 
	{
		for (parms = 1; parms <= files; parms++) 
		{
			Str255 nameTail = "\p.FILM";
			
			InitGraf((Ptr) &qd.thePort);				/* Need an a5 world to use Quicktime */
			srcMoviePath = argv[parms];
			
			/* build the new file name from the output file name (if specified) or the source */
			if ( outFilePath )
			{
				strcpy((char*)newFileName, outFilePath);
				nameTail[0] = 0; 						/* nameTail = "\p"; */
			}
			else
			{
				strcpy((char*)newFileName, srcMoviePath);
			}
			
			/* convert the new file name to a pstring */
			c2pstr((char*)newFileName);
				
			/* convert the source path to a pstring */
			c2pstr((char*)srcMoviePath);

			/* Make the destination path from the new file name, and the specified directory */
			MakeDestPath((StringPtr)newFilePathPtr, newFileName, (StringPtr)destFilmDir, nameTail);

			/* If there's more than one movie file to stream
			 * append a number to the filename.
			 */
			if ( files > 1 )
			{	
				sprintf ( (char*)FileNum, (char*)"%d", parms );
				
				/* Append number to the end of filename. */
				p2cstr((unsigned char*)newFilePathPtr);
				strcat((char*)newFilePathPtr, (char*)FileNum);
				c2pstr( newFilePathPtr );
				
			}
				
			MakeDestPath((StringPtr)keyFilePath, (StringPtr)newFileName, (StringPtr)destFilmDir, (StringPtr)"\p.KEY");
			status = EnterMovies();
			if ( (theMovie = OpenMovie((unsigned char*)srcMoviePath)) != 0 )
			{
				gEmptyPtr = NewPtrClear(gPhysBlockSize);
				VFPRINTF(stderr,"%s - Begin converting \"%P\" to \"%P\"...\n", CurrTime((char *)timeStr),  (StringPtr)srcMoviePath, newFilePathPtr);
				status = DumpMovie(theMovie, (StringPtr)newFilePathPtr, keyFilePath);
				if ( status == noErr )
					VFPRINTF(stderr,"%s - Finished converting \"%P\"\n\n", CurrTime((char *)timeStr), newFilePathPtr);

				DisposPtr(gEmptyPtr);
			}
			else
				fprintf(stderr, "### Error processing \"%P\" movie file.\n", srcMoviePath );

			ExitMovies();
		}
	}
	if (status != noErr) 
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
