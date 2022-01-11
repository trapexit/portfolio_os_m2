/****************************************************************************
**
**  @(#) dserror.h 96/03/13 1.17
**  Error codes for ALL the Data Stream libraries.
**
*****************************************************************************/
#ifndef __STREAMING_DSERROR_H
#define __STREAMING_DSERROR_H

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif


/* -------------------- Error codes -------------------- */

/* Some of these definitions are included so this will compile with OS Versions prior to 1.4.x. */

#ifndef ER_LINKLIB
	#define ER_LINKLIB					Make6Bit('L')
#endif

#ifndef ER_NoSignals
	#define ER_NoSignals    			24      /* No signals available */
#endif
 
#ifndef ER_DATASTREAMING
	#define ER_DATASTREAMING			MakeErrId('D','S')
#endif

#ifndef MakeDSErr
	#define MakeDSErr(svr,class,err)	MakeErr(ER_LINKLIB,ER_DATASTREAMING,svr,ER_E_SSTM,class,err)
#endif

#ifndef MAKEDSERR
	#define MAKEDSERR(svr,class,err)	MakeDSErr(svr,class,err)
#endif


/* -------------------- Errors numbered by standard system errors -------------------- */
#define kDSNoErr					0
#define kDSBadItemErr				MakeDSErr(ER_SEVERE,ER_C_STND,ER_BadItem)		/* undefined Item passed in */
#define kDSNoMemErr					MakeDSErr(ER_SEVERE,ER_C_STND,ER_NoMem)			/* couldn't allocate needed memory */
#define kDSBadPtrErr				MakeDSErr(ER_SEVERE,ER_C_STND,ER_BadPtr)		/* ptr/range is illegal for this task (e.g. NULL) */
#define kDSAbortErr					MakeDSErr(ER_SEVERE,ER_C_STND,ER_Aborted)		/* operation aborted */
#define kDSBadNameErr				MakeDSErr(ER_SEVERE,ER_C_STND,ER_BadName)		/* Bad Name for Item */
#define kDSUnImplemented			MakeDSErr(ER_SEVERE,ER_C_STND,ER_NotSupported)	/* requested function not implemented */
#define kDSEndOfFileErr				MakeDSErr(ER_INFO,ER_C_STND,ER_EndOfMedium)		/* end of file reached */
#define kDSNoSignalErr				MakeDSErr(ER_SEVERE,ER_C_STND,ER_NoSignals)		/* couldn't allocate needed signal */

/* -------------------- Streaming-specific errors -------------------- */
/* --- THIS MUST BE KEPT IN SYNCH WITH src/errors/LDS.errs.c --- */

#define kDSWasFlushedErr			MakeDSErr(ER_INFO,ER_C_NSTND,1)		/* data acq read request was flushed */
#define kDSNotRunningErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,2)	/* stream not running */
#define kDSWasRunningErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,3)	/* stream is already running */
#define kDSNoPortErr				MakeDSErr(ER_SEVERE,ER_C_NSTND,4)	/* couldn't allocate a message port */
#define kDSNoMsgErr					MakeDSErr(ER_SEVERE,ER_C_NSTND,5)	/* couldn't allocate message item */
#define kDSNotOpenErr				MakeDSErr(ER_SEVERE,ER_C_NSTND,6)	/* stream is not open */
#define kDSSignalErr				MakeDSErr(ER_SEVERE,ER_C_NSTND,7)	/* problem sending/receiving a signal */
#define kDSNoReplyPortErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,8)	/* message requires a reply port */
#define kDSBadConnectPortErr		MakeDSErr(ER_SEVERE,ER_C_NSTND,9)	/* invalid port specified for data connection */
#define kDSSubDuplicateErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,10)	/* duplicate subscriber */
#define kDSSubMaxErr				MakeDSErr(ER_SEVERE,ER_C_NSTND,11)	/* subscriber table is full */
#define kDSSubNotFoundErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,12)	/* specified subscriber not found */
#define kDSInvalidTypeErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,13)	/* invalid subscriber data type specified */
#define kDSBadBufAlignErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,14)	/* buffer list contains non QUADBYTE aligned buffer */
#define kDSBadChunkSizeErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,15)	/* chunk size in stream is out of range */
#define kDSInitErr					MakeDSErr(ER_SEVERE,ER_C_NSTND,16)	/* some internal initialization failed */
#define kDSClockNotValidErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,17)	/* clock-dependent call failed because clock wasn't set */
#define kDSInvalidDSRequest			MakeDSErr(ER_SEVERE,ER_C_NSTND,18)	/* unknown request message send to server thread */
#define kDSEOSRegistrationErr		MakeDSErr(ER_SEVERE,ER_C_NSTND,19)	/* EOS registrant replaced by new registrant */
#define kDSRangeErr					MakeDSErr(ER_SEVERE,ER_C_NSTND,20)	/* parameter out of range */
#define kDSBranchNotDefined			MakeDSErr(ER_SEVERE,ER_C_NSTND,21)	/* branch destination not defined */
#define kDSHeaderNotFound			MakeDSErr(ER_WARN,ER_C_NSTND,22)	/* stream header not found in the stream */
#define kDSVersionErr				MakeDSErr(ER_SEVERE,ER_C_NSTND,23)	/* error in stream header version number */

#define kDSSubscriberVersionErr		MakeDSErr(ER_SEVERE,ER_C_NSTND,24)	/* incompatible subscriber version */
#define kDSChanOutOfRangeErr		MakeDSErr(ER_SEVERE,ER_C_NSTND,25)	/* subscriber channel number out of range */
#define kDSSubscriberBusyErr		MakeDSErr(ER_SEVERE,ER_C_NSTND,26)	/* illegal operation on a busy subscriber */
#define kDSBadFrameBufferDepthErr	MakeDSErr(ER_SEVERE,ER_C_NSTND,27)	/* invalid or incompatible frame buffer bit depth */

#define kDSSTOPChunk				MakeDSErr(ER_INFO,ER_C_NSTND,28)	/* stopped at a STOP chunk */
	/* (deleted 29) */
	/* (deleted 30) */
	/* (deleted 31) */
#define kDSMPEGDevPTSNotValidErr	MakeDSErr(ER_INFO,ER_C_NSTND,32)	/* there's no current, valid MPEG PTS */

#define kDSTemplateNotFoundErr		MakeDSErr(ER_SEVERE,ER_C_NSTND,33)	/* required audio instrument template not found */
#define kDSVolOutOfRangeErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,34)	/* audio volume out of range */
#define kDSPanOutOfRangeErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,35)	/* audio pan out of range */
#define kDSChannelAlreadyInUse      MakeDSErr(ER_SEVERE,ER_C_NSTND,36)  /* audio channel is already in use */
#define kDSSoundSpoolerErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,37)  /* a Sound Spooler error, e.g. couldn't create one */
	/* (deleted 38) */
#define kDSDSPGrabKnobErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,39)	/* couldn't grab a DSP instrument knob */
#define kDSAudioChanErr				MakeDSErr(ER_SEVERE,ER_C_NSTND,40)	/* neither mono, stereo, nor ... */

#define kDSBadStreamSizeErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,41)	/* stream file is empty or its block size doesn't suit the device */
#define kDSInternalStructureErr		MakeDSErr(ER_SEVERE,ER_C_NSTND,42)	/* an internal structural consistency check failed */

#define kDataErrMemsBeenAllocated	MakeDSErr(ER_SEVERE,ER_C_NSTND,43)	/* can't reset alloc/free fcns after memory has been allocated */
#define kDataChunkNotFound			MakeDSErr(ER_SEVERE,ER_C_NSTND,44)	/* can't find partial chunk on sub context */
#define kDataChunkRecievedOutOfOrder	MakeDSErr(ER_SEVERE,ER_C_NSTND,45)	/* chunk arrived out of order */
#define kDataChunkOverflow			MakeDSErr(ER_SEVERE,ER_C_NSTND,46)	/* about to overwrite data chunk buffer */
#define kDataChunkInvalidSize		MakeDSErr(ER_SEVERE,ER_C_NSTND,47)	/* DATA header has impossible size field */
#define kDataChunkInvalidChecksum	MakeDSErr(ER_SEVERE,ER_C_NSTND,48)	/* DATA block is corrupt, checksum doesn't match expectation */

#define	kDSSeekPastSOFErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,50)	/* seek past (before) the start of file */
#define	kDSSeekPastEOFErr			MakeDSErr(ER_SEVERE,ER_C_NSTND,51)	/* seek past (after) the end of file */
#define kDSSOSRegistrationErr		MakeDSErr(ER_SEVERE,ER_C_NSTND,19)	/* SOS registrant replaced by new registrant */

#endif  /* __STREAMING_DSERROR_H */
