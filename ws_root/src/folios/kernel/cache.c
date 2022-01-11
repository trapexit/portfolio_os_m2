/* @(#) cache.c 96/07/19 1.31 */

#include <kernel/types.h>
#include <kernel/operror.h>
#include <kernel/cache.h>
#include <kernel/task.h>
#include <kernel/kernel.h>
#include <kernel/sysinfo.h>
#include <kernel/internalf.h>
#include <kernel/super.h>
#include <hardware/PPCasm.h>
#include <string.h>


/*****************************************************************************/


void GetCacheInfo(CacheInfo *info, uint32 infoSize)
{
CacheInfo    ci;
SysCacheInfo sci;
uint32       flags;
uint32       hid0;

    SuperQuerySysInfo(SYSINFO_TAG_CACHE, &sci, sizeof(sci));

    flags = 0;

    hid0 = _GetCacheState();
    if (hid0 & HID_DCE)
        flags |= CACHE_DATA_ENABLED;

    if (hid0 & HID_DLOCK)
        flags |= CACHE_DATA_LOCKED;

    if (hid0 & HID_WIMG_WRTHU)
        flags |= CACHE_DATA_WRITETHROUGH;

    if (hid0 & HID_ICE)
        flags |= CACHE_INSTR_ENABLED;

    if (hid0 & HID_ILOCK)
        flags |= CACHE_INSTR_LOCKED;

    if (sci.sci_Flags & SYSINFO_CACHE_PUNIFIED)
        flags |= CACHE_UNIFIED;

    ci.cinfo_Flags          = flags;
    ci.cinfo_ICacheSize     = sci.sci_PICacheSize;
    ci.cinfo_ICacheLineSize = sci.sci_PICacheLineSize;
    ci.cinfo_ICacheSetSize  = sci.sci_PICacheSetSize;
    ci.cinfo_DCacheSize     = sci.sci_PDCacheSize;
    ci.cinfo_DCacheLineSize = sci.sci_PDCacheLineSize;
    ci.cinfo_DCacheSetSize  = sci.sci_PDCacheSetSize;

    if (infoSize > sizeof(CacheInfo))
    {
        memset(info,0,sizeof(infoSize));
        infoSize = sizeof(CacheInfo);
    }

    memcpy(info, &ci, infoSize);
}


/*****************************************************************************/


typedef void (* CacheFunc)(void);

/* map from a command to a function */
static const CacheFunc maps[] =
{
    EnableICache,
    DisableICache,
    EnableDCache,
    DisableDCache,
    WriteThroughDCache,
    CopyBackDCache,
};

Err externalControlCaches(ControlCachesCmds cmd)
{
    if (cmd > CACHEC_DATA_COPYBACK)
        return BADCACHECMD;

    (* maps[cmd])();
    return 0;
}
