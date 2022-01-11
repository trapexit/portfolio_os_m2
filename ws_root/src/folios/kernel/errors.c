/* @(#) errors.c 96/07/18 1.41 */

/* Error text management */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/kernelnodes.h>
#include <kernel/item.h>
#include <kernel/folio.h>
#include <kernel/kernel.h>
#include <kernel/semaphore.h>
#include <kernel/operror.h>
#include <stdio.h>
#include <string.h>
#include <kernel/internalf.h>


/* table defining ascii to 6 bit encoding and back */
static const char _SixToAscii[64] =
{
	' ',
	'0','1','2','3','4','5','6','7','8','9',
	'a','b','c','d','e','f','g','h','i','j','k','l','m','n',
	'o','p','q','r','s','t','u','v','w','x','y','z',
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N',
	'O','P','Q','R','S','T','U','V','W','X','Y','Z',
	'.'
};

#define	MakeASCII(n) (_SixToAscii[(n)])

static const char * const err_severity[] =
{
    "%s Info - ",
    "%s Warning - ",
    "Severe %s Error - ",
    "Fatal %s Error - "
};

static const char * const err_stnd[] =
{
    "no error",
    "Bad item number",
    "Undefined tag",
    "Bad argument in TagArg array",
    "Not privileged",
    "Item with that name not found",
    "Not enough memory to complete",
    "Bad subtype for this folio or command",
    "Evil system error, should not occur!",
    "Ptr/range is illegal for this task",
    "Operation aborted",
    "Component is not currently enabled",
    "Bad command in IOInfo",
    "Bad values in IOInfo",
    "Invalid name",
    "I/O in progress for this IOReq",
    "Operation not supported at this time",
    "I/O incomplete for this IOReq",
    "Item or memory not owned by current task",
    "Device is offline",
    "Generic device error",
    "Generic medium error",
    "End of medium",
    "Illegal parameter(s)",
    "No signals available",
    "Required hardware is not available",
    "Device is read-only",
};

static const char * const krnl_tbl[] =
{
    "no error",
    "Bad item type specifier",
    "Couldn't extend task resource table",
    "Couldn't extend system item table",
    "Item is not opened by current task",
    "Illegal command for ControlMem()",
    "Message already sent",
    "Message has no reply port",
    "Invalid size",
    "Invalid priority",
    "MsgPort must be empty to perform this operation",
    "Quanta not in valid range",
    "Stack size not in valid range",
    "Can't open",
    "This operation requires a reply port",
    "Illegal signal",
    "Illegal flag specified to LockSemaphore()",
    "Can't transfer item ownership",
    "Can't delete item since it is still open",
    "An item of that name already exists",
    "Bad exit status",
    "Task got killed",
    "The message is not currently on a message port",
    "No handler for interrupt",
    "Function ptr is NULL",
    "Couldn't create an item",
    "Couldn't create operator",
    "Error in MemLock",
    "No mem list",
    "Unexpected exception",
    "Invalid exception number",
    "Log buffer overflow",
    "No persistent memory available",
    "Illegal command for ControlCaches()",
};

const TagArg _ErrTA[] =
{
    TAG_ITEM_NAME,	(void *)"Kernel",
    ERRTEXT_TAG_OBJID,	(void *)((ER_FOLI<<ERR_IDSIZE)|(ER_KRNL)),
    ERRTEXT_TAG_MAXERR,	(void *)(sizeof(krnl_tbl)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,	(void *)krnl_tbl,
    TAG_END,		0
};


/*****************************************************************************/


static int32 icet_c(ErrorText *et, void *p, uint32 tag, uint32 arg)
{
    TOUCH(p);

    switch (tag)
    {
	case ERRTEXT_TAG_OBJID:
	{
            et->et_ObjID = arg;
            break;
        }

	case ERRTEXT_TAG_MAXERR:
	{
            et->et_MaxErr = (uint8)arg;
            break;
        }

	case ERRTEXT_TAG_TABLE:
	{
            et->et_ErrorTable = (char **)arg;
            break;
        }

	default:
	{
            return BADTAG;
        }
    }

    return 0;
}


/**
|||	AUTODOC -class Items -name ErrorText
|||	A table of error messages.
|||
|||	  Description
|||
|||	    An error text item is a table of error messages. The kernel maintains a
|||	    list of these items and uses them to convert a Portfolio error code into a
|||	    descriptive string.
|||
|||	  Folio
|||
|||	    Kernel
|||
|||	  Item Type
|||
|||	    ERRORTEXTNODE
|||
|||	  Create
|||
|||	    CreateItem()
|||
|||	  Delete
|||
|||	    DeleteItem()
|||
|||	  Use
|||
|||	    GetSysErr(), PrintfSysErr()
|||
|||	  Tags
|||
|||	    ERRTEXT_TAG_OBJID (uint32) - Create.
|||	        This tag specifies the 3
|||	        6-bit characters that idenify these errors.
|||	        You use the Make6Bit() macro to convert from
|||	        ASCII into the 6-bit character set.
|||
|||	    ERRTEXT_TAG_MAXERR (uint8) - Create.
|||	        Indicates the number of
|||	        error strings being defined. This
|||	        corresponds to the number of entries in the
|||	        string table.
|||
|||	    ERRTEXT_TAG_TABLE (const char **) - Create
|||	        A pointer to an array of
|||	        string pointers. These strings will be used
|||	        when converting an error code into a string.
|||	        The array is indexed by the lower-8 bits of
|||	        the error code.
|||
**/

Item
internalCreateErrorText(ErrorText *et, TagArg *a)
{
	Item ret;
	ErrorText *node;

	ret = TagProcessor(et, a, icet_c, 0);
	if (ret < 0) return ret;

        /* These values are required */
	if ((et->et_ObjID == 0)
	 || (et->et_MaxErr == 0)
	 || (et->et_ErrorTable == NULL))
	{
	    return BADTAGVAL;
	}

        /* the error table must be in RAM */
	if (!IsMemReadable(et->et_ErrorTable,et->et_MaxErr * sizeof(char *)))
	    return BADPTR;

        /* when initing the system, we come through here without a current task! */
        if (CURRENTTASK)
            externalLockSemaphore(KB_FIELD(kb_ErrorSemaphore),SEM_WAIT);

        /* make sure we don't already have errors for this object id */
	ScanList(&KB_FIELD(kb_Errors), node, ErrorText)
	{
	    if (node->et_ObjID == et->et_ObjID)
	    {
                externalUnlockSemaphore(KB_FIELD(kb_ErrorSemaphore));
	        return BADTAGVAL;
	    }
	}

	ADDTAIL(&KB_FIELD(kb_Errors),(Node *)et);

        if (CURRENTTASK)
            externalUnlockSemaphore(KB_FIELD(kb_ErrorSemaphore));

	return et->et.n_Item;
}

int32
internalDeleteErrorText(ErrorText *et, Task *t)
{
	TOUCH(t);

        externalLockSemaphore(KB_FIELD(kb_ErrorSemaphore),SEM_WAIT);
	REMOVENODE((Node *)et);
        externalUnlockSemaphore(KB_FIELD(kb_ErrorSemaphore));
	return 0;
}

/**
|||	AUTODOC -class Kernel -group Debugging -name GetSysErr
|||	Gets the error string for an error.
|||
|||	  Synopsis
|||
|||	    int32 GetSysErr(char *buff, int32 buffsize, Err err);
|||
|||	  Description
|||
|||	    This function returns a character string that describes a
|||	    Portfolio error code. The resulting string is placed in a buffer.
|||
|||	  Arguments
|||
|||	    buff
|||	        A pointer to a buffer to hold the error string.
|||
|||	    buffsize
|||	        The size of the error-text buffer, in bytes. This should be
|||	        at least 128.
|||
|||	    err
|||	        The error code whose error string to get.
|||
|||	  Return Value
|||
|||	    Returns the number copied to the description buffer, or a
|||	    negative error code if a bad buffer pointer is supplied.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/operror.h>, libc.a
|||
|||	  See Also
|||
|||	    PrintfSysErr()
|||
**/

int32 GetSysErr(char *ubuff, int32 n, Err i)
{
uint32        obj;
uint32        id;
uint32        severity;
uint32        errclass;
uint32        errnum;
uint32        maxerr;
char * const *errtbl;
ErrorText    *et;
char          local[128];
char          localName[25];
char         *name;

    if (n == 0)
    {
        /* buffer has got no room! */
        return 0;
    }

    if (IsUser())
    {
        if (!IsMemWritable(ubuff,n))
        {
            /* trying to pass a bad buffer, are we? */
            return BADPTR;
        }
    }

    if (i >= 0)
    {
        /* not an error code */
	sprintf(local,"<%d, not an error>",i);
    }
    else if (i == -1)
    {
        /* -1 is the catch-all error code */
	strcpy(local,"<-1, generic error>");
    }
    else
    {
        /* decompose the error code */
        obj      = ((uint32)i >> ERR_OBJSHIFT)    & ((1 << ERR_OBJSIZE) - 1);
        id       = ((uint32)i >> ERR_IDSHIFT)     & ((1 << ERR_IDSIZE) - 1);
        severity = ((uint32)i >> ERR_SEVERESHIFT) & ((1 << ERR_SEVERESIZE) - 1);
        errclass = ((uint32)i >> ERR_CLASHIFT)    & ((1 << ERR_CLASSIZE) - 1);
        errnum   = ((uint32)i >> ERR_ERRSHIFT)    & ((1 << ERR_ERRSIZE) - 1);

        LockSemaphore(KB_FIELD(kb_ErrorSemaphore), SEM_SHAREDREAD | SEM_WAIT);

	if (errclass == ER_C_STND)
	{
	    errtbl = err_stnd;
	    maxerr = sizeof(err_stnd)/sizeof(char *);
	}
	else
	{
	    errtbl = NULL;
	    maxerr = 0;
	}
	name = NULL;

        ScanList(&KB_FIELD(kb_Errors), et, ErrorText)
        {
            if (et->et_ObjID == ((obj << ERR_IDSIZE) | id))
            {
                if (errtbl == NULL)
                {
                    /* if we're not using the default error strings, use these */
                    errtbl = et->et_ErrorTable;
                    maxerr = et->et_MaxErr;
                }
                name = et->et.n_Name;
                break;
            }
	}

        if (name)
        {
            strncpy(localName, name, sizeof(localName));
            localName[sizeof(localName) - 1] = 0;
        }
        else
        {
            sprintf(localName,"%c%c%c",MakeASCII(obj),
                                       MakeASCII((id >> 6) & 0x3f),
                                       MakeASCII(id & 0x3f));
        }

        sprintf(local, err_severity[severity], localName);

	if ((errnum < maxerr) && IsMemReadable(errtbl[errnum],4))
        {
            strncat(local,errtbl[errnum],sizeof(local));
            local[sizeof(local) - 1] = 0;
        }
        else
        {
	    sprintf(&local[strlen(local)],"<%03u>",errnum);
        }

        UnlockSemaphore(KB_FIELD(kb_ErrorSemaphore));
    }

    strncpy(ubuff,local,n);
    ubuff[n - 1] = 0;

    return strlen(ubuff);
}


/**
|||	AUTODOC -class Kernel -group Debugging -name PrintfSysErr
|||	Prints the error string for an error.
|||
|||	  Synopsis
|||
|||	    void PrintfSysErr(Err err);
|||
|||	  Description
|||
|||	    Portfolio provides a uniform error management scheme. Errors
|||	    are denoted by negative values, and each error code is unique in
|||	    the system, allowing an easy determination of what caused the
|||	    error to occur.
|||
|||	    Each error code in Portfolio has a string associated with it that
|||	    attempts to describe the error in human terms. This function takes
|||	    a Portfolio error code and displays its string to the debugging
|||	    terminal.
|||
|||	  Arguments
|||
|||	    err
|||	        An error code as obtained from many Portfolio system calls.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V30.
|||
|||	  Associated Files
|||
|||	    <kernel/operror.h>, libc.a
|||
|||	  See Also
|||
|||	    GetSysErr()
|||
**/

void PrintfSysErr(Err err)
{
char errtext[128];

    GetSysErr(errtext, sizeof(errtext), err);
    printf("%s\n", errtext);
}


/**
|||	AUTODOC -class Kernel -group Debugging -name PrintError
|||	Prints the error string for an error.
|||
|||	  Synopsis
|||
|||	    void PrintError(const char *who, const char *what, const char *whom,
|||	                    Err err )
|||
|||	  Description
|||
|||	    This function the same effect as using printf() to print the who,
|||	    what, and whom strings with the string constructed by GetSysErr().
|||
|||	  Arguments
|||
|||	    who
|||	        A string that identifies the task, folio, or
|||	        module that encountered the error; if a NULL
|||	        pointer or a zero-length string is passed,
|||	        the name of the current task is used (or, if
|||	        there is no current task, "notask").
|||
|||	    what
|||	        A string that describes what action was
|||	        being taken that failed. Normally, the words
|||	        "unable to" are printed before this string
|||	        to save client data space; if the first
|||	        character of the "what" string is a
|||	        backslash, these words are not printed.
|||
|||	    whom
|||	        A string that describes the object to which
|||	        the described action is being taken (for
|||	        instance, the name of a file). It is printed
|||	        surrounded by quotes; if a NULL pointer or
|||	        zero length string is passed, the quotes are
|||	        not printed.
|||
|||	    err
|||	        A (normally negative) number denoting the
|||	        error that occurred, which will be decoded
|||	        with GetSysErr(). If not negative, then no
|||	        decoded error will be printed.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V29.
|||
|||	  Associated Files
|||
|||	    <kernel/operror.h>
|||
|||	  See Also
|||
|||	    PrintfSysErr(), GetSysErr()
|||
**/

void clib_PrintError(const char *who, const char *what, const char *whom, Err err)
{
#ifdef BUILD_STRINGS
	Task *ct;

	if (!who || !*who) {
		if ((ct = CURRENTTASK) != (Task *)0)
			who = ct->t.n_Name;
		else
			who = "notask";
	}
	printf("%s:", who);
	if (!what || !*what) {
		if (whom && *whom)
			what = "\\error from";
		else
			what = "\\error";
	}
	if (*what != '\\')
		printf(" unable to");
	else
		what++;
	printf(" %s", what);
	if (whom && *whom)
		printf(" '%s'", whom);
	printf("\n");
	if (err < 0)
		PrintfSysErr(err);
#else
	TOUCH(who);
	TOUCH(what);
	TOUCH(whom);
	TOUCH(err);
#endif
}
