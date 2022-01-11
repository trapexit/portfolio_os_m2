/* @(#) compare.c 96/09/23 1.1 */

/**
|||	AUTODOC -class Shell_Commands -name compare
|||	Compares files
|||
|||	  Format
|||
|||	    compare <file1> <file2>
|||
|||	  Description
|||
|||	    Compares the contents of two files.  If the files are not
|||	    identical, compare prints the first differing byte and quits.
|||
|||	  Implementation
|||
|||	    Command implemented in V28.
|||
|||	  Location
|||
|||	    System.m2/Programs/compare
|||
**/

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/directory.h>
#include <file/directoryfunctions.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#undef min
#define min(a,b)	(((a) < (b)) ? (a) : (b))

/* #define	DEBUG */

#ifdef	DEBUG
#define	DBG(x)	printf x
#else	/* DEBUG */
#define	DBG(x)	/* x */
#endif	/* DEBUG */

#define	STAT_NOTHING	1
#define	STAT_FILE	2
#define	STAT_DIR	3
#define	STAT_FAIL	4


char	*pname;

void
usage(void)
{
	printf("USAGE: %s file1 file2\n", pname);
	exit(2);
}


int
CheckType(char *file, int typ)
{
	switch (typ)
	{
	case STAT_FILE:
		return 0;
	case STAT_FAIL:
		printf("%s: failed to get status of %s\n", pname, file);
		break;
	case STAT_NOTHING:
		printf("%s: %s does not exist\n", pname, file);
		break;
	case STAT_DIR:
		printf("%s: %s is a directory\n", pname, file);
		break;
	default:
		printf("%s: %s returned unknown status 0x%x\n", pname, file, typ);
		break;
	}
	return 1;
}

int
stat(char *fpath, FileStatus *fstatp)
{
	Item		fi, fioreqi = 0;
	IOInfo		finfo;
	Err		err;
	FileStatus	fstat;

	TOUCH(fioreqi);

	if (fstatp == NULL)
		fstatp = &fstat;

	if ((fi = OpenFile(fpath)) < 0) {
		return STAT_NOTHING;
        }

	if ((fioreqi = CreateIOReq(NULL, 50, fi, 0)) < 0) {
		PrintError(0, "stat: create IOReq", fpath, fioreqi);
		goto failed;
	}

	memset(&finfo, 0, sizeof(IOInfo));
	finfo.ioi_Recv.iob_Buffer = (char *) fstatp;
	finfo.ioi_Recv.iob_Len = sizeof (FileStatus);
	finfo.ioi_Command = CMD_STATUS;
	DBG(("stat: %s\n", fpath));
	if ((err = DoIO(fioreqi, &finfo)) < 0) {
                PrintError(0, "stat: status", fpath, err);
		goto failed;
	}

	DeleteIOReq(fioreqi);
	CloseFile(fi);
	return (fstatp->fs_Flags & FILE_IS_DIRECTORY) ? STAT_DIR : STAT_FILE;

failed:
	if (fioreqi)
		DeleteIOReq(fioreqi);
	CloseFile(fi);
	return STAT_FAIL;
}

Err 
CompareData(char *file1, FileStatus *stat1, Item ioreq1,
	    char *file2, FileStatus *stat2, Item ioreq2)
{
	IOInfo	info1, info2;
	uint8	*buf1, *buf2;
	uint32	bufSize1, bufSize2;
	uint32	bufIndex1, bufIndex2;
	uint32	toCompare1, toCompare2, toCompare;
	uint32	totalSize;
	uint32	bytesCompared;
	uint32	i;
	Err	err;

	totalSize = stat1->fs_ByteCount;
	if (stat2->fs_ByteCount < totalSize)
		totalSize = stat2->fs_ByteCount;
	if (stat1->fs_ByteCount != stat2->fs_ByteCount)
	{
		printf("Warning: %s has %d bytes, but %s has %d bytes\n",
			file1, stat1->fs_ByteCount, file2, stat2->fs_ByteCount);
		printf("         Comparing only %d bytes\n", totalSize);
	}
	bufSize1 = stat1->fs.ds_DeviceBlockSize;
	bufSize2 = stat2->fs.ds_DeviceBlockSize;

	buf2 = NULL;
	buf1 = AllocMem(bufSize1, MEMTYPE_NORMAL);
	if (buf1 == NULL)
	{
		printf("CompareData: failed to allocate %d bytes\n", bufSize1);
		err = NOMEM;
		goto Exit;
	}
	buf2 = AllocMem(bufSize2, MEMTYPE_NORMAL);
	if (buf2 == NULL)
	{
		printf("CompareData: failed to allocate %d bytes\n", bufSize2);
		err = NOMEM;
		goto Exit;
	}

	memset(&info1, 0, sizeof(IOInfo));
	info1.ioi_Command = CMD_BLOCKREAD;
	info1.ioi_Recv.iob_Buffer = buf1;
	info1.ioi_Recv.iob_Len = bufSize1;
	info1.ioi_Offset = 0;

	memset(&info2, 0, sizeof(IOInfo));
	info2.ioi_Command = CMD_BLOCKREAD;
	info2.ioi_Recv.iob_Buffer = buf2;
	info2.ioi_Recv.iob_Len = bufSize2;
	info2.ioi_Offset = 0;

	err = 0;
	bufIndex1 = bufSize1;
	bufIndex2 = bufSize2;
	for (bytesCompared = 0;  bytesCompared < totalSize; )
	{
		toCompare1 = bufSize1 - bufIndex1;
		if (toCompare1 > totalSize - bytesCompared)
			toCompare1 = totalSize - bytesCompared;
		if (toCompare1 == 0)
		{
			if ((err = DoIO(ioreq1, &info1)) < 0)
			{
				PrintError(0, "CompareData: read", file1, err);
				goto Exit;
			}
			info1.ioi_Offset++;
			bufIndex1 = 0;
			continue;
		}

		toCompare2 = bufSize2 - bufIndex2;
		if (toCompare2 > totalSize - bytesCompared)
			toCompare2 = totalSize - bytesCompared;
		if (toCompare2 == 0)
		{
			if ((err = DoIO(ioreq2, &info2)) < 0)
			{
				PrintError(0, "CompareData: read", file2, err);
				goto Exit;
			}
			info2.ioi_Offset++;
			bufIndex2 = 0;
			continue;
		}

		toCompare = min(toCompare1, toCompare2);
		for (i = 0;  i < toCompare;  i++)
		{
			if (buf1[i + bufIndex1] != buf2[i + bufIndex2])
			{
				printf("Mismatch at byte %d: %s has 0x%02x, but %s has 0x%02x\n",
					bytesCompared+i, 
					file1, buf1[i+bufIndex1],
					file2, buf2[i+bufIndex2]);
				err = 1;
				goto Exit;
			}
		}
		bytesCompared += toCompare;
		bufIndex1 += toCompare;
		bufIndex2 += toCompare;
	}
	printf("%d bytes compared ok\n", bytesCompared);
Exit:
	FreeMem(buf1, bufSize1);
	if (buf2 != NULL)
		FreeMem(buf2, bufSize2);
	return err;
}

Err
Compare(char *file1, FileStatus *stat1, char *file2, FileStatus *stat2)
{
	Item		ifile1, ifile2;
	Item		ioreq1, ioreq2;
	Err		err;

	ioreq1 = ioreq2 = -1;
	ifile2 = -1;

	if ((ifile1 = OpenFile(file1)) < 0)
	{
		err = ifile1;
                PrintError(0, "Compare: OpenFile", file1, err);
		goto Exit;
	}
	if ((ioreq1 = CreateIOReq(NULL, 50, ifile1, 0)) < 0)
	{
		err = ioreq1;
                PrintError(0, "Compare: CreateIOReq", file1, err);
		goto Exit;
        }
	if ((ifile2 = OpenFile(file2)) < 0)
	{
		err = ifile2;
                PrintError(0, "Compare: OpenFile", file2, err);
		goto Exit;
	}
	if ((ioreq2 = CreateIOReq(NULL, 50, ifile2, 0)) < 0)
	{
		err = ioreq2;
                PrintError(0, "Compare: CreateIOReq", file2, err);
		goto Exit;
        }

	err = CompareData(file1, stat1, ioreq1, file2, stat2, ioreq2);
Exit:
	if (ioreq1 >= 0)
		DeleteIOReq(ioreq1);
	if (ioreq2 >= 0)
		DeleteIOReq(ioreq2);
	if (ifile1 >= 0)
		CloseFile(ifile1);
	if (ifile2 >= 0)
		CloseFile(ifile2);
	return err;
}

int
main(int ac, char *av[])
{
	char		*file1, *file2;
	int		typ1, typ2;
	FileStatus	stat1, stat2;
	Err		err;

	pname = *av;
	if (ac != 3)
		usage();

	file1 = av[1];
	typ1 = stat(file1, &stat1);
	if (CheckType(file1, typ1))
		return 2;

	file2 = av[2];
	typ2 = stat(file2, &stat2);
	if (CheckType(file2, typ2))
		return 2;

	err = Compare(file1, &stat1, file2, &stat2);
	if (err < 0)
		return 2;
	return err;
}

