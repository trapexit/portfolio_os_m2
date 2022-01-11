/******************************************************************************
**
**  @(#) soundplayer_file.c 96/05/15 1.28
**  $Id: soundplayer_file.c,v 1.24 1994/10/14 23:47:45 peabody Exp $
**
**  Advanced Sound Player - sound file class.
**
**  By: Bill Barton
**
**  Copyright (c) 1994, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  940809 WJB  Added to music.lib.
**  940818 WJB  Replaced marker index system with marker name system.
**              Added support for INST chunk in AIFF parser.
**  940819 WJB  Added some debug code.
**  940826 WJB  Added minimum buffer size trap based on block size.
**  940901 WJB  Added trap for readlen less than blocksize.
**  940913 WJB  Improved AIFF reader.
**  940915 WJB  Improved AIFF reader some more.
**  940915 WJB  Now expecting DoIO() to check io_Error.
**  940922 WJB  Removed redundant non-NULL tests before calling FreeMem().
**  940922 WJB  Added error handler for non-DMA-aligned markers and lengths.
**  940927 WJB  Changed allocation functions from returning pointer to returning error code.
**  941007 WJB  Added caveats re sample frame size.
**  941010 WJB  Moved cursor advancement code from ReadSoundMethods to ReadSoundData().
**  941014 WJB  Replaced all int16's with 8-bit arrays (packed_uint16, etc).
**  960207 WJB  Now uses IFF folio (but needs a tidying)
**  960208 WJB  Tidied up new AIFF parser.
**  960208 WJB  Removed IntDoIO().
**  960209 WJB  Published many AIFF support functions for use by this module and LoadSample().
**  960213 WJB  Replaced a bunch of stuff with calls to common AIFF services used by this and GetAIFFSampleInfo().
**  960515 WJB  Replaced SP_DMA_ alignment with SP_SPOOLER_ alignment to avoid spooling 16-bit data on odd addresses.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <audio/aiff_format.h>      /* FORM AIFF definitions */
#include <audio/music_iff.h>
#include <file/filefunctions.h>
#include <file/filesystem.h>
#include <kernel/mem.h>             /* AllocMem() */
#include <misc/iff.h>
#include <string.h>                 /* memcpy() */

#include "soundplayer_internal.h"


/* -------------------- Debug */

#define DEBUG_AIFF      0       /* AIFF parser */


/* -------------------- Class definition */

    /* file class */
typedef struct SPSoundFile {
    SPSound spsf;                       /* Base class */
    Item    spsf_File;                  /* File to read from */
    Item    spsf_IOReq;                 /* IOReq to use w/ spso_File */
    uint32  spsf_DataOffset;            /* Byte offset in file where sound data begins. */
    uint32  spsf_BlockSize;             /* Block size of file */
} SPSoundFile;

static void DeleteSoundFile (SPSoundFile *);
static Err ReadSoundFile (SPSoundFile *, uint32 cursorbyte, uint32 rembytes, char *bufaddr, uint32 bufsize, bool optimized, char **useaddrp, uint32 *uselenp, uint32 *nextpartiallenp);
static const SPSoundClassDefinition SoundFileClass = {
    sizeof (SPSoundFile),
    (SPDeleteSoundMethod)DeleteSoundFile,
    (SPReadSoundMethod)ReadSoundFile,
};


/* -------------------- Interesting macros */

    /*
        Determines the maximum number of blocks that the requested number
        of bytes would span. (e.g. 2 bytes could span 2 blocks, but 1 byte could only span 1)
    */
#define MAXSPANBLOCKS(readsize,blocksize) UCEIL ((readsize) + (blocksize)-1, (blocksize))


/* -------------------- Local functions */

    /* AIFF parser */
static Err ParseAIFF (SPSoundFile **resultSoundFile, const SPPlayer *, const char *fileName);

    /* Device-level file I/O */
static Err OpenSoundFile (SPSoundFile *, const char *fileName);


/* -------------------- Create/Delete */

 /**
 |||	AUTODOC -public -class libmusic -group SoundPlayer -name spAddSoundFile
 |||	Create an SPSound for an AIFF sound file.
 |||
 |||	  Synopsis
 |||
 |||	    Err spAddSoundFile (SPSound **resultSound, SPPlayer *player,
 |||	                        const char *fileName)
 |||
 |||	  Description
 |||
 |||	    Creates an SPSound for the specified AIFF sound file and adds it to the
 |||	    specified player. SPSounds created this way cause the player to spool
 |||	    the sound data directly off of disc instead of buffering the whole sound
 |||	    in memory. This is useful for playing back really long sounds.
 |||
 |||	    This function opens the specified AIFF file and scans collects its
 |||	    properties (e.g. number of channels, size of frame, number of frames,
 |||	    markers, etc). The sound is checked for sample frame formatting compatibility
 |||	    with the other SPSounds in the SPPlayer and for buffer size compatibility.
 |||	    A mismatch causes an error to be returned.
 |||
 |||	    Once that is done, all of the markers from the AIFF file are translated into
 |||	    SPMarkers. Additionally the following special markers are created:
 |||
 |||	    SP_MARKER_NAME_BEGIN
 |||	        Set to the beginning of the sound data.
 |||
 |||	    SP_MARKER_NAME_END
 |||	        Set to the end of the sound data.
 |||
 |||	    SP_MARKER_NAME_SUSTAIN_BEGIN
 |||	        Set to the beginning of the sustain loop if the sound file has a sustain
 |||	        loop.
 |||
 |||	    SP_MARKER_NAME_SUSTAIN_END
 |||	        Set to the end of the sustain loop if the sound file has a sustain loop.
 |||
 |||	    SP_MARKER_NAME_RELEASE_BEGIN
 |||	        Set to the beginning of the release loop if the sound file has a release
 |||	        loop.
 |||
 |||	    SP_MARKER_NAME_RELEASE_END
 |||	        Set to the end of the release loop if the sound file has a release loop.
 |||
 |||	    The file is left open for the entire life of this type of SPSound for
 |||	    later reading by the player.
 |||
 |||	    The length of the sound file and all of its markers must be byte-aligned
 |||	    or else this function will return ML_ERR_BAD_SAMPLE_ALIGNMENT.
 |||
 |||	    All SPSounds added to an SPPlayer are automatically disposed of when
 |||	    the SPPlayer is deleted with spDeletePlayer() (by calling spRemoveSound()).
 |||	    You can manually dispose of an SPSound with spRemoveSound().
 |||
 |||	  Arguments
 |||
 |||	    resultSound
 |||	        Pointer to buffer to write resulting
 |||	        SPSound pointer. Must be supplied or
 |||	        or else this function returns ML_ERR_BADPTR.
 |||
 |||	    player
 |||	        Pointer to an SPPlayer.
 |||
 |||	    fileName
 |||	        Name of an AIFF file to read.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Outputs
 |||
 |||	    A pointer to an allocated SPSound is written to the buffer
 |||	    pointed to by resultSound on success. NULL is written to this
 |||	    buffer on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in libmusic.a V24.
 |||
 |||	  Notes
 |||
 |||	    SoundDesigner II has several classes of markers that it supports in sound:
 |||	    loop, numeric, and text. When it saves to an AIFF file, it silently throws away
 |||	    all but the first 2 loops that may be in the edited sound. Numeric markers are
 |||	    written to an AIFF file with a leading "# " which SDII apparently uses to
 |||	    recognize numeric markers when reading an AIFF file. It unfortunately ignores the
 |||	    rest of the marker name in that case, making the actual numbers somewhat variable.
 |||	    Text markers, thankfully, have user editable names that are saved verbatim in an
 |||	    AIFF file. For this reason, we recommend using only text markers (and possibly loops 1
 |||	    and 2) when preparing AIFF files for use with the sound player in SoundDesigner II.
 |||
 |||	    Since all SPSounds belonging to an SPPlayer are played by the same
 |||	    sample player instrument, they must all have the same frame sample frame
 |||	    characteristics (width, number of channels, compression type, and compression ratio).
 |||
 |||	    SPSound to SPSound cross verification is done: an error is returned
 |||	    if they don't match. However, there is no way to verify the correctness of sample frame
 |||	    characteristics for the instrument supplied to spCreatePlayer().
 |||
 |||	  Caveats
 |||
 |||	    The sound player will not work correctly unless the sample frame
 |||	    size for the sample follows these rules:
 |||
 |||	    If frame size < 1 byte, then frames must not span byte boundaries. There
 |||	    must be an integral number of frames per byte.
 |||
 |||	    If frame size >= 1, then all frame boundaries must fall on byte boundaries.
 |||	    There must be an integral number of bytes per frame.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/soundplayer.h>, libmusic.a, System.m2/Modules/iff
 |||
 |||	  See Also
 |||
 |||	    spRemoveSound(), spAddSample()
 **/

Err spAddSoundFile (SPSound **resultSound, SPPlayer *player, const char *fileName)
{
    SPSoundFile *sound = NULL;
    Err errcode;

  #if DEBUG_Create
    printf ("spAddSoundFile() '%s'\n", fileName);
  #endif

        /* initialize result (must be done first) */
    if (!resultSound) return ML_ERR_BADPTR;
    *resultSound = NULL;

        /* Parse AIFF/AIFC file - return SPSoundFile initialized with sound parameters */
    if ((errcode = ParseAIFF (&sound, player, fileName)) < 0) goto clean;

        /* open device-level file I/O */
    if ((errcode = OpenSoundFile (sound, fileName)) < 0) goto clean;

  #if DEBUG_Create
    printf ("  file=0x%x ioreq=0x%x dataoffset=%u(0x%x) blocksize=%u(0x%x)\n",
        sound->spsf_File, sound->spsf_IOReq, sound->spsf_DataOffset, sound->spsf_DataOffset, sound->spsf_BlockSize, sound->spsf_BlockSize);
  #endif

        /* success: add to player's sounds list and return */
    spAddSound ((SPSound *)sound);
    *resultSound = (SPSound *)sound;
    return 0;

clean:
    spFreeSound ((SPSound *)sound);
    return errcode;
}


/*
    SoundFileClass delete method.
    Closes file.
    sound is never NULL.
*/
static void DeleteSoundFile (SPSoundFile *sound)
{
  #if DEBUG_Create
    printf ("DeleteSoundFile() fileitem=%ld\n", sound->spsf_File);
  #endif

        /* close file */
    DeleteIOReq (sound->spsf_IOReq);
    CloseFile (sound->spsf_File);
}


/* -------------------- Parse AIFF */

static const char *GetSpecialMarkerName (const AIFFInstrument *, uint16 id);

/*
    Parse AIFF file and return SPSoundFile initialized with sound parameters and
    markers from AIFF. Further initialization must be done to complete the
    SPSoundFile (i.e., open the device-level file I/O).

    Arguments
        resultSoundFile
            Buffer to store resulting SPSoundFile. Assumes caller has
            initialized this to NULL.

        player
            SPPlayer for the sound being added.

        fileName
            Name of AIFF/AIFC file to parse.

    Results
        0 on success, Err code on failure.

        Sets *resultSoundFile on success. Leaves *resultSoundFile unchanged on
        failure.

    @@@ this function opens/closes the IFF folio.
*/
static Err ParseAIFF (SPSoundFile **resultSoundFile, const SPPlayer *player, const char *fileName)
{
    SPSoundFile *sound = NULL;
    IFFParser *iff = NULL;
    PackedID formType;          /* AIFF or AIFC */
    Err errcode;

        /* open IFF folio */
        /* note: separate failure path to avoid calling unopened IFF folio */
    if ((errcode = OpenIFFFolio()) < 0) return errcode;

        /* scan AIFF, returns IFFParser if successfully parsed FORM AIFF or AIFC */
    if ((errcode = ScanAIFF (&iff, fileName, FALSE)) < 0) goto clean;

        /* get FORM type (AIFF or AIFC) */
    {
        const ContextNode * const top = GetCurrentContext(iff);

        if (!top) {
            errcode = ML_ERR_BAD_FORMAT;
            goto clean;
        }
        formType = top->cn_Type;
    }
  #if DEBUG_AIFF
    printf ("FORM: '%.4s'\n", &formType);
  #endif

        /* alloc/init SPSound based on info from COMM and SSND chunks */
    {
        AIFFPackedCommon commx;
        const SSNDInfo *ssnd;
        uint32 numFrames;
        uint32 sampleWidth;
        PackedID compressionType;
        uint32 compressionRatio;

            /* get COMM prop chunk */
        if (!GetPropChunk (iff, formType, ID_COMM, &commx, sizeof commx)) {
            errcode = ML_ERR_BAD_FORMAT;
            goto clean;
        }

            /* find SSND chunk ContextInfo */
        if (!(ssnd = FindSSNDInfo (iff, formType))) {
            errcode = ML_ERR_BAD_FORMAT;
            goto clean;
        }

            /* cook some fields */
        numFrames        = UNPACK_UINT32(commx.commx_NumSampleFrames);
        sampleWidth      = UCEIL (commx.comm_SampleSize, 8);
        compressionType  = formType == ID_AIFC ? UNPACK_UINT32 (commx.commx_CompressionType) : 0;
        compressionRatio = GetAIFFCompressionRatio (compressionType);

      #if DEBUG_AIFF
        printf ("COMM: frames=%lu width=%lu chan=%lu comp='%.4s' X%lu\n",
            numFrames,
            sampleWidth,
            commx.comm_NumChannels,
            &compressionType,
            compressionRatio);
        printf ("SSND: dataoffset=%lu datasize=%lu bytes\n",
            ssnd->ssnd_DataOffset,
            ssnd->ssnd_DataSize);
      #endif

            /* sanity check sound parameters */
            /* !!! publish as ValidateAIFFCommon()? */
        if ( !numFrames ||
             !sampleWidth ||
             !commx.comm_NumChannels ||
             ssnd->ssnd_DataSize * compressionRatio / (sampleWidth * commx.comm_NumChannels) < numFrames ) {

            errcode = ML_ERR_BAD_FORMAT;
            goto clean;
        }

            /* alloc SPSoundFile */
        if ((errcode = spAllocSound ((SPSound **)&sound, player, &SoundFileClass,
            numFrames,
            sampleWidth,
            commx.comm_NumChannels,
            compressionRatio)) < 0) goto clean;
        sound->spsf_DataOffset = ssnd->ssnd_DataOffset;
    }

        /* add markers from MARK and INST chunks */
    {
        bool hasInst;
        AIFFInstrument inst;
        const AIFFMarkerChunk *markerChunk = NULL;

            /* find INST prop chunk (optional)
            **
            ** note: INST requires MARK, but since we scan INST contents only
            ** when scanning markers, it doesn't matter to the sound player.
            */
        if (hasInst = (GetPropChunk (iff, formType, ID_INST, &inst, sizeof inst) != 0)) {
          #if DEBUG_AIFF
            printf ("INST: sustain: %u %u %u  release: %u %u %u\n",
                inst.inst_SustainLoop.alop_PlayMode,
                inst.inst_SustainLoop.alop_BeginMarker,
                inst.inst_SustainLoop.alop_EndMarker,
                inst.inst_ReleaseLoop.alop_PlayMode,
                inst.inst_ReleaseLoop.alop_BeginMarker,
                inst.inst_ReleaseLoop.alop_EndMarker);
          #endif
        }

            /* find MARK prop chunk (optional) */
        {
            const PropChunk *pc;

            if (pc = FindPropChunk (iff, formType, ID_MARK)) {
                markerChunk = pc->pc_Data;
                if ((errcode = ValidateAIFFMarkerChunk (markerChunk, pc->pc_DataSize)) < 0) goto clean;
            }
        }

            /* process AIFFPackedMarkers */
        if (markerChunk) {
            const AIFFPackedMarker *markx = markerChunk->mark_Markers;
            uint16 numMarkers = markerChunk->mark_NumMarkers;

          #if DEBUG_AIFF
            printf ("MARK: num=%u buf=0x%x\n", markerChunk->mark_NumMarkers, markerChunk->mark_Markers);
          #endif

            for (; numMarkers--; markx = NextAIFFPackedMarker (markx)) {
                const uint32 pos = UNPACK_UINT32 (markx->markx_Position);
                char namebuf [AIFF_PSTR_MAX_LENGTH + 1];
                const char *name;

                    /* look up name */
                if (!hasInst || !(name = GetSpecialMarkerName (&inst, markx->mark_ID))) {
                    name = DecodeAIFFPString (namebuf, markx->mark_Name, sizeof namebuf);
                }

              #if DEBUG_AIFF
                printf ("MARK: id=%u pos=%u name='%s'\n", markx->mark_ID, pos, name);
              #endif

                    /* create marker */
                if ((errcode = spAddMarker ((SPSound *)sound, pos, name)) < 0) goto clean;
            }
        }
    }

        /* success: set result */
    *resultSoundFile = sound;
    errcode = 0;

clean:
        /* delete sound on failure */
    if (errcode < 0) {
        spFreeSound ((SPSound *)sound);
    }
    DeleteIFFParser (iff);
    CloseIFFFolio();
    return errcode;
}


/*
    Returns special name for marker ID if there is one. Otherwise returns NULL.
*/
static const char *GetSpecialMarkerName (const AIFFInstrument *inst, uint16 id)
{
    if (inst->inst_SustainLoop.alop_PlayMode) {
        if (inst->inst_SustainLoop.alop_BeginMarker == id) return SP_MARKER_NAME_SUSTAIN_BEGIN;
        if (inst->inst_SustainLoop.alop_EndMarker == id)   return SP_MARKER_NAME_SUSTAIN_END;
    }
    if (inst->inst_ReleaseLoop.alop_PlayMode) {
        if (inst->inst_ReleaseLoop.alop_BeginMarker == id) return SP_MARKER_NAME_RELEASE_BEGIN;
        if (inst->inst_ReleaseLoop.alop_EndMarker == id)   return SP_MARKER_NAME_RELEASE_END;
    }
    return NULL;
}


/* -------------------- device-level file I/O */

static int32 GetFileBlockSize (Item fileioreq);
static Err ReadFile (Item fileioreq, uint32 blocknum, void *readaddr, uint32 readlen);

/*
    Open sound file and create I/O request for partially initialized
    SPSoundFile.

    Arguments
        sound
            SPSoundFile initialized with sound parameters and markers.

        fileName
            Name of AIFF/AIFC file from which sound was created.

    Results
        0 on success, Err on failure.

        Sets SPSoundFile fields:
            spsf_File
            spsf_IOReq
            spsf_BlockSize

    @@@ This function doesn't clean up after partial success, caller must do
        that by calling spFreeSound().

    @@@ This function assumes that it isn't called more than once.
*/
static Err OpenSoundFile (SPSoundFile *sound, const char *fileName)
{
    const SPPlayer * const player = sound->spsf.spso_Player;
    int32 result;

        /* open file */
    if ( (result = OpenFile (fileName)) < 0 ) return result;
    sound->spsf_File = (Item)result;

        /* create IOReq */
    if ( (result = CreateIOReq (fileName, 0, sound->spsf_File, 0)) < 0 ) return result;
    sound->spsf_IOReq = (Item)result;

        /* get file block size */
    if ( (result = GetFileBlockSize (sound->spsf_IOReq)) < 0 ) return result;
    sound->spsf_BlockSize = (uint32)result;

        /* test player's buffer size against minimum buffer size to support
        ** file's blocksize (depends on SampleInfo and blocksize) */
    {
        const uint32 reqdbufsize =
            spMinBufferSize (&sound->spsf.spso_SampleFrameInfo)-1 +     /* nearly enough space to hold a sample frame or min required for spooler */
            SP_SPOOLER_ALIGNMENT-1 +                                    /* enough space to offset reading */
                                                                        /* enough space to read blocks necessary to read a frame */
            MAXSPANBLOCKS (sound->spsf.spso_SampleFrameInfo.spfi_AlignmentBytes, sound->spsf_BlockSize) * sound->spsf_BlockSize;

      #if DEBUG_Create
        printf ("  minbufsize=%lu\n", reqdbufsize);
      #endif

        if (player->sp_BufferSize < reqdbufsize) return ML_ERR_BUFFER_TOO_SMALL;
    }

        /* success */
    return 0;
}


/*
    SoundFileClass read method
*/
static Err ReadSoundFile (SPSoundFile *sound, uint32 cursorbyte, uint32 rembytes, char *bufaddr, uint32 bufsize, bool optimized, char **useaddrp, uint32 *uselenp, uint32 *nextpartiallenp)
{
    SPPlayer * const player = sound->spsf.spso_Player;
    uint32 filepos, fileblocknum, fileblockoffset;
    uint32 readoffset, readlen;
    uint32 validlen;
    uint32 useoffset, uselen;
    uint32 nextpartiallen;
    Err errcode;

    TOUCH(optimized);

        /* find data in file */
    filepos         = sound->spsf_DataOffset + cursorbyte;
    fileblocknum    = filepos / sound->spsf_BlockSize;
    fileblockoffset = filepos % sound->spsf_BlockSize;

        /*
            Trap misalignment

            Only one of PartialFrameLen and fileblockoffset can be non-zero
            because PartialFrameLen is the amount left over from the previous
            block. If adding it to the current cursor doesn't advance us to
            an even block, there's an error.
        */
    if (player->sp_PartialFrameLen && fileblockoffset) {
        errcode = ML_ERR_CORRUPT_DATA;
        goto clean;
    }

        /*
            If partiallen == 0: first byte we care about in block needs to be safely aligned for the spooler.
            Find a useoffset that is so aligned, then figure out readoffset

            If partiallen > 0: partial gets copied to beginning of buffer and block
            should be appended to it (block offset must be 0)

            @@@ We're assuming that the buffer's starting address is already
                aligned to some multiple of SP_SPOOLER_ALIGNMENT.
        */
    useoffset = (fileblockoffset + SP_SPOOLER_ALIGNMENT - 1) & SP_SPOOLER_ALIGNMENT_MASK;
    readoffset = useoffset - fileblockoffset + player->sp_PartialFrameLen;

        /* compute part of buffer to read into (bound by read offset and # of blocks we need and can fit) */
    readlen = MIN ( UCEIL (fileblockoffset + rembytes, sound->spsf_BlockSize),  /* # of blocks we need */
                    UFLOOR (bufsize - readoffset, sound->spsf_BlockSize)        /* # of blocks that can fit into buffer */
                  ) * sound->spsf_BlockSize;

  #if DEBUG_FillSpooler
    printf ("    file: pos=%lu block=%lu offset=%lu\n", filepos, fileblocknum, fileblockoffset);
    printf ("    read: offset=%lu len=%lu\n", readoffset, readlen);
  #endif

        /* check for readlen being less than a block (could only happen if bufsize
           was too small, so shouldn't actually need to succeed for this case) */
    if (readlen < sound->spsf_BlockSize) {
        errcode = ML_ERR_BUFFER_TOO_SMALL;
        goto clean;
    }

        /* compute valid part of buffer, next partial frame, and amount to return to caller (uselen) */
    validlen       = player->sp_PartialFrameLen + MIN (readlen - fileblockoffset, rembytes);
    nextpartiallen = validlen % player->sp_SampleFrameInfo.spfi_AlignmentBytes;
    uselen         = validlen - nextpartiallen;

  #if DEBUG_FillSpooler
    printf ("    result: useoffset=%lu validlen=%lu uselen=%lu partiallen=%lu\n", useoffset, validlen, uselen, nextpartiallen);
  #endif

        /* read into buffer */
        /* @@@ this memcpy() could be centralized */
    memcpy (bufaddr, player->sp_PartialFrameAddr, player->sp_PartialFrameLen);
    if ( (errcode = ReadFile (sound->spsf_IOReq, fileblocknum, bufaddr + readoffset, readlen)) < 0 ) goto clean;

        /* set resulting useaddr, uselen, nextpartiallen */
    *useaddrp        = bufaddr + useoffset;
    *uselenp         = uselen;
    *nextpartiallenp = nextpartiallen;

    return 0;

clean:
    return errcode;
}


/*
    This function returns an error instead of returning a block size of 0.
*/
static int32 GetFileBlockSize (Item fileioreq)
{
    IOInfo ioinfo;
    FileStatus stat;
    Err errcode;

    memset (&ioinfo, 0, sizeof ioinfo);
    ioinfo.ioi_Command         = CMD_STATUS;
    ioinfo.ioi_Recv.iob_Buffer = &stat;
    ioinfo.ioi_Recv.iob_Len    = sizeof stat;

    return (errcode = DoIO (fileioreq, &ioinfo)) < 0 ? errcode
         : stat.fs.ds_DeviceBlockSize ? stat.fs.ds_DeviceBlockSize
         : ML_ERR_BADITEM;      /* @@@ could have a more suitable error code, but this should be fine */
}

/*
    Synchronous file read
*/
static Err ReadFile (Item fileioreq, uint32 blocknum, void *readaddr, uint32 readlen)
{
    IOInfo ioinfo;

    memset (&ioinfo, 0, sizeof ioinfo);
    ioinfo.ioi_Command         = CMD_BLOCKREAD;
    ioinfo.ioi_Offset          = blocknum;
    ioinfo.ioi_Recv.iob_Buffer = readaddr;
    ioinfo.ioi_Recv.iob_Len    = readlen;

    return DoIO (fileioreq, &ioinfo);
}
