#include <bootcode/bootglobals.h>
#include <bootcode/boothw.h>
#include "bootcode.h"
#include <kernel/list.h>
#include <kernel/listmacros.h>

extern bootGlobals BootGlobals;

typedef struct ReqAlloc
{
	void *		ra_Start;
	uint32		ra_Size;
	uint32		ra_Flags;
} ReqAlloc;

static const ReqAlloc reqAllocs[] = 
{
	{ (void *) VECTORSTART,		VECTORSIZE,	0 },
	{ (void *) BOOTDATASTART,	BOOTDATASIZE,	0 },
	{ (void *) DIPIRDATASTART,	DIPIRDATASIZE,	0 },
	{ (void *) DIPIRBUFSTART,	DIPIRBUFSIZE,	BA_DIPIR_SHARED },
#ifdef BUILD_DEBUGGER
#ifdef BUILD_FLASH
	{ (void *) DEBUGGERSTART,	DEBUGGERSIZE,	0 },
#else
	{ (void *) SYSROMIMAGE,		1*1024*1024,	0 },
#endif
#endif /* BUILD_DEBUGGER */
	{ 0, 0, 0 }
};
#define	NUM_REQ_ALLOCS	((sizeof(reqAllocs)/sizeof(ReqAlloc))-1)


static void 
PrepList(List *l)
{
    l->l_Flags  = 0;
    l->l_Head   = (Link *)&l->l_Filler;
    l->l_Filler = NULL;
    l->l_Tail   = (Link *)&l->l_Head;
}

static BootAlloc *
RemHeadBootAlloc(List *l)
{
	BootAlloc *base;
	BootAlloc *head;

	base = (BootAlloc *)&l->ListAnchor.head.links;
	head = base->ba_Next;
	if (head->ba_Next)
	{
		base->ba_Next = head->ba_Next;
		head->ba_Next->ba_Prev = base;
		return head;
	}
	return NULL;
}

#if 1
void
DumpBootAllocs(void)
{
	BootAlloc *ba;

	PrintString("BootAlloc list:\n");
	ScanList(&BootGlobals.bg_BootAllocs, ba, BootAlloc)
	{
		PRINTVALUE("===      ", (uint32)ba);
		PRINTVALUE("    addr ", (uint32)ba->ba_Start);
		PRINTVALUE("    size ", ba->ba_Size);
		PRINTVALUE("    flag ", ba->ba_Flags);
	}
	PrintString("--- end\n");
}
#endif

static void
Panic(char *msg, void *v1, void *v2)
{
	PrintString("BootAlloc: ");
	PrintString(msg);
	PrintString("\n");
	PRINTVALUE("#", (uint32)v1);
	PRINTVALUE("#", (uint32)v2);
	DumpBootAllocs();
	for (;;) ;
}

void
SanityCheckBootAllocs(void)
{
	BootAlloc *ba;
	void *startp;
	void *endp;
	void *nextstart;
	void *startRAM;
	void *endRAM;
	uint32 i;
	bool foundReqAlloc[NUM_REQ_ALLOCS];

	startRAM = (void*)BootGlobals.bg_SystemRAM.mr_Start;
	endRAM = (uint8*)startRAM + BootGlobals.bg_SystemRAM.mr_Size;

	for (i = 0;  i < NUM_REQ_ALLOCS;  i++)
		foundReqAlloc[i] = FALSE;

	ScanList(&BootGlobals.bg_BootAllocs, ba, BootAlloc)
	{
		startp = ba->ba_Start;
		endp = (uint8*)startp + ba->ba_Size;
		nextstart = ISNODE(&BootGlobals.bg_BootAllocs, ba->ba_Next) ? 
			ba->ba_Next->ba_Start : NULL;
		if (startp > endp)
			Panic("negative size!", startp, endp);
		if (startp < startRAM || endp > endRAM)
			Panic("beyond RAM!", startp, endp);
		if (nextstart != NULL && endp > nextstart)
			Panic("overlap!", endp, nextstart);
		for (i = 0;  i < NUM_REQ_ALLOCS;  i++)
			if (reqAllocs[i].ra_Start == ba->ba_Start &&
			    reqAllocs[i].ra_Size == ba->ba_Size &&
			    reqAllocs[i].ra_Flags == ba->ba_Flags)
				foundReqAlloc[i] = TRUE;
	}

	for (i = 0;  i < NUM_REQ_ALLOCS;  i++)
		if (!foundReqAlloc[i])
			Panic("did not find req alloc", 
				reqAllocs[i].ra_Start, 
				(void*)reqAllocs[i].ra_Size);
}

void
AddBootAllocNoChecks(void *startp, uint32 size, uint32 flags)
{
	BootAlloc *ba;
	BootAlloc *new;

	new = RemHeadBootAlloc(&BootGlobals.bg_BootAllocPool);
	if (new == NULL)
		Panic("cannot alloc BootAlloc", 0,0);
	new->ba_Start = startp;
	new->ba_Size = size;
	new->ba_Flags = flags;

	ScanList(&BootGlobals.bg_BootAllocs, ba, BootAlloc)
	{
		if (ba->ba_Start > startp)
		{
			new->ba_Next = ba;
			new->ba_Prev = ba->ba_Prev;
			ba->ba_Prev->ba_Next = new;
			ba->ba_Prev = new;
			return;
		}
	}
	ADDTAIL(&BootGlobals.bg_BootAllocs, new);
}

void
AddBootAlloc(void *startp, uint32 size, uint32 flags)
{
	SanityCheckBootAllocs();
	AddBootAllocNoChecks(startp, size, flags);
	SanityCheckBootAllocs();
}

void
DeleteBootAlloc(void *startp, uint32 size, uint32 flags)
{
	BootAlloc *ba;

	SanityCheckBootAllocs();

	ScanList(&BootGlobals.bg_BootAllocs, ba, BootAlloc)
	{
		if (ba->ba_Start == startp && 
		    ba->ba_Size == size && 
		    ba->ba_Flags == flags)
		{
			REMOVENODE((Node*)ba);
			ADDTAIL(&BootGlobals.bg_BootAllocPool, ba);
			SanityCheckBootAllocs();
			return;
		}
	}
	Panic("cannot find to delete", startp, (void*)size);
}

void
DeleteAllBootAllocs(uint32 mask, uint32 flags)
{
	BootAlloc *ba;

Restart:
	ScanList(&BootGlobals.bg_BootAllocs, ba, BootAlloc)
	{
		if ((ba->ba_Flags & mask) == flags)
		{
			DeleteBootAlloc(ba->ba_Start, ba->ba_Size, ba->ba_Flags);
			goto Restart;
		}
	}
}

uint32
VerifyBootAlloc(void *startp, uint32 size, uint32 flags)
{
	BootAlloc *ba;

	SanityCheckBootAllocs();

	ScanList(&BootGlobals.bg_BootAllocs, ba, BootAlloc)
	{
		if (ba->ba_Start == startp && 
		    ba->ba_Size == size &&
		    ba->ba_Flags == flags)
			return 1;
	}
	return 0;
}

void *
BootAllocate(uint32 size, uint32 flags)
{
	BootAlloc *ba;
	void *startp;
	void *endp;
	void *endRAM;

	endRAM = (uint8*)BootGlobals.bg_SystemRAM.mr_Start + 
			BootGlobals.bg_SystemRAM.mr_Size;

	ScanListB(&BootGlobals.bg_BootAllocs, ba, BootAlloc)
	{
		startp = (uint8*)ba->ba_Start + ba->ba_Size;
		endp = ISNODE(&BootGlobals.bg_BootAllocs, ba->ba_Next) ? 
			ba->ba_Next->ba_Start : endRAM;
		if ((uint8*)endp - (uint8*)startp >= size)
		{
			startp = (uint8*)endp - size;
			AddBootAlloc(startp, size, flags);
			return startp;
		}
	}
	return NULL;
}

void
InitBootAllocs(void)
{
	uint32 i;

	PrepList(&BootGlobals.bg_BootAllocs);
	PrepList(&BootGlobals.bg_BootAllocPool);

	for (i = 0;  i < NUM_BOOT_ALLOCS;  i++)
	{
		ADDTAIL(&BootGlobals.bg_BootAllocPool, 
			&BootGlobals.bg_BootAllocArray[i]);
	}

	for (i = 0;  i < NUM_REQ_ALLOCS;  i++)
	{
		AddBootAllocNoChecks(reqAllocs[i].ra_Start,
				reqAllocs[i].ra_Size, reqAllocs[i].ra_Flags);
	}

	SanityCheckBootAllocs();
}
