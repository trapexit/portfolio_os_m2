/* @(#) autodocs.c 96/12/09 1.22 */

/**
|||	AUTODOC -class Kernel -group Memory -name GetMemTrackSize
|||	Gets the size of a block of memory allocated with MEMTYPE_TRACKSIZE.
|||
|||	  Synopsis
|||
|||	    int32 GetMemTrackSize(const void *mem);
|||
|||	  Description
|||
|||	    This function returns the size that was used to allocate a block of
|||	    memory. The block of memory must have been allocated using the
|||	    MEMTYPE_TRACKSIZE flag, otherwise this function will return garbage.
|||
|||	  Arguments
|||
|||	    mem
|||	        Pointer obtained from AllocMem(). The block of memory must
|||	        have been allocated using the MEMTYPE_TRACKSIZE flag, otherwise
|||	        the value returned by this function will be random.
|||
|||	  Return Value
|||
|||	    Returns the size in bytes of the memory block. This size
|||	    corresponds to the size provided to AllocMem() when the block was
|||	    first allocated.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V24.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  See Also
|||
|||	    AllocMem(), FreeMem()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name AllocMem
|||	Allocates a block of memory.
|||
|||	  Synopsis
|||
|||	    void *AllocMem(int32 memSize, uint32 memFlags);
|||
|||	  Description
|||
|||	    This function allocates a block of memory for use by the current
|||	    task. You use FreeMem() to free a block of memory that was
|||	    allocated with AllocMem().
|||
|||	    If there is insufficient memory in a task's free memory list to
|||	    allocate the needed block, the kernel automatically attempts
|||	    to obtain more memory pages from the system page pool. These
|||	    pages become the property of the task and get added to its free
|||	    memory list. They remain the property of the task until they
|||	    are returned to the system page pool by calling ScavengeMem().
|||
|||	    When a task dies, any pages of memory it owns automatically get
|||	    returned to the system page pool.
|||
|||	    Memory allocated by a thread is owned by the parent task, and not
|||	    by the thread itself. Therefore, when a thread dies, memory it
|||	    allocated remains allocated until it is explicitly freed by the
|||	    parent task (or another thread in the same task family), or if the
|||	    parent task dies.
|||
|||	  Arguments
|||
|||	    memSize
|||	        The size of the memory block to allocate, in bytes.
|||
|||	    memFlags
|||	        Flags that specify some options for this memory allocation.
|||
|||	  Flags
|||
|||	    These are the possible values for the memFlags argument.
|||
|||	    MEMTYPE_NORMAL
|||	        Allocate standard memory. This flag must currently always be
|||	        supplied when allocating memory.
|||
|||	    The following flags specify some options concerning the allocation:
|||
|||	    MEMTYPE_FILL
|||	        Sets every byte in the memory block to the value of the lower
|||	        eight bits of the flags argument. If this bit is not set, the
|||	        previous contents of the memory block are not changed.
|||
|||	    MEMTYPE_TRACKSIZE
|||	        Tells the kernel to track the size of this memory allocation.
|||	        When you allocate memory with this flag, you must specify
|||	        TRACKED_SIZE as the size when freeing the memory using FreeMem().
|||	        This flag saves you the work of tracking the allocation's size.
|||	        Keep in mind that it slightly increases the memory overhead of
|||	        the allocation.
|||
|||	  Return Value
|||
|||	    Returns a pointer to the memory block that was allocated or
|||	    NULL if there was not enough memory available.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  Notes
|||
|||	    You can enable memory debugging in your application by compiling
|||	    your entire project with the MEMDEBUG value defined on the
|||	    compiler's command line. Refer to the CreateMemDebug() function for
|||	    more details.
|||
|||	  See Also
|||
|||	    AllocMemMasked(), FreeMem(), GetMemTrackSize()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name AllocMemTrack
|||	Allocates a block of memory with size-tracking.
|||
|||	  Synopsis
|||
|||	    void *AllocMemTrack(int32 memSize);
|||
|||	  Description
|||
|||	    This convenience macro allocates a block of memory using AllocMem(),
|||	    requesting size-tracking (that is, using the MEMTYPE_TRACKSIZE flag).
|||
|||	    You use FreeMemTrack() to free a block of memory that was allocated
|||	    by AllocMemTrack().
|||
|||	    Size-tracking saves you the work of tracking the allocation size.
|||	    Keep in mind that it slightly increases the memory overhead of the
|||	    allocation.
|||
|||	  Arguments
|||
|||	    memSize
|||	        The size of the memory block to allocate, in bytes.
|||
|||	  Return Value
|||
|||	    Returns a pointer to the memory block that was allocated or
|||	    NULL if there was not enough memory available.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/mem.h> V27.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>
|||
|||	  Notes
|||
|||	    You can enable memory debugging in your application by compiling
|||	    your entire project with the MEMDEBUG value defined on the
|||	    compiler's command line. Refer to the CreateMemDebug() function for
|||	    more details.
|||
|||	  See Also
|||
|||	    AllocMemTrackWithOptions(), FreeMemTrack(), AllocMem(),
|||	    AllocMemMasked(), GetMemTrackSize()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name AllocMemTrackWithOptions
|||	Allocates a block of memory with size-tracking and other options.
|||
|||	  Synopsis
|||
|||	    void *AllocMemTrackWithOptions(int32 memSize, uint32 memFlags);
|||
|||	  Description
|||
|||	    This convenience macro allocates a block of memory using AllocMem(),
|||	    requesting size-tracking (that is, MEMTYPE_TRACKSIZE) and allowing you
|||	    to supply other memFlags (e.g. MEMTYPE_FILL).
|||
|||	    You use FreeMemTrack() to free a block of memory that was allocated
|||	    with AllocMemTrackWithOptions().
|||
|||	    Size-tracking saves you the work of tracking the allocation's size.
|||	    Keep in mind that it slightly increases the memory overhead of the
|||	    allocation.
|||
|||	  Arguments
|||
|||	    memSize
|||	        The size of the memory block to allocate, in bytes.
|||
|||	    memFlags
|||	        Flags that specify some options for this memory allocation.
|||
|||	  Flags
|||
|||	    The following flags specify some options concerning the allocation:
|||
|||	    MEMTYPE_FILL
|||	        Sets every byte in the memory block to the value of the lower
|||	        eight bits of the flags argument. If this bit is not set, the
|||	        previous contents of the memory block are not changed.
|||
|||	  Return Value
|||
|||	    Returns a pointer to the memory block that was allocated or
|||	    NULL if there was not enough memory available.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/mem.h> V27.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>
|||
|||	  Notes
|||
|||	    You can enable memory debugging in your application by compiling
|||	    your entire project with the MEMDEBUG value defined on the
|||	    compiler's command line. Refer to the CreateMemDebug() function for
|||	    more details.
|||
|||	  See Also
|||
|||	    AllocMemTrack(), FreeMemTrack(), AllocMem(),
|||	    AllocMemMasked(), GetMemTrackSize()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name AllocMemMasked
|||	Allocates a block of memory with specific bits set or cleared in
|||	its address.
|||
|||	  Synopsis
|||
|||	    void *AllocMemMasked(int32 memSize, uint32 memFlags,
|||	                         uint32 careBits, uint32 stateBits);
|||
|||	  Description
|||
|||	    This function allocates a block of memory for use by the current
|||	    task. You use FreeMem() to free a block of memory that was
|||	    allocated with AllocMemMasked().
|||
|||	    The difference between this function and AllocMem() is that you can
|||	    specify that certain bits be set or cleared in the address of
|||	    returned memory block. This function is useful to allocate memory
|||	    aligned to a particular boundary in memory. See the
|||	    AllocMemAligned() macro for more details on this.
|||
|||	    Although this function has a deceptively generic appearance, it is
|||	    in fact meant to support the M2 triangle engine. For best
|||	    performance, the TE wants the Z buffer allocated on alternate 4K
|||	    pages from the main frame buffer. That is, if the Z buffer starts
|||	    on an even numbered 4K page, the frame buffer should start on an
|||	    odd numbered 4K page.
|||
|||	    Using this function, you can allocate your frame buffer, and then
|||	    by passing the right bit masks to this function, you can allocate
|||	    the Z buffer on an alternate 4K boundary.
|||
|||	    If there is insufficient memory in a task's free memory list to
|||	    allocate the needed block, the kernel automatically attempts
|||	    to obtain more memory pages from the system page pool. These
|||	    pages become the property of the task and get added to its free
|||	    memory list. They remain the property of the task until they
|||	    are returned to the system page pool by calling ScavengeMem().
|||
|||	    When a task dies, any pages of memory it owns automatically get
|||	    returned to the system page pool.
|||
|||	    Memory allocated by a thread is owned by the parent task, and not
|||	    by the thread itself. Therefore, when a thread dies, memory it
|||	    allocated remains allocated until it is explicitly freed by the
|||	    parent task (or another thread in the same task family), or if the
|||	    parent task dies.
|||
|||	  Arguments
|||
|||	    memSize
|||	        The size of the memory block to allocate, in bytes.
|||
|||	    memFlags
|||	        Flags that specify some options for this memory allocation.
|||
|||	    careBits
|||	        Indicates which bits of the returned address matter to you.
|||	        Bits set to 1 mean that you care about those bits, while bits
|||	        set to 0 indicate you don't care what the value of these
|||	        bits are.
|||
|||	    stateBits
|||	        For those bits you care about, this indicates what their state
|||	        should be. So for example, if you want the returned pointer to
|||	        be allocated on a 4K boundary, it means you want the pointer
|||	        returned to have the bottom 12 bits clear. You therefore would
|||	        pass careBits of 0x00000fff and stateBits of 0.
|||
|||	  Flags
|||
|||	    These are the possible values for the memFlags argument.
|||
|||	    MEMTYPE_NORMAL
|||	        Allocate standard memory. This flag must currently always be
|||	        supplied when allocating memory.
|||
|||	    The following flags specify some options concerning the allocation:
|||
|||	    MEMTYPE_FILL
|||	        Sets every byte in the memory block to the value of the lower
|||	        eight bits of the flags argument. If this bit is not set, the
|||	        previous contents of the memory block are not changed.
|||
|||	    MEMTYPE_TRACKSIZE
|||	        Tells the kernel to track the size of this memory allocation.
|||	        When you allocate memory with this flag, you must specify
|||	        TRACKED_SIZE as the size when freeing the memory using FreeMem().
|||	        This flag saves you the work of tracking the allocation's size.
|||	        Keep in mind that it slightly increases the memory overhead of
|||	        the allocation.
|||
|||	  Return Value
|||
|||	    Returns a pointer to the memory block that was allocated or
|||	    NULL if there was not enough memory available.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  Notes
|||
|||	    You can enable memory debugging in your application by compiling
|||	    your entire project with the MEMDEBUG value defined on the
|||	    compiler's command line. Refer to the CreateMemDebug() function for
|||	    more details.
|||
|||	  See Also
|||
|||	    AllocMem(), FreeMem(), GetMemTrackSize()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name AllocMemAligned
|||	Allocates a block of memory aligned to a particular boundary.
|||
|||	  Synopsis
|||
|||	    void *AllocMemAligned(int32 memSize, uint32 memFlags,
|||	                          uint32 alignment);
|||
|||	  Description
|||
|||	    This macro allocates a block of memory for use by the current
|||	    task. You use FreeMem() to free a block of memory that was
|||	    allocated with AllocMemAligned().
|||
|||	    The difference between this macro and AllocMem() is that you can
|||	    specify a particular alignment for the memory block. For example,
|||	    you can allocate a block of memory which is aligned on a 4K
|||	    boundary. This capability is very useful when interfacing to
|||	    many kinds of hardware devices which have particular alignment
|||	    needs. You can also use the ALLOC_ROUND() macro in <kernel/mem.h>
|||	    in order to round up the allocation size to a particular multiple.
|||
|||	    If there is insufficient memory in a task's free memory list to
|||	    allocate the needed block, the kernel automatically attempts
|||	    to obtain more memory pages from the system page pool. These
|||	    pages become the property of the task and get added to its free
|||	    memory list. They remain the property of the task until they
|||	    are returned to the system page pool by calling ScavengeMem().
|||
|||	    When a task dies, any pages of memory it owns automatically get
|||	    returned to the system page pool.
|||
|||	    Memory allocated by a thread is owned by the parent task, and not
|||	    by the thread itself. Therefore, when a thread dies, memory it
|||	    allocated remains allocated until it is explicitly freed by the
|||	    parent task (or another thread in the same task family), or if the
|||	    parent task dies.
|||
|||	  Arguments
|||
|||	    memSize
|||	        The size of the memory block to allocate, in bytes.
|||
|||	    memFlags
|||	        Flags that specify some options for this memory allocation.
|||
|||	    alignment
|||	        The alignment requirement. The address of the returned memory
|||	        block will be aligned on a multiple of this value. This
|||	        parameter must be a power of 2 and at be larger or equal to 8.
|||
|||	  Flags
|||
|||	    These are the possible values for the memFlags argument.
|||
|||	    MEMTYPE_NORMAL
|||	        Allocate standard memory. This flag must currently always be
|||	        supplied when allocating memory.
|||
|||	    The following flags specify some options concerning the allocation:
|||
|||	    MEMTYPE_FILL
|||	        Sets every byte in the memory block to the value of the lower
|||	        eight bits of the flags argument. If this bit is not set, the
|||	        previous contents of the memory block are not changed.
|||
|||	    MEMTYPE_TRACKSIZE
|||	        Tells the kernel to track the size of this memory allocation.
|||	        When you allocate memory with this flag, you must specify
|||	        TRACKED_SIZE as the size when freeing the memory using FreeMem().
|||	        This flag saves you the work of tracking the allocation's size.
|||	        Keep in mind that it slightly increases the memory overhead of
|||	        the allocation.
|||
|||	  Return Value
|||
|||	    Returns a pointer to the memory block that was allocated or
|||	    NULL if there was not enough memory available.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/mem.h> V27.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  Notes
|||
|||	    You can enable memory debugging in your application by compiling
|||	    your entire project with the MEMDEBUG value defined on the
|||	    compiler's command line. Refer to the CreateMemDebug() function for
|||	    more details.
|||
|||	  See Also
|||
|||	    AllocMem(), FreeMem(), GetMemTrackSize()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name ReallocMem
|||	Reallocates a block of memory to a different size.
|||
|||	  Synopsis
|||
|||	    void *ReallocMem(void *mem, int32 oldSize, int32 newSize,
|||	                     uint32 memFlags);
|||
|||	  Description
|||
|||	    This function reallocates a block of memory to a different size.
|||
|||	    When making a block smaller, the excess memory at the end of the
|||	    block is returned to the task's free memory list, where it becomes
|||	    available for future allocations.
|||
|||	    When making a block larger, an attempt is made to expand the
|||	    memory block in place. If this is not possible because the
|||	    region following the memory block has already been allocated,
|||	    then an attempt will be made to move the memory block to a
|||	    different sufficiently large area in memory. In such a case, this
|||	    function will run much slower since it will need to copy the
|||	    contents of the old memory area into the new area.
|||
|||	    NOTE: Making a block larger is not currently supported.
|||
|||	  Arguments
|||
|||	    mem
|||	        The memory block to reallocate. This value may be NULL, in which
|||	        case this function just returns NULL.
|||
|||	    oldSize
|||	        The old size of the memory block, in bytes. This must be the
|||	        same size that was specified when the block was allocated. If
|||	        the memory block was allocated using MEMTYPE_TRACKSIZE,
|||	        this argument should be set to TRACKED_SIZE.
|||
|||	    newSize
|||	        The new size of the block of memory.
|||
|||	    memFlags
|||	        Flags that specify some options for this memory allocation.
|||
|||	  Flags
|||
|||	    These are the possible values for the memFlags argument.
|||
|||	    MEMTYPE_NORMAL
|||	        Allocate standard memory. This flag must currently always be
|||	        supplied when reallocating memory.
|||
|||	    The following flags specify some options concerning the allocation:
|||
|||	    MEMTYPE_FILL
|||	        Sets every new byte in the memory block to the value of the
|||	        lower eight bits of the flags argument. If this bit is not set,
|||	        the previous contents of the new extent of the memory block are
|||	        not changed. This flag has no effect when making a memory block
|||	        smaller.
|||
|||	    MEMTYPE_TRACKSIZE
|||	        Tells the kernel to track the size of this memory allocation.
|||	        When you allocate memory with this flag, you must specify
|||	        TRACKED_SIZE as the size when freeing the memory using FreeMem().
|||	        This flag saves you the work of tracking the allocation's size.
|||	        Keep in mind that it slightly increases the memory overhead of
|||	        the allocation.
|||
|||	  Return Value
|||
|||	    Returns a pointer to the new memory block or NULL if there was not
|||	    enough memory available. This pointer will often, but not always,
|||	    be equal to the memory block pointer supplied to the function.
|||
|||	    When this function returns NULL, the original block of memory
|||	    is still allocated.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Caveats
|||
|||	    This function currently only allows a block of memory to shrink.
|||	    Attempting to make a block bigger will always return NULL.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  Notes
|||
|||	    You can enable memory debugging in your application by compiling
|||	    your entire project with the MEMDEBUG value defined on the
|||	    compiler's command line. Refer to the CreateMemDebug() function for
|||	    more details.
|||
|||	  See Also
|||
|||	    AllocMem(), FreeMem(), GetMemTrackSize()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name FreeMem
|||	Frees memory that was allocated with AllocMem() or AllocMemAligned().
|||
|||	  Synopsis
|||
|||	    void FreeMem(void *mem, int32 memSize);
|||
|||	  Description
|||
|||	    This function frees memory that was previously allocated by a call
|||	    to AllocMem(). The size argument specifies the number of bytes to
|||	    free.
|||
|||	    The memory is added to the list of available memory for the current
|||	    task. The pages of RAM containing the block of memory being freed
|||	    are not returned to the system page pool and remain the property
|||	    of the current task. You must call ScavengeMem() in order to return
|||	    these pages.
|||
|||	  Arguments
|||
|||	    mem
|||	        The memory block to free. This value may be NULL, in which case
|||	        this function just returns.
|||
|||	    memSize
|||	        The size of the block to free, in bytes. This must be the same
|||	        size that was specified when the block was allocated. If
|||	        the memory being freed was allocated using MEMTYPE_TRACKSIZE,
|||	        this argument should be set to TRACKED_SIZE.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  Notes
|||
|||	    You can enable memory debugging in your application by compiling
|||	    your entire project with the MEMDEBUG value defined on the
|||	    compiler's command line. Refer to the CreateMemDebug() function for
|||	    more details.
|||
|||	  See Also
|||
|||	    AllocMem(), ScavengeMem()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name FreeMemTrack
|||	Frees memory that was allocated with AllocMemTrack() or AllocMemTrackWithOptions().
|||
|||	  Synopsis
|||
|||	    void FreeMemTrack(void *mem);
|||
|||	  Description
|||
|||	    This convenience macro frees memory (via FreeMem()) that was previously
|||	    allocated by a call to AllocMemTrack() or AllocMemTrackWithOptions().
|||	    The system is tracking this memory block's size for your convenience,
|||	    so FreeMemTrack() doesn't need a memSize argument.
|||
|||	    The memory is added to the list of available memory for the current
|||	    task. The pages of RAM containing the block of memory being freed
|||	    are not returned to the system page pool and remain the property
|||	    of the current task. You must call ScavengeMem() in order to return
|||	    these pages.
|||
|||	  Arguments
|||
|||	    mem
|||	        The memory block to free. This value may be NULL, in which case
|||	        this function just returns.
|||
|||	  Implementation
|||
|||	     Macro implemented in <kernel/mem.h> V27.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>
|||
|||	  Notes
|||
|||	    You can enable memory debugging in your application by compiling
|||	    your entire project with the MEMDEBUG value defined on the
|||	    compiler's command line. Refer to the CreateMemDebug() function for
|||	    more details.
|||
|||	  See Also
|||
|||	    AllocMemTrack(), AllocMemTrackWithOptions(), AllocMem(), FreeMem(),
|||	    ScavengeMem()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name ScavengeMem
|||	Returns the current task's unused memory pages to the system page pool.
|||
|||	  Synopsis
|||
|||	    int32 ScavengeMem(void);
|||
|||	  Description
|||
|||	    This function finds pages of memory in the current task's free
|||	    memory list that are currently unused and returns them to the
|||	    system page pool, where they can be reallocated for other uses.
|||
|||	  Return Value
|||
|||	    Returns the amount of memory that was freed to the system page
|||	    pool, in bytes.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  See Also
|||
|||	    AllocMem(), FreeMem()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name GetPageSize
|||	Gets the size in bytes of a page of memory.
|||
|||	  Synopsis
|||
|||	    int32 GetPageSize(uint32 memFlags);
|||
|||	  Description
|||
|||	    This function gets the number of bytes in a page of memory.
|||
|||	  Arguments
|||
|||	    memFlags
|||	        The type of memory to inquire about. This value must currently
|||	        always be MEMTYPE_NORMAL.
|||
|||	  Return Value
|||
|||	    Returns the number of bytes in a page of memory, or a negative
|||	    error code if some illegal flags were specified.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name GetMemInfo
|||	Gets information about available memory.
|||
|||	  Synopsis
|||
|||	    void GetMemInfo(MemInfo *minfo, uint32 infoSize, uint32 memFlags);
|||
|||	  Description
|||
|||	    This function returns information about the amount of memory that
|||	    is currently available. The information about available memory is
|||	    returned in a MemInfo structure.
|||
|||	    The MemInfo structure contains the following fields:
|||
|||	    minfo_TaskAllocatedPages
|||	        Specifies the number of pages the current task owns.
|||
|||	    minfo_TaskAllocatedBytes
|||	        Specifies the number of bytes allocated by the current task.
|||
|||	    minfo_FreePages
|||	        Specifies the number of unallocated pages in the system.
|||
|||	    minfo_LargestFreePageSpan
|||	        Specifies the largest number of contiguous free pages in the
|||	        system. This value is returned in number of bytes.
|||
|||	    minfo_SystemAllocatedPages
|||	        Specifies the number of pages allocated by the system to store
|||	        items and other system-private allocations.
|||
|||	    minfo_SystemAllocatedBytes
|||	        Specifies the number of bytes allocated by the system to store
|||	        items and other system-private allocations.
|||
|||	    minfo_OtherAllocatedPages
|||	        Specifies the number of pages allocated by tasks other
|||	        than the current task.
|||
|||	  Arguments
|||
|||	    minfo
|||	        A pointer to a MemInfo structure where the information will
|||	        be stored.
|||
|||	    infoSize
|||	        The size in bytes of the MemInfo structure.
|||
|||	    memFlags
|||	        This must currently always be MEMTYPE_NORMAL.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  Caveats
|||
|||	    The information returned by GetMemInfo() is inherently flawed,
|||	    since you are existing in a multitasking environment. Memory can be
|||	    allocated or freed asynchronous to the operation of the task
|||	    calling GetMemInfo().
|||
|||	  See Also
|||
|||	    AllocMem(), FreeMem(), ScavengeMem()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name IsMemWritable
|||	Determines whether a region of memory is fully writable by the current
|||	task.
|||
|||	  Synopsis
|||
|||	    bool IsMemWritable(const void *mem, int32 memSize);
|||
|||	  Description
|||
|||	    This function considers the described block of the address space in
|||	    relation to the pages of memory the current task has write access
|||	    to. This function returns TRUE If the current task can write to the
|||	    memory block, and FALSE if it cannot.
|||
|||	  Arguments
|||
|||	    mem
|||	        A pointer to the start of the block.
|||
|||	    memSize
|||	        The number of bytes in the block.
|||
|||	  Return Value
|||
|||	    Returns TRUE if the block can be written to by the
|||	    calling task, and FALSE if it cannot.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V24.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  See Also
|||
|||	    IsMemReadable(), IsMemOwned()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name IsMemOwned
|||	Determines whether a region of memory is owned by the current task.
|||
|||	  Synopsis
|||
|||	    bool IsMemOwned(const void *mem, int32 memSize);
|||
|||	  Description
|||
|||	    This function considers the described block of the address space in
|||	    relation to the pages of memory the current task owns.
|||	    This function returns TRUE If the current task owns all pages
|||	    in the specified memory block, and FALSE if it doesn't.
|||
|||	  Arguments
|||
|||	    mem
|||	        A pointer to the start of the block.
|||
|||	    memSize
|||	        The number of bytes in the block.
|||
|||	  Return Value
|||
|||	    Returns TRUE if the block is entirely owned by the calling task,
|||	    and FALSE if it cannot.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V24.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  See Also
|||
|||	    IsMemReadable(), IsMemWritable()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name IsMemReadable
|||	Determines whether a region of memory is fully readable by the current
|||	task.
|||
|||	  Synopsis
|||
|||	    bool IsMemReadable(const void *mem, int32 memSize);
|||
|||	  Description
|||
|||	    This function considers the described block of the address space in
|||	    relation to the known locations of RAM in the system, and returns
|||	    TRUE if the block is entirely contained within RAM accessible by
|||	    the current task, and FALSE otherwise.
|||
|||	  Arguments
|||
|||	    mem
|||	        A pointer to the start of the block.
|||
|||	    memSize
|||	        The number of bytes in the block.
|||
|||	  Return Value
|||
|||	    Returns TRUE if the block is entirely readable by the
|||	    current task, or FALSE if any part of the block is not.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V24.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  See Also
|||
|||	    IsMemWritable(), IsMemOwned()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name ControlMem
|||	Controls memory permissions and ownership.
|||
|||	  Synopsis
|||
|||	    Err ControlMem(void *mem, int32 memSize, int32 cmd, Item task);
|||
|||	  Description
|||
|||	    When a task allocates memory, it becomes the owner of that memory.
|||	    Other tasks cannot write to the memory unless they are given
|||	    permission by its owner. A task can give another task permission to
|||	    write to one or more of its memory pages, revoke write permission
|||	    that was previously granted, or transfer ownership of memory to
|||	    another task or the system by calling ControlMem(). Using
|||	    ControlMem() a task can also make a memory range persistent or
|||	    retain its contents across system reboot.
|||
|||	    Each page of memory has a control status that specifies which task
|||	    owns the memory, which tasks can write to it, and if the memory
|||	    is persistent. Calls to ControlMem() change the control status
|||	    for entire pages. If the p and size arguments (which specify the
|||	    the memory to change) specify any part of a page, the changes apply
|||	    to the entire page.
|||
|||	    A task can grant write permission for pages that it owns
|||	    to any number of tasks. To accomplish this, the task must make a
|||	    separate call to this function for each task that is to be granted
|||	    write permission.
|||
|||	    A task that calls ControlMem() must own the memory whose control
|||	    status it is changing, with one exception: A task that has write
|||	    access to memory it doesn't own can relinquish its write access by
|||	    using MEMC_NOWRITE as the value of the cmd argument. If a task
|||	    transfers ownership of memory, it still retains write access.
|||
|||	    A task can use ControlMem() to prevent itself from writing to
|||	    memory it owns. This may be useful during debugging to prevent one
|||	    section of your code from stomping on the data for another section
|||	    of the code.
|||
|||	    A task can use ControlMem() to return ownership of memory pages to
|||	    the system, thereby returning them to the system page pool. You
|||	    You can do this by using 0 as the value of the task argument.
|||
|||	    A task can use ControlMem() to request memory pages to be marked
|||	    persistent across reboot of an application. When the pages
|||	    are marked persistent, they are returned to the system but the
|||	    caller retains his current write privelege on those pages. After
|||	    a system reset, if the same application that marked memory pages
|||	    persistent (before the reboot) is run, the persistent memory pages
|||	    are not allocated to any task. The value of the task argument must
|||	    be 0 for this call. Currently, only one memory range can be marked
|||	    as persistent.
|||
|||	  Arguments
|||
|||	    mem
|||	        A pointer to the memory whose control status to change.
|||
|||	    memSize
|||	        The amount of memory for which to change the control status,
|||	        in bytes. If the memSize and mem arguments specify any part of
|||	        a page, the control status is changed for the entire page.
|||
|||	    cmd
|||	        A constant that specifies the change to be made to the control
|||	        status; possible values are listed below.
|||
|||	    task
|||	        The item number of the task for which to change the control
|||	        status or 0 for global changes.
|||
|||	    The possible values of "cmd" are:
|||
|||	    MEMC_OKWRITE
|||	        Grants permission to write to this memory to the task specified
|||	        by the task argument, or to all tasks if the task argument is
|||	        0. When granting write permission to all tasks, it essentially
|||	        makes the memory completely unprotected. To undo the effect of
|||	        doing this, you must call ControlMem() with MEMC_NOWRITE and a
|||	        task of 0, which removes write access for all tasks, and then
|||	        call ControlMem() with MEMC_OKWRITE and the current task's item
|||	        to allow the current task to write to the memory.
|||
|||	    MEMC_NOWRITE
|||	        Revokes permission to write to this memory from the task
|||	        specified by the task argument. If task is 0, revokes write
|||	        permission for all tasks including the current task.
|||
|||	    MEMC_GIVE
|||	        If the calling task is the owner of the memory, this transfers
|||	        ownership of the memory to the task specified by the task
|||	        argument. If task is 0, it gives the memory back to the system
|||	        page pool.
|||
|||	    MEMC_PERSISTENT
|||	        If the calling task is the owner of the memory, this transfers
|||	        ownership of the memory to the system and marks the memory
|||	        as persistent across reboot of the application.
|||
|||	  Return Value
|||
|||	    Returns >= 0 if the change was successful or a a negative error
|||	    code for failure. Possible error codes currently include:
|||
|||	    BADITEM
|||	        The task argument does not specify a current task or
|||	        is not 0 for MEMC_PERSISTENT cmd.
|||
|||	    ER_Kr_BadMemCmd
|||	        The cmd argument is not one of the valid values.
|||
|||	    ER_BadPtr
|||	        The mem argument is not a valid pointer to memory.
|||
|||	    NOSUPPORT
|||	        The current task is attempting to set more than one
|||	        persistent memory range.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  See Also
|||
|||	    ScavengeMem()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name GetPersistentMem
|||	Gets access to a memory block allocated prior to the previous reboot.
|||
|||	  Synopsis
|||
|||	    Err GetPersistentMem(PersistentMemInfo *info, uint32 infoSize);
|||
|||	  Description
|||
|||	    This function lets you get control of a persistent memory area
|||	    that was dedicated by a previous call to ControlMem(). The memory
|||	    dedicated with ControlMem()'s MEMC_PERSISTENT option will survive
|||	    one reboot of the system. This lets you squirl some data away
|||	    into this permanent memory, ask the user to change to a
|||	    different CD, which relaunches the OS and reruns your title. You
|||	    then call GetPersistentMem() to locate the permanent allocation and
|||	    retrieve your data.
|||
|||	  Arguments
|||
|||	    info
|||	        A pointer to a PersistentMemInfo structure which will be
|||	        initiailized to point to the persistent memory.
|||
|||	    infoSize
|||	        Set to sizeof(PersistentMemInfo).
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    Possible failure codes currently include:
|||
|||	    NOPERSISTENTMEM
|||	        No persistent memory is available. This means that either
|||	        the application didn't run before, or didn't get to save
|||	        its state the previous time it ran.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V32.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>
|||
|||	  See Also
|||
|||	    ControlMem()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name AllocMemPages
|||	Allocates whole pages of memory.
|||
|||	  Synopsis
|||
|||	    void *AllocMemPages(int32 memSize, uint32 memFlags);
|||
|||	  Description
|||
|||	    This function allocates pages of memory directly from the system's
|||	    free page pool, bypassing the current task's free memory list.
|||	    This call is seldom used by client code, and is meant mostly as a
|||	    support function to higher-level memory managers.
|||
|||	    When a task dies, any pages of memory it owns automatically get
|||	    returned to the system page pool.
|||
|||	    Memory allocated by a thread is owned by the parent task, and not
|||	    by the thread itself. Therefore, when a thread dies, memory it
|||	    allocated remains allocated until it is explicitly freed by the
|||	    parent task (or another thread in the same task family), or if the
|||	    parent task dies.
|||
|||	  Arguments
|||
|||	    memSize
|||	        The size of the memory block to allocate, in bytes.
|||
|||	    memFlags
|||	        Flags that specify some options for this memory allocation.
|||
|||	  Flags
|||
|||	    These are the possible values for the memFlags argument.
|||
|||	    MEMTYPE_NORMAL
|||	        Allocate standard memory. This flag must currently always be
|||	        supplied when allocating memory.
|||
|||	    The following flags specify some options concerning the allocation:
|||
|||	    MEMTYPE_FILL
|||	        Sets every byte in the memory block to the value of the lower
|||	        eight bits of the flags argument. If this bit is not set, the
|||	        previous contents of the memory block are not changed.
|||
|||	  Return Value
|||
|||	    Returns a pointer to the memory block that was allocated or
|||	    NULL if there was not enough memory available.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  Notes
|||
|||	    You can enable memory debugging in your application by compiling
|||	    your entire project with the MEMDEBUG value defined on the
|||	    compiler's command line. Refer to the CreateMemDebug() function for
|||	    more details.
|||
|||	  See Also
|||
|||	    FreeMemPages(), ControlMem()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name FreeMemPages
|||	Frees memory that was allocated with AllocMemPages().
|||
|||	  Synopsis
|||
|||	    void FreeMemPages(void *mem, int32 memSize);
|||
|||	  Description
|||
|||	    This function frees memory that was previously allocated by a call
|||	    to AllocMemPages(). The size argument specifies the number of bytes
|||	    to free.
|||
|||	    The memory is immediately returned to the system's free page pool,
|||	    where it becomes available for reallocation by another task or
|||	    by the system.
|||
|||	  Arguments
|||
|||	    mem
|||	        The memory block to free. This value may be NULL, in which case
|||	        this function just returns.
|||
|||	    memSize
|||	        The size of the block to free, in bytes. This must be the same
|||	        size that was specified when the block was allocated.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  Notes
|||
|||	    You can enable memory debugging in your application by compiling
|||	    your entire project with the MEMDEBUG value defined on the
|||	    compiler's command line. Refer to the CreateMemDebug() function for
|||	    more details.
|||
|||	  See Also
|||
|||	    AllocMemPages()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name CreateMemDebug
|||	Initializes MemDebug, the Portfolio memory debugging package.
|||
|||	  Synopsis
|||
|||	    Err CreateMemDebug(const TagArg *tags);
|||
|||	  Description
|||
|||	    This function creates the needed data structures and initializes
|||	    them as needed for MemDebug, the system memory debugging package.
|||
|||	    MemDebug provides a general-purpose mechanism to track and validate
|||	    all memory allocations done in the system. Using MemDebug, you can
|||	    easily determine where memory leaks occur within a program, and
|||	    find illegal uses of the memory subsystem.
|||
|||	    To enable memory debugging in a program, do the following:
|||
|||	    * Add a call to CreateMemDebug() as the first statement in the
|||	      main() routine of your program.
|||
|||	    * Add calls to DumpMemDebug() and DeleteMemDebug() as the last
|||	      statements in the main() routine of your program.
|||
|||	    * Recompile your entire project with MEMDEBUG defined on the
|||	      compiler's command-line (done by using -DMEMDEBUG)
|||
|||	    With these steps taken, all memory allocations done by your program
|||	    will be tracked, and specially managed. On exiting your program, any
|||	    memory left allocated will be displayed to the debugging terminal,
|||	    along with the line number and source file where the memory was
|||	    allocated from.
|||
|||	    In addition, MemDebug makes sure that illegal or dangerous uses of
|||	    memory are detected and flagged. Most messages generated by the
|||	    package indicate the offending source file and line within your
|||	    source code where the problem originated.
|||
|||	    When all options are turned on, MemDebug will check and report the
|||	    following problems:
|||
|||	    * memory allocations with a size <= 0
|||
|||	    * memory free with a bogus memory pointer
|||
|||	    * memory free with a size not matching the size used when the memory
|||	      was allocated
|||
|||	    * cookies on either side of all memory allocations are checked to
|||	      make sure they are not altered from the time a memory allocation
|||	      is made to the time the memory is released. This would indicate
|||	      that something is writing beyond the bounds of allocated memory.
|||
|||	    When source code is recompiled with MEMDEBUG defined, then all
|||	    memory allocation and deallocation calls are vectored through
|||	    special versions of these routines which track the source file and
|||	    line number where the calls are made from. If memory is allocated
|||	    from code not recompiled with MEMDEBUG (say if a folio allocates
|||	    memory on your behalf), then the source file and line information
|||	    is not available.
|||
|||	    By calling the DumpMemDebug() function at any time within your
|||	    program, you can get a detailed listing of all memory currently
|||	    allocated, showing from which source line and source file the
|||	    allocation occurred.
|||
|||	  Arguments
|||
|||	    tags
|||	        This is reserved for future use and should currently always be
|||	        NULL.
|||
|||	  Return Value
|||
|||	    Returns >= 0 if successful or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  Notes
|||
|||	    You should make sure to turn off memory debugging prior to creating
|||	    the final version of your program. Enabling memory debugging incurs
|||	    an overhead of currently 32 bytes per allocation made. If you use
|||	    the MEMDEBUGF_PAD_COOKIES option, this overhead grows to 64 bytes
|||	    per allocation.
|||
|||	    In addition, specifying the MEMDEBUGF_ALLOC_PATTERNS and
|||	    MEMDEBUGF_FREE_PATTERNS options will slow down memory allocations
|||	    and free operations, due to the extra work of filling the memory
|||	    with the patterns.
|||
|||	    When reporting errors to the debugging terminal, the memory
|||	    debugging subsystem will normally print the source file and line
|||	    number where the error occurred. When using link libraries which
|||	    have not been recompiled with MEMDEBUG defined, the memory debugging
|||	    subsystem will still be able to track the allocations, but will not
|||	    report the source file or line number where the error occurred. It
|||	    will report < unknown source file > instead.
|||
|||	  See Also
|||
|||	    ControlMemDebug(), DeleteMemDebug(), DumpMemDebug(),
|||	    SanityCheckMemDebug()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name ControlMemDebug
|||	Controls what MemDebug does and doesn't do.
|||
|||	  Synopsis
|||
|||	    Err ControlMemDebug(uint32 controlFlags);
|||
|||	  Description
|||
|||	    This function lets you control various options that determine
|||	    what MemDebug does.
|||
|||	  Arguments
|||
|||	    controlFlags
|||	        A set of bit flags controlling various MemDebug options. See
|||	        below.
|||
|||	  Flags
|||
|||	    The control flags can be any of:
|||
|||	    MEMDEBUGF_ALLOC_PATTERNS
|||	        When this flag is set, it instructs MemDebug to fill newly
|||	        allocated memory with the constant MEMDEBUG_ALLOC_PATTERN.
|||	        Doing so will likely cause your program to fail in some way if
|||	        it tries to read newly allocated memory without first
|||	        initializing it. Note that this option has no effect if memory
|||	        is allocated using the MEMTYPE_FILL memory flag.
|||
|||	    MEMDEBUGF_FREE_PATTERNS
|||	        When this flag is set, it instructs MemDebug to fill memory that
|||	        is being freed with the constant MEMDEBUG_FREE_PATTERN. Doing so
|||	        will likely cause your program to fail in some way if it tries
|||	        to read memory that has been freed.
|||
|||	    MEMDEBUGF_PAD_COOKIES
|||	        When this flag is set, it causes MemDebug to put special memory
|||	        cookies in the 16 bytes before and 16 bytes after every block
|||	        of memory allocated. When a memory block is freed, the cookies
|||	        are checked to make sure that they have not been altered. This
|||	        option makes sure that your program is not writing outside the
|||	        bounds of memory it allocates. This option requires an extra
|||	        overhead of 32 bytes per allocation.
|||
|||	    MEMDEBUGF_BREAKPOINT_ON_ERRORS
|||	        When this flag is set, MemDebug automatically invokes the
|||	        debugger if an error is detected. Errors include such things as
|||	        mangled pad cookies, incorrect size for a FreeMem() call, etc.
|||	        Normally, MemDebug simply prints out the error to the debugging
|||	        terminal and keeps executing.
|||
|||	    MEMDEBUGF_CHECK_ALLOC_FAILURES
|||	        When this flag is set, MemDebug emits a message when a memory
|||	        allocation call fails due to lack of memory. This is useful to
|||	        track down where in a program memory is running out.
|||
|||	    MEMDEBUGF_KEEP_TASK_DATA
|||	        MemDebug maintains some task-specific statistics about memory
|||	        allocations performed by that task. This information gets
|||	        displayed by DumpMemDebug(). Whenever all of the memory
|||	        allocated by a thread is freed, or when a task dies, the data
|||	        structure holding the statistics for that task automatically
|||	        gets freed by MemDebug. This is undesirable if you wish to
|||	        dump out statistics of the code just before a program exits.
|||	        Setting this flag causes the data structure not to be freed,
|||	        making the statistical information available to DumpMemDebug().
|||
|||	  Return Value
|||
|||	    Returns >= 0 if successful or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateMemDebug(), DeleteMemDebug(), DumpMemDebug(),
|||	    SanityCheckMemDebug()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name RationMemDebug
|||	Rations memory allocations to test failure paths.
|||
|||	  Synopsis
|||
|||	    Err RationMemDebug(const TagArg *tags);
|||
|||	    Err RationMemDebugVA(uint32 tag, ...);
|||
|||	  Description
|||
|||	    This function lets you cause selected memory allocations to fail,
|||	    allowing various failure paths to be tested.
|||
|||	    The many tags supported let you tailor when and under which
|||	    conditions memory allocations are to fail.
|||
|||	  Arguments
|||
|||	    tags
|||	        A pointer to an array of tag arguments containing extra data
|||	        for this function. See below for a description of the tags
|||	        supported.
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function. The array must be terminated with TAG_END.
|||
|||	    RATIONMEMDEBUG_TAG_ACTIVE (bool)
|||	        This tag lets you control whether memory rationing is turned
|||	        on. Passing TRUE turns rationing on, while passing FALSE turns
|||	        it off.
|||
|||	    RATIONMEMDEBUG_TAG_TASK (Item)
|||	        Specifies that rationing is to only occur for a specific task.
|||	        You supply the item number of the task to this call. If you
|||	        pass a task item of 0, it means that all tasks should be
|||	        rationed.
|||
|||	    RATIONMEMDEBUG_TAG_MINSIZE (uint32)
|||	        Lets you specify the minimum size of allocations to ration.
|||	        Allocations smaller than this size will never be rationed.
|||
|||	    RATIONMEMDEBUG_TAG_MAXSIZE (uint32)
|||	        Lets you specify the maximum size of allocations to ration.
|||	        Allocations larger than this size will never be rationed.
|||
|||	    RATIONMEMDEBUG_TAG_COUNTDOWN (uint32)
|||	        Specifies a countdown of allocations before rationing should
|||	        start. Rationing will be disabled until that many allocations
|||	        are performed.
|||
|||	    RATIONMEMDEBUG_TAG_INTERVAL (uint32)
|||	        Specifies the period over which the rationing occurs. Only
|||	        a single allocation per interval is rationed.
|||
|||	    RATIONMEMDEBUG_TAG_RANDOM (bool)
|||	        Specifies that within the rationing interval, a random
|||	        allocation should fail. If random mode is turned off, the
|||	        sequencing of rationed allocations will be consistent.
|||
|||	    RATIONMEMDEBUG_TAG_VERBOSE (bool)
|||	        When set to TRUE, specifies that when a rationing occurs,
|||	        information should be printed about the allocation that was
|||	        denied.
|||
|||	    RATIONMEMDEBUG_TAG_BREAKPOINT_ON_RATIONING (bool)
|||	        When set to TRUE, specifies that a debugger breakpoint should
|||	        be triggered whenever rationing occurs.
|||
|||	    RATIONMEMDEBUG_TAG_SUPER (bool)
|||	        When set to TRUE, specifies that supervisor allocations should
|||	        be rationed just like normal user allocations. This will make
|||	        things like item allocations start to fail.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V30.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>
|||
|||	  See Also
|||
|||	    ControlMemDebug(), CreateMemDebug(), DeleteMemDebug()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name DeleteMemDebug
|||	Releases memory debugging resources.
|||
|||	  Synopsis
|||
|||	    Err DeleteMemDebug(void);
|||
|||	  Description
|||
|||	    Deletes any resources allocated by CreateMemDebug() and any
|||	    resources to perform memory debugging.
|||
|||	    This call is generally very risky to make if the
|||	    MEMDEBUGF_PAD_COOKIES option was being used. In such a case, it is
|||	    a good idea to reboot the system once the test is done running.
|||	    When testing a program, you can also simply exit your program
|||	    without turning off memory debugging. This will leave memory
|||	    debugging active, and will avoid any problems associated with
|||	    left over pad cookies,
|||
|||	  Return Value
|||
|||	    Returns >= 0 if successful, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateMemDebug(), ControlMemDebug(), DumpMemDebug(),
|||	    SanityCheckMemDebug()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name DumpMemDebug
|||	Dumps memory allocation debugging information.
|||
|||	  Synopsis
|||
|||	    Err DumpMemDebug(const TagArg *tags);
|||
|||	    Err DumpMemDebugVA(uint32 tag, ...);
|||
|||	  Description
|||
|||	    This function outputs a table showing all memory currently allocated
|||	    through the memory debugging code. This table shows the allocation
|||	    size, address, as well as the source file and the source line where
|||	    the allocation took place.
|||
|||	    This function also outputs statistics about general memory
|||	    allocation patterns. This includes the number of memory allocation
|||	    calls that have been performed, the maximum number of bytes
|||	    allocated at any one time, current amount of allocated memory, etc.
|||	    All this information is displayed on a per-thread basis, as well as
|||	    globally for all threads.
|||
|||	    To use this function, the memory debugging code must have been
|||	    previously initialized using CreateMemDebug().
|||
|||	  Arguments
|||
|||	    tags
|||	        A pointer to an array of tag arguments containing extra data
|||	        for this function. See below for a description of the tags
|||	        supported.
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function. The array must be terminated with TAG_END.
|||
|||	    DUMPMEMDEBUG_TAG_TASK (Item)
|||	        Controls which task's information should be displayed. You
|||	        can pass the item number of any task to display. Passing 0
|||	        prints the task information of all tasks. If this tag is not
|||	        supplied, the default is to only display the information for
|||	        the current task.
|||
|||	    DUMPMEMDEBUG_TAG_SUPER (bool)
|||	        When set to TRUE, causes information to be displayed about
|||	        allocations done in supervisor mode by the system.
|||
|||	  Return Value
|||
|||	    Returns >= 0 if successful, or a negative error code if not. Current
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateMemDebug(), ControlMemDebug(), DeleteMemDebug(),
|||	    SanityCheckMemDebug()
|||
**/

/**
|||	AUTODOC -class Kernel -group Memory -name SanityCheckMemDebug
|||	Checks all current memory allocations to make sure all the allocation
|||	cookies are intact
|||
|||	  Synopsis
|||
|||	    Err SanityCheckMemDebug(const char *banner, const TagArg *tags);
|||
|||	  Description
|||
|||	    This function checks all current memory allocations to see if any
|||	    of the memory cookies have been corrupted. This is useful when
|||	    trying to track down at which point in a program's execution memory
|||	    cookies are being trashed.
|||
|||	  Arguments
|||
|||	    banner
|||	        Descriptive text to print before any status message displayed.
|||	        May be NULL.
|||
|||	    tags
|||	        This is reserved for future use and should currently always be
|||	        NULL.
|||
|||	  Return Value
|||
|||	    Returns >= 0 if successful or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/mem.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateMemDebug(), ControlMemDebug(), DeleteMemDebug(),
|||	    DumpMemDebug()
|||
**/

/**
|||	AUTODOC -class Kernel -group Tasks -name ControlUserExceptions
|||	Controls which exceptions are to be captured and reported to a
|||	user-mode handler.
|||
|||	  Synopsis
|||
|||	    Err ControlUserExceptions(uint32 exceptions, bool captured);
|||
|||	  Description
|||
|||	    This function lets you specify which sets of exceptions should or
|||	    shouldn't be captured and reported to a exception handler that
|||	    was installed using RegisterUserExceptionHandler().
|||
|||	    Capturing floating-point exceptions changes the overall way
|||	    floating-point operations are handled by the CPU. If capturing of
|||	    a particular type of floating-point exception is not enabled, then
|||	    when a condition that would trigger one such exception occurs, the
|||	    CPU handles the exception itself, following the rules of IEEE
|||	    floating-point math.
|||
|||	    If a floating-point exception is being captured, the CPU will
|||	    trigger an exception when appropriate. If a user-mode handler
|||	    has been installed using RegisterUserExceptionHandler(), then the
|||	    handler will get notification that the exception occured, and
|||	    will get a chance to either correct the situation and resume the
|||	    normal flow of execution, or it can remove the task that triggered
|||	    the exception.
|||
|||	  Arguments
|||
|||	    exceptions
|||	        The set of exceptions to affect. The possible exceptions
|||	        are listed below.
|||
|||	    captured
|||	        When set to TRUE, causes the specified exceptions to be
|||	        captured. When set to FALSE, causes the specified exceptions
|||	        to not be captured.
|||
|||	  Exceptions
|||
|||	    The types of exceptions are listed in <kernel/task.h>. You OR
|||	    these values together to specify different types of exceptions
|||	    in the same call. The types of exceptions currently supported
|||	    are:
|||
|||	    USEREXC_TRAP
|||	        Triggers when a PowerPC "tw" instruction or one of its variants
|||	        is executed.
|||
|||	    USEREXC_FP_INVALID_OP
|||	        Triggers when an invalid floating-point operation is performed.
|||
|||	    USEREXC_FP_OVERFLOW
|||	        Triggers when a floating-point overflow occurs.
|||
|||	    USEREXC_FP_UNDERFLOW
|||	        Triggers when a floating-point underflow occurs.
|||
|||	    USEREXC_FP_ZERODIVIDE
|||	        Triggers when a floating-point divide by zero occurs.
|||
|||	    USEREXC_FP_INEXACT
|||	        Triggers when a floating-point inexact result is generated.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V31.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>
|||
|||	  See Also
|||
|||	    RegisterUserExceptionHandler(), CompleteUserException()
|||
**/

/**
|||	AUTODOC -class Kernel -group Tasks -name RegisterUserExceptionHandler
|||	Registers a message port to receive notifications when a target task
|||	or thread causes an exception.
|||
|||	  Synopsis
|||
|||	    Err RegisterUserExceptionHandler(Item task, Item port);
|||
|||	  Description
|||
|||	    As a task executes, it may cause exceptions to occur within the
|||	    CPU. Certain exceptions may cause the task to crash and be removed
|||	    from the system, while others may be silently ignored by the task.
|||
|||	    This function lets you register a message port that will receive
|||	    a notification message whenever an exception occurs within a given
|||	    task.
|||
|||	    The task that monitor the message port and receives notification
|||	    that another task has caused an exception is supplied a
|||	    UserExceptionContext structure which indicates which task has
|||	    caused the exception, what the exception is, and the current
|||	    CPU and FPU registers for the task.
|||
|||	    The exception handling task can print out useful information
|||	    about the task that triggered the exception, can simply delete
|||	    the task quietly, or can do more sophisticated fixup operations
|||	    in order to correct the condition that triggered the exception.
|||
|||	    When the exception handling task wishes to allow the task that
|||	    caused the exception to resume execution, it must call the
|||	    CompleteUserException() function. When you call this function, you
|||	    can supply pointers to a pair of structures that specify a new
|||	    set of CPU and FPU registers for the task. By reading the original
|||	    registers values from the UserExceptionContext structure and
|||	    supplying a modified set of registers to CompleteUserException(),
|||	    the exception handling task can change the running state of the
|||	    task that caused the exception.
|||
|||	  Arguments
|||
|||	    task
|||	        The item number of the task or thread for which exceptions
|||	        should be monitored.
|||
|||	    port
|||	        The item number of a message port where the kernel will post
|||	        a message if the specified task triggers an exception.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V31.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>
|||
|||	  See Also
|||
|||	    ControlUserExceptions(), CompleteUserException()
|||
**/

/**
|||	AUTODOC -class Kernel -group Tasks -name CompleteUserException
|||	Allows a task that triggered an exception to resume execution.
|||
|||	  Synopsis
|||
|||	    Err CompleteUserException(Item task,
|||	                              const RegBlock *rb,
|||	                              const FPRegBlock *fprb);
|||
|||	  Description
|||
|||	    After a handler task has been informed that another task has
|||	    triggered an exception, it can tell the system to resume execution
|||	    of the task by calling this function.
|||
|||	    You can supply a new set of registers for the task when it is
|||	    started. This lets the handler task adjust any values it wants in
|||	    the task's state in order to compensate for the exception that
|||	    occured.
|||
|||	    Note that when handling a floating-point exception, the FPSCR
|||	    register provided to the handler will have bits set which the
|||	    floating-point unit in the CPU sets in order to trigger an
|||	    exception. If you supply a replacement register set by passing a
|||	    non-NULL fprb parameter, you should take care to clear these bits,
|||	    otherwise the original exception will occur again. Refer to the
|||	    documentation on the PowerPC for information on which bits to
|||	    clear. If you don't supply a replacement set of registers, the
|||	    proper bits in the FPSCR are cleared for you by the kernel before
|||	    relaunching the task that got the exception.
|||
|||	  Arguments
|||
|||	    task
|||	        The item number of the task or thread for which exceptions
|||	        should be monitored.
|||
|||	    rb
|||	        Pointer to a RegBlock structure which holds the CPU registers
|||	        to use for the task being resumed. If this pointer is NULL,
|||	        the original task registers are used, except that the value
|||	        of rb_PC is incremented by 4 in order to skip over the
|||	        instruction that triggered the exception.
|||
|||	    fprb
|||	        Pointer to an FPRegBlock structure which holds the FPU
|||	        registers to use for the task being resume. If this pointer is
|||	        NULL, the original task registers are used, except that the
|||	        value of fprb_FPSCR is cleared of its exception bits.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V31.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>
|||
|||	  See Also
|||
|||	    RegisterUserExceptionHandler(), ControlUserExceptions()
|||
**/

/**
|||	AUTODOC -class Kernel -group MP -name IsMasterCPU
|||	Determines which CPU this is.
|||
|||	  Synopsis
|||
|||	    bool IsMasterCPU(void);
|||
|||	  Description
|||
|||	    This function returns TRUE if it is called by the master
|||	    CPU and FALSE if it is called by the slave.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V32.
|||
|||	  Associated Files
|||
|||	    <device/mp.h>
|||
|||	  See Also
|||
|||	    IsSlaveCPU()
|||
**/

/**
|||	AUTODOC -class Kernel -group MP -name IsSlaveCPU
|||	Determines which CPU this is.
|||
|||	  Synopsis
|||
|||	    bool IsSlaveCPU(void);
|||
|||	  Description
|||
|||	    This function returns TRUE if it is called by the slave
|||	    CPU and FALSE if it is called by the master.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V32.
|||
|||	  Associated Files
|||
|||	    <device/mp.h>
|||
|||	  See Also
|||
|||	    IsMasterCPU()
|||
**/

/**
|||	AUTODOC -class Kernel -group Messaging -name CreateBufferedMsg
|||	Creates a buffered message.
|||
|||	  Synopsis
|||
|||	    Item CreateBufferedMsg(const char *name, uint8 pri, Item msgPort,
|||	                           int32 dataSize);
|||
|||	  Description
|||
|||	    One of the ways tasks communicate is by sending messages to each
|||	    other. This function creates an item for a buffered message
|||	    (a message that includes an internal buffer for sending data to
|||	    the receiving task).
|||
|||	    The advantage of using a buffered message instead of a standard
|||	    message is that the sending task doesn't need to keep the data
|||	    block containing the message data after the message is sent. All
|||	    the necessary data is included in the message.
|||
|||	    The same message item can be resent any number of times. When you
|||	    are finished with a message item, use DeleteMsg() to delete it.
|||
|||	    You can use FindNamedItem() to find a message item by name.
|||
|||	  Arguments
|||
|||	    name
|||	        The optional name of the message.
|||
|||	    pri
|||	        The priority of the message. This determines the position of
|||	        the message in the receiving task's message queue and thus, how
|||	        soon it is likely to be handled. A larger number specifies a
|||	        higher priority.
|||
|||	    msgPort
|||	        The item number of the message port at which to receive the
|||	        reply, or 0 if no reply is expected.
|||
|||	    dataSize
|||	        The maximum size of the message's internal buffer, in bytes.
|||
|||	  Return Value
|||
|||	    Returns the item number of the message or a negative error code
|||	    for failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V20.
|||
|||	  Associated Files
|||
|||	    <kernel/msgport.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateMsg(), CreateMsgPort(), CreateSmallMsg(), DeleteMsg(),
|||	    DeleteMsgPort(), SendMsg()
|||
**/

/**
|||	AUTODOC -class Kernel -group Tasks -name CreateModuleThread
|||	Creates a thread from a loaded code module.
|||
|||	  Synopsis
|||
|||	    Item CreateModuleThread(Item module, const char *name,
|||	                            const TagArg *tags);
|||
|||	    Item CreateModuleThreadVA(Item module, const char *name,
|||	                              uint32 tags, ...);
|||
|||	  Description
|||
|||	    This function creates a thread. Threads provide a preemptively
|||	    scheduled execution context which has all of the characteristics
|||	    of a full-fledged task, except that they don't have their own
|||	    address space, and therefore share the one of their parent
|||
|||	    This function launches a loaded code module as a thread. You can
|||	    also launch a function within the current code module by using
|||	    CreateThread().
|||
|||	    The stack size and priority for the new thread are determined by
|||	    the values specified to the linker when the module was linked. If
|||	    no priority was given at link time, the thread will be launched
|||	    with the priority of the current task.
|||
|||	    When you no longer need a thread, use DeleteModuleThread() to
|||	    delete it. Alternatively, the thread can return or call exit().
|||
|||	  Arguments
|||
|||	    module
|||	        The loaded code module to execute as a thread, as obtained from
|||	        OpenModule()
|||
|||	    name
|||	        The name of the thread to be created. You can later use
|||	        FindTask() to find it by name.
|||
|||	    tags
|||	        A pointer to an array of optional tag arguments containing
|||	        extra data for this function, or NULL. See below for a
|||	        description of the tags supported.
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function. The array must be terminated with TAG_END.
|||
|||	    CREATETASK_TAG_ARGC (uint32)
|||	        A 32-bit value that will be passed to the thread being
|||	        launched as argc for its main() function. If this tag is
|||	        omitted, argc will be 0.
|||
|||	    CREATETASK_TAG_ARGP (uint32)
|||	        A 32-bit value that will be passed to the thread being
|||	        launched as argv for its main() function. If this tag is
|||	        omitted, argv will be NULL.
|||
|||	    CREATETASK_TAG_MSGFROMCHILD (Item)
|||	        Provides the item number of a message port.  The kernel will
|||	        send a status message to this port whenever the thread
|||	        being created exits. The message is sent by the kernel
|||	        after the thread has been deleted. The msg_Result field of the
|||	        message contains the exit status of the thread. This
|||	        is the value the thread provided to exit(), or the value returned
|||	        by the thread's main() function. The msg_Val1 field of the
|||	        message contains the item number of the thread that just
|||	        terminated. Finally, the msg_Val2 field contains the item
|||	        number of the task or thread that terminated the thread. If the
|||	        thread exited on its own, this will be the item number of the
|||	        thread itself. It is the responsibility of the task that
|||	        receives the status message to delete it when it is no longer
|||	        needed by using DeleteMsg().
|||
|||	    CREATETASK_TAG_MAXQ (uint32)
|||	        A value indicating the maximum quanta for the thread in
|||	        microseconds.
|||
|||	    CREATETASK_TAG_USERDATA (void *)
|||	        This specifies an arbitrary 32-bit value that is put in the
|||	        new thread's t_UserData field. This is a convenient way to pass
|||	        a pointer to a shared data structure when starting a thread.
|||
|||	    CREATETASK_TAG_USEREXCHANDLER (UserHandler)
|||	        Lets you specify the exception handler for the thread.
|||
|||	    CREATETASK_TAG_DEFAULTMSGPORT (void)
|||	        When this tag is present, the kernel automatically creates a
|||	        message port for the new thread being started. The
|||	        item number of this port is stored in the Task structure's
|||	        t_DefaultMsgPort field. This is a convenient way to quickly
|||	        establish a communication channel between a parent and a child.
|||
|||	  Return Value
|||
|||	    Returns the item number of the thread or a negative error code
|||	    for failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V27.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>, libc.a
|||
|||	  See Also
|||
|||	    DeleteModuleThread(), exit(), OpenModule(), system()
|||
**/

/**
|||	AUTODOC -class Kernel -group Tasks -name DeleteModuleThread
|||	Deletes a thread.
|||
|||	  Synopsis
|||
|||	    Err DeleteModuleThread(Item thread);
|||
|||	  Description
|||
|||	    This function deletes a thread. Any items owned by the thread
|||	    are automatically freed. Memory allocated by the thread is NOT
|||	    freed however and remains the property of the parent task.
|||
|||	  Arguments
|||
|||	    thread
|||	        The item number of the thread to be deleted.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Macro implemented implemented in <kernel/task.h> V27.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateModuleThread(), exit()
|||
**/

/**
|||	AUTODOC -class Kernel -group Messaging -name CreateSmallMsg
|||	Creates a small message.
|||
|||	  Synopsis
|||
|||	    Item CreateSmallMsg(const char *name, uint8 pri, Item mp);
|||
|||	  Description
|||
|||	    This function creates a small message (a message that can contain
|||	    up to eight bytes of data). Small messages are the fastest kind
|||	    of messages you can send, as no data is copied, and no pointers
|||	    need to be validated.
|||
|||	    To create a standard message (a message in which any data to be
|||	    communicated to the receiving task is contained in a data block
|||	    allocated by the sending task), use CreateMsg(). To create a
|||	    buffered message (a message that includes an internal buffer for
|||	    sending data to the receiving task), use CreateBufferedMsg().
|||
|||	    The same message item can be resent any number of times. When you are
|||	    finished with a message item, use DeleteMsg() to delete it.
|||
|||	    You can use FindNamedItem() to find a message by name.
|||
|||	  Arguments
|||
|||	    name
|||	        The optional name of the message.
|||
|||	    pri
|||	        The priority of the message. This determines the position of
|||	        the message in the receiving task's message queue and thus, how
|||	        soon it is likely to be handled. A larger number specifies a
|||	        higher priority.
|||
|||	    mp
|||	        The item number of the message port at which to receive the
|||	        reply, or 0 if no reply is expected.
|||
|||	  Return Value
|||
|||	    Returns the item number of the message or a negative error code
|||	    for failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V20.
|||
|||	  Associated Files
|||
|||	    <kernel/msgport.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateMsg(), CreateMsgPort(), CreateBufferedMsg(), DeleteMsg(),
|||	    DeleteMsgPort(), SendMsg()
|||
**/

/**
|||	AUTODOC -class Kernel -group Tasks -name CreateTask
|||	Creates a task from a loaded code module.
|||
|||	  Synopsis
|||
|||	    Item CreateTask(Item module, const char *name, const TagArg *tags);
|||	    Item CreateTaskVA(Item module, const char *name, uint32 tags, ...);
|||
|||	  Description
|||
|||	    This function launches a loaded code module as a task.
|||
|||	    The stack size and priority for the new task are determined by
|||	    the values specified to the linker when the module was linked. If
|||	    no priority was given at link time, the task will be launched
|||	    with the priority of the current task.
|||
|||	    When you no longer need a task, use DeleteTask() to
|||	    delete it. Alternatively, the task can return or call exit().
|||
|||	  Arguments
|||
|||	    module
|||	        The loaded code module to launch as a task, as obtained from
|||	        OpenModule()
|||
|||	    name
|||	        The name of the task to be created. You can later use
|||	        FindTask() to find it by name.
|||
|||	    tags
|||	        A pointer to an array of optional tag arguments containing
|||	        extra data for this function, or NULL. See below for a
|||	        description of the tags supported.
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function. The array must be terminated with TAG_END.
|||
|||	    CREATETASK_TAG_ARGC (uint32)
|||	        A 32-bit value that will be passed to the task being
|||	        launched as argc for its main() function. If this tag is
|||	        omitted, argc will be 0.
|||
|||	    CREATETASK_TAG_ARGP (uint32)
|||	        A 32-bit value that will be passed to the task being
|||	        launched as argv for its main() function. If this tag is
|||	        omitted, argv will be NULL.
|||
|||	    CREATETASK_TAG_CMDSTR (char *)
|||	        A pointer to a string to use to build an argv[]-style array
|||	        to pass in to the task being launched for its main() function.
|||	        Using this tag overrides values you might have supplied with
|||	        CREATETASK_TAG_ARGC or CREATETASK_TAG_ARGP.
|||
|||	    CREATETASK_TAG_MSGFROMCHILD (Item)
|||	        Provides the item number of a message port.  The kernel will
|||	        send a status message to this port whenever the task
|||	        being created exits. The message is sent by the kernel
|||	        after the task has been deleted. The msg_Result field of the
|||	        message contains the exit status of the task. This
|||	        is the value the task provided to exit(), or the value returned
|||	        by the task's main() function. The msg_Val1 field of the
|||	        message contains the item number of the task that just
|||	        terminated. Finally, the msg_Val2 field contains the item
|||	        number of the thread or task that terminated the task. If the
|||	        task exited on its own, this will be the item number of the
|||	        task itself. It is the responsibility of the task that
|||	        receives the status message to delete it when it is no longer
|||	        needed by using DeleteMsg().
|||
|||	    CREATETASK_TAG_MAXQ (uint32)
|||	        A value indicating the maximum quanta for the task in
|||	        microseconds.
|||
|||	    CREATETASK_TAG_USERDATA (void *)
|||	        This specifies an arbitrary 32-bit value that is put in the
|||	        new task's t_UserData field. This is a convenient way to pass
|||	        a pointer to a shared data structure.
|||
|||	    CREATETASK_TAG_USEREXCHANDLER (UserHandler)
|||	        Lets you specify the exception handler for the task.
|||
|||	    CREATETASK_TAG_DEFAULTMSGPORT (void)
|||	        When this tag is present, the kernel automatically creates a
|||	        message port for the new task being started. The
|||	        item number of this port is stored in the Task structure's
|||	        t_DefaultMsgPort field. This is a convenient way to quickly
|||	        establish a communication channel between a parent and a child.
|||
|||	  Return Value
|||
|||	    Returns the item number of the task or a negative error code
|||	    for failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V27.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>, libc.a
|||
|||	  See Also
|||
|||	    DeleteTask(), exit(), OpenModule(), system()
|||
**/

/**
|||	AUTODOC -class Kernel -group Tasks -name DeleteTask
|||	Deletes a task.
|||
|||	  Synopsis
|||
|||	    Err DeleteTask(Item task);
|||
|||	  Description
|||
|||	    This function deletes a task. Any items owned by the task, which
|||	    includes any threads of that task, are automatically freed. Any
|||	    memory allocated by the task is also freed.
|||
|||	  Arguments
|||
|||	    task
|||	        The item number of the task to be deleted.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Macro implemented implemented in <kernel/task.h> V27.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateTask(), exit()
|||
**/

/**
|||	AUTODOC -class Kernel -group Tasks -name CreateThread
|||	Creates a thread.
|||
|||	  Synopsis
|||
|||	    Item CreateThread(void (*code)(), const char *name, uint8 pri,
|||	                      int32 stackSize, const TagArg *tags);
|||
|||	    Item CreateThreadVA(void (*code)(), const char *name, uint8 pri,
|||	                        int32 stackSize, uint32 tags, ...);
|||
|||	  Description
|||
|||	    This function creates a thread. Threads provide a preemptively
|||	    scheduled execution context which has all of the characteristics
|||	    of a full-fledged task, except that they don't have their own
|||	    address space, and therefore share the one of their parent task.
|||
|||	    There is no default size for a thread's stack. To avoid stack
|||	    overflow errors, the stack must be large enough to handle any
|||	    possible uses. One way to find the proper stack size for a thread
|||	    is to start with a very large stack, reduce its size until a stack
|||	    overflow occurs, and then double its size. The memory for the
|||	    stack is allocated by this function and gets freed automatically
|||	    when the thread exits.
|||
|||	    This function takes a function pointer and the thread will begin
|||	    execution at that point in memory. You can also load in some
|||	    external code and run it as a thread using CreateModuleThread().
|||
|||	    When you no longer need a thread, use DeleteThread() to delete it.
|||	    Alternatively, the thread can return or call exit().
|||
|||	  Arguments
|||
|||	    code
|||	        A pointer to the code that the thread executes.
|||
|||	    name
|||	        The name of the thread to be created. You can later use
|||	        FindTask() to find it by name.
|||
|||	    pri
|||	        The priority of the thread, in the range 11 to 199. A
|||	        larger number specifies a higher priority. A value of
|||	        0 for this argument specifies that the thread should be
|||	        launched at the same priority as the current task.
|||
|||	    stackSize
|||	        The size in bytes of the thread's stack. A good default value
|||	        for this is 4096.
|||
|||	    tags
|||	        A pointer to an array of optional tag arguments containing
|||	        extra data for this function, or NULL. See below for a
|||	        description of the tags supported.
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function. The array must be terminated with TAG_END.
|||
|||	    CREATETASK_TAG_ARGC (uint32)
|||	        A 32-bit value that will be passed to the thread being
|||	        launched as its first argument. If this is omitted, the
|||	        first argument will be 0.
|||
|||	    CREATETASK_TAG_ARGP (uint32)
|||	        A 32-bit value that will be passed to the thread being
|||	        launched as a second argument. If this is omitted, the second
|||	        argument will be 0.
|||
|||	    CREATETASK_TAG_MSGFROMCHILD (Item)
|||	        Provides the item number of a message port.  The kernel will
|||	        send a status message to this port whenever the thread
|||	        being created exits. The message is sent by the kernel
|||	        after the thread has been deleted. The msg_Result field of the
|||	        message contains the exit status of the thread. This
|||	        is the value the task provided to exit(), or the value returned
|||	        by the thread initial function. The msg_Val1 field of the
|||	        message contains the item number of the thread that just
|||	        terminated. Finally, the msg_Val2 field contains the item
|||	        number of the thread or task that terminated the thread. If the
|||	        thread exited on its own, this will be the item number of the
|||	        thread itself. It is the responsibility of the task that
|||	        receives the status message to delete it when it is no longer
|||	        needed by using DeleteMsg().
|||
|||	    CREATETASK_TAG_MAXQ (uint32)
|||	        A value indicating the maximum quanta for the thread in
|||	        microseconds.
|||
|||	    CREATETASK_TAG_USERDATA (void *)
|||	        This specifies an arbitrary 32-bit value that is put in the
|||	        new thread's t_UserData field. This is a convenient way to pass
|||	        a pointer to a shared data structure when starting a thread.
|||
|||	    CREATETASK_TAG_USEREXCHANDLER (UserHandler)
|||	        Lets you specify the exception handler for the thread.
|||
|||	    CREATETASK_TAG_DEFAULTMSGPORT (void)
|||	        When this tag is present, the kernel automatically creates a
|||	        message port for the new thread being started. The
|||	        item number of this port is stored in the Task structure's
|||	        t_DefaultMsgPort field. This is a convenient way to quickly
|||	        establish a communication channel between a parent and a child.
|||
|||	  Return Value
|||
|||	    Returns the item number of the thread or a negative error code
|||	    for failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V27.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>, libc.a
|||
|||	  See Also
|||
|||	    DeleteThread(), exit()
|||
**/

/**
|||	AUTODOC -class Kernel -group Tasks -name DeleteThread
|||	Deletes a thread.
|||
|||	  Synopsis
|||
|||	    Err DeleteThread(Item thread);
|||
|||	  Description
|||
|||	    This function deletes a thread. Any items owned by the thread
|||	    are automatically freed. Memory allocated by the thread is NOT
|||	    freed however and remains the property of the parent task.
|||
|||	  Arguments
|||
|||	    thread
|||	        The item number of the thread to be deleted.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Macro implemented implemented in <kernel/task.h> V27.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateThread(), exit()
|||
**/

/**
|||	AUTODOC -class Kernel -group Semaphores -name FindSemaphore
|||	Finds a semaphore by name.
|||
|||	  Synopsis
|||
|||	    Item FindSemaphore(const char *name);
|||
|||	  Description
|||
|||	    This macro finds a semaphore with the specified name. The search is
|||	    not case-sensitive.
|||
|||	  Arguments
|||
|||	    name
|||	        The name of the semaphore to find.
|||
|||	  Return Value
|||
|||	    Returns the item number of the semaphore that was found, or a
|||	    negative error code for failure.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/semaphore.h> V20.
|||
|||	  Associated Files
|||
|||	    <kernel/semaphore.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateSemaphore(), CreateUniqueSemaphore()
|||
**/

/**
|||	AUTODOC -class Kernel -group Tasks -name FindTask
|||	Finds a task by name.
|||
|||	  Synopsis
|||
|||	    Item FindTask(const char *name);
|||
|||	  Description
|||
|||	    This macro finds a task with the specified name. The search is not
|||	    case-sensitive.
|||
|||	    To get a pointer to the current task, use the CURRENTTASK macro
|||	    defined in <kernel/task.h>
|||
|||	  Arguments
|||
|||	    name
|||	        The name of the task to find.
|||
|||	  Return Value
|||
|||	    Returns the item number of the task that was found, or a
|||	    negative error code for failure.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/task.h> V20.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateThread()
|||
**/

/**
|||	AUTODOC -class Kernel -group Messaging -name FindMsgPort
|||	Finds a message port by name.
|||
|||	  Synopsis
|||
|||	    Item FindMsgPort(const char *name);
|||
|||	  Description
|||
|||	    This macro finds a message port with the specified name. The search is not
|||	    case-sensitive.
|||
|||	  Arguments
|||
|||	    name
|||	        The name of the message port to find.
|||
|||	  Return Value
|||
|||	    Returns the item number of the message port that was found, or a
|||	    negative error code for failure.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/msgport.h> V20.
|||
|||	  Associated Files
|||
|||	    <kernel/msgport.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateMsgPort(), CreateUniqueMsgPort()
|||
**/

/**
|||	AUTODOC -class Kernel -group Items -name FindNamedItem
|||	Finds an item by name.
|||
|||	  Synopsis
|||
|||	    Item FindNamedItem(int32 ctype, const char *name);
|||
|||	  Description
|||
|||	    This function finds an item of the specified type and name.
|||	    The search is not case-sensitive.
|||
|||	  Arguments
|||
|||	    ctype
|||	        The type of the item to find. Use MkNodeID() to create this
|||	        value.
|||
|||	    name
|||	        The name of the item to find.
|||
|||	  Return Value
|||
|||	    Returns the item number of the item that was found, or a
|||	    negative error code for failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V20.
|||
|||	  Associated Files
|||
|||	    <kernel/item.h>, libc.a
|||
|||	  See Also
|||
|||	    FindItem()
|||
**/

/**
|||	AUTODOC -class kernel -group Timer -name SampleSystemTimeVBL
|||	Samples the system VBL count with very low overhead.
|||
|||	  Synopsis
|||
|||	    void SampleSystemTimeVBL(TimeValVBL *tv);
|||
|||	  Description
|||
|||	    This function records the current system VBL count. There are 60
|||	    VBLs per second on an NTSC system and 50 VBLs per second on a PAL
|||	    system.
|||
|||	  Arguments
|||
|||	    tv
|||	        A pointer to a TimeValVBL structure which will receive the
|||	        current system VBL count.
|||
|||	  Warning
|||
|||	    The VBlank timer runs at either 50Hz or 60Hz depending on whether
|||	    the system is displaying PAL or NTSC.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, libc.a
|||
**/

/**
|||	AUTODOC -class Kernel -group SpinLock -name CreateSpinLock
|||	Creates a SpinLock.
|||
|||	  Synopsis
|||
|||	    Err CreateSpinLock(SpinLock **sl);
|||
|||	  Description
|||
|||	    This function allocates a SpinLock structure. Spin locks are used
|||	    to synchronize execution of code on the two CPUs. Use semaphores to
|||	    synchronize execution of mutliple threads running on the main CPU.
|||
|||	  Arguments
|||
|||	    sl
|||	        A pointer to a variable where a handle to the SpinLock will
|||	        be stored. The value is set to NULL if the file can't be
|||	        created.
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
|||	    <kernel/spinlock.h>
|||
|||	  See Also
|||
|||	    DeleteSpinLock(), ObtainSpinLock(), ReleaseSpinLock()
|||
**/

/**
|||	AUTODOC -class Kernel -group SpinLock -name DeleteSpinLock
|||	Deletes a SpinLock.
|||
|||	  Synopsis
|||
|||	    Err DeleteSpinLock(SpinLock *sl);
|||
|||	  Description
|||
|||	    Releases any resources allocated by CreateSpinLock().
|||
|||	  Arguments
|||
|||	    sl
|||	        The SpinLock structure as obtained from CreateSpinLock(). This
|||	        value may be NULL in which case this function does nothing.
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
|||	    <kernel/spinlock.h>
|||
|||	  See Also
|||
|||	    CreateSpinLock(), ObtainSpinLock(), ReleaseSpinLock()
|||
**/

/**
|||	AUTODOC -class Kernel -group SpinLock -name ObtainSpinLock
|||	Attempts to acquire a spin lock.
|||
|||	  Synopsis
|||
|||	    bool ObtainSpinLock(SpinLock *sl);
|||
|||	  Description
|||
|||	    This function attempts to lock a spin lock. Only one CPU at a time
|||	    can hold a given spin lock, so you use them to coordinate access to
|||	    resources shared by the two processors.
|||
|||	    Spin locks are intended purely for cross-processor synchronization.
|||	    Use semaphores to synchronize execution of multiple threads.
|||
|||	  Arguments
|||
|||	    sl
|||	        The spin lock to obtain.
|||
|||	  Return Value
|||
|||	    Returns TRUE if the spin lock has been obtained, or FALSE if it
|||	    was already held.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V32.
|||
|||	  Associated Files
|||
|||	    <kernel/spinlock.h>
|||
|||	  See Also
|||
|||	    CreateSpinLock(), DeleteSpinLock(), ReleaseSpinLock()
|||
**/

/**
|||	AUTODOC -class Kernel -group SpinLock -name ReleaseSpinLock
|||	Releases a previously held spin lock.
|||
|||	  Synopsis
|||
|||	    void ReleaseSpinLock(SpinLock *sl);
|||
|||	  Description
|||
|||	    This function relinquishes a spin lock previously acquired
|||	    using ObtainSpinLock(). The spin lock will then become available
|||	    for locking by the other CPU.
|||
|||	  Arguments
|||
|||	    sl
|||	        The spin lock to release
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V32.
|||
|||	  Associated Files
|||
|||	    <kernel/spinlock.h>
|||
|||	  See Also
|||
|||	    CreateSpinLock(), DeleteSpinLock(), ObtainSpinLock()
|||
**/

/**
|||	AUTODOC -class Kernel -group Tasks -name IncreaseResourceTable
|||	Increase the size of the caller's resource table.
|||
|||	  Synopsis
|||
|||	    Err IncreaseResourceTable(uint32 numSlots);
|||
|||	  Description
|||
|||	    Each task and thread in the system has a table that holds the
|||	    number of all the items owned or opened by the task or thread.
|||	    The kernel uses this table to perform cleanup operations when a
|||	    task or thread exits. This table is dynamic, the kernel grows it
|||	    automatically when more room is needed.
|||
|||	    There are certain cases when it is desirable to preallocate a large
|||	    resource table. This function lets you specify the number of slots
|||	    by which to expand the table. Each slot can hold a single item
|||	    number.
|||
|||	  Arguments
|||
|||	    numSlots
|||	        The number of resource slots to add.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V33.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>
|||
**/

/* keep the compiler happy... */
extern int foo;
