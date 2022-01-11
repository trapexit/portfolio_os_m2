/*
 * @(#) bootallocs.c 96/07/22 1.1
 *
 * bootallocs
 * Displays the list of BootAlloc structures,
 * representing boot-time memory allocations.
 */

#include <kernel/types.h>
#include <kernel/list.h>
#include <kernel/sysinfo.h>
#include <kernel/super.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct FlagInfo
{
	uint32	fi_Flags;
	char *	fi_Desc;
} FlagInfo;

static const FlagInfo flagInfo[] =
{
	{ BA_DIPIR_PRIVATE,	"Dipir-private" },
	{ BA_PERSISTENT,	"Persistent" },
	{ BA_GRAPHICS,		"Graphics" },
	{ BA_OSDATA,		"OS-data" },
	{ BA_PREMULTITASK,	"Pre-multitasking" },
	{ BA_DIPIR_SHARED,	"Dipir-shared" },
	{ 0, NULL }
};

int 
main(int argc, char **argv)
{
	List *list;
	BootAlloc *ba;
	const FlagInfo *fi;

	TOUCH(argc);
	TOUCH(argv);

	printf("Address  Size     Flags\n");
	/* Walk the list and print each one. */
	QUERY_SYS_INFO(SYSINFO_TAG_BOOTALLOCLIST, list);
	ScanList(list, ba, BootAlloc)
	{
		printf("%8x %8x %8x ", 
			ba->ba_Start, ba->ba_Size, ba->ba_Flags);
		for (fi = flagInfo;  fi->fi_Flags != 0;  fi++)
			if (fi->fi_Flags & ba->ba_Flags)
				printf(" %s", fi->fi_Desc);
		printf("\n");
	}
	return 0;
}
