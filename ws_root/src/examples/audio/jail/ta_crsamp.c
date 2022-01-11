/***************************************************************
**
** Test CreateSample
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

#include <audio.h>
#include <filestreamfunctions.h>
#include <mem.h>
#include <operror.h>
#include <stdio.h>

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError (NULL, name, NULL, Result); \
		goto cleanup; \
	}


Err DemoSampleFromFile (char *FileName);
Err DemoSampleFromMemory (char *FileName);
char *loadfileimage (char *Name, int32 *NumBytesPtr);

int main(int argc, char *argv[])
{
    /* !!! const removed because of diab compiler bug */
	char * /* !!! const */ FileName = argc > 1 ? argv[1] : "$samples/Unpitched/Cowbell/Cowbell1.M44k.aiff";

/* Initialize audio, return if error. */
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}

	DemoSampleFromFile (FileName);
	DemoSampleFromMemory (FileName);

	CloseAudioFolio();
	return 0;
}


/*
    Use CreateSample() to load a sample from a file.
*/
Err DemoSampleFromFile (char *FileName)
{
    int32 Result = 0;
    Item SampleItem = 0;

    PRT(("Load file = %s\n", FileName ));

    SampleItem = CreateSampleVA (AF_TAG_NAME, FileName,
                                 TAG_END);

    CHECKRESULT(SampleItem,"CreateSample");
    Result = DebugSample( SampleItem );
    CHECKRESULT(Result,"DebugSample");

cleanup:
    UnloadSample( SampleItem );
    return Result;
}


/*
    Use CreateSample() to create a sample from an AIFF file image in memory.
*/
Err DemoSampleFromMemory (char *FileName)
{
    int32 Result = 0;
    uint8 *Image = NULL;
    int32 NumBytes;
    Item SampleItem = 0;

    PRT(("Load image.\n"));

    Image = loadfileimage( FileName, &NumBytes );
    if( Image == NULL ) return -1;

    SampleItem = CreateSampleVA (AF_TAG_IMAGE_ADDRESS, Image,
                                 AF_TAG_IMAGE_LENGTH,  NumBytes,
                                 TAG_END);

    CHECKRESULT(SampleItem,"CreateSample");
    Result = DebugSample( SampleItem );
    CHECKRESULT(Result,"DebugSample");

cleanup:
    UnloadSample( SampleItem );
    free( Image );
    return Result;
}

char *loadfileimage( char *Name, int32 *NumBytesPtr )
{
	Stream *str;
	char *Image;
	int32 NumBytes;
	int32 Result;

	str = OpenDiskStream (Name, 0);
	if (str == NULL)
	{
		ERR(("Could not open file = %s\n", Name));
		return NULL;
	}

	NumBytes = str->st_FileLength;

	Image = (char *) malloc(NumBytes);
	if( Image == NULL )
	{
		ERR(("Insufficient memory.\n"));
		return NULL;
	}

	Result = ReadDiskStream( str, Image, NumBytes );
	if( Result != NumBytes)
	{
		ERR(("Could not read file.\n"));
		free(Image);
		return NULL;
	}

	CloseDiskStream( str );

	*NumBytesPtr = NumBytes;
	return Image;
}
