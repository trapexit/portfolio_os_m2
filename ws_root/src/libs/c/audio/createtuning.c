/* @(#) createtuning.c 95/10/12 1.1 */

#include <audio/audio.h>
#include <kernel/item.h>

Item CreateTuning (const float32 *frequencies, int32 numIntervals, int32 notesPerOctave, int32 baseNote)
{
    return CreateItemVA (MKNODEID(AUDIONODE,AUDIO_TUNING_NODE),
        AF_TAG_ADDRESS,        frequencies,
        AF_TAG_FRAMES,         numIntervals,
        AF_TAG_NOTESPEROCTAVE, notesPerOctave,
        AF_TAG_BASENOTE,       baseNote,
        TAG_END);
}
