#ifndef __KERNEL_TAGS_H
#define __KERNEL_TAGS_H


/******************************************************************************
**
**  @(#) tags.h 95/03/23 1.7
**  $Id: tags.h,v 1.2 1994/09/21 19:11:20 peabody Exp $
**
**  Tag management definitions
**
******************************************************************************/

#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


void DumpTagList(const TagArg *tagList, const char *desc);
TagArg *FindTagArg(const TagArg *tagList, uint32 tag);
TagData GetTagArg(const TagArg *tagList, uint32 tag, TagData defaultValue);
TagArg *NextTagArg(const TagArg **tagList);


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


/*****************************************************************************/


#ifdef __DCC__
#pragma no_side_effects FindTagArg, GetTagArg, NextTagArg(1), DumpTagList
#endif


/*****************************************************************************/


#endif	/* __KERNEL_TAGS_H */
