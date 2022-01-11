/* @(#) microlook.c 96/09/25 1.25 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/msgport.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <kernel/time.h>
#include <dipir/hwresource.h>
#include <file/discdata.h>
#include <file/filesystem.h>
#include <file/filesystemdefs.h>
#include <file/acrofs.h>
#include <file/filefunctions.h>
#include <international/intl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#undef	DEBUG
#define		TBG(x)	printf x
#ifdef		DEBUG
#define		DBG(x)	printf x
#else		/* DEBUG */
#define		DBG(x)	/* x */
#endif		/* DEBUG */

#define	FORMAT_CMD	"/remote/System.m2/Programs/format -fstype acrobat.fs"
#define	LIGHT_LOAD	0x0
#define	HEAVY_LOAD	0x1
#define	READ_LOAD	0x2

#define	USAGE		printf("USAGE: %s [-f]\n", pname);

#define WAITTASK(itemid)	\
		{ \
			while (LookupItem(itemid)) { \
				WaitSignal(SIGF_DEADTASK); \
			} \
		}


char	*sub_slash(char *cp);
void	run_tests(char *devName, uint32 offset, char *mountpnt);
int32	loadup(char *mntpnt);
void	hang_loose(uint32 times);

Err	create(char *path, uint32 size, uint32 offset, uint32 val, uint32 flg);
Err	rdwr(char *path, uint32 size, uint32 offset, uint32 val, uint32 flg);
Err	rm(char *path, uint32 size, uint32 offset, uint32 val, uint32 flg);
Err	mkdir(char *path, uint32 size, uint32 offset, uint32 val, uint32 flg);
Err	rmdir(char *path, uint32 size, uint32 offset, uint32 val, uint32 flg);

typedef	struct load {
	uint32	l_flg;
	Err	(*l_cmd) (char *path, uint32 size, uint32 offset,
			  uint32 val, uint32 flg);
	char	*l_path;
	uint32	l_size;
	uint32	l_offset;
	uint32	l_val;
} load_t;

load_t	ld[] = {
		 {LIGHT_LOAD, create, "/file0", 10, 0, 0},
		 {LIGHT_LOAD, rdwr, "/file0", 10, 0, 0xdeadbeef},
		 {(READ_LOAD|LIGHT_LOAD), rdwr, "/file0", 1, 2, 0xdeadbeef},
		 {LIGHT_LOAD, rm, "/file0", 0, 0, 0},
		 {LIGHT_LOAD, mkdir, "/somedir", 0, 0, 0},
		 {LIGHT_LOAD, rmdir, "/somedir", 0, 0, 0},
		 /* heavy load starts here */
		 {HEAVY_LOAD, mkdir, "/smalldir", 0, 0, 0},
		 {HEAVY_LOAD, mkdir, "/fragdir", 0, 0, 0},
		 {HEAVY_LOAD, mkdir, "/bigdir", 0, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/f0", 1, 0, 0},
		 {HEAVY_LOAD, create, "/smalldir/file1", 10, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/f1", 1, 0, 0},
		 {HEAVY_LOAD, create, "/smalldir/file2", 13, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/f2", 1, 0, 0},
		 {HEAVY_LOAD, create, "/smalldir/indirfile", 2, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/f3", 1, 0, 0},
		 {HEAVY_LOAD, create, "/smalldir/indirfile", 3, 0, 0},
		 {HEAVY_LOAD, create, "/smalldir/indirfile", 5, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/f4", 1, 0, 0},
		 {HEAVY_LOAD, create, "/smalldir/indirfile", 10, 0, 0},
		 {HEAVY_LOAD, create, "/smalldir/indirfile", 20, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/f5", 1, 0, 0},
		 {HEAVY_LOAD, create, "/smalldir/indirfile", 30, 0, 0},
		 {HEAVY_LOAD, create, "/smalldir/indirfile", 50, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/one", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/two", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/three", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/four", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/five", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/six", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/seven", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/eight", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/nine", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/ten", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/eleven", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/twelve", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/thirteen", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/fourteen", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/fifteen", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/sixteen", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/seventeen", 0, 0, 0},
		 {HEAVY_LOAD, create, "/bigdir/eighteen", 0, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/indirfile", 1, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/indirfile", 2, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/indirfile", 3, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/f6", 1, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/indirfile", 5, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/indirfile", 10, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/f7", 1, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/indirfile", 20, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/indirfile", 30, 0, 0},
		 {HEAVY_LOAD, create, "/fragdir/f8", 1, 0, 0},
		 {HEAVY_LOAD, rm, "/smalldir/file1", 0, 0, 0},
		 {HEAVY_LOAD, rm, "/fragdir/f0", 0, 0, 0},
		 {HEAVY_LOAD, rm, "/fragdir/f1", 0, 0, 0},
		 {HEAVY_LOAD, rm, "/fragdir/f2", 0, 0, 0},
		 {HEAVY_LOAD, rm, "/fragdir/f4", 0, 0, 0},
		 {HEAVY_LOAD, rm, "/fragdir/f6", 0, 0, 0},
		 {HEAVY_LOAD, rm, "/fragdir/f8", 0, 0, 0},
		 {0, NULL, NULL, 0, 0, 0}		/* last entry */
	};

typedef	struct devinfo {
	char	*d_hwnm;
	char	*d_devnm;
	uint32	d_offset;
	char	*d_mountpnt;
}devinfo_t;

devinfo_t	devs[] = {
			{NULL, "ramdisk", 0, "/acrobat_ram"},
			{"Microcard00", "storagecard", 0, "/acrobat_micro"},
			{NULL, 0, 0, NULL}	/* last entry */
		};

int	full = 0;
char	*pname;

int
main(int ac, char *av[])
{
	uint32	i, j;
	char	devName[40];
	char	mountnm[ FILESYSTEM_MAX_NAME_LEN];
	HWResource hwr;

	pname = av[0];

	if (ac > 2) {
		printf("%s: Invalid number of arguments\n", pname);
		USAGE;
		return 1;
	}

	if (ac == 2) {
		if (strcmp(av[1], "-f")) {
			printf("%s: Invalid option - %s\n", pname, av[1]);
			USAGE;
			return 1;
		} else {
			full++;
		}
	}

	/* First run all the ones that don't have HWResources. */
	for (i = 0; devs[i].d_devnm; i++) {
		if (devs[i].d_hwnm == NULL)
		{
			run_tests(devs[i].d_devnm, devs[i].d_offset, 
				devs[i].d_mountpnt);
		}
	}

	/* Now run all the ones that have HWResources. */
	for (i = 0; devs[i].d_devnm; i++) {
		if (devs[i].d_hwnm == NULL)
			continue;
		for (j = hwr.hwr_InsertID = 0; NextHWResource(&hwr) >= 0;
		     j++) {
			if (!MatchDeviceName(devs[i].d_hwnm, hwr.hwr_Name,
						DEVNAME_TYPE))
				continue;
			sprintf(devName, "%s,%d",
				devs[i].d_devnm, hwr.hwr_InsertID);
			sprintf(mountnm, "%s%d",
				devs[i].d_mountpnt, j);
			run_tests(devName, devs[i].d_offset, mountnm);
		}
	}
	return 0;
}


#define	MAX_CMDLEN	128
void
run_tests(char *devName, uint32 offset, char *mountpnt)
{
	Item		ti, fsi;
	Err		err;
	char		buf[MAX_CMDLEN];
	FileSystem	*fsp;
	int32		errs;

	printf("***** Testing: %s %d %s\n", devName, offset, mountpnt);

	if ((fsi = NamedDeviceStackMounted(devName)) != 0) {
		if (fsi < 0) {
                	PrintError(0,"NamedDeviceStckMounted", devName,fsi);
			return;
		}
		fsp = (FileSystem *) LookupItem(fsi);
		DBG(("%s: Dismounting \"%s\" from \"%s\"\n",
			pname, fsp->fs_FileSystemName, devName));
		if ((err = DismountFileSystem(fsp->fs_FileSystemName)) < 0) {
			TOUCH(err);
		}
	}
	DBG(("DISMOUNTED 0\n"));
	hang_loose(40);
	/* format */
	memset(buf, 0, MAX_CMDLEN);
	sprintf(buf, "%s %s %s", FORMAT_CMD, devName, sub_slash(mountpnt));
	if ((ti = system(buf)) != 0) {
		printf("***** ERROR: (0x%x) FAILED to %s\n", ti, buf);
		return;
	}
	DBG(("FORMATED\n"));
	hang_loose(100);

	errs = loadup(mountpnt);

	/* unmount */
	if ((err = DismountFileSystem(mountpnt)) < 0) {
		TOUCH(err);
		/* PrintError(0,"Dismount",mountpnt,err); */
	}
	DBG(("DISMOUNTED 1\n"));
	hang_loose(40);

#if 0
	/* check for validity */
	memset(buf, 0, MAX_CMDLEN);
	sprintf(buf, "%s %s %d %s", ADMIN_CMD,
		     devName, offset, sub_slash(mountpnt));
	if ((ti = system(buf)) != 0)
		printf("***** ERROR: (0x%x) FAILED TO %s\n", ti, buf);
	else
		printf("***** TEST COMPLETED WITHOUT ERRORS FOR %s\n",

	DBG(("CHECKED\n"));
done:
	CloseDeviceStack(di);
#endif
	printf("***** Test of %s completed with %d errors\n\n", devName, errs);
}


int32
loadup(char *mntpnt)
{
	int	i;
	char	fullpath[FILESYSTEM_MAX_PATH_LEN];
	load_t	*lp;

	DBG(("loadup: %s, full %d\n", mntpnt, full));

	for (i = 0; ld[i].l_cmd; i++) {
		if (!full && (ld[i].l_flg & HEAVY_LOAD))
			continue;
		lp = &ld[i];
		memset(fullpath, 0, FILESYSTEM_MAX_PATH_LEN);
		strncpy(fullpath, mntpnt, FILESYSTEM_MAX_PATH_LEN);
		strncat(fullpath, lp->l_path, FILESYSTEM_MAX_PATH_LEN);
		DBG(("loadup: %d\n", i));
		if ((*ld[i].l_cmd)(fullpath, lp->l_size, lp->l_offset,
				   lp->l_val, lp->l_flg)) {
			printf("***** ERROR: TEST %d FAILED FOR %s\n", i,
				mntpnt);
			return 1;
		}
	}
	return 0;
}


char *
sub_slash(char *cp)
{
	if (*cp != '/')
		return(cp);

	while (*cp++ == '/')
		;
	return(cp - 1);
}




Err
create(char *path, uint32 size, uint32 offset, uint32 val, uint32 flg)
{
	Err	err = 0;
	Item	ofi, iori = 0;
	IOInfo	ioinf;

	TOUCH(iori), TOUCH(err), TOUCH(flg), TOUCH(offset), TOUCH(val);
	DBG(("create: %s, %d, %d, 0x%x, 0x%x\n", path, size, offset, val, flg));

	DBG(("create: opening %s\n", path));
	if ((ofi = OpenDiskFile(path)) < 0) {
		DBG(("create: creating %s\n", path));
		if ((err = CreateFile(path)) < 0) {
			PrintError(0,"create file",path,err);
                	return err;
		}
		err = 0;
		DBG(("create: opening %s\n", path));
		if ((ofi = OpenDiskFile(path)) < 0) {
			PrintError(0,"open",path,ofi);
 			return ofi;
		}
	}

	if (size == 0)		/* for zero size file, no alloc block needed */
		goto done;

	DBG(("create: creating ioreq %s\n", path));
	if ((iori = CreateIOReq(NULL, 50, ofi, 0)) < 0) {
		PrintError(0,"create IOReq",0,iori);
		CloseDiskFile(ofi);
		return iori;
	}

	memset(&ioinf, 0, sizeof ioinf);

	ioinf.ioi_Command = FILECMD_ALLOCBLOCKS;
	ioinf.ioi_Recv.iob_Buffer = NULL;
	ioinf.ioi_Recv.iob_Len = 0;
	ioinf.ioi_Offset = size;
	DBG(("create: allocating %d blocks for %s\n", size, path));
	if ((err = DoIO(iori, &ioinf)) < 0) {
		PrintError(0,"allocate blocks",0,err);
		goto done;
	}
	err = 0;

done:
	if (iori)
		DeleteIOReq(iori);
	CloseDiskFile(ofi);
	DBG(("OUT create\n"));
	return err;
}


Err
rdwr(char *path, uint32 size, uint32 offset, uint32 val, uint32 flg)
{
	Err		err = 0;
	Item		ofi, iori = 0;
	IOInfo		ioinf;
	int		i;
	char		*bp = NULL;
	uint32		*uintp;
	FileStatus	stat;

	TOUCH(err), TOUCH(iori), TOUCH(bp);
	DBG(("rdwr: %s, %d, %d, 0x%x, 0x%x\n", path, size, offset, val, flg));
	DBG(("rdwr: opening file %s\n", path));
	if ((ofi = OpenDiskFile(path)) < 0) {
		PrintError(0,"open",path,ofi);
 		return ofi;
	}
	DBG(("rdwr: creating ioreq for %s\n", path));
	if ((iori = CreateIOReq(NULL, 50, ofi, 0)) < 0) {
		PrintError(0,"create IOReq",0,iori);
		CloseDiskFile(ofi);
 		return iori;
	}

	memset(&ioinf, 0, sizeof(IOInfo));
	ioinf.ioi_Recv.iob_Buffer = &stat;
	ioinf.ioi_Recv.iob_Len = sizeof (FileStatus);
	ioinf.ioi_Command = CMD_STATUS;
	DBG(("rdwr: stating %s\n", path));
	if ((err = DoIO(iori, &ioinf)) < 0) {
                PrintError(0, "get status", path, err);
		goto done;
	}

	size *= stat.fs.ds_DeviceBlockSize;
	DBG(("rdwr: allocated %d bytes buffer\n", size));
	if ((bp = (char *) AllocMem(size, MEMTYPE_DMA)) == NULL) {
		printf("%s: rdwr: Failed to AllocMem  %d\n",
			 pname, size);
		err = NOMEM;
		goto done;
	}

	memset(&ioinf, 0, sizeof(IOInfo));
	if (flg & READ_LOAD) {
		ioinf.ioi_Command = CMD_BLOCKREAD;
		ioinf.ioi_Send.iob_Buffer = NULL;
        	ioinf.ioi_Send.iob_Len = 0;
		ioinf.ioi_Recv.iob_Buffer = bp;
        	ioinf.ioi_Recv.iob_Len = size;
	} else {	/* write */
		uintp = (uint32 *) bp;
		for (i = 0; i < size; i += 4, uintp++)
			*uintp = val;
		ioinf.ioi_Command = CMD_BLOCKWRITE;
		ioinf.ioi_Recv.iob_Buffer = NULL;
        	ioinf.ioi_Recv.iob_Len = 0;
		ioinf.ioi_Send.iob_Buffer = bp;
        	ioinf.ioi_Send.iob_Len = size;
	}
	ioinf.ioi_Offset = offset;
	if ((err = DoIO(iori, &ioinf)) < 0) {
		PrintError(0,(flg & READ_LOAD)?"read":"write",path,err);
		goto done;
	}

	if (flg & READ_LOAD) {
		uintp = (uint32 *) bp;
		for (i = 0; i < size; i += 4, uintp++) {
			if (*uintp != val) {
				printf("%s: rdwr: read error at word %d\n",
					pname, i);
				err = -1;
				break;
			}
		}
#ifdef	DEBUG
		uintp = (uint32 *) bp;
		for (i = 0; i < size; i += 4, uintp++) {
			printf("%x ", (uint32) *uintp);
			if ((i != 0) && ((i % 40) == 0))
				putchar('\n');
		}
		putchar('\n');
#endif	/* DEBUG */
	}

done:
	if(bp)
		FreeMem(bp, size);
	if (iori)
		DeleteIOReq(iori);
	CloseDiskFile(ofi);
	return err;
}


Err
rm(char *path, uint32 size, uint32 offset, uint32 val, uint32 flg)
{
	Err	err;

	TOUCH(size),TOUCH(offset), TOUCH(val), TOUCH(flg);
	DBG(("rm: %s, %d, %d, 0x%x, 0x%x\n", path, size, offset, val, flg));

	if ((err = DeleteFile(path)) < 0) {
		PrintError(0,"delete file",path,err);
 		return err;
	}
	return 0;
}


Err
mkdir(char *path, uint32 size, uint32 offset, uint32 val, uint32 flg)
{
	Err	err;

	TOUCH(size),TOUCH(offset), TOUCH(val), TOUCH(flg);
	DBG(("mkdir: %s, %d, %d, 0x%x, 0x%x\n", path, size, offset, val, flg));

	if ((err = CreateDirectory(path)) < 0) {
		PrintError(0,"create directory",path,err);
               	return err;
	}
	return 0;
}


Err
rmdir(char *path, uint32 size, uint32 offset, uint32 val, uint32 flg)
{
	Err	err;

	TOUCH(size),TOUCH(offset), TOUCH(val), TOUCH(flg);
	DBG(("rmdir: %s, %d, %d, 0x%x, 0x%x\n", path, size, offset, val, flg));

	if ((err = DeleteDirectory(path)) < 0) {
		PrintError(0,"delete directory",path,err);
               	return err;
	}
	return 0;
}


/*
 *	XXX - FIXME
 *	This should go away after I re desiagned the mounting stuff, after
 *	DDL stuff is checked in.
 */
void
hang_loose(uint32 times)
{
	Item	ioReqItem;
	IOInfo	timerCmd;
	Err	err;

	ioReqItem = CreateTimerIOReq();
	if (ioReqItem < 0) {
		PrintError(0,"CreateTimerIOReq", 0, ioReqItem);
		return;
	}
	memset(&timerCmd, 0, sizeof timerCmd);
	timerCmd.ioi_Command = TIMERCMD_DELAY_VBL;
	timerCmd.ioi_Offset = times;
	DBG(("hang_Loose: Doing timer IO\n"));
	if ((err = DoIO(ioReqItem, &timerCmd)) < 0)
	{
		PrintError(0,"DoIO Timer", 0, err);
#ifndef	BUILD_STRINGS
		TOUCH(err);
#endif
	}
	DeleteTimerIOReq(ioReqItem);
	DBG(("hang_Loose: timer done\n"));
}
