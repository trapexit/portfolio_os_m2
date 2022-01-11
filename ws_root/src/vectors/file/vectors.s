#include <hardware/PPCMacroequ.i>

	DECFN main
	blr

	DECFN OpenFile
	blr

	DECFN CloseFile
	blr

	DECFN FormatFileSystem
	blr

	DECFN FormatFileSystemVA
	blr

	DECFN MountFileSystem
	blr

	DECFN OpenFileInDir
	blr

	DECFN ChangeDirectory
	blr

	DECFN GetDirectory
	blr

	DECFN CreateFile
	blr

	DECFN DeleteFile
	blr

	DECFN CreateAlias
	blr

	DECFN DismountFileSystem
	blr

	DECFN CreateDirectory
	blr

	DECFN DeleteDirectory
	blr

	DECFN ChangeDirectoryInDir
	blr

	DECFN CreateFileInDir
	blr

	DECFN DeleteFileInDir
	blr

	DECFN CreateDirectoryInDir
	blr

	DECFN DeleteDirectoryInDir
	blr

	DECFN FindFileAndOpen
	blr

	DECFN FindFileAndIdentify
	blr

	DECFN MountAllFileSystems
	blr

	DECFN RecheckAllFileSystems
	blr

	DECFN MinimizeFileSystem
	blr

	DECFN Rename
	blr

	DECFN GetUnmountedList
	blr

	DECFN DeleteUnmountedList
	blr

	DECFN OpenDirectoryItem
	blr

	DECFN OpenDirectoryPath
	blr

	DECFN ReadDirectory
	blr

	DECFN CloseDirectory
	blr

	DECFN OpenRawFile
	blr

	DECFN CloseRawFile
	blr

	DECFN ReadRawFile
	blr

	DECFN WriteRawFile
	blr

	DECFN SeekRawFile
	blr

	DECFN GetRawFileInfo
	blr

	DECFN SetRawFileAttrs
	blr

	DECFN SetFileAttrs
	blr

	DECFN ClearRawFileError
	blr

	DECFN SetRawFileSize
	blr

	DECFN OpenRawFileInDir
	blr

	DECFN SetRawFileAttrsVA
	blr

	DECFN SetFileAttrsVA
	blr

	DECFN FindFileAndIdentifyVA
	blr

	DECFN FindFileAndOpenVA
	blr

	DECFN StartStatusRequest
	blr

	DECFN StatusEndAction
	blr

	DECFN fileFolio
	blr

	DECFN RelinquishCachePages
	blr

	DECFN RelinquishCachePage
	blr

	DECFN ReserveCachePages
	blr

	DECFN InvalidateFilesystemCachePages
	blr

	DECFN OpenFileSWI
	blr

	DECFN LoadBlockIntoCache
	blr

	DECFN FinishDismount
	blr

	DECFN fsListSemaphore
	blr

	DECFN PlugNPlay
	blr

	DECFN SetMountLevel
	blr

	DECFN GetMountLevel
	blr

	DECFN awakenSleepers
	blr

	DECFN fsOpenForBusiness
	blr

	DECFN FileDaemonInternals
	blr

	DECFN fileFolioTags
	blr

	DECFN DeleteIOReqItem
	blr

	DECFN CleanupOpenFile
	blr

	DECFN CleanupOnClose
	blr

	DECFN FileDriverDispatch
	blr

	DECFN FileDriverAbortIo
	blr

	DECFN InitDirSema
	blr

	DECFN InitHost
	blr

	DECFN InitAcrobat
	blr

	DECFN InitOpera
	blr

	DECFN fileDriver
	blr

	DECFN fileFolioErrorTags
	blr

	DECFN FileFolioCreateTaskHook
	blr

	DECFN DeleteFileTask
	blr

	DECFN SetFileItemOwner
	blr

	DECFN DeleteFileItem
	blr

	DECFN CreateFileItem
	blr

	DECFN fsCache
	blr

	DECFN fsCacheEntryArray
	blr

	DECFN fsCacheEntrySize
	blr

	DECFN fsCacheBase
	blr

	DECFN fsCacheSize
	blr

	DECFN fsDaemonStack
	blr

	DECFN fsDaemonStackSize
	blr

	DECFN FileSystemMountedOn
	blr

	DECFN SuperFindFileAndIdentify
	blr

	DECFN InvalidateCachePage
	blr

