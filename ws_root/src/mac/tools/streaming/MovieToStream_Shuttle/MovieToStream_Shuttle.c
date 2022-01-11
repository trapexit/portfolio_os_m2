
/******************************************************************************
**
**  @(#) MovieToStream_Shuttle.c 95/05/26 1.1
**
******************************************************************************/
/*------------------------------------------------------------------------------

NAME
      MovieToStream_Shuttle --	MPW tool to convert Cinepak compressed QuickTime movie
	  					to a 3DO DataStream chunk file.

SYNOPSIS
      MovieToStream_Shuttle [flags] [movieFile(s)�]

------------------------------------------------------------------------------*/

#include	<Types.h>
#include 	<ctype.h>
#include 	<fcntl.h>
#include 	<string.h>
#include 	<stdio.h>
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

#include	"films.h"
#include	"StdLib.h"
#include	"VQDecoder.h"

#define	PROGRAM_VERSION_STRING	"2.1"

#define	kPhysBlockSize			(32 * 1024)
#define	kMinSoundChunkSize		(sizeof(StreamSoundChunk) + 1024)
#define	kMaxTrackCount			50

/* Define Audio ticks (44100/184) in double format.
 */
#define	AUDIO_TICKS_FIXED	(double) (44100.0 / 184.0)

#define 	COMPRESSION_TYPE_NONE		'NONE'

/******************************************/
/* Command line settable global variables */
/******************************************/
long		gPhysBlockSize		= kPhysBlockSize; /* default to 32k blocking factor */
//long		gFrameRate			= 30;			/* default frame rate == 30fps */
Boolean		gEnableMarkerOutput	= false;		/* default is no marker output file (set by -m) */
double		gMarkerRate 		= 1.0; 			/* default markers to 1 second intervals */
Boolean		gLoopIt				= false;		/* default for movie looping == FALSE */
Boolean		gVerbose			= false;		/* default for verbose output == FALSE */
Ptr			gEmptyPtr;
long		gStartTime			= 0;			/* default for a one clip movie */				// lla 12/6/93

long		gChannel 			= 0; 			/* default channel == 0 */

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
	
	if (Oops = FSMakeFSSpec(0,0,MovieName,&Spec))	{
		fprintf(stderr,"### %d Occurred while FSMakeFSSpec for %P\n", Oops, MovieName);
		return 0;
	}
	if (Oops = OpenMovieFile(&Spec,&resRefNum,0))	{
		fprintf(stderr,"### %d Occurred while opening %P\n", Oops, MovieName);
		return 0;
	}
	resID = 0;
	if (Oops = NewMovieFromFile(&theMovie,resRefNum,&resID,theStr,0,&dataRefWasChanged))	{
		fprintf(stderr,"### %d Occurred while opening movie in %P\n", Oops, theStr);
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
				
	*(FrameHeader *)dd = *(FrameHeader *)sd;
	dd += sizeof(FrameHeader);
	*((long *)dd)++ = 0xFE000006;
	*((short *)dd)++ = 0x0000;
	
	sd += sizeof(FrameHeader);
	
	// Loop through all tiles and expand every code book
	
	for (t = 0; t < tileCount; t++) 
	{
		// parse atoms within frame looking for tiles
		
		do {
			tileType = *(unsigned char *)sd;
			if (tileType == kTileKeyType8 || tileType == kTileInterType8)
				break;
			sd += *(long *)sd & 0x00FFFFFF;
		} while (true);
		
		// skip tile header and get length of tile atom
		
		tileStart = dd;
		tileEnd = sd + (*(long *)sd & 0x00FFFFFF);
		*(TileHeader *)dd = *(TileHeader *)sd;		// Copy the frame header
		dd += sizeof(TileHeader);
		sd += sizeof(TileHeader);
		
		// parse atoms within tile looking for codebooks
		
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
	IUTimeString(currTime, true, timeStr);
	return p2cstr(timeStr);
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

	if (remainingBlockSize < sizeof(FillerChunk)) {
		fprintf(stderr,"### %d Too small for filler chunk --- wrote zeros.\n", remainingBlockSize);
		writeSize = remainingBlockSize;
		Oops = FSWrite(filmFile,&writeSize,gEmptyPtr);
		return;
	}
		
	fill.chunkType = kFillerChunkType;
	fill.chunkSize = remainingBlockSize;
	
	writeSize = sizeof(FillerChunk);
	if (Oops = FSWrite(filmFile,&writeSize,&fill))	{
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
	CtrlMarkerChunk		mark;
	long				writeSize;
	OSErr				Oops;

	if (*remainingBlockSize < sizeof(CtrlMarkerChunk)) {
		fprintf(stderr,"### remainingBlockSize less than CtrlMarkerChunk!!!\n");
		return;
	}
		
	mark.chunkType		= CTRL_CHUNK_TYPE;
	mark.chunkSize		= sizeof(CtrlMarkerChunk);
//dtc	mark.time			= 0;
	mark.time			= gStartTime;
	mark.chunkFlags		= 0;
	mark.channel		= gChannel;
	mark.subChunkType	= GOTO_CHUNK_TYPE;
	mark.marker			= offset;
	
	writeSize = sizeof(CtrlMarkerChunk);
	if (Oops = FSWrite(filmFile,&writeSize,&mark))	{
		fprintf(stderr,"### %d Occurred during WriteMarkerChunk\n", Oops);
	}
	
	*remainingBlockSize -= sizeof(CtrlMarkerChunk);

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

	if ((pSSndH->sampleSize == 8) && (pSSndH->dataFormat == 'raw ')) {
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

	numBytes = numberOfSamples * (pSSndH->sampleSize >> 3) * pSSndH->numChannels;
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
	conversionValue = (UInt32)pSSndH->sampleRate / pFilmH->scale;
	
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
	conversionValue = (double)pSSndH->sampleRate / (double)pFilmH->scale;
	
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
// lla 12/23/93		attempt to correct inaccuracy in 240
//	conversionValue = (extended)( Fix2X(240 << 16) / Fix2X(scale) );
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
	remBlkTime = remainingBlockSize / (pSSndH->sampleSize >> 3) / pSSndH->numChannels;
	soundSampleTime = soundChunkTime / (pSSndH->sampleSize >> 3) / pSSndH->numChannels;
	
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
	return (numberOfSamples * (pSSndH->sampleSize >> 3) * pSSndH->numChannels);
}



OSErr	
DumpMovie(Movie theMovie, Str255 filmFileName, Str255 keyFileName)
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
	long				size,writeSize,remainingBlockSize;
	ImageDescription	**IDesc;
	SoundDescription	**SDesc;
	long				soundScale;
	long				currentSoundTime,currentVideoTime, soundTimeInMediaUnits;
	FilmHeader			FilmH;
	FilmFrame			FilmF;
	StreamSoundHeader	SSndH;
	StreamSoundChunk	SSndC;
	Handle				sampledata	= nil;
	Handle				sounddata	= nil;
	short				FilmFile;
	Ptr					AlignedData	= nil;
	Boolean				soundLeft = true;
	
	FILE*				keyFile		= nil;
	TimeValue			lastMarkerTime = 0;	
	
	
	Oops = Create(filmFileName,0,'CYBD','FILM');
	if ( (Oops != noErr) && (Oops != dupFNErr))	
	{
		fprintf(stderr,"### %d Occurred during Create of %P\n", Oops, filmFileName);
		goto CLEAN_UP;
	}

	if ( Oops = FSOpen(filmFileName,0,&FilmFile) ) 
	{
		fprintf(stderr,"### %d Occurred during Open of %P\n", Oops, filmFileName);
		goto CLEAN_UP;
	}

	/* Reset the file position to the beginning if file exists. */
	if ( Oops == dupFNErr )
	{
		if ( Oops = SetFPos( FilmFile, fsFromStart,0 ) ) 
		{
			fprintf(stderr,"### %d Occurred during SetFPos of %P\n", Oops, filmFileName);
			goto CLEAN_UP;
		} /* if noErr != SetFPos */

		if ( Oops = SetEOF ( FilmFile, (long) 0 ) )
		{
			fprintf(stderr,"### %d Occurred during SetEOF of %P\n", Oops, filmFileName);
			goto CLEAN_UP;
		} /* if noErr != SetEOF */
		
	} /* if Oops == dupFNErr */
	
	/* Open output marker text file if marker output is enabled
	 */
	if ( gEnableMarkerOutput )
		{
		p2cstr( keyFileName );
		keyFile = fopen( keyFileName, "w" );
		if ( keyFile == NULL )
			{
			fprintf(stderr,"### Error opening marker file: %s\n", keyFileName );
			goto CLEAN_UP;
			}
		}

	SpinCursor(32);

//	Get the length of the movie in movie time scale
	movieDuration = GetMovieDuration(theMovie);
	
	
//	Find the video media and the sound media for this movie

	for (n = 0; n < kMaxTrackCount; n++)	
	{
		VideoMediaTrackOffset[n] = 0;
		SoundMedia[n] = VideoMedia[n] = nil;
	}
	
	SoundMediaCount = 0;
	VideoMediaCount = 0;
	
	SpinCursor(32);

// make sure it doesn't have too many tracks for us to handle

	if ( (numberOfTracks = GetMovieTrackCount(theMovie)) >= kMaxTrackCount )
	{
		fprintf(stderr,"#### Can't convert this movie, it has more than %d tracks.  Sorry\n", kMaxTrackCount);
		goto CLEAN_UP;
	}


// and grab a reference to each

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

			// make sure that it is a compact video track!
			GetMediaSampleDescription(m,1,(SampleDescriptionHandle)IDesc);
			if ( (*IDesc)->cType != 'cvid' )
			{
				fprintf(stderr,"\t\t#### This track is not compressed with Compact Video.\n");
				fprintf(stderr,"\t\t#### Recompress the movie and try again.\n");
				goto CLEAN_UP;
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
			VFPRINTF(stderr,"\t  SDesc sampleRate: %d\n", ( ((UInt32)(*SDesc)->sampleRate) >> 16) );
			
		}
	}
			

//	Create the film header -----------------------------------------------------------

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
	
	FilmH.chunkType = kVideoChunkType;
	FilmH.chunkSize = sizeof(FilmHeader);
	FilmH.subChunkType = kVideoHeaderType;
//dtc	FilmH.time			= 0;
	FilmH.time			= gStartTime;
	FilmH.chunkFlags	= 0;
	FilmH.channel		= gChannel;
	FilmH.version		= 0;
	FilmH.cType			= (*IDesc)->cType;
	FilmH.height		= (*IDesc)->height;
	FilmH.width			= (*IDesc)->width;
	FilmH.scale		= movieTimeScale / ( movieDuration / frameCount );
	FilmH.count			= frameCount;
	
	VFPRINTF(stderr,"\t  videoScale: %ld\n", mediaTimeScale );
	VFPRINTF(stderr,"\t  video frame count: %d\n",frameCount);

	movieDuration = movieDuration / ( movieTimeScale / mediaTimeScale);
	VFPRINTF(stderr,"\t  movieDuration: %d\n",movieDuration);

	DisposeHandle((Handle)IDesc);


	SpinCursor(32);

//	Write the Film Header to disk -----------------------------------------------------------

	writeSize = sizeof(FilmHeader);
	if (Oops = FSWrite(FilmFile,&writeSize,&FilmH))	{
		fprintf(stderr,"### %d Occurred during FSWrite of %P\n", Oops, filmFileName);
		goto CLEAN_UP;
	}

	
	
//	Create the Streamed Sound Header -----------------------------------------------------------

	currentSoundTime = 0;
	soundTimeInMediaUnits	= 0;
	
	/*	��� TEMP TEMP TEMP ���	*/
	/*		Defeat the Sound	*/

	SoundMedia[0] = nil;
	
	if (SoundMedia[0]) 
	{
		soundScale			= GetMediaTimeScale(SoundMedia[0]);
		sounddata			= NewHandle(0);
		
		SSndH.chunkType 	= kSoundChunkType;
		SSndH.chunkSize 	= sizeof(StreamSoundHeader);
		SSndH.subChunkType	= kSoundHeaderType;
//dtc		SSndH.time			= 0;
		SSndH.time			= gStartTime;
		SSndH.channel		= gChannel + 1;	/* Use a different channel for the audio data */;
		SSndH.dataFormat	= (*SDesc)->dataFormat;
		SSndH.numChannels	= (*SDesc)->numChannels;
		SSndH.sampleSize	= (*SDesc)->sampleSize;
		SSndH.sampleRate	= ((UInt32)(*SDesc)->sampleRate>>16);
		SSndH.sampleCount	= GetMediaSampleCount(SoundMedia[0]);
		SSndH.numBuffers 	= 8;					/* max # of buffers that can be queued to the AudioFolio	*/
		SSndH.initialAmplitude	= 0x7500;			/* initial volume of the stream */
		SSndH.initialPan 		= 0x4000;			/* initial pan of the stream */
		SSndH.compressionType 	= COMPRESSION_TYPE_NONE;
		SSndH.compressionRatio 	= 1;
		SSndH.version 		= 0;
		
		VFPRINTF(stderr,"\t  %d sound samples at %d khz\n",SSndH.sampleCount,soundScale);
	}
	/* If no sound make a dummy sound header to use in the conversion routines.
	 */
	else {
		SSndH.channel		= gChannel;
		SSndH.numChannels	= 1;
		SSndH.sampleSize	= 8;
		SSndH.sampleRate	= 44100;
		soundLeft			= false;	
	}

	DisposeHandle((Handle)SDesc);

	
	SpinCursor(32);

//	Write the Streamed Sound Header to disk -----------------------------------------------

	if (SoundMedia[0]) 
	{
		writeSize = sizeof(StreamSoundHeader);
		if (Oops = FSWrite(FilmFile,&writeSize,&SSndH))	{
			fprintf(stderr,"### %d Occurred during FSWrite of %P\n", Oops, filmFileName);
			goto CLEAN_UP;
		}
	}
	
//	Copy all the audio samples and video frames as chunks ---------------------------------

	VFPRINTF(stderr,"\tconverting media...\n");
	sampledata = NewHandle(0);
	AlignedData = malloc(gPhysBlockSize);
	if (!AlignedData) {
		return memFullErr;
	}
	
	
	/* Make the remainingBlockSize allow for the headers we just wrote out.
	 */
	remainingBlockSize = gPhysBlockSize;
	if (VideoMedia[0]) remainingBlockSize -= sizeof(FilmHeader);
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
				
				// Check the size of each sample.  Make sure that the
				// size of each sample will fit into the physical block size.
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
				SSndC.channel		= gChannel + 1;	/* Use a different channel for the audio data */
//dtc				SSndC.time			= CvtTo3DOTime( currentSoundTime, SSndH.sampleRate );
				SSndC.time			= CvtTo3DOTime( currentSoundTime, SSndH.sampleRate ) + gStartTime;
				/* SSndC.numSamples	= numberOfSamples; ��� might need this someday... */
			
				writeSize = sizeof(StreamSoundChunk);
				if (Oops = FSWrite(FilmFile,&writeSize,&SSndC))	{
					fprintf(stderr,"### %d Occurred while writing StreamSoundChunk to FilmFile\n", Oops);
					goto CLEAN_UP;
				}
	
				/* Convert the data if the format is not Twos Complement.
				 * The Audio Folio needs samples in Twos Complement format.
				 */
				CvtToTwosComplement (&SSndH, *sounddata, size);
			
	
				if (Oops = FSWrite(FilmFile,&size,*sounddata))	{
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
				if ( currentSoundTime >= SSndH.sampleCount ) {
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

//	LOOP UNTIL WE HAVE WRITTEN ONE SECOND OF VIDEO ���
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
			
			// Check the size of each sample.  Make sure that the
			// size of each sample will fit into the physical block size.
			if ( size > gPhysBlockSize )
			{
				fprintf(stderr,"### Error : Sample size (%d) is greater than the physical blocksize (%d).\n",
					size, gPhysBlockSize );
				fprintf(stderr, "### Error : Increase the blocksize and re-run the program.\n\n");
				Oops = maxSizeToGrowTooSmall;
				goto CLEAN_UP;
			}
			
			size = AlignSample(*sampledata,AlignedData);
			
			if ( (sizeof(FilmFrame) + size) > remainingBlockSize)
			{
				WriteFillerChunk (FilmFile, remainingBlockSize);
				remainingBlockSize = gPhysBlockSize;
			}
	
			/* Adjust sampleTime to represent movie based time.
			 */
			sampleTime = mediatime + VideoMediaTrackOffset[x];
			
			VFPRINTF(stderr,"Media Time %3d, sampleTime %6d, Duration %3d, Size %5d\n",
	 				mediatime, sampleTime, durationPerSample, size);
	
			FilmF.chunkType 	= kVideoChunkType;
			FilmF.chunkSize 	= sizeof(FilmFrame) + size;

//lla 1/25/94
			/* distinguish between key frames and difference frames */
			if ( sampleFlags != mediaSampleNotSync )
				{
				FilmF.chunkFlags	= kKeyChunkFlag;
				FilmF.subChunkType	= kVideoKeyFrameType;
				}
			else
				{
				FilmF.chunkFlags	= kDependentChunkFlag;
				FilmF.subChunkType	= kVideoDifferenceFrameType;
				}
			
			FilmF.channel		= gChannel;
//dtc			FilmF.time			= CvtTo3DOTime( sampleTime, (Fixed)(mediaTimeScale << 16) );
			FilmF.time			= CvtTo3DOTime( sampleTime, mediaTimeScale );
			FilmF.duration		= CvtTo3DOTime( durationPerSample, mediaTimeScale ); /* Bug #3616 */
			FilmF.frameSize		= size;
		
		
			// Note the occurances of key frames...per marker rate

			if ( gEnableMarkerOutput && (sampleFlags != mediaSampleNotSync) )
				{
				if ( (FilmF.time - lastMarkerTime) >= (long) (gMarkerRate * AUDIO_TICKS_FIXED) )
//lla stuff
					{
					fprintf( keyFile, "markertime %d\n", FilmF.time + gStartTime );
					VFPRINTF( stderr,"\t\t\tmarkertime %d, lastMarkerTime <%d>\n", FilmF.time, lastMarkerTime );
					lastMarkerTime = FilmF.time;
					}
				else if ( FilmF.time == 0 && gStartTime != 0)
					{
					fprintf( keyFile, "markertime %d\n", FilmF.time + gStartTime );
					VFPRINTF( stderr,"\t\t\tmarkertime %d, lastMarkerTime <%d>\n", FilmF.time, lastMarkerTime );
					lastMarkerTime = FilmF.time;
					}
//lla stuff
				}

		
			/* If the remaining amount of this phisical block is less than
			 * the size of a filler chunk than bump up the chunk size.
			 */
			if ( (remainingBlockSize - FilmF.chunkSize) < sizeof(FillerChunk) )
				FilmF.chunkSize += remainingBlockSize - FilmF.chunkSize;

			/* Write out the Film Frame Chunk.
			 */
			writeSize = sizeof(FilmFrame);
			if (Oops = FSWrite(FilmFile, &writeSize, &FilmF))
			{
				fprintf(stderr,"### %d Occurred while writing Film Frame Chunk to FilmFile\n", Oops);
				goto CLEAN_UP;
			}
	
	
			/* Write out the Film Frame Data.
			 */
			writeSize = FilmF.chunkSize - sizeof(FilmFrame);
			if (Oops = FSWrite(FilmFile, &writeSize, AlignedData))
			{
				fprintf(stderr,"### %d Occurred while writing Film Frame Data to FilmFile\n", Oops);
				goto CLEAN_UP;
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

	} // while ( frameCount > 0 ) 
	
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
		
	if ( AlignedData != nil )
		free(AlignedData);
	
	// Close the output file if we opened one.
	if ( FilmFile )
		FSClose(FilmFile);

	return Oops;
}


/*
 * MakeDestPath - make the movie destination name by pulling the source file name from 
 *		it's full path and adding the name to the destination folder path.  
 *
 *		sourceFile	- Src Disk:Folder1:Folder2:source movie
 *		destPath	- Dest Disk:Films:
 *	gives:
 *					- Dest Disk:Films:source movie.FILM
 */
void	
MakeDestPath(StringPtr destPath, StringPtr sourceFile, StringPtr destDir, StringPtr newFileExt)
{
	short	srcLastColon,
			destDirLen,
			fileNameLen;

	// first, find the last colon in the source file name
	for ( srcLastColon = StrLength(sourceFile); srcLastColon > 0 && sourceFile[srcLastColon] != ':'; 
			srcLastColon-- )
		;

	// do we have a dest path?  if not, copy the path of the original
	if ( (srcLastColon > 0) && (StrLength(destDir) == 0) )
	{
		BlockMove(sourceFile, destDir, srcLastColon + 1);
		destDir[0] = srcLastColon;
	}
	
	// make sure the dest path is a valid directory (last char = ':')
	destDirLen = *destDir;
	if ( destDir[destDirLen] != ':' )
	{
		destDir[++destDirLen] = ':';
		*destDir = destDirLen;
	}

	// construct the dest file
	BlockMove(destDir, destPath, destDirLen + 1);
	fileNameLen = sourceFile[0] - srcLastColon;	
	BlockMove(&sourceFile[srcLastColon + 1], &destPath[destDirLen + 1], fileNameLen);
	destPath[0] += fileNameLen;

	// and finally, append the file extension
	BlockMove(&newFileExt[1], &destPath[destPath[0] + 1], StrLength(newFileExt));
	destPath[0] += StrLength(newFileExt);
}


/*
 * Usage - output command line options and flags
 */
static void Usage( char *toolName )
	{
	fprintf( stderr, "# Version %s\n", PROGRAM_VERSION_STRING );
	fprintf( stderr, "usage: %s flags movieFile(s)� \n", toolName );
	fprintf( stderr, "		[-b <size>] blocking size\n" );
	fprintf( stderr, "		[-c <channel>]\n" );
	fprintf( stderr, "		[-d <outputDirectory>]\n" );
	fprintf( stderr, "		[-l] loop to first frame\n" );
	fprintf( stderr, "		[-m <rate>] marker rate (sec)\n" );
	fprintf( stderr, "		[-o <outputfilename>]\n" );
	fprintf( stderr,"		[-s <start time>] audio ticks (0)\n");
	fprintf( stderr, "		[-v] toggle verbose diagnostic output\n" );
	fprintf( stderr, "		[movieFile(s)�].\n" );
	}


// Do the conversion
main(int argc, char *argv[])
{
	OSErr	status;
	long	parms;
	long	files;
	Movie	theMovie;
	Str255	newFilmName = "\p";
	Str255	destFilmDir = "\p";
	Str255	keyFileName = "\p";		
	Str255	timeStr;
	char	*srcMovieName;
	char	*pEnd;

	char	*outFileName	= "\0";
	char	*newFnNamePtr	= newFilmName;
	Str255	FileNum			= "\0";
	Boolean	UserOutputFn	= false;

	long	result;

	status = files = 0;

	/* So we can spin cursor */
	InitCursorCtl(NULL);

	// Check to see if QuickTime is installed
	if ( ( status = Gestalt( gestaltQuickTime, &result ) ) != noErr )
	{
		fprintf(stderr, "QuickTime� is not installed.\n");
		fprintf(stderr, "Please place the QuickTime� extension in your system folder and restart your Machintosh.\n");
		exit(status);
	}
	
	for (parms = 1; parms < argc; parms++) 
	{
		// not an option? must be a file name.  move it down so all file names are sequential
		if (*argv[parms] != '-') 
		{
			argv[++files] = argv[parms];
		} 
		else if ( (tolower(*(argv[parms]+1)) == 'd') && (strlen(argv[parms]) == 2) ) 
		{
			// bump to the next param & grab the output directory, as long as we have a next param
			//  of course
			++parms;
			if ( parms == argc )
			{
				fprintf(stderr,"### %s - invalid output directory.\n", argv[0]);
				Usage( argv[0] );
				return EXIT_FAILURE;
			}
			
			strcpy(destFilmDir, argv[parms]);
			c2pstr(destFilmDir);
		} 
		else if ( (tolower(*(argv[parms]+1)) == 'b') && (strlen(argv[parms]) == 2) ) 
		{
			// bump to the next param & grab the blocking size, as long as we have a next param
			//  of course
			++parms;
			if ( parms == argc )
			{
				fprintf(stderr,"### %s - invalid physical block size.\n", argv[0]);
				Usage( argv[0] );
				return EXIT_FAILURE;
			}
			
			gPhysBlockSize = strtol(argv[parms], &pEnd, 10);

			// Make sure the blocksize is a multiple of 2048 bytes.
			if ( (gPhysBlockSize % 2048) != 0 )
				{
				fprintf( stderr, "### %s -  Block size must be multiple of 2K bytes.\n", argv[0] );
				return EXIT_FAILURE;
				}

		} 
		else if ( (tolower(*(argv[parms]+1)) == 'm') && (strlen(argv[parms]) == 2) ) 
		{
			// bump to the next param & grab the marker rate, as long as we have a next param
			//  of course
			++parms;
			if ( parms == argc )
			{
				fprintf(stderr,"### %s - invalid marker rate.\n", argv[0]);
				Usage( argv[0] );
				return EXIT_FAILURE;
			}
			gEnableMarkerOutput = true;
			gMarkerRate = strtod(argv[parms], &pEnd);
		}

		else if ( (tolower(*(argv[parms]+1)) == 's') && (strlen(argv[parms]) == 2) ) 
		{
			// bump to the next param & grab the start time, as long as we have a next param
			//  of course
			++parms;
			if ( parms == argc )
			{
				fprintf(stderr,"### %s - invalid start time.\n", argv[0]);
				Usage( argv[0] );
				return EXIT_FAILURE;
			}
			gStartTime = strtol(argv[parms], &pEnd, 10);
		}
		else if ( (tolower(*(argv[parms]+1)) == 'c') && (strlen(argv[parms]) == 2) ) 
		{
			// bump to the next param & grab the channel, as long as we have a next param
			//  of course
			++parms;
			if ( parms == argc )
			{
				fprintf(stderr,"### %s - invalid channel.\n", argv[0]);
				Usage( argv[0] );
				return EXIT_FAILURE;
			}
			gChannel = strtol(argv[parms], &pEnd, 10);
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
			if ((argc - 2) < 0)
				{
				Usage( argv[0] );
				return EXIT_FAILURE;
				}

			/* bump to the next param and grab the output filename. */
			++parms;
			
			outFileName  = argv[parms];	/* save pointer to file name */
			UserOutputFn = true;
			}
		else 
		{
			fprintf(stderr,"### %s - \"%s\" is not an option.\n", argv[0], argv[parms]);
			Usage( argv[0] );
			return EXIT_FAILURE;
		}
	}
	
	// Spin cursor before we start processing the files.
	SpinCursor(32);	

	// process the files
	if ( files == 0 ) 
	{
		fprintf(stderr,"### No files specified\n");
		Usage( argv[0] );
	} 
	else 
	{
		for (parms = 1; parms <= files; parms++) 
		{

				InitGraf((Ptr) &qd.thePort);			/* Need an a5 world to use Quicktime */
				srcMovieName = c2pstr(argv[parms]);

				/* Use the movie filename if output file not specified. */
				if ( !UserOutputFn ) 
					{
					newFnNamePtr = c2pstr( outFileName );
//					MakeDestPath(newFnNamePtr, srcMovieName, destFilmDir, "\p.chunk");		// ccw
					MakeDestPath(newFnNamePtr, srcMovieName, destFilmDir, "\p.FILM");
					}

				/* Use the user specified output filename. */
				else
					{
					/* If there's more than one movie file to stream
					 * append a number to the filename.
					 */
					if ( files > 1 )
						{	
						/* Make a copy of the user output filename if
						 * there's more than one movie to stream.
						 */
						strcpy( newFnNamePtr, outFileName );

						sprintf ( FileNum, "%d", parms );
						
						/* Append number to the end of filename. */
						strcat( newFnNamePtr, FileNum );
	
						newFnNamePtr = c2pstr( newFnNamePtr );
						} /* if ( files > 1 ) */

					else
						newFnNamePtr = c2pstr( outFileName );
				
					} /* end if-else (!UserOutputFn) */
					
				MakeDestPath(keyFileName, srcMovieName, destFilmDir, "\p.KEY");
				status = EnterMovies();
				if ( (theMovie = OpenMovie(srcMovieName)) != 0 )
				{
					gEmptyPtr = NewPtrClear(gPhysBlockSize);
					fprintf(stderr,"%s - Begin converting \"%P\" to \"%P\"...\n", CurrTime(timeStr), srcMovieName, newFnNamePtr);
					status = DumpMovie(theMovie, newFnNamePtr, keyFileName);
					if ( status == noErr )
						fprintf(stderr,"%s - Finished converting \"%P\"\n\n", CurrTime(timeStr), newFnNamePtr);

					DisposPtr(gEmptyPtr);
				}
				else
					fprintf(stderr, "Error opening \"%P\" movie file.\n", srcMovieName );

				ExitMovies();
			}
	}
	return EXIT_SUCCESS;
}
