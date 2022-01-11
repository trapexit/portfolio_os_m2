
/******************************************************************************
**
**  @(#) ls.c 96/09/25 1.19
**
******************************************************************************/

/**
|||	AUTODOC -class Shell_Commands -name Ls
|||	Displays the contents of a directory.
|||
|||	  Synopsis
|||
|||	    ls {-l} {directory}
|||
|||	  Description
|||
|||	    Scan directories and display their contents.
|||
|||	  Arguments
|||
|||	    {directory}
|||	        Name of the directories to list. You can specify an arbitrary
|||	        number of directory names. If no name is specified, the
|||	        current directory is displayed.
|||
|||	  Location
|||
|||	    System.m2/Programs/ls
|||
**/

/**
|||	AUTODOC -class Examples -name ls
|||	Displays the contents of a directory.
|||
|||	  Synopsis
|||
|||	    ls {-l} {directory}
|||
|||	  Description
|||
|||	    Demonstrates how to scan a directory to determine its contents.
|||
|||	  Arguments
|||
|||	    {directory}
|||	        Name of the directories to list. You can specify an arbitrary
|||	        number of directory names. If no name is specified, the
|||	        directory in which the ls program is located is displayed.
|||
|||	  Associated Files
|||
|||	    ls.c
|||
|||	  Location
|||
|||	    Examples/FileSystem
|||
**/

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/directory.h>
#include <file/directoryfunctions.h>
#include <misc/date.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


static const char *const month[] = { "???", 
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

typedef struct FSFlags { uint32 fsf_Flag; char fsf_Char; } FSFlags;
static  FSFlags fsFlags[] =
{
	{ FILE_IS_DIRECTORY,		'd' },
	{ FILE_IS_READONLY,		'r' },
	{ FILE_SUPPORTS_DIRSCAN,	's' },
	{ FILE_SUPPORTS_ADDDIR,		'a' },
	{ FILE_SUPPORTS_ENTRY,		'e' },
	{ FILE_BLOCKS_CACHED,		'c' },
	{ FILE_USER_STORAGE_PLACE,	'u' },
	{ FILE_STATIC_MAPPABLE,		'm' },
	{ FILE_DYNAMIC_MAPPABLE,	'M' },
	{ FILE_INFO_NOT_CACHED,		'i' },
	{ FILE_HAS_VALID_VERSION,	'v' },
	{ FILE_IS_BLESSED,		'y' },
	{ 0, 0 }
};
#define	NUM_FSFLAGS	((sizeof(fsFlags)/sizeof(FSFlags)) - 1)

int verbose = 0;

static void usage(void)
{
	printf("ls [-l] [directory]...\n");
	printf("   Lists files in directories (current directory if none specified)\n");
	printf("   Normal output is:\n");
	printf("       type  size  name\n");
	printf("   Long (-l) output is:\n");
	printf("       type  size  numblocks*blocksize  avatars  flags  version  date  name\n");
	printf("   Flags are:\n");
	printf("        d  Directory\n");
	printf("        r  Read-only\n");
	printf("        s  Scannable\n");
	printf("        a  Supports add-directory\n");
	printf("        e  Supports add-entry\n");
	printf("        c  Blocks are cached\n");
	printf("        u  User storage\n");
	printf("        m  Static mappable\n");
	printf("        M  Dynamic mappable\n");
	printf("        i  File info not cached\n");
	printf("        v  Has version\n");
	printf("        y  Contains system files\n");
	exit(1);
}

static char *PrintType(uint32 type)
{
char            ch;
int32           i;
uint32          t;
char           *p;
static char     buf[9];

    if (type == 0)
	return "        ";
    strcpy(buf, "    ");
    p = buf + strlen(buf);
    t = type;
    for (i = 0;  i < sizeof(type);  i++) {
	ch = (t >> 24) & 0xFF;
	t <<= 8;
	if (!isprint(ch)) break;
	*p++ = ch;
    }
    *p = '\0';
    if (i < sizeof(type))
        sprintf(buf, "%8x", type);
    return buf;
}

static char *PrintFlags(uint32 flags)
{
FSFlags		*f;
char		*p;
static char	buf[NUM_FSFLAGS+1];

	p = buf;
	for (f = fsFlags;  f->fsf_Flag != 0;  f++)
		if (f->fsf_Flag & flags)
			*p++ = f->fsf_Char;
	while (p < &buf[NUM_FSFLAGS])
		*p++ = ' ';
	*p = '\0';
	return buf;
}


/*****************************************************************************/


static void ListDirectory(const char *path)
{
Directory      *dir;
DirectoryEntry  de;
Item            ioReqItem;
int32           err;
char            fullPath[FILESYSTEM_MAX_PATH_LEN];
GregorianDate	gd;
IOInfo          ioInfo;
Item            dirItem;

    /* open the directory for access */
    dirItem = OpenFile((char *)path);
    if (dirItem < 0) {
	PrintError(NULL, "open file", path, dirItem);
	return;
    }

    /* create an IOReq for the directory */
    ioReqItem = CreateIOReq(NULL, 0, dirItem, 0);
    if (ioReqItem < 0) {
	PrintError(NULL, "create IOReq for", path, ioReqItem);
        CloseFile(dirItem);
	return;
    }

    /* Ask the directory its full name. This will expand any aliases
     * given on the command-line into fully qualified pathnames.  */
    memset(&ioInfo, 0, sizeof(ioInfo));
    ioInfo.ioi_Command         = FILECMD_GETPATH;
    ioInfo.ioi_Recv.iob_Buffer = fullPath;
    ioInfo.ioi_Recv.iob_Len    = sizeof(fullPath);
    err = DoIO(ioReqItem, &ioInfo);
    if (err < 0)
    {
	PrintError(NULL, "get full path name for", path, err);
        CloseFile(dirItem);
        return;
    }

    /* now open the directory for scanning */
    dir = OpenDirectoryPath(path);
    if (dir == NULL)
    {
        PrintError(NULL, "open directory", fullPath, 0);
	return;
    }

    printf("\nContents of directory %s:\n\n", fullPath);
    while (ReadDirectory(dir, &de) >= 0)
    {
	printf("%5s %7d ", 
		PrintType(de.de_Type), de.de_ByteCount);
	if (verbose) {
	    printf("%4d*%-4d %3d %s ",
		de.de_BlockCount, de.de_BlockSize,
		de.de_AvatarCount, PrintFlags(de.de_Flags));

	    if (de.de_Flags & FILE_HAS_VALID_VERSION)
	        printf("%3d.%-3d ", de.de_Version, de.de_Revision);
	    else
	        printf("        ");

	    if ((de.de_Date.tv_Seconds != 0 ||
	         de.de_Date.tv_Microseconds != 0) &&
	        (err = ConvertTimeValToGregorian(&de.de_Date, &gd)) >= 0)
	        printf("%2d %3s %4d %02d:%02d ",
			gd.gd_Day, month[gd.gd_Month], gd.gd_Year,
			gd.gd_Hour, gd.gd_Minute);
	   else
	         printf("                  ");
	}
	printf(" %s\n", de.de_FileName);
    }
    printf("\nEnd of directory\n\n");
    CloseDirectory(dir);
    DeleteIOReq(ioReqItem);
    CloseFile(dirItem);
}


/*****************************************************************************/


int main(int32 argc, char **argv)
{
int32 i;

    i = 1;
    if (argc == 2 && 
	(strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "--help") == 0)) {
	usage();
    }

    if (argc > 1 && strcmp(argv[1], "-l") == 0) {
	i++;
	verbose = 1;
    }
    if (i >= argc)
    {
	/* if no directory name was given, scan the current directory */
	ListDirectory(".");
    }
    else
    {
	/* go through all the arguments */
	for ( ; i < argc; i++)
	        ListDirectory(argv[i]);
    }

    return 0;
}
