/* @(#) EventBroker.c 96/10/08 1.50 */

/* The main top-level code for the Event Broker */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/msgport.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <kernel/time.h>
#include <kernel/semaphore.h>
#include <misc/event.h>
#include <misc/poddriver.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#ifdef BUILD_STRINGS
#define ERR(x) PrintfSysErr(x)
#else
#define ERR(x) TOUCH(x)
#endif

#define MASKFLAGS 0xFFFFFF00

typedef struct Listener {
  Node                  li;
  MsgPort              *li_Port;
  Item                  li_PortItem;
  enum ListenerCategory li_Category;
  uint32                li_TriggerMask[8]; /* events to trigger on */
  uint32                li_CaptureMask[8]; /* events to capture */
  uint8                 li_QueueMax;
  uint8                 li_QueueOutstanding;
  uint8                 li_HasFocus;
  uint8                 li_ToldFocus;
  uint8                 li_LostEvents;
  uint8                 li_NewlyInitialized;
  uint8                 li_ReportPortChange;
  uint8                 li_SeedCurrentStatus;
  uint8                 li_UsingPodStateTable;
} Listener;

typedef struct EventMsg {
  Node                  em;
  Message              *em_Message;
  Listener             *em_Listener;
} EventMsg;

int32 debugFlag;

static List listenerList;
static List eventMsgsPending;
static List eventMsgsAvail;
static Item eventBrokerPortItem;
static MsgPort *eventBrokerPort;
static Item eventReplyPortItem;
static MsgPort *eventReplyPort;
static Listener *focusListener;

static Item controlPortDeviceItem;
static Item controlPortWriteItem;
static Item controlPortConfigItem;
static IOReq *controlPortWrite;

static Item eventPortDeviceItem;

static int32 units;
static uint32 controlPortBufferSize;

static uint32 currentBufferSize;
static uint32 wantBufferSize;

static List gotPods;
static List lostPods;
List podDrivers;
List sharedDrivers;
static List readRequests;
static Pod *currentSplitter;
static ManagedBuffer *cpBuffer;
PodInterface *interfaceStruct;
static uint8 nextPodNumber = 0;
static int32 blipvert, flipvert;
static int32 bufferHoldoff = 5;
static int32 portChanged = FALSE;
static int32 busChanged = FALSE;
static int32 stabilityTimer;
static int32 forceToMaximum = TRUE;
int32 driverletInitiatedChange = FALSE;
static TimeValVBL timestamp;

#define FILTER_INHIBIT_TIME 3
#define STABILITY_DOWNSHIFT_TIME 30

static int32 filterInhibit = FILTER_INHIBIT_TIME;

static uint32 mergedPodFlags;
static uint32 currentConfigFlags;
static uint32 prevConfigFlags = 0xFFFFFFFF; /* force config update */

static List *portReaders;
static int32 *readersCamped;
static int32 *readersAllocated;

static PodStateTable *podStateTable;
const static uint32 podStateTableEntrySizes[16] = { PODSTATETABLEARRAYSIZES };

static MakeTableResponse tableResponse;

#define READER_LIMIT 3

#define EVENT_PAYLOAD_LIMIT 1024

struct {
  EventBrokerHeader ev;
  char              ev_Frame[EVENT_PAYLOAD_LIMIT];
} ev;

/* #defines for bogosity bailout codes in ProcessControlPortInput */
#define	BOGUS_NOBOGUS		0x0		/* no bogosity */
#define	BOGUS_POD_MISMATCH	0x00000001	/* pod id mismatch */
#define	BOGUS_POD_LOGOUT	0x00000002	/* pod logging out */
#define	BOGUS_POD_LOGIN		0x00000004	/* pod logging in */
#define	BOGUS_DEVICE_CHANGE	0x00000008	/* online device change */
#define	BOGUS_POD_ADDED		0x00000010	/* new pod device added */
#define BOGUS_NEED_MAXTRANSFER  0x00000020      /* must maximize xfer len */
#define BOGUS_PREVALIDATION     0x00000040      /* pod veto */

typedef struct {
  NamelessNode  reader;
  IOReq        *reader_IOReq;
  IOInfo        reader_IOInfo;
  uint32        reader_Busy;
  uint8		reader_Unit;
  uint8         reader_Buffer[1]; /* This array is of a variable
                                   * length. AddPortReader() allocates
                                   * sizeof(PortReader) + BufferSize,
                                   *so the buffer is at the end of the
                                   *PortReader structure.
                                   */
} PortReader;

typedef struct {
  NamelessNode  request;
  Item          request_Item;
} DeferredMessage;

static Boolean DequeuePendingEvent(Item msgItem);
static void ProcessNewMessage(Item msgItem, int32 isDeferred);
static Listener *GetListener(Item replyPort, int32 canAllocate);
static Message *GetMessageFor(Listener *listener, int32 dataSize);

static void AddStaticDriver(int32 deviceType, Err (*entry)());
static Err CallPodDriver(Pod *thisPod, PodInterface *podInterface);
extern Err ControlPadDriver(PodInterface *pi);
extern Err SplitterDriver(PodInterface *pi);
extern Err EndOfSplitterDriver(PodInterface *pi);
extern Err DefaultDriver(PodInterface *pi);
static int32 GetNextPodNumber(void);
static Pod *LocatePod(int32 podNumber);
static EventFrame *AllocEventFrame(uint32 eventNum,
				   uint32 frameSize,
				   EventFrame **next,
				   void **end);
static Err PackControlBits(uint32 dataBits,
			   uint32 bitCount,
			   uint32 leftJustified,
			   ManagedBuffer *buf,
			   uint32 bufferSegment);
static void AddPortReader(int32 unit, int32 bufferSize);
static void StartPortRead(PortReader *reader);
static void ProcessControlPortInput(PortReader *reader);
static void ProcessHighLevelEvents(PortReader *reader);
static void ConstructControlPortOutput(void);
static int32 ProcessPortInputList(int32 listNumber,
				  void (*processor) (PortReader *));
static void DispatchEvent(EventFrame *frame, uint32 actualBytes,
			  Item originatorItem, ControlPortInputHeader *header);
static void SeedEventMessages(void);
static Err MakePodStateTable(void);
static void UpdatePodStateTable(int32 portChanged);
static void DestroyPodStateTable(void);
static void DrainReplyPort(void);

extern int32 controlPortLine;


#ifdef DEBUG
# define DBUG(x)  printf x
#else
# define DBUG(x) /* x */
#endif

#ifdef DEBUG2
# define DBUG2(x)  printf x
#else
# define DBUG2(x) /* x */
#endif

#ifndef BUILD_STRINGS
# define DBUG0(x) /* x */
# define qprintf(x) /* x */
#else
# define DBUG0(x) printf x
# define qprintf(x) printf x
#endif

static const struct {
  uint8 in;
  uint8 out;
} BitTable[24] = {
  { 16, 16 },              /* RFU*/
  { 8, 4 },                /* Stereoscopic glasses, output only */
  { 32, 8 },               /* Gun */
  { 16, 8 },               /* RFU */
  { 8, 8 },                /* Gamesaver, off FIXME */
  { 16, 16 },              /* Gamesaver, read FIXME */
  { 16, 16 },              /* Gamesaver, write FIXME */
  { 16, 16 },              /* Gamesaver, reserved FIXME */
  { 16, 8 },               /* RFU */
  { 32, 2 },               /* Mouse */
  { 40, 2 },               /* Air mouse */
  { 24, 16 },              /* Keyboard */
  { 32, 2 },               /* Analog joystick */
  { 40, 8 },               /* Light pen */
  { 48, 8 },               /* Graphics tablet */
  { 40, 8 },               /* Steering wheel */
  { 24, 16 },              /* Diagnostics controller */
  { 16, 8 },               /* Barcode reader */
  { 40, 32 },              /* Infra-red transceiver */
  { 40, 32 },              /* Dual ported interconnection device */
  { 40, 32 },              /* Inertial controller */
  { 16, 8 },               /* RFU */
  { 32, 32 },              /* Control Port splitter */
  { 32, 32 },              /* reserved */
};

static const struct {
  uint8 in;
  uint8 out;
} FunkyTable[8] = {
  { 0, 0 },                /* unused entry */
  { 0, 0 },                /* unused entry */
  { 0, 0 },                /* unused entry */
  { 16, 8 },               /* IDs 0x58 - 0x5F */
  { 24, 8 },               /* IDs 0x60 - 0x67 */
  { 24, 24 },              /* IDs 0x68 - 0x6F */
  { 32, 32 },              /* IDs 0x70 - 0x77 */
  { 40, 32 },              /* IDs 0x78 - 0x7F */
};

static const PodInterface iTemplate = {
  1,
  PD_ParsePodInput,
  NULL,                     /* pod ptr */
  NULL,                     /* buffer management struct ptr */
  NULL,                     /* command-in ptr and len */
  0,
  NULL,                     /* command-out ptr and len */
  0,
  NULL,                     /* next-available-frame */
  NULL,                     /* end-of-frame-area */
  NULL,                     /* trigger event array ptr */
  NULL,                     /* capture event array ptr */
  0,                        /* VBL at capture time */
  FALSE,                    /* recovering from lost event? */
  PackControlBits,
  malloc,
  free,
  AllocEventFrame,
};


#ifdef MEMWATCH
void
printmemusage(l)
List *l;
{
	MemList *m;
	for ( m=(MemList *)FIRSTNODE(l);ISNODE(l,m); m = (MemList *)NEXTNODE(m))
	{
	    ulong *p = m->meml_OwnBits;
	    int ones = 0;
	    int32 size;
	    int i =  m->meml_OwnBitsSize;
	    while (i--) ones += CountBits(*p++);
	    size = ones*m->meml_MemHdr->memh_PageSize;
	    kprintf("MemList :0x%lx ",(long)m);
	    kprintf("%s allocated:%d ($%lx) ",m->meml_n.n_Name,
			(int)size,(ulong)size);
	    {
		int32 largest = 0;
		List *ml = m->meml_l;	/* list of freenodes */
		Node *n;
		size = 0;
		for (n = FIRSTNODE(ml); ISNODE(ml,n); n = NEXTNODE(n) )
		{
		    if (largest < n->n_Size) largest = n->n_Size;
		    size += n->n_Size;
		}
		kprintf("unused: %d largest=%d\n",(int)size,(int)largest);
	    }
	}
}
#endif

static Item OpenPortDevice(bool controlPort)
{
List *list;
Err   result;

    /* Find devices that support the right commands */

    if (controlPort)
    {
        result = CreateDeviceStackListVA(&list,"cmds", DDF_EQ, DDF_INT, 4,
                                         CPORT_CMD_READ,
                                         CPORT_CMD_WRITE,
                                         CPORT_CMD_CONFIGURE,
                                         CMD_STATUS,
                                         NULL);
    }
    else
    {
        result = CreateDeviceStackListVA(&list,"cmds", DDF_EQ, DDF_INT, 1,
                                         CPORT_CMD_READEVENT,
                                         NULL);
    }

    if (result >= 0)
    {
        if (!IsEmptyList(list))
        {
            /* Just take the first one. */
            result = OpenDeviceStack((DeviceStack *)FirstNode(list));
        }
        else
        {
            result = MAKEEB(ER_SEVER,ER_C_STND,ER_NoHardware);;
        }
        DeleteDeviceStackList(list);
    }

    return result;
}


static Err EventBrokerMain(uint32 parentSignal, Item parentTaskItem)
{
  Item msgItem;

  uint32 theSignal;

  int32 i;
  Err err;
  int32 writeInProgress;
  char *base;
  Listener *listener;

  IOInfo portIO;
  DeviceStatus status;

#ifdef MEMWATCH
  qprintf(("After launch:\n"));
  printmemusage(CURRENTTASK->t_FreeMemoryLists);
#endif

  PrepList(&eventMsgsPending);
  PrepList(&eventMsgsAvail);
  PrepList(&readRequests);
  PrepList(&sharedDrivers);
  PrepList(&listenerList);

  eventBrokerPortItem = CreateMsgPort(EventPortName, 0, 0);

  if (eventBrokerPortItem < 0) {
    PrintError(0,"create eventbroker port",0,eventBrokerPortItem);
    return 0;
  }

  eventBrokerPort = (MsgPort *) LookupItem(eventBrokerPortItem);

  DBUG(("Event broker port item 0x%x at 0x%x\n",eventBrokerPortItem,eventBrokerPort));

  eventReplyPortItem = CreateItem(MKNODEID(KERNELNODE,MSGPORTNODE), NULL);

  if (eventReplyPortItem < 0) {
    PrintError(0,"create event reply port",0,eventReplyPortItem);
    return 0;
  }

  eventReplyPort = (MsgPort *) LookupItem(eventReplyPortItem);

  DBUG(("Event reply port item 0x%x at 0x%x\n",eventBrokerPortItem,eventBrokerPort));

  PrepList(&gotPods);
  PrepList(&lostPods);
  PrepList(&podDrivers);

  AddStaticDriver(-1, DefaultDriver);
  AddStaticDriver(0xFE, EndOfSplitterDriver);
  AddStaticDriver(0x56, SplitterDriver);
  AddStaticDriver(0x80, ControlPadDriver);
  AddStaticDriver(0xA0, ControlPadDriver);
  AddStaticDriver(0xC0, ControlPadDriver);

  /*
   * At this point, the podDrivers list contains a linked list
   * of podDriver structures. Each is empty except for the
   * pd_DriverEntry and pd_DeviceType fields.
   * The list looks like this:
   *
   * (1st list item:)	PodDriver->pd_DeviceType = 0xc0
   *			PodDriver->pd_DriverEntry = ControlPadDriver
   *			  (rest of elements of this PodDriver are empty)
   *
   * (2nd list item:)	PodDriver->pd_DeviceType = 0xa0
   *			PodDriver->pd_DriverEntry = ControlPadDriver
   *			  (rest of elements of this PodDriver are empty)
   *
   *			. . .
   *
   * (last list item:)	PodDriver->pd_DeviceType = -1
   *			PodDriver->pd_DriverEntry = DefaultDriver
   *			  (rest of elements of this PodDriver are empty)
   *
   */

  controlPortDeviceItem = OpenPortDevice(TRUE);
  DBUG(("controlport device %x\n", controlPortDeviceItem));
  eventPortDeviceItem = OpenPortDevice(FALSE);
  DBUG(("eventport device item %x\n", eventPortDeviceItem));

  if (controlPortDeviceItem < 0) {
    PrintError(0,"open control port",0,controlPortDeviceItem);
    return controlPortDeviceItem;
  }
  if (eventPortDeviceItem < 0) {
    PrintError(0,"open event port",0,eventPortDeviceItem);
  }

  units = 1;
  if (eventPortDeviceItem >= 0) units++;

  portReaders = (List *) AllocMem(units * sizeof (List), MEMTYPE_FILL);
  readersCamped = (int32 *) AllocMem(units * sizeof (int32), MEMTYPE_FILL);
  readersAllocated = (int32 *) AllocMem(units * sizeof (int32), MEMTYPE_FILL);

  for (i = 0; i < units; i++) {
    readersAllocated[i] = 0;
    readersCamped[i] = 0;
    PrepList(&portReaders[i]);
    DBUG(("Init reader list for unit %d\n", i));
  }

  controlPortWriteItem = CreateIOReq(NULL, 0, controlPortDeviceItem, 0);

  if (controlPortWriteItem < 0) {
    PrintError(0,"creating IOReq",0,controlPortWriteItem);
    return -1;
  }

  controlPortWrite = (IOReq *) LookupItem(controlPortWriteItem);

  controlPortConfigItem = CreateIOReq(NULL, 0, controlPortDeviceItem, 0);

  if (controlPortConfigItem < 0) {
    PrintError(0,"creating IOReq",0,controlPortConfigItem);
    return -1;
  }

  DBUG(("ControlPort IOReq item 0x%x\n", controlPortWriteItem));

  memset(&portIO, 0, sizeof portIO);

  portIO.ioi_Recv.iob_Buffer = &status;
  portIO.ioi_Recv.iob_Len = sizeof status;
  portIO.ioi_Command = CMD_STATUS;

  err = DoIO(controlPortWriteItem, &portIO);

  if (err < 0) {
    PrintError(0,"get control port status",0,err);
    return -1;
  }

  controlPortBufferSize = status.ds_DeviceBlockSize *
    status.ds_DeviceBlockCount;

  currentBufferSize = 0;
  wantBufferSize = controlPortBufferSize;

  cpBuffer = (ManagedBuffer *) AllocMem(sizeof (ManagedBuffer),
					MEMTYPE_FILL);
  base = (uchar *) AllocMem(2 * controlPortBufferSize, MEMTYPE_FILL);

  if (!cpBuffer) {
    qprintf(("Unable to allocate Control Port buffers:\n"));
    return MAKEEB(ER_SEVER,ER_C_STND,ER_NoMem);
  }

  cpBuffer->mb_BufferTotalSize = controlPortBufferSize * 2;
  cpBuffer->mb_BufferSegmentSize = controlPortBufferSize;
  cpBuffer->mb_NumSegments = 3;
  cpBuffer->mb_Segment[MB_OUTPUT_SEGMENT].bs_SegmentBase = base;
  cpBuffer->mb_Segment[MB_FLIPBITS_SEGMENT].bs_SegmentBase = base +
    controlPortBufferSize;

  writeInProgress = FALSE;

  AddPortReader(0, controlPortBufferSize + sizeof (ControlPortInputHeader));

  if (units >= 2) {
    AddPortReader(1, EVENT_PAYLOAD_LIMIT);
  }

  SeedEventMessages();

  SendSignal(parentTaskItem, parentSignal);

#ifdef MEMWATCH
  qprintf(("After full startup:\n"));
  printmemusage(CURRENTTASK->t_FreeMemoryLists);
#endif

  blipvert = FALSE;

  while (TRUE) {

    /*
       See if we need to send new output data to the port.  If so, and
       if a write is not already in progress, do so.
    */

    if (forceToMaximum) {
      wantBufferSize = controlPortBufferSize;
    }

    if ((blipvert || currentBufferSize != wantBufferSize) &&
	!writeInProgress) {
      flipvert = FALSE;
      blipvert = FALSE;
      currentBufferSize = wantBufferSize;
      forceToMaximum = FALSE;
      ConstructControlPortOutput();
      memset(&portIO, 0, sizeof portIO);
      DBUG(("CP write, wantBufferSize %d\n", wantBufferSize));
      portIO.ioi_Command = CPORT_CMD_WRITE;
      portIO.ioi_Send.iob_Buffer =
	cpBuffer->mb_Segment[MB_OUTPUT_SEGMENT].bs_SegmentBase;
      if (flipvert) {
	portIO.ioi_Send.iob_Len = 2 * controlPortBufferSize;
      } else {
	portIO.ioi_Send.iob_Len = controlPortBufferSize;
      }
      portIO.ioi_CmdOptions = (timestamp & 0x0000FFFF) |
	(wantBufferSize << 16);
      err = SendIO(controlPortWriteItem, &portIO);
      if (err < 0) {
	qprintf(("Error starting Control Port write: "));
	return err;
      }
      writeInProgress = TRUE;
    }

    currentConfigFlags = mergedPodFlags;

    /*
       filterInhibit will go positive any time the bus configuration is
       in a state of flux, whenever listener configurations change,
       and whenever an event is triggered as the result of an identical
       frame of Control Port data being received.  Under these
       conditions, turn filtering completely off.

       Otherwise, we can filter unless:

       -  Doing so would result in a loss of pod state (i.e. driverlet to
          pod handshaking protocol failure), or
       -  Doing so would disrupt the pod state table.
    */

    if (filterInhibit > 0) {
      currentConfigFlags &= ~(POD_StateFilterOK|POD_EventFilterOK);
    } else if (!podStateTable) {
      currentConfigFlags |= POD_StateFilterOK;
    }

    if (currentConfigFlags != prevConfigFlags) {
      memset(&portIO, 0, sizeof portIO);
      portIO.ioi_Command = CPORT_CMD_CONFIGURE;
      portIO.ioi_CmdOptions = currentConfigFlags;
      DBUG(("Set CP configuration 0x%08X\n", currentConfigFlags));
      err = DoIO(controlPortConfigItem, &portIO);
      if (err < 0) {
	qprintf(("Could not configure Control Port: \n"));
	ERR(err);
      }
      prevConfigFlags = currentConfigFlags;
    }

    theSignal = eventBrokerPort->mp_Signal | SIGF_IODONE;

    ScanList(&listenerList, listener, Listener) {
      if (listener->li_LostEvents) {
	theSignal |= eventReplyPort->mp_Signal;
	break;
      }
    }

    theSignal = WaitSignal(theSignal);

    DrainReplyPort();

    if (theSignal & eventBrokerPort->mp_Signal) {
      while ((msgItem = GetMsg(eventBrokerPortItem)) > 0) {
	ProcessNewMessage(msgItem, FALSE);
      }
      if (msgItem < 0) {
	DBUG(("Broker GetMsg error 0x%x: ", msgItem));
	ERR(msgItem);
	return msgItem;
      }
    }

    if (theSignal & SIGF_IODONE) {

      ProcessPortInputList(0, ProcessControlPortInput);
      if (units >= 2) {
	ProcessPortInputList(1, ProcessHighLevelEvents);
      }

      if (writeInProgress) {
	err = CheckIO(controlPortWriteItem);
	if (err != 0) {
	  writeInProgress = FALSE;
	  if (err < 0 || (err = controlPortWrite->io_Error) < 0) {
	    qprintf(("Error writing Control Port: "));
	    return err;
	  }
	}
      }
      if (currentBufferSize != wantBufferSize) {
	filterInhibit = FILTER_INHIBIT_TIME;
      }

    }
  }
}

Err main(uint32 parentSignal, Item parentTaskItem)
{
Err result;

    result = EventBrokerMain(parentSignal, parentTaskItem);

    /* we only get back here in case of failure */
    ERR(result);

    SendSignal(parentTaskItem, parentSignal);

    return result;
}

static int32 ProcessPortInputList(int32 listNumber,
				 void (*processor) (PortReader *))
{
  int32 minCampers;
  PortReader *thisReader, *nextReader;
  Err err;
  int32 burstLimit;
  minCampers = readersCamped[listNumber];
  thisReader = (PortReader *) FIRSTNODE(&portReaders[listNumber]);
  burstLimit = debugFlag ? 2 : 10;
  while (IsNode(&portReaders[listNumber], thisReader) && --burstLimit >= 0) {
    nextReader = (PortReader *) NextNode(thisReader);
    if (thisReader->reader_Busy) {
      err = CheckIO(thisReader->reader_IOReq->io.n_Item);
      if (err < 0) {
	qprintf(("Error checking Control Port: "));
	ERR(err);
	return err;
      }
      if (err == 0) {
	break;
      }
      if ((err = thisReader->reader_IOReq->io_Error) < 0) {
	qprintf(("Error checking Control Port: "));
	ERR(err);
      }
      minCampers --;
      readersCamped[listNumber] --;
      thisReader->reader_Busy = FALSE;
      RemNode((Node *) thisReader);
      AddTail(&portReaders[listNumber], (Node *) thisReader);
      (*processor) (thisReader);
      StartPortRead(thisReader);
    }
    thisReader = nextReader;
  }
  if (minCampers < 1 && readersAllocated[listNumber] < READER_LIMIT) {
    AddPortReader(listNumber, controlPortBufferSize +
		  sizeof (ControlPortInputHeader));
  }
  return 0;
}

static Boolean DequeuePendingEvent(Item msgItem)
{
  EventMsg *pendingEvent;
  Listener *itsListener;
  pendingEvent = (EventMsg *) FirstNode(&eventMsgsPending);
  while (IsNode(&eventMsgsPending,pendingEvent)) {
    if (pendingEvent->em_Message->msg.n_Item == msgItem) {
      DBUG(("Event message 0x%x returned\n", msgItem));
      RemNode((Node *) pendingEvent);
      itsListener = pendingEvent->em_Listener;
      AddHead(&eventMsgsAvail, (Node *) pendingEvent);
      if (itsListener) {
	itsListener->li_QueueOutstanding --;
	if (itsListener->li_LostEvents) {
	  filterInhibit = FILTER_INHIBIT_TIME;
	}
	DBUG(("Listener queue now %d\n", itsListener->li_QueueOutstanding));
      } else {
	DBUG(("Orphaned event\n"));
      }
      return TRUE;
    }
    pendingEvent = (EventMsg *) NextNode(pendingEvent);
  }
  DBUG(("Message 0x%x no event-message-pending\n"));
  return FALSE;
}

static Pod *LocatePod(int32 podNumber)
{
  Pod *pod;
  pod = (Pod *) FirstNode(&gotPods);
  while (IsNode(&gotPods,pod)) {
    if (pod->pod_Number == podNumber) {
      return pod;
    }
    pod = (Pod *) NextNode(pod);
  }
  return (Pod *) NULL;
}

static void ProcessNewMessage(Item msgItem, int32 isDeferred)
{
  Message *incoming;
  EventBrokerHeader *header, response;
  ConfigurationRequest *configure;
  Listener *listener;
  int32 i, replySent;
  int32 responseDataManditory;
  int32 responseDataIsStatic;
  Err err, errorCode;
  void *sendResponseData;
  int32 sendResponseSize, deallocateResponseSize;
  SetFocus *setFocus;
  SendEvent *sendEvent;
  ListenerList *listeners;
  PodDescriptionList *podDescriptionList;
  Pod *pod;
  PodData *podData;
  DeferredMessage *defer;
#define PDR_Size 64
  struct {
    PodData      pr_Base;
    uint8        pd_Buf[PDR_Size];
  } podResponse;
  PodInterface podInterface;
  int32 responseSize, responseAvail;
  errorCode = sendResponseSize = deallocateResponseSize = 0;
  responseDataManditory = FALSE;
  responseDataIsStatic = FALSE;
  sendResponseData = NULL;
  response.ebh_Flavor = EB_NoOp;
  incoming = (Message *) CheckItem(msgItem, KERNELNODE, MESSAGENODE);
  if (!incoming) {
    return;
  }
  if (!incoming->msg_ReplyPort) {
    return;
  }
  if ((incoming->msg.n_Flags & MESSAGE_SMALL) ||
      (((uint32) incoming->msg_DataPtr) & 0x00000003) != 0 ||
      incoming->msg_DataSize < sizeof (EventBrokerHeader)) {
    DBUG(("Reject message, it's small or badly aligned\n"));
    ReplyMsg(msgItem, MAKEEB(ER_SEVERE,ER_C_STND,ER_BadPtr), NULL, 0);
    return;
  }
  header = (EventBrokerHeader *) incoming->msg_DataPtr;
  switch (header->ebh_Flavor) {
  case EB_NoOp:
    break;
  case EB_Configure:
    DBUG(("Got configure message from port 0x%x\n", incoming->msg_ReplyPort));
    configure = (ConfigurationRequest *) incoming->msg_DataPtr;
#ifdef BUILD_PARANOIA
    if ((configure->rfu[0] | configure->rfu[1] | configure->rfu[2] |
	 configure->rfu[3] | configure->rfu[4] | configure->rfu[5] |
	 configure->rfu[6] | configure->rfu[7]) != 0) {
      qprintf(("Reject request due to nonzero rfu field\n"));
      errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_ParamError);
      break;
    }
#endif
    filterInhibit = FILTER_INHIBIT_TIME;
    sendResponseData = &response;
    sendResponseSize = sizeof response;
    response.ebh_Flavor = EB_ConfigureReply;
    listener = GetListener(incoming->msg_ReplyPort, TRUE);
    if (!listener) {
      errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_SoftErr);
      break;
    }
    listener->li_Category = configure->cr_Category;
/**
  Rewrite what follows.  If we're switching from a non-focus listener to a
  focus listener, seize the focus.  If we switch from focus to non-focus,
  we must relinquish focus.  dcp
**/
    memcpy(listener->li_TriggerMask, configure->cr_TriggerMask,
	   sizeof listener->li_TriggerMask);
    memcpy(listener->li_CaptureMask, configure->cr_CaptureMask,
	   sizeof listener->li_CaptureMask);
    if (configure->cr_QueueMax > 0 &&
	configure->cr_QueueMax <= EVENT_QUEUE_MAX_PERMITTED) {
      listener->li_QueueMax = (uint8) configure->cr_QueueMax;
    }
    if (listener->li_NewlyInitialized) {
      listener->li_NewlyInitialized = FALSE;
      if (listener->li_Category == LC_FocusListener ||
	  listener->li_Category == LC_FocusUI) {
	if (focusListener) {
	  DBUG(("Removing focus from port 0x%x\n",
		 focusListener->li_PortItem));
	  focusListener->li_HasFocus = FALSE;
	}
	RemNode((Node *) listener);
	AddHead(&listenerList, (Node *) listener);
	focusListener = listener;
	focusListener->li_HasFocus = TRUE;
	DBUG(("Newly-established listener on port 0x%x has the focus\n",
	       focusListener->li_PortItem));
      }
    }
    break;
  case EB_SetFocus:
    response.ebh_Flavor = EB_SetFocusReply;
    setFocus = (SetFocus *) header;
    listener = GetListener(setFocus->sf_DesiredFocusListener, FALSE);
    if (listener) {
      if (listener->li_Category == LC_FocusListener ||
	  listener->li_Category == LC_FocusUI) {
	if (focusListener) {
	  focusListener->li_HasFocus = FALSE;
	}
	focusListener = listener;
	focusListener->li_HasFocus = TRUE;
	filterInhibit = FILTER_INHIBIT_TIME;
	errorCode = focusListener->li_PortItem;
      } else {
	errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_BadItem);
      }
    } else {
      errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_BadItem);
    }
    break;
  case EB_GetFocus:
    if (focusListener) {
      errorCode = focusListener->li_PortItem;
    } else {
      errorCode = 0;
    }
    /*DBUG(("Hit count %d, lossage count %d\n", controlPortHits, controlPortLossage));*/
    break;
  case EB_GetListeners:
    responseSize = incoming->msg_DataPtrSize;
    responseDataManditory = TRUE;
    if (responseSize < sizeof (ListenerList) ||
	(listeners = (ListenerList *) AllocMem(responseSize, MEMTYPE_FILL)) == NULL) {
      errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_NoMem);
      break;
    }
    listeners->ll_Header.ebh_Flavor = EB_GetListenersReply;
    listeners->ll_Count = 0;
    responseAvail = responseSize - sizeof (ListenerList);
    listener = (Listener *) FirstNode(&listenerList);
    while (IsNode(&listenerList,listener)) {
      if (responseAvail <= 0) {
	errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_NoMem);
	break;
      }
      i = listeners->ll_Count ++;
      listeners->ll_Listener[i].li_PortItem = listener->li_PortItem;
      listeners->ll_Listener[i].li_Category = listener->li_Category;
      listeners->ll_Listener[i].li_QueueOutstanding =
	listener->li_QueueOutstanding;
      responseAvail -= sizeof (listeners->ll_Listener[0]);
      listener = (Listener *) NextNode(listener);
    }
    sendResponseData = listeners;
    sendResponseSize = deallocateResponseSize = responseSize;
    break;
  case EB_DescribePods:
    /*responseSize = incoming->msg.n_Size - sizeof (Message);*/
    responseSize = incoming->msg_DataPtrSize;
    responseDataManditory = TRUE;
    if (responseSize < sizeof (PodDescriptionList) ||
	(podDescriptionList = (PodDescriptionList *) AllocMem(responseSize, MEMTYPE_FILL)) == NULL) {
      errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_NoMem);
      break;
    }
    podDescriptionList->pdl_Header.ebh_Flavor = EB_DescribePodsReply;
    podDescriptionList->pdl_PodCount = 0;
    responseAvail = responseSize - sizeof (PodDescriptionList);
    pod = (Pod *) FirstNode(&gotPods);
    while (IsNode(&gotPods,pod)) {
      if (responseAvail <= 0) {
	errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_NoMem);
	break;
      }
      i = podDescriptionList->pdl_PodCount ++;
      podDescriptionList->pdl_Pod[i].pod_Number = pod->pod_Number;
      podDescriptionList->pdl_Pod[i].pod_Position = pod->pod_Position;
      podDescriptionList->pdl_Pod[i].pod_Type = pod->pod_Type;
      podDescriptionList->pdl_Pod[i].pod_BitsIn = pod->pod_BitsIn;
      podDescriptionList->pdl_Pod[i].pod_BitsOut = pod->pod_BitsOut;
#ifdef MASKFLAGS
      podDescriptionList->pdl_Pod[i].pod_Flags = pod->pod_Flags & MASKFLAGS;
#else
      podDescriptionList->pdl_Pod[i].pod_Flags = pod->pod_Flags;
#endif
      podDescriptionList->pdl_Pod[i].pod_LockHolder = pod->pod_LockHolder;
      memcpy(podDescriptionList->pdl_Pod[i].pod_GenericNumber,
	     pod->pod_GenericNumber,
	     sizeof pod->pod_GenericNumber);
      responseAvail -= sizeof (podDescriptionList->pdl_Pod[0]);
      pod = (Pod *) NextNode(pod);
    }
    sendResponseData = podDescriptionList;
    sendResponseSize = deallocateResponseSize = responseSize;
    break;
  case EB_ReadPodData:
    if (!isDeferred) {
      defer = (DeferredMessage *) AllocMem(sizeof (DeferredMessage), MEMTYPE_FILL);
      if (!defer) {
	errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_NoMem);
	break;
      }
      defer->request_Item = msgItem;
      AddTail(&readRequests, (Node *) defer);
      return;
    }
  case EB_IssuePodCmd:
  case EB_WritePodData:
    /*responseSize = incoming->msg.n_Size - sizeof (Message);*/
    responseSize = incoming->msg_DataPtrSize;
    podData = (PodData *) header;
    pod = LocatePod(podData->pd_PodNumber);
    if (!pod || pod->pod_LoginLogoutPhase != POD_Online) {
      errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_DeviceError);
      break;
    }
    podInterface = iTemplate;
    podInterface.pi_Command = PD_ProcessCommand;
    podInterface.pi_Pod = pod;
    podInterface.pi_CommandIn = podData->pd_Data;
    podInterface.pi_CommandInLen = podData->pd_DataByteCount;
    podInterface.pi_CommandOut = podResponse.pr_Base.pd_Data;
    podInterface.pi_CommandOutLen = PDR_Size;
    memcpy(&podResponse.pr_Base, podData, sizeof (PodData));
    podResponse.pr_Base.pd_Header.ebh_Flavor =
      (enum EventBrokerFlavor) (header->ebh_Flavor + 1); /*ugh*/
    errorCode = CallPodDriver(pod, &podInterface);
    podResponse.pr_Base.pd_DataByteCount = podInterface.pi_CommandOutLen;
    if (podResponse.pr_Base.pd_DataByteCount > 0) {
      sendResponseSize = podResponse.pr_Base.pd_DataByteCount +
	sizeof (PodData);
      if (sendResponseSize <= responseSize) {
	sendResponseData = &podResponse;
      }
    }
    blipvert |= pod->pod_Blipvert; /* may need to push new output */
    break;
  case EB_SendEvent:
    if (!incoming->msg_ReplyPort) {
      break;
    }
    sendEvent = (SendEvent *) header;
    DispatchEvent(&sendEvent->se_FirstFrame,
		  incoming->msg_DataPtrSize - sizeof (EventBrokerHeader),
		  incoming->msg_ReplyPort, NULL);
    response.ebh_Flavor = EB_SendEventReply;
    break;
  case EB_MakeTable:
    listener = GetListener(incoming->msg_ReplyPort, TRUE);
    if (!listener) {
      errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_SoftErr);
      break;
    }
    errorCode = MakePodStateTable();
    if (errorCode < 0)
       break;

    tableResponse.mtr_Header.ebh_Flavor = EB_MakeTableReply;
    tableResponse.mtr_PodStateTable = podStateTable;
    sendResponseData = &tableResponse;
    sendResponseSize = sizeof tableResponse;
    responseDataIsStatic = TRUE;
    listener->li_UsingPodStateTable = TRUE;
    break;
  case EB_RegisterEvent:
  case EB_SendEventReply:
  case EB_CommandReply:
  case EB_EventRecord:
  case EB_EventReply:
  case EB_ConfigureReply:
  case EB_MakeTableReply:
  default:
   DBUG(("Sending reject due to arrival of message %d\n", header->ebh_Flavor));
    errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_NotSupported);
  }
  replySent = FALSE;
  if (sendResponseData) {
    err = 0;
    if (responseDataIsStatic ||
	(incoming->msg.n_Flags & MESSAGE_PASS_BY_VALUE)) {
      err = ReplyMsg(msgItem, errorCode, sendResponseData, sendResponseSize);
      if (err >= 0) {
	replySent = TRUE;
      }
    } else if (responseDataManditory) {
/*
  They asked for information, the information is of a variable and transient
  nature, and they didn't send a message which can carry the information
  back to them.
*/
	err = MAKEEB(ER_SEVERE,ER_C_STND,ER_NoMem);
    }
    if (errorCode == 0) {
      errorCode = err;
    }
  }
  if (!replySent) {
    (void) ReplyMsg(msgItem, errorCode, NULL, 0);
  }
  if (deallocateResponseSize) {
    FreeMem(sendResponseData, deallocateResponseSize);
  }
  return;
}

static Listener *GetListener(Item replyPort, int32 canAllocate)
{
  Listener *listener;
  listener = (Listener *) FirstNode(&listenerList);
  while (IsNode(&listenerList,listener)) {
    if (listener->li_PortItem == replyPort) {
      return listener;
    }
    listener = (Listener *) NextNode((Node *) listener);
  }
  if (!canAllocate) {
    return NULL;
  }
  listener = (Listener *) AllocMem(sizeof(Listener), MEMTYPE_FILL);
  if (!listener) {
    return listener;
  }
  listener->li_PortItem = replyPort;
  listener->li_QueueMax = EVENT_QUEUE_DEFAULT; /* useful default */
  listener->li_NewlyInitialized = TRUE;
#ifdef DOSEEDING
  listener->li_SeedCurrentStatus = TRUE;
#endif
  AddHead(&listenerList, (Node *) listener);
  DBUG(("Broker added event listener at port 0x%x\n", replyPort));
  return listener;
}

static EventFrame *AllocEventFrame(uint32 eventNum,
				   uint32 frameSize,
				   EventFrame **next,
				   void **end)
{
  EventFrame *frame;
  uint32 totalSize;
  frame = *next;
  if (frameSize + 2 * sizeof (EventFrame) >
      ((char *) *end) - ((char *) frame)) {
    DBUG(("Reject building a %d-byte frame, %d left\n", frameSize, ((char *) *end) - ((char *) frame)));
    return NULL;
  }
  totalSize = sizeof (EventFrame) + frameSize - sizeof frame->ef_EventData;
  DBUG(("Build frame, total size %x\n", totalSize));
  memset(frame, 0x00, totalSize);
  frame->ef_ByteCount = totalSize;
  frame->ef_EventNumber = (uint8) eventNum;
  frame->ef_SystemTimeStamp = timestamp;
  *next = (EventFrame *) (totalSize + (char *) frame);
  return frame;
}

static EventMsg *GetEventMsg(int32 messageSize)
{
  EventMsg *eventMsg;
  Item msgItem;
  Message *message;
  TagArg msgTags[3];
  msgTags[0].ta_Tag = CREATEMSG_TAG_REPLYPORT;
  msgTags[0].ta_Arg = (void *) eventReplyPortItem;
  msgTags[1].ta_Tag = CREATEMSG_TAG_DATA_SIZE;
  msgTags[1].ta_Arg = (void *) messageSize;
  msgTags[2].ta_Tag = TAG_END;
  msgItem = CreateItem(MKNODEID(KERNELNODE,MESSAGENODE), msgTags);
  if (msgItem < 0) {
    qprintf(("Can't create event message: "));
    ERR(msgItem);
    return NULL;
  }
  message = (Message *) LookupItem(msgItem);
  eventMsg = (EventMsg *) AllocMem(sizeof (EventMsg), MEMTYPE_FILL);
  if (!eventMsg) {
    DBUG(("EventBroker out of memory!\n"));
    DeleteItem(msgItem);
    return NULL;
  }
  eventMsg->em_Message = message;
  DBUG(("New message item 0x%x at 0x%x\n", message->msg.n_Item, message));
  DBUG(("Data area at 0x%x size %d, flags 0x%x\n", message->msg_DataPtr,
	message->msg_DataPtrSize, message->msg.n_Flags));
  return eventMsg;
}

static void DrainReplyPort(void)
{
  Item msgItem;
  while ((msgItem = GetMsg(eventReplyPortItem)) > 0) {
    if (!DequeuePendingEvent(msgItem)) {
      qprintf(("Bogus message 0x%X on reply port!\n", msgItem));
    }
  }
}

static Message *GetMessageFor(Listener *listener, int32 dataSize)
{
  int32 messageSize;
  EventMsg *eventMsg;
  Message *message;
  if (listener && listener->li_QueueOutstanding >= listener->li_QueueMax) {
    return NULL;
  }
  messageSize = dataSize;
  messageSize = (messageSize + 15) & 0xFFFFFFF0;
  eventMsg = (EventMsg *) FirstNode(&eventMsgsAvail);
  message = NULL;
  while (IsNode(&eventMsgsAvail,eventMsg)) {
    message = eventMsg->em_Message;
    if (message->msg_DataPtrSize >= messageSize) {
      RemNode((Node *) eventMsg);
      break;
    }
    message = NULL;
    eventMsg = (EventMsg *) NextNode((Node *) eventMsg);
  }
  if (!message) {
    eventMsg = GetEventMsg(messageSize);
    if (!eventMsg) {
      return NULL;
    }
    message = eventMsg->em_Message;
  }
  AddTail(&eventMsgsPending, (Node *) eventMsg);
  eventMsg->em_Listener = listener;
  return message;
}

static void AddStaticDriver(int32 deviceType, Err (*entry)(PodInterface *))
{
  PodDriver *pd;
  pd = (PodDriver *) AllocMem(sizeof (PodDriver), MEMTYPE_FILL);
  if (!pd) {
    return;
  }
  pd->pd_DriverEntry = entry;
  pd->pd_DeviceType = deviceType;
  AddHead(&podDrivers, (Node *) pd);
}

static Err PackControlBits(uint32 dataBits,
			   uint32 bitCount,
			   uint32 leftJustified,
			   ManagedBuffer *buf,
			   uint32 bufferSegment)
{
/*
  May wish to add a check to avoid walking off beginning of buffer if
  some device gets really piggish about output-buffer space
*/
  BufferSegment *seg;
  uint32 *bufferWord;
  uint32 wordIndex, bitsAvail, bitShift, bitsToChew;
  seg = &buf->mb_Segment[bufferSegment];
  DBUG(("Pack %d bits, %d filled now\n", bitCount, seg->bs_SegmentBitsFilled));
  wordIndex = (buf->mb_BufferSegmentSize / 4) - 1 -
    (seg->bs_SegmentBitsFilled / 32);
  bufferWord = ((uint32 *) seg->bs_SegmentBase) + wordIndex;
  if (leftJustified) {
    dataBits = dataBits >> (32 - bitCount);
  }
  while (bitCount > 0) {
    bitsAvail = 32 - (seg->bs_SegmentBitsFilled % 32);
    bitsToChew = (bitsAvail > bitCount) ? bitCount : bitsAvail;
    DBUG(("Pack %d bits into this word\n", bitsToChew));
    bitShift = 32 - bitsAvail;
    *bufferWord |= (dataBits & ~(((uint32) 0xFFFFFFFF) << bitsToChew)) << bitShift;
    dataBits = dataBits >> bitsToChew;
    seg->bs_SegmentBitsFilled += bitsToChew;
    bitCount -= bitsToChew;
    if (bitsAvail == bitsToChew) {
      bufferWord --;
    }
  }
  return 0;
}

static Err CallPodDriver(Pod *thisPod, PodInterface *podInterface)
{
  return (*thisPod->pod_Driver->pd_DriverEntry)(podInterface);
}

static void KillListener(Listener *listener)
{
  Listener *newListener;
  EventMsg *pendingEvent;
  RemNode((Node *) listener);
  FreeMem(listener, sizeof (Listener));
  pendingEvent = (EventMsg *) FirstNode(&eventMsgsPending);
  while (IsNode(&eventMsgsPending,pendingEvent)) {
    if (pendingEvent->em_Listener == listener) {
      DBUG(("Orphaning an event\n"));
      pendingEvent->em_Listener = NULL;
    }
    pendingEvent = (EventMsg *) NextNode(pendingEvent);
  }
  filterInhibit = FILTER_INHIBIT_TIME;
  if (listener == focusListener) {
    DBUG(("Focus listener on port 0x%x has been killed\n",
	   listener->li_PortItem));
    focusListener = NULL;
    newListener = (Listener *) FirstNode(&listenerList);
    while (IsNode(&listenerList,newListener)) {
      if (newListener->li_Category == LC_FocusListener ||
	  newListener->li_Category == LC_FocusUI) {
	focusListener = newListener;
	focusListener->li_HasFocus = TRUE;
	DBUG(("Reestablished port 0x%x as focus listener\n",
	       focusListener->li_PortItem));
	break;
      }
      newListener = (Listener *) NextNode(newListener);
    }
  }
}

/*
  This code isn't working quite right in some transient cases... two pods
  end up logging in with the same number.  To reproduce: start up with
  a glasses controller alone on the bus.  Let it log in.  Disconnect,
  plug it into an old-style pad, connect pad to 3DO port... both
  devices log in as ID 2.
*/

static int32 GetNextPodNumber(void) {
  Pod *thisPod;
  int32 isOK, tries;
  tries = 256;
  do {
    nextPodNumber ++;
    isOK = TRUE;
    thisPod = (Pod *) FirstNode(&gotPods);
    while (isOK && IsNode(&gotPods, thisPod)) {
      if (thisPod->pod_Number == nextPodNumber) {
	isOK = FALSE;
      } else {
	thisPod = (Pod *) NextNode(thisPod);
      }
    }
    thisPod = (Pod *) FirstNode(&lostPods);
    while (isOK && IsNode(&lostPods, thisPod)) {
      if (thisPod->pod_Number == nextPodNumber) {
	isOK = FALSE;
      } else {
	thisPod = (Pod *) NextNode(thisPod);
      }
    }
  } while (!isOK && --tries != 0);
  if (!isOK) {
    if (IsEmptyList(&lostPods)) {
      return -1;
    }
    thisPod = (Pod *) LastNode(&lostPods);
    RemNode((Node *) thisPod);
    nextPodNumber = thisPod->pod_Number;
    FreeMem(thisPod, sizeof (Pod));
  }
  return (int32) nextPodNumber;
}

static int32 TriggerListener(Listener *listener, uint32 *accumulatedEvents)
{
  if (!CheckItem(listener->li_PortItem, KERNELNODE, MSGPORTNODE)) {
    DBUG(("Listener died\n"));
    KillListener(listener);
    return FALSE;
  }
  if (listener->li_LostEvents &&
      listener->li_QueueOutstanding >= listener->li_QueueMax) {
    return -1; /* we might trigger but the queue is full */
  }
  if (listener->li_Category == LC_NoSeeUm) {
    return FALSE;
  }
  if (listener->li_HasFocus != listener->li_ToldFocus) {
    return TRUE;
  }
  if (listener->li_Category == LC_FocusListener &&
      listener != focusListener) {
    return FALSE;
  }
  if (listener->li_Category == LC_Observer ||
      listener == focusListener) {
    if (listener->li_HasFocus != listener->li_ToldFocus ||
	(listener->li_TriggerMask[0] & accumulatedEvents[0]) ||
	(listener->li_TriggerMask[1] & accumulatedEvents[1]) ||
	(listener->li_TriggerMask[4] & accumulatedEvents[4]) ||
	(listener->li_TriggerMask[5] & accumulatedEvents[5])) {
      return TRUE;
    }
  }
  if (listener->li_LostEvents ||
      (listener->li_ReportPortChange &&
       (listener->li_TriggerMask[2] & EVENTBIT2_ControlPortChange)) ||
      (listener->li_TriggerMask[2] & accumulatedEvents[2]) ||
      (listener->li_TriggerMask[3] & accumulatedEvents[3]) ||
      (listener->li_TriggerMask[6] & accumulatedEvents[6]) ||
      (listener->li_TriggerMask[7] & accumulatedEvents[7])) {
    return TRUE;
  }
  return FALSE;
}

static void DoBoilerplateEvents(Listener *listener,
				PodInterface *podInterface,
				ControlPortInputHeader *header)
{
  EventFrame *frame;
  if (listener->li_LostEvents) {
    (void) AllocEventFrame(EVENTNUM_EventQueueOverflow, 0,
			   &podInterface->pi_NextFrame,
			   &podInterface->pi_EndOfFrameArea);
  }
  if (header &&
      ((listener->li_TriggerMask[2] | listener->li_CaptureMask[2]) &
       EVENTBIT2_DetailedEventTiming)) {
    frame = AllocEventFrame(EVENTNUM_DetailedEventTiming,
			    sizeof *header,
			    &podInterface->pi_NextFrame,
			    &podInterface->pi_EndOfFrameArea);
    if (frame) {
      memcpy(frame->ef_EventData, header, sizeof *header);
    }
  }
  if (listener->li_ReportPortChange &&
      (listener->li_TriggerMask[2] & EVENTBIT2_ControlPortChange)) {
    (void) AllocEventFrame(EVENTNUM_ControlPortChange, 0,
			   &podInterface->pi_NextFrame,
			   &podInterface->pi_EndOfFrameArea);
  }
  if (listener->li_HasFocus != listener->li_ToldFocus) {
    listener->li_ToldFocus = listener->li_HasFocus;
    if (listener->li_HasFocus) {
      if (listener->li_TriggerMask[0] & EVENTBIT0_GivingFocus) {
	(void) AllocEventFrame(EVENTNUM_GivingFocus, 0,
			       &podInterface->pi_NextFrame,
			       &podInterface->pi_EndOfFrameArea);
      }
    } else {
      if (listener->li_TriggerMask[0] & EVENTBIT0_LosingFocus) {
	(void) AllocEventFrame(EVENTNUM_LosingFocus, 0,
			       &podInterface->pi_NextFrame,
			       &podInterface->pi_EndOfFrameArea);
      }
    }
  }
}

static void SendEventToListener(Listener *listener, void *ev, uint32 eventSize)
{
  Err err;
  Message *message;
  if (listener->li_QueueOutstanding >= listener->li_QueueMax) {
    DBUG(("Listener queue limit, events lost\n"));
    listener->li_LostEvents = TRUE;
    return;
  }
  message = GetMessageFor(listener, eventSize);
  if (message) {
    err = SendMsg(listener->li_PortItem, message->msg.n_Item,
		  ev, eventSize);
    if (err < 0) {
      if (debugFlag) {
	qprintf(("Failed to send event 0x%x to 0x%x:\n", message->msg.n_Item, listener->li_PortItem));
	ERR(err);
	qprintf(("Removing listener on port 0x%x\n", listener->li_PortItem));
      }
      KillListener(listener);
      (void) DequeuePendingEvent(message->msg.n_Item);
    } else {
      DBUG(("Sent 0x%x to 0x%x\n", message->msg.n_Item, listener->li_PortItem));
      listener->li_LostEvents = FALSE;
      listener->li_ReportPortChange = FALSE;
      listener->li_SeedCurrentStatus = FALSE;
      listener->li_QueueOutstanding ++;
    }
  } else {
    listener->li_LostEvents = TRUE;
    DBUG(("Listener 0x%x lost event\n", listener->li_PortItem));
  }
}

static void DispatchEvent(EventFrame *firstFrame, uint32 actualBytes,
			  Item originatorItem,
			  ControlPortInputHeader *header)
{
  uint32 accumulatedEvents[8];
  EventFrame *thisFrame, *targetFrame;
  Listener *listener, *nextListener;
  PodInterface podInterface;
  uint32 offset, frameSize, eventNumber, eventWord, eventBit;
  int32 eventSize;
/*
   Figure out what events arrived in this frameset of stuff
*/
  memset(accumulatedEvents, 0, sizeof accumulatedEvents);
  offset = 0;
  while (offset < actualBytes) {
    thisFrame = (EventFrame *) (offset + (char *) firstFrame);
    frameSize = thisFrame->ef_ByteCount;
    frameSize = (frameSize + 3) & ~ 3;
    if (frameSize + offset > actualBytes) {
      break;
    }
    eventNumber = thisFrame->ef_EventNumber;
    accumulatedEvents[eventNumber >> 5] |= 1L << (31 - (eventNumber & 0x1F));
    offset += frameSize;
  }
/*
  Go through the list of listeners, and send each one any events which
  are appropriate for it.
*/
  listener = (Listener *) FirstNode(&listenerList);
  while (IsNode(&listenerList,listener)) {
    DBUG(("Try listener 0x%x\n", listener));
    nextListener = (Listener *) NextNode((Node *) listener);
    if (TriggerListener(listener, accumulatedEvents) != TRUE) {
      listener = nextListener;
      continue;
    }
    ev.ev.ebh_Flavor = EB_EventRecord;
    podInterface.pi_NextFrame = (EventFrame *) &ev.ev_Frame;
    podInterface.pi_EndOfFrameArea = sizeof ev + (char *) &ev;
    DoBoilerplateEvents(listener, &podInterface, header);
    offset = 0;
    while (offset < actualBytes) {
      thisFrame = (EventFrame *) (offset + (char *) firstFrame);
      frameSize = thisFrame->ef_ByteCount;
      frameSize = (frameSize + 3) & ~ 3;
      if (frameSize + offset > actualBytes) {
	break;
      }
      eventNumber = thisFrame->ef_EventNumber;
      eventWord = eventNumber >> 5;
      eventBit = 1L << (31 - (eventNumber & 0x1F));
      if (eventBit & (listener->li_TriggerMask[eventWord] |
		      listener->li_CaptureMask[eventWord])) {
	targetFrame = podInterface.pi_NextFrame;
	if ((char *) podInterface.pi_EndOfFrameArea -
	    (char *) targetFrame >= frameSize + 2 * sizeof (EventFrame)) {
	  podInterface.pi_NextFrame =
	    (EventFrame *) (frameSize + (char *) targetFrame);
	  memcpy(targetFrame, thisFrame, frameSize);
	  if (originatorItem != 0) {
	    targetFrame->ef_Submitter = originatorItem;
	  }
	  targetFrame->ef_Trigger =
	    (eventBit & listener->li_TriggerMask[eventWord]) ? 1 : 0;
	  targetFrame->ef_ByteCount = frameSize;
	  if (targetFrame->ef_SystemTimeStamp == 0) {
	    targetFrame->ef_SystemTimeStamp = timestamp;
	  }
	} else {
	  listener->li_LostEvents = TRUE;
	}
      }
      offset += frameSize;
    }
    podInterface.pi_NextFrame->ef_ByteCount = 0;
    podInterface.pi_NextFrame =
      (EventFrame *) (sizeof (int32) + (char *) podInterface.pi_NextFrame);
    eventSize = ((char *) podInterface.pi_NextFrame) - ((char *) &ev.ev_Frame);
    DBUG(("Event built, %d bytes\n", eventSize));
    if (eventSize > sizeof (int32)) {
      eventSize += sizeof (EventBrokerHeader);
      SendEventToListener(listener, &ev, eventSize);
    }
    listener = nextListener;
  }
  return;
}

static void ProcessHighLevelEvents(PortReader *reader)
{
  DBUG(("Processing high-level event of %d bytes\n", reader->reader_IOReq->io_Actual));
  timestamp = reader->reader_IOReq->io_Info.ioi_Offset;
  DispatchEvent((EventFrame *) reader->reader_IOInfo.ioi_Recv.iob_Buffer,
		reader->reader_IOReq->io_Actual, 0, NULL);
  return;
}

static void ProcessControlPortInput(PortReader *reader)
{
  uint32 actual;
  uint8 streamByte, *buffer, topThree;
  int32 offset, devOffset, nextDevOffset, inputLength;
  int32 podNumber, podPosition, genericNumbers[16];
  int32 deviceCode;
  int32 inBits, outBits, totalOut, needBytes;
  int32 haltScan, i;
  int32 eventSize;
  int32 bogus;
  int32 hasSplitter;
  uint32 found, flags, identical;
  uint32 accumulatedEvents[8];
  uint8 jjj, kkkk;
  uint32 podStateTableInUse;
  ControlPortInputHeader *header;
  Err err;
  Pod *thisPod, *lastPod, *newPod;
  PodDriver *thisDriver;
  PodInterface podInterface;
  Listener *listener, *nextListener;
  DeferredMessage *defer;
  if (bufferHoldoff > 0) {
    -- bufferHoldoff;
    return;
  }
  filterInhibit --;
  bogus = BOGUS_NOBOGUS;
  currentSplitter = NULL;
  hasSplitter = FALSE;
  actual = reader->reader_IOReq->io_Actual;
  DBUG(("Got %d bytes\n", actual));
  buffer = cpBuffer->mb_Segment[MB_INPUT_SEGMENT].bs_SegmentBase =
    reader->reader_Buffer;
  header = (ControlPortInputHeader *) buffer;
  identical = header->cpih_IsIdentical;
  timestamp = header->cpih_VBL;
  podPosition = 0;
  offset = sizeof (ControlPortInputHeader);
  thisPod = (Pod *) FirstNode(&gotPods);
  haltScan = FALSE;
  totalOut = 0;
  busChanged |= driverletInitiatedChange;
  portChanged |= driverletInitiatedChange;
  driverletInitiatedChange = FALSE;
  while (offset <= actual && !haltScan) { /* assume we got at least one pullup byte */
    podPosition ++;
    devOffset = offset;
    streamByte = buffer[offset++];
    topThree = streamByte & POD_CONTROL_PAD_MASK;
    if (streamByte == PODID_SPLITTER_END_CHAIN_1 ||
	streamByte == PODID_END_CHAIN) {
      deviceCode = streamByte;
      inBits = 8;
      outBits = 2;
    } else if (topThree == PODID_3DO_OPERA_CONTROL_PAD) {
      deviceCode = topThree;
      inBits = 16;
      outBits = 2;
    } else if (topThree == PODID_3DO_M2_CONTROL_PAD) {
      deviceCode = topThree;
      inBits = 64;
      outBits = 8;
    } else if (topThree == PODID_3DO_SILLY_CONTROL_PAD) {
      deviceCode = topThree;
      inBits = 24;
      outBits = 8;
    } else if ((streamByte & 0xC0) == 0x40) {
      deviceCode = streamByte;
      if (deviceCode <= 0x57) {
	inBits = BitTable[streamByte & 0x3F].in;
	outBits = BitTable[streamByte & 0x3F].out;
      } else {
	inBits = FunkyTable[(deviceCode >> 3) & 0x07].in;
	outBits = FunkyTable[(deviceCode >> 3) & 0x07].out;
      }
    } else {
      deviceCode = streamByte;
      streamByte = buffer[offset++];
      if (deviceCode == 0x00) {
	deviceCode = ((uint32) buffer[offset++]) << 8;
      }
      jjj = (streamByte >> 4) & 0x07;
      kkkk = streamByte & 0x0F;
      if (jjj == 0x7) {
	jjj = buffer[offset++];
      }
      inBits = ((int32) jjj + 1) * 8;
      if (kkkk == 0x0F) {
	outBits = ((int32) buffer[offset++] + 1) * 8;
      } else {
	outBits = ((int32) kkkk + 1) * 2;
      }
    }
    DBUG2(("Device code 0x%x at offset %d\n", deviceCode, devOffset));
    DBUG2(("Input %d bits, output %d bits\n", inBits, outBits));
    nextDevOffset = devOffset + (inBits >> 3);
    if (nextDevOffset < offset) {
/*
   Whoops!  We got a pod whose input length is smaller than the
   amount of data we've already chewed through.  This can occur if a
   pod becomes ill and sends in a string of zeros.  In this case, halt
   scanning 'cause we can't trust the data which follows.  Don't force
   bogosity - let this device log in if it can, so we can diagnose the
   problem.
*/
      DBUG(("Erf... inBits %d, offset %d, nextDevOffset %d\n",
	     inBits, offset, nextDevOffset));
      haltScan = TRUE;
    } else {
      offset = nextDevOffset;
    }
    totalOut += outBits;
    if (IsNode(&gotPods,thisPod)) {
      if (thisPod->pod_Type != deviceCode) {
/*
  Pod mismatch - the data on this frame is inconsistent with the pod
  we thought was out there, or, we've hit the pullup code.

  Start or progress with disconnection of
  this pod and everything downstream.  Terminate scan of the port data
  during this cycle.
*/
	bogus |= BOGUS_POD_MISMATCH;
	if (thisPod->pod_LoginLogoutPhase != POD_LoggingIn) {
	  thisPod->pod_LoginLogoutPhase = POD_LoggingOut;
	}
	DBUG(("Start logout of type 0x%x due to seeing 0x%x at offset %d\n",
	       thisPod->pod_Type, deviceCode, devOffset));
	if (++ thisPod->pod_LoginLogoutTimer > POD_LoginLogoutDelay) {
	  qprintf(("Logged out pod number %d type 0x%x position %d offset %d\n",
		 thisPod->pod_Number, thisPod->pod_Type, podPosition,
		 devOffset));
	  busChanged = TRUE;
	  portChanged = TRUE;
	  if (currentSplitter) {
	    currentSplitter->pod_Flags = 0; /* turn off filtering */
	    currentSplitter->pod_PrivateData[0] = SplitterUncertain;
	    currentSplitter->pod_PrivateData[1] = LATCHBIT;
	    blipvert = TRUE; /* force new data out */
	    DBUG(("Device logged off arm 1, reconfigure splitter\n"));
	  }
	  while (TRUE) {
	    lastPod = (Pod *) LastNode(&gotPods);
	    RemNode((Node *) lastPod);
	    DBUG(("Detached pod number %d type 0x%x\n",
		  lastPod->pod_Number, lastPod->pod_Type));
	    /** Send the pod driver a farewell kiss **/
	    if (lastPod->pod_Driver) {
	      podInterface = iTemplate;
	      podInterface.pi_Command = PD_TeardownPod;
	      podInterface.pi_Pod = lastPod;
	      (void) CallPodDriver(lastPod, &podInterface);
	      thisDriver = lastPod->pod_Driver;
	      if (thisDriver->pd_Flags & PD_LOADED_INTO_RAM) {
		thisDriver->pd_UseCount --;
		if (thisDriver->pd_UseCount == 0 &&
		    !(thisDriver->pd_Flags & PD_SHARED)) {
		  DBUG(("Release RAM-loaded driverlet\n"));
		  podInterface.pi_Command = PD_ShutdownDriver;
		  (void) CallPodDriver(lastPod, &podInterface);
		  free(thisDriver->pd_DriverArea);
		  free(thisDriver);
		}
	      }
	      lastPod->pod_Driver = (PodDriver *) NULL;
	    }
	    if (lastPod->pod_SuccessfulLogin) {
	      AddHead(&lostPods, (Node *) lastPod);
	    } else {
	      FreeMem(lastPod, sizeof (Pod));
	    }
	    if (lastPod == thisPod) {
	      break;
	    }
	  }
	}
	haltScan = TRUE; /* force exit */
      } else {
	/*
	  Same kind we saw last time.  If pod was logging in or out during the
	  last cycle, move it towards online status and (if it finishes logging
	  in) give it a wakeup kiss.  In either case stop scanning the port for
	  this cycle.
	  */
	switch (thisPod->pod_LoginLogoutPhase) {
	case POD_LoggingOut:
	  DBUG(("Pod at position %d restabilizing\n", podPosition));
	  if (-- thisPod->pod_LoginLogoutTimer <= 0) {
	    DBUG(("Pod at position %d going back online\n", podPosition));
	    thisPod->pod_LoginLogoutPhase = POD_Online;
	  }
	  haltScan = TRUE; /* force exit */
	  bogus |= BOGUS_POD_LOGOUT;
	  break;
	case POD_LoggingIn:
	  DBUG(("Pod at position %d login continues\n", podPosition));
	  if (-- thisPod->pod_LoginLogoutTimer <= 0) {
	    thisPod->pod_LoginLogoutPhase = POD_LoadDriver;
	    DBUG(("Pod at position %d offset %d is now online\n",
	      podPosition, devOffset));
	  }
	  haltScan = TRUE; /* force exit */
	  bogus |= BOGUS_POD_LOGIN;
	  break;
	case POD_LoadDriver:
	  DBUG(("Here we load the driver for pod %d\n", podPosition));
	  thisPod->pod_LoginLogoutPhase = POD_Online;
	  haltScan = TRUE; /* force exit */
	  busChanged = TRUE;
	  portChanged = TRUE;
	  qprintf(("Logged in pod number %d type 0x%X position %d offset %d\n",
		 thisPod->pod_Number, deviceCode, podPosition,
		 devOffset));
	  thisDriver = (PodDriver *) FirstNode(&podDrivers);
	  while (IsNode(&podDrivers,thisDriver)) {
	    if (thisDriver->pd_DeviceType == -1 ||
		thisDriver->pd_DeviceType == deviceCode) {
	      thisPod->pod_Driver = thisDriver;
	      break;
	    }
	    thisDriver = (PodDriver *) NextNode(thisDriver);
	  }
	  if (thisPod->pod_Driver) {
	    podInterface = iTemplate;
	    podInterface.pi_ControlPortBuffers = cpBuffer;
	    if (thisPod->pod_SuccessfulLogin) {
	      podInterface.pi_Command = PD_ReconnectPod;
	    } else {
	      podInterface.pi_Command = PD_InitPod;
	    }
	    podInterface.pi_Pod = thisPod;
	    if ((err = CallPodDriver(thisPod, &podInterface)) < 0) {
	      thisPod->pod_LoginLogoutPhase = POD_InitFailure;
	      DBUG(("Pod init failed: "));
	      ERR(err);
	    } else {
	      thisPod->pod_SuccessfulLogin = TRUE;
	    }
	  } else {
	    DBUG(("No driver available for this pod\n"));
	  }
#ifdef MEMWATCH
	  qprintf(("After pod login:\n"));
	  printmemusage(CURRENTTASK->t_FreeMemoryLists);
#endif
	  break;
	case POD_Online:
	  /*
	    Update insie, outsie, position values in case the device or somebody
	    upstream changed things
	    */
	  if (currentSplitter) {
	    if (thisPod->pod_BitsIn != inBits ||
		thisPod->pod_BitsOut != outBits ||
		thisPod->pod_InputByteOffset != devOffset) {
	      DBUG(("Device change, reset splitter\n"));
	      currentSplitter->pod_PrivateData[0] = 0; /** force reset **/
	      bogus |= BOGUS_DEVICE_CHANGE;
	      haltScan = TRUE;
	    }
	    if (deviceCode == 0xFE) {
	      currentSplitter = NULL; /* hit air, end of arm 1 */
	    } else if (deviceCode == PODID_3DO_SPLITTER ||
		       deviceCode == PODID_3DO_SPLITTER_2) {
	      currentSplitter->pod_PrivateData[0] = 0; /** force reset **/
	      DBUG(("Second splitter on arm 1, reconfigure\n"));
	      haltScan = TRUE; /* splitter chained on arm 1, illegal, stop */
	    }
	  } else {
	    if (deviceCode == PODID_3DO_SPLITTER ||
		deviceCode == PODID_3DO_SPLITTER_2) {
	      currentSplitter = thisPod;
	      hasSplitter = TRUE;
	    }
	  }
	  thisPod->pod_BitsIn = inBits;
	  thisPod->pod_BitsOut = outBits;
	  thisPod->pod_InputByteOffset = devOffset;
	  break;
	case POD_InitFailure:
	  break;
	}
      }
      thisPod = (Pod *) NextNode(thisPod);
    } else {
      if (streamByte == PODID_END_CHAIN) {
	break; /*** end of input stream ***/
      }
/*
  Something has been added to the daisychain.  Search the lost-pods list
  for a pod of this type and (if one is found) steal it - the new device
  acquires the identity of the old/lost one.  If no such pod can be found,
  create a new one.  In either case, terminate the scan through the port
  during this cycle.
*/
      if (currentSplitter && deviceCode != PODID_SPLITTER_END_CHAIN_1) {
	currentSplitter->pod_PrivateData[0] = 0; /** force reset **/
	DBUG(("New device added to arm 1, reconfigure splitter\n"));
      }
      found = FALSE;
      bogus |= BOGUS_POD_ADDED | BOGUS_NEED_MAXTRANSFER;
      newPod = (Pod *) FirstNode(&lostPods);
      while (IsNode(&lostPods,newPod)) {
	if (newPod->pod_Type == deviceCode) {
	  found = TRUE;
	  break;
	}
	newPod = (Pod *) NextNode(newPod);
      }
      if (found) {
	DBUG(("Recycling pod number %d type 0x%X\n", newPod->pod_Number,
	       deviceCode));
	RemNode((Node *) newPod);
      } else {
	podNumber = GetNextPodNumber();
	if (podNumber < 0) {
	  DBUG(("Can't get new pod number\n"));
	  goto bailout;
	}
	DBUG(("Got new pod number %d\n", podNumber));
	newPod = (Pod *) AllocMem(sizeof (Pod), MEMTYPE_FILL);
	if (!newPod) {
	  DBUG(("Cannot allocate memory space for new pod\n"));
	  goto bailout;
	}
	newPod->pod_Type = deviceCode;
	newPod->pod_Number = (uint8) podNumber;
	DBUG(("New pod allocated\n"));
      }
      AddTail(&gotPods, (Node *) newPod);
      newPod->pod_LoginLogoutPhase = POD_LoggingIn;
      newPod->pod_LoginLogoutTimer = POD_LoginLogoutDelay;
      newPod->pod_BitsIn = inBits;
      newPod->pod_BitsOut = outBits;
      newPod->pod_InputByteOffset = devOffset;
      DBUG(("Pod type 0x%X offset %d set for login\n", deviceCode, devOffset));
      haltScan = TRUE;
    }
  }
 bailout:
  DBUG2(("Final offset was %d\n", offset));
  inputLength = offset;
/*
  If nothing went bogus so far, scan through the pod chain and do a
  prevalidation of the input.  This gives specialized pods (e.g. splitters)
  the ability to report bogosity, if they have reason to believe that
  data on the bus may be unreliable.
*/
  if (!bogus) {
    ScanList(&gotPods, thisPod, Pod) {
      if (thisPod->pod_Driver) {
	podInterface = iTemplate;
	podInterface.pi_Command = PD_PrecheckPodInput;
	podInterface.pi_ControlPortBuffers = cpBuffer;
	podInterface.pi_Pod = thisPod;
	if (CallPodDriver(thisPod, &podInterface) < 0) {
	  bogus |= BOGUS_PREVALIDATION;
	  blipvert = TRUE;
	}
      }
    }
  }
  if (bogus) {
    DBUG(("Bogosity occurred, code 0x%X\n", bogus));
/*
   Turn off identical-frame filtering.  If necessary (a new pod is starting
   to log in), kick the DMA size back up to the maximum.  Reset the stability
   timer, to ensure that we don't try to shrink the DMA transfer again too
   quickly.
*/
    stabilityTimer = 0;
    filterInhibit = FILTER_INHIBIT_TIME;
    if (bogus & BOGUS_NEED_MAXTRANSFER) {
      DBUG(("Need maximum-sized transfer\n"));
      forceToMaximum = TRUE;
      blipvert = TRUE;
    }
    return;
  }
/*
   Bump the "bus is reasonably stable" timer.  Turn off filtering if
   we haven't gotten to a point of reasonable stability.
*/
  if (++stabilityTimer < STABILITY_DOWNSHIFT_TIME) {
    filterInhibit = FILTER_INHIBIT_TIME;
  }
/*
  Tell all the pods to parse their input and record the identities
  of the events they wish to generate this time around.
*/
  if (busChanged) {
    memset(genericNumbers, 0, sizeof genericNumbers);
    podPosition = 0;
  }
  memset(accumulatedEvents, 0, sizeof accumulatedEvents);

  mergedPodFlags = POD_ShortFramesOK + POD_MultipleFramesOK +
    POD_EventFilterOK + POD_StateFilterOK + POD_FastClockOK;

  ScanList(&gotPods, thisPod, Pod) {
    flags = thisPod->pod_Flags;
    mergedPodFlags &= flags;
    if (busChanged) {
      thisPod->pod_Position = (uint8) ++ podPosition;
      i = 0;
      do {
	if (flags & 0x80000000) {
	  thisPod->pod_GenericNumber[i] = (uint8) ++ genericNumbers[i];
	  DBUG0(("Pod position %d is generic %d of class %d\n", podPosition, thisPod->pod_GenericNumber[i], i));
	} else {
	  thisPod->pod_GenericNumber[i] = 0;
	}
	flags = flags << 1;
      } while (++i < 16);
    }
    switch (thisPod->pod_LoginLogoutPhase) {
    case POD_InitFailure:
      break;
    case POD_Online:
      if (thisPod->pod_Driver) {
	podInterface = iTemplate;
	podInterface.pi_Command = PD_ParsePodInput;
	podInterface.pi_ControlPortBuffers = cpBuffer;
	podInterface.pi_Pod = thisPod;
	podInterface.pi_VBL = timestamp;
	thisPod->pod_EventsReady[0] =
	  thisPod->pod_EventsReady[1] =
	    thisPod->pod_EventsReady[2] =
	      thisPod->pod_EventsReady[3] =
		thisPod->pod_EventsReady[4] =
		  thisPod->pod_EventsReady[5] =
		    thisPod->pod_EventsReady[6] =
		      thisPod->pod_EventsReady[7] = 0;
	if ((err = CallPodDriver(thisPod, &podInterface)) < 0) {
	  thisPod->pod_LoginLogoutPhase = POD_InitFailure;
	  DBUG(("Pod parse failed: "));
	  ERR(err);
	}
	accumulatedEvents[0] |= thisPod->pod_EventsReady[0];
	accumulatedEvents[1] |= thisPod->pod_EventsReady[1];
	accumulatedEvents[2] |= thisPod->pod_EventsReady[2];
	accumulatedEvents[3] |= thisPod->pod_EventsReady[3];
	accumulatedEvents[4] |= thisPod->pod_EventsReady[4];
	accumulatedEvents[5] |= thisPod->pod_EventsReady[5];
	accumulatedEvents[6] |= thisPod->pod_EventsReady[6];
	accumulatedEvents[7] |= thisPod->pod_EventsReady[7];
	mergedPodFlags &= thisPod->pod_Flags; /* catch filter turnoff */
      }
      break;
    default:
      break;
    }
    blipvert |= thisPod->pod_Blipvert;
    thisPod->pod_Blipvert = FALSE;
  }
  busChanged = FALSE;
/*
  Go through the list of listeners, and send each one any events which
  are appropriate for it.  Keep track of whether anybody is still using
  the in-memory event table.
*/
  podStateTableInUse = FALSE;
  listener = (Listener *) FirstNode(&listenerList);
  while (IsNode(&listenerList, listener)) {
    DBUG(("Try listener 0x%x\n", listener));
    podStateTableInUse |= listener->li_UsingPodStateTable;
    nextListener = (Listener *) NextNode((Node *) listener);
    listener->li_ReportPortChange |= (uint8) portChanged;
    switch (TriggerListener(listener, accumulatedEvents)) {
    case TRUE:
      if (identical) {
	filterInhibit = FILTER_INHIBIT_TIME;
      }
      break;
    case -1: /* might have triggered, but the listener queue was full */
      if (identical) {
	filterInhibit = FILTER_INHIBIT_TIME;
      }
      /**** drop through ****/
    case FALSE:
      listener = nextListener;
      continue;
    }
    ev.ev.ebh_Flavor = EB_EventRecord;
    podInterface = iTemplate;
    podInterface.pi_Command = PD_AppendEventFrames;
    podInterface.pi_VBL = timestamp;
    podInterface.pi_ControlPortBuffers = cpBuffer;
    podInterface.pi_NextFrame = (EventFrame *) &ev.ev_Frame;
    podInterface.pi_EndOfFrameArea = sizeof ev + (char *) &ev;
    podInterface.pi_TriggerMask = listener->li_TriggerMask;
    podInterface.pi_CaptureMask = listener->li_CaptureMask;
    podInterface.pi_RecoverFromLostEvents = listener->li_LostEvents |
      listener->li_SeedCurrentStatus;
    DoBoilerplateEvents(listener, &podInterface, header);
    thisPod = (Pod *) FirstNode(&gotPods);
    while (IsNode(&gotPods,thisPod)) {
      if (thisPod->pod_LoginLogoutPhase == POD_Online && thisPod->pod_Driver) {
	podInterface.pi_Pod = thisPod;
	if ((err = CallPodDriver(thisPod, &podInterface)) < 0) {
	  DBUG(("Pod event-build failed: "));
	  ERR(err);
	}
      }
      thisPod = (Pod *) NextNode(thisPod);
    }
    podInterface.pi_NextFrame->ef_ByteCount = 0;
    podInterface.pi_NextFrame =
      (EventFrame *) (sizeof (int32) + (char *) podInterface.pi_NextFrame);
    eventSize = ((char *) podInterface.pi_NextFrame) - ((char *) &ev.ev_Frame);
    DBUG(("Event built, %d bytes\n", eventSize));
    if (eventSize > sizeof (int32)) {
      eventSize += sizeof (EventBrokerHeader);
      SendEventToListener(listener, &ev, eventSize);
    }
    listener = nextListener;
  }
/*
   Update the in-memory event table if present and still needed.
*/
  if (podStateTable) {
    if (podStateTableInUse) {
      UpdatePodStateTable(portChanged);
    } else {
      DestroyPodStateTable();
    }
  }
/*
  Process any ReadPodData messages which had been queued up since
  the last time we came through here.  We have a stable buffer of
  data, newly-received;  the driverlets can use it.
*/
  while (!IsEmptyList(&readRequests)) {
    defer = (DeferredMessage *) RemHead(&readRequests);
    ProcessNewMessage(defer->request_Item, TRUE);
    FreeMem(defer, sizeof (DeferredMessage));
  }
  portChanged = FALSE;
/*
   Figure out the port DMA transfer length to use during future frames.

   If devices are in the process of logging in, and for about half a second
   afterwards, use maximum-length transfers to make sure we get all of the
   data.  Also, use maximum-lenght transfers if any pod requires them
   (unlikely but possible).  Otherwise, consider shifting to a shorter
   transfer to speed up the DMA operation.

   If a splitter is present, be conservative and use the _sum_ of the
   input and output lengths (avoiding input/output overlap, which does
   not always work when splitters are involved)... otherwise, use the
   _larger_ of the input and output lengths.  Add 5 bytes to allow for
   the maximum-length header of a newly connected pod.  Round up to a
   doubleword.  Add an extra doubleword just to be safe.
*/

  if ((stabilityTimer < STABILITY_DOWNSHIFT_TIME &&
       currentBufferSize == controlPortBufferSize) ||
      (mergedPodFlags & POD_ShortFramesOK) == 0) {
    DBUG(("Want maximum buffer size, timer %d, mpf 0x%X\n",
	  stabilityTimer, mergedPodFlags));
    wantBufferSize = controlPortBufferSize;
  } else {
    totalOut = (totalOut + 7) >> 3;
    if (hasSplitter) {
      needBytes = inputLength + totalOut;
    } else {
      needBytes = (inputLength > totalOut) ? inputLength : totalOut;
    }
    needBytes = ((needBytes + (5+7)) & ~ 7) + 8;
    if (needBytes > controlPortBufferSize) {
      needBytes = controlPortBufferSize;
    }
    wantBufferSize = needBytes;
    DBUG(("Want %d bytes\n", needBytes));
  }
  return;
}

static void ConstructControlPortOutput()
{
  uint32 consumed;
  Err err;
  Pod *thisPod;
  PodInterface podInterface;
  memset(cpBuffer->mb_Segment[MB_OUTPUT_SEGMENT].bs_SegmentBase, 0,
	 cpBuffer->mb_BufferSegmentSize);
  memset(cpBuffer->mb_Segment[MB_FLIPBITS_SEGMENT].bs_SegmentBase, 0,
	 cpBuffer->mb_BufferSegmentSize);
  cpBuffer->mb_Segment[MB_OUTPUT_SEGMENT].bs_SegmentBitsFilled =
    cpBuffer->mb_Segment[MB_FLIPBITS_SEGMENT].bs_SegmentBitsFilled =
      (controlPortBufferSize - currentBufferSize) * 8;
  podInterface = iTemplate;
  podInterface.pi_Command = PD_ConstructPodOutput;
  podInterface.pi_VBL = timestamp;
  podInterface.pi_ControlPortBuffers = cpBuffer;
  thisPod = (Pod *) FirstNode(&gotPods);
  while (IsNode(&gotPods,thisPod)) {
    if (thisPod->pod_Driver) {
      podInterface.pi_Pod = thisPod;
      DBUG(("Build output for pod %d\n", thisPod->pod_Number));
      consumed = cpBuffer->mb_Segment[MB_OUTPUT_SEGMENT].bs_SegmentBitsFilled;
      if ((err = CallPodDriver(thisPod, &podInterface)) < 0) {
	DBUG(("Pod output-build failed: "));
	ERR(err);
      }
      cpBuffer->mb_Segment[MB_OUTPUT_SEGMENT].bs_SegmentBitsFilled =
	cpBuffer->mb_Segment[MB_FLIPBITS_SEGMENT].bs_SegmentBitsFilled =
	  consumed + thisPod->pod_BitsOut; /* Don't assume driverlets do it! */
    }
    flipvert |= thisPod->pod_Flipvert;
    thisPod->pod_Flipvert = FALSE;
    thisPod = (Pod *) NextNode(thisPod);
  }
  return;
}

static void AddPortReader(int32 unit, int32 bufferSize)
{
  PortReader *newReader;
  Item ioReqItem;
  newReader = (PortReader *) AllocMem(sizeof (PortReader) + bufferSize,
				      MEMTYPE_FILL);
  if (!newReader) {
    return;
  }
  ioReqItem = CreateIOReq(NULL, 200,
		unit == 0 ? controlPortDeviceItem : eventPortDeviceItem, 0);
  if (ioReqItem < 0) {
    ERR(ioReqItem);
    FreeMem(newReader, sizeof (PortReader) + bufferSize);
    return;
  }
  newReader->reader_IOReq = (IOReq *) LookupItem(ioReqItem);
  newReader->reader_IOInfo.ioi_Recv.iob_Buffer = newReader->reader_Buffer;
  newReader->reader_IOInfo.ioi_Recv.iob_Len = bufferSize;

  if (unit == 0)
      newReader->reader_IOInfo.ioi_Command = CPORT_CMD_READ;
  else
      newReader->reader_IOInfo.ioi_Command = CPORT_CMD_READEVENT;

  newReader->reader_Unit = unit;
  AddTail(&portReaders[unit], (Node *) newReader);
  readersAllocated[unit] ++;
  DBUG(("Added port reader %d for unit %d\n", readersAllocated[unit], unit));
  DBUG(("Buffer 0x%X, %d bytes\n",
	 newReader->reader_IOInfo.ioi_Recv.iob_Buffer,
	 newReader->reader_IOInfo.ioi_Recv.iob_Len));
  StartPortRead(newReader);
}

static void StartPortRead(PortReader *reader)
{
  Err err;
  DBUG2(("Issuing read, buffer 0x%x, %d bytes\n",
	 reader->reader_IOInfo.ioi_Recv.iob_Buffer,
	 reader->reader_IOInfo.ioi_Recv.iob_Len));
  err = SendIO(reader->reader_IOReq->io.n_Item,
	       &reader->reader_IOInfo);
  DBUG2(("Read initiated\n"));
  if (err < 0) {
    ERR(err);
  } else {
    reader->reader_Busy = TRUE;
    readersCamped[reader->reader_Unit] ++;
  }
}

/*
   In order to avoid fragmentation of kernel memory space when the first
   real event is sent (as occurred in Opera) the Event Broker will pre-
   seed its list of available event messages when it starts up.  The
   heuristic is 3 event messages, each large enough to carry four
   minimum-size event frames (which are large enough for a control pad
   event) and a final terminating null frame.
*/

static void SeedEventMessages(void)
{
  EventMsg *m1, *m2, *m3;
  int32 neededSize;

  neededSize = sizeof (EventBrokerHeader) + 5 * sizeof (EventFrame);

  m1 = GetEventMsg(neededSize);
  m2 = GetEventMsg(neededSize);
  m3 = GetEventMsg(neededSize);

  if (m1) {
    AddHead(&eventMsgsAvail, (Node *) m1);
  }

  if (m2) {
    AddHead(&eventMsgsAvail, (Node *) m2);
  }

  if (m3) {
    AddHead(&eventMsgsAvail, (Node *) m3);
  }

}

static uint32 CreateTableEntry(TimeValVBL **stampPtr, void **structPtr,
			uint32 structSize, uint8 maxGeneric) {
  int32 stampSize, i;
  TimeValVBL *stamp;
  void *structure;
  i = (uint32) maxGeneric + 8;
  stampSize = i * sizeof (TimeValVBL);
  structSize *= i;
  stamp = (TimeValVBL *) AllocMem(stampSize, MEMTYPE_FILL);
  structure = (void *) AllocMem(structSize, MEMTYPE_FILL);
  if (stamp && structure) {
    *stampPtr = stamp;
    *structPtr = structure;
    return i;
  }
  if (stamp) {
    FreeMem((void *) stamp, stampSize);
  }
  if (structure) {
    FreeMem(structure, structSize);
  }
  return 0;
}

static void PrepPodStateTable(PodStateTable *pst)
{
  Pod *pod;
  PodStateTableOverlay *psto;
  uint8 maxGeneric;
  uint32 i;
  psto = (PodStateTableOverlay *) pst;
  for (i = 0; i < 16; i++) {
    if (podStateTableEntrySizes[i] > 0 &&
	psto->psto_Array[i].gt_HowMany == 0) {
      maxGeneric = 0;
      pod = (Pod *) FirstNode(&gotPods);
      while (IsNode(&gotPods,pod)) {
	if (pod->pod_GenericNumber[i] > maxGeneric) {
	  maxGeneric = pod->pod_GenericNumber[i];
	}
	pod = (Pod *) NextNode(pod);
      }
      if (maxGeneric > 0) {
	qprintf(("Prepping event table generic class %d\n", i));
	psto->psto_Array[i].gt_HowMany =
	  CreateTableEntry(&psto->psto_Array[i].gt_ValidityTimeStamps,
			   (void **) &psto->psto_Array[i].gt_EventSpecificData,
			   podStateTableEntrySizes[i], maxGeneric);
      }
    }
  }
}

static Err MakePodStateTable(void)
{
  PodStateTable *pst;
  Err result;

  if (podStateTable) {
    return 0;
  }
  qprintf(("Initializing in-memory pod state table\n"));
  pst = (PodStateTable *) AllocMem(sizeof (PodStateTable), MEMTYPE_FILL);
  if (!pst) {
    return MAKEEB(ER_SEVER,ER_C_STND,ER_NoMem);;
  }
  pst->pst_SemaphoreItem = result = CreateSemaphore("PodStateTable", 0);
  if (pst->pst_SemaphoreItem < 0)
  {
      FreeMem(pst, sizeof(PodStateTable));
      return result;
  }
  PrepPodStateTable(pst);
  podStateTable = pst;
  filterInhibit = FILTER_INHIBIT_TIME; /* force table initialization */
  return 0;
}

static void DestroyPodStateTable(void)
{
  int i;
  PodStateTableOverlay *psto;
  if (podStateTable) {
    qprintf(("Destroying pod-state table\n"));
    psto = (PodStateTableOverlay *) podStateTable;
    for (i = 0 ; i < 16 ; i++ ) {
      if (psto->psto_Array[i].gt_HowMany > 0) {
	qprintf(("Destroying generic %d\n", i));
 	FreeMem(psto->psto_Array[i].gt_ValidityTimeStamps,
		psto->psto_Array[i].gt_HowMany * sizeof (uint32));
	FreeMem(psto->psto_Array[i].gt_EventSpecificData,
		psto->psto_Array[i].gt_HowMany * podStateTableEntrySizes[i]);
      }
    }
    DeleteItem(podStateTable->pst_SemaphoreItem);
    FreeMem(podStateTable, sizeof *podStateTable);
    podStateTable = NULL;
  }
}

static void UpdatePodStateTable(int32 portChanged)
{
  Pod *pod;
  PodInterface podInterface;
  Err err;

  LockSemaphore(podStateTable->pst_SemaphoreItem, TRUE);
  if (portChanged) {
    PrepPodStateTable(podStateTable);
  }
  podInterface = iTemplate;
  podInterface.pi_Command = PD_UpdatePodStateTable;
  podInterface.pi_VBL = timestamp;
  podInterface.pi_PodStateTable = podStateTable;
  pod = (Pod *) FirstNode(&gotPods);
  while (IsNode(&gotPods,pod)) {
    podInterface.pi_Pod = pod;
    if ((err = CallPodDriver(pod, &podInterface)) < 0) {
      DBUG(("Pod table update failed: "));
      ERR(err);
    }
    pod = (Pod *) NextNode(pod);
  }
  UnlockSemaphore(podStateTable->pst_SemaphoreItem);
}
