#ifndef __INTERNATIONAL_JSTRING_H
#define __INTERNATIONAL_JSTRING_H


/******************************************************************************
**
**  @(#) jstring.h 95/10/03 1.7
**  $Id: jstring.h,v 1.4 1994/11/04 17:53:11 vertex Exp $
**
**  JString folio interface definitions
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif


/*****************************************************************************/


/* kernel interface definitions */
#define JSTR_FOLIONAME  "jstring"


/*****************************************************************************/


/* jstring folio errors */
#define MakeJstrErr(svr,class,err) MakeErr(ER_FOLI,ER_JSTR,svr,ER_E_SSTM,class,err)

#define JSTR_ERR_BUFFERTOOSMALL MakeJstrErr(ER_SEVERE,ER_C_NSTND,1)


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


/* folio management */
Err OpenJStringFolio(void);
Err CloseJStringFolio(void);

/* conversion routines */
int32 ConvertShiftJIS2UniCode(const char *string, unichar *result,
                              uint32 resultSize, uint8 filler);
int32 ConvertUniCode2ShiftJIS(const unichar *string, char *result,
                              uint32 resultSize, uint8 filler);
int32 ConvertASCII2ShiftJIS(const char *string, char *result,
                            uint32 resultSize, uint8 filler);
int32 ConvertShiftJIS2ASCII(const char *string, char *result,
                            uint32 resultSize, uint8 filler);
int32 ConvertRomaji2Hiragana(const char *string, char *result,
                             uint32 resultSize, uint8 filler);
int32 ConvertRomaji2FullKana(const char *string, char *result,
                             uint32 resultSize, uint8 filler);
int32 ConvertRomaji2HalfKana(const char *string, char *result,
                             uint32 resultSize, uint8 filler);
int32 ConvertHiragana2Romaji(const char *string, char *result,
                             uint32 resultSize, uint8 filler);
int32 ConvertHiragana2HalfKana(const char *string, char *result,
                               uint32 resultSize, uint8 filler);
int32 ConvertHiragana2FullKana(const char *string, char *result,
                               uint32 resultSize, uint8 filler);
int32 ConvertFullKana2Romaji(const char *string, char *result,
                             uint32 resultSize, uint8 filler);
int32 ConvertFullKana2HalfKana(const char *string, char *result,
                               uint32 resultSize, uint8 filler);
int32 ConvertFullKana2Hiragana(const char *string, char *result,
                               uint32 resultSize, uint8 filler);
int32 ConvertHalfKana2Romaji(const char *string, char *result,
                             uint32 resultSize, uint8 filler);
int32 ConvertHalfKana2FullKana(const char *string, char *result,
                               uint32 resultSize, uint8 filler);
int32 ConvertHalfKana2Hiragana(const char *string, char *result,
                               uint32 resultSize, uint8 filler);


#ifdef __cplusplus
}
#endif


/*****************************************************************************/


#endif /* __INTERNATIONAL_JSTRING_H */
