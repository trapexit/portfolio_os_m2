#ifndef __AUDIO_IFF_FS_TOOLS_H
#define __AUDIO_IFF_FS_TOOLS_H

/****************************************************************************
**
**  @(#) iff_fs_tools.h 95/11/22 1.13
**  $Id: iff_fs_tools.h,v 1.17 1994/09/10 00:17:48 peabody Exp $
**
**  Include file for simple IFF reader using 3DO FIle System
**
****************************************************************************/

#ifdef EXTERNAL_RELEASE        /* { */

#error !!! <audio/iff_fs_tools.h> is no longer supported.

#else

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __AUDIO_FLEX_STREAM_H
#include <audio/flex_stream.h>
#endif


typedef struct
{
	FlexStream iffc_FlexStream;  /* For reading file or memory. */

	int32	iffc_length;
	int32   iffc_NextPos;          /* Position at end of next chunk */
	int32 (*iffc_ChunkHandler)();  /* Callback function for Parser */
	int32 (*iffc_FormHandler)();   /* Callback function for Parser */
	void   *iffc_UserContext;
	char   *iffc_LastChanceDir;    /* Directory to look in if not found. */
	int32   iffc_Level;            /* Level of recursion in file. */
} iff_control;

#if 0
#define IFFWRITE(buf,len) \
	Result = fwrite ((char *) buf, 1, len, iffc->iffc_FileStream); \
	iffc->iffc_length += len
#endif

#define EVENUP(n) { if (n & 1) n++; }

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* function prototypes  */
int32 iffScanChunks ( iff_control *iffc, uint32 Length );
int32 iffParseFile (iff_control *iffc, const char *FilePathName );
int32 iffParseImage ( iff_control *iffc, char *Image, int32 NumBytes );
int32 iffParseChunk ( iff_control *iffc );
int32 iffOpenFileRead (iff_control *iffc, const char *filename);
int32 iffCloseFile (iff_control *iffc);

int32 iffSkipChunkData (iff_control *iffc, uint32 numbytes);
int32 iffReadChunkData (iff_control *iffc,
			void *data, uint32 numbytes);
int32 iffReadChunkHeader (iff_control *iffc,
			uint32 *type, uint32 *size);
int32 iffReadForm (iff_control *iffc, uint32 *type);
int32 iffReadStream( iff_control *iffc, char *Addr, int32 NumBytes );
int32 iffSeekStream( iff_control *iffc, int32 Offset, FileSeekModes Mode );

int32 iffCloseImage (iff_control *iffc);
int32 iffOpenImage (iff_control *iffc, char *Image, int32 NumBytes );

#if 0
int32 iffOpenFileWrite (iff_control *iffc, const char *filename);
int32 iffBeginForm (iff_control *iffc, uint32 type);
int32 iffEndForm (iff_control *iffc);
int32 iffWriteChunk (iff_control *iffc, uint32 type,
		void *data, int32 numbytes);
#endif

/*
 * Universal IFF identifiers.
 */
#define	ID_FORM			MAKE_ID('F','O','R','M')
#define	ID_XREF			MAKE_ID('X','R','E','F')

/* Old obsolete names. */
#if 0
int32 iff_scan_chunks ( iff_control *iffc, uint32 Length );
int32 iff_parse_file (iff_control *iffc, char *FilePathName );
int32 iff_parse_chunk ( iff_control *iffc );
int32 iff_open_file_read (iff_control *iffc, char *filename);
int32 iff_open_file_write (iff_control *iffc, char *filename);
int32 iff_close_file (iff_control *iffc);
int32 iff_begin_form (iff_control *iffc, uint32 type);
int32 iff_end_form (iff_control *iffc);
int32 iff_write_chunk (iff_control *iffc, uint32 type,
		void *data, int32 numbytes);

int32 iff_skip_chunk_data (iff_control *iffc, uint32 numbytes);
int32 iff_read_chunk_data (iff_control *iffc,
			void *data, uint32 numbytes);
int32 iff_read_chunk_header (iff_control *iffc,
			uint32 *type, uint32 *size);
int32 iff_read_form (iff_control *iffc, uint32 *type);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* } EXTERNAL_RELEASE */

/*****************************************************************************/


#endif /* __AUDIO_IFF_FS_TOOLS_H */
