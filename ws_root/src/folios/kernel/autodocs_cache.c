/* @(#) autodocs_cache.c 96/09/17 1.2 */

/**
|||	AUTODOC -class Kernel -group Caches -name GetCacheInfo
|||	Gets information about the CPU caches.
|||
|||	  Synopsis
|||
|||	    void GetCacheInfo(CacheInfo *info, uint32 infoSize);
|||
|||	  Description
|||
|||	    This function returns information about the CPU caches.
|||	    The information includes their size and their state. The info
|||	    is returned in a CacheInfo structure, containing the following
|||	    fields:
|||
|||	    cinfo_Flags
|||	        Provides a number of flag that show the state of the
|||	        caches. See the CACHE_XXX flags in <kernel/cache.h>.
|||
|||	    cinfo_ICacheSize and cinfo_DCacheSize
|||	        Specifies the number of bytes in the primary CPU instruction
|||	        and data caches.
|||
|||	    cinfo_ICacheLineSize and cinfo_DCacheLineSize
|||	        Specifies the number of bytes in one line of the primary
|||	        CPU instruction and data cache.
|||
|||	    cinfo_ICacheSetSize and cinfo_DCacheSetSize
|||	        Specifies the level of set associativity implemented in the
|||	        primary CPU instruction and data cache.
|||
|||	  Arguments
|||
|||	    info
|||	        Pointer to a structure where the cache information will
|||	        be put.
|||
|||	    infoSize
|||	        Size of the structure to receive the cache information.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/cache.h>
|||
|||	  See Also
|||
|||	    FlushDCache(), WriteBackDCache(), FlushDCacheAll(),
|||	    ControlCaches(), InvalidateICache()
|||
**/

/**
|||	AUTODOC -class Kernel -group Caches -name FlushDCacheAll
|||	Write back modified contents of the data cache to memory, and
|||	remove the data from the cache.
|||
|||	  Synopsis
|||
|||	    void FlushDCacheAll(uint32 reserved);
|||
|||	  Description
|||
|||	    This function writes back data to main memory from the CPU data
|||	    cache, and then removes the data from the cache. The writes only
|||	    occur if the data has been modified and not written back to memory
|||	    previously.
|||
|||	  Arguments
|||
|||	    reserved
|||	        Reserved for future use, must be 0.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Warning
|||
|||	    This function should very seldom be used by application code.
|||	    Abusing this function will cause a tremendous decrease in system
|||	    performance.
|||
|||	  Associated Files
|||
|||	    <kernel/cache.h>
|||
|||	  See Also
|||
|||	    WriteBackDCache(), FlushDCache(), InvalidateICache(),
|||	    ControlCaches(), GetCacheInfo()
|||
**/

/**
|||	AUTODOC -class Kernel -group Caches -name WriteBackDCache
|||	Write back modified contents of the data cache to memory.
|||
|||	  Synopsis
|||
|||	    void WriteBackDCache(uint32 reserved, const void *start,
|||	                         uint32 numBytes);
|||
|||	  Description
|||
|||	    This function writes back data to main memory from the CPU data
|||	    cache. The writes only occur if the data has been modified and not
|||	    written back to memory previously. Only the data in the supplied
|||	    address range is affected.
|||
|||	  Arguments
|||
|||	    reserved
|||	        Reserved for future use, must be 0.
|||
|||	    start
|||	        The starting address in the memory range to make sure is
|||	        written back.
|||
|||	    numBytes
|||	        The number of bytes in the memory range.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Warning
|||
|||	    This function should very seldom be used by application code.
|||	    Abusing this function will cause a tremendous decrease in system
|||	    performance.
|||
|||	  Associated Files
|||
|||	    <kernel/cache.h>
|||
|||	  See Also
|||
|||	    FlushDCache(), FlushDCacheAll(), InvalidateICache(),
|||	    ControlCaches(), GetCacheInfo()
|||
**/

/**
|||	AUTODOC -class Kernel -group Caches -name FlushDCache
|||	Write back modified contents of the data cache to memory, and
|||	remove the data from the cache.
|||
|||	  Synopsis
|||
|||	    void FlushDCache(uint32 reserved, const void *start,
|||	                     uint32 numBytes);
|||
|||	  Description
|||
|||	    This function writes back data to main memory from the CPU data
|||	    cache, and then removes the data from the cache. The writes only
|||	    occur if the data has been modified and not written back to memory
|||	    previously. Only the data in the supplied address range is affected.
|||
|||	  Arguments
|||
|||	    reserved
|||	        Reserved for future use, must be 0.
|||
|||	    start
|||	        The starting address in the memory range to make sure is
|||	        written back and removed from the cache.
|||
|||	    numBytes
|||	        The number of bytes in the memory range.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Warning
|||
|||	    This function should very seldom be used by application code.
|||	    Abusing this function will cause a tremendous decrease in system
|||	    performance.
|||
|||	  Associated Files
|||
|||	    <kernel/cache.h>
|||
|||	  See Also
|||
|||	    FlushDCacheAll(), WriteBackDCache(), InvalidateICache(),
|||	    ControlCaches(), GetCacheInfo()
|||
**/

/**
|||	AUTODOC -class Kernel -group Caches -name InvalidateICache
|||	Removes all data from the instruction cache, forcing the CPU to
|||	fetch instructions from main memory.
|||
|||	  Synopsis
|||
|||	    void InvalidateICache(void);
|||
|||	  Description
|||
|||	    This function invalidates the contents of the CPU's instruction
|||	    cache, which forces the CPU to refill its cache by fetching
|||	    instructions from main memory.
|||
|||	    This function is required when you want to execute some code that
|||	    has just been somehow manipulated by the CPU. For example, when
|||	    creating self-modifying code, or when loading new code in from
|||	    disk. Note that the system loader already invalidates the
|||	    instruction cache when bringing modules into memory, so you don't
|||	    need to do it too. However, if you write your own code loading
|||	    engine, you will then need to invalidate the instruction cache.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V32.
|||
|||	  Associated Files
|||
|||	    <kernel/cache.h>
|||
|||	  See Also
|||
|||	    FlushDCache(), FlushDCacheAll(), WriteBackDCache(),
|||	    ControlCaches(), GetCacheInfo()
|||
**/

/**
|||	AUTODOC -class Kernel -group Caches -name ControlCaches
|||	Lets you control the CPU caches.
|||
|||	  Synopsis
|||
|||	    Err ControlCaches(ControlCachesCmds cmd);
|||
|||	  Description
|||
|||	    This function lets you control the state of the CPU caches.
|||
|||	  Arguments
|||
|||	    cmd
|||	        A constant that specifies the cache operation to perform.
|||
|||	    The possible values of "cmd" are:
|||
|||	    CACHEC_INSTR_ENABLE
|||	        Enable the primary instruction cache. This has no effect if the
|||	        cache is already enabled. Before the instruction cache is
|||	        enabled, it is automatically invalidated in order to avoid
|||	        stale cached instructions from causing problems.
|||
|||	    CACHEC_INSTR_DISABLE
|||	        Disable the primary instruction cache. This has no effect if the
|||	        cache is already disabled.
|||
|||	    CACHEC_DATA_ENABLE
|||	        Enable the primary data cache. This has no effect if the
|||	        cache is already enabled. Before the data cache is enabled, it
|||	        is automatically invalidated in order to avoid stale cached
|||	        data from causing problems.
|||
|||	    CACHEC_DATA_DISABLE
|||	        Disable the primary data cache. This has no effect if the
|||	        cache is already disabled. Before the data cache is
|||	        disabled, it is automatically flushed of its contents.
|||
|||	    CACHEC_DATA_WRITETHROUGH
|||	        Enable the primary data cache's write-through mode. This
|||	        means that write operations are performed into the cache and
|||	        immediately written back out to memory.
|||
|||	    CACHEC_DATA_COPYBACK
|||	        Disable the primary data cache's write-through mode. This
|||	        means that write operations are performed into the cache and
|||	        the affected cache line is written out to memory at an
|||	        arbitrary later time, whenever the CPU determines that it
|||	        needs the cache line for fresh data.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V32.
|||
|||	  Associated Files
|||
|||	    <kernel/cache.h>
|||
|||	  See Also
|||
|||	    GetCacheInfo(), WriteBackDCache(),
|||	    FlushDCache(), FlushDCacheAll(), InvalidateICache()
|||
**/

/* keep the compiler happy... */
extern int foo;
