/* @(#) createknob.c 95/10/12 1.1 */

#include <audio/audio.h>
#include <kernel/item.h>

Item CreateKnob (Item instrument, const char *knobName, const TagArg *tagList)
{
    return CreateItemVA (MKNODEID(AUDIONODE,AUDIO_KNOB_NODE),
                         AF_TAG_INSTRUMENT, instrument,
                         AF_TAG_NAME,       knobName,
                         TAG_JUMP,          tagList);
}
