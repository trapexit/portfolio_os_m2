/* @(#) copy.c 96/10/29 1.16 */

/**
|||	AUTODOC -class Shell_Commands -name Copy
|||	Copies files or directories
|||
|||	  Format
|||
|||	    Copy <file1> <file2>
|||	    Copy <directory1> <directory2>
|||	    Copy {file | directory} <directory>
|||
|||	  Description
|||
|||	    In the first form, copy copies the content of filename1 to
|||	    filename2. If filename2 exits, appropriate error is returned.
|||	    In the second form copy recursively copies all content of
|||	    directory1 to directory2. The destination directory must
|||	    already exist.
|||
|||	    In the third form the content of each filename or directory in
|||	    the argument list is copied to the destination directory. The
|||	    destination directory must already exist.
|||
|||	    WARNING: Beware of a recursive copy of a parent directory
|||	    into its child. For example:
|||
|||	        Copy /remote/mydir /remote/mydir/mysubdir
|||
|||	    This would fill up the file system.
|||
|||	  Implementation
|||
|||	    Command implemented in V20.
|||
|||	  Location
|||
|||	    System.m2/Programs/Copy
|||
|||	  See Also
|||
|||	    Delete, MkDir, RmDir
|||
**/

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/fileio.h>
#include <file/directory.h>
#include <file/directoryfunctions.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


void	usage(void);
void	copy_afile(char *srcPath, char *dstPath);
void	copy_dir(char *src, char *dst);
char	*basenm(char *dir);

char	*pname;


int
main(int ac, char *av[])
{
	RawFile		*srcFile, *dstFile;
	Err		srcErr, dstErr;
	char		*dst, *src;
	char		dstPath[FILESYSTEM_MAX_PATH_LEN];

	pname = *av;
	ac--, av++;
	if (ac < 2) {
		printf("%s: Wrong number of arguments\n", pname);
		usage();
	}

	dst = av[ac - 1];

	if (ac == 2) {
		src = *av;
		srcErr = OpenRawFile(&srcFile, src, FILEOPEN_READ);
		if (srcErr >= 0)
			CloseRawFile(srcFile);
		dstErr = OpenRawFile(&dstFile, dst, FILEOPEN_READ);
		if (dstErr >= 0)
			CloseRawFile(dstFile);

		if (srcErr >= 0 && dstErr >= 0) {
			printf("%s: %s exists\n", pname, dst);
			usage();
		}
		if (srcErr < 0 && srcErr != FILE_ERR_NOTAFILE) {
			PrintError(NULL, "get status of", src, srcErr);
			return 1;
		}
		if (srcErr == FILE_ERR_NOTAFILE && dstErr != FILE_ERR_NOTAFILE){
			printf("%s: %s is not a directory\n", pname, dst);
			usage();
		}
		/* easy case */
		if (srcErr >= 0 && dstErr == FILE_ERR_NOFILE) {
			copy_afile(src, dst);
			return 0;
		}
	}

	for (;  ac > 1;  ac--, av++) {
		src = *av;
		srcErr = OpenRawFile(&srcFile, src, FILEOPEN_READ);
		if (srcErr >= 0) {
			CloseRawFile(srcFile);
			sprintf(dstPath, "%s/%s", dst, basenm(src));
			copy_afile(src, dstPath);
		} else if (srcErr == FILE_ERR_NOTAFILE) {
			/* hierarchy */
			copy_dir(src, dst);
		} else {
			PrintError(NULL, "get status of", src, srcErr);
		}
	}
	return 0;
}


void
usage(void)
{
	printf("USAGE: %s file1 file2\n", pname);
	printf("       %s directory1 directory2\n", pname);
	printf("       %s file|directory ... directory\n", pname);
	exit(1);
}



void
copy_afile(char *srcPath, char *dstPath)
{
	Err		err;
	RawFile		*srcFile, *dstFile;
	FileInfo	srcInfo, dstInfo;
	uint32		nread;
	uint32		byts;
	char		buffer[1024];
	TagArg		attrTags[5];

	srcFile = dstFile = NULL;

	err = OpenRawFile(&srcFile, srcPath, FILEOPEN_READ);
	if (err < 0) {
                PrintError(NULL, "open", srcPath, err);
		goto Exit;
	}
	err = GetRawFileInfo(srcFile, &srcInfo, sizeof(srcInfo));
	if (err < 0) {
                PrintError(NULL, "get info about", srcPath, err);
		goto Exit;
	}

	err = OpenRawFile(&dstFile, dstPath, FILEOPEN_WRITE);
	if (err < 0) {
                PrintError(NULL, "create", dstPath, err);
		goto Exit;
        }
	err = GetRawFileInfo(dstFile, &dstInfo, sizeof(dstInfo));
	if (err < 0) {
                PrintError(NULL, "get info about", dstPath, err);
		goto Exit;
	}

	err = SetRawFileSize(dstFile, srcInfo.fi_ByteCount);
	if (err < 0) {
                PrintError(NULL, "set file size for", dstPath, err);
		goto Exit;
	}

	printf("%s: copying %s to %s\n", pname, srcPath, dstPath);
	for (byts = 0;  byts < srcInfo.fi_ByteCount;  byts += nread) {
		err = ReadRawFile(srcFile, buffer, sizeof(buffer));
		if (err < 0) {
                	PrintError(0, "read", srcPath, err);
			goto Exit;
		}
		nread = err;
		err = WriteRawFile(dstFile, buffer, nread);
		if (err < 0) {
                	PrintError(0, "write", dstPath, err);
			goto Exit;
		}
		if (err != nread) {
			printf("read %d bytes from %s, but only wrote %d to %s\n",
				nread, srcPath, err, dstPath);
			goto Exit;
		}
	}

	attrTags[0].ta_Tag = FILEATTRS_TAG_FILETYPE;
	attrTags[0].ta_Arg = (TagData) srcInfo.fi_FileType;
	attrTags[1].ta_Tag = FILEATTRS_TAG_VERSION;
	attrTags[1].ta_Arg = (TagData) srcInfo.fi_Version;
        attrTags[2].ta_Tag = FILEATTRS_TAG_REVISION;
	attrTags[2].ta_Arg = (TagData) srcInfo.fi_Revision;
	attrTags[3].ta_Tag = TAG_END;
	err = SetRawFileAttrs(dstFile, attrTags);
	if (err < 0)
	{
		PrintError(NULL, "set attributes on", dstPath, err);
		goto Exit;
	}

	/* Kludge: some filesystems (e.g. host) do not support SETDATE.
	 * So ignore errors from this. */
	attrTags[0].ta_Tag = FILEATTRS_TAG_DATE;
	attrTags[0].ta_Arg = (TagData) &srcInfo.fi_Date;
	attrTags[1].ta_Tag = TAG_END;
	err = SetRawFileAttrs(dstFile, attrTags);
	if (err < 0)
		PrintError("Warning", "set date on", dstPath, err);
	err = 0;

Exit:
	if (dstFile != NULL)
		CloseRawFile(dstFile);
	if (srcFile != NULL)
		CloseRawFile(srcFile);
	if (err < 0)
		(void) DeleteFile(dstPath);
}



void
copy_dir(char *srcDir, char *dst)
{
	Directory      *dir;
	DirectoryEntry  de;
	char		srcPath[FILESYSTEM_MAX_PATH_LEN];
	char		dstPath[FILESYSTEM_MAX_PATH_LEN];
	RawFile		*srcFile;
	Err		err;

	if ((dir = OpenDirectoryPath(srcDir)) == NULL) {
		printf("%s: failed to OpenDirectoryPath for \"%s\"\n", srcDir);
		return;
	}
	/* Scan the source directory. */
	while (ReadDirectory(dir, &de) >= 0) {
		sprintf(srcPath, "%s/%s", srcDir, de.de_FileName);
		sprintf(dstPath, "%s/%s", dst, de.de_FileName);
		err = OpenRawFile(&srcFile, srcPath, FILEOPEN_READ);
		if (err >= 0) {
			/* It's a plain file.  Copy it. */
			CloseRawFile(srcFile);
			copy_afile(srcPath, dstPath);
		} else if (err == FILE_ERR_NOTAFILE) {
			/* It's a directory.  Create a destination directory
			 * and recursively copy into it. */
			if ((err = CreateDirectory(dstPath)) < 0) {
				PrintError(NULL, "create directory", 
						dstPath, err);
				continue;
			}
			copy_dir(srcPath, dstPath);
		} else {
			PrintError(NULL, "open", srcPath, err);
		}
	}
	CloseDirectory(dir);
}


char *
basenm(char *dir)
{
	char *cp = dir;

	while (*cp++)
		;
	cp--;
	while ((*cp != '/') && (cp != dir))
		cp--;

	if (cp == dir)
		return dir;
	else
		return (cp + 1);
}

