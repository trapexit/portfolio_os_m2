
#ifdef macintosh
#	pragma segment STAT
#endif

/*********************************************************************
File		:	stat.c			-	Macintosh implementation of stat()
Author	:	Matthias Neeracher
Started	:	28May91								Language	:	MPW C
				28May91	MN	Created
				28May91	MN	isatty()
Last		:	28May91

Copyright (c) 1991, Matthias Neeracher

Permission is granted to anyone to use this software for any
purpose on any computer system, and to redistribute it freely,
subject to the following restrictions:

1. The author is not responsible for the consequences of use of
	this software, no matter how awful, even if they arise
	from defects in it.

2. The origin of this software must not be misrepresented, either
	by explicit claim or by omission.

3. Altered versions must be plainly marked as such, and must not
	be misrepresented as being the original software.

*********************************************************************/

#include <string.h>
#include <ioctl.h>
#include <errno.h>
#include <Errors.h>
#include <Files.h>

#include "tcl.h"
#include "tclUnix.h"

/* As the field names of a CInfoParamBlockRec are spelled inconsistently,
	we have to use manual casts.
*/

#define FILEINFO(cb)	(*(HFileInfo *) (cb))
#define DIRINFO(cb)	(*(DirInfo *) (cb))

OSErr	GetCatInfo(char * path, CInfoPBPtr cb)
{
	char	*	nextPath;
	char	*  tryPath;
	OSErr		err;
	Str255	name;

	DIRINFO(cb).ioNamePtr	=	name;
	DIRINFO(cb).ioDrDirID	=	0;
	DIRINFO(cb).ioVRefNum	=	0;
	DIRINFO(cb).ioFDirIndex	=	0;

	/* handle very long pathnames */

	while (strlen(path) > 255)	{
		for (nextPath	=	path; tryPath = strchr(nextPath, ':'); nextPath = tryPath)
			if (tryPath-nextPath > 31)
				return bdNamErr;
			else if (tryPath-path > 255)
				break;

		if (nextPath == path && !tryPath)
			return bdNamErr;

		name[0]	=	nextPath-path;
		strncpy((char *) name+1, path, name[0]);

		err = PBGetCatInfo(cb, false);
		if (err != noErr)
			return err;

		if (! (DIRINFO(cb).ioFlAttrib & 0x10)) {
			return bdNamErr;
			}

		DIRINFO(cb).ioFDirIndex	=	0;

		path	=	nextPath+1;
	}
	
	name[0]	=	strlen(path);
	strncpy((char *) name+1, path, name[0]);

	err = PBGetCatInfo(cb, false);
	
	return err;
}

OSErr	GetFDCatInfo(int fd, CInfoPBPtr cb)
{
	short				fRef;
	OSErr				err;
	FCBPBRec			fcb;
	Str255			fname;

	if (ioctl(fd, FIOREFNUM, (long *) &fRef) == -1)
		return fnfErr;

	fcb.ioNamePtr	= 	fname;
	fcb.ioRefNum	=	fRef;
	fcb.ioFCBIndx	= 	0;
	if (err = PBGetFCBInfo(&fcb, false))
		return err;

	FILEINFO(cb).ioNamePtr		=	fname;
	FILEINFO(cb).ioDirID		=	fcb.ioFCBParID;
	FILEINFO(cb).ioVRefNum		=	fcb.ioFCBVRefNum;
	FILEINFO(cb).ioFDirIndex	=	0;

	return PBGetCatInfo(cb, false);
}

OSErr GetVolume(char * path, ParmBlkPtr pb)
{
	char *	volEnd;
	OSErr		err;
	WDPBRec	wd;
	Str255	name;

	volEnd	=	strchr(path, ':');

	pb->volumeParam.ioNamePtr	=	name;

	if (path[0] == ':' || ! volEnd)	{
		err = PBGetVol(pb, false);
		if (err != noErr)
			return err;
		
		wd.ioNamePtr	=	name;
		wd.ioVRefNum	= 	pb->volumeParam.ioVRefNum;
		wd.ioWDIndex	=	0;
		err = PBGetWDInfo(&wd, false);
		if (err == noErr)
			pb->volumeParam.ioVRefNum	=	wd.ioWDVRefNum;
			
		pb->volumeParam.ioVolIndex	=	0;
		}
	else {
		pb->volumeParam.ioVolIndex	=	-1;

		name[0]	=	(volEnd - path) + 1; /* include ':' */
		strncpy((char *) name+1, path, name[0]);
		}

	err = PBGetVInfo(pb, false);
	
	return err;
	}

OSErr GetFDVolume(int fd, ParmBlkPtr pb)
{
	OSErr				err;
	short				fRef;
	Str255			name;
	FCBPBRec			fcb;

	if (ioctl(fd, FIOREFNUM, (long *) &fRef) == -1)
		return fnfErr;

	fcb.ioNamePtr	= 	name;
	fcb.ioRefNum	=	fRef;
	fcb.ioFCBIndx	= 	0;
	if (err = PBGetFCBInfo(&fcb, false))
		return err;

	pb->volumeParam.ioNamePtr	=	name;
	pb->volumeParam.ioVRefNum	=	fcb.ioFCBVRefNum;
	pb->volumeParam.ioVolIndex	=	0;

	return PBGetVInfo(pb, false);
}

int	mac_do_stat(CInfoPBPtr cb, ParmBlkPtr pb, struct stat * buf)
{

	buf->st_dev		=	pb->ioParam.ioVRefNum;
	buf->st_ino		=	FILEINFO(cb).ioDirID;
	buf->st_nlink	=	1;
	buf->st_uid		=	0;
	buf->st_gid		=	0;
	buf->st_rdev	=	0;
	buf->st_atime	=	FILEINFO(cb).ioFlMdDat;
	buf->st_mtime	=	FILEINFO(cb).ioFlMdDat;
	buf->st_ctime	=	FILEINFO(cb).ioFlCrDat;
	buf->st_blksize=	pb->volumeParam.ioVAlBlkSiz;

	if (FILEINFO(cb).ioFlAttrib & 0x10)	{
		buf->st_mode	=	S_IFDIR | 0777;
		buf->st_size	=	buf->st_blksize;	/* Not known for directories	*/
	} else {
		buf->st_mode	=	S_IFREG | 0666;

		if (FILEINFO(cb).ioFlAttrib & 0x01)
			buf->st_mode &=	0222;

		if (FILEINFO(cb).ioFlFndrInfo.fdType == 'APPL')
			buf->st_mode |=	0111;

		buf->st_size	=	FILEINFO(cb).ioFlLgLen;		/* Resource fork is ignored	*/
	}

	buf->st_blocks	=	(buf->st_size + buf->st_blksize - 1) / buf->st_blksize;

	return 0;
}

int	stat(char * path, struct stat * buf)
{
	int				result;
	CInfoPBRec		cb;
	ParamBlockRec	pb;

	if (GetCatInfo(path, &cb) || GetVolume(path, &pb)) {
		errno = ENOENT;

		return -1;
	} else {
		result = mac_do_stat(&cb, &pb, buf);
		return result;
		}
}

int	lstat(char * path, struct stat * buf)
{
	return stat(path, buf);
}

int	fstat(int fd, struct stat * buf)
{
	CInfoPBRec		cb;
	ParamBlockRec	pb;

	if (!ioctl(fd, FIOINTERACTIVE, NULL))	{	/* MPW window */
		buf->st_dev			=	0;
		buf->st_ino			=	0;
		buf->st_mode		=	S_IFCHR | 0777;
		buf->st_nlink		=	1;
		buf->st_uid			=	0;
		buf->st_gid			=	0;
		buf->st_rdev		=	0;
		buf->st_size		=	1;
		buf->st_atime		=	time(NULL);
		buf->st_mtime		=	time(NULL);
		buf->st_ctime		=	time(NULL);
		buf->st_blksize	=	1;
		buf->st_blocks		=	1;

		return 0;
	}

	if (GetFDCatInfo(fd, &cb) || GetFDVolume(fd, &pb)) {
		errno = ENOENT;

		return -1;
	} else
		return mac_do_stat(&cb, &pb, buf);
}

access(filename, mode)
char	*filename;
int	mode;
{
CInfoPBRec		cpb;
int				myresult;
char				myname[256], volname[32];
short				vRefNum;

	strncpy(myname, filename, 255);
	myname[255] = '\0';
	c2pstr(myname);
	
	GetVol(volname, &vRefNum);
	
	cpb.hFileInfo.ioCompletion = 0;				/* Synchronous */
	cpb.hFileInfo.ioNamePtr = myname;
	cpb.hFileInfo.ioVRefNum = vRefNum;			/* Returned here */
	cpb.hFileInfo.ioFDirIndex = 0;				/* Use ioNamePtr */
	cpb.hFileInfo.ioDirID = 0;						/* same offset as ioFlNum */
	myresult = PBGetCatInfo(&cpb, (Boolean)0);/* Synchronous */
	if (myresult == noErr) {
		if ((cpb.hFileInfo.ioFlAttrib & ioDirMask) != 0) { /* DIR */
			switch (mode) {
				case R_OK: return 0;
				case W_OK: return -1;
				case X_OK: return 0;
				case F_OK: return 0;
				}
			}
		else {
			switch (mode) {
				case R_OK: return 0;
				case W_OK: return (cpb.hFileInfo.ioFRefNum > 0) ? -1 : 0;
				case X_OK: return (cpb.hFileInfo.ioFlFndrInfo.fdType == 'APPL') ? 0 : -1;
				case F_OK: return 0;
				}
			}
		}
	
	return -1;
	}

int isatty(int fd)
{
	return !ioctl(fd, FIOINTERACTIVE, NULL);
}

#ifdef test

#include <stdio.h>

#define DUMP(EXPR, MODE)	printf("%s = %"#MODE"\n", #EXPR, EXPR);

int main(int argc, char ** argv)
{
	int 			res;
	struct stat	statbuf;

	printf("StdIn is %s TTY.\n", isatty(0)?"a":"no");
	printf("StdOut is %s TTY.\n", isatty(1)?"a":"no");
	printf("StdErr is %s TTY.\n\n", isatty(2)?"a":"no");
	
	if (argc == 1)
		res = fstat(0, &statbuf);
	else
		res = stat(argv[1], &statbuf);

	if (res)
		printf("Error occurred.\n");
	else {
		DUMP(statbuf.st_dev,d);
		DUMP(statbuf.st_ino,d);
		DUMP(statbuf.st_mode,#o);
		DUMP(statbuf.st_nlink,d);
		DUMP(statbuf.st_uid,d);
		DUMP(statbuf.st_gid,d);
		DUMP(statbuf.st_rdev,d);
		DUMP(statbuf.st_size,d);
		DUMP(statbuf.st_atime,u);
		DUMP(statbuf.st_mtime,u);
		DUMP(statbuf.st_ctime,u);
		DUMP(statbuf.st_blksize,d);
		DUMP(statbuf.st_blocks,d);
	}
}
#endif
