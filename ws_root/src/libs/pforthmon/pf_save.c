/* @(#) pf_save.c 96/07/15 1.17 */
/***************************************************************
** Save and Load Dictionary
** for PForth based on 'C'
**
** Compile file based version or static data based version
** depending on PF_NO_FILEIO switch.
**
** Author: Phil Burk
** Copyright 1994 3DO, Phil Burk, Larry Polansky, Devid Rosenboom
**
** The pForth software code is dedicated to the public domain,
** and any third party may reproduce, distribute and modify
** the pForth software code or any derivative works thereof
** without any compensation or license.  The pForth software
** code is provided on an "as is" basis without any warranty
** of any kind, including, without limitation, the implied
** warranties of merchantability and fitness for a particular
** purpose and their equivalents under the laws of any jurisdiction.
**
****************************************************************
** 940225 PLB Fixed CodePtr save, was using NAMEREL instead of CODEREL
**            This would only work if the relative location
**            of names and code was the same when saved and reloaded.
** 940228 PLB Added PF_NO_FILEIO version
***************************************************************/

#include "pf_all.h"

#ifndef TOUCH
#define TOUCH(argument) ((void)argument)
#endif

#ifndef PF_NO_FILEIO

#if 0
Dictionary File Format based on IFF standard.

	'FORM'
	size
	'P4TH'  -  Form Identifier

Chunks
	'P4DI'
	size
	PForthVersion (fraction * 1000)
	EntryPoint (relative to base of code)
	UserStackSize (in cells)
	ReturnStackSize (in cells)

	'P4NM'
	size
	Name and Header portion of dictionary. (Optional)

	'P4CD'
	size
	Code portion of dictionary.
#endif

#ifndef PF_NO_SHELL
/***************************************************************/
static int32 WriteLong( FileStream *fid, int32 Val )
{
	int32 numw;

	numw = sdWriteFile( (char *) &Val, 1, sizeof(int32), fid );
	if( numw != sizeof(int32) ) return -1;
	return 0;
}

/***************************************************************/
static int32 WriteChunk( FileStream *fid, int32 ID, char *Data, int32 NumBytes )
{
	int32 numw;
	int32 EvenNumW;

	EvenNumW = EVENUP(NumBytes);

	if( WriteLong( fid, ID ) < 0 ) goto error;
	if( WriteLong( fid, EvenNumW ) < 0 ) goto error;

	numw = sdWriteFile( Data, 1, EvenNumW, fid );
	if( numw != EvenNumW ) goto error;
	return 0;
error:
	pfReportError("WriteChunk", PF_ERR_WRITE_FILE);
	return -1;
}

/****************************************************************
** Save Dictionary in File.
** If EntryPoint is NULL, save as development environment.
** If EntryPoint is non-NULL, save as turnKey environment with no names.
*/
int32 ffSaveForth( char *FileName, ExecToken EntryPoint, int32 NameSize, int32 CodeSize)
{
	FileStream *fid;
	DictionaryInfoChunk SD;
	int32 FormSize;
	int32 NameChunkSize = 0;
	int32 CodeChunkSize;


	fid = sdOpenFile( FileName, "wb" );
	if( fid == NULL )
	{
		pfReportError("pfSaveDictionary", PF_ERR_OPEN_FILE);
		return -1;
	}

/* Write FORM Header ---------------------------- */
	if( WriteLong( fid, ID_FORM ) < 0 ) goto error;
	if( WriteLong( fid, 0 ) < 0 ) goto error;
	if( WriteLong( fid, ID_P4TH ) < 0 ) goto error;

/* Write P4DI Dictionary Info  ------------------ */
	SD.sd_Version = PF_FILE_VERSION;

	SD.sd_RelCodePtr = ABS_TO_CODEREL(gCurrentDictionary->dic_CodePtr.Byte); /* 940225 */
	SD.sd_UserStackSize = gCurrentTask->td_StackBase - gCurrentTask->td_StackLimit;
	SD.sd_ReturnStackSize = gCurrentTask->td_ReturnBase - gCurrentTask->td_ReturnLimit;
	SD.sd_NumPrimitives = gNumPrimitives;  /* Must match compiled dictionary. */

	if( EntryPoint )
	{
		SD.sd_EntryPoint = EntryPoint;  /* Turnkey! */
	}
	else
	{
		SD.sd_EntryPoint = 0;
	}

/* Do we save names? */
	if( NameSize == 0 )
	{
		SD.sd_RelContext = 0;
		SD.sd_RelHeaderPtr = 0;
		SD.sd_NameSize = 0;
	}
	else
	{
/* Development mode. */
		SD.sd_RelContext = ABS_TO_NAMEREL(gVarContext);
		SD.sd_RelHeaderPtr = ABS_TO_NAMEREL(gCurrentDictionary->dic_HeaderPtr.Byte);

/* How much real name space is there? */
		NameChunkSize = QUADUP(SD.sd_RelHeaderPtr);  /* Align */

/* NameSize must be 0 or greater than NameChunkSize + 1K */
		NameSize = QUADUP(NameSize);  /* Align */
		if( NameSize > 0 )
		{
			NameSize = MAX( NameSize, (NameChunkSize + 1024) );
		}
		SD.sd_NameSize = NameSize;
	}

/* How much real code is there? */
	CodeChunkSize = QUADUP(SD.sd_RelCodePtr);
	CodeSize = QUADUP(CodeSize);  /* Align */
	CodeSize = MAX( CodeSize, (CodeChunkSize + 2048) );
	SD.sd_CodeSize = CodeSize;

	if( WriteChunk( fid, ID_P4DI, (char *) &SD, sizeof(DictionaryInfoChunk) ) < 0 ) goto error;

/* Write Name Fields if NameSize non-zero ------- */
	if( NameSize > 0 )
	{
		if( WriteChunk( fid, ID_P4NM, (char *) NAME_BASE,
			NameChunkSize ) < 0 ) goto error;
	}

/* Write Code Fields ---------------------------- */
	if( WriteChunk( fid, ID_P4CD, (char *) CODE_BASE,
		CodeChunkSize ) < 0 ) goto error;

	FormSize = sdTellFile( fid ) - 8;
	sdSeekFile( fid, 4, PF_SEEK_SET );
	if( WriteLong( fid, FormSize ) < 0 ) goto error;

	sdCloseFile( fid );
	return 0;

error:
	sdSeekFile( fid, 0, PF_SEEK_SET );
	WriteLong( fid, ID_BADF ); /* Mark file as bad. */
	sdCloseFile( fid );
	return -1;
}
#endif /* !PF_NO_SHELL */

/***************************************************************/
static int32 ReadLong( FileStream *fid, int32 *ValPtr )
{
	int32 numr;

	numr = sdReadFile( ValPtr, 1, sizeof(int32), fid );
	if( numr != sizeof(int32) ) return -1;
	return 0;
}

/***************************************************************/
cfDictionary *pfLoadDictionary( char *FileName, ExecToken *EntryPointPtr )
{
	cfDictionary *dic = NULL;
	FileStream *fid;
	DictionaryInfoChunk *sd;
	int32 ChunkID;
	int32 ChunkSize;
	int32 FormSize;
	int32 BytesLeft;
	int32 numr;

DBUG(("pfLoadDictionary( %s )\n", FileName ));

/* Open file. */
	fid = sdOpenFile( FileName, "rb" );
	if( fid == NULL )
	{
		pfReportError("pfLoadDictionary", PF_ERR_OPEN_FILE);
		goto xt_error;
	}

/* Read FORM, Size, ID */
	if (ReadLong( fid, &ChunkID ) < 0) goto read_error;
	if( ChunkID != ID_FORM )
	{
		pfReportError("pfLoadDictionary", PF_ERR_WRONG_FILE);
		goto error;
	}

	if (ReadLong( fid, &FormSize ) < 0) goto read_error;
	BytesLeft = FormSize;

	if (ReadLong( fid, &ChunkID ) < 0) goto read_error;
	BytesLeft -= 4;
	if( ChunkID != ID_P4TH )
	{
		pfReportError("pfLoadDictionary", PF_ERR_BAD_FILE);
		goto error;
	}

/* Scan and parse all chunks in file. */
	while( BytesLeft > 0 )
	{
		if (ReadLong( fid, &ChunkID ) < 0) goto read_error;
		if (ReadLong( fid, &ChunkSize ) < 0) goto read_error;
		BytesLeft -= 8;

		DBUG(("ChunkID = %4s, Size = %d\n", &ChunkID, ChunkSize ));

		switch( ChunkID )
		{
		case ID_P4DI:
			sd = (DictionaryInfoChunk *) pfAllocMem( ChunkSize );
			if( sd == NULL ) goto nomem_error;

			numr = sdReadFile( sd, 1, ChunkSize, fid );
			if( numr != ChunkSize ) goto read_error;
			BytesLeft -= ChunkSize;

			if( sd->sd_Version > PF_FILE_VERSION )
			{
				pfReportError("pfLoadDictionary", PF_ERR_VERSION_FUTURE );
				goto error;
			}
			if( sd->sd_Version < PF_EARLIEST_FILE_VERSION )
			{
				pfReportError("pfLoadDictionary", PF_ERR_VERSION_PAST );
				goto error;
			}
			if( sd->sd_NumPrimitives > NUM_PRIMITIVES )
			{
				pfReportError("pfLoadDictionary", PF_ERR_NOT_SUPPORTED );
				goto error;
			}
			if( !gVarQuiet )
			{
				MSG("pForth loading dictionary from file "); MSG(FileName);
					EMIT_CR;
				MSG_NUM_D("     File format version is ", sd->sd_Version);
				MSG_NUM_D("     Name space size = ", sd->sd_NameSize);
				MSG_NUM_D("     Code space size = ", sd->sd_CodeSize);
				MSG_NUM_D("     Entry Point     = ", sd->sd_EntryPoint);
			}
			dic = pfCreateDictionary( sd->sd_NameSize, sd->sd_CodeSize );
			if( dic == NULL ) goto nomem_error;
			gCurrentDictionary = dic;
			if( sd->sd_NameSize > 0 )
			{
				gVarContext = (char *) NAMEREL_TO_ABS(sd->sd_RelContext); /* Restore context. */
				gCurrentDictionary->dic_HeaderPtr.Byte = (uint8 *) NAMEREL_TO_ABS(sd->sd_RelHeaderPtr);
			}
			else
			{
				gVarContext = 0;
				gCurrentDictionary->dic_HeaderPtr.Byte = NULL;
			}
			gCurrentDictionary->dic_CodePtr.Byte = (uint8 *) CODEREL_TO_ABS(sd->sd_RelCodePtr);
			gNumPrimitives = sd->sd_NumPrimitives;  /* Must match compiled dictionary. */
/* Pass EntryPoint back to caller. */
			if( EntryPointPtr != NULL ) *EntryPointPtr = sd->sd_EntryPoint;
			pfFreeMem(sd);
			break;

		case ID_P4NM:
#ifdef PF_NO_SHELL
			pfReportError("pfLoadDictionary", PF_ERR_NO_SHELL );
			goto error;
#else
			if( NAME_BASE == NULL )
			{
				pfReportError("pfLoadDictionary", PF_ERR_NO_NAMES );
				goto error;
			}
			if( gCurrentDictionary == NULL )
			{
				pfReportError("pfLoadDictionary", PF_ERR_BAD_FILE );
				goto error;
			}
			if( ChunkSize > NAME_SIZE )
			{
				pfReportError("pfLoadDictionary", PF_ERR_TOO_BIG);
				goto error;
			}
			numr = sdReadFile( NAME_BASE, 1, ChunkSize, fid );
			if( numr != ChunkSize ) goto read_error;
			BytesLeft -= ChunkSize;
#endif /* PF_NO_SHELL */
			break;


		case ID_P4CD:
			if( gCurrentDictionary == NULL )
			{
				pfReportError("pfLoadDictionary", PF_ERR_BAD_FILE );
				goto error;
			}
			if( ChunkSize > CODE_SIZE )
			{
				pfReportError("pfLoadDictionary", PF_ERR_TOO_BIG);
				goto error;
			}
			numr = sdReadFile( CODE_BASE, 1, ChunkSize, fid );
			if( numr != ChunkSize ) goto read_error;
			BytesLeft -= ChunkSize;
			break;

		default:
			pfReportError("pfLoadDictionary", PF_ERR_BAD_FILE );
			sdSeekFile( fid, ChunkSize, PF_SEEK_CUR );
			break;
		}
	}

	sdCloseFile( fid );

#ifndef PF_NO_SHELL
/* Find special words in dictionary for global XTs. */
DBUG(("pfLoadDictionary: FindSpecialXTs()\n"));
	if( NAME_BASE != NULL)
	{
		ExecToken  autoInitXT;

		int32 Result = FindSpecialXTs();
		if( Result < 0 )
		{
			pfReportError("pfLoadDictionary: FindSpecialXTs", Result);
			goto error;
		}
/* Initialize system by finding and executing AUTO.INIT */
		if( ffFindC( "AUTO.INIT", &autoInitXT ) )
		{
			pfExecuteToken( autoInitXT );
		}
	}
#endif /* !PF_NO_SHELL */

DBUG(("pfLoadDictionary: return 0x%x\n", dic));
	return dic;

nomem_error:
	pfReportError("pfLoadDictionary", PF_ERR_NO_MEM);
	sdCloseFile( fid );
	return NULL;

read_error:
	pfReportError("pfLoadDictionary", PF_ERR_READ_FILE);
error:
	sdCloseFile( fid );
xt_error:
	return NULL;
}

#else /* PF_NO_FILEIO ============================================== */

/*
** Dictionary must come from data array because there is no file I/O.
*/
#ifndef HEADERPTR
	#include "pfdicdat.h"
#endif

int32 ffSaveForth( char *FileName, ExecToken EntryPoint, int32 NameSize, int32 CodeSize)
{
	TOUCH(FileName);
	TOUCH(EntryPoint);
	TOUCH(NameSize);
	TOUCH(CodeSize);

	pfReportError("ffSaveForth", PF_ERR_NOT_SUPPORTED);
	return -1;
}

/***************************************************************/
cfDictionary *pfLoadDictionary( char *FileName, ExecToken *EntryPointPtr )
{
	cfDictionary *dic;
	int32 Result;
	int32 NewNameSize, NewCodeSize;

MSG("pfLoadDictionary - Filename ignored!\n");
MSG("    Loading from embedded data.\n");

	TOUCH(FileName);
	TOUCH(EntryPointPtr);

	dic = ( cfDictionary * ) pfAllocMem( sizeof( cfDictionary ) );
	if( !dic ) goto nomem_error;
	pfSetMemory( dic, 0, sizeof( cfDictionary ));
	gCurrentDictionary = dic;

#if 0
/* Use static data directly. */
	dic->dic_HeaderBase = (uint8 *) MinDicNames;
	dic->dic_CodeBase = (uint8 *) MinDicCode;
	NewNameSize = sizeof(MinDicNames);
	NewCodeSize = sizeof(MinDicCode);
#else
/* Static data too small. Copy it to larger array. */
#ifndef PF_EXTRA_HEADERS
	#define PF_EXTRA_HEADERS  (20000)
#endif
#ifndef PF_EXTRA_CODE
	#define PF_EXTRA_CODE  (40000)
#endif
	NewNameSize = sizeof(MinDicNames) + PF_EXTRA_HEADERS;
	NewCodeSize = sizeof(MinDicCode) + PF_EXTRA_CODE;
	dic->dic_HeaderBase = (uint8 *) pfAllocMem(NewNameSize);
	if( dic->dic_HeaderBase == NULL ) goto nomem_error;
	dic->dic_CodeBase = (uint8 *) pfAllocMem(NewCodeSize);
	if( dic->dic_CodeBase == NULL ) goto nomem_error;
	pfCopyMemory( dic->dic_HeaderBase, MinDicNames, sizeof(MinDicNames) );
	pfCopyMemory( dic->dic_CodeBase, MinDicCode, sizeof(MinDicCode) );
	MSG("Static data copied to newly allocated dictionaries.\n");
#endif

	dic->dic_CodePtr.Byte = (char *) CODEREL_TO_ABS(CODEPTR);
	dic->dic_CodeLimit = dic->dic_CodeBase + NewCodeSize;
	gNumPrimitives = NUM_PRIMITIVES;

	if( NAME_BASE != NULL)
	{
		ExecToken  autoInitXT;

/* Setup name space. */
		gVarContext = (char *) NAMEREL_TO_ABS(RELCONTEXT); /* Restore context. */
		dic->dic_HeaderPtr.Byte = (char *) NAMEREL_TO_ABS(HEADERPTR);
		dic->dic_HeaderLimit = dic->dic_HeaderBase + NewNameSize;

/* Find special words in dictionary for global XTs. */
		if( (Result = FindSpecialXTs()) < 0 )
		{
			pfReportError("pfLoadDictionary: FindSpecialXTs", Result);
			goto error;
		}

/* Initialize system by finding and executing AUTO.INIT */
		if( ffFindC( "AUTO.INIT", &autoInitXT ) )
		{
			pfExecuteToken( autoInitXT );
		}
	}

	return dic;

error:
	pfReportError("pfLoadDictionary", -1);
	return NULL;

nomem_error:
	pfReportError("pfLoadDictionary", PF_ERR_NO_MEM);
	return NULL;
}

#endif /* PF_NO_FILEIO */
