/* @(#) createenvelope.c 95/09/26 1.1 */

#include <audio/audio.h>
#include <kernel/item.h>

Item CreateEnvelope (const EnvelopeSegment *points, int32 numPoints, const TagArg *tags)
{
    return CreateItemVA (MKNODEID(AUDIONODE,AUDIO_ENVELOPE_NODE),
        AF_TAG_ADDRESS, points,
        AF_TAG_FRAMES,  numPoints,
        TAG_JUMP,       tags);
}
