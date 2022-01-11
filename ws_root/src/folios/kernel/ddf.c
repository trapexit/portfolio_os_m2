/* @(#) ddf.c 96/07/29 1.30 */

/*#define DEBUG*/

/* Routines to manipulate DDFs (Device Description Files). */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/semaphore.h>
#include <kernel/ddfnode.h>
#include <kernel/ddftoken.h>
#include <kernel/operror.h>
#include <kernel/sysinfo.h>
#include <kernel/super.h>
#include <kernel/internalf.h>
#include <string.h>
#include <stdio.h>

#ifndef BUILD_STRINGS
#undef DEBUG
#endif

#ifdef DEBUG
#define	SYNTAX_ERROR(n)    printf("Syntax error %d\n", n)
#define	DBUG(x) printf x
#else
#define	SYNTAX_ERROR(n)
#define	DBUG(x)
#endif

/**
|||	AUTODOC -private -class Kernel -group DDF -name NextDDFToken
|||	Gets the next token from a token sequence.
|||
|||	  Synopsis
|||
|||	    Err NextDDFToken(DDFTokenSeq *seq, DDFToken *tokptr, bool peek);
|||
|||	  Description
|||
|||	    This is a super vector that may be called in user mode.
|||	    The next token in the token sequence is put into the given
|||	    token buffer.  If the peek flag is FALSE, the token sequence
|||	    is updated to point to the following token.  Otherwise, it is
|||	    not modified.
|||
|||	  Arguments
|||
|||	    seq
|||	        Pointer to the DDFTokenSeq token sequence.
|||
|||	    tokptr
|||	        Pointer to a DDFToken buffer in which to place the
|||	        returned token.
|||
|||	    peek
|||	        Boolean flag to request that the sequence not be updated.
|||
|||	  Return Value
|||
|||	    Returns zero or a negative error code for failure.  Possible
|||	    error codes currently include:
|||
|||	    ER_SoftErr
|||	        An unknown token was encountered in the sequence.
|||
|||	  Notes
|||
|||	    Two convenience macros, GetDDFToken() and PeekDDFToken(), which
|||	    take the first two arguments, clear and set the peek flag,
|||	    respectively.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/super.h>
|||
|||	  See Also
|||
|||	    ScanForDDFToken()
|||
**/

Err NextDDFToken(DDFTokenSeq *seq, DDFToken *tokptr, bool peek)
{
	uint8 *p;
	uint8 tbyte;
	uint32 num;
	uint32 i;
#define token (*tokptr)

	p = seq->tokseq_Ptr;
	tbyte = *p++;
	switch (token.tok_Type = (tbyte >> 4))
	{
	case TOK_INT1:
	case TOK_INT2:
	case TOK_INT3:
	case TOK_INT4:
	case TOK_INT5:
		num = (tbyte & 0x7);
		for (i = 0;  i < token.tok_Type - TOK_INT1;  i++)
			num = (num << 8) | (*p++);
		if (tbyte & TOK_INT_NEG)
			num = -num;
		token.tok_Value.v_Int = num;
		token.tok_Type = TOK_INT;
		break;
	case TOK_STRING:
		token.tok_Value.v_String = p;
		while (*p++ != '\0')
			;
		break;
	case TOK_KEYWORD:
		token.tok_Value.v_Keyword = (tbyte & 0xF);
		break;
	case TOK_OP:
		token.tok_Value.v_Op = (tbyte & 0xF);
		break;
	default:
#ifdef BUILD_STRINGS
		printf("NextDDFToken: unknown token byte %x\n", tbyte);
#endif
		return MakeKErr(ER_SEVERE,ER_C_STND,ER_SoftErr);
	}
	if (!peek)
		seq->tokseq_Ptr = p;
	return 0;
#undef token
}

/**
|||	AUTODOC -private -class Kernel -group DDF -name ScanForDDFToken
|||	Scans for a particular token in a token sequence.
|||
|||	  Synopsis
|||
|||	    Err ScanForDDFToken(void* np, char const* name, DDFTokenSeq* rhstokens);
|||
|||	  Description
|||
|||	    This is a super vector that may be called in user mode.
|||	    The input token sequence is searched for a token whose name
|||	    matches the specified parameter.  If the token is found, the
|||	    "right-hand-side" token sub-sequence whose first token is the
|||	    specified token is returned in the DDFTokenSeq buffer.  If the
|||	    token is not found, the buffer is unchanged.
|||
|||	  Arguments
|||
|||	    np
|||	        Pointer to the token sequence to search.
|||
|||	    name
|||	        The name of the token for which to search.
|||
|||	    rhstokens
|||	        DDFTokenSeq token sequence buffer that will accept
|||	        the sub-sequence.
|||
|||	  Return Value
|||
|||	    Returns zero or a negative error code for failure.  Possible
|||	    error codes currently include:
|||
|||	    ER_NotFound
|||	        No token with the given name was encountered.
|||
|||	    ER_SoftErr
|||	        An unknown token was encountered in the sequence.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/super.h>
|||
|||	  See Also
|||
|||	    NextDDFToken()
|||
**/

Err ScanForDDFToken(void* np, char const* name, DDFTokenSeq* rhstokens)
{
    DDFTokenSeq seq;
    DDFToken token;
    Err err;

    InitTokenSeq(&seq, np);
    for(;;)
    {
	/* Look for the name. */
	err= GetDDFToken(&seq, &token);
	if(err < 0) return err;
	if(token.tok_Type == TOK_KEYWORD)
	{
	    if(token.tok_Value.v_Keyword == K_END_NEED_SECTION
	    	|| token.tok_Value.v_Keyword == K_END_PROVIDE_SECTION)
	    {
		/* Didn't find it. */
		return MakeKErr(ER_INFO,ER_C_STND,ER_NotFound);
	    }
	    else if(token.tok_Value.v_Keyword == K_OR) continue;
	}
	if(token.tok_Type != TOK_STRING)
	{
	    SYNTAX_ERROR(__LINE__);
	    return MakeKErr(ER_SEVERE,ER_C_STND,ER_SoftErr);
	}
	if(!strcmp(token.tok_Value.v_String, name))
	{
	    /* Found it. */
	    if(rhstokens) *rhstokens= seq;
	    return 0;
	}
	/* Skip to the end of this need/provide. */
	do
	{
	    err= GetDDFToken(&seq, &token);
	    if(err < 0) return err;
	}
	while(token.tok_Value.v_Keyword != K_END_NEED
	    && token.tok_Value.v_Keyword != K_END_PROVIDE);
    }
}

#ifdef DEBUG
#define ABORTIF(cond) if(cond) { err= __LINE__; goto abort; }
#define ABORT() { err= __LINE__; goto abort; }
#else
#define ABORTIF(cond) if(cond) goto abort
#define ABORT() goto abort
#endif

/*****************************************************************************
 SatisfiesNeedValue
 Does a DDF satisfy a single desired value of a need?
*/
static bool SatisfiesNeedValue(DDFNode* lo, char* name, uint8 op, DDFToken const* want)
{
    DDFTokenSeq prov;
    DDFToken token;
    int32 cmp;
#ifdef DEBUG
    int err;
#endif

    InitTokenSeq(&prov, lo->ddf_Provides);

    /* Look thru all the PROVIDES of the DDF and find
	the attribute with the desired name. */
    for (;;)
    {
	ABORTIF(GetDDFToken(&prov, &token) < 0);
	if (token.tok_Type == TOK_KEYWORD)
	{
	    ABORTIF(token.tok_Value.v_Keyword != K_END_PROVIDE_SECTION);
	    /* Name not found; need is not satisfied. */
	    if (op == (OP_NOT | OP_DEFINED))
	    {
		/* We *want* the name to be undefined. */
		return TRUE;
	    }
	    return FALSE;
	}
	ABORTIF(token.tok_Type != TOK_STRING);
	if (!strcmp(token.tok_Value.v_String, name)) break; /* Found it. */

	/* Skip to the end of this PROVIDE. */
	for(;;)
	{
	    ABORTIF(GetDDFToken(&prov, &token) < 0);
	    if (token.tok_Type == TOK_KEYWORD)
	    {
		ABORTIF(token.tok_Value.v_Keyword != K_END_PROVIDE);
		break;
	    }
	}
    }

    /* Found the right attribute.
	Now look thru all its VALUEs and see if one satisfies
	the desired (op,value) pair. */
    if ((op & ~OP_NOT) == OP_DEFINED)
    {
	/* Don't care about the value; just care if the attribute is defined. */
	if (op & OP_NOT)
	    return FALSE;
	return TRUE;
    }

    for(;;)
    {
	ABORTIF(GetDDFToken(&prov, &token) < 0);
	if (token.tok_Type == TOK_KEYWORD)
	{
	    ABORTIF(token.tok_Value.v_Keyword != K_END_PROVIDE);
	    /* No value satisfies the need. */
	    return FALSE;
	}
	if (token.tok_Type != want->tok_Type)
	{
	    /* Weird; it's not even the right type! */
	    continue;
	}
	/* Get the comparison result. */
	if (token.tok_Type == TOK_STRING)
	    cmp = strcmp(token.tok_Value.v_String, want->tok_Value.v_String);
	else
	    cmp = token.tok_Value.v_Int - want->tok_Value.v_Int;
	/* See if it satisfies the operator. */
	switch (op & ~OP_NOT)
	{
	case OP_EQ:
	    cmp= (cmp == 0);
	    break;
	case OP_GT:
	    cmp= (cmp > 0);
	    break;
	case OP_LT:
	    cmp= (cmp < 0);
	    break;
	}
	if (op & OP_NOT) cmp= !cmp;
	if (cmp) return TRUE;
    }

abort:
    SYNTAX_ERROR(err);
    return FALSE;
}

/**
|||	AUTODOC -private -class Kernel -group DDF -name SatisfiesNeed
|||	Checks if a DDF satisfies a single need.
|||
|||	  Synopsis
|||
|||	    bool SatisfiesNeed(DDFNode* ddf, DDFTokenSeq* needs);
|||
|||	  Description
|||
|||	    This is a super vector that may be called in user mode.
|||	    The token sequence is scanned to determine if the DDF can
|||	    satisfy all of its needs.
|||
|||	  Arguments
|||
|||	    ddf
|||	        Pointer to the DDFNode.
|||
|||	    needs
|||	        Pointer to the DDFTokenSeq token sequence.
|||
|||	  Return Value
|||
|||	    Returns TRUE if the DDF satisfies all of the needs in the
|||	    token sequence; FALSE otherwise.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/super.h>
|||
|||	  See Also
|||
|||	    ScanForDDFToken(), NextDDFToken()
|||
**/

bool SatisfiesNeed(DDFNode* ddf, DDFTokenSeq* needs)
{
    DDFToken nameToken;
    DDFToken opToken;
    DDFToken valueToken;
#ifdef DEBUG
    int err;
#endif

    /* On entry we must be pointing to the NAME of the need.
	Get the NAME and the OP of the attribute. */
    ABORTIF(GetDDFToken(needs, &nameToken) < 0 ||
		nameToken.tok_Type != TOK_STRING);
    ABORTIF(GetDDFToken(needs, &opToken) < 0 ||
		opToken.tok_Type != TOK_OP);

    if ((opToken.tok_Value.v_Op & ~OP_NOT) == OP_DEFINED)
    {
	/* Special case for OP_DEFINED: there are no VALUEs with it. */
	return SatisfiesNeedValue(ddf,
		nameToken.tok_Value.v_String,
		opToken.tok_Value.v_Op,
		NULL);
    }

    /* Now look at each of the VALUEs and make sure each one
	is satisfied by the DDF. */
    for(;;)
    {
	ABORTIF(GetDDFToken(needs, &valueToken) < 0);
	if(valueToken.tok_Type == TOK_KEYWORD)
	{
	    ABORTIF(valueToken.tok_Value.v_Keyword != K_END_NEED);
	    /* No more values; this need is satisifed. */
	    return TRUE;
	}

	/* Value must be a string or an integer. */
	ABORTIF(valueToken.tok_Type != TOK_STRING &&
	    valueToken.tok_Type != TOK_INT);

	if(!SatisfiesNeedValue(ddf,
	    nameToken.tok_Value.v_String,
	    opToken.tok_Value.v_Op,
	    &valueToken))
	{
	    /* Value is not satisfied. */
	    for(;;)
	    {
		ABORTIF(GetDDFToken(needs, &valueToken) < 0);
		if(valueToken.tok_Type != TOK_KEYWORD) continue;
		ABORTIF(valueToken.tok_Value.v_Keyword != K_END_NEED);
		return FALSE;
	    }
	}
    }

abort:
    SYNTAX_ERROR(err);
    return FALSE;
}

static Err AddIfSatisfied(List* l, DDFNode* ddf, DDFQuery query[])
{
    DDFQuery* qp;
    DeviceListNode* np;
    int32 value;

    for (qp = query; qp->ddfq_Name; qp++)
    {
	DDFTokenSeq seq;
	int i, length;
	char* need;
	char* cp;
	bool satisfied;

	/* Massage each query into a DDFTokenSeq of need tokens. */
	length= 16 + strlen(qp->ddfq_Name);
	if (qp->ddfq_Flags & DDF_STRING)
	{
	    length += strlen(qp->ddfq_Value.ddfq_String);
	}
	need= (char*)AllocMem(length, MEMTYPE_NORMAL);
	if(!need) return NOMEM;
	i= 0;
	need[i++]= TOK_STRING << 4;
	cp= qp->ddfq_Name;
	do need[i++]= *cp; while(*cp++);
	switch(qp->ddfq_Operator)
	{
	    case DDF_EQ: need[i++]= (TOK_OP << 4) | OP_EQ; break;
	    case DDF_GT: need[i++]= (TOK_OP << 4) | OP_GT; break;
	    case DDF_LT: need[i++]= (TOK_OP << 4) | OP_LT; break;
	    case DDF_GE: need[i++]= (TOK_OP << 4) | OP_LT | OP_NOT; break;
	    case DDF_LE: need[i++]= (TOK_OP << 4) | OP_GT | OP_NOT; break;
	    case DDF_NE: need[i++]= (TOK_OP << 4) | OP_EQ | OP_NOT; break;
	    case DDF_DEFINED: need[i++]= (TOK_OP << 4) | OP_DEFINED; break;
	    case DDF_NOTDEFINED: need[i++]= (TOK_OP << 4) | OP_DEFINED | OP_NOT; break;
	    default: 
#ifdef BUILD_STRINGS
		printf("op %x?\n", qp->ddfq_Operator);
#endif
		FreeMem(need, length); return BADTAGVAL;
	}
	if (qp->ddfq_Flags & DDF_INT)
	{
	    value = qp->ddfq_Value.ddfq_Int;
	    if (value < 0)
	    {
	        need[i++] = (TOK_INT5 << 4) | TOK_INT_NEG;
		value = -value;
	    } else
	    {
	        need[i++] = (TOK_INT5 << 4);
	    }
	    need[i++] = value >> 24;
	    need[i++] = value >> 16;
	    need[i++] = value >> 8;
	    need[i++] = value;
	}
	else if (qp->ddfq_Flags & DDF_STRING)
	{
	    need[i++]= TOK_STRING << 4;
	    cp= qp->ddfq_Value.ddfq_String;
	    do need[i++]= *cp; while(*cp++);
	}
	need[i++]= (TOK_KEYWORD << 4) | K_END_NEED;
	need[i]= (TOK_KEYWORD << 4) | K_END_NEED_SECTION;

	InitTokenSeq(&seq, need);
	satisfied= SatisfiesNeed(ddf, &seq);
	FreeMem(need, length);
	if (!satisfied)
	    return 0;
    }

    /*
     * The entire query array was successfully traversed.
     * Add a new node for this DDF to the end of the list.
     */
    np = AllocMem(sizeof(DeviceListNode), MEMTYPE_NORMAL);
    if (np == NULL) return NOMEM;
    memset(np, 0, sizeof(DeviceListNode));
    np->dln_DDFNode = ddf;
    np->dln_n.n_Name = ddf->ddf_n.n_Name;
    AddTail(l, (Node*)np);
    return 0;
}

/**
|||	AUTODOC -private -class Kernel -group Devices -name CreateDeviceList
|||	Finds the devices which support the requested capabilities.
|||
|||	  Synopsis
|||
|||	    Err CreateDeviceList(List **pList, const DDFQuery query[]);
|||
|||	  Description
|||
|||	    The list of all devices currently supported by the system
|||	    is searched for those devices which support all of the
|||	    capabilities in the provided array.  The returned list
|||	    of DeviceListNodes describes all devices which do support
|||	    the specified capabilities.  The n_Name field of each
|||	    DeviceListNode is set to the name of the device, and the
|||	    dln_DDFNode field is set to point to the DDFNode for the
|||	    device.
|||
|||	  Arguments
|||
|||	    pList
|||	        Pointer to a List * which upon successful completion
|||	        is set to point to a list of DeviceListNodes.
|||	        If no devices in the system satisfy the query,
|||	        the List will be empty.
|||	    query
|||	        Array of device description capability requirements.
|||
|||	  Return Value
|||
|||	    Returns zero or a negative error code for failure.  Possible
|||	    error codes currently include:
|||
|||	    TBD
|||
|||	  Notes
|||
|||	    Normally there is no need to call CreateDeviceList().
|||	    CreateDeviceList() is one of a set of low-level functions
|||	    which should rarely be called directly.  Other functions
|||	    in this set of low level functions are DeleteDeviceList(),
|||	    CreateDeviceStack(), DeleteDeviceStack(),
|||	    AppendDeviceStackDDF() and AppendDeviceStackHW().
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/device.h> <kernel/ddfnode.h>
|||
|||	  See Also
|||
|||	    CreateDeviceStackList(), DeleteDeviceList()
|||
**/

Err CreateDeviceList(List **pList, const DDFQuery query[])
{
    List *devList;
    DDFNode* ddf;
    Err err = 0;

    devList = AllocMem(sizeof(List), MEMTYPE_NORMAL);
    PrepList(devList);
    StartScanProtectedList(&KB_FIELD(kb_DDFs), ddf, DDFNode)
    {
        err = AddIfSatisfied(devList, ddf, query);
        if (err < 0)
	{
	    DeleteDeviceList(devList);
	    break;
	}
    }
    EndScanProtectedList(&KB_FIELD(kb_DDFs));
    *pList = devList;
    return err;
}

/**
|||	AUTODOC -private -class Kernel -group Devices -name DeleteDeviceList
|||	Destroys the List created by CreateDeviceList().
|||
|||	  Synopsis
|||
|||	    void DeleteDeviceList(List* l);
|||
|||	  Description
|||
|||	    Deletes a list previously returned from CreateDeviceList.
|||	    The list itself, and all DeviceListNodes in it, are deleted.
|||
|||	  Arguments
|||
|||	    l
|||	        Pointer to the List to be deleted.
|||
|||	  Notes
|||
|||	    See the Notes in CreateDeviceList().
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/device.h> <kernel/ddfnode.h>
|||
|||	  See Also
|||
|||	    CreateDeviceList()
|||
**/

void DeleteDeviceList(List* l)
{
    DeviceListNode *dln;

    while ((dln = (DeviceListNode *) RemHead(l)) != NULL)
    {
	FreeMem(dln, sizeof(DeviceListNode));
    }
    FreeMem(l, sizeof(List));
}

/**
|||	AUTODOC -private -class Kernel -group Devices -name CreateDeviceStack
|||	Creates an empty device stack.
|||
|||	  Synopsis
|||
|||	    DeviceStack * CreateDeviceStack(void);
|||
|||	  Description
|||
|||	    Creates an empty device stack.  Devices may be added
|||	    to the stack by AppendDeviceStackDDF() and
|||	    AppendDeviceStackHW().
|||
|||	  Return Value
|||
|||	    Returns NULL if there was an error in constructing the
|||	    DeviceStack.  Otherwise returns a pointer to a new empty
|||	    DeviceStack.
|||
|||	  Notes
|||
|||	    See the Notes in CreateDeviceList().
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/device.h> <kernel/ddfnode.h>
|||
|||	  See Also
|||
|||	    CreateDeviceStackList(), AppendDeviceStackDDF(),
|||	    AppendDeviceStackHW(), DeleteDeviceStack()
|||
**/

DeviceStack *
CreateDeviceStack(void)
{
	DeviceStack *ds;

	ds = AllocMem(sizeof(DeviceStack), MEMTYPE_NORMAL);
	if (ds == NULL)
		return NULL;
	memset(ds, 0, sizeof(DeviceStack));
	PrepList(&ds->ds_Stack);
	return ds;
}

/**
|||	AUTODOC -private -class Kernel -group Devices -name DeleteDeviceStack
|||	Destroys a DeviceStack returned from CreateDeviceStack().
|||
|||	  Synopsis
|||
|||	    void DeleteDeviceStack(DeviceStack *ds);
|||
|||	  Description
|||
|||	    Deletes a DeviceStack previously returned from CreateDeviceStack.
|||
|||	  Arguments
|||
|||	    ds
|||	        Pointer to the DeviceStack to be deleted.
|||
|||	  Notes
|||
|||	    See the Notes in CreateDeviceList().
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/device.h> <kernel/ddfnode.h>
|||
|||	  See Also
|||
|||	    CreateDeviceStack()
|||
**/

void
DeleteDeviceStack(DeviceStack *ds)
{
	DeviceStackNode *dsn;

	if (ds == NULL)
		return;
	while ((dsn = (DeviceStackNode *) RemHead(&ds->ds_Stack)) != NULL)
	{
		FreeMem(dsn, sizeof(DeviceStackNode));
	}
	FreeMem(ds, sizeof(DeviceStack));
}

/**
|||	AUTODOC -private -class Kernel -group Devices -name AppendDeviceStackDDF
|||	Appends a device to a device stack.
|||
|||	  Synopsis
|||
|||	    Err AppendDeviceStackDDF(DeviceStack *ds, const DDFNode *ddfn);
|||
|||	  Description
|||
|||	    Appends a DeviceStackNode to the bottom of a DeviceStack.
|||	    The new DeviceStackNode points to the specified DDFNode.
|||
|||	  Arguments
|||
|||	    ds
|||	        The DeviceStack to which the new node is appended.
|||	    ddfn
|||	        Pointer to the DDFNode which the new DeviceStackNode
|||	        should point to.
|||
|||	  Return Value
|||
|||	    Returns zero or a negative error code for failure.  Possible
|||	    error codes currently include:
|||
|||	    TBD
|||
|||	  Notes
|||
|||	    See the Notes in CreateDeviceList().
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/device.h> <kernel/ddfnode.h>
|||
|||	  See Also
|||
|||	    CreateDeviceStack(), DeleteDeviceStack()
|||
**/

Err
AppendDeviceStackDDF(DeviceStack *ds, const DDFNode *ddfn)
{
	DeviceStackNode *dsn;

	dsn = AllocMem(sizeof(DeviceStackNode), MEMTYPE_NORMAL);
	if (dsn == NULL)
		return NOMEM;
	memset(dsn, 0, sizeof(DeviceStackNode));
	dsn->dsn_IsHW = FALSE;
	dsn->dsn_DDFNode = ddfn;
	AddTail(&ds->ds_Stack, (Node*)dsn);
	return 0;
}

/**
|||	AUTODOC -private -class Kernel -group Devices -name AppendDeviceStackHW
|||	Appends a hardware resource to a device stack.
|||
|||	  Synopsis
|||
|||	    Err AppendDeviceStackHW(DeviceStack *ds, HardwareID id);
|||
|||	  Description
|||
|||	    Appends a DeviceStackNode to the bottom of a DeviceStack.
|||	    The new DeviceStackNode points to the HWResource identified
|||	    by the specified HardwareID.
|||
|||	  Arguments
|||
|||	    ds
|||	        The DeviceStack to which the new node is appended.
|||	    id
|||	        ID of the HWResource which the new DeviceStackNode
|||	        should point to.
|||
|||	  Return Value
|||
|||	    Returns zero or a negative error code for failure.  Possible
|||	    error codes currently include:
|||
|||	    TBD
|||
|||	  Notes
|||
|||	    See the Notes in CreateDeviceList().
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/device.h> <kernel/ddfnode.h>
|||
|||	  See Also
|||
|||	    CreateDeviceStack(), DeleteDeviceStack()
|||
**/

Err
AppendDeviceStackHW(DeviceStack *ds, HardwareID id)
{
	DeviceStackNode *dsn;

	dsn = AllocMem(sizeof(DeviceStackNode), MEMTYPE_NORMAL);
	if (dsn == NULL)
		return NOMEM;
	memset(dsn, 0, sizeof(DeviceStackNode));
	dsn->dsn_IsHW = TRUE;
	dsn->dsn_HWResource = id;
	AddTail(&ds->ds_Stack, (Node*)dsn);
	return 0;
}

DeviceStack *
CopyDeviceStack(DeviceStack *ds)
{
	DeviceStack *ds2;
	DeviceStackNode *dsn;
	DeviceStackNode *dsn2;


	ds2 = CreateDeviceStack();
	if (ds != NULL)
	{
		ScanList(&ds->ds_Stack, dsn, DeviceStackNode)
		{
			dsn2 = AllocMem(sizeof(DeviceStackNode), MEMTYPE_NORMAL);
			*dsn2 = *dsn;
			AddTail(&ds2->ds_Stack, (Node*)dsn2);
		}
	}
	return ds2;
}

static bool
DDFNodeInDeviceStack(DeviceStack *ds, DDFNode *ddfn)
{
	DeviceStackNode *dsn;

	ScanList(&ds->ds_Stack, dsn, DeviceStackNode)
	{
		if (!dsn->dsn_IsHW && dsn->dsn_DDFNode == ddfn)
			return TRUE;
	}
	return FALSE;
}

bool
DDFCanManage(const DDFNode* ddf, const char *hwName)
{
    DDFTokenSeq needs;
    DDFToken token;

    if (ScanForDDFNeed(ddf, "HW", &needs) < 0) return FALSE;

    /* "HW" must equal something. */
    if (GetDDFToken(&needs, &token) < 0) return FALSE;
    if (token.tok_Type != TOK_OP) return FALSE;
    if (token.tok_Value.v_Op != OP_EQ) return FALSE;

    /* "HW" must equal a string. */
    if (GetDDFToken(&needs, &token) < 0) return FALSE;
    if (token.tok_Type != TOK_STRING) return FALSE;
    if (strcmp(token.tok_Value.v_String, "*") == 0)
    {
	/* "HW=*" means accept any HWResource. */
	return TRUE;
    }
    return MatchDeviceName(token.tok_Value.v_String, hwName, DEVNAME_TYPE);
}

/**
|||	AUTODOC -class Kernel -group Devices -name CreateDeviceStackList
|||	Gets a list of device stacks which support the requested capabilities.
|||
|||	  Synopsis
|||
|||	    Err CreateDeviceStackList(List **pList, const DDFQuery query[]);
|||	    Err CreateDeviceStackListVA(List **pList, const char *name, DDFOp op, uint32 flags, uint32 numValues, ...);
|||
|||	  Description
|||
|||	    The list of all devices currently supported by the system
|||	    is searched for those devices which support all of the
|||	    capabilities in the provided array.  The returned list
|||	    of DeviceStacks describes all stacks of devices which
|||	    support the specified capabilities.
|||
|||	    The query is an array of DDFQuery structures.  In each, the
|||	    ddfq_Name field is the name of a device capability; the
|||	    ddfq_Operator field is a matching operator (DDFQ_EQ for
|||	    equality, DDFQ_GT for "greater than", etc.); the ddfq_Value
|||	    field is the value to be matched; and the ddfq_Flags field
|||	    is set to DDF_INT if the ddfq_Value field is an integer
|||	    (int32) or DDF_STRING if it is a string (char *).
|||
|||	    CreateDeviceStackListVA() provides a simplified query
|||	    interface.  The capability to be queried is specified
|||	    by the "name" argument.  The "op" argument specifies the
|||	    matching operator to be applied to each value.  The numValues
|||	    argument specifies the number of values following the flags
|||	    argument.  If the flags argument is DDF_INT, the values are
|||	    taken to be int32 values; if it is DDF_STRING they are taken
|||	    to be char * values.  Following the last value argument, this
|||	    entire sequence of arguments (name, op, flags, numValues,
|||	    and a list of values) may be repeated to query additional
|||	    capabilties.  The last value argument for the last capability
|||	    is followed by a (char*)NULL in place of the name argument.
|||
|||	  Arguments (CreateDeviceStackList)
|||
|||	    pList
|||	        Pointer to a List * which upon successful completion
|||	        is set to point to a list of DeviceStacks.
|||	        If no devices in the system satisfy the query,
|||	        the List will be empty.
|||	    query
|||	        Array of device description capability requirements.
|||
|||	  Arguments (CreateDeviceStackListVA)
|||
|||	    pList
|||	        Same as in CreateDeviceStackList.
|||
|||	    name
|||	        Name of device capability.
|||
|||	    op
|||	        Operator to be applied to each value (one of
|||	        DDF_EQ, DDF_GT, DDF_LT, DDF_GE, DDF_LE, DDF_NE).
|||
|||	    flags
|||	        DDF_INT if values are int32s, DDF_STRING if values are
|||	        char *'s.
|||
|||	    numValues
|||	        Number of value arguments.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/device.h>
|||
|||	  See Also
|||
|||	    OpenDeviceStack(), CloseDeviceStack(), DeleteDeviceStackList()
|||
**/

/*
 * Append the device represented by ddfn to the bottom of the DeviceStack.
 * If more devices can be appended to the bottom of the resulting stack,
 * recurse until we've created all possible stacks.
 */
Err
CreateDeviceStackListRecursive(List *devStackList, DeviceStack *devStack,
	const DDFNode *ddfn)
{
	DeviceStack *ds;
	DDFLink *child;
	HWResource hwr;
	Err err;

	if (!ddf_is_enabled(ddfn))
		return 0;

	if (ddf_is_llvl(ddfn))
	{
		/*
		 * Low-level device.
		 * Find all HWResources which can be appended to the
		 * bottom of the stack.
		 */
		for (hwr.hwr_InsertID = 0;
		     SuperQuerySysInfo(SYSINFO_TAG_HWRESOURCE,
			&hwr, sizeof(hwr)) == SYSINFO_SUCCESS; )
		{
			if (!DDFCanManage(ddfn, hwr.hwr_Name))
				continue;
			/*
			 * Append ddfn and the HWResource to the bottom
			 * of the DeviceStack and add the new DeviceStack
			 * to the list.
			 */
			ds = CopyDeviceStack(devStack);
			if (ds == NULL)
				return NOMEM;
			err = AppendDeviceStackDDF(ds, ddfn);
			if (err < 0)
			{
				DeleteDeviceStack(ds);
				return err;
			}
			err = AppendDeviceStackHW(ds, hwr.hwr_InsertID);
			if (err < 0)
			{
				DeleteDeviceStack(ds);
				return err;
			}
			AddTail(devStackList, (Node *) ds);
		}
	} else
	{
		/*
		 * High-level device.
		 * Append ddfn to the bottom of the DeviceStack and
		 * recurse to find all possible continuations of the stack.
		 */
		ds = CopyDeviceStack(devStack);
		if (ds == NULL)
			return NOMEM;
		err = AppendDeviceStackDDF(ds, ddfn);
		if (err < 0)
		{
			DeleteDeviceStack(ds);
			return err;
		}
		if (IsEmptyList(&ddfn->ddf_Children))
		{
			/* No children: end the stack here. */
			AddTail(devStackList, (Node *) ds);
			return 0;
		}
		/* Recurse thru all the children. */
		ScanList(&ddfn->ddf_Children, child, DDFLink)
		{
			if (DDFNodeInDeviceStack(ds, child->ddfl_Link))
				continue;
			err = CreateDeviceStackListRecursive(devStackList,
					ds, child->ddfl_Link);
			if (err < 0)
			{
				DeleteDeviceStack(ds);
				return err;
			}
		}
		DeleteDeviceStack(ds);
	}
	return 0;
}

#ifdef BUILD_STRINGS
void
PrintDeviceStack(const DeviceStack *ds)
{
	DeviceStackNode *dsn;
	HWResource hwr;

	ScanList(&ds->ds_Stack, dsn, DeviceStackNode)
	{
		if (dsn->dsn_IsHW)
		{
			if (FindHWResource(dsn->dsn_HWResource, &hwr) >= 0)
				printf("HW(%x)=%s ",
					dsn->dsn_HWResource, hwr.hwr_Name);
			else
				printf("HW(%x)=? ",
					dsn->dsn_HWResource);
		} else
			printf("%s ", dsn->dsn_DDFNode->ddf_n.n_Name);
	}
}
#endif

Err
CreateDeviceStackList(List **pList, const DDFQuery query[])
{
	List *devStackList;
	List *devList;
	DeviceListNode *dln;
	Err err;

	err = CreateDeviceList(&devList, query);
	if (err < 0)
		return err;

	devStackList = AllocMem(sizeof(List), MEMTYPE_NORMAL);
	PrepList(devStackList);

	ScanList(devList, dln, DeviceListNode)
	{
		err = CreateDeviceStackListRecursive(devStackList, NULL,
			dln->dln_DDFNode);
		if (err < 0)
			return err;
	}
	DeleteDeviceList(devList);

#ifdef DEBUG
{
	DeviceStack *ds;

	printf("CreateDeviceStackList: found stacks: \n");
	ScanList(devStackList, ds, DeviceStack)
	{
		printf("--  ");
		PrintDeviceStack(ds);
		printf("\n");
	}
	printf("----\n");
}
#endif

	*pList = devStackList;
	return 0;
}

/**
|||	AUTODOC -class Kernel -group Devices -name DeleteDeviceStackList
|||	Destroys the List created by CreateDeviceStackList().
|||
|||	  Synopsis
|||
|||	    void DeleteDeviceStackList(List* l);
|||
|||	  Description
|||
|||	    Deletes a list previously returned from CreateDeviceStackList.
|||	    The list itself, and all DeviceStacks in it, are deleted.
|||
|||	  Arguments
|||
|||	    l
|||	        Pointer to the List to be deleted.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V28.
|||
|||	  Associated Files
|||
|||	    <kernel/device.h>
|||
|||	  See Also
|||
|||	    CreateDeviceStackList()
|||
**/

void DeleteDeviceStackList(List *l)
{
    DeviceStack *ds;

    if (l == NULL)
	return;
    while ((ds = (DeviceStack *) RemHead(l)) != NULL)
    {
	DeleteDeviceStack(ds);
    }
    FreeMem(l, sizeof(List));
}

