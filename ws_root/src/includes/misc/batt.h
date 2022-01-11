#ifndef __MISC_BATT_H
#define __MISC_BATT_H


/******************************************************************************
**
**  @(#) batt.h 96/06/07 1.3
**
**  Definitions to interface to the battery-backed system resources.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

#ifndef __MISC_DATE_H
#include <misc/date.h>
#endif


#ifndef EXTERNAL_RELEASE
/*****************************************************************************/


typedef struct BattMemInfo
{
    uint32 bminfo_NumBytes;
} BattMemInfo;


/* There are currently 120 bits of battery-backed memory.
 *
 * The first 4 bits specify the language for the machine. These four bits are
 * interpreted using the same values as the SYSINFO_TAG_INTLLANG tag defined in
 * <kernel/sysinfo.h> plus one. A value of 0 means the language is not set.
 *
 * The next 32 bits are reserved for 3DO use. The
 * remaining bits can be assigned to any purpose.
 */
#define BATTMEM_LANG  0
#define BATTMEM_3DO   4
#define BATTMEM_EXTRA 36

#define BATTMEM_LANG_OFFSET 0
#define GetBattLangCode(b)  ((b) & 0xf)


#endif
/*****************************************************************************/


#define MakeBattErr(svr,class,err) MakeErr(ER_FOLI,ER_BATT,svr,ER_E_SSTM,class,err)

#define BATT_ERR_NOHARDWARE   MakeBattErr(ER_SEVERE,ER_C_STND,ER_NoHardware)
#define BATT_ERR_BADPTR       MakeBattErr(ER_SEVERE,ER_C_STND,ER_BadPtr)
#define BATT_ERR_BADPRIV      MakeBattErr(ER_SEVERE,ER_C_STND,ER_NotPrivileged)



/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif /* cplusplus */


/* folio management */
Err OpenBattFolio(void);
Err CloseBattFolio(void);

#ifndef EXTERNAL_RELEASE
/* battery-backed memory interface */
void GetBattMemInfo(BattMemInfo *bmi, uint32 infoSize);
void LockBattMem(void);
void UnlockBattMem(void);
Err  ReadBattMem(void *buffer, uint32 numBytes, uint32 offset);
Err  WriteBattMem(const void *buffer, uint32 numBytes, uint32 offset);
#endif

/* battery-backed clock interface */
Err WriteBattClock(const GregorianDate *gd);
Err ReadBattClock(GregorianDate *gd);


#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __MISC_BATT_H */
