/* @(#) audio_folio.c 96/09/11 1.183 */
/* $Id: audio_folio.c,v 1.133 1995/03/18 02:20:41 peabody Exp phil $ */
/****************************************************************
**
** Audio Folio
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/

/*
** 921207 PLB Add TraceAudio.
** 921208 PLB Add TRACE. Changed name to "audio"
** 930126 PLB Change to use .dsp files instead of .ofx
**            Use relative pathname if none specified.  (Not /ad)
** 930127 PLB Proper error returns for allocating functions.
** 930208 PLB Add support for AudioTimer and AudioCue
** 930311 PLB Add UnloadInstrument that unloads template
** 930315 PLB Extensive API changes, added Envelope & Attachment
** 930415 PLB Track connections between Templates/Instruments/Samples/Attachments/Knobs
** 930519 PLB Print OperaVersion.
** 930521 PLB Allow arbitrary DAC Control words to be passed with a -c option.
** 930524 PLB Add LoadSampleHere
** 930602 PLB Began Envelopes
** 930612 PLB Envelopes Working, tuning too
** 930713 PLB Add copyright. Add romhack.
** 930728 PLB Do not play note when no sample in range.
** 930809 PLB Envelopes have RELEASEJUMP
** 940203 GLL Revise Folio Greeting to use "print_vinfo"
** 940309 PLB Add semaphore to prevent race condition on startup.
** 940314 PLB Lock semaphore in InitAudioFolio() to completely prevent
**            race condition.
** 940407 PLB Remove BETATEST and some other messages.
** 940411 WJB Patched to prevent dereferencing NULL pointer in swiSetAudioItemInfo()
**            and GetAudioItemInfo() when fed a bad item.
** 940427 PLB Added EnableAudioInput().
** 940506 PLB Added SetAudioFolioInfo(), gKernelAmpKnob
** 940608 PLB SetAudioFolioInfo() caller must be privileged. Adjust ticks available.
** 940617 PLB Add support for SetItemOwner()
** 940622 WJB Added AUDIO_TUNING_NODE to GetAudioItemInfo() so that it returns
**            the correct error code.
** 940802 WJB Made demand loadable (but not unloadable).
** 940809 WJB Removed redundant PrintError() calls.
** 940831 PLB Some magic requested by Martin Taillefer for demand loading support.
** 940901 WJB Fixed typo in DEMANDLOAD_MAIN_CREATE comparison.
** 940907 WJB Changed audio folio's thread name to just 'audio'.
** 940920 WJB Implemented GetAudioFolioInfo().
** 940921 PLB Use |NODE_SIZELOCKED in node database.
** 941010 WJB Tweaked SetAudioFolioInfo() autodocs.
** 941011 WJB Replaced FindAudioDevice(), ControlAudioDevice(), and GetNumInstruments() with AFReserved().
** 941128 PLB Restore valid sample values if SetInfo fails.
** 941209 PLB Added support for DuckAndCover, removed some dead code.
** 950125 WJB Removed local prototypes for SuperMemAlloc/Free().
** 950131 WJB Cleaned up includes.
** 950222 PLB Merged in Early Access changes, mostly reversed function tables
** 950222 WJB Realigned spacing in AudioFolioTags - someone used 8-space tabs in it.
** 950222 WJB Added dspp_touch.h.
** 950307 WJB Fixed to compile properly for CPU_ARM and BE_COMPATIBLE.
** 950308 WJB Added AF_TAG_DMA_GRANULARITY to GetAudioFolioInfo().
** 950308 WJB Now calling dsppAdjustAvailTicks() (dspp_resources) instead of DSPPAdjustTicksPerFrame() (dspp_instr).
** 950317 WJB Added call to dsppGrabSystemReservedResources().
** 950411 WJB Now prints mode description string right after print_vinfo(). Under control of DEBUG_PrintMode.
** 950424 WJB Publicized gKernelInstrumentItem.
**            Now calling dsphInitInstrumentation().
** 950426 WJB Split system instrument loading code from StartAudioFolio() out to LoadSystemInstruments().
** 950501 WJB Added AF_BDA_PASS to startup message.
** 950512 WJB Added call to Init/TermAudioTrigger().
** 950515 WJB Added MonitorTrigger() SWI table entry.
** 950525 WJB Renamed swiMonitorTrigger() to swiArmTrigger().
** 950711 WJB Removed AF_TAG_DMA_GRANULARITY. Not needed with new compatibility strategy.
** 960517 WJB TermAudioFolio() no longer returns an error code, prevents audio folio from crashing if it fails to start (CR 5879).
** 960528 WJB Moved system clock creation to CreateSystemClocks().
** 960529 WJB Audio Thread no longer opens audio folio.
** 960529 WJB No longer open/close audio folio item. Depend on module open/close only.
** 960530 WJB Enabled DEMANDLOAD_MAIN_DELETE. The audio folio now demand unloads (poof!).
** 960531 WJB Audio daemon is now supervisor mode only. No longer needs the 16K user stack.
** 960531 WJB Moved HandleAudioSignalTask() here from audio_timer.c
** 960603 WJB Removed support for af_ReadySemaphore.
** 960603 WJB Now uses OwnDSPP().
** 960606 WJB Major reorganization.
** 960805 PLB Removed TraceAudio() CR3562
** 960911 WJB EnableAudioInput() now returns non-zero when audio input hardware is present.
*/

#include <dspptouch/dspp_touch.h>   /* OwnDSPP(), dsphTraceSimulator() */
#include <kernel/cache.h>           /* GetCacheInfo() */
#include <kernel/kernel.h>
#include <kernel/semaphore.h>
#include <kernel/sysinfo.h>         /* enable audio input */
#include <kernel/tags.h>            /* tag iteration */
#include <loader/loader3do.h>       /* FindCurrentModule() */

#include "audio_internal.h"
#include "dspp.h"                   /* DSPPData */
#include "dspp_resources.h"         /* dsppAdjustAvailTicks() */


/* -------------------- Debug */

#define DEBUG_PrintMode     0       /* Enabled printing audio folio mode during startup. */
#define DEBUG_DLL           0       /* Debug audio folio load/unload */
#define DEBUG_Daemon        0       /* Debug audio daemon's signal handler loop */
#define DEBUG_TaskData      0       /* Debug f_FolioCreate/DeleteTask vectors. */
#define DEBUG_AudioInput    0       /* debug EnableAudioInput() */

#define DBUG(x)     /* PRT(x) */
#define BREAKPOINT  Debug()

#if DEBUG_DLL
#include <stdio.h>
#define DBUG_DLL(x)     { PRTDBUGINFO(); PRT(x); }
#else
#define DBUG_DLL(x)
#endif

#if DEBUG_Daemon
#include <stdio.h>
#define DBUG_DAEMON(x)  { PRTDBUGINFO(); PRT(x); }
#else
#define DBUG_DAEMON(x)
#endif

#if DEBUG_TaskData
#include <stdio.h>
/* CURRENTTASKITEM is undefined in DeleteAudioTaskHook(); don't dereference it. */
#define DBUG_TASKDATA(x) PRT(x)
#else
#define DBUG_TASKDATA(x)
#endif

#if DEBUG_AudioInput
#define DBUGAUDIN(x) { PRTDBUGINFO(); PRT(x); }
#else
#define DBUGAUDIN(x)
#endif


/* -------------------- Globals */

	/* Unique semaphore returned by OwnDSPP() */
static Item dsppLock;

	/* In-place Folio structure for use with CREATEFOLIO_TAG_BASE */
AudioFolio AudioBase;


/* -------------------- Local Functions */

	/* audio daemon */
static Err LaunchAudioDaemon (const DSPPTemplate *nanokernelUserDTmp);

	/* folio callbacks */
static int32 InitAudioFolio (AudioFolio *);
static int32 TermAudioFolio (AudioFolio *);
static int32 CreateAudioTaskHook (Task *, TagArg *);
static void DeleteAudioTaskHook (Task *);

	/* Audio Input */
static void DecrementGlobalAudioInputEnableCount (void);

	/* local SWIs */
static Err swiSetAudioFolioInfo (const TagArg *);
static int32 swiEnableAudioInput (int32 OnOrOff, const TagArg *);
static void swiNotImplemented (void);       /* !!! remove me */


/* -------------------- Folio Definition */

	/* audio daemon */
#define AUDIODAEMON_NAME    AUDIOFOLIONAME
#define AUDIODAEMON_PRI     205
#define AUDIODAEMON_STACK   2048

	/* nanokernel instrument name */
#define NANOKERNEL_NAME     "nanokernel.dsp"
#define NANOKERNEL_PRI      255

	/* System calls */
	/* !!! remove holes */
static const void *(*AudioSWIFuncs[])() =
{
	(void *(*)())swiSetKnobPart,            /* 0 */
	(void *(*)())swiStartInstrument,        /* 1 */
	(void *(*)())swiReleaseInstrument,      /* 2 */
	(void *(*)())swiStopInstrument,         /* 3 */
	(void *(*)())swiTuneInsTemplate,        /* 4 */
	(void *(*)())swiTuneInstrument,         /* 5 */
	(void *(*)())swiGetDynamicLinkResourceUsage, /* 6, private */
	(void *(*)())swiNotImplemented, /* WAS: swiTestHack,               7 */
	(void *(*)())swiConnectInstrumentsHack, /* 8, private (!!! hack to workaround 4-arg SWI limit) */
	(void *(*)())swiNotImplemented, /* WAS: swiTraceAudio,    9 */
	(void *(*)())swiReadKnobPart,           /* 10 */
	(void *(*)())swiNotImplemented,         /* 11 */
	(void *(*)())swiDisconnectInstrumentParts, /* 12 */
	(void *(*)())swiSignalAtAudioTime,      /* 13 */
	(void *(*)())swiNotImplemented, /* WAS: swiRunAudioSignalTask,    14, private */
	(void *(*)())swiSetAudioClockRate,      /* 15 */
	(void *(*)())swiSetAudioClockDuration,  /* 16 */
	(void *(*)())swiNotImplemented,         /* 17 */
	(void *(*)())swiStartAttachment,        /* 18 */
	(void *(*)())swiReleaseAttachment,      /* 19 */
	(void *(*)())swiStopAttachment,         /* 20 */
	(void *(*)())swiLinkAttachments,        /* 21 */
	(void *(*)())swiMonitorAttachment,      /* 22 */
	(void *(*)())swiNotImplemented, /* WAS: swiSetMasterTuning,       23 */
	(void *(*)())swiAbandonInstrument,      /* 24 */
	(void *(*)())swiAdoptInstrument,        /* 25 */
	(void *(*)())swiScavengeInstrument,     /* 26 */
	(void *(*)())swiSetAudioItemInfo,       /* 27 */
	(void *(*)())swiPauseInstrument,        /* 28 */
	(void *(*)())swiResumeInstrument,       /* 29 */
	(void *(*)())swiWhereAttachment,        /* 30 */
	(void *(*)())swiNotImplemented, /* WAS: swiIncrementGlobalIndex,  31 */
	(void *(*)())swiBendInstrumentPitch,    /* 32 */
	(void *(*)())swiAbortTimerCue,          /* 33 */
	(void *(*)())swiEnableAudioInput,       /* 34 */
	(void *(*)())swiSetAudioFolioInfo,      /* 35 */
	(void *(*)())swiReadProbePart,          /* 36 */
	(void *(*)())swiNotImplemented,         /* 37 */
	(void *(*)())dsphGetAudioFrameCount,    /* 38 */
	(void *(*)())swiNotImplemented, /* WAS: dsphGetAudioCyclesUsed,   39 */
	(void *(*)())swiGetAudioTime,           /* 40 */
	(void *(*)())swiArmTrigger,             /* 41 */
	(void *(*)())swiReadAudioClock,         /* 42 */
};
#define NUM_AUDIOSWIFUNCS (sizeof(AudioSWIFuncs)/sizeof(void *))

	/* Item Types match the order of the entries in this database. */
	/* Note: NODE_NAMEVALID is implied by NODE_ITEMVALID, so setting it in the table below has no effect. */
static const NodeData AudioNodeData[] = {
	{ 0, 0 },
	{ sizeof(AudioInsTemplate), NODE_NAMEVALID | NODE_ITEMVALID | NODE_SIZELOCKED | NODE_OPENVALID },
	{ sizeof(AudioInstrument),  NODE_NAMEVALID | NODE_ITEMVALID | NODE_SIZELOCKED | NODE_OPENVALID },
	{ sizeof(AudioKnob),        NODE_NAMEVALID | NODE_ITEMVALID | NODE_SIZELOCKED },
	{ sizeof(AudioSample),      NODE_NAMEVALID | NODE_ITEMVALID | NODE_SIZELOCKED },
	{ sizeof(AudioCue),         NODE_NAMEVALID | NODE_ITEMVALID | NODE_SIZELOCKED },
	{ sizeof(AudioEnvelope),    NODE_NAMEVALID | NODE_ITEMVALID | NODE_SIZELOCKED },
	{ sizeof(AudioAttachment),  NODE_NAMEVALID | NODE_ITEMVALID | NODE_SIZELOCKED },
	{ sizeof(AudioTuning),      NODE_NAMEVALID | NODE_ITEMVALID | NODE_SIZELOCKED },
	{ sizeof(AudioProbe),       NODE_NAMEVALID | NODE_ITEMVALID | NODE_SIZELOCKED },
	{ sizeof(AudioClock),       NODE_NAMEVALID | NODE_ITEMVALID | NODE_SIZELOCKED }
};
#define AUDIONODECOUNT (sizeof(AudioNodeData)/sizeof(NodeData))

	/* CreateItem() Tags used when creating the Folio */
static const TagArg AudioFolioTags[] = {
	{ TAG_ITEM_NAME,                (TagData) AUDIOFOLIONAME },         /* name of audio folio */
	{ CREATEFOLIO_TAG_ITEM,         (TagData) AUDIONODE },              /* audio folio's magic item number */
	{ CREATEFOLIO_TAG_BASE,         (TagData) &AudioBase },             /* use statically allocated AudioBase */
	{ CREATEFOLIO_TAG_DATASIZE,     (TagData) sizeof (AudioFolio) },    /* size of audio folio */
	{ CREATEFOLIO_TAG_NSWIS,        (TagData) NUM_AUDIOSWIFUNCS },      /* number of SWI functions */
	{ CREATEFOLIO_TAG_SWIS,         (TagData) AudioSWIFuncs },          /* list of swi functions */
	{ CREATEFOLIO_TAG_INIT,         (TagData) InitAudioFolio },         /* initialization code */
	{ CREATEFOLIO_TAG_DELETEF,      (TagData) TermAudioFolio },         /* deletion code */
	{ CREATEFOLIO_TAG_NODEDATABASE, (TagData) AudioNodeData },          /* Audio node database */
	{ CREATEFOLIO_TAG_MAXNODETYPE,  (TagData) AUDIONODECOUNT },         /* number of nodes */
	{ CREATEFOLIO_TAG_TASKDATA,     (TagData) sizeof (AudioFolioTaskData) },
	TAG_END
};


/* -------------------- Open/Close docs */

/**
|||	AUTODOC -public -class audio -group Miscellaneous -name OpenAudioFolio
|||	Opens the audio folio.
|||
|||	  Synopsis
|||
|||	    Err OpenAudioFolio (void)
|||
|||	  Description
|||
|||	    This procedure connects a task to the audio folio and must be executed by
|||	    each task before it makes any audio folio calls. If a task makes an audio
|||	    folio call before it executes OpenAudioFolio(), the task fails.
|||
|||	    Call CloseAudioFolio() to relinquish a task's access to the audio folio.
|||
|||	  Return Value
|||
|||	    The procedure returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Library call implemented in libc.a V29.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>, libc.a
|||
|||	  See Also
|||
|||	    CloseAudioFolio()
**/

/**
|||	AUTODOC -public -class audio -group Miscellaneous -name CloseAudioFolio
|||	Closes the audio folio.
|||
|||	  Synopsis
|||
|||	    Err CloseAudioFolio (void)
|||
|||	  Description
|||
|||	    This procedure closes a task's connection with the audio folio. Call
|||	    it when your application finishes using audio calls.
|||
|||	  Return Value
|||
|||	    The procedure returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Library call implemented in libc.a V29.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>, libc.a
|||
|||	  See Also
|||
|||	    OpenAudioFolio()
**/


/* -------------------- Notes */

/**********************************************************************************************************************************
**
**  Twisted maze of create/delete audio daemon callbacks (132 column text)
**
**  task calls: OpenAudioFolio() / ImportByName("audio")
**
**      'Loader Daemon'                                         'audio'
**      priveleged thread of operator, pri=200                  supervisor audio daemon thread of operator, pri=205
**      ----------------------------------------------------    ---------------------------------------------------
**
**      if audio module not yet loaded:
**
**      main (DEMANDLOAD_MAIN_CREATE, task, module)
**          CreateAudioModule()
**              dsppLoadInsTemplate()
**              LaunchAudioDaemon()
**                  alloc ready signal
**                  CreateItem() audio daemon thread
**                  WaitSignal (ready signal | SIGF_DEADTASK)
**
**                                                              AudioDaemonMain()
**                                                                  InitAudioDaemon()
**                                                                      OwnDSPP()
**                                                                      CreateItem() audio folio
**                                                                          InitAudioFolio() (callback from kernel)
**                                                                              dspp_Init()
**                                                                              init interrupt handlers
**                                                                          CreateAudioTaskHook() (callback from kernel) for all tasks in system
**                                                                      create system clocks:
**                                                                          CreateAudioClock() of af_GlobalClock
**                                                                          CreateAudioClock() of af_EnvelopeClock
**                                                                          SetAudioClockDuration of af_EnvelopeClock
**                                                                      create and start nanokernel (user mode DSPPTemplate):
**                                                                          CreateItem() of nanokernel.dsp Template Item
**                                                                              using user-mode DSPPTemplate handed to this
**                                                                              task
**                                                                          CreateItem() of nanokernel.dsp Instrument
**                                                                          CreateItem() of nanokernel.dsp's Amplitude knob
**                                                                          StartInstrument() nanokernel.dsp
**                                                                          dsphInitInstrumentation() (depends on nanokernel)
**                                                                      dsppRegisterDuckAndCover()
**                                                                      SendSignal (t->t.n_Owner, ReadySignal)
**                                                                  RunAudioDaemon()
**                                                                      enter signal handling loop, terminate on SIGF_ABORT
**
**                  (WaitSignal() returns)
**                  check result from audio daemon
**                  free ready signal
**              dsppDeleteUserTemplate()
**              ScavengeMem()
**          return
**
**  task calls: CloseAudioFolio() / UnimportByName("audio")
**
**      'Loader Daemon'                                         'audio'
**      priveleged thread of operator, pri=200                  supervisor audio daemon thread of operator, pri=205
**      ----------------------------------------------------    ----------------------------------------------------
**
**      if usage count is now 0:
**
**      main (DEMANDLOAD_MAIN_DELETE, task, module)
**          DeleteAudioModule()
**              DeleteItem() audio daemon thread
**                  sends SIGF_ABORT, and then waits
**                  for audio daemon to exit
**
**                                                              (in AudioDaemonMain())
**                                                                  (in RunAudioDaemon())
**                                                                      receive SIGF_ABORT
**                                                                      return
**                                                                  TermAudioDaemon()
**                                                                      dsppTermDuckAndCover()
**                                                                      DeleteItem() nanokernel.dsp template
**                                                                      DeleteItem() global clock
**                                                                      DeleteItem() envelope clock
**                                                                      DeleteItem() audio folio
**                                                                          DeleteAudioTaskHook() (callback from kernel) for all tasks in system
**                                                                          internalDeleteAudioItem() (callback from kernel) for any still existing audio items
**                                                                          TermAudioFolio() (callback from kernel)
**                                                                              term interrupt handlers
**                                                                              dspp_Term()
**                                                                  return
**
**              (DeleteItem() returns)
**          return
**
*********************************************************************************************************************************/


/* -------------------- Loader Interface */

static Err CreateAudioModule (void);
static Err DeleteAudioModule (void);

/*
	main - loader entry point called by the Loader Daemon (thread of the Operator).

	Arguments
		op
			One of the DEMANDLOAD_MAIN_ op codes.

		task
			Corresponds to the Task parameter given to the
			create/delete/open/close module item routines.

		module
			The item of the current module.

	Results
		Non-negative value on success. No meaning is derived from this result
		other than than that the call succeeded.

		Negative error code on failure.
*/

int32 main (int32 op, Item task, Item module)
{
	TOUCH(task);
	TOUCH(module);

#if DEBUG_DLL
	if (op < 0) {
		const char *opDesc;

		switch (op) {
			case DEMANDLOAD_MAIN_CREATE: opDesc = "create"; break;
			case DEMANDLOAD_MAIN_DELETE: opDesc = "delete"; break;
			default                    : opDesc = "?";      break;
		}

		DBUG_DLL((
			"main: %s (op=%d) module=0x%x opencount=%d\n"
			"    on behalf of: task=0x%x '%s' pri=%d %s\n",
			opDesc, op, module, ((Module *)LookupItem(module))->n.n_OpenCount,
			task, TASK(task)->t.n_Name, TASK(task)->t.n_Priority, IsPriv(TASK(task)) ? "priv" : "user"));
	}
#endif

	switch (op) {
		case DEMANDLOAD_MAIN_CREATE: return CreateAudioModule();
		case DEMANDLOAD_MAIN_DELETE: return DeleteAudioModule();
		default                    : return 0;
	}
}

static Err CreateAudioModule (void)
{
	DSPPTemplate *nanokernelUserDTmp = NULL;
	Err errcode;

	DBUG_DLL(("CreateAudioModule\n"));

#if DEBUG_PrintMode
	{
		#ifdef SIMULATE_DSPP
			#define AF_DSPP_DESC "Simulation"
		#else
			#define AF_DSPP_DESC "Hardware"
		#endif
		#ifdef DSPP_DISASM
			#define AF_DISASM_DESC " + DSPP Disassembler"
		#else
			#define AF_DISASM_DESC ""
		#endif

		PRT(("AudioFolio: DSP=" AF_DSPP_DESC));
		PRT((AF_DISASM_DESC "\n"));

		#undef AF_DSPP_DESC
		#undef AF_DISASM_DESC
	}
#endif

#ifdef PARANOID
	PRT(("AudioFolio: PARANOID error checking turned on!\n"));
#endif

		/* load nanokernel.dsp */
	DBUG_DLL(("Load nanokernel.dsp\n"));
	if ((errcode = dsppLoadInsTemplate (&nanokernelUserDTmp, NANOKERNEL_NAME)) < 0) {
		ERR(("CreateAudioModule: Error loading instrument '%s'\n", NANOKERNEL_NAME));
		goto clean;
	}

		/* launch daemon */
	errcode = LaunchAudioDaemon (nanokernelUserDTmp);

clean:
	dsppDeleteUserTemplate (nanokernelUserDTmp);

		/* Return any free pages from this task family to system. The task
		** family includes both this loader daemon and the audio daemon as they
		** are both threads of the Operator. */
	ScavengeMem();

	return errcode;
}

static Err DeleteAudioModule (void)
{
	Err errcode;

	DBUG_DLL(("DeleteAudioModule\n"));

		/* Delete the audio daemon. Since the audio daemon is in supervisor mode
		** this simply sends SIGF_ABORT and blocks until the audio daemon exits.
		** No further task synchronization is necessary. */
	errcode = DeleteItem (AB_FIELD(af_AudioDaemonItem));
	TOUCH(errcode);         /* errcode is just for DEBUG_DLL. Silence a warning when DEBUG_DLL is 0. */

	DBUG_DLL(("delete audio daemon returned 0x%x\n", errcode));

	return 0;
}


/* -------------------- Audio Daemon */

typedef struct AFStartupData {
	const DSPPTemplate *afsd_NanokernelUserDTmp;    /* nanokernel.dsp user-mode DSPPTemplate */
	int32        afsd_ReadySignal;                  /* signal to send when ready */
	volatile Err afsd_Result;                       /* success: >=0; failure: an error code */
} AFStartupData;

static void AudioDaemonMain (AFStartupData *);
static Err InitAudioDaemon (AFStartupData *);
static void RunAudioDaemon (void);
static void TermAudioDaemon (void);
static Err CreateSystemClocks (void);
static Err CreateSystemInstruments (const DSPPTemplate *nanokernelUserDTmp);


/************************************************************************/
/*
	Launch audio folio daemon and wait for it to complete initialization (or
	fail).

	Arguments
		nanokernelUserDTmp
			User mode DSPPTemplate for nanokernel.dsp to be promoted to become
			the real nanokernel.dsp Template Item by the audio daemon.

	Result
		>=0 on success, negative errcode on failure.
*/
static Err LaunchAudioDaemon (const DSPPTemplate *nanokernelUserDTmp)
{
	AFStartupData startupData;
	int32 sigs;
	Err errcode;

	DBUG_DLL(("Launch audio daemon\n"));

		/* set up startup data (cannot permit failure before here) */
	memset (&startupData, 0, sizeof startupData);
	startupData.afsd_NanokernelUserDTmp = nanokernelUserDTmp;

		/* alloc reply signal */
	if ((errcode = startupData.afsd_ReadySignal = AllocSignal (0)) <= 0) {
		if (!errcode) errcode = AF_ERR_NOSIGNAL;
		goto clean;
	}

		/* clear task died signal */
	ClearCurrentSignals (SIGF_DEADTASK);

		/* create audio daemon thread */
	if ( (errcode = CreateItemVA (MKNODEID(KERNELNODE,TASKNODE),
		TAG_ITEM_NAME,                  AUDIODAEMON_NAME,
		TAG_ITEM_PRI,                   AUDIODAEMON_PRI,
		CREATETASK_TAG_PC,              AudioDaemonMain,
		CREATETASK_TAG_THREAD,          0,              /* Occupy operator's memory space */
		CREATETASK_TAG_SUPERVISOR_MODE, 0,              /* Thread runs entirely in supervisor mode. Also implies CREATETASK_TAG_PRIVILEGED, CREATETASK_TAG_SINGLE_STACK */
		CREATETASK_TAG_STACKSIZE,       AUDIODAEMON_STACK,
		CREATETASK_TAG_ARGC,            &startupData,   /* Pass startupData as first arg to AudioDaemonMain(). */
		TAG_END) ) < 0 ) goto clean;

		/* wait for notification of success signal (afsd_ReplySignal) or
		** failure signal (SIGF_DEADTASK) from AudioDaemon */
	sigs = WaitSignal (startupData.afsd_ReadySignal | SIGF_DEADTASK);
	errcode = startupData.afsd_Result;

	DBUG_DLL(("audio daemon sent signal=0x%x and set result=0x%x.\n", sigs, errcode));
	TOUCH(sigs);

clean:
	if (startupData.afsd_ReadySignal > 0) FreeSignal (startupData.afsd_ReadySignal);
	return errcode;
}


/************************************************************************/
/*
	Audio daemon entry point.

	Supervisor mode thread of the operator which creates the audio folio and
	various other items, then services signals sent by the various interrupt
	handlers. Shuts down when it receives SIGF_ABORT.

	If initialization fails, this task is allowed to die when it falls off the
	end, which sends SIGF_DEADTASK to the creator.

	Arguments
		startupData
			AFStartupData sent from creating thread, passed to InitAudioDaemon().

	Notes
		startupData becomes invalid after this task replies to creator.
*/
static void AudioDaemonMain (AFStartupData *startupData)
{
	DBUG_DLL(("AudioDaemonMain: creator=0x%x readysig=0x%x\n", CURRENTTASK->t.n_Owner, startupData->afsd_ReadySignal));

		/* Note: StartupData becomes invalid when InitAudioDaemon() returns.
		** This is OK, because InitAudioDaemon() is the only part of this
		** process which can actually fail. */
	if (InitAudioDaemon (startupData) >= 0) {
		RunAudioDaemon();
	}
	TermAudioDaemon();

	/*
		Fall off end. The audio module must not be unloaded until after this
		function returns, so here's how we synchronize:

		If initialization was successful, we got here after RunAudioDaemon()
		received the SIGF_ABORT sent by DeleteAudioModule()'s attempt to
		DeleteItem() the audio daemon. DeleteItem() shouldn't return until the
		task is actually deleted (i.e., after this function returns).

		If initialization failed, the creator is waiting for SIGF_ABORT,
		which is sent to it when the Task item is finally deleted.
	*/
	DBUG_DLL(("AudioDaemonMain: exiting\n"));
}

/************************************************************************/
/*
	Initialize stuff for audio daemon.

	If initialization succeeds, writes 0 to startupData->afsd_Result and sends
	startupData->afsd_ReadySignal to owner.

	If initialization fails, writes error code to startupData->afsd_Result and
	returns.

	Arguments
		startupData->afsd_ReadySignal
			Task and signal to send to CURRENTASK->t.n_Owner if initialization
			succeeds.

		startupData->afsd_NanokernelUserDTmp
			DSPPTemplate to promote to supervisor mode.

	Results
		Sets startupData->afsd_Result with error code from startup.

	Note
		Expects caller to clean up by calling TermAudioDaemon().
*/
static Err InitAudioDaemon (AFStartupData *startupData)
{
	Err errcode;

		/* Try to gain exclusive access to DSPP. If this fails, its probably
		** because something else already has exclusive access to DSPP. */
	DBUG_DLL(("InitAudioDaemon: try to own DSPP.\n"));
	if ((errcode = dsppLock = OwnDSPP (AF_ERR_DSP_BUSY)) < 0) goto clean;

		/* Create the Audio folio item. */
	DBUG_DLL(("InitAudioDaemon: try to create folio item.\n"));
	if ((errcode = CreateItem(MKNODEID(KERNELNODE,FOLIONODE),AudioFolioTags)) < 0) goto clean;

		/* Create system clocks */
	DBUG_DLL(("InitAudioDaemon: create system clocks.\n"));
	if ((errcode = CreateSystemClocks()) < 0) goto clean;

		/* Create System Instruments. When done with this all system DSP
		** resources have been accounted for. */
	DBUG_DLL(("InitAudioDaemon: create and start system instruments.\n"));
	if ((errcode = CreateSystemInstruments (startupData->afsd_NanokernelUserDTmp)) < 0) goto clean;

		/* Install DuckAndCover for DIPIR. */
		/* !!! maybe don't fail if this can't be done? */
	DBUG_DLL(("InitAudioDaemon: duck and cover\n"));
	if ((errcode = dsppInitDuckAndCover()) < 0) goto clean;

		/* success: clear result, send ready signal to creator */
	DBUG_DLL(("InitAudioDaemon: success\n"));
	startupData->afsd_Result = 0;
	SendSignal (CURRENTTASK->t.n_Owner, startupData->afsd_ReadySignal);
	return 0;

clean:
		/* failure: set error code, return, allow clean up to occur and task
		** to die, which sends SIGF_DEADTASK to creator */
	DBUG_DLL(("InitAudioDaemon: failed errcode=0x%x\n", errcode));
	startupData->afsd_Result = errcode;
	return errcode;
}

/************************************************************************/
/*
	Create system clocks:
		. 240Hz AF_GLOBAL_CLOCK
		. Envelope clock

	Results
		Returns 0 on success or Err code on failure.
		Sets AudioBase fields:
			af_GlobalClock
			af_EnvelopeClock

	Notes
		Expects caller to clean up.
*/
static Err CreateSystemClocks (void)
{
	Item globalClock;
	Item envClock;
	Err errcode;

		/* Create system clock. */
	if ((errcode = globalClock = CreateAudioClock (NULL)) < 0) {
		ERR(("CreateSystemClocks: Error creating custom clock = 0x%x\n", errcode));
		return errcode;
	}
	AB_FIELD(af_GlobalClock) = (AudioClock *)LookupItem (globalClock);

		/* Create envelope clock. */
	if ((errcode = envClock = CreateAudioClock (NULL)) < 0) {
		ERR(("CreateSystemClocks: Error creating envelope clock = 0x%x\n", errcode));
		return errcode;
	}
	AB_FIELD(af_EnvelopeClock) = (AudioClock *)LookupItem (envClock);

		/* Set to envelope clock to highest possible resolution. */
	if ((errcode = SetAudioClockDuration (envClock, 1)) < 0) {
		ERR(("CreateSystemClocks: Error setting envelope clock duration = 0x%x\n", errcode));
		return errcode;
	}

		/* success */
	return 0;
}

/************************************************************************/
/*
	Create and start system instruments.
		. Create nanokernel template and instrument.
		. Create master amplitude knob.
		. Start nanokernel.
		. Initialize stuff that depends on the system instruments being
		  loaded (e.g. GetAudioFrameCount())

	Arguments
		nanokernelUserDTmp
			User mode DSPPTemplate passed in from creator to be blessed for
			nanokernel's Template Item.

	Results
		Returns 0 on success or Err code on failure.
		Sets AudioBase fields:
			af_NanokernelInsTemplate
			af_NanokernelInstrument
			af_NanokernelAmpKnob

	Notes
		. Expects caller to clean up.
		. All DSPP resources remaining after this function returns are available
		  for application use.
*/
static Err CreateSystemInstruments (const DSPPTemplate *nanokernelUserDTmp)
{
	Err errcode;

		/* Create nanokernel instrument template */
		/* @@@ This must _not_ require loading any subroutines. The loader daemon will
		**     probably fail when it opens the Audio Module because the audio module
		**     isn't yet ready. */
	if ((errcode = AB_FIELD(af_NanokernelInsTemplate) =
		CreateItemVA (MKNODEID(AUDIONODE,AUDIO_TEMPLATE_NODE),
			TAG_ITEM_NAME,      NANOKERNEL_NAME,
			AF_TAG_TEMPLATE,    nanokernelUserDTmp,
			TAG_END)
		) < 0)
	{
		ERR(("CreateSystemInstruments: Error creating instrument template '%s'\n", NANOKERNEL_NAME));
		return errcode;
	}

		/* Create nanokernel instrument */
	if ((errcode = AB_FIELD(af_NanokernelInstrument) = AllocInstrumentSpecial (AB_FIELD(af_NanokernelInsTemplate), NANOKERNEL_PRI, AF_SPECIAL_KERNEL)) < 0) {
		ERR(("CreateSystemInstruments: Error creating instrument '%s'\n", NANOKERNEL_NAME));
		return errcode;
	}

		/* Get nanokernel amplitude knob for master volume control. */
	if ((errcode = AB_FIELD(af_NanokernelAmpKnob) =
		CreateItemVA (MKNODEID(AUDIONODE,AUDIO_KNOB_NODE),
			AF_TAG_INSTRUMENT, AB_FIELD(af_NanokernelInstrument),
			AF_TAG_NAME,       "Amplitude",
			TAG_END)
		) < 0)
	{
		ERR(("CreateSystemInstruments: Error creating "NANOKERNEL_NAME"'s 'Amplitude' knob\n"));
		return errcode;
	}

		/* Start nanokernel (starts DSPP) */
	if ((errcode = StartInstrument (AB_FIELD(af_NanokernelInstrument), NULL)) < 0) return errcode;

		/* Initialize instrumentation functions (e.g., GetAudioFrameCount())
		** (depends on system instruments) */
	if ((errcode = dsphInitInstrumentation()) < 0) return errcode;

		/* Success */
	return 0;
}

/************************************************************************/
/*
	Main body of audio daemon.

	Receive signals from various interrupts:
		. timer
		. dma
		. trigger
	and manage list of pending timer requests.

	Returns when SIGF_ABORT received.
*/
static void RunAudioDaemon (void)
{
	/* @@@ Note: Don't declare any floats in this routine so that we may invalidate the FP state. */
	const int32 waitSignals = AB_FIELD(af_TimerSignal) |
							  AB_FIELD(af_TriggerSignal) |
							  AB_FIELD(af_DMASignal);
	int32 recvSignals;

	DBUG_DAEMON(("RunAudioDaemon\n"));

	for (;;) {
			/* We have no floats that we care about. */
		InvalidateFPState();

			/* wait for signals */
		recvSignals = WaitSignal (waitSignals);
		DBUG_DAEMON(("RunAudioDaemon: received signals 0x%x\n", recvSignals));

			/* shutdown signal from DeleteAudioModule()? */
		if (recvSignals & SIGF_ABORT) {
			DBUG_DAEMON(("RunAudioDaemon: received SIGF_ABORT\n"));
			return;
		}

			/* normal signals */
		if (recvSignals & AB_FIELD(af_DMASignal))     HandleDMASignal();
		if (recvSignals & AB_FIELD(af_TimerSignal))   HandleTimerSignal();
		if (recvSignals & AB_FIELD(af_TriggerSignal)) HandleTriggerSignal();
	}
}

/************************************************************************/
/*
	Audio daemon clean up. Undoes everything done by InitAudioDaemon().

	Notes
		. Must be called prior to exiting regardless of how successful
		  InitAudioDaemon() was.
		. Doesn't tolerate being called twice.
*/
static void TermAudioDaemon (void)
{
		/* unduck and cover */
	DBUG_DLL(("TermAudioDaemon: unregister duck and cover\n"));
	dsppTermDuckAndCover();

		/* delete folio and related items, but only if we created the folio successfully */
	if (AB_FIELD(af_Folio).fn.n_Owner == CURRENTTASKITEM) {

			/* Stop and delete system instruments. Simply deleting the template like this
			** automatically causes the following to occur in this order:
			**  . delete nanokernel knobs
			**  . stop nanokernel instrument (halts DSPP)
			**  . delete nanokernel instrument
			**  . delete nanokernel template
			*/
		DBUG_DLL(("TermAudioDaemon: delete system instruments\n"));
		DeleteItem (AB_FIELD(af_NanokernelInsTemplate));

			/* delete system clocks */
		DBUG_DLL(("TermAudioDaemon: delete system clocks\n"));
		if (AB_FIELD(af_GlobalClock)) {
			DeleteItem (AB_FIELD(af_GlobalClock)->aclk_Item.n_Item);
		}
		if (AB_FIELD(af_EnvelopeClock)) {
			DeleteItem (AB_FIELD(af_EnvelopeClock)->aclk_Item.n_Item);
		}

			/* delete folio */
		DBUG_DLL(("TermAudioDaemon: delete audio folio\n"));
		DeleteItem (AB_FIELD(af_Folio).fn.n_Item);
	}

		/* disown DSPP */
	DBUG_DLL(("TermAudioDaemon: disown DSPP\n"));
	DisownDSPP (dsppLock);
}


/* -------------------- Audio Folio callbacks */

/******************************************************************/
/*
	Audio Folio CREATEFOLIO_TAG_INIT vector.

	This is called by the kernel when the folio item is created. This
	runs on the same task which creates the folio, which for the audio folio
	is the audio daemon. So it's ok for this to allocate signals.

	Arguments
		ab
			AudioFolio base structure.

	Results
		Non-negative value for success, negative error code on failure.

	Notes
		Since TermAudioFolio() isn't called by the kernel if this returns
		an error, this function must take care of cleaning up after itself.
*/
static Err InitAudioFolio (AudioFolio *ab)
{
	Err errcode;

	DBUG_DLL(("InitAudioFolio: ab=0x%x owner=0x%x f_TaskDataIndex=%d\n", ab, ab->af_Folio.fn.n_Owner, ab->af_Folio.f_TaskDataIndex));

		/* Init misc folio fields */
	ab->af_AudioModule     = FindCurrentModule();
	ab->af_AudioDaemonItem = CURRENTTASKITEM;
	ab->af_AudioDaemon     = CURRENTTASK;
	PrepList (&ab->af_ClockList);
	PrepList (&ab->af_TemplateList);
	PrepList (&ab->af_SampleList);
	PrepList (&ab->af_ConnectionList);

		/* Install folio callbacks */
	InstallAudioItemRoutines (ab);
	ab->af_Folio.f_FolioCreateTask = CreateAudioTaskHook;
	ab->af_Folio.f_FolioDeleteTask = DeleteAudioTaskHook;

		/* Store some frequently used cache information */
	{
		CacheInfo cinfo;

		GetCacheInfo (&cinfo, sizeof cinfo);
		ab->af_DCacheLineSize = cinfo.cinfo_DCacheLineSize;
	}

		/* init DSPP */
	DBUG_DLL(("InitAudioFolio: init DSPP\n"));
	if ((errcode = DSPP_Init()) < 0) goto clean;

		/* init interrupt handlers */
	DBUG_DLL(("InitAudioFolio: init interrupt handlers\n"));
	if ((errcode = InitAudioDMA()) < 0) goto clean;
	if ((errcode = InitAudioTimer()) < 0) goto clean;
	if ((errcode = InitAudioTrigger()) < 0) goto clean;
	if ((errcode = InitDSPPInterrupt()) < 0) goto clean;

		/* success */
	DBUG_DLL(("InitAudioFolio: success\n"));
	return 0;

clean:
	TermAudioFolio(ab);
	return errcode;
}

/******************************************************************/
/*
	Audio Folio CREATEFOLIO_TAG_DELETEF vector.

	This is called by the kernel when the folio item is deleted, or by
	InitAudioFolio() if it is unsuccessful.
*/
static Err TermAudioFolio (AudioFolio *ab)
{
	TOUCH(ab);
	DBUG_DLL(("TermAudioFolio: ab=0x%x owner=0x%x f_TaskDataIndex=%d\n", ab, ab->af_Folio.fn.n_Owner, ab->af_Folio.f_TaskDataIndex));

		/* terminate interrupt handlers */
	DBUG_DLL(("TermAudioFolio: term interrupt handlers\n"));
	TermDSPPInterrupt();

		/* stop the DSPP */
	DBUG_DLL(("TermAudioFolio: term DSPP\n"));
	DSPP_Term();

	return 0;
}

/*******************************************************************/
/*
	Audio Folio f_FolioDeleteTask vector.

	This is called by the kernel when a task is being created.

	Arguments
		t
			Task being created.

		tagList
			Tags passed to CreateTask()

	Results
		Non-negative value on success (kernel ignores the specific value), Err
		code on failure.

	Notes
		This function first called immediately after the audio Folio Item has
		become completely initialized (i.e., when InitAudioFolio() returns, not
		when CreateAudioModule() returns).
*/
static int32 CreateAudioTaskHook (Task *t, TagArg *tagList)
{
	TOUCH(tagList);

	DBUG_TASKDATA(("CreateAudioTaskHook: Create task data (f_TaskDataIndex=%d) for 0x%x ('%s')", AB_FIELD(af_Folio).f_TaskDataIndex, t->t.n_Item, t->t.n_Name));
	if (!GetAudioFolioTaskData(t)) {
		AudioFolioTaskData *aftd;

		if (!(aftd = SuperAllocMem (sizeof(AudioFolioTaskData), MEMTYPE_NORMAL | MEMTYPE_FILL))) {
		  #if DEBUG_TaskData
			PRT(("\n"));
		  #endif
			ERR(("audio folio: could not alloc folio private data for task 0x%x ('%s').\n", t->t.n_Item, t->t.n_Name));
			return AF_ERR_NOMEM;
		}
		SetAudioFolioTaskData (t, aftd);
	}
#if DEBUG_TaskData
	PRT((" @ 0x%x\n", GetAudioFolioTaskData(t)));
#endif

	return 0;
}

/******************************************************************/
/*
	Audio Folio f_FolioDeleteTask vector.

	This is called by the kernel when a task is being deleted.

	Arguments
		t
			Task being deleted.

	Notes
		This function is called on all existing tasks when the audio folio is
		deleted (i.e., after tearing down system instruments and clocks).
*/
static void DeleteAudioTaskHook (Task *t)
{
	AudioFolioTaskData * const aftd = GetAudioFolioTaskData(t);

  #if DEBUG_TaskData || DEBUG_AudioInput
	PRT(("DeleteAudioTaskHook: Delete task data @ 0x%x for 0x%x ('%s')\n", aftd, t->t.n_Item, t->t.n_Name));
  #endif

	if (aftd) {
	  #if DEBUG_AudioInput
		PRT(("  task audio input enable count: %u\n", aftd->aftd_InputEnables));
	  #endif

			/* Decrement number of tasks using audio input if this task ever enabled audio input. */
		if (aftd->aftd_InputEnables > 0) DecrementGlobalAudioInputEnableCount();

			/* unlink and delete AudioFolioTaskData */
		SetAudioFolioTaskData (t, NULL);
		SuperFreeMem (aftd, sizeof (AudioFolioTaskData));
	}

  #if DEBUG_AudioInput
	PRT(("  system audio input enable count: %u\n", AB_FIELD(af_InputEnableCount)));
  #endif
}


/* -------------------- Global folio control */

/******************************************************************/
/**
|||	AUTODOC -private -class audio -group Miscellaneous -name SetAudioFolioInfo
|||	Set certain system-wide audio folio settings.
|||
|||	  Synopsis
|||
|||	    Err SetAudioFolioInfo (const TagArg *tags)
|||
|||	    Err SetAudioFolioInfoVA (uint32 tag1, ...)
|||
|||	  Description
|||
|||	    Sets certain system-wide audio folio settings.
|||
|||	  Tag Arguments
|||
|||	    AF_TAG_SAMPLE_RATE_FP (float32)
|||	        Sets audio folio's sample rate in samples / second. Initial default
|||	        is 44100.0.
|||
|||	    AF_TAG_AMPLITUDE_FP (float32)
|||	        Sets audio folio's master volume level. Initial default is 1.0.
|||
|||	  Return Value
|||
|||	    Non-negative value on success; negative error code on failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V24.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    GetAudioFolioInfo()
**/
static Err swiSetAudioFolioInfo (const TagArg *tagList)
{
	const TagArg *tag;
	int32 tagResult;
	Err errcode;

	TRACEE(TRACE_INT,TRACE_ITEM,("swiSetAudioFolioInfo (Tags=0x%lx)\n", tagList));
	CHECKAUDIOOPEN;

		/* Verify that caller is privileged. */
	if (!IsPriv(CURRENTTASK)) {
		ERR(("SetAudioFolioInfo: caller must be privileged\n"));
		return AF_ERR_BADPRIV;  /* 940608 */
	}

	for (tagResult = SafeFirstTagArg (&tag, tagList); tagResult > 0; tagResult = SafeNextTagArg (&tag)) {
		DBUG(("SetAudioFolioInfo: tag { %d, 0x%x }\n", tag->ta_Tag, tag->ta_Arg));

		switch (tag->ta_Tag) {
			case AF_TAG_SAMPLE_RATE_FP:
				{
					const float32 SampleRate = ConvertTagData_FP (tag->ta_Arg);

						/* Bounds check sample rate. Out of bounds value causes
						** various FP exceptions (e.g., div by 0, inexact when converting
						** to 1.15 fixed point) */
						/* !!! these limits are guesses and may not prevent all FP exceptions */
					if (SampleRate < 10.0 || SampleRate > 1.0E6) {
						ERR(("SetAudioFolioInfo: Sample rate (%g samp/s) out of range\n", SampleRate));
						return AF_ERR_OUTOFRANGE;
					}

						/* Check to see that we will have enough DSPP ticks in a sample frame.940608  */
					if ((errcode = dsppAdjustAvailTicks (SampleRate)) < 0) return errcode;

					DSPPData.dspp_SampleRate = SampleRate;
					DBUG(("SetAudioFolioInfo: Sample rate changed to %g\n", DSPPData.dspp_SampleRate ));

						/* Recalculate all sample base frequencies based on new rate for proper tuning. */
						/* !!! no failure path for this */
					if ((errcode = UpdateAllSampleBaseFreqs()) < 0) return errcode;

				  #if 0
						/* Update audio timer rate.  FIXME !!! */
					if (AB_FIELD(af_DesiredTimerRate)) {
						if ((errcode = lowSetAudioRate (AB_FIELD(af_DesiredTimerRate))) < 0) return errcode;
					}
				  #endif
				}
				break;

			case AF_TAG_AMPLITUDE_FP:
				if ((errcode = swiSetKnobPart (AB_FIELD(af_NanokernelAmpKnob), 0, ConvertTagData_FP(tag->ta_Arg))) < 0) return errcode;
				DBUG(("SetAudioFolioInfo: Amplitude changed to %g\n", ConvertTagData_FP(tag->ta_Arg)));
				break;

			default:
				ERR(("SetAudioFolioInfo: Unrecognized tag { %d, 0x%x }\n", tag->ta_Tag, tag->ta_Arg));
				return AF_ERR_BADTAG;
		}
	}

		/* Catch tag processing errors */
	if ((errcode = tagResult) < 0) {
		ERR(("SetAudioFolioInfo: Error processing tag list 0x%x\n", tagList));
		return errcode;
	}

	return 0;
}


/******************************************************************/
/**
|||	AUTODOC -public -class audio -group Miscellaneous -name GetAudioFolioInfo
|||	Get system-wide audio settings.
|||
|||	  Synopsis
|||
|||	    Err GetAudioFolioInfo (TagArg *tagList)
|||
|||	  Description
|||
|||	    Queries audio folio for certain system-wide settings. It takes a tag list
|||	    with the ta_Tag fields set to parameters to query and fills in the ta_Arg
|||	    field of each TagArg with the parameter requested by that TagArg.
|||
|||	    For normal multiplayers, these settings are set to the described normal
|||	    values during startup. Other kinds of hardware or execution environments
|||	    (e.g. cable) may have different settings.
|||
|||	    None of these paramaters can be set directly by an application.
|||
|||	  Arguments
|||
|||	    tagList
|||	        Pointer to tag list containing AF_TAG_ tags to query. The ta_Arg fields
|||	        of each matched tag is filled in with the corresponding setting from the
|||	        audio folio. TAG_JUMP and TAG_NOP are treated as they normally are for
|||	        tag processing.
|||
|||	  Tag Arguments
|||
|||	    AF_TAG_SAMPLE_RATE_FP (float32)
|||	        Returns the audio DAC sample rate in samples/second expressed as a float32
|||	        value. Normally this is 44,100 samples/second. For certain cable
|||	        applications this can be 48,000 samples/second.
|||
|||	        When the sample rate is something other than 44,100, substitute this to
|||	        everywhere in the audio documentation that specifies the sample rate as
|||	        44,100 samples/second. If your application needs to cope with variable
|||	        DAC sample rates and you have need the sample rate for something, then
|||	        you should read this value from here rather than hardcoding 44,100, or
|||	        any other sample rate, into your code.
|||
|||	    AF_TAG_AMPLITUDE_FP (float32)
|||	        Returns the audio folio's master volume level. Normally this is 1.0.
|||
|||	  Return Value
|||
|||	    The procedure returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	    AF_ERR_BADTAG
|||	        If this function is passed any tag that is not listed above.
|||
|||	  Additional Results
|||
|||	    Fills in the ta_Arg field of each matched TagArg in tagList with the
|||	    corresponding setting from the audio folio.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V24.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>, libc.a
|||
|||	  See Also
|||
|||	    GetAudioItemInfo()
**/
Err GetAudioFolioInfo (TagArg *tagList)
{
	TagArg *tag;
	int32 tagResult;

	for (tagResult = SafeFirstTagArg (&tag, tagList); tagResult > 0; tagResult = SafeNextTagArg (&tag)) {
		DBUG(("GetAudioFolioInfo: %d\n", tag->ta_Tag));

		switch (tag->ta_Tag) {
			case AF_TAG_SAMPLE_RATE_FP:
				tag->ta_Arg = ConvertFP_TagData (DSPPData.dspp_SampleRate);
				break;

			case AF_TAG_AMPLITUDE_FP:
				{
					float32 temp;
					Err errcode;

					if ( (errcode = ReadKnobPart (AB_FIELD(af_NanokernelAmpKnob), 0, &temp)) < 0 ) return errcode;
					tag->ta_Arg = ConvertFP_TagData (temp);
				}
				break;

			default:
				ERR (("GetAudioFolioInfo: Unrecognized tag (%d)\n", tag->ta_Tag));
				return AF_ERR_BADTAG;
		}
	}

		/* Catch tag processing errors */
	if (tagResult < 0) {
		ERR(("GetAudioFolioInfo: Error processing tag list 0x%x\n", tagList));
		return tagResult;
	}

	return 0;
}


/* -------------------- Enable Audio Input */

static void IncrementGlobalAudioInputEnableCount (void);


/*************************************************************************/
/**
|||	AUTODOC -public -class audio -group Miscellaneous -name EnableAudioInput
|||	Enables or disables audio input.
|||
|||	  Synopsis
|||
|||	    int32 EnableAudioInput (int32 onOrOff, const TagArg *tagList)
|||
|||	  Description
|||
|||	    Enables or disables the use of audio input for the calling task. A successful
|||	    call to EnableAudioInput() must be made before an Instrument(@) can be created
|||	    from the standard DSP instrument template line_in.dsp(@) (or a patch template
|||	    containing line_in.dsp(@)).
|||
|||	    Enable/Disable calls nest. Enabling audio input is tracked for each task;
|||	    you don't need disable it as part of your cleanup.
|||
|||	  Arguments
|||
|||	    onOrOff
|||	        Non-zero to enable, zero to disable. An enable count is maintained for
|||	        each task so that you may nest calls to EnableAudioInput (TRUE, ...).
|||
|||	    tagList
|||	        Reserved for future expansion. Currently must be NULL.
|||
|||	  Return Value
|||
|||	    > 0
|||	        Audio input hardware present and successfully enabled or disabled as
|||	        indicated by onOrOff.
|||
|||	    0
|||	        No audio input hardware present.
|||
|||	    < 0
|||	        Error code.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V24.
|||
|||	  Caveats
|||
|||	    The use of audio input may require a special licensing agreement.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>, libc.a
|||
|||	  See Also
|||
|||	    line_in.dsp(@)
**/
static int32 swiEnableAudioInput (int32 onOrOff, const TagArg *tagList)
{
	AudioFolioTaskData * const aftd = GetAudioFolioTaskData(CURRENTTASK);
	AudInInfo aii;
	Err errcode;

		/* @@@ This shouldn't actually happen, but it seems a safe enough thing to test for. */
	if (!aftd) {
		ERR(("EnableAudioInput: folio task data corrupt\n"));
		return AF_ERR_BADPTR;
	}

		/* No tags currently supported. Must only allow NULL. */
	if (tagList) return AF_ERR_BADTAG;

		/* Support new name for MUD. 940829 */
		/* !!! Shouldn't this be done once during audio folio startup and the results cached? */
	errcode = SuperQuerySysInfo (SYSINFO_TAG_AUDIN, &aii, sizeof(aii));
	DBUGAUDIN(("EnableAudioInput: SuperQuerySysInfo returns 0x%x\n", errcode));
	if (errcode != SYSINFO_SUCCESS) return errcode;
	DBUGAUDIN(("EnableAudioInput: AudInInfo=%u\n", aii));

		/* AudInInfo is non-zero when there is audio input hardware present */
	if (!aii) {
		DBUGAUDIN(("EnableAudioInput: no audio input hardware\n"));
		return 0;
	}
	else if (onOrOff) {
			/* Increment task's enable count. Increment global count if first time for task */
		if (!aftd->aftd_InputEnables++) IncrementGlobalAudioInputEnableCount();
	}
	else {
			/* Decrement task's enable count if not already zero. Decrement global count if last time for task */
		if (aftd->aftd_InputEnables > 0 && !--(aftd->aftd_InputEnables)) DecrementGlobalAudioInputEnableCount();
	}

	DBUGAUDIN(("EnableAudioInput: enables: task=%u global=%u\n", aftd->aftd_InputEnables, AB_FIELD(af_InputEnableCount)));

	return 1;
}

/*
	Increment global audio input enable count (number of tasks using audio
	input). Enable hardware if first time for system.
*/
static void IncrementGlobalAudioInputEnableCount (void)
{
		/* Increment global enable count. Enable hardware if first time for system. */
	if (!AB_FIELD(af_InputEnableCount)++) {
	  #if DEBUG_AudioInput
		PRT(("IncrementGlobalAudioInputEnableCount: enable audio input hardware\n"));
	  #endif
	  #if 0     /* FIXME:  !!! Already enabled! */
		dsphEnableAudioInput();
	  #endif
	}
}

/*
	Decrement global audio input enable count if not already zero. (number of
	tasks using audio input). Disable hardware if last time for system.
*/
static void DecrementGlobalAudioInputEnableCount (void)
{
		/* Decrement global enable count if not already zero. Disable hardware if last time for system. */
	if (AB_FIELD(af_InputEnableCount) > 0 && !--(AB_FIELD(af_InputEnableCount))) {
	  #if DEBUG_AudioInput
		PRT(("DecrementGlobalAudioInputEnableCount: disable audio input hardware\n"));
	  #endif
	  #if 0     /* FIXME:  !!! Unimplemented! */
		dsphDisableAudioInput();
	  #endif
	}
}


/* -------------------- Miscellaneous */

/**************************************************************/
/* !!! remove */
static void swiNotImplemented( void )
{
	ERR(("Audiofolio SWI not implemented.\n"));
	while(1); /* Hang to prevent calls from going undetected. */    /* !!! turn this into a Panic() or Debug() call */
}
