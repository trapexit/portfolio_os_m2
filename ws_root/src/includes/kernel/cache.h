#ifndef __KERNEL_CACHE_H
#define __KERNEL_CACHE_H


/******************************************************************************
**
**  @(#) cache.h 96/07/18 1.14
**
**  Cache management services.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************/


typedef struct CacheInfo
{
    uint32 cinfo_Flags;

    uint32 cinfo_ICacheSize;
    uint32 cinfo_ICacheLineSize;
    uint32 cinfo_ICacheSetSize;

    uint32 cinfo_DCacheSize;
    uint32 cinfo_DCacheLineSize;
    uint32 cinfo_DCacheSetSize;
} CacheInfo;

#define CACHE_UNIFIED           (1 << 0)  /* there is only a single unified cache    */
#define CACHE_INSTR_ENABLED     (1 << 1)  /* the instruction cache is on             */
#define CACHE_INSTR_LOCKED      (1 << 2)  /* the instruction cache is locked         */
#define CACHE_DATA_ENABLED      (1 << 3)  /* the data cache is on                    */
#define CACHE_DATA_LOCKED       (1 << 4)  /* the data cache is locked                */
#define CACHE_DATA_WRITETHROUGH (1 << 5)  /* the data cache is in write-through mode */


/*****************************************************************************/


/* ControlCaches() commands */
typedef enum ControlCachesCmds
{
    /* instruction cache control */
    CACHEC_INSTR_ENABLE,          /* enable the I cache              */
    CACHEC_INSTR_DISABLE,         /* disable the I cache             */

    /* data cache control */
    CACHEC_DATA_ENABLE,           /* enable the D cache              */
    CACHEC_DATA_DISABLE,          /* disable the D cache             */
    CACHEC_DATA_WRITETHROUGH,     /* enable write-through mode       */
    CACHEC_DATA_COPYBACK          /* disable write-through mode      */
} ControlCachesCmds;


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif /* cplusplus */


/* general cache control */
Err ControlCaches(ControlCachesCmds cmd);
void GetCacheInfo(CacheInfo *info, uint32 infoSize);

/* instruction cache management */
void InvalidateICache(void);

/* data cache management */
void WriteBackDCache(uint32 reserved, const void *start, uint32 numBytes);
void FlushDCache(uint32 reserved, const void *start, uint32 numBytes);
void FlushDCacheAll(uint32 reserved);

/* for source compatibility, do not use in new code */
#define GetDCacheFlushCount() 0

#ifndef EXTERNAL_RELEASE
void SuperInvalidateDCache(const void *start, uint32 numBytes);
#endif /* EXTERNAL_RELEASE */

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __KERNEL_CACHE_H */
