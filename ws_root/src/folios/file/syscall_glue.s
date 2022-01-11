/* @(#) syscall_glue.s 96/11/13 1.6 */

#include <hardware/PPCMacroequ.i>


/*****************************************************************************/


/* Calls the system call handler, the correct arguments are already in
 * the registers (r3 = folio/selector code, r4-r10 has the arguments)
 * We just do an sc and a return.
 */
	.macro
	SYSTEMCALL	&number

#ifdef BUILD_DEBUGGER
	mflr		r10
	stw		r10,4(r1)
#endif

	lis		r0,(((&number)>>16) & 0xFFFF)
	ori		r0,r0,((&number) & 0xFFFF)
	sc
	.endm

FILEFOLIOSWI	.equ 0x030000


/*****************************************************************************/


	DECFN	FormatFileSystem
	SYSTEMCALL	FILEFOLIOSWI+24

	DECFN	SetMountLevel
	SYSTEMCALL	FILEFOLIOSWI+23

	DECFN	Rename
	SYSTEMCALL	FILEFOLIOSWI+22

	DECFN	MinimizeFileSystem
	SYSTEMCALL	FILEFOLIOSWI+21

	DECFN	RecheckAllFileSystems
	SYSTEMCALL	FILEFOLIOSWI+20

	DECFN	MountAllFileSystems
	SYSTEMCALL	FILEFOLIOSWI+19

	DECFN	FindFileAndIdentify
	SYSTEMCALL	FILEFOLIOSWI+18

	DECFN	FindFileAndOpen
	SYSTEMCALL	FILEFOLIOSWI+17

	DECFN	DeleteDirectoryInDir
	SYSTEMCALL	FILEFOLIOSWI+16

	DECFN	CreateDirectoryInDir
	SYSTEMCALL	FILEFOLIOSWI+15

	DECFN	DeleteFileInDir
	SYSTEMCALL	FILEFOLIOSWI+14

	DECFN	CreateFileInDir
	SYSTEMCALL	FILEFOLIOSWI+13

	DECFN	ChangeDirectoryInDir
	SYSTEMCALL	FILEFOLIOSWI+12

	DECFN	DeleteDirectory
	SYSTEMCALL	FILEFOLIOSWI+11

	DECFN	CreateDirectory
	SYSTEMCALL	FILEFOLIOSWI+10

	DECFN	DismountFileSystem
	SYSTEMCALL	FILEFOLIOSWI+9

	DECFN	CreateAlias
	SYSTEMCALL	FILEFOLIOSWI+8

	DECFN	DeleteFile
	SYSTEMCALL	FILEFOLIOSWI+7

	DECFN	CreateFile
	SYSTEMCALL	FILEFOLIOSWI+6

	DECFN	GetDirectory
	SYSTEMCALL	FILEFOLIOSWI+5

	DECFN	ChangeDirectory
	SYSTEMCALL	FILEFOLIOSWI+4

	DECFN	OpenFileInDir
	SYSTEMCALL	FILEFOLIOSWI+3

	DECFN	MountFileSystem
	SYSTEMCALL	FILEFOLIOSWI+2

	DECFN	CloseFile
	SYSTEMCALL	FILEFOLIOSWI+1

	DECFN	OpenFile
	SYSTEMCALL	FILEFOLIOSWI+0
