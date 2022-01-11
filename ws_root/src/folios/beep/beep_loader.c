/*******************************************************
**
** @(#) beep_loader.c 96/08/05 1.13
**
** Beep folio machine loader.
**
** Author: Phil Burk
** Copyright 1995 3DO
** All Rights Reserved
*******************************************************/

#include <beep/beep.h>
#include <kernel/types.h>
#include <kernel/debug.h>       /* print_vinfo() */
#include <kernel/tags.h>        /* tag iteration */
#include <kernel/kernel.h>
#include <kernel/super.h>
#include <file/fileio.h>
#include <file/filefunctions.h>
#include <stdlib.h>
#include <dspptouch/dspp_touch.h>
#include "beep_internal.h"
#include <string.h>

#define DBUG(x)   /* PRT(x) */

#define MAX_NAME_LENGTH  (32)
/*********************************************************************
    Finds newest available .bm file in System.m2/Audio/dsp/ using
    FindFileAndIdentify().
    (lifted from Bill Barton's AudioFolio code)

    Arguments
        pathBuf
            A buffer into which the call may return the complete (absolute)
            pathname of the file which was found.

        pathBufSize
            The size of the pathBuf buffer.

        insName
            Simple file name of .dsp file to find (e.g., sawtooth.dsp)

    Results
        0 on success, Err code on failure.
        Writes to *pathBuf on success. *pathBuf unchanged on failure.
*/
static Err FindBeepFile (char *pathBuf, int32 pathBufSize, const char *insName)
{
	static const TagArg fileSearchTags[] = {
        { FILESEARCH_TAG_SEARCH_FILESYSTEMS, (TagData)DONT_SEARCH_UNBLESSED },
		/* @@@ could search for matching version instead of newest if we need to:
		**     { FILESEARCH_TAG_VERSION_EQ, PORTFOLIO_OS_VERSION } */
		TAG_END
	};
	static const char insDir[] = "System.m2/Audio/dsp/";
	char insTempPath [sizeof(insDir)-1 + MAX_NAME_LENGTH + 1];
	Err errcode;

/* Build temp path name for .dsp file */
	if (strlen(insName) > MAX_NAME_LENGTH)
	{
		ERR(("FindBeepFile: instrument name '%s' too long\n", insName));
		return BEEP_ERR_NAME_TOO_LONG;
	}
	strcpy (insTempPath, insDir);
	strcat (insTempPath, insName);

/* Find file */
	if ((errcode = FindFileAndIdentify (pathBuf, pathBufSize, insTempPath, fileSearchTags)) < 0)
	{
		ERR(("dsppLoadInsTemplate: instrument '%s' not found\n", insName));
		return errcode;
	}

	DBUG(("FindBeepFile: search for '%s'\n"
            "                          found '%s'\n", insTempPath, pathBuf));

    return 0;
}

/***************************************************************/
static int32 ReadLong( RawFile *fid, int32 *ValPtr )
{
	int32 numr;

	numr = ReadRawFile( fid, ValPtr, sizeof(int32) );
	if( numr < 0 ) return numr;
	if( numr != sizeof(int32) ) return -1;
	return 0;
}

/***************************************************************/
static Err ReadMachineFile( const char *MachineName, BeepMachine *bm)
{
	RawFile *fid = 0;
	int32 ChunkID;
	int32 ChunkSize;
	int32 FormSize;
	int32 BytesLeft;
	char insPathName [FILESYSTEM_MAX_PATH_LEN];
	int32 result;

DBUG(("dspReadMachineFile( %s )\n", MachineName ));


/* Find .bm file, returns path name in insPathName */
	if ((result = FindBeepFile (insPathName, sizeof(insPathName), MachineName)) < 0) goto error;

DBUG(("dspReadMachineFile: \n", insPathName ));
/* Open file. */
	result = OpenRawFile( &fid, insPathName, FILEOPEN_READ );
	if( result < 0 )
	{
		result = BEEP_ERR_OPEN_FILE;
		goto error;
	}

/* Read FORM, Size, ID */
	if (ReadLong( fid, &ChunkID ) < 0) goto read_error;
	if( ChunkID != ID_FORM )
	{
		result = BEEP_ERR_BAD_FILE;
		goto error;
	}

	if (ReadLong( fid, &FormSize ) < 0) goto read_error;
	BytesLeft = FormSize;
	
	if (ReadLong( fid, &ChunkID ) < 0) goto read_error;
	BytesLeft -= 4;
	if( ChunkID != ID_BEEP )
	{
		result = BEEP_ERR_BAD_FILE;
		goto error;
	}
	
/* Scan and parse all chunks in file. */
	while( BytesLeft > 0 )
	{
		void *temp;

		if (ReadLong( fid, &ChunkID ) < 0) goto read_error;
		if (ReadLong( fid, &ChunkSize ) < 0) goto read_error;
		BytesLeft -= 8;
	
		DBUG(("ChunkID = %4s, Size = %d\n", &ChunkID, ChunkSize ));
		
		temp = (uint8 *) AllocMem( ChunkSize, MEMTYPE_TRACKSIZE );
		if( temp == NULL ) goto error;
		if( ReadRawFile( fid, temp, ChunkSize ) != ChunkSize ) goto read_error;
		BytesLeft -= ChunkSize;
			
/* Do all validation later in supervisor mode. */
		switch( ChunkID )
		{
		case ID_INFO:
			bm->bm_Info = *((BeepMachineInfo *) temp);
			FreeMem( temp, TRACKED_SIZE );
			break;

/* Load Voice Data Offsets */
		case ID_VCDO:
			if( bm->bm_VoiceDataOffsets != NULL ) return BEEP_ERR_ILLEGAL_MACHINE;
			bm->bm_VoiceDataOffsets = temp;
			bm->bm_NumVoices = (GetMemTrackSize(temp)/sizeof(uint16)) - 1;
			break;
			
/* Load Code */
		case ID_CODE:
			if( bm->bm_CodeImage != NULL ) return BEEP_ERR_ILLEGAL_MACHINE;
			bm->bm_CodeImage = temp;
			bm->bm_NumCodeWords = ChunkSize/sizeof(uint16);
			break;
			
/* Load Initial Parameters */
		case ID_INIT:
			if( bm->bm_Initializer != NULL ) return BEEP_ERR_ILLEGAL_MACHINE;
			bm->bm_Initializer = temp;
			break;
			
		default:
			FreeMem( temp, TRACKED_SIZE );
			break;
		}
	}

error:
	if( fid ) CloseRawFile( fid );
	fid = NULL;
	return result;

read_error:
	result = BEEP_ERR_READ_FAILED;
	goto error;
}

	
 /**
 |||	AUTODOC -class Beep -name LoadBeepMachine
 |||	Load the Beep Machine DSP program into DSP
 |||
 |||	  Synopsis
 |||
 |||	    Item LoadBeepMachine( const char *machineName )
 |||
 |||	  Description
 |||
 |||	    Load the Beep Machine into the DSP. The machine name
 |||	    is defined in the *_machine.h include file
 |||	    as BEEP_MACHINE_NAME.  The call to LoadBeepMachine()
 |||	    should, therefore, always be:
 |||	        LoadBeepMachine( BEEP_MACHINE_NAME )
 |||
 |||	    Only one Beep Machine can be loaded at a time.
 |||	    You must, therefore, call UnloadBeepMachine(@)
 |||	    between calls to LoadBeepMachine().
 |||
 |||	  Arguments
 |||
 |||	    machineName
 |||	        Set to BEEP_MACHINE_NAME name as defined in *_machine.h.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns an Item if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    V30.
 |||
 |||	  Associated Files
 |||
 |||	    <beep/beep.h>, System.m2/Modules/beep
 |||
 |||	  Module Open Requirements
 |||
 |||	    OpenBeepFolio()
 |||
 |||	  See Also
 |||
 |||	    UnloadBeepMachine()
 **/
 /**
 |||	AUTODOC -class Beep -name UnloadBeepMachine
 |||	Unload the Beep Machine from the DSP
 |||
 |||	  Synopsis
 |||
 |||	    Err UnloadBeepMachine( beepMachine )
 |||
 |||	  Description
 |||
 |||	    Unload the Beep Machine into the DSP. 
 |||
 |||	  Arguments
 |||
 |||	    beepMachine
 |||	        Item returned by LoadBeepMachine(@).
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    V30.
 |||
 |||	  Associated Files
 |||
 |||	    <beep/beep.h>, System.m2/Modules/beep
 |||
 |||	  Module Open Requirements
 |||
 |||	    OpenBeepFolio()
 |||
 |||	  See Also
 |||
 |||	    LoadBeepMachine()
 **/

/*******************************************************
** dspLoadMachine - load a machine, set all the parameters
** to zero, and start DSP execution.
*/
Item LoadBeepMachine( const char *MachineName )
{
	BeepMachine BM;
	Err result;
	
	memset( &BM, 0, sizeof(BeepMachine) );
	result = ReadMachineFile( MachineName, &BM );
	if( result < 0 ) return result;
	
/* Now create Item from BM */
	return CreateItemVA( MKNODEID(BEEPNODE,BEEP_MACHINE_NODE),
	                     BEEP_TAG_MACHINE, (TagData) &BM,
	                     TAG_END );
}

/*************************************************************************/
static Err beepTagHandler( void *bm, void *bmPtrPtr, uint32 TagArg, void *TagData  )
{
	Err result = 0;
	TOUCH(bm);
	switch( TagArg )
	{
		case BEEP_TAG_MACHINE:
			*((BeepMachine **)bmPtrPtr) = (BeepMachine *) TagData;
			break;
		
		default:
			ERR(("beepTagHandler - unexpected Tag = 0x%x\n", TagArg ));
			result = BEEP_ERR_BADTAG;
			break;
	}
	return result;
}

/*************************************************************************/
static ValidateBeepTemplate( const BeepMachine *UserTemplate )
{
	uint32 i;
	uint16 *voiceBases;

/* Check to make sure UserTemplate memory is readable. */
	CHECK_VALID_RAM(UserTemplate, sizeof(BeepMachine));

/* Check to make sure allocated memory is readable. */
	CHECK_VALID_RAM(UserTemplate->bm_CodeImage, sizeof(uint32));
	CHECK_VALID_RAM(UserTemplate->bm_VoiceDataOffsets, UserTemplate->bm_NumVoices*sizeof(uint16));
	CHECK_VALID_RAM(UserTemplate->bm_Initializer, sizeof(uint32));

/* Check to make sure Voice Data Offsets are in range. */
	voiceBases = UserTemplate->bm_VoiceDataOffsets;
	for( i=0; i<UserTemplate->bm_NumVoices; i++ )
	{
		if( voiceBases[i] > DSPI_DATA_MEMORY_SIZE ) return BEEP_ERR_ILLEGAL_MACHINE;
	}

/* Range check various values against hardware limits. */
	if( UserTemplate->bm_NumCodeWords >= DSPI_CODE_MEMORY_SIZE ) return BEEP_ERR_ILLEGAL_MACHINE;
	if( UserTemplate->bm_Info.bminfo_NumChannelsAssigned > DSPI_MAX_DMA_CHANNELS ) return BEEP_ERR_ILLEGAL_MACHINE;

/* FIXME - validate bm_SiliconVersion */
	return 0;
}

/*************************************************************************/
static void DestroyBeepTemplate( BeepMachine *SuperTemplate )
{
DBUG(("DestroyBeepTemplate\n" ));
	if( SuperTemplate->bm_CodeImage ) FreeMem( SuperTemplate->bm_CodeImage, TRACKED_SIZE );
	if( SuperTemplate->bm_VoiceDataOffsets ) FreeMem( SuperTemplate->bm_VoiceDataOffsets, TRACKED_SIZE );
	if( SuperTemplate->bm_Initializer ) FreeMem( SuperTemplate->bm_Initializer, TRACKED_SIZE );
}

/*************************************************************************/
static CloneBeepTemplate( const BeepMachine *UserTemplate, BeepMachine *SuperTemplate )
{
	int32 size;

/* Copy all of our custom data. */
	memcpy( ((char *)SuperTemplate) + sizeof(ItemNode),
		((char *)UserTemplate) + sizeof(ItemNode),
		sizeof(BeepMachine) - sizeof(ItemNode) );

/* Clear out allocated fields in SuperTemplate. */
	SuperTemplate->bm_CodeImage = NULL;
	SuperTemplate->bm_VoiceDataOffsets = NULL;
	SuperTemplate->bm_Initializer = NULL;

#define CLONE_MEMBER(member) \
DBUG(("CloneBeepTemplate: member = 0x%x\n", UserTemplate->member )); \
	if(UserTemplate->member) \
	{ \
		size = GetMemTrackSize(UserTemplate->member); \
DBUG(("CloneBeepTemplate: size = %d\n", size )); \
		SuperTemplate->member = AllocMem( size, MEMTYPE_TRACKSIZE ); \
		if( SuperTemplate->member == NULL) goto mem_error; \
		memcpy( SuperTemplate->member, UserTemplate->member, size ); \
	}

	CLONE_MEMBER(bm_CodeImage);
	CLONE_MEMBER(bm_VoiceDataOffsets);
	CLONE_MEMBER(bm_Initializer);

#undef CLONE_MEMBER
DBUG(("CloneBeepTemplate: done.\n" ));
	return 0;

mem_error:
	DestroyBeepTemplate( SuperTemplate );
	return BEEP_ERR_NOMEM;
}

/* Initialize parameters based on INIT chunk. */
Err InitBeepMachine( BeepMachine *bm )
{
	int32 NumInits;
	int32 i,j;
	BeepMachineInit  *bmin;
	int32 voiceNum;
	int32 result = 0;

	bmin = bm->bm_Initializer;
	NumInits = GetMemTrackSize(bmin) / sizeof(BeepMachineInit);
	for( i=0; i<NumInits ; i++ )
	{
		DBUG(("InitBeepMachine: ParamID = 0x%x, Value = 0x%x\n", bmin->bmin_ParamID, bmin->bmin_Value ));
		DBUG(("InitBeepMachine: FirstVoice = %d, NumVoices = 0x%x\n", bmin->bmin_FirstVoice, bmin->bmin_NumVoices ));
/* Iterate through voices for each INIT record. */
		for( j=0; j<bmin->bmin_NumVoices; j++ )
		{
			voiceNum = bmin->bmin_FirstVoice + j;
			result = lowSetBeepVoiceParameter( voiceNum,
				bmin->bmin_ParamID, bmin->bmin_Value );
			if(result < 0) return result;
		}
		bmin++;
	}
	return result;
}

/*************************************************************************/
Item internalCreateBeepMachine ( BeepMachine *bm, TagArg *args)
{
	const BeepMachine *UserTemplate;
	int32 result;
	
DBUG(("internalCreateBeepMachine: bm = 0x%x, tags = 0x%x\n", bm, args ));
/* Make sure we haven't already loaded a Beep Machine */
	if( CUR_BEEP_MACHINE != NULL )
	{
		ERR(("internalCreateBeepMachine - machine already loaded.\n"));
		return BEEP_ERR_ALREADY_LOADED;
	}
	
	result = TagProcessor( (void *)bm, args, beepTagHandler, &UserTemplate );
	if(result < 0)
	{
		ERR(("internalCreateBeepMachine: TagProcessor failed.\n"));
		return result;
	}

/* Make sure it is a legal beep machine template. */
	result = ValidateBeepTemplate( UserTemplate );
	if( result < 0 ) return result;
	
/* Make an internal clone of Beep Machine */
	result = CloneBeepTemplate( UserTemplate, bm );
	if( result < 0 ) return result;

/* Load Beep Machine code. */
	CUR_BEEP_MACHINE = bm;

/* Initialize parameters based on INIT chunk. */
	result = InitBeepMachine( bm );
	if( result < 0 ) goto error;

/* DownloadCode to DSPP. */
	dsphDownloadCode( bm->bm_CodeImage, 0, bm->bm_NumCodeWords );
	
/* Start DSP Execution. */
	dsphStart();
	dsphEnableADIO();   /* Now enable ADIO so DSPP can proceed. */

DBUG(("internalCreateBeepMachine: returns 0x%x\n", bm->bm_Node.n_Item  ));
	return (bm->bm_Node.n_Item);
	
error:
	DestroyBeepTemplate( bm );
	return result;
}

/*************************************************************************/
Err internalDeleteBeepMachine ( BeepMachine *bm )
{
	DBUG(("internalDeleteBeepMachine(0x%x)\n", bm));
/* Disable all DMA channels that may be running. */
	WriteHardware( DSPX_CHANNEL_DISABLE, -1 );
	dsphDisableADIO(); /* Stop DAC */
	dsphHalt();  /* Stop processor. */
	DestroyBeepTemplate( bm );
	CUR_BEEP_MACHINE = NULL;
	return 0;
}

