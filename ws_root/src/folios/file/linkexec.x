! @(#) linkexec.x 96/09/04 1.18
!
! The Filesystem exports
!
MODULE 3
!
! SWI wrappers
!
EXPORTS 0=OpenFile
EXPORTS 1=CloseFile
EXPORTS 2=FormatFileSystem
EXPORTS 3=FormatFileSystemVA
EXPORTS 4=MountFileSystem
EXPORTS 5=OpenFileInDir
! EXPORTS 6=NULL
EXPORTS 7=ChangeDirectory
EXPORTS 8=GetDirectory
EXPORTS 9=CreateFile
EXPORTS 10=DeleteFile
EXPORTS 11=CreateAlias
! EXPORTS 12=NULL
EXPORTS 13=DismountFileSystem
! EXPORTS 14=NULL
EXPORTS 15=CreateDirectory
EXPORTS 16=DeleteDirectory
EXPORTS 17=ChangeDirectoryInDir
EXPORTS 18=CreateFileInDir
EXPORTS 19=DeleteFileInDir
EXPORTS 20=CreateDirectoryInDir
EXPORTS 21=DeleteDirectoryInDir
EXPORTS 22=FindFileAndOpen
EXPORTS 23=FindFileAndIdentify
EXPORTS 24=MountAllFileSystems
EXPORTS 25=RecheckAllFileSystems
EXPORTS 26=MinimizeFileSystem
EXPORTS 27=Rename
!
! User functions
!
EXPORTS 28=GetUnmountedList
EXPORTS 29=DeleteUnmountedList
! EXPORTS 30=NULL
EXPORTS 31=OpenDirectoryItem
EXPORTS 32=OpenDirectoryPath
EXPORTS 33=ReadDirectory
EXPORTS 34=CloseDirectory
EXPORTS 35=OpenRawFile
EXPORTS 36=CloseRawFile
EXPORTS 37=ReadRawFile
EXPORTS 38=WriteRawFile
EXPORTS 39=SeekRawFile
EXPORTS 40=GetRawFileInfo
EXPORTS 41=SetRawFileAttrs
EXPORTS 42=SetFileAttrs
EXPORTS 43=ClearRawFileError
EXPORTS 44=SetRawFileSize
EXPORTS 45=OpenRawFileInDir
EXPORTS 46=SetRawFileAttrsVA
EXPORTS 47=SetFileAttrsVA
EXPORTS 48=FindFileAndIdentifyVA
EXPORTS 49=FindFileAndOpenVA
EXPORTS 50=StartStatusRequest
EXPORTS 51=StatusEndAction
!EXPORTS 52=NULL
EXPORTS 53=fileFolio
EXPORTS 54=RelinquishCachePages
EXPORTS 55=RelinquishCachePage
EXPORTS 56=ReserveCachePages
EXPORTS 57=InvalidateFilesystemCachePages
EXPORTS 58=OpenFileSWI
EXPORTS 59=LoadBlockIntoCache
EXPORTS 60=FinishDismount
EXPORTS 61=fsListSemaphore
EXPORTS 62=PlugNPlay
EXPORTS 63=SetMountLevel
EXPORTS 64=GetMountLevel
!
! Preliminary init decomposition added by Kevin
!
EXPORTS 65=awakenSleepers
EXPORTS 66=fsOpenForBusiness
EXPORTS 67=FileDaemonInternals
EXPORTS 68=fileFolioTags
EXPORTS 69=DeleteIOReqItem
EXPORTS 70=CleanupOpenFile
EXPORTS 71=CleanupOnClose
EXPORTS 72=FileDriverDispatch
EXPORTS 73=FileDriverAbortIo
! All this init stuff should be in the init module
EXPORTS 74=InitDirSema
EXPORTS 75=InitHost
EXPORTS 76=InitAcrobat
!EXPORTS 77=NULL
EXPORTS 78=InitOpera
!EXPORTS 79=NULL
EXPORTS 80=fileDriver
! more decomposition
EXPORTS 81=fileFolioErrorTags
EXPORTS 82=FileFolioCreateTaskHook
EXPORTS 83=DeleteFileTask
EXPORTS 84=SetFileItemOwner
EXPORTS 85=DeleteFileItem
EXPORTS 86=CreateFileItem
EXPORTS 87=fsCache
EXPORTS 88=fsCacheEntryArray
EXPORTS 89=fsCacheEntrySize
EXPORTS 90=fsCacheBase
EXPORTS 91=fsCacheSize
EXPORTS 92=fsDaemonStack
EXPORTS 93=fsDaemonStackSize
EXPORTS 94=FileSystemMountedOn
EXPORTS 95=SuperFindFileAndIdentify
EXPORTS 96=InvalidateCachePage
