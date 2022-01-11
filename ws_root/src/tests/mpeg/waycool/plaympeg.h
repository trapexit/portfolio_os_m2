/* @(#) plaympeg.h 96/02/21 1.2 */

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/folio.h>
#include <kernel/io.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/list.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/operror.h>
#include <kernel/semaphore.h>
#include <kernel/task.h>
#include <kernel/time.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <graphics/graphics.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>
#include <misc/event.h>
#include <device/mpegvideo.h>

#define NWRITEBUFFERS	            8L
#define WRITEBUFFERSIZE	            (8*1024)
#define DEFAULT_WIDTH				320L
#define DEFAULT_HEIGHT				240L

typedef struct mpegStreamStruct {
	Item        videoDevice;
	Item        diskReadReqItem[ NWRITEBUFFERS ];
	int32       diskReadReqSent[ NWRITEBUFFERS ];
	int32       videoWriteReqSent[ NWRITEBUFFERS ];
	IOInfo      diskReadInfo[ NWRITEBUFFERS ];
	IOReq       *diskReadReq;
	Item        videoReadReqItem;
	Item        videoWriteReqItem[ NWRITEBUFFERS ];
	Item        videoCmdItem;
	IOInfo      videoReadInfo;
	IOInfo      videoWriteInfo[ NWRITEBUFFERS ];
	IOInfo      videoCmdInfo;
	IOReq       *videoReadReq;
	IOReq       *videoWriteReq[ NWRITEBUFFERS ];
	int32       bytesRead;
	int32       wBuffer;
	int32       rBuffer;
	uint32      *inBuffer;
	uint32      inBufferSize;
	uint32      fileOffset;
	Item        theDiskFile;
	Item        diskFile;
	FileStatus  fileStatus;
} mpegStream;


Err init_mpeg(mpegStream *stream, char *filename, int32 pixelMode);
void decodeFrame(mpegStream *stream, Item *bmi);
Err decodeComplete(mpegStream *stream );
int32 feedBitstream(mpegStream *stream);
void closempeg(mpegStream *stream);

