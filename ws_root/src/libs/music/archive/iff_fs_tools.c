/* @(#) iff_fs_tools.c 96/01/02 1.11 */
/* $Id: iff_fs_tools.c,v 1.33 1994/09/13 20:34:23 peabody Exp $ */
/* Simple IFF tools for reading and writing files.
** using the 3DO File System.
**
** Copyright: Phil Burk 6/10/92
**
** 921207 PLB Proper error check for OpenDiskStream
** 930208 PLB Fix write form size.
** 930319 PLB Return proper error from iff_parse_file
** 930609 PLB Use LastChanceDir scheme if file open fails.
** 930708 PLB Fixed naming inconsistencies.
** 9308?? PLB Use FlexStream for RAM image parsing.
** 930820 PLB Fix nesting of iffc_Length when handling nested FORMs
** 930824 PLB Always seek to end of Chunk or Form after custom handler functions.
** 930407 PLB Made ERR() a comment for PRODUCTION code.
** 940721 WJB Fixed strncpy() usage in iffParseFile() that could result in an unterminated string.
** 940726 WJB Added a triple bang or two.
** 940727 WJB Added const to filenames.
**            Made GetSuffix() and SplitFileName() private.
** 950823 PLB Removed file search path code. Big, slow and unnecessary.
**            Removed XREF support because it conflicted with ROM/CD.
*/

#include <audio/flex_stream.h>
#include <audio/iff_fs_tools.h>
#include <audio/musicerror.h>
#include <file/filefunctions.h>
#include <stdio.h>

#define PRT(x)  { printf x; }
#ifdef BUILD_STRINGS
	#define ERR(x)  PRT(x)
#else
	#define ERR(x)  /* PRT(x) */
#endif

#define DBUG(x)	/* PRT(x) */

#define CHECKERR(msg) \
	if (Result < 0) \
	{ \
		ERR(msg); \
		goto error; \
	}

#define MAX_PATH_LEN (256)

/******************************************************/
int32 iffScanChunks ( iff_control *iffc, uint32 Length )
{
	int32 Result=0;
	uint32 OldLength;

DBUG(("iffScanChunks: Length = %d\n", Length));
/* adjust Length for this FORM */
	OldLength = iffc->iffc_length;
	iffc->iffc_length = Length;

/* Set NextPos to current position */
	iffc->iffc_NextPos = iffSeekStream( iffc, 0, FILESEEK_CURRENT);

DBUG(("iffScanChunks: CurPos = %d\n", iffc->iffc_NextPos));

/* Nest into iffc */
	while(iffc->iffc_length > 0)
	{
		Result = iffParseChunk ( iffc );
		CHECKERR(("iffScanChunks: error = 0x%x\n", Result));
	}

	iffc->iffc_length = OldLength - Length;

error:
	return Result;
}

/******************************************************/
int32 iffParseChunk ( iff_control *iffc )
{
	uint32 ChunkType;
	uint32 FormType;
	uint32 ChunkSize;
	uint32 FormSize;
	int32  Result;
	int32  OriginalLength;
	int32  NextPos, CurPos;

	Result = iffReadChunkHeader(iffc, &ChunkType, &ChunkSize);
	CHECKERR(("iffParseChunk: Error reading CHUNK header = 0x%x\n", Result));
	switch(ChunkType)
	{
		case ID_FORM:
DBUG(("iffParseChunk: length before FORM = %d\n", iffc->iffc_length));

			Result = iffReadChunkData(iffc, &FormType, 4);
			CHECKERR(("iffParseChunk: Error reading FORM type = 0x%x\n", Result));

			iffc->iffc_Level += 1;
			OriginalLength = iffc->iffc_length;
			EVENUP(ChunkSize);
			FormSize = ChunkSize-4;
			iffc->iffc_length = FormSize;

			NextPos = iffSeekStream( iffc, 0, FILESEEK_CURRENT) + FormSize;

			Result = (*(iffc->iffc_FormHandler))( iffc, FormType, FormSize );
			CHECKERR(("iffParseChunk: Error in user FormHandler = 0x%x\n", Result));

			CurPos = iffSeekStream( iffc, 0, FILESEEK_CURRENT);
			if(NextPos != CurPos)
			{
DBUG(("iffParseChunk: Not at end of FORM so skip!\n"));
				Result = iffSkipChunkData( iffc, NextPos - CurPos );
				CHECKERR(("iffParseChunk: error skipping chunk = 0x%x\n", Result));
			}

			iffc->iffc_length = OriginalLength - (FormSize);
			iffc->iffc_Level -= 1;

DBUG(("iffParseChunk: length after FORM = %d\n", iffc->iffc_length));
			break;

		default:
			NextPos = iffSeekStream( iffc, 0, FILESEEK_CURRENT) + ChunkSize;
			EVENUP(NextPos);
			Result = (*(iffc->iffc_ChunkHandler))( iffc, ChunkType, ChunkSize);
			CHECKERR(("iffParseChunk: error reading chunk = 0x%x\n", Result));
			CurPos = iffSeekStream( iffc, 0, FILESEEK_CURRENT);
			if(NextPos != CurPos)
			{
DBUG(("iffParseChunk: Not at end of chunk so skip!\n"));
				Result = iffSkipChunkData( iffc, NextPos - CurPos );
				CHECKERR(("iffParseChunk: error skipping chunk = 0x%x\n", Result));
			}
	}

error:
	return Result;
}

/**********************************************************************
** Parse top level FORM of a file.
**********************************************************************/
int32 iffParseForm( iff_control *iffc )
{
	uint32 ChunkType;
	uint32 FormType;
	uint32 FormSize;
	int32  Result;

	iffc->iffc_length = 8;  /* So we can read header. */

	Result = iffReadChunkHeader(iffc, &ChunkType, &FormSize);
	CHECKERR(("iffParseForm: Error reading CHUNK header: 0x%x\n", Result));

	if(ChunkType == ID_FORM)
	{
		EVENUP(FormSize);           /* !!! shouldn't actually do this: odd-length FORMs are illegal */
		iffc->iffc_length = FormSize;
		Result = iffReadChunkData(iffc, &FormType, 4);
		CHECKERR(("Error reading FORM type: 0x%x\n", Result));
/* Call user specified parser. */
		Result = (*(iffc->iffc_FormHandler))( iffc, FormType, FormSize-4 );
		CHECKERR(("Error in user FormHandler: 0x%x\n", Result));
	}
	else
	{
		ERR(("iffParseForm: Sorry. Only FORMs supported, not 0x%x\n", ChunkType));
		Result = ML_ERR_NOT_IFF_FORM;
		goto error;
	}

DBUG(("iffParseForm: Length remaining. = %d\n", iffc->iffc_length ));

error:
	return Result;
}

/******************************************************/
int32 iffParseImage ( iff_control *iffc, char *Image, int32 NumBytes )
{
	int32 Result;

	iffOpenImage( iffc, Image, NumBytes );
	iffc->iffc_length = 0;
	Result = iffParseForm( iffc );
	iffCloseImage( iffc );
	return Result;
}

/******************************************************/
int32 iffParseFile ( iff_control *iffc, const char *FilePathName )
{
	int32 Result;

	Result = iffOpenFileRead (iffc, FilePathName);
	if (Result < 0)
	{
		ERR(("iffParseFile: Could not find file %s\n", FilePathName ));
		goto error;
	}
	else
	{
		iffc->iffc_length = 0;
		Result = iffParseForm( iffc );
	}
error:
	iffCloseFile(iffc);
	if( Result < 0 )
	{
		ERR(("iffParseFile: error 0x%x on file = %s\n", Result, FilePathName));
	}
	return Result;
}

/******************************************************/
int32 iffOpenFileRead (iff_control *iffc, const char *filename)
{
	return OpenFlexStreamFile (&iffc->iffc_FlexStream, filename);
}

/******************************************************/
int32 iffCloseFile (iff_control *iffc)
{
	return CloseFlexStreamFile ( &iffc->iffc_FlexStream );
}

/******************************************************/
int32 iffOpenImage (iff_control *iffc, char *Image, int32 NumBytes )
{
	return OpenFlexStreamImage ( &iffc->iffc_FlexStream, Image, NumBytes );
}

/******************************************************/
int32 iffCloseImage (iff_control *iffc)
{
	return CloseFlexStreamImage ( &iffc->iffc_FlexStream );
}


/******************************************************/
int32 iffReadStream( iff_control *iffc, char *Addr, int32 NumBytes )
{
	return ReadFlexStream( &iffc->iffc_FlexStream, Addr, NumBytes );
}

/******************************************************/
int32 iffSeekStream( iff_control *iffc, int32 Offset, FileSeekModes Mode )
{
	return SeekFlexStream( &iffc->iffc_FlexStream, Offset, Mode );
}


/******************************************************
**
**    Read the type and size of a FORM - OBSOLETE
**
******************************************************/
int32 iffReadForm (iff_control *iffc, uint32 *type)
{
	uint32	pad[3];
	int32   Result;

	Result = iffReadStream ( iffc, (char *) pad, 12);

DBUG(("iffReadForm - %d, $%x, %d, $%x\n", Result, pad[0], pad[1], pad[2]));

	*type = pad[2];
	iffc->iffc_length = pad[1] - 4;  /* account for FORM type */

	if (pad[0] != ID_FORM) return(-1);
	return Result;
}

/******************************************************
**
**    Read the type and size of a CHUNK
**
******************************************************/
int32 iffReadChunkHeader (iff_control *iffc, uint32 *type, uint32 *size)
{
	uint32	pad[2];
	int32	Result;

	if ( (iffc->iffc_length - 8) >= 0)
	{
		Result = iffReadStream ( iffc, (char *) pad, 8);
		iffc->iffc_length -= 8;
	}
	else
	{
		Result = ML_ERR_END_OF_FILE;
		ERR(("iffReadChunkHeader: Attempted read past end of IFF file!\n"));
		goto error;
	}


	*type = pad[0];
	*size = pad[1];

DBUG(("iffReadChunkHeader - %d, $%x, %d\n", Result, pad[0], pad[1]));

error:
	return Result;
}

/******************************************************
**
**    Read the data from a CHUNK
**
******************************************************/
int32 iffReadChunkData (iff_control *iffc,
			void  *data, uint32 numbytes)
{
	int32		Result;

DBUG(("iffReadChunkData: left = %d, numbytes = %d\n", iffc->iffc_length, numbytes));

    /* !!! this function has no protection against reading beyond end of chunk */

	EVENUP(numbytes);           /* !!! shouldn't do this here: prevents doing byte-oriented reads from a chunk */
	if ( (iffc->iffc_length - (int32) numbytes) >= 0)
	{
		Result = iffReadStream ( iffc, (char *) data, numbytes);
		iffc->iffc_length -= numbytes;
	}
	else
	{
		Result = ML_ERR_END_OF_FILE;
		ERR(("iffReadChunkData: Attempted read past end of IFF file! %d\n", numbytes));
		goto error;
	}

error:
	return Result;
}


/******************************************************
**
**    Skip the data in a CHUNK
**
******************************************************/
int32 iffSkipChunkData (iff_control *iffc, uint32 NumBytes)
{
	int32   Result;
	int32   CurPos, NewPos;

DBUG(("iffSkipChunkData: NumBytes = %d\n", NumBytes));
/* Even up file position.  May be sitting on odd boundary now. */
	CurPos = iffSeekStream(iffc, 0, FILESEEK_CURRENT);
	NewPos = CurPos + NumBytes;
	EVENUP(NewPos);
	NumBytes = NewPos - CurPos;

DBUG(("iffSkipChunkData: CurPos = %d, NewPos = %d, NumBytes = %d\n",
	CurPos, NewPos, NumBytes));
/* fseek 1 for relative to current */
	if (((int32)iffc->iffc_length - (int32)NumBytes) >= 0)
	{
		Result = iffSeekStream(iffc, NumBytes, FILESEEK_CURRENT);
		iffc->iffc_length -= NumBytes;
	}
	else
	{
		Result = ML_ERR_END_OF_FILE;
		ERR(("Attempted seek past end of FORM! %d > %d\n", NumBytes, iffc->iffc_length ));
	}
DBUG(("iffSkipChunkData returns = 0x%x\n", Result));
	return Result;
}


/********************************************************************/
/************* Write IFF File ***************************************/
/********************************************************************/
#if 0
int32 iff_open_file_write (iff_control *iffc, const char *filename)
{
	iffc->iffc_FileStream =  fopen( filename, "w" );

DBUG(("iff_open_file_write - %x\n", iffc->iffc_FileStream));

	return(iffc->iffc_FileStream == NULL);
}

/******************************************************/
int32 iff_begin_form (iff_control *iffc, uint32 type)
{
	uint32	pad[3];
	int32		Result;

DBUG(("iff_begin_form - $%x\n", type));

/* load pad with data to be written */
	pad[0] = ID_FORM;
	pad[1] = 0x3FFFFFFF;  /* Default set to very long for TAPE: */
	pad[2] = type;
	iffc->iffc_length = 4; /* cuz type already written */

	Result = fwrite ((char *) &pad, 1, 12, iffc->iffc_FileStream);

	return(Result != 12);
}

/******************************************************/
int32 iff_end_form (iff_control *iffc)
{
	uint32	pad;
	int32	Result;

DBUG(("iff_end_form - %d\n", iffc->iffc_length));

/* load pad with data to be written */
	pad = iffc->iffc_length;

/* try to seek to FORM size field */
	Result = fseek (iffc->iffc_FileStream, 4, 0); /* 0 means from beginning */
	if (Result) return(2);

	Result = fwrite ((char *) &pad, 1, 4, iffc->iffc_FileStream);

	if ((Result != 4) && (Result > 0)) return -1;
	return(Result);
}


/******************************************************/
int32 iff_write_chunk (iff_control *iffc, uint32 type,
		void *data, int32 NumBytes)
{
	uint32	pad[2];
	int32		Result;

DBUG(("iff_write_chunk - %d\n", NumBytes));

/* load pad with data to be written */
	pad[0] = type;
	pad[1] = NumBytes;

	IFFWRITE(&pad, 8);
	if (Result != 8) return(TRUE);

	EVENUP(NumBytes);
	if (NumBytes > 0) IFFWRITE(data, NumBytes);
	return(Result);
}

#endif
