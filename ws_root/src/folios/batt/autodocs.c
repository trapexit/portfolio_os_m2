/* @(#) autodocs.c 96/03/11 1.3 */

/**
|||	AUTODOC -private -class Batt -name GetBattMemInfo
|||	Gets information about battery-backed memory.
|||
|||	  Synopsis
|||
|||	    void GetBattMemInfo(BattMemInfo *info, uint32 infoSize);
|||
|||	  Description
|||
|||	    This function returns information about the system's battery-backed
|||	    memory. The info is returned in a BattMemInfo structure, containing
|||	    the following fields:
|||
|||	    bminfo_NumBytes
|||	        The number of bytes of battery-backed memory available.
|||
|||	  Arguments
|||
|||	    info
|||	        Pointer to a structure where the memory information will
|||	        be put.
|||
|||	    infoSize
|||	        Size of the structure to receive the memory information.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Batt folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/batt.h>, System.m2/Modules/batt
|||
|||	  See Also
|||
|||	    ReadBattMem(), WriteBattMem()
|||
**/

/**
|||	AUTODOC -private -class Batt -name WriteBattMem
|||	Writes bytes to the battery-backed memory.
|||
|||	  Synopsis
|||
|||	    Err WriteBattMem(const void *buffer, uint32 numBytes, uint32 offset);
|||
|||	  Description
|||
|||	    This function writes data to the system's battery-backed
|||	    memory. The contents of this memory is preserved even when the
|||	    power is turned on.
|||
|||	    There are typically very few bytes of this type of memory. You
|||	    can obtain the current size of the memory by calling
|||	    GetBattMemInfo().
|||
|||	    Before doing read-modify-write operations to the battery-backed
|||	    memory, you should first call LockBattMem() to guarantee
|||	    exclusive access. Once your operations are complete, you then call
|||	    UnlockBattMem() to allow others to access the memory.
|||
|||	  Arguments
|||
|||	    buffer
|||	        The bytes to write to the memory.
|||
|||	    numBytes
|||	        The number of bytes to write to the memory.
|||
|||	    offset
|||	        Offset within memory where the write operation should
|||	        start.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    Possible error codes currently include:
|||
|||	    BATT_ERR_NOHARDWARE
|||	        This system doesn't have any battery-backed memory.
|||
|||	    BATT_ERR_BADPTR
|||	        The supplied buffer pointer is not valid.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Batt folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/batt.h>, System.m2/Modules/batt
|||
|||	  See Also
|||
|||	    ReadBattMem(), GetBattMemInfo(), LockBattMem(), UnlockBattMem()
|||
**/

/**
|||	AUTODOC -private -class Batt -name ReadBattMem
|||	Reads bytes from the battery-backed memory.
|||
|||	  Synopsis
|||
|||	    Err ReadBattMem(void *buffer, uint32 numBytes, uint32 offset);
|||
|||	  Description
|||
|||	    This function reads data from the system's battery-backed
|||	    memory. The contents of this memory is preserved even when the
|||	    power is turned on.
|||
|||	    There are typically very few bytes of this type of memory. You
|||	    can obtain the current size of the memory by calling
|||	    GetBattMemInfo().
|||
|||	    Before doing read-modify-write operations to the battery-backed
|||	    memory, you should first call LockBattMem() to guarantee
|||	    exclusive access. Once your operations are complete, you then call
|||	    UnlockBattMem() to allow others to access the memory.
|||
|||	  Arguments
|||
|||	    buffer
|||	        Pointer to where to deposit the data being read.
|||
|||	    numBytes
|||	        Number of bytes of data to read.
|||
|||	    offset
|||	        Offset within the memory to start reading.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    Possible error codes currently include:
|||
|||	    BATT_ERR_NOHARDWARE
|||	        This system doesn't have any battery-backed memory.
|||
|||	    BATT_ERR_BADPTR
|||	        The supplied buffer pointer is not valid.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Batt folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/batt.h>, System.m2/Modules/batt
|||
|||	  See Also
|||
|||	    WriteBattMem(), GetBattMemInfo(), LockBattMem(), UnlockBattMem()
|||
**/

/**
|||	AUTODOC -class Batt -name WriteBattClock
|||	Sets the battery-backed clock.
|||
|||	  Synopsis
|||
|||	    Err WriteBattClock(const GregorianDate *gd);
|||
|||	  Description
|||
|||	    This function sets the time and date of the system's battery-backed
|||	    clock.
|||
|||	  Arguments
|||
|||	    gd
|||	        A GregorianDate structure initialized with the time and date
|||	        value to set in the battery-backed clock.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    Possible error codes currently include:
|||
|||	    BATT_ERR_NOHARDWARE
|||	        This system doesn't have any battery-backed clock.
|||
|||	    BATT_ERR_BADPTR
|||	        The supplied date pointer is not valid.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Batt folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/batt.h>, System.m2/Modules/batt
|||
|||	  See Also
|||
|||	    ReadBattClock()
|||
**/

/**
|||	AUTODOC -class Batt -name ReadBattClock
|||	Reads the current setting of the battery-backed clock.
|||
|||	  Synopsis
|||
|||	    Err ReadBattClock(GregorianDate *gd);
|||
|||	  Description
|||
|||	    This function reads the current value of the system's
|||	    battery-backed clock.
|||
|||	  Arguments
|||
|||	    gd
|||	        Pointer to a GregorianDate structure which is filled in by
|||	        this function.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    Possible error codes currently include:
|||
|||	    BATT_ERR_NOHARDWARE
|||	        This system doesn't have any battery-backed clock.
|||
|||	    BATT_ERR_BADPTR
|||	        The supplied buffer pointer is not valid.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Batt folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/batt.h>, System.m2/Modules/batt
|||
|||	  See Also
|||
|||	    WriteBattClock()
|||
**/

/**
|||	AUTODOC -private -class Batt -name LockBattMem
|||	Gets exclusive access to the battery-backed memory.
|||
|||	  Synopsis
|||
|||	    void LockBattMem(void);
|||
|||	  Description
|||
|||	    This function grants the current task exclusive access to the
|||	    battery-backed memory. This allows read-modify-write operations
|||	    to be performed atomically.
|||
|||	    When you are done with the memory, you should call UnlockBattMem()
|||	    to allow other tasks to access the battery-backed memory.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Batt folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/batt.h>, System.m2/Modules/batt
|||
|||	  See Also
|||
|||	    UnlockBattMem()
|||
**/

/**
|||	AUTODOC -private -class Batt -name UnlockBattMem
|||	Relinquishes exclusive access to the battery-backed memory.
|||
|||	  Synopsis
|||
|||	    void UnlockBattMem(void);
|||
|||	  Description
|||
|||	    This function allows other tasks to access the battery-backed
|||	    memory.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Batt folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/batt.h>, System.m2/Modules/batt
|||
|||	  See Also
|||
|||	    LockBattMem()
|||
**/

/* keep the compiler happy... */
extern int foo;
