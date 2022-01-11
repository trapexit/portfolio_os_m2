#ifndef __KERNEL_TAGS_H
#define __KERNEL_TAGS_H


/******************************************************************************
**
**  @(#) tags.h 96/08/06 1.11
**
**  Tag management definitions
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif



#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


TagData ConvertFP_TagData(float32 a);
float32 ConvertTagData_FP(TagData a);
void DumpTagList(const TagArg *tagList, const char *desc);
TagArg *FindTagArg(const TagArg *tagList, uint32 tag);
TagData GetTagArg(const TagArg *tagList, uint32 tag, TagData defaultValue);
TagArg *NextTagArg(const TagArg **tagList);

#ifndef EXTERNAL_RELEASE
int32 SafeFirstTagArg(const TagArg **tagp, const TagArg *tagList);
int32 SafeNextTagArg(const TagArg **tagp);
#endif


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


/*****************************************************************************/


#ifdef __DCC__
#pragma no_side_effects FindTagArg, GetTagArg, NextTagArg(1), DumpTagList
#pragma pure_function ConvertFP_TagData, ConvertTagData_FP
#ifndef EXTERNAL_RELEASE
#pragma no_side_effects SafeFirstTagArg(1), SafeNextTagArg(1)
#endif
#endif


/*****************************************************************************/


#endif	/* __KERNEL_TAGS_H */
