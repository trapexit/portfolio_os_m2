/*********************************************************************
File		:	stat.h			-	Macintosh implementation of stat()
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

/* We try to be as conformant as possible with the UNIX specification
	of stat(2) according to the SunOS 3.0 manpage. Pathnames are, however,
	Macintosh-style (with ':' as a directory separator). This code
	is not A/UX compatible and doesn't know about System 7 aliases
	(because it's author doesn't know, either :-) and AppleTalk access
	rights (too complicated right now).
*/

#include <time.h>

typedef unsigned short 	u_short;
typedef long				off_t;
typedef short				dev_t;
typedef long				ino_t;

/* mode information */

#define S_IFMT		0170000
#define  S_IFDIR	0040000
#define	S_IFCHR	0020000
#define	S_IFBLK	0060000
#define	S_IFREG	0100000
#ifdef NEVER_DEFINED
#define	S_IFLNK	0120000
#define	S_IFSOCK	0140000
#endif

#define S_ISUID	0004000
#define S_ISGID	0002000
#define S_ISVTX	0001000
#define S_IREAD	0000400
#define S_IWRITE	0000200
#define S_IEXEC	0000100

#ifdef NEVER_DEFINED

enum {
	R_OK = 0x01,
	W_OK = 0x02,
	X_OK = 0x04,
	F_OK = 0x08,
	};

#endif

/* Group and others permission is same as owner */

struct stat	{
	dev_t		st_dev;		/* Set to vol. refNum. 	*/
	ino_t		st_ino;		/* Set to file ID      	*/
	u_short	st_mode;
	short		st_nlink;	/* Always 1					*/
	short		st_uid;		/* Set to 0					*/
	short		st_gid;		/* Set to 0					*/
	dev_t		st_rdev;		/* Set to 0					*/
	off_t		st_size;
	time_t	st_atime;	/* Set to st_mtime 		*/
	time_t	st_mtime;
	time_t	st_ctime;
	long		st_blksize;
	long		st_blocks;
};

#ifdef __cplusplus
extern "C" {
#endif

int	stat(char * path, struct stat * buf);
int	lstat(char * path, struct stat * buf);
int	fstat(int fd, struct stat * buf);
int	isatty(int);

#ifdef __cplusplus
}
#endif
