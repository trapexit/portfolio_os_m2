#ifndef __KERNEL_MEM_H
#define __KERNEL_MEM_H


/******************************************************************************
**
**  @(#) mem.h 96/07/15 1.46
**
**  Kernel memory management definitions
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef EXTERNAL_RELEASE
#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#endif /* EXTERNAL_RELEASE */

/*****************************************************************************/


/* types of memory that exist */
#define MEMTYPE_NORMAL     (uint32)0x00000000   /* general-purpose RAM       */

/* options for allocator */
#define MEMTYPE_FILL       (uint32)0x00000100   /* fill memory with value    */
#define MEMTYPE_TRACKSIZE  (uint32)0x00000200   /* track allocation size     */
#define MEMTYPE_FROMTOP    (uint32)0x00000400   /* allocate from top of mem  */

/* bit-fields within a memory type specification */
#define MEMTYPE_TYPESMASK  (uint32)0x0fff0000   /* bits to indicate type     */
#define MEMTYPE_OPTSMASK   (uint32)0x0000ff00   /* bits to indicate options  */
#define MEMTYPE_FILLMASK   (uint32)0x000000ff   /* bits to use as fill byte  */
#define MEMTYPE_RSVDMASK   (uint32)0xf0000000   /* bits for future use       */

/* bits that are currently illegal to set in a memory type specification */
#define MEMTYPE_ILLEGAL    (uint32)0xfffff800   /* bits that can't be set    */

/* these are here for source compatibility, please do not use in new code */
#define MEMTYPE_ANY MEMTYPE_NORMAL
#define MEMTYPE_DMA MEMTYPE_NORMAL


/*****************************************************************************/


/* When allocating memory with MEMTYPE_TRACKSIZE, use this as a size argument
 * to FreeMem() when you release the memory.
 */
#define TRACKED_SIZE -1


/* You can use this macro to pad out an allocation to a multiple of a
 * given size. This macro only works if the size is a power of 2
 */
#define ALLOC_ROUND(size,alignment) (((size) + ((alignment) - 1)) & ~((alignment) - 1))


/*****************************************************************************/


/* ControlMem() commands */
typedef enum ControlMemCmds
{
    MEMC_NOWRITE,        /* make memory unwritable */
    MEMC_OKWRITE,        /* make memory writable   */
    MEMC_GIVE,           /* give memory away       */
    MEMC_PERSISTENT      /* mark memory persistent */
} ControlMemCmds;


/*****************************************************************************/


/* Structure passed to GetMemInfo() */
typedef struct MemInfo
{
    uint32 minfo_TaskAllocatedPages;      /* pages allocated by current task */
    uint32 minfo_TaskAllocatedBytes;      /* bytes allocated by current task */

    uint32 minfo_FreePages;               /* unallocated pages               */
    uint32 minfo_LargestFreePageSpan;     /* largest extent of free pages    */
    uint32 minfo_SystemAllocatedPages;    /* pages allocated by supervisor   */
    uint32 minfo_SystemAllocatedBytes;    /* bytes allocated by supervisor   */
    uint32 minfo_OtherAllocatedPages;     /* pages allocated by other tasks  */
} MemInfo;


/*****************************************************************************/


/* Structure passed to GetPersistentMem() */
typedef struct PersistentMemInfo
{
    void   *pinfo_Start;
    uint32  pinfo_NumBytes;
} PersistentMemInfo;


/*****************************************************************************/


/* debugging control flags for ControlMemDebug() */
#define MEMDEBUGF_ALLOC_PATTERNS       (1 << 0) /* fill memory when allocating  */
#define MEMDEBUGF_FREE_PATTERNS        (1 << 1) /* fill memory when freeing     */
#define MEMDEBUGF_PAD_COOKIES          (1 << 2) /* put cookies around allocs    */
#define MEMDEBUGF_BREAKPOINT_ON_ERRORS (1 << 3) /* invoke debugger on errors    */
#define MEMDEBUGF_CHECK_ALLOC_FAILURES (1 << 4) /* complain when alloc fails    */
#define MEMDEBUGF_KEEP_TASK_DATA       (1 << 5) /* always keep stats around     */

/* fill patterns used by memdebug subsystem */
#define MEMDEBUG_ALLOC_PATTERN 0xac0debad   /* fill value after allocating */
#define MEMDEBUG_FREE_PATTERN  0xfc0debad   /* fill value before freeing   */

typedef enum DumpMemDebugTags
{
    DUMPMEMDEBUG_TAG_TASK = 1,    /* limit dump to this task, or 0 for all */
    DUMPMEMDEBUG_TAG_SUPER        /* include super allocations in dump     */
} DumpMemDebugTags;

typedef enum RationMemDebugTags
{
    RATIONMEMDEBUG_TAG_ACTIVE = 1,
    RATIONMEMDEBUG_TAG_TASK,
    RATIONMEMDEBUG_TAG_MINSIZE,
    RATIONMEMDEBUG_TAG_MAXSIZE,
    RATIONMEMDEBUG_TAG_COUNTDOWN,
    RATIONMEMDEBUG_TAG_INTERVAL,
    RATIONMEMDEBUG_TAG_RANDOM,
    RATIONMEMDEBUG_TAG_VERBOSE,
    RATIONMEMDEBUG_TAG_BREAKPOINT_ON_RATIONING,
    RATIONMEMDEBUG_TAG_SUPER
} RationMemDebugTags;


/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


/*****************************************************************************/


/* byte allocator */
void *AllocMem(int32 memSize, uint32 memFlags);
void *AllocMemMasked(int32 minSize, uint32 memFlags, uint32 careBits, uint32 stateBits);
void  FreeMem(void *mem, int32 memSize);
void *ReallocMem(void *mem, int32 oldSize, int32 newSize, uint32 memFlags);
int32 GetMemTrackSize(const void *mem);
void  GetMemInfo(MemInfo *minfo, uint32 infoSize, uint32 memFlags);

/* page control */
void *AllocMemPages(int32 memSize, uint32 memFlags);
void  FreeMemPages(void *mem, int32 memSize);
Err   ControlMem(void *mem, int32 memSize, ControlMemCmds cmd, Item task);
int32 ScavengeMem(void);
int32 GetPageSize(uint32 memFlags);
Err   GetPersistentMem(PersistentMemInfo *info, uint32 infoSize);

/* pointer validator */
bool IsMemReadable(const void *mem, int32 size);
bool IsMemWritable(const void *mem, int32 size);
bool IsMemOwned(const void *mem, int32 size);

/* memory debugging */
Err CreateMemDebug(const TagArg *tags);
Err DeleteMemDebug(void);
Err ControlMemDebug(uint32 controlFlags);
Err DumpMemDebug(const TagArg *tags);
Err DumpMemDebugVA(uint32 tag, ...);
Err SanityCheckMemDebug(const char *banner, const TagArg *tags);
Err RationMemDebug(const TagArg *tags);
Err RationMemDebugVA(uint32 tag, ...);
void *AllocMemDebug(int32 memSize, uint32 memFlags, const char *file, int32 lineNum);
void *AllocMemMaskedDebug(int32 memSize, uint32 memFlags, uint32 careBits, uint32 stateBits, const char *file, int32 lineNum);
void  FreeMemDebug(void *mem, int32 memSize, const char *file, int32 lineNum);
void *ReallocMemDebug(void *mem, int32 oldSize, int32 newSize, uint32 memFlags, const char *file, int32 lineNum);
int32 GetMemTrackSizeDebug(const void *mem, const char *file, int32 lineNum);
void *AllocMemPagesDebug(int32 memSize, uint32 memFlags, const char *file, int32 lineNum);
void  FreeMemPagesDebug(void *mem, int32 memSize, const char *file, int32 lineNum);


#ifdef MEMDEBUG
/* when doing memory debugging, redirect these functions to the debugging versions */
#define AllocMem(s,f)	        AllocMemDebug(s,f,__FILE__,__LINE__)
#define AllocMemMasked(s,f,a,b) AllocMemMaskedDebug(s,f,a,b,__FILE__,__LINE__)
#define FreeMem(p,s)	        FreeMemDebug(p,s,__FILE__,__LINE__)
#define ReallocMem(p,o,n,f)     ReallocMemDebug(p,o,n,f,__FILE__,__LINE__)
#define GetMemTrackSize(p)      GetMemTrackSizeDebug(p,__FILE__,__LINE__)
#define AllocMemPages(s,f)      AllocMemPagesDebug(s,f,__FILE__,__LINE__)
#define FreeMemPages(m,s)       FreeMemPagesDebug(m,s,__FILE__,__LINE__)
#endif


/*****************************************************************************/
#ifndef EXTERNAL_RELEASE


/* a block of free memory in a page pool's free list */
typedef struct FreeBlock
{
    struct FreeBlock *fb_Next;   /* next free block    */
    uint32            fb_Size;   /* size of this block */
} FreeBlock;

/* one of these for every type of memory we have */
typedef struct MemRegion
{
    Node       mr;               /* name and linkage                      */
    uint32     mr_MemoryType;    /* type bits                             */
    uint32     mr_PageSize;      /* size of a page                        */
    uint32     mr_PageMask;      /* (page size) - 1                       */
    uint32     mr_PageShift;     /* page size == (1 << (page shift))      */
    uint32     mr_NumPages;      /* number of pages in range              */
    void      *mr_MemBase;       /* first byte of range                   */
    void      *mr_MemTop;        /* last byte of range                    */
    uint32    *mr_PublicPages;   /* bitarray of pages anyone can write to */
    Item       mr_Lock;          /* lock for making global changes        */
} MemRegion;

/* one of these per task, and a global one */
typedef struct PagePool
{
    FreeBlock   *pp_FreeBlocks;        /* list of free blocks           */
    Item         pp_Lock;              /* cached from MemRegion         */
    uint32       pp_PageSize;          /* cached from MemRegion         */
    uint32       pp_PageMask;          /* cached from MemRegion         */
    struct Task *pp_Owner;             /* task owning this pool         */
    uint32      *pp_OwnedPages;        /* pages owned by pool           */
    uint32      *pp_WritablePages;     /* pages pool owner can write to */
    MemRegion   *pp_MemRegion;         /* description of memory range   */
} PagePool;

/* Very simple memory range description structure for use by some of the */
/* low level routines */
typedef struct MemRange {
    uint32      mr_Start;
    uint32      mr_Size;
} MemRange;

/* allocate and free supervisor memory */
void *SuperAllocMem(int32 size, uint32 flags);
void *SuperAllocMemMasked(int32 size, uint32 flags, uint32 careBits, uint32 stateBits);
void SuperFreeMem(void *p, int32 size);
void *SuperReallocMem(void *p, int32 oldSize, int32 newSize, uint32 flags);
Err SuperFreeRawMem(void *p, uint32 size);
Err GrabBootGraphicsMem(void **pp, uint32 *psize);

/* safely allocate and free user memory from supervisor mode */
void *SuperAllocUserMem(int32 size, uint32 flags, struct Task *task);
void SuperFreeUserMem(void *p, int32 size, struct Task *task);
int32 SuperControlUserMem(uint8 *p, int32 size, ControlMemCmds cmd, Item it,
			  struct Task *t);

/* allocate and free user memory for an arbitrary task */
void *AllocUserMem(int32 size, uint32 flags, struct Task *task);
void FreeUserMem(void *p, int32 size, struct Task *task);

/* memory debugging */
void *SuperAllocMemDebug(int32 memSize, uint32 memFlags, const char *file, int32 lineNum);
void *SuperAllocMemMaskedDebug(int32 memSize, uint32 memFlags, uint32 careBits, uint32 stateBits, const char *file, int32 lineNum);
void  SuperFreeMemDebug(void *mem, int32 memSize, const char *file, int32 lineNum);
void *SuperReallocMemDebug(void *p, int32 oldSize, int32 newSize, uint32 flags, const char *sourceFile, int32 lineNum);
#define SuperAllocMemAligned(s,f,a)  (SuperAllocMemMasked(s,f,a-1,0))

#ifdef MEMDEBUG
/* when doing memory debugging, redirect these functions to the debugging versions */
#define SuperAllocMem(s,f)           SuperAllocMemDebug(s,f,__FILE__,__LINE__)
#define SuperAllocMemMasked(s,f,a,b) SuperAllocMemMaskedDebug(s,f,a,b,__FILE__,__LINE__)
#define SuperFreeMem(p,s)            SuperFreeMemDebug(p,s,__FILE__,__LINE__)
#define SuperReallocMem(p,o,n,f)     SuperReallocMemDebug(p,o,n,f,__FILE__,__LINE__)
#endif


/*****************************************************************************/
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */


/* some marginally useful macros */
#define AllocMemAligned(s,f,a)  (AllocMemMasked(s,f,a-1,0))
#define AllocMemTrack(size)	(AllocMem(size, MEMTYPE_TRACKSIZE | MEMTYPE_NORMAL))
#define AllocMemTrackWithOptions(size, options)	(AllocMem(size, MEMTYPE_TRACKSIZE | MEMTYPE_NORMAL | (options)))
#define FreeMemTrack(ptr)	(FreeMem(ptr, TRACKED_SIZE))


/*****************************************************************************/


#ifdef __DCC__
#pragma no_side_effects AvailMem(1), GetMemTrackSize, GetPageSize
#pragma no_side_effects IsMemReadable, IsMemWritable, IsMemOwned
#endif


/*****************************************************************************/


#endif	/* __KERNEL_MEM_H */
