/* @(#) ddfinternal.c 96/04/29 1.8 */

/* #define DEBUG */

/* Routines to manipulate DDFs (Device Description Files). */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/mem.h>
#include <kernel/semaphore.h>
#include <kernel/ddfnode.h>
#include <kernel/ddftoken.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/internalf.h>
#include <kernel/ddfuncs.h>
#include <string.h>
#include <stdio.h>

#ifndef BUILD_STRINGS
#undef DEBUG
#endif

#ifdef DEBUG
#define	SYNTAX_ERROR(line)    printf("Syntax error %d\n", line)
#define	DBUG(x) printf x
#else
#define	SYNTAX_ERROR(line)
#define	DBUG(x)
#endif

#if 0 /* We never delete! */
/*****************************************************************************
 DeleteDDF
 Delete a DDF structure, and all links to or from it.
*/
	void
DeleteDDF(DDFNode *del_ddf)
{
	DDFNode *ddf;
	DDFLink *link;

	SuperLockSemaphore(KB_FIELD(kb_DDFSemaphore), SEM_WAIT);
	for (;;)
	{
		link = (DDFLink *) FIRSTNODE(&del_ddf->ddf_Children);
		if (!ISNODE(&del_ddf->ddf_Children, link))
			break;
		REMOVENODE((Node *)link);
		SuperFreeMem(link, sizeof(DDFLink));
	}
	for (;;)
	{
		link = (DDFLink *) FIRSTNODE(&del_ddf->ddf_Parents);
		if (!ISNODE(&del_ddf->ddf_Parents, link))
			break;
		REMOVENODE((Node *)link);
		SuperFreeMem(link, sizeof(DDFLink));
	}
	ScanList(&KB_FIELD(kb_DDFs), ddf, DDFNode)
	{
		ScanList(&ddf->ddf_Children, link, DDFLink)
		{
			if (link->ddfl_Link == del_ddf)
			{
				REMOVENODE((Node *)link);
				SuperFreeMem(link, sizeof(DDFLink));
			}
		}
		ScanList(&ddf->ddf_Parents, link, DDFLink)
		{
			if (link->ddfl_Link == del_ddf)
			{
				REMOVENODE((Node *)link);
				SuperFreeMem(link, sizeof(DDFLink));
			}
		}
	}
	REMOVENODE((Node *)del_ddf);
	SuperUnlockSemaphore(KB_FIELD(kb_DDFSemaphore));
	SuperFreeMem(del_ddf, sizeof(DDFNode));
}
#endif /* if 0 */

#ifdef DEBUG
#define ABORTIF(cond) if(cond) { err= __LINE__; goto abort; }
#define ABORT() { err= __LINE__; goto abort; }
#else
#define ABORTIF(cond) if(cond) goto abort
#define ABORT() goto abort
#endif

/*****************************************************************************
 DDFSatisfies
 Does one DDF satisfy the needs of another?
*/
static bool DDFSatisfies(DDFNode* lo, DDFNode* hi)
{
    DDFTokenSeq needs;
    DDFToken token;
#ifdef DEBUG
    int err;
#endif

    InitTokenSeq(&needs, hi->ddf_Needs);
    /* Loop thru all the OR clauses, looking for one that is satisfied. */
    for(;;)
    {
	ABORTIF(PeekDDFToken(&needs, &token) < 0);
	if(token.tok_Type == TOK_KEYWORD)
	{
	    ABORTIF(token.tok_Value.v_Keyword != K_OR &&
		token.tok_Value.v_Keyword != K_END_NEED_SECTION);
	    /* Reached end of OR clause; it is satisfied. */
	    return TRUE;
	}

	/* The name token type must be a string. */
	ABORTIF(token.tok_Type != TOK_STRING);

	if(!SatisfiesNeed(lo, &needs))
	{
	    /* A need is not satisfied.
		Skip to the end of this OR clause and try the next one. */
	    for(;;)
	    {
		ABORTIF(GetDDFToken(&needs, &token) < 0);
		if(token.tok_Type != TOK_KEYWORD) continue;
		if(token.tok_Value.v_Keyword == K_OR) break;
		ABORTIF(token.tok_Value.v_Keyword != K_END_NEED_SECTION);
		/* No more OR clauses; DDF is not satisfied. */
		return FALSE;
	    }
	}
    }

abort:
    SYNTAX_ERROR(err);
    return FALSE;
}

/*****************************************************************************
 MakeDDFLink
 Make a link from a parent DDF to a child DDF, and vice-versa.
*/
static Err MakeDDFLink(DDFNode* parent, DDFNode* child)
{
    DDFLink *clink;
    DDFLink *plink;

    clink= SuperAllocMem(sizeof(*clink), MEMTYPE_NORMAL);
    if(!clink) return NOMEM;
    plink= SuperAllocMem(sizeof(*plink), MEMTYPE_NORMAL);
    if(!plink)
    {
	SuperFreeMem(clink, sizeof(*clink));
	return NOMEM;
    }
    clink->ddfl_Link= child;
    plink->ddfl_Link= parent;
    AddTail(&parent->ddf_Children, (Node*)clink);
    AddTail(&child->ddf_Parents, (Node*)plink);
    return 0;
}

static void MakeUniqueLink(DDFNode* hi, DDFNode* lo)
{
    if(DDFSatisfies(lo, hi))
    {
	DDFLink* ddfl;

	/* Make sure it's not in its list already. */
	ScanList(&hi->ddf_Children, ddfl, DDFLink)
	{
	    if(ddfl->ddfl_Link == lo) return;
	}
	DBUG(("linking from `%s' to `%s'\n",
	    hi->ddf_n.n_Name, lo->ddf_n.n_Name));
	MakeDDFLink(hi, lo);
    }
}

static void UnlinkDDF(DDFNode* ddf)
{
    /* FIXME: what the hell do we do if the device is open? */

    DDFLink *pl;
    DDFLink *cl;

    DBUG(("UnlinkDDF(%x) %s\n", ddf, ddf->ddf_n.n_Name));
    /* Remove me from my parents' child lists. */
    ScanList(&ddf->ddf_Parents, pl, DDFLink)
    {
	ScanList(&pl->ddfl_Link->ddf_Children, cl, DDFLink)
	{
	    if(cl->ddfl_Link == ddf)
	    {
		RemNode((Node*)cl);
		SuperFreeMem(cl, sizeof(*cl));
		break;
	    }
	}
    }

    /* Remove me from my children's parent lists. */
    ScanList(&ddf->ddf_Children, cl, DDFLink)
    {
	ScanList(&cl->ddfl_Link->ddf_Parents, pl, DDFLink)
	{
	    if(pl->ddfl_Link == ddf)
	    {
		RemNode((Node*)pl);
		SuperFreeMem(pl, sizeof(*pl));
		break;
	    }
	}
    }
}

/*****************************************************************************
*/
void EnableDDF(DDFNode* ddf)
{
    DDFLink* par;

    DBUG(("enabling `%s'\n", ddf->ddf_n.n_Name));

    set_ddf_flags(ddf, DDFF_ENABLED);
    ScanList(&ddf->ddf_Parents, par, DDFLink)
    {
	EnableDDF(par->ddfl_Link);
    }
}

/*****************************************************************************
*/
static bool
DDFCanManageAnyHW(DDFNode *ddf)
{
	HWResource hwr;

	for (hwr.hwr_InsertID = 0;  NextHWResource(&hwr) >= 0; )
	{
		if (DDFCanManage(ddf, hwr.hwr_Name))
			return TRUE;
	}
	return FALSE;
}

/*****************************************************************************
*/
void RebuildDDFEnables(void)
{
	DDFNode *ddf;

	DBUG(("RebuildEnables\n"));
	AccessProtectedListWith(
	{
		ScanList(&KB_FIELD(kb_DDFs), ddf, DDFNode)
		{
			ddf->ddf_Flags &= ~DDFF_ENABLED;
		}
		ScanList(&KB_FIELD(kb_DDFs), ddf, DDFNode)
		{
		  DBUG(("checking `%s'\n", ddf->ddf_n.n_Name));
			if (ddf_is_llvl(ddf))
			{
				if (DDFCanManageAnyHW(ddf))
					EnableDDF(ddf);
			} else if (IsEmptyList(&ddf->ddf_Children))
			{
				/* High-level with no children: enable it. */
				EnableDDF(ddf);
			}
		}
	}, &KB_FIELD(kb_DDFs));
	DBUG(("RebuildEnables done\n"));
}

/*****************************************************************************
 CreateDDF
 Create a DDF structure.
*/
static DDFNode* CreateDDF(char* name, char *module, char *driver,
			  uint8 version, uint8 revision, void* needs, void* provides)
{
	DDFNode* ddf;
	DDFNode* ddf2;
	DDFNode* oldddf;
	DDFTokenSeq flagseq;
	DDFToken flagtok;
	Driver *drv;
	Device *dev;

	if (name == NULL || needs == NULL || provides == NULL)
	{
		DBUG(("Missing DDF parameters\n"));
		return NULL;
	}
	DBUG(("Create DDF %s, mod %s, drv %s, ver %x\n",
		name, module ? module : "-",
		driver ? driver : "-", version));
	if (LockSemaphore(KB_FIELD(kb_DDFs).l_Semaphore,
			SEM_WAIT | SEM_SHAREDREAD) < 0)
	{
		DBUG(("Cannot lock semaphore\n"));
		return NULL;
	}

	oldddf = NULL;
	ScanList(&KB_FIELD(kb_DDFs), ddf, DDFNode)
	{
		if (strcasecmp(ddf->ddf_n.n_Name, name) == 0)
		{
			oldddf = ddf;
			break;
		}
	}

	if (oldddf != NULL)
	{
		/* Already have one: take the newer one. */
		if ((ddf->ddf_Version > version)
		 || (ddf->ddf_Version == version) && (ddf->ddf_Revision >= revision))
		{
			/* The one we already have is newer. */
			DBUG(("Already have newer DDF named %s\n", name));

			UnlockSemaphore(KB_FIELD(kb_DDFs).l_Semaphore);
			return oldddf;
		}
	}

	ddf = SuperAllocMem(sizeof(*ddf), MEMTYPE_NORMAL | MEMTYPE_FILL);
	if (ddf == NULL)
	{
		UnlockSemaphore(KB_FIELD(kb_DDFs).l_Semaphore);
		return NULL;
	}
	ddf->ddf_n.n_SubsysType = NST_KERNEL;
	ddf->ddf_n.n_Type = DDFNODE;
	ddf->ddf_n.n_Flags = NODE_NAMEVALID;
	ddf->ddf_n.n_Name = name;
	ddf->ddf_n.n_Size = sizeof(DDFNode);
	ddf->ddf_Version= version;
	ddf->ddf_Revision= revision;
	PrepList(&ddf->ddf_Children);
	PrepList(&ddf->ddf_Parents);
	ddf->ddf_Needs= needs;
	ddf->ddf_Provides= provides;
	ddf->ddf_Module = module != NULL ? module : name;
	ddf->ddf_Driver = driver != NULL ? driver : name;

	if(ScanForDDFToken(provides, "flags", &flagseq) >= 0 &&
	   GetDDFToken(&flagseq, &flagtok) >= 0 &&
	   flagtok.tok_Type == TOK_INT)
		ddf->ddf_Flags |= flagtok.tok_Value.v_Int;

	if(ScanForDDFToken(needs, "HW", NULL) >= 0)
		ddf->ddf_Flags |= DDFF_LOWLEVEL;

	if (oldddf != NULL)
	{
		/* Change all references to the old DDF, and remove it. */
		UnlinkDDF(oldddf);
		ScanList(&KB_FIELD(kb_Drivers), drv, Driver)
		{
			if (drv->drv_DDF == oldddf)
				drv->drv_DDF = ddf;
			ScanList(&drv->drv_Devices, dev, Device)
			{
				if (dev->dev_DDFNode == oldddf)
					dev->dev_DDFNode = ddf;
			}
		}
		RemNode((Node*)oldddf);
		SuperFreeMem(oldddf, sizeof(*oldddf));
	}

	ScanList(&KB_FIELD(kb_DDFs), ddf2, DDFNode)
	{
		MakeUniqueLink(ddf, ddf2);
		MakeUniqueLink(ddf2, ddf);
	}
	AddTail((List*)&KB_FIELD(kb_DDFs), (Node*)ddf);

	DBUG(("created DDF `%s' flags %x\n", name, ddf->ddf_Flags));
	RebuildDDFEnables();
	UnlockSemaphore(KB_FIELD(kb_DDFs).l_Semaphore);
	return ddf;
}


/*****************************************************************************/


typedef struct
{
    PackedID ck_ID;
    uint32   ck_Size;
} IFFChunk;


#define ID_NAME MAKE_ID('N','A','M','E')
#define ID_NEED MAKE_ID('N','E','E','D')
#define ID_PROV MAKE_ID('P','R','O','V')
#define ID_VER  MAKE_ID('!','V','E','R')


void ProcessDDFBuffer(uint8 *buf, int32 numBytes,
                      uint8 version, uint8 revision)
{
void     *data;
char     *name;
char     *module;
char     *driver;
void     *needs;
void     *provides;
uint32    skip;
IFFChunk *chunk;
char     *ch;
bool      dot;
uint32    len;

    name     = NULL;
    module   = NULL;
    driver   = NULL;
    needs    = NULL;
    provides = NULL;
    chunk    = (IFFChunk *)buf;

    while (numBytes > sizeof(IFFChunk))
    {
        if (chunk->ck_Size > numBytes - sizeof(IFFChunk))
            break;

        data = (void *)((uint32)chunk + sizeof(IFFChunk));
	switch (chunk->ck_ID)
	{
	    case ID_NAME: if (name)
	                  {
	                      /* we hit a NAME chunk which means this is a
	                       * new description. Add the current one...
	                       */
                              CreateDDF(name, module, driver, version, revision, needs, provides);
                              needs    = NULL;
                              provides = NULL;
                          }

                          name   = (char *)data;
	                  module = name + strlen(name) + 1;
                          driver = module + strlen(module) + 1;

                          if (*module == '\0')
                              module = NULL;

                          if (*driver == '\0')
                              driver = NULL;

                          break;

            case ID_NEED: needs = data;
                          break;

            case ID_PROV: provides = data;
                          break;

            case ID_VER : version  = 0;
                          revision = 0;
                          len      = chunk->ck_Size;
                          dot      = FALSE;
                          ch       = (char *)data;
                          while (len--)
                          {
                              if (*ch == '.')
                              {
                                  if (dot)
                                  {
                                      /* we only do version.revision */
                                      break;
                                  }
                                  dot = TRUE;
                              }
                              else if ((*ch < '0') && (*ch > '9'))
                              {
                                  /* illegal char */
                                  break;
                              }
                              else
                              {
                                  if (dot)
                                      revision = (revision * 10) + (*ch - '0');
                                  else
                                      version = (version * 10) + (*ch - '0');
                              }
                              ch++;
                          }
                          break;

            default     : break;
	}

        skip      = chunk->ck_Size + (chunk->ck_Size & 1);
	numBytes -= skip + sizeof(IFFChunk);
	chunk     = (IFFChunk *)((uint32)chunk + sizeof(IFFChunk) + skip);
    }

    if (numBytes)
    {
#ifdef BUILD_STRINGS
        printf("Corrupt DDF buffer!\n");
#endif
    }
    else if (name)
    {
        CreateDDF(name, module, driver, version, revision, needs, provides);
    }
}
