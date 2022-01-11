/******************************************************************************
**
**  @(#) EZFlixChunkifier.c 96/04/09 1.11
**
******************************************************************************/

/**
|||	AUTODOC -public -class Streaming_Tools -group EZFlix -name EZFlixChunkifier
|||	Chunkifies an uncompressed QuickTime movie.
|||	
|||	  Format -preformatted
|||	
|||	    EZFlixChunkifier [options] <moviefile>
|||	
|||	  Description
|||	
|||	    Converts an uncompressed ("raw") QuickTime movie to a 3DO chunkified
|||	    format. Performs EZFlix compression as part of this conversion. Is
|||	    an alternative to compressing with a QuickTime app using the EZFlix
|||	    QuickTime component and then chunkifying with QTVideoChunkifier. 
|||	    Enjoys sentences without subjects.
|||	
|||	  Arguments
|||	
|||	    -c <channel>
|||	        Logical channel number.  Default: 0
|||	
|||	    -f <framerate>
|||	        Specifies the frame rate in fps.  Default: 15
|||	
|||	    -h <frameheight>
|||	        Specifies the maximum image frame height, in pixels, which
|||	        will be encoded. If the actual height of the input QuickTime
|||	        movie's frames is greater than <frameheight>, then the tool
|||	        will crop vertically to this number.  Default: 192
|||	
|||	    -i <inputfilename>
|||	        Specifies the QuickTime movie input filename.
|||	
|||	    -o <outputfilename>
|||	        Specifies the output filename. If no name is specified, the output
|||	        filename will be the input filename with .EZFL appended to the end.
|||	
|||	    -opera
|||	        Output an Opera format stream.
|||	
|||	    -q
|||	        Specifies the EZFlix video compression quality, on a scale from
|||	        0 to 100. Currently supported values include 10, 25, 40, 60, or 75.
|||	        Default: 25
|||	
|||	    -v
|||	        Enables verbose diagnostic output.
|||	
|||	    -w <framewidth>
|||	        Specifies the maximum image frame width, in pixels, which
|||	        will be encoded. If the actual width of the input QuickTime
|||	        movie's frames is greater than <framewidth>, then the tool
|||	        will crop horizontally to this number.  Default: 256
|||	
|||	  Example
|||	
|||	    EZFlixChunkifier -i video.moov -o video.EZFL -w 288 -h 208 -f 12
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
#include	<Movies.h>
#include	<GestaltEqu.h>
#include	<StdLib.h>

#if ! GENERATINGPOWERPC
#include 	<sane.h>
#else
#include 	<fenv.h>
#endif

#include	"version.h"
#include	"EZFlixStream.h"
#include	"EZFlixEncoder.h"
#include	"EZFlixDefines.h"

#define	kKeyChunkFlag					1<<0	/* indicates a chunk that does not */

/* Constants */

#define kUncompressedVideoSample	('raw ')		/* signature for uncompressed QuickTime video sample */
#define kOutputFileSuffix			(".EZFL")		/* EZFlix chunk file suffix */
#define kEZFlixFileType				('EZFL')		/* Macintosh type signature for EZFlix chunk file */
#define kEZFlixFileCreator			('EZFL')		/* Macintosh creator signature for EZFlix chunk file */
#define kGenericError				(-1)			/* Used for internal errors */

#define kVideoChannelFlag			('c')
#define kFrameRateFlag				('f')
#define kFrameHeightFlag			('h')
#define kInputFileFlag				('i')
#define kOutputFileFlag				('o')
#define kVerboseFlag				('v')
#define kFrameWidthFlag				('w')
#define kQualityFlag				('q')
#define kOperaFlag					("opera")
#define kMinVideoChannel			0				/* range chack values for channel flag argument */
#define kMaxVideoChannel			1024
#define kMinFrameRate				1				/* range check values for frame-rate flag argument */
#define kMaxFrameRate				60
#define kMinFrameHeight				4				/* range check values for frame-height flag argument */
#define kMaxFrameHeight				480
#define kMinFrameWidth				4				/* range check values for frame-width flag argument */
#define kMaxFrameWidth				640

#define kDefaultFrameHeight			192
#define kDefaultFrameWidth			256
#define kDefaultFrameRate			15
#define kDefaultVideoChannel		0
#define kDefaultQuality				25

#define kChunkBufferSize			98304	/* data chunk cannot be larger than this */

/* A time scale is the number of time units in one second of
 * real time.  For example, the time scale of Redbook audio is
 * 44,100 time units (in this case, audio samples) per second.
 */
#define kCDAudioTimeScale			((ClockType)44100.0)

/* The 3DO audio clock is incremented once for every 184 ticks of
 * the DSP clock, which runs at the same 44.1KHz rate as CD audio.
 * The time scale of the 3DO audio clock (units/second) is therefore
 *		44100/184 = 239.6739 ticks/second,
 * slightly less than an integral 240 ticks/second.
 */
#define k3DOAudioTimeScale			(kCDAudioTimeScale / (ClockType)184.0)

/* Integer approximation of 3DO audio time scale */
#define kApprox3DOAudioTimeScale	240

/* Scale up the time scale of CD audio to yield a very high-precision
 * time scale in which to calculate time values.  A unit of this high-
 * precision time scale will be referred to as a "tick"
 */
#define kClockPrecision				((ClockType)10E8) 	/* Error of 0.01035391 frames per hour */
#define kTicksTimeScale				(kCDAudioTimeScale * kClockPrecision)

/* Compute the number of high-precision ticks in a single
 * 3DO audio time unit.  This is the multiplier used to convert
 * time values in the 3DO audio time scale to time values in the
 * high-precision time scale (and the divisor to convert time values
 * in the high-precision time scale to time values in the 3DO audio
 * time scale).  The 3DO audio time scale is the scale used for all
 * time values appearing in a data stream.
 */
#define kTicksPer3DOAudio		(kTicksTimeScale / k3DOAudioTimeScale)


/* Macros */

/* Return the length of the specified Pascal string. */
#define LENGTH(pasStr)	((unsigned long) (*((char*)(pasStr))))

/* Macro used to make chatty printf's go away unless user
 * specifies the "-v" verbose option.
 */
#define	VFPRINTF				if (gVerbose) fprintf
#define OUTFILE					stdout

/* Yield minimum of two values */
#define MIN(a,b) 				(((a)<(b))?(a):(b))

/* Return the number of high-precision ticks in a single time unit
 * of the specified time scale.  This is the scale conversion factor
 * (multiplier or divisor) between the specified time scale and the
 * high-precision time scale.
 */
#define TICKS_PER_TIME_UNIT(timeScale) 	(kTicksTimeScale / (ClockType)(timeScale))



/* Types */

typedef extended ClockType;		/* high-precision time value */
		


/* Global variables hold command-line options */

static Boolean	gVerbose		= false;				/* enable extra printf output */
static Boolean	gOpera			= false;				/* output Opera compatible stream */
static long		gFrameRate		= 0;					/* frame rate of EZFlix output file */
static long		gVideoChannel	= kDefaultVideoChannel;	/* stream channel on which EZFlix chunks are written */
static long		gFrameHeight	= kDefaultFrameHeight;	/* height of frame of EZFlix */
static long		gFrameWidth		= kDefaultFrameWidth;	/* width of frame of EZFlix */
static Movie	gMovie			= NULL;					/* QuickTime movie structure */
static long		gQuality		= kFixedTwoBitQuality;	/* Quality level for encoding */
	
/* QuickDraw Globals world */
QDGlobals	qd;

/* Truncate to current file position and then close (added by WSD)
 */
static OSErr TruncateAndClose (short vRefNum)
{
	long	filePos;
	OSErr	Oops;

	if ((Oops = GetFPos(vRefNum, &filePos)) == noErr)
		Oops = SetEOF(vRefNum, filePos);

	return FSClose(vRefNum);
}



/* Convert 4-character type to string for printing.
 * str must point to allocated storage.
 */
static char *TypeToStr (char *str, long type)
{
	sprintf(str, "\"%c%c%c%c\"", type >> 24, type >> 16, type >> 8, type);
	return(str);
}



/* Write the specified chunk of data to the specified open file.
 */
static OSErr WriteData (short file, long size, void *data)
{
	OSErr	Oops;
	long	writeSize;

	Oops = noErr;
	writeSize = size;

	if ((Oops = FSWrite(file, &writeSize, data)) != noErr)
		return(Oops);

	if (writeSize != size) 
	{
		fprintf(OUTFILE, "\t### I/O Error: requested bytes not written (written = %d, requested = %d)\n",
				writeSize, size);
		Oops = -1;
	}

	return Oops;
}



/* Read the movie structure from a QuickTime movie file.
 */
static OSErr OpenMovie (Str255 movieFileName, Movie* movie)
{
	OSErr		Oops;				/* local error result */
	FSSpec		fsSpec;				/* file-spec for the movie file */
	short		resRefNum;			/* refnum for opened resource fork of movie file */
	short		resID;				/* resource ID of movie resource */
	Str255		resName;			/* name of movie resource */
	Boolean		dataRefWasChanged;	/* unused argument to NewMovieFromFile() */ 

	/* Open the movie file */

	Oops = FSMakeFSSpec(0, 0, movieFileName, &fsSpec);
	if (Oops < noErr) goto Return;

	Oops = OpenMovieFile(&fsSpec, &resRefNum, 0);
	if (Oops < noErr) goto Return;

	/* Load the first movie resource from the movie file */

	resID = 0;
	Oops = NewMovieFromFile(movie, resRefNum, &resID, resName, 0, &dataRefWasChanged);

	CloseMovieFile(resRefNum);

Return:
	return Oops;
}



/* Convert the uncompressed video samples in the specified QuickTime movie
 * into EZFlix chunks written to the specified output chunk file.
 * Returns the number of frames compressed and an error result.
 */
static OSErr DumpMovie(Movie movie, Str255 ezFlixFileName, long* frameCount)
{ 
	short				ezFlixFileRef;			/* refnum of open EZFlix output chunk file */
	short				movieFrameRate;			/* frame rate of the QuickTime movie */
	ClockType			movieTimeUnitTicks;		/* ticks per one time unit in the movie's time scale */
	Handle				movieSampleData;		/* handle to video sample data from QuickTime movie */
	Ptr					ezFlixData;				/* buffer for data part of EZFlix data chunk */
	ClockType			ezFlixFrameTicks;		/* ticks per frame of EZFlix output */
	ClockType			ezFlixDurationTicks;	/* duration of EZFlix output in high-precision time base */
	EZFlixHeader		ezFlixHeader;			/* EZFlix header chunk */
	EZFlixM2Header		ezFlixM2Header;			/* M2 EZFlix headers are quite different */
	EZFlixFrame			ezFlixFrame;			/* EZFlix frame (data) chunk */
	EZFlixEncoderParams	params;					/* EZFlix encoder parameters carried in header chunk */
	long				videoBytesPerSecond;	/* size of compressed EZFlix data for the next second */
	Boolean				foundVideo;				/* true when a video track has been found */
	ImageDescription	**IDesc;				/* handle to description of QuickTime video sample */
	ClockType 			outputTicks;			/* time of next output chunk in ticks */
	ClockType			secondTime;				/* next one-second time boundary */
	int					second;					/* number of seconds that have been processed */
	long				size;					/* size of frame of compressed EZFlix data */
	TimeValue			mediaTime;				/* time within the QuickTime track's underlying media */
	TimeValue			sampleTime;				/* time stamp of QuickTime video sample */
	TimeValue			durationPerSample;		/* duration of QuickTime video sample */
	char				tempStr[256];			/* general-purpose string buffer */
	short				sampleFlags;			/* dummy argument to GetMediaSample() */
	OSErr				Oops;					/* local error result */
	int					tracksInMovie;			/* number of tracks in the QuickTime movie */
	long				trackNum;				/* index of next track in the QuickTime movie */
	Track				track;					/* descriptor for next track in the QuickTime movie */
	Media				media;					/* QuickTime media organizes the video samples */
	OSType				mediaType;				/* type of media = video, sound, etc. */
	Track				videoTrack;				/* QuickTime track that will be converted */
	Media				videoTrackMedia;		/* QuickTime media that will be converted */
	int					videoTrackHeight;		/* pixel height of QuickTime video sample (frame of movie) */
	int					videoTrackWidth;		/* pixel width of QuickTime video sample (frame of movie) */
	int					videoTrackDepth;		/* bit depth of QuickTime video samples */
	ClockType			videoTrackStartTicks;	/* start time (offset) of video track in ticks */
	ClockType			mediaTimeUnitTicks;		/* ticks per one time unit in the track media's time scale */
	ClockType			mediaDurationTicks;		/* duration of video track media in ticks */
	long				nextFrameTime;			/* time of the next EZFlix frame in audio units */

	*frameCount = 0;
	ezFlixFileRef = 0;
	IDesc = NULL;
	ezFlixData = NULL;
	movieSampleData = NULL;
	movieFrameRate = 0;
	videoTrackHeight = 0;
	videoTrackWidth = 0;
	videoTrackDepth = 0;
	foundVideo = false;

	/* Create and open the output file; overwrite any existing file
	 * of the same name.
	 */
	Oops = Create(ezFlixFileName, 0, kEZFlixFileCreator, kEZFlixFileType);
	if (Oops != noErr && Oops != dupFNErr)
	{
		fprintf(OUTFILE, "### Cannot create output file \"%P\"; error = %d\n", ezFlixFileName, Oops);
		goto Return;
	}

	Oops = FSOpen(ezFlixFileName, 0, &ezFlixFileRef);
	if (Oops != noErr)
	{
		fprintf(OUTFILE, "### Cannot open output file \"%P\"; error = %d\n", ezFlixFileName, Oops);
		goto Return;
	}
	SpinCursor(32);
	
	/* Initialize the EZFlix compressor */
	OpenEZFlixEncoder(gVerbose, gQuality);
	
	/* Allocate the handle to hold a QuickTime video sample description */
	IDesc = (ImageDescription **) NewHandle(sizeof(ImageDescription));

	/* Compute the movie's duration in high-precision ticks.
	 */
	movieTimeUnitTicks = TICKS_PER_TIME_UNIT(GetMovieTimeScale(movie));

	/* Look at each track in the QuickTime movie */

	tracksInMovie = GetMovieTrackCount(movie);

	for (trackNum = 1; trackNum <= tracksInMovie; ++trackNum)
	{
		track = GetMovieIndTrack(movie, trackNum);
		media = GetTrackMedia(track);

		/* See what kind of sample data constitutes the media */

		GetMediaHandlerDescription(media, &mediaType, NULL, NULL);

		if (mediaType == SoundMediaType)
		{
			/* Track contains audio data */
			fprintf(OUTFILE, "\t*Audio Track %n ignored\n", trackNum);
			continue;
		}
		
		if (mediaType != VideoMediaType)
		{
			/* Track contains some other non-video data */
			fprintf(OUTFILE, "\t*Non-Video Track %n ignored\n", trackNum);
			continue;
		}

		/* Track contains video samples.
		 * Select the first video track that contains uncompressed samples.
		 */
		if (foundVideo)
		{
			/* We've already selected the video track we're going to use,
			 * so any others are ignored.
			 */
			fprintf(OUTFILE, "\t*Video Track %n ignored\n", trackNum);
			continue;
		}

		/* Take the first video sample description as representative
		 * of all the video samples in the media.
		 */
		GetMediaSampleDescription(media, 1, (SampleDescriptionHandle)IDesc);

		/* Make sure the video sample is uncompressed */
		if ((**IDesc).cType != kUncompressedVideoSample)
		{
			fprintf(OUTFILE, "\t*Video Track %d type \"%s\" is not supported\n",
					trackNum, TypeToStr(tempStr, (**IDesc).cType));
			fprintf(OUTFILE, "\t Looking for video type \"raw \"\n");
			continue;
		}

		/* This is the video track we'll use.  Hang on to the
		 * information in the sample descriptor.
		 */
		foundVideo = true;
		videoTrackHeight = (**IDesc).height;
		videoTrackWidth = (**IDesc).width;
		videoTrackDepth = (**IDesc).depth;

		/* Save information about track we have selected for conversion */
		videoTrack = track;
		videoTrackMedia = media;
		mediaTimeUnitTicks = TICKS_PER_TIME_UNIT(GetMediaTimeScale(media));
		mediaDurationTicks = GetMediaDuration(media) * mediaTimeUnitTicks;
		videoTrackStartTicks = GetTrackOffset(track) * movieTimeUnitTicks;

		/* Compute a frame rate for the QuickTime input.
		 * Calculation is based on the number of samples in the media
		 * underlying the track, and thus depends on a direct linear mapping
		 * between the track and the media.  The track should not pick-and-choose
		 * from samples in the media, even though QuickTime allows this.
		 */
		movieFrameRate = kTicksTimeScale /
						 ((GetTrackDuration(track) * movieTimeUnitTicks - videoTrackStartTicks)
						  / GetMediaSampleCount(media));

		/* In verbose mode, print some info about this track */

		VFPRINTF(OUTFILE, "\tVideo Track %d: media time scale = %ld, duration = %ld, offset = %ld\n\n",
				 trackNum,
				 (long) (kTicksTimeScale / mediaTimeUnitTicks),
				 (long) (mediaDurationTicks / mediaTimeUnitTicks),
				 (long) (videoTrackStartTicks / movieTimeUnitTicks));
	}

	/* Make sure we found a suitable video track */
	if (!foundVideo)
	{
		fprintf(OUTFILE, "### Movie contains no suitable uncompressed video tracks (type \"raw \")\n");
		goto Recover;
	}

	/* Make sure the frame size of the QuickTime movie is at least as
	 * big as the requested output frame size.
	 */
	if (videoTrackHeight < gFrameHeight || videoTrackWidth < gFrameWidth)
	{
		fprintf(OUTFILE, "### Movie dimensions (%ld x %ld) are smaller than EZFlix frame dimensions (%ld x %ld)\n",
				(long)videoTrackWidth, (long)videoTrackHeight, (long)gFrameWidth, (long)gFrameHeight);
		goto Recover;
	}

	/* Determine the frame rate for the EZFlix output */

	if (gFrameRate == 0) 
	{
		/* Don't have a frame rate for the output chunk file yet.
		 * Use the input QuickTime movie's frame rate, if available,
		 * or the default frame rate.
		 */
		if (movieFrameRate != 0)
			gFrameRate = movieFrameRate;
		else
			gFrameRate = kDefaultFrameRate;
	}

	/* Now that we have a frame rate for the EZFlix output, compute
	 * the number of ticks per frame.  The duration of the EZFlix
	 * output is the same as the duration of the selected video track
	 * of the input movie (in ticks).
	 */
	ezFlixFrameTicks = TICKS_PER_TIME_UNIT(gFrameRate);
	ezFlixDurationTicks = GetTrackDuration(videoTrack) * movieTimeUnitTicks;

	/* Build the EZFlix Header chunk */
	
	ezFlixHeader.chunkType 		= EZFLIX_CHUNK_TYPE;
	ezFlixHeader.chunkSize 		= sizeof(EZFlixHeader);
	ezFlixHeader.subChunkType 	= EZFLIX_HDR_CHUNK_SUBTYPE;
	ezFlixHeader.time			= 0;
	ezFlixHeader.channel		= gVideoChannel;
	ezFlixHeader.cType			= (**IDesc).cType;
	ezFlixHeader.height			= gFrameHeight;
	ezFlixHeader.width			= gFrameWidth;
	ezFlixHeader.scale			= kApprox3DOAudioTimeScale;
	ezFlixHeader.count			= ezFlixDurationTicks / ezFlixFrameTicks;	/* count of frames */
	ezFlixHeader.version		= kEZFlixFormatVersion;

	/* Get decompression parameters from the EZFlix decoder and copy
	 * them into the header chunk.
	 */
	GetEZFlixEncoderParams(&params);
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
	ezFlixHeader.unused			= 0;
	ezFlixHeader.codecVersion	= params.version;
	
	if (! gOpera)
	{
		ezFlixM2Header.chunkType 				= EZFLIX_CHUNK_TYPE;
		ezFlixM2Header.chunkSize 				= sizeof(EZFlixM2Header);
		ezFlixM2Header.subChunkType 			= EZFLIX_HDR_CHUNK_SUBTYPE;
		ezFlixM2Header.time						= 0;
		ezFlixM2Header.channel					= gVideoChannel;
		ezFlixM2Header.version					= kEZFlixM2FormatVersion;
		ezFlixM2Header.cType					= (*IDesc)->cType;
		ezFlixM2Header.height					= gFrameHeight;
		ezFlixM2Header.width					= gFrameWidth;
		ezFlixM2Header.scale					= kApprox3DOAudioTimeScale;
		ezFlixM2Header.count					= ezFlixDurationTicks / ezFlixFrameTicks;
		
		ezFlixM2Header.codecVersion	= params.version;
		ezFlixM2Header.min			= params.min;
		ezFlixM2Header.max			= params.max;
		ezFlixM2Header.invgamma		= params.invgamma;
		ezFlixM2Header.unused = 0;
	}
	/* Initialize fields of film frame chunk that won't change */

	ezFlixFrame.chunkType 		= EZFLIX_CHUNK_TYPE;
	ezFlixFrame.subChunkType	= kEZFlixFrameType;
	ezFlixFrame.channel			= gVideoChannel;

	/* Print out information about the source and destination */
	
	fprintf(OUTFILE, "\tCompressing %d X %d (%d fps)", videoTrackWidth, videoTrackHeight, gFrameRate);
	if ((videoTrackWidth != ezFlixHeader.width) || (videoTrackHeight != ezFlixHeader.height))
		fprintf(OUTFILE, " as %d X %d (%d fps)\n", ezFlixHeader.width, ezFlixHeader.height, gFrameRate);
	else
		fprintf(OUTFILE, " image\n", ezFlixHeader.width, ezFlixHeader.height);
	VFPRINTF(OUTFILE, "\tFrame Rate = %d, Channel = %d\n", gFrameRate, gVideoChannel);
	fprintf(OUTFILE, "\tDuration is %ld frames;%5.1f secs\n",
			(long)(ezFlixDurationTicks / ezFlixFrameTicks), ezFlixDurationTicks / kTicksTimeScale);
	fflush(OUTFILE);
	SpinCursor(32);

	/* Write the EZFlix header chunk */

	if (gOpera)
		Oops = WriteData(ezFlixFileRef, sizeof(EZFlixHeader), &ezFlixHeader);
	else
		Oops = WriteData(ezFlixFileRef, sizeof(EZFlixM2Header), &ezFlixM2Header);

	if (Oops != noErr)
	{
		fprintf(OUTFILE, "### Cannot write EZFlix header chunk; error = %d\n", Oops);
		goto Recover;
	}

	/* Allocate a buffer for the EZFlix output chunk and a handle
	 * for the video sample data from the QuickTime movie.
	 */
	movieSampleData = NewHandle(0);
	if ((ezFlixData = malloc(kChunkBufferSize)) == NULL)
		{
		fprintf(OUTFILE, "### Out of memory; cannot allocate EZFlix chunk buffer\n");
		Oops = memFullErr;
		goto Recover;
		}

	/* START OF MAIN OUTPUT LOOP */
	/* Outer loop iterates once per second */

	outputTicks = 0.0;

	while (outputTicks < ezFlixDurationTicks)
	{
		videoBytesPerSecond = 0;

		/* Compute time of next one-second boundary, or end of sample data */
		secondTime = MIN(outputTicks + kTicksTimeScale, ezFlixDurationTicks); 

		/* What second's-worth of data are we about to output */
		second = (outputTicks + ezFlixFrameTicks - 1 + (kTicksTimeScale / 2)) / kTicksTimeScale;
		if (!gVerbose)
			fprintf(OUTFILE, "\tSecond #%d ", second);

		/* Inner loop iterates over the samples inside one second */

		for ( ;outputTicks < secondTime; outputTicks += ezFlixFrameTicks)
		{
			SpinCursor(32);
			if (!gVerbose) fprintf(OUTFILE, ".");
			
			/* Don't start emiting EZFlix data until the time corresponding
			 * to the start of the QuickTime the track has arrived.
			 */
			if (outputTicks < videoTrackStartTicks)
				break;

			/* Current time within the QuickTime movie's track converted
			 * to the approprate time in the time scale of the track's media.
			 */
			mediaTime = TrackTimeToMediaTime((outputTicks - videoTrackStartTicks) / movieTimeUnitTicks,
											 videoTrack);

			VFPRINTF(OUTFILE, "\n\tFrame %ld: Movie time = %ld, Media time = %ld, ",
				(long)(outputTicks / ezFlixFrameTicks), (long)(outputTicks / movieTimeUnitTicks),
				(long)mediaTime);

			/* Fetch the QuickTime video sample from the media at the media time
			 * corresponding to the current output time.
			 */
			Oops = GetMediaSample(videoTrackMedia, movieSampleData, 0, &size,
				mediaTime, &sampleTime, &durationPerSample, 0, 0, 1, 0, &sampleFlags);
			if (Oops != noErr) 
			{
				fprintf(OUTFILE, "### Cannot read sample from QuickTime file; error = %d\n", Oops);
				goto Recover;
			}

			/* Compress the video sample into the EZFlix data chunk */

			HLock(movieSampleData);
			size = CompressEZFlixImage(videoTrackWidth, videoTrackHeight, videoTrackDepth, *movieSampleData,
									   gFrameWidth, gFrameHeight, (char *)ezFlixData);
			HUnlock(movieSampleData);
			if (size <= 0)
			{
				fprintf(OUTFILE, "### EZFlix compressor returned error\n");
				goto Recover;
			}

			/* Compressed another frame */
			*frameCount += 1;

			VFPRINTF(OUTFILE, "Sample time = %ld, Duration = %ld, Size = %ld\n",
					 (long)sampleTime, (long)durationPerSample, (long)size);
			
			/* Fill in the variable fields of the EZFlix data chunk */

			
			nextFrameTime			= (outputTicks + ezFlixFrameTicks) / kTicksPer3DOAudio;
			ezFlixFrame.time		= outputTicks / kTicksPer3DOAudio + 1;
			ezFlixFrame.duration	= nextFrameTime - ezFlixFrame.time;

			if (gOpera)
			{
				/* To make an Opera frame we must strip off the Flix Frame Header */
				/*	that the QT component puts on */
				FlixFrameHeader* qtFlixData = (FlixFrameHeader*) ezFlixData;
				ezFlixFrame.frameSize	= size - (qtFlixData->chunk1Offset<<2);
				ezFlixFrame.chunkSize = offsetof(EZFlixFrame, frameData) + ezFlixFrame.frameSize;
			}
			else
			{
				/* To make an M2 Flix frame we just strip the height and width */
				ezFlixFrame.frameSize	= size;
				ezFlixFrame.chunkSize = offsetof(EZFlixM2Frame, frameData) + ezFlixFrame.frameSize;
			}

			VFPRINTF(OUTFILE, "\t\tChunk Time = %d, Duration = %d, Frame Size = %d, Chunk Size = %d\n",
					 ezFlixFrame.time, ezFlixFrame.duration, ezFlixFrame.frameSize, ezFlixFrame.chunkSize);
			fflush(OUTFILE);	
			
			/* Write out the EZFlix data chunk header */
				
			Oops = WriteData(ezFlixFileRef, offsetof(EZFlixFrame, frameData), &ezFlixFrame);
			if (Oops != noErr)
			{
				fprintf(OUTFILE, "### Cannot write EZFlix data chunk header; error = %d\n", Oops);
				goto Recover;
			}
			
			/* Write out the EZFlix data chunk data */
				
			if (gOpera)
			{
				FlixFrameHeader* qtFlixData = (FlixFrameHeader*) ezFlixData;
				long writeSize;
				
				/* The compressor gives us data in M2 format, with a header already on it. */
				/* Rearrange the order of the U, V, and Y portions of the frame */
				writeSize = ezFlixFrame.frameSize - (long)(qtFlixData->chunk3Offset<<2)
								+ (long)(qtFlixData->chunk1Offset<<2); 		/* amount of Y data */
				Oops = WriteData(ezFlixFileRef, writeSize, (char*)qtFlixData 
									+ (long)(qtFlixData->chunk3Offset<<2));
				if (Oops != noErr)
				{
					fprintf(OUTFILE, "### Cannot write EZFlix data chunk; error = %d\n", Oops);
					goto Recover;
				}
				writeSize = ezFlixFrame.frameSize - writeSize; /* remaining amount to write */
				Oops = WriteData(ezFlixFileRef, writeSize, (long*)qtFlixData + (qtFlixData->chunk1Offset));
				if (Oops != noErr)
				{
					fprintf(OUTFILE, "### Cannot write EZFlix data chunk; error = %d\n", Oops);
					goto Recover;
				}
			}
			else /* M2 EZFlix frame output */
			{
				FlixFrameHeader* qtFlixData = (FlixFrameHeader*) ezFlixData;
				long writeSize;
				
				/* write up to the width parameter */
				writeSize = ezFlixFrame.frameSize;
				Oops = WriteData(ezFlixFileRef, writeSize, qtFlixData);
				if (Oops != noErr)
				{
					fprintf(OUTFILE, "### Cannot write EZFlix data chunk; error = %d\n", Oops);
					goto Recover;
				}
			}

			videoBytesPerSecond += ezFlixFrame.chunkSize;

		} /* End of one-second output loop */

		/* Another second of video has been converted */

		VFPRINTF(OUTFILE, "\n\tSecond #%d", second);
		fprintf(OUTFILE, ": %d bytes\n", videoBytesPerSecond);

	} /* End of main output loop */

Recover:
	CloseEZFlixEncoder();	
	if (ezFlixFileRef != 0)
		TruncateAndClose(ezFlixFileRef);
	if (IDesc != NULL)
		DisposeHandle((Handle)IDesc);
	if (movieSampleData != NULL)
		DisposHandle(movieSampleData);
	if (ezFlixData != NULL)
		free(ezFlixData);

Return:
	return Oops;
}



/* Print usage of command line options and flags.
             [-c channel number {0}]
             [-f frame rate (fps) {15}]
             [-h frame height {192}]
             [-i <inputFile>]
             [-o <outputFile>]
             [-opera]
             [-q compression quality {25}]
             [-v]
             [-w frame width {256}]
 */
static void Usage(char *toolName)
	{
	fprintf(OUTFILE,
		"# Version %s\n", kCurrentReleaseVersion);
	fprintf(OUTFILE, 
		"# Usage - %s\n", toolName );
	fprintf(OUTFILE,
		"             [-%c channel number {%d}]\n", kVideoChannelFlag, (int) kDefaultVideoChannel);
	fprintf(OUTFILE,
		"             [-%c frame rate (fps) {%d}]\n", kFrameRateFlag, (int) kDefaultFrameRate);	
	fprintf(OUTFILE,
		"             [-%c frame height {%d}]\n", kFrameHeightFlag, (int) kDefaultFrameHeight);	
	fprintf(OUTFILE,
		"             [-%c <inputFile>]\n", kInputFileFlag);
	fprintf(OUTFILE,
		"             [-%s]  generate Opera-compatible stream\n", kOperaFlag);	
	fprintf(OUTFILE,
		"             [-%c <outputFile>]\n", kOutputFileFlag);
	fprintf(OUTFILE,
		"             [-%c compression quality {%d}]\n", kQualityFlag, (int) kDefaultQuality);	
	fprintf(OUTFILE,
		"             [-%c]  verbose\n", kVerboseFlag);	
	fprintf(OUTFILE,
		"             [-%c frame width {%d}]\n", kFrameWidthFlag, (int) kDefaultFrameWidth);	
	}

/* CloseMovie - Call DisposeMovie(gMovie) and ExitMovies(); */

void CloseMovie(void)
{
	/* Pitch the movie from memory */
	if (gMovie != NULL)
	{
		DisposeMovie(gMovie);
		gMovie = NULL;
	}
	/* Take down QuickTime */
	ExitMovies();
}	


/*
 * EZFlixChunkifier
 * Convert an uncompressed QuickTime movie to an EZFlix chunk file.
 */
int main (int argc, char *argv[])
{
	OSErr			error;					/* local error result */
	long			parms;					/* index of next argument on command line */
	char			*movieFileName;			/* path name of the input QuickTime movie file */
	Str255			ezFlixFileName;			/* path name of the output EZFlix chunk file */
	char			*pEnd;					/* dummy argument passed to strtol() */
	long			frameCount;				/* count of frames compressed */
	long			gestaltResponse;		/* argument return from Gestalt Manager */
	unsigned long	elapsedTime;			/* 60Hz clock ticks elapsed during run of this tool */
	unsigned long	minutes;				/* minutes elapsed */
	unsigned long	seconds;				/* seconds elapsed */

	gMovie = NULL;
	error = noErr;
	movieFileName = NULL;
	*ezFlixFileName = '\0';

	/* So we can spin cursor */
	InitCursorCtl(NULL);

	/* Need an A5 world to use Quicktime */
	InitGraf((Ptr) &qd.thePort);			/* Need an a5 world to use Quicktime */

	/* Donn & Daisy - updated setfound to fesetround - checks for bogus rounding direction */
	/* Set up rounding for floating point calculations */
#ifdef powerc
	if (fesetround(FE_TOWARDZERO) == 0)
	{
		fprintf(OUTFILE, "### Internal error: Invalid rounding direction \n");
		goto UsageExit;
	}
#else
	setround(TOWARDZERO);
#endif

	/* Parse the command-line */

	for (parms = 1; parms < argc; ++parms) 
		{
		/* Looking for flag options */

		if (*argv[parms] != '-') 
			{
			fprintf(OUTFILE, "### Invalid argument \"%s\"\n", argv[parms]);
			goto UsageExit;
			} 

		/* All flag options have the format -c, where
		 * c is a single-character identifier, except the "Opera" flag.
		 */
		if (strlen(argv[parms]) != 2 && strlen(argv[parms]) != 6)
			goto BadOptionExit;

		/* -Opera ? */
		if (strlen(argv[parms]) == 6)
			{
				if ( tolower(argv[parms][1]) == 'o' &&
					 tolower(argv[parms][2]) == 'p' &&
					 tolower(argv[parms][3]) == 'e' &&
					 tolower(argv[parms][4]) == 'r' &&
					 tolower(argv[parms][5]) == 'a' )
					 {
						gOpera = true;
						continue;
					}
				else goto BadOptionExit;
			}
			
			
		/* Option syntax is valid, now see if it's a known option */

		switch (tolower(*(argv[parms]+1)))
			{

			case kVideoChannelFlag:
				/* Set the logical channel number for all EZFlix chunks written
				 * to the output chunk file.
				 */
				if (++parms == argc || *argv[parms] == '-')
					{
					fprintf(OUTFILE,"### Channel number option requires an argument.\n");
					goto UsageExit;
					}
				gVideoChannel = strtol(argv[parms], &pEnd, 10);
				if (pEnd[0] != 0) 
					{
					fprintf(stderr,"### %s - invalid channel number: %s\n", argv[0], argv[parms]);
					goto UsageExit;
					}

				if (gVideoChannel < kMinVideoChannel || gVideoChannel > kMaxVideoChannel)
					{
					fprintf(OUTFILE,"### Channel number %ld out of range (%ld..%ld)\n",
							gVideoChannel, (long)kMinVideoChannel, (long)kMaxVideoChannel);
					goto UsageExit;
					}
				break;

			case kInputFileFlag:
				/* Set the path name of the QuickTime movie input file.
				 */
				if (++parms == argc || *argv[parms] == '\0' || *argv[parms] == '-')
					{
					fprintf(OUTFILE,"### Input file option requires an argument.\n");
					goto UsageExit;
					}
				movieFileName = argv[parms];
				break;

			case kOutputFileFlag:
				/* Set the path name of the file into which the output
				 * chunk files will be written.
				 */
				if (++parms == argc || *argv[parms] == '\0' || *argv[parms] == '-')
					{
					fprintf(OUTFILE,"### Output file option requires an argument.\n");
					goto UsageExit;
					}
				strcpy((char*)ezFlixFileName, argv[parms]);
				break;

			case kFrameRateFlag:
				/* Set the frame rate of the EZFlix data.
				 */
				if (++parms == argc || *argv[parms] == '-')
					{
					fprintf(OUTFILE,"### Frame rate option requires an argument.\n");
					goto UsageExit;
					}
				gFrameRate = strtol(argv[parms], &pEnd, 10);
				if (pEnd[0] != 0) 
					{
					fprintf(stderr,"### %s - invalid frame rate: %s\n", argv[0], argv[parms]);
					goto UsageExit;
					}

				if (gFrameRate < kMinFrameRate || gFrameRate > kMaxFrameRate)
					{
					fprintf(OUTFILE,"### Frame rate %ld out of range (%ld..%ld)\n",
							gFrameRate, (long)kMinFrameRate, (long)kMaxFrameRate);
					goto UsageExit;
					}
				break;

			case kVerboseFlag:
				/* Turn on verbosity.
				 */
				gVerbose = true;
				break;

			case kQualityFlag:
				/* set the quality level.
				 */
				 {
				 	long quality;
					
					if (++parms == argc || *argv[parms] == '-')
						{
						fprintf(OUTFILE,"### Quality setting requires an argument.\n");
						goto UsageExit;
						}
					quality = strtol(argv[parms], &pEnd, 10);
					if (pEnd[0] != 0) 
						{
						fprintf(stderr,"### %s - invalid quality setting: %s\n", argv[0], argv[parms]);
						goto UsageExit;
						}
					
					switch (quality)
						{
							case kQualitySetting1:
								gQuality = kHuffman3Quality;
								break;
							case kQualitySetting2:
								gQuality = kFixedTwoBitQuality;
								break;
							case kQualitySetting3:
								gQuality = kHuffman4Quality;
								break;
							case kQualitySetting4:
								gQuality = kHuffman5Quality;
								break;
							case kQualitySetting5:
								gQuality = kHuffman6Quality;
								break;
							default:
								fprintf(stderr,"### %s - invalid quality setting.  Use %d, %d, %d, %d, or %d.\n", argv[0],
									kQualitySetting1, kQualitySetting2, kQualitySetting3, kQualitySetting4, kQualitySetting5);
								return EXIT_FAILURE;
						}
				}
				break;

			case kFrameHeightFlag:
				/* Set the pixel height of the frames of EZFlix data.
				 */
				if (++parms == argc || *argv[parms] == '-')
					{
					fprintf(OUTFILE,"### Frame height option requires an argument.\n");
					goto UsageExit;
					}
				gFrameHeight = strtol(argv[parms], &pEnd, 10);
				if (pEnd[0] != 0) 
					{
					fprintf(stderr,"### %s - invalid height value: %s\n", argv[0], argv[parms]);
					goto UsageExit;
					}

				if (gFrameHeight < kMinFrameHeight || gFrameHeight > kMaxFrameHeight)
					{
					fprintf(OUTFILE,"### Frame height %ld out of range (%ld..%ld)\n",
							gFrameHeight, (long)kMinFrameHeight, (long)kMaxFrameHeight);
					goto UsageExit;
					}
				break;

			case kFrameWidthFlag:
				/* Set the pixel width of the frames of EZFlix data.
				 */
				if (++parms == argc || *argv[parms] == '-')
					{
					fprintf(OUTFILE,"### Frame width option requires an argument.\n");
					goto UsageExit;
					}
				gFrameWidth = strtol(argv[parms], &pEnd, 10);
				if (pEnd[0] != 0) 
					{
					fprintf(stderr,"### %s - invalid width value: %s\n", argv[0], argv[parms]);
					goto UsageExit;
					}

				if (gFrameWidth < kMinFrameWidth || gFrameWidth > kMaxFrameWidth)
					{
					fprintf(OUTFILE,"### Frame width %ld out of range (%ld..%ld)\n",
							gFrameWidth, (long)kMinFrameWidth, (long)kMaxFrameWidth);
					goto UsageExit;
					}
				break;

			default:
				goto BadOptionExit;
			}
		}

	/* If Opera, enforce 25% */
	if (gOpera && gQuality != kFixedTwoBitQuality)
		{
		fprintf(OUTFILE,"### Opera only supports a quality level of 25.\n");
		goto UsageExit;
		}
	
	/* Make sure there is an input file */

	if (movieFileName == NULL) 
		{
		fprintf(OUTFILE,"### No input movie file.\n");
		goto UsageExit;
		}

	/* If an explicit output file path name has not been
	 * provided, we'll build one by appending a suffix to
	 * the name of the input file.
	 */
	if (*ezFlixFileName == '\0')
		{
		strcpy((char*)ezFlixFileName, movieFileName);
		strcat((char*)ezFlixFileName, kOutputFileSuffix);
		}

	/* Input and output file names have to be in Pascal-string format */
	c2pstr(movieFileName);
	c2pstr((char*)ezFlixFileName);

	/* Make sure QuickTime is available */

	if (Gestalt(gestaltQuickTime, &gestaltResponse) != noErr)
		{
		fprintf(OUTFILE,"### QuickTime extension is not installed.\n");
		goto Return;
		}

	/* Initialize QuickTime */
	EnterMovies();

	/* Load the movie resource from the movie file */
	error = OpenMovie((unsigned char*)movieFileName, &gMovie);
	if (error < noErr)
		{
		fprintf(OUTFILE, "### Cannot open movie \"%P\"; error = %d\n\n", movieFileName, error);
		goto Recover;
		}
	
	elapsedTime = -clock();
	error = DumpMovie(gMovie, ezFlixFileName, &frameCount);
	elapsedTime += clock();

	/* Print the real time it took to compress the input file */

	fprintf(OUTFILE, "\tCompressed %ld frames in ", frameCount);
	seconds = elapsedTime / 60;
	minutes = seconds / 60;
	seconds -= minutes * 60;
	if (minutes > 0)
		fprintf(OUTFILE, "%ld mins ", minutes);
	fprintf(OUTFILE, "%ld secs\n\n", seconds);

Recover:
	CloseMovie();	/* DisposeMovie(gMovie); etc */
Return:
	return EXIT_SUCCESS;

UsageExit:
	Usage(argv[0]);
	return EXIT_FAILURE;

BadOptionExit:
	fprintf(OUTFILE,"### \"%s\" is not an option.\n", argv[parms]);
	goto UsageExit;
	}

