#ifndef __AUDIO_FLEX_STREAM_H
#define __AUDIO_FLEX_STREAM_H


/****************************************************************************
**
**  @(#) flex_stream.h 95/10/23 1.8
**
**  Flexible stream I/O
**
****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __FILE_FILEIO_H
#include <file/fileio.h>
#endif


typedef struct FlexStream
{
	RawFile *flxs_FileStream;
/* The following fields are used for parsing from an in memory image. */
	char   *flxs_Image;            /* Image in memory. */
	int32   flxs_Cursor;           /* Position in image. */
	int32   flxs_Size;             /* Size of image. */
} FlexStream;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int32 CloseFlexStreamFile( FlexStream *flxs );
int32 CloseFlexStreamImage( FlexStream *flxs );
int32 OpenFlexStreamFile( FlexStream *flxs, const char *filename );
int32 OpenFlexStreamImage( FlexStream *flxs, char *Image, int32 NumBytes );
int32 ReadFlexStream( FlexStream *flxs, char *Addr, int32 NumBytes );
int32 SeekFlexStream( FlexStream *flxs, int32 Offset, FileSeekModes Mode );
int32 TellFlexStream( const FlexStream *flxs );
char *TellFlexStreamAddress( const FlexStream *flxs );
char *LoadFileImage( const char *Name, int32 *NumBytesPtr );

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __AUDIO_FLEX_STREAM_H */
