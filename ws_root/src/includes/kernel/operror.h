#ifndef __KERNEL_OPERROR_H
#define __KERNEL_OPERROR_H


/******************************************************************************
**
**  @(#) operror.h 96/07/18 1.45
**
**  System error definitions.
**
**  Portfolio error codes are 32-bit numbers that are subdivided into
**  multiple sub-components. Using these components, you can identify
**  which subsystem generated the error, and get a fairly descriptive error
**  number. The various components of an error code are:
**
**	Bit(s)  Purpose
**	------  -------
**
**	31      Always set, makes the error codes negative numbers
**
**      25-30   Object type which generated the error. This tells you
**	        whether a folio, a device, or a task generated the
**	        error. Possible values for this field include ER_FOLI,
**	        ER_DEVC, ER_TASK, and ER_LINKLIB.
**
**	13-24   Object ID. This is a code that uniquely identifies the
**	        component that generated the error. The value for this field
**	        is created with the MakeErrId() macro and is basically two
**	        6-bit ASCII characters identifying the module. For example,
**	        Kernel errors have an object ID of 'Kr'.
**
**	11-12   A severity code for the error. This can be one of ER_INFO,
**	        ER_WARN, ER_SEVERE, or ER_FATAL.
**
**	9-10	An environment code which defines who created the component
**	        that generated the error. This can be one of ER_E_SSTM for
**	        system errors, ER_E_APPL for application errors, and
**	        ER_E_USER for user-code errors.
**
**	8	The error class. This can be either ER_C_STND for a
**	        standard error code, or ER_C_NSTND for a non-standard
**	        error codes. Standard error codes are errors which
**	        are well known in the system, and have a default string
**	        to describe them. Non-standard errors are module-specific
**	        and the module must provide a string to the system
**	        to describe the error.
**
**	0-7	The actual error code.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif


#define Make6Bit(n) ((n >= 'a')? (n-'a'+11): ((n <= '9')? (n-'0'+1):(n-'A'+37)))
#define MAKE6BIT(n) Make6Bit(n)

/* Bits in errfield */
#define ERR_ERRSIZE	(8)
#define ERR_CLASSIZE	(1)
#define ERR_ENVSIZE	(2)
#define ERR_SEVERESIZE	(2)
#define ERR_IDSIZE	(12)
#define ERR_OBJSIZE	(6)

#define ERR_ERRSHIFT	(0)
#define ERR_CLASHIFT	(ERR_ERRSHIFT+ERR_ERRSIZE)
#define ERR_ENVSHIFT	(ERR_CLASHIFT+ERR_CLASSIZE)
#define ERR_SEVERESHIFT	(ERR_ENVSHIFT+ERR_ENVSIZE)
#define ERR_IDSHIFT	(ERR_SEVERESHIFT+ERR_SEVERESIZE)
#define ERR_OBJSHIFT	(ERR_IDSHIFT+ERR_IDSIZE)

#define MAKEERR(o,id,severity,env,class,err) ((Err)(0x80000000L \
					| ((id)<<ERR_IDSHIFT) \
					| (severity<<ERR_SEVERESHIFT) \
					| (env<<ERR_ENVSHIFT) \
					| (class<<ERR_CLASHIFT) \
					| (o<<ERR_OBJSHIFT) \
					| (err<<ERR_ERRSHIFT) ))
#define MakeErr(o,id,severity,env,class,er) MAKEERR(o,id,severity,env,class,er)

/* Make ID from the 2-char erring object name */
#define	MakeErrId(ch1,ch2) ((Make6Bit((uchar)(ch1))<<6)|Make6Bit((uchar)(ch2)))
#define	MAKEERRID(ch1,ch2) MakeErrId(ch1,ch2)

/* Objects */
#define ER_FOLI	   Make6Bit('F')
#define ER_DEVC	   Make6Bit('D')
#define ER_TASK	   Make6Bit('T')
#define ER_LINKLIB Make6Bit('L')
#define ER_USER    Make6Bit('U')  /* for user code */

/* Folio ids */
#define ER_KRNL           MakeErrId('K','r')
#define ER_GRFX           MakeErrId('G','r')
#define ER_FSYS           MakeErrId('F','S')
#define ER_ADIO           MakeErrId('A','u')
#define ER_INTL           MakeErrId('I','n')
#define ER_COMP           MakeErrId('C','o')
#define ER_JSTR           MakeErrId('J','S')
#define ER_ICON           MakeErrId('I','c')
#define ER_REQ            MakeErrId('R','Q')
#define ER_FONT           MakeErrId('F','o')
#define ER_IFF            MakeErrId('I','F')
#define ER_FSUTILS        MakeErrId('F','U')
#define ER_MPA            MakeErrId('M','A')
#define ER_AUDIOPATCH     MakeErrId('A','P')
#define ER_FRAME2D        MakeErrId('F','2')
#define ER_SAVEGAME       MakeErrId('S','G')
#define ER_BATT           MakeErrId('B','a')
#define	ER_BEEP	          MakeErrId('B','p')
#define ER_AUDIOPATCHFILE MakeErrId('P','F')
#define ER_DATE           MakeErrId('D','a')
#define ER_BLITTER        MakeErrId('B','l')
#define ER_LOADER         MakeErrId('L','d')     /* code loader          */

/* Device ids */
#define	ER_TIMR	 MakeErrId('T','M')
#define ER_CDROM MakeErrId('C','D')
#define ER_CPORT MakeErrId('C','P')
#define ER_SER   MakeErrId('S','e')
#define ER_USLOT MakeErrId('u','S')
#define ER_TE    MakeErrId('T','E')

/* Task ids */
#define ER_EVBR  MakeErrId('E','B')

/* Link library ids */
#define ER_MUSICLIB      MakeErrId('M','u')     /* libmusic.a         */
#define ER_DATASTREAMING MakeErrId('D','S')
#define ER_CLT           MakeErrId('C','L')     /* command-list toolkit */

/* Severity */
#define ER_INFO   0
#define ER_WARN   1
#define ER_SEVERE 2
#define ER_SEVER  ER_SEVERE	/* for old code */
#define ER_KYAGB  3
#define ER_FATAL  ER_KYAGB

/* Environment */
#define ER_E_SSTM	0
#define ER_E_APPL	1
#define ER_E_USER	2
#define ER_E_RESERVED	3

/* Class */
#define ER_C_STND	0
#define ER_C_NSTND	1

/* Standard error codes */
#define ER_BadItem	  1	/* undefined Item passed in */
#define ER_BadTagArg	  2	/* undefined tag */
#define ER_BadTagArgVal	  3	/* bad value in tagarg list */
#define ER_NotPrivileged  4	/* attempt to do privileged op by nonpriv task */
#define ER_NotFound	  5	/* Item with that name not found */
#define ER_NoMem	  6	/* insufficient memory to complete */
#define ER_BadSubType	  7	/* Bad SubType for this Folio/Cmd*/
#define ER_SoftErr	  8	/* Evil OS error, should not occur! */
#define ER_BadPtr	  9	/* ptr/range is illegal for this task */
#define ER_Aborted	  10	/* operation aborted */
#define ER_NotEnabled     27	/* Component is not currently enabled */
#define ER_BadCommand	  12	/* Bad Command field in IOReq */
#define ER_BadIOArg	  13	/* Bad field in IOReq */
#define ER_BadName	  14	/* Bad Name for Item */
#define ER_IONotDone	  15	/* IO in progress for this IOReq */
#define ER_NotSupported	  16	/* Function not supported in this rev */
#define ER_IOIncomplete	  17	/* IO terminated but incomplete (bytes left over) */
#define ER_NotOwner	  18	/* Attempt to manipulate object not owned by task */
#define ER_DeviceOffline  19	/* Device is off-line and inaccessible */
#define ER_DeviceError    20	/* I/O error due to device/hardware trouble */
#define ER_MediaError     21	/* I/O error due to media (e.g. scratched CD) */
#define ER_EndOfMedium    22	/* Physical/logical end of medium or file */
#define ER_ParamError     23	/* I/O error due to illegal operation or parameters */
#define ER_NoSignals      24	/* No signals available */
#define ER_NoHardware     25	/* Required HW isn't available */
#define ER_DeviceReadonly 26	/* Device is read-only */


/*****************************************************************************/


/* Useful macros to build up error codes */

/* Make Folio-specific Errors */
#define MakeKErr(svr,class,err) MakeErr(ER_FOLI,ER_KRNL,svr,ER_E_SSTM,class,err)
#define MakeGErr(svr,class,err) MakeErr(ER_FOLI,ER_GRFX,svr,ER_E_SSTM,class,err)
#define MakeFErr(svr,class,err) MakeErr(ER_FOLI,ER_FSYS,svr,ER_E_SSTM,class,err)
#define MakeIErr(svr,class,err) MakeErr(ER_FOLI,ER_INTL,svr,ER_E_SSTM,class,err)

#define MAKEKERR(svr,class,err) MakeKErr(svr,class,err)
#define MAKEGERR(svr,class,err) MakeGErr(svr,class,err)
#define MAKEFERR(svr,class,err) MakeFErr(svr,class,err)
#define MAKEIERR(svr,class,err) MakeIErr(svr,class,err)

/* Make task errors */
#define MakeTErr(id,svr,class,err) MakeErr(ER_TASK,id,svr,ER_E_SSTM,class,err)
#define MAKETERR(id,svr,class,err) MakeTErr(id,svr,class,err)

/* Make link-library specific errors */
#define MakeLErr(id,svr,class,err) MakeErr(ER_LINKLIB,id,svr,ER_E_SSTM,class,err)
#define MAKELERR(id,svr,class,err) MakeLErr(id,svr,class,err)

/* Make Panic errors */
#define MakePanicErr(err)  MakeKErr(ER_FATAL,ER_C_NSTND,err)


/*****************************************************************************/


/* This is the structure used by the kernel to map error codes to strings */
typedef struct ErrorText
{
    ItemNode et;
    uint32   et_ObjID;	       /* 12 bit identifier       */
    char   **et_ErrorTable;    /* ptr to table of strings */
    uint8    et_MaxErr;	       /* size of table           */
    uint8    et_Reserved[3];
} ErrorText;

enum errtxt_tags
{
    ERRTEXT_TAG_OBJID = TAG_ITEM_LAST+1,
    ERRTEXT_TAG_MAXERR,
    ERRTEXT_TAG_TABLE
};


/*****************************************************************************/


/* Kernel-only non-standard error codes */
#define ER_Kr_BadType		1  /* Bad item type specifier                         */
#define ER_Kr_RsrcTblOvrFlw	2  /* Couldn't extend task resource table             */
#define ER_Kr_ItemTblOvrFlw	3  /* Couldn't extend system item table               */
#define ER_Kr_ItemNotOpen	4  /* Item is not opened by current task              */
#define ER_Kr_BadMemCmd		5  /* Illegal command for ControlMem()                */
#define ER_Kr_MsgSent		6  /* Message already sent                            */
#define ER_Kr_NoReplyPort	7  /* Message has no reply port                       */
#define ER_Kr_BadSize		8  /* Invalid size                                    */
#define ER_Kr_BadPriority	9  /* Invalid priority                                */
#define ER_Kr_MsgPortNotEmpty	10 /* MsgPort must be empty to perform this operation */
#define ER_Kr_BadQuanta		11 /* Quanta not in valid range                       */
#define ER_Kr_BadStackSize	12 /* Stack size not in valid range                   */
#define ER_Kr_CantOpen		13 /* Can't open                                      */
#define ER_Kr_ReplyPortNeeded	14 /* This operation requires a reply port            */
#define ER_Kr_IllegalSignal	15 /* Illegal signal                                  */
#define ER_Kr_BadLockArg	16 /* Illegal flag specified to LockSemaphore()       */
#define ER_Kr_CantSetOwner	17 /* Can't transfer item ownership                   */
#define ER_Kr_ItemStillOpened   18 /* Can't delete item since it is still open        */
#define ER_Kr_UniqueItemExists	19 /* An item of that name already exists             */
#define ER_Kr_BadExitStatus	20 /* Bad exit status                                 */
#define ER_Kr_TaskKilled	21 /* Task got killed                                 */
#define ER_Kr_MsgNotOnPort	22 /* The message is not currently on a message port  */
#define ER_Kr_NoIntHandler      23 /* No handler for interrupt                        */
#define ER_Kr_NoFuncPtr         24 /* Function ptr is NULL                            */
#define ER_Kr_CantCreate        25 /* Couldn't create an item                         */
#define ER_Kr_NoOperator        26 /* Couldn't create operator                        */
#define ER_Kr_MemLockErr        27 /* Error in MemLock                                */
#define ER_Kr_NoMemList         28 /* No mem list                                     */
#define ER_Kr_UnwantedException 29 /* Unexpected exception                            */
#define ER_Kr_BadExceptionNum   30 /* Invalid exception number                        */
#define ER_Kr_LogOverflow       31 /* Log buffer overflow                             */
#define ER_Kr_NoPersistentMem   32 /* No persistent memory available                  */
#define ER_Kr_BadCacheCmd	33 /* Illegal command for ControlCaches()             */

/* Kernel shortcuts */
#define BADTAG		  MakeKErr(ER_SEVERE,ER_C_STND,ER_BadTagArg)
#define BADTAGVAL	  MakeKErr(ER_SEVERE,ER_C_STND,ER_BadTagArgVal)
#define BADPRIV		  MakeKErr(ER_SEVERE,ER_C_STND,ER_NotPrivileged)
#define BADSUBTYPE	  MakeKErr(ER_SEVERE,ER_C_STND,ER_BadSubType)
#define BADITEM		  MakeKErr(ER_SEVERE,ER_C_STND,ER_BadItem)
#define NOMEM		  MakeKErr(ER_SEVERE,ER_C_STND,ER_NoMem)
#define BADPTR		  MakeKErr(ER_SEVERE,ER_C_STND,ER_BadPtr)
#define BADUNIT		  MakeKErr(ER_SEVERE,ER_C_STND,ER_BadUnit)
#define BADIOARG	  MakeKErr(ER_SEVERE,ER_C_STND,ER_BadIOArg)
#define BADCOMMAND	  MakeKErr(ER_SEVERE,ER_C_STND,ER_BadCommand)
#define ABORTED		  MakeKErr(ER_SEVERE,ER_C_STND,ER_Aborted)
#define BADNAME		  MakeKErr(ER_SEVERE,ER_C_STND,ER_BadName)
#define IOINCOMPLETE	  MakeKErr(ER_SEVERE,ER_C_STND,ER_IOIncomplete)
#define NOSUPPORT	  MakeKErr(ER_SEVERE,ER_C_STND,ER_NotSupported)
#define NOTOWNER	  MakeKErr(ER_SEVERE,ER_C_STND,ER_NotOwner)
#define PARAMERROR        MakeKErr(ER_SEVERE,ER_C_STND,ER_ParamError)
#define NOHARDWARE        MakeKErr(ER_SEVERE,ER_C_STND,ER_NoHardware)
#define NOTENABLED        MakeKErr(ER_SEVERE,ER_C_STND,ER_NotEnabled)
#define MSGSENT		  MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_MsgSent)
#define UNIQUEITEMEXISTS  MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_UniqueItemExists)
#define NOREPLYPORT	  MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_NoReplyPort)
#define BADSIZE		  MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_BadSize)
#define REPLYPORTNEEDED	  MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_ReplyPortNeeded)
#define ILLEGALSIGNAL	  MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_IllegalSignal)
#define KILLED            MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_TaskKilled)
#define MSGNOTONPORT      MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_MsgNotOnPort)
#define UNWANTEDEXCEPTION MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_UnwantedException)
#define BADEXCEPTIONNUM   MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_BadExceptionNum)
#define LOGOVERFLOW       MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_LogOverflow)
#define NOPERSISTENTMEM   MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_NoPersistentMem)
#define BADCACHECMD       MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_BadCacheCmd)


/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */

void PrintfSysErr(Item);	/* printfs the error */
int32 GetSysErr(char *buff,int32 buffsize,Item err);	/* fills buffer with error text */

#ifdef EXTERNAL_RELEASE
void	clib_PrintError(const char *who, const char *what, const char *whom, Err err);
#define	PrintError(who, what, whom, err) clib_PrintError(((char *)(who)),((char *)(what)),((char *)(whom)),((Err)(err)))
#else
#ifdef BUILD_STRINGS
void	clib_PrintError(const char *who, const char *what, const char *whom, Err err);
#define	PrintError(who, what, whom, err) clib_PrintError(((char *)(who)),((char *)(what)),((char *)(whom)),((Err)(err)))
#else
#define	PrintError(who, what, whom, err)
#endif
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#ifdef __DCC__
#pragma no_side_effects GetSysErr(1)
#endif


/*****************************************************************************/


#endif /* __KERNEL_OPERROR_H */
