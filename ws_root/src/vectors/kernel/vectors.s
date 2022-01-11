#include <hardware/PPCMacroequ.i>

	DECFN main
	blr

	DECFN NextTagArg
	blr

	DECFN FindTagArg
	blr

	DECFN AllocMem
	blr

	DECFN FreeMem
	blr

	DECFN CheckIO
	blr

	DECFN ReallocMem
	blr

	DECFN GetMemTrackSize
	blr

	DECFN FreeMemPages
	blr

	DECFN exit
	blr

	DECFN SetBitRange
	blr

	DECFN ClearBitRange
	blr

	DECFN IsBitRangeClear
	blr

	DECFN IsBitRangeSet
	blr

	DECFN IsItemOpened
	blr

	DECFN FindSetBitRange
	blr

	DECFN FindClearBitRange
	blr

	DECFN GetMemInfo
	blr

	DECFN ScavengeMem
	blr

	DECFN GetPageSize
	blr

	DECFN IsMemReadable
	blr

	DECFN IsMemWritable
	blr

	DECFN SampleSystemTimeTT
	blr

	DECFN SampleSystemTimeVBL
	blr

	DECFN GetSysErr
	blr

	DECFN vcprintf
	blr

	DECFN InsertNodeFromHead
	blr

	DECFN SetNodePri
	blr

	DECFN ConvertTimerTicksToTimeVal
	blr

	DECFN UniversalInsertNode
	blr

	DECFN CheckItem
	blr

	DECFN IsMemOwned
	blr

	DECFN memmove
	blr

	DECFN memcpy
	blr

	DECFN memset
	blr

	DECFN LookupItem
	blr

	DECFN AllocMemMasked
	blr

	DECFN FindNamedNode
	blr

	DECFN PrepList
	blr

	DECFN GetCacheInfo
	blr

	DECFN MayGetChar
	blr

	DECFN RemNode
	blr

	DECFN InsertNodeFromTail
	blr

	DECFN AddTail
	blr

	DECFN RemTail
	blr

	DECFN AddHead
	blr

	DECFN RemHead
	blr

	DECFN ConvertTimeValToTimerTicks
	blr

	DECFN AddTimerTicks
	blr

	DECFN SubTimerTicks
	blr

	DECFN CompareTimerTicks
	blr

	DECFN SuperLogEvent
	blr

	DECFN FlushDCache
	blr

	DECFN WriteBackDCache
	blr

	DECFN InvalidateICache
	blr

	DECFN MatchDeviceName
	blr

	DECFN DumpMemDebug
	blr

	DECFN SanityCheckMemDebug
	blr

	DECFN AllocMemDebug
	blr

	DECFN AllocMemMaskedDebug
	blr

	DECFN FreeMemDebug
	blr

	DECFN ReallocMemDebug
	blr

	DECFN GetMemTrackSizeDebug
	blr

	DECFN CallAsItemServer
	blr

	DECFN setjmp
	blr

	DECFN longjmp
	blr

	DECFN CreateDeviceList
	blr

	DECFN DeleteDeviceList
	blr

	DECFN CreateDeviceStackList
	blr

	DECFN DeleteDeviceStackList
	blr

	DECFN OpenDeviceStack
	blr

	DECFN CloseDeviceStack
	blr

	DECFN DeviceStackIsIdentical
	blr

	DECFN DDFCanManage
	blr

	DECFN CreateDeviceStack
	blr

	DECFN DeleteDeviceStack
	blr

	DECFN AppendDeviceStackDDF
	blr

	DECFN AppendDeviceStackHW
	blr

	DECFN FindDDF
	blr

	DECFN NextHWResource
	blr

	DECFN ReadUniqueID
	blr

	DECFN CreateItemVA
	blr

	DECFN CreateModuleThreadVA
	blr

	DECFN CreateSizedItemVA
	blr

	DECFN CreateTaskVA
	blr

	DECFN CreateThreadVA
	blr

	DECFN DumpMemDebugVA
	blr

	DECFN FindAndOpenItemVA
	blr

	DECFN FindItemVA
	blr

	DECFN RationMemDebugVA
	blr

	DECFN __tagCall_0
	blr

	DECFN __tagCall_1
	blr

	DECFN __tagCall_2
	blr

	DECFN __tagCall_3
	blr

	DECFN __tagCall_4
	blr

	DECFN DoMemRation
	blr

	DECFN RationMemDebug
	blr

	DECFN ExpectDataDisc
	blr

	DECFN NoExpectDataDisc
	blr

	DECFN BeginNoReboot
	blr

	DECFN EndNoReboot
	blr

	DECFN UnimportByName
	blr

	DECFN GetPersistentMem
	blr

	DECFN RegisterOperator
	blr

	DECFN ImportByName
	blr

	DECFN FreeInitModules
	blr

	DECFN IsMasterCPU
	blr

	DECFN _GetCacheState
	blr

	DECFN memswap
	blr

	DECFN FreeMemPagesDebug
	blr

	DECFN AllocMemPagesDebug
	blr

	DECFN RemAllocation
	blr

	DECFN AddAllocation
	blr

	DECFN ControlMemDebug
	blr

	DECFN DeleteMemDebug
	blr

	DECFN CreateMemDebug
	blr

	DECFN OpenRomAppMedia
	blr

	DECFN RegisterUserExceptionHandler
	blr

	DECFN ControlUserExceptions
	blr

	DECFN DebugPutStr
	blr

	DECFN CreateItem
	blr

	DECFN DebugPutChar
	blr

	DECFN ControlLumberjack
	blr

	DECFN ReleaseLumberjackBuffer
	blr

	DECFN ObtainLumberjackBuffer
	blr

	DECFN LogEvent
	blr

	DECFN DeleteLumberjack
	blr

	DECFN CreateLumberjack
	blr

	DECFN ControlCaches
	blr

	DECFN Print3DOHeader
	blr

	DECFN WaitIO
	blr

	DECFN WaitPort
	blr

	DECFN SetExitStatus
	blr

	DECFN DoIO
	blr

	DECFN FindAndOpenItem
	blr

	DECFN CompleteIO
	blr

	DECFN CloseItemAsTask
	blr

	DECFN FlushDCacheAll
	blr

	DECFN SuperInvalidateDCache
	blr

	DECFN CallBackSuper
	blr

	DECFN SetItemOwner
	blr

	DECFN OpenItemAsTask
	blr

	DECFN AbortIO
	blr

	DECFN SendIO
	blr

	DECFN FreeSignal
	blr

	DECFN AllocSignal
	blr

	DECFN InvalidateFPState
	blr

	DECFN GetMsg
	blr

	DECFN ReplyMsg
	blr

	DECFN ReplySmallMsg
	blr

	DECFN ReadHardwareRandomNumber
	blr

	DECFN SendMsg
	blr

	DECFN SendSmallMsg
	blr

	DECFN GetThisMsg
	blr

	DECFN kprintf
	blr

	DECFN NullSysCall
	blr

	DECFN AllocMemPages
	blr

	DECFN ControlMem
	blr

	DECFN SetItemPri
	blr

	DECFN Yield
	blr

	DECFN CloseItem
	blr

	DECFN LockSemaphore
	blr

	DECFN UnlockSemaphore
	blr

	DECFN OpenItem
	blr

	DECFN FindItem
	blr

	DECFN DeleteItem
	blr

	DECFN SendSignal
	blr

	DECFN WaitSignal
	blr

	DECFN CreateSizedItem
	blr

	DECFN CreateBufferedMsg
	blr

	DECFN CreateFIRQ
	blr

	DECFN CreateIOReq
	blr

	DECFN CreateMsg
	blr

	DECFN CreateMsgPort
	blr

	DECFN CreateNamedItemVA
	blr

	DECFN CreateSemaphore
	blr

	DECFN CreateSmallMsg
	blr

	DECFN FindNamedItem
	blr

	DECFN SuperInternalPutMsg
	blr

	DECFN Disable
	blr

	DECFN Enable
	blr

	DECFN EnableInterrupt
	blr

	DECFN SuperInternalSendIO
	blr

	DECFN SuperCompleteIO
	blr

	DECFN SuperInternalAbortIO
	blr

	DECFN SuperReallocMem
	blr

	DECFN SuperInternalSignal
	blr

	DECFN SuperFreeUserMem
	blr

	DECFN Permit
	blr

	DECFN TagProcessor
	blr

	DECFN TagProcessorNoAlloc
	blr

	DECFN SuperInternalUnlockSemaphore
	blr

	DECFN SuperInternalLockSemaphore
	blr

	DECFN SuperInternalDeleteItem
	blr

	DECFN SuperInternalFreeSignal
	blr

	DECFN SuperAllocUserMem
	blr

	DECFN SuperAllocMemMasked
	blr

	DECFN SuperReportEvent
	blr

	DECFN SuperQuerySysInfo
	blr

	DECFN SuperSetSysInfo
	blr

	DECFN SectorECC
	blr

	DECFN SuperInternalWaitPort
	blr

	DECFN SuperInternalWaitIO
	blr

	DECFN SuperInternalDoIO
	blr

	DECFN SuperInternalInsertMemLockHandler
	blr

	DECFN SuperInternalRemoveMemLockHandler
	blr

	DECFN CreateSpinLock
	blr

	DECFN DeleteSpinLock
	blr

	DECFN SuperInternalReplyMsg
	blr

	DECFN SuperAllocMem
	blr

	DECFN SuperFreeMem
	blr

	DECFN ChannelRead
	blr

	DECFN ChannelMap
	blr

	DECFN ChannelUnmap
	blr

	DECFN ControlIODebug
	blr

	DECFN ChannelGetHWIcon
	blr

	DECFN TransferItems
	blr

	DECFN SuperAllocMemDebug
	blr

	DECFN SuperAllocMemMaskedDebug
	blr

	DECFN SuperFreeMemDebug
	blr

	DECFN IsUser
	blr

	DECFN SoftReset
	blr

	DECFN SatisfiesNeed
	blr

	DECFN RegisterDuck
	blr

	DECFN RegisterRecover
	blr

	DECFN UnregisterDuck
	blr

	DECFN UnregisterRecover
	blr

	DECFN TriggerDeviceRescan
	blr

	DECFN ScanForDDFToken
	blr

	DECFN NextDDFToken
	blr

	DECFN SuperInternalOpenItem
	blr

	DECFN SuperInternalCloseItem
	blr

	DECFN FindHWResource
	blr

	DECFN OpenSlotDevice
	blr

	DECFN CreateMemMapRecord
	blr

	DECFN DeleteMemMapRecord
	blr

	DECFN DeleteAllMemMapRecords
	blr

	DECFN SuperInternalFindAndOpenItem
	blr

	DECFN SuperCreateItem
	blr

	DECFN SuperWaitIO
	blr

	DECFN SuperWaitPort
	blr

	DECFN SuperDoIO
	blr

	DECFN SuperSetItemOwner
	blr

	DECFN SuperSendIO
	blr

	DECFN SuperFreeSignal
	blr

	DECFN SuperAllocSignal
	blr

	DECFN SuperControlMem
	blr

	DECFN SuperGetMsg
	blr

	DECFN SuperReplyMsg
	blr

	DECFN SuperSendMsg
	blr

	DECFN SuperSendSmallMsg
	blr

	DECFN SuperGetThisMsg
	blr

	DECFN Superkprintf
	blr

	DECFN SuperCloseItem
	blr

	DECFN SuperLockSemaphore
	blr

	DECFN SuperUnlockSemaphore
	blr

	DECFN SuperOpenItem
	blr

	DECFN SuperFindItem
	blr

	DECFN SuperDeleteItem
	blr

	DECFN SuperWaitSignal
	blr

	DECFN SuperCreateSizedItem
	blr

	DECFN __ctype
	blr

	DECFN KernelBase
	blr

	DECFN FindCurrentModule
	blr

	DECFN RegisterReportEvent
	blr

	DECFN ProcessDDFBuffer
	blr

	DECFN EnableDDF
	blr

	DECFN RebuildDDFEnables
	blr

	DECFN __va_arg
	blr

	DECFN OpenModule
	blr

	DECFN CloseModule
	blr

	DECFN ExecuteModule
	blr

	DECFN _InterruptStack
	blr

	DECFN _cblank
	blr

	DECFN SuperAllocNode
	blr

	DECFN SuperFreeNode
	blr

	DECFN SuperInternalCreateFirq
	blr

	DECFN OSPanic
	blr

	DECFN SuperInternalCreateTimer
	blr

	DECFN decrementerHandler
	blr

	DECFN smiHandler
	blr

	DECFN instrAddrHandler
	blr

	DECFN handle602
	blr

	DECFN systemCall
	blr

	DECFN ioErrorHandler
	blr

	DECFN FPExceptionHandler
	blr

	DECFN programHandler
	blr

	DECFN alignmentHandler
	blr

	DECFN instrAccessHandler
	blr

	DECFN dataAccessHandler
	blr

	DECFN machineCheckHandler
	blr

	DECFN interruptHandler
	blr

	DECFN KernelVBLHandler
	blr

	DECFN KernelCDEHandler
	blr

	DECFN traceHandler
	blr

	DECFN DisableInterrupt
	blr

	DECFN MonitorFirq
	blr

	DECFN _waitq
	blr

	DECFN _expiredq
	blr

	DECFN _ticksPerSecond
	blr

	DECFN _quantaclock
	blr

	DECFN SuperInternalCreateTaskVA
	blr

	DECFN SuperInternalCreateSemaphore
	blr

	DECFN SuperInternalCreateErrorText
	blr

	DECFN _ErrTA
	blr

	DECFN _MLioReqHandler
	blr

	DECFN _MLsem
	blr

	DECFN _DevSemaphore
	blr

	DECFN _gInBuf
	blr

	DECFN _gOutBuf
	blr

	DECFN printf
	blr

	DECFN clib_PrintError
	blr

	DECFN PrintfSysErr
	blr

	DECFN strlen
	blr

	DECFN strcasecmp
	blr

	DECFN strncasecmp
	blr

	DECFN strcpy
	blr

	DECFN strncpy
	blr

	DECFN strcat
	blr

	DECFN strncat
	blr

	DECFN CompleteUserException
	blr

	DECFN strcmp
	blr

	DECFN strncmp
	blr

	DECFN toupper
	blr

	DECFN tolower
	blr

	DECFN sprintf
	blr

	DECFN __itof
	blr

	DECFN __utof
	blr

	DECFN _restfpr_14_l
	blr

	DECFN _restfpr_15_l
	blr

	DECFN _restfpr_16_l
	blr

	DECFN _restfpr_17_l
	blr

	DECFN _restfpr_18_l
	blr

	DECFN _restfpr_19_l
	blr

	DECFN _restfpr_20_l
	blr

	DECFN _restfpr_21_l
	blr

	DECFN _restfpr_22_l
	blr

	DECFN _restfpr_23_l
	blr

	DECFN _restfpr_24_l
	blr

	DECFN _restfpr_25_l
	blr

	DECFN _restfpr_26_l
	blr

	DECFN _restfpr_27_l
	blr

	DECFN _restfpr_28_l
	blr

	DECFN _restfpr_29_l
	blr

	DECFN _restfpr_30_l
	blr

	DECFN _restfpr_31_l
	blr

	DECFN _restfprs_14_l
	blr

	DECFN _restfprs_15_l
	blr

	DECFN _restfprs_16_l
	blr

	DECFN _restfprs_17_l
	blr

	DECFN _restfprs_18_l
	blr

	DECFN _restfprs_19_l
	blr

	DECFN _restfprs_20_l
	blr

	DECFN _restfprs_21_l
	blr

	DECFN _restfprs_22_l
	blr

	DECFN _restfprs_23_l
	blr

	DECFN _restfprs_24_l
	blr

	DECFN _restfprs_25_l
	blr

	DECFN _restfprs_26_l
	blr

	DECFN _restfprs_27_l
	blr

	DECFN _restfprs_28_l
	blr

	DECFN _restfprs_29_l
	blr

	DECFN _restfprs_30_l
	blr

	DECFN _restfprs_31_l
	blr

	DECFN _restf14
	blr

	DECFN _restf15
	blr

	DECFN _restf16
	blr

	DECFN _restf17
	blr

	DECFN _restf18
	blr

	DECFN _restf19
	blr

	DECFN _restf20
	blr

	DECFN _restf21
	blr

	DECFN _restf22
	blr

	DECFN _restf23
	blr

	DECFN _restf24
	blr

	DECFN _restf25
	blr

	DECFN _restf26
	blr

	DECFN _restf27
	blr

	DECFN _restf28
	blr

	DECFN _restf29
	blr

	DECFN _restf30
	blr

	DECFN _restf31
	blr

	DECFN _restgpr_14
	blr

	DECFN _restgpr_15
	blr

	DECFN _restgpr_16
	blr

	DECFN _restgpr_17
	blr

	DECFN _restgpr_18
	blr

	DECFN _restgpr_19
	blr

	DECFN _restgpr_20
	blr

	DECFN _restgpr_21
	blr

	DECFN _restgpr_22
	blr

	DECFN _restgpr_23
	blr

	DECFN _restgpr_24
	blr

	DECFN _restgpr_25
	blr

	DECFN _restgpr_26
	blr

	DECFN _restgpr_27
	blr

	DECFN _restgpr_28
	blr

	DECFN _restgpr_29
	blr

	DECFN _restgpr_30
	blr

	DECFN _restgpr_31
	blr

	DECFN _restgpr_14_l
	blr

	DECFN _restgpr_15_l
	blr

	DECFN _restgpr_16_l
	blr

	DECFN _restgpr_17_l
	blr

	DECFN _restgpr_18_l
	blr

	DECFN _restgpr_19_l
	blr

	DECFN _restgpr_20_l
	blr

	DECFN _restgpr_21_l
	blr

	DECFN _restgpr_22_l
	blr

	DECFN _restgpr_23_l
	blr

	DECFN _restgpr_24_l
	blr

	DECFN _restgpr_25_l
	blr

	DECFN _restgpr_26_l
	blr

	DECFN _restgpr_27_l
	blr

	DECFN _restgpr_28_l
	blr

	DECFN _restgpr_29_l
	blr

	DECFN _restgpr_30_l
	blr

	DECFN _restgpr_31_l
	blr

	DECFN _restgprs_14_l
	blr

	DECFN _restgprs_15_l
	blr

	DECFN _restgprs_16_l
	blr

	DECFN _restgprs_17_l
	blr

	DECFN _restgprs_18_l
	blr

	DECFN _restgprs_19_l
	blr

	DECFN _restgprs_20_l
	blr

	DECFN _restgprs_21_l
	blr

	DECFN _restgprs_22_l
	blr

	DECFN _restgprs_23_l
	blr

	DECFN _restgprs_24_l
	blr

	DECFN _restgprs_25_l
	blr

	DECFN _restgprs_26_l
	blr

	DECFN _restgprs_27_l
	blr

	DECFN _restgprs_28_l
	blr

	DECFN _restgprs_29_l
	blr

	DECFN _restgprs_30_l
	blr

	DECFN _restgprs_31_l
	blr

	DECFN _savefpr_14_l
	blr

	DECFN _savefpr_15_l
	blr

	DECFN _savefpr_16_l
	blr

	DECFN _savefpr_17_l
	blr

	DECFN _savefpr_18_l
	blr

	DECFN _savefpr_19_l
	blr

	DECFN _savefpr_20_l
	blr

	DECFN _savefpr_21_l
	blr

	DECFN _savefpr_22_l
	blr

	DECFN _savefpr_23_l
	blr

	DECFN _savefpr_24_l
	blr

	DECFN _savefpr_25_l
	blr

	DECFN _savefpr_26_l
	blr

	DECFN _savefpr_27_l
	blr

	DECFN _savefpr_28_l
	blr

	DECFN _savefpr_29_l
	blr

	DECFN _savefpr_30_l
	blr

	DECFN _savefpr_31_l
	blr

	DECFN _savefprs_14_l
	blr

	DECFN _savefprs_15_l
	blr

	DECFN _savefprs_16_l
	blr

	DECFN _savefprs_17_l
	blr

	DECFN _savefprs_18_l
	blr

	DECFN _savefprs_19_l
	blr

	DECFN _savefprs_20_l
	blr

	DECFN _savefprs_21_l
	blr

	DECFN _savefprs_22_l
	blr

	DECFN _savefprs_23_l
	blr

	DECFN _savefprs_24_l
	blr

	DECFN _savefprs_25_l
	blr

	DECFN _savefprs_26_l
	blr

	DECFN _savefprs_27_l
	blr

	DECFN _savefprs_28_l
	blr

	DECFN _savefprs_29_l
	blr

	DECFN _savefprs_30_l
	blr

	DECFN _savefprs_31_l
	blr

	DECFN _savef14
	blr

	DECFN _savef15
	blr

	DECFN _savef16
	blr

	DECFN _savef17
	blr

	DECFN _savef18
	blr

	DECFN _savef19
	blr

	DECFN _savef20
	blr

	DECFN _savef21
	blr

	DECFN _savef22
	blr

	DECFN _savef23
	blr

	DECFN _savef24
	blr

	DECFN _savef25
	blr

	DECFN _savef26
	blr

	DECFN _savef27
	blr

	DECFN _savef28
	blr

	DECFN _savef29
	blr

	DECFN _savef30
	blr

	DECFN _savef31
	blr

	DECFN _savegpr_14
	blr

	DECFN _savegpr_15
	blr

	DECFN _savegpr_16
	blr

	DECFN _savegpr_17
	blr

	DECFN _savegpr_18
	blr

	DECFN _savegpr_19
	blr

	DECFN _savegpr_20
	blr

	DECFN _savegpr_21
	blr

	DECFN _savegpr_22
	blr

	DECFN _savegpr_23
	blr

	DECFN _savegpr_24
	blr

	DECFN _savegpr_25
	blr

	DECFN _savegpr_26
	blr

	DECFN _savegpr_27
	blr

	DECFN _savegpr_28
	blr

	DECFN _savegpr_29
	blr

	DECFN _savegpr_30
	blr

	DECFN _savegpr_31
	blr

	DECFN _savegpr_14_l
	blr

	DECFN _savegpr_15_l
	blr

	DECFN _savegpr_16_l
	blr

	DECFN _savegpr_17_l
	blr

	DECFN _savegpr_18_l
	blr

	DECFN _savegpr_19_l
	blr

	DECFN _savegpr_20_l
	blr

	DECFN _savegpr_21_l
	blr

	DECFN _savegpr_22_l
	blr

	DECFN _savegpr_23_l
	blr

	DECFN _savegpr_24_l
	blr

	DECFN _savegpr_25_l
	blr

	DECFN _savegpr_26_l
	blr

	DECFN _savegpr_27_l
	blr

	DECFN _savegpr_28_l
	blr

	DECFN _savegpr_29_l
	blr

	DECFN _savegpr_30_l
	blr

	DECFN _savegpr_31_l
	blr

	DECFN _savegprs_14_l
	blr

	DECFN _savegprs_15_l
	blr

	DECFN _savegprs_16_l
	blr

	DECFN _savegprs_17_l
	blr

	DECFN _savegprs_18_l
	blr

	DECFN _savegprs_19_l
	blr

	DECFN _savegprs_20_l
	blr

	DECFN _savegprs_21_l
	blr

	DECFN _savegprs_22_l
	blr

	DECFN _savegprs_23_l
	blr

	DECFN _savegprs_24_l
	blr

	DECFN _savegprs_25_l
	blr

	DECFN _savegprs_26_l
	blr

	DECFN _savegprs_27_l
	blr

	DECFN _savegprs_28_l
	blr

	DECFN _savegprs_29_l
	blr

	DECFN _savegprs_30_l
	blr

	DECFN _savegprs_31_l
	blr

	DECFN SuperFreeRawMem
	blr

	DECFN BytesMemMapped
	blr

	DECFN AllocDMAChannel
	blr

	DECFN FreeDMAChannel
	blr

	DECFN StartDMA
	blr

	DECFN AbortDMA
	blr

	DECFN SuperSlaveRequest
	blr

	DECFN InstallMasterHandler
	blr

	DECFN ServiceMasterExceptions
	blr

	DECFN SlaveExit
	blr

	DECFN CreateTask
	blr

	DECFN CreateModuleThread
	blr

	DECFN CreateThread
	blr

	DECFN ClearInterrupt
	blr

	DECFN CopyDeviceStack
	blr

	DECFN GetTEReadPointer
	blr

	DECFN SetTEReadPointer
	blr

	DECFN GetTEWritePointer
	blr

	DECFN SetTEWritePointer
	blr

	DECFN WaitLumberjackBuffer
	blr

	DECFN SafeFirstTagArg
	blr

	DECFN SafeNextTagArg
	blr

	DECFN ReinitDevices
	blr

	DECFN ObtainSpinLock
	blr

	DECFN ReleaseSpinLock
	blr

	DECFN SuperSetSystemTimeTT
	blr

	DECFN Dbgr_MPIOReqCreated
	blr

	DECFN Dbgr_MPIOReqDeleted
	blr

	DECFN IncreaseResourceTable
	blr

