/* @(#) createprobe.c 95/10/12 1.1 */

#include <audio/audio.h>
#include <kernel/item.h>

Item CreateProbe (Item instrument, const char *outputName, const TagArg *tagList)
{
    return CreateItemVA (MKNODEID(AUDIONODE,AUDIO_PROBE_NODE),
                         AF_TAG_INSTRUMENT, instrument,
                         AF_TAG_NAME,       outputName,
                         TAG_JUMP,          tagList);
}
