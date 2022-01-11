/* @(#) FMa.errs.c 96/03/28 1.3 */

#include <kernel/types.h>
#include <kernel/operror.h>


static const char * const ErrorMsgs[] =
{
									"no error",
	/* 1 MPAParsingErr */ 			"Error during parsing",
	/* 2 MPAUnsupportedLayerErr */	"Unsupported layer: only layer II is supported",
	/* 3 MPABufrTooSmallErr */		"Result buffer is too small for decoded data"
	/* 4 MPAInvalidBitrateErr */ 		"Invalid bitrate",
	/* 5 MPAInvalidSampleFreqErr */	"Invalid sampling frequence",
	/* 6 MPAInvalidBitrateModeErr */	"Invalid bitrate and mode combination",
	/* 7 MPAInvalidIDErr */			"Only ISO/IEC 11172-3 audio supported",
	/* 8 MPAUndefinedLayerErr */		"Undefined or reserved layer",
	/* 9 MPAOutputDataNotAvailErr */	" Decompressed data not yet available",
	/* 10 MPAInputDataNotAvailErr */	" Compressed data not yet available",
	/* 11 MPAEOFErr */					" EOF Encountered."
};

static const TagArg ErrorTags[] =
{
    TAG_ITEM_NAME,	(void *)"MPEGAudioDecoder Folio",
    ERRTEXT_TAG_OBJID,	(void *)((ER_FOLI<<ERR_IDSIZE)|(ER_MPA)),
    ERRTEXT_TAG_MAXERR,	(void *)(sizeof(ErrorMsgs)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,	(void *)ErrorMsgs,
    TAG_END,		0
};

const TagArg *main (void)
{
    return ErrorTags;
}
