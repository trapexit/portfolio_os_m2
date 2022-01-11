/****
 *
 *	@(#) os.h 95/03/18 1.15
 *	Copyright 1994, The 3DO Company
 *
 * OS types and constants for K93D
 *
 ****/
#ifndef _K9OS
#define _K9OS

#ifdef __cplusplus
extern "C" {
#endif

#define GFX_SeekSet 0
#define GFX_SeekCur 1
#define GFX_SeekEnd 2

#define Stream_Read 0
#define Stream_Write 1

typedef struct {
	void	*stream;
	bool	gotChar;
	int32	savedChar;
} ByteStream;

ByteStream *K9_OpenByteStream(char *fileName, int32 mode, int32 bSize);
int32 K9_SeekByteStream(ByteStream *theStream, int32 offset, int32 whence);
void K9_CloseByteStream(ByteStream *theStream);
int32 K9_ReadByteStream(ByteStream *theStream, char *buffer, int32 nBytes);
int32 K9_WriteByteStream(ByteStream *theStream, char *buffer, int32 nBytes);

void K9_ExpandFileName(char *name, char *expname);

int32 K9_GetChar(ByteStream *theStream);

void K9_UngetChar(ByteStream *theStream, int32 c);

bool K9_EndOfStream(ByteStream *theStream);

int32 K9_GetByteStreamLength(ByteStream *theStream);

bool K9_ErrorOnStream(ByteStream *theStream);

#ifdef __cplusplus
}
#endif
#endif /* K9OS */
