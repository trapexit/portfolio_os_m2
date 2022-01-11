/* @(#) createattachment.c 95/10/12 1.1 */

#include <audio/audio.h>
#include <kernel/item.h>

Item CreateAttachment (Item master, Item slave, const TagArg *tagList)
{
    return CreateItemVA (MKNODEID(AUDIONODE,AUDIO_ATTACHMENT_NODE),
                         AF_TAG_MASTER, master,
                         AF_TAG_SLAVE,  slave,
                         TAG_JUMP,      tagList);
}
