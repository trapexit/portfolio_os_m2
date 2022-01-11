#ifndef __AUDIO_MIDIFILE_H
#define __AUDIO_MIDIFILE_H


/****************************************************************************
**
**  @(#) midifile.h 95/12/12 1.11
**
**  MIDI File Parser Includes
**
****************************************************************************/

#ifndef __AUDIO_FLEX_STREAM_H
#include <audio/flex_stream.h>
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __MISC_FRAC16_H
#include <misc/frac16.h>
#endif


#define NUMMIDICHANNELS (16)
#define NUMMIDIPROGRAMS (128)

typedef struct MIDIFileParser
{
	FlexStream mfp_FlexStream;
	int32   mfp_Format;
	int32   mfp_NumTracks;
	int32   mfp_TrackIndex;
	int32   mfp_Division;
	int32   mfp_Shift;          /* Scale times by shifting if needed. */
	int32   mfp_Tempo;
	frac16  mfp_Rate;           /* TicksPerSecond */
	int32   mfp_Time;           /* Current Time as we scan track. */
	int32   mfp_RunningStatus;  /* Current running status command byte */
	int32   mfp_NumData;        /* Number of data bytes for above command. */
	int32   mfp_BytesLeft;      /* Bytes left in current track */
	int32 (*mfp_HandleTrack)(struct MIDIFileParser *, int32 size);  /* User function to handle Track */
	int32 (*mfp_HandleEvent)(struct MIDIFileParser *, const uint8 *data, int32 numBytes);  /* User function to handle Event */
	void   *mfp_UserData;       /* For whatever user wants. */
} MIDIFileParser;

typedef struct MIDIEvent
{
	uint32	mev_Time;
	unsigned char mev_Command;
	unsigned char mev_Data1;
	unsigned char mev_Data2;
	unsigned char mev_Data3;
} MIDIEvent;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Err ParseMFEvent( MIDIFileParser *mfpptr );
Err ParseMFHeader( MIDIFileParser *mfpptr );
Err ParseMFMetaEvent( MIDIFileParser *mfpptr );
Err ParseMFTrack( MIDIFileParser *mfpptr );
Err ScanMFTrack( MIDIFileParser *mfpptr, int32 Size );
int32 GetMFChar( MIDIFileParser *mfpptr );
int32 MIDIDataBytesPer( int32 command );
int32 ParseMFVLN( MIDIFileParser *mfpptr );
int32 ReadMFTrackHeader( MIDIFileParser *mfpptr );

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __AUDIO_MIDIFILE_H */
