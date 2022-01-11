/****************************************************************************
 **  @(#) LDS.errs.c 96/06/10 1.15
 ****************************************************************************/

#include <streaming/dserror.h>


/* -------------------- error table -------------------- */
/* --- NOTE: THIS MUST BE KEPT IN SYNCH WITH dserror.h --- */

static const char *const DSErrorText[] =
{
/*  0 kDSNoErr */					"no error",
/*  1 kDSWasFlushedErr */			"data acq read request was flushed",
/*  2 kDSNotRunningErr */			"stream isn't running",
/*  3 kDSWasRunningErr */			"stream is already running",
/*  4 kDSNoPortErr */				"couldn't allocate a message port",
/*  5 kDSNoMsgErr */				"couldn't allocate a message item",
/*  6 kDSNotOpenErr */				"stream is not open",
/*  7 kDSSignalErr */				"problem sending/receiving a signal",
/*  8 kDSNoReplyPortErr */			"message requires a reply port",
/*  9 kDSBadConnectPortErr */		"invalid port specified for data connection",
/* 10 kDSSubDuplicateErr */			"duplicate subscriber",
/* 11 kDSSubMaxErr */				"subscriber table is full",
/* 12 kDSSubNotFoundErr */			"specified subscriber not found",
/* 13 kDSInvalidTypeErr */			"invalid subscriber data type specified",
/* 14 kDSBadBufAlignErr */			"buffer list contains non QUADBYTE aligned buffer",
/* 15 kDSBadChunkSizeErr */			"chunk size in stream is out of range",
/* 16 kDSInitErr */					"streamer internal initialization failed",
/* 17 kDSClockNotValidErr */		"clock-dependent call failed because clock wasn't set",
/* 18 kDSInvalidDSRequest */		"unknown request message send to server thread",
/* 19 kDSEOSRegistrationErr */		"End Of Stream registrant replaced by new registrant",
/* 20 kDSRangeErr */				"parameter out of range",
/* 21 kDSBranchNotDefined */		"branch destination not defined",

/* 22 kDSHeaderNotFound */			"stream header not found in the stream",
/* 23 kDSVersionErr */				"error in stream header version number",

/* 24 kDSSubscriberVersionErr */	"incompatible subscriber version",
/* 25 kDSChanOutOfRangeErr */		"subscriber channel number out of range",
/* 26 kDSSubscriberBusyErr */		"illegal operation on a busy subscriber",
/* 27 kDSBadFrameBufferDepthErr */	"invalid or incompatible frame buffer bit depth",

/* 28 kDSSTOPChunk */				"stopped at a STOP chunk",
/* 29 (deleted) */					"",
/* 30 (deleted) */					"",
/* 31 (deleted) */					"",
/* 32 kDSMPEGDevPTSNotValidErr */	"there's no current, valid MPEG PTS",

/* 33 kDSTemplateNotFoundErr */		"required audio instrument template not found",
/* 34 kDSVolOutOfRangeErr */		"audio volume out of range",
/* 35 kDSPanOutOfRangeErr */		"audio pan out of range",
/* 36 kDSChannelAlreadyInUse */		"audio channel is already in use",
/* 37 kDSSoundSpoolerErr	*/		"Sound Spooler error, e.g. couldn't create one",
/* 38 (deleted) */					"",
/* 39 kDSDSPGrabKnobErr */			"couldn't grab a DSP instrument knob",
/* 40 kDSAudioChanErr */			"neither mono, stereo, nor ...",

/* 41 kDSBadStreamSizeErr */		"stream file is empty or its block size doesn't suit the device",
/* 42 kDSInternalStructureErr */	"an internal structural consistency check failed",

/* 43 kDataErrMemsBeenAllocated */	"can't reset alloc/free fcns after memory has been allocated",
/* 44 kDataChunkNotFound */			"data subscriber can't find partial chunk",
/* 45 kDataChunkRecievedOutOfOrder */	"data subscriber chunk arrived out of order",
/* 46 kDataChunkOverflow */			"about to overwrite data chunk buffer",
/* 47 kDataChunkInvalidSize */		"DATA header has impossible size field",
/* 48 kDataChunkInvalidChecksum */	"DATA block checksum mismatch"
};


const TagArg *main(void)
	{
	static const TagArg ErrorTags[] =
		{
		TAG_ITEM_NAME,		(void *)"DataStream Library",
		ERRTEXT_TAG_OBJID,	(void *)((ER_LINKLIB << ERR_IDSIZE) | (ER_DATASTREAMING)),
		ERRTEXT_TAG_MAXERR,	(void *)(sizeof(DSErrorText) / sizeof(DSErrorText[0])),
		ERRTEXT_TAG_TABLE,	(void *)DSErrorText,
		TAG_END,			0
		};

    return ErrorTags;
	}
