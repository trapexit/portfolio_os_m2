
/******************************************************************************
**
**  @(#) photo.c 96/08/29 1.8
**
**  Display MPEG Pictures (one I frame).
**
******************************************************************************/

#define  PRINT_LEVEL 1

#include <string.h>
#include <kernel/debug.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <file/filesystem.h>
#include <file/fileio.h>
#include <misc/event.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <device/mpegvideo.h>
#include "photo-util.h"

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

typedef struct {
	Item bitmap;
	Item view;
	void* bitmapBuffer;
	int32 pixelMode;
	int32 mode640;
	uint32 width;
	uint32 height;
	Item mpegDevice;
	Item readReq;
	Item writeReq;
	RawFile* rFile;
	char* mpegData;
	uint32 fileSize;
	char* filename;
} MPEGImageInfo;

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

static int   Initialize(MPEGImageInfo* imageInfo);
static int   GetOptions(int argc, char* argv[], MPEGImageInfo* imageInfo);
static int   OpenFile(MPEGImageInfo* imageInfo);
static int   ReadMPEGPicture(MPEGImageInfo* imageInfo);
static int   SetupGraphics(MPEGImageInfo* imageInfo);
static int   OpenMPEGDevice(MPEGImageInfo* imageInfo);
static int   SetupMPEGDevice(MPEGImageInfo* imageInfo);
static int   DecompressAndDisplay(MPEGImageInfo* imageInfo);
static int32 MainLoop(void);
static int   Shutdown(MPEGImageInfo* imageInfo);
static Item  AllocateBitmap(MPEGImageInfo* imageInfo);
static void  PrintUsage(const char* name);
static int32 EndofStream(Item mpegDevice);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

int
main(int argc, char *argv[])
{
	Err err;
	MPEGImageInfo imageInfo;

    (void) &err;

	if ((err = Initialize(&imageInfo)))
		goto abort;

	if ((err = GetOptions(argc, argv, &imageInfo)))
		goto abort;

	if ((err = OpenFile(&imageInfo)))
		goto abort;

	if ((err = ReadMPEGPicture(&imageInfo)))
		goto abort;

	if (!imageInfo.mode640)
		err = GetMPEGPictureDimension(imageInfo.mpegData, imageInfo.fileSize,
									  &imageInfo.width, &imageInfo.height);

	if ((err = SetupGraphics(&imageInfo)))
		goto abort;

	if ((err = OpenMPEGDevice(&imageInfo)))
		goto abort;

	if ((err = SetupMPEGDevice(&imageInfo)))
		goto abort;

	err = DecompressAndDisplay(&imageInfo);

	APRNT(("\n--> There is your picture. Press Stop to exit.\n"));

	MainLoop();

abort:
	err = Shutdown(&imageInfo);

	return 0;
}

static int
Initialize(MPEGImageInfo* imageInfo)
{
	int32 err;

	/* Initialize EventBroker. */
    if ((err = InitEventUtility(1, 0, LC_ISFOCUSED)) < 0) {
		APERR(("InitEventUtility Failed: "));
        PrintfSysErr(err);
		return 1;
	}

	if ((err = OpenGraphicsFolio()) < 0) {
		PrintfSysErr(err);
		return 1;
	}

	memset(imageInfo, 0, sizeof(MPEGImageInfo));
	imageInfo->width = 320;
	imageInfo->height= 240;
	imageInfo->pixelMode = 24;
	imageInfo->mode640 = 0;
	return 0;
}

static int
GetOptions(int argc, char* argv[], MPEGImageInfo* imageInfo)
{
	int index;

	if (argc <= 1) {
		PrintUsage(argv[0]);
		return 1;
	}

	for (index = 1; index < argc; index++) {
		if ((strcmp(argv[index], "-f") == 0L) && (argc > index + 1))
			imageInfo->filename = argv[++index];
		else if (strcmp(argv[index], "-16") == 0L)
			imageInfo->pixelMode = 16L;
		else if (strcmp(argv[index], "-640") == 0)
			imageInfo->mode640 = 1;
		else {
			PrintUsage(argv[0]);
			return 1;
		}
	}

	if (imageInfo->mode640) {
		imageInfo->width = 640;
		imageInfo->height = 480;
	}

	if (!imageInfo->filename) {
		PrintUsage(argv[0]);
		return 1;
	}
	return 0;
}

static int
OpenFile(MPEGImageInfo* imageInfo)
{
	FileInfo fileInfo;
	Err err;

	/* Open input MPEG bitstream */
	if (OpenRawFile(&imageInfo->rFile, imageInfo->filename,
					FILEOPEN_READ) < 0) {
		APERR(("Couldn't open %s\n", imageInfo->filename));
		return 1;
	}

	/* Get the file size */
	if ((err = GetRawFileInfo(imageInfo->rFile, &fileInfo,
							  sizeof(FileInfo))) < 0) {
		PrintfSysErr(err);
		return 1;
	}

	imageInfo->fileSize = fileInfo.fi_ByteCount;
	return 0;
}

static int
ReadMPEGPicture(MPEGImageInfo* imageInfo)
{
	int32 bytesRead;

	/* Allocate a buffer; same size as our input file; to read the data */
	if (!(imageInfo->mpegData = (char *) AllocMem(imageInfo->fileSize,
												  MEMTYPE_NORMAL))) {
		APERR(("Couldn't allocate read buffer size:=%d\n", imageInfo->fileSize));
		return -1;
	}

	bytesRead = ReadRawFile(imageInfo->rFile, (void *) imageInfo->mpegData,
							imageInfo->fileSize);
	if (bytesRead < 0) {
		PrintfSysErr(bytesRead);
		return -1;
	}
	if (bytesRead != imageInfo->fileSize) {
		APERR(("Did not read the whole file.\n"));
		return -1;
	}
	return 0;
}

static int
SetupGraphics(MPEGImageInfo* imageInfo)
{
	int32 rSignal;
	Item view;
	enum ViewType theViewType;
	int32 viewWidth;

	if(!(rSignal = AllocSignal(0))) {
		APERR(("Couldn't allocate render signal\n"));
		return 1;
	}

	/* IMPORTANT: image width & height must be multiple of 16 */
	if (imageInfo->width % 16)
		imageInfo->width = imageInfo->width + (16 - (imageInfo->width % 16));
	if (imageInfo->height % 16)
		imageInfo->height = imageInfo->height + (16 - (imageInfo->height % 16));

	if (AllocateBitmap(imageInfo) < 0)
		return 1;

	if (imageInfo->pixelMode == 16) {
		if (imageInfo->mode640)
			theViewType = VIEWTYPE_16_640_LACE;
		else
			theViewType = VIEWTYPE_16;
	} else {
		if (imageInfo->mode640)
			theViewType = VIEWTYPE_32_640_LACE;
		else
			theViewType = VIEWTYPE_32;
	}

	viewWidth = imageInfo->width;
	if (imageInfo->mode640 && imageInfo->width > 640)
		viewWidth = 640;
	else if (imageInfo->width > 320)
		viewWidth = 320;

   	view = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
						VIEWTAG_VIEWTYPE, theViewType,
						VIEWTAG_PIXELWIDTH, viewWidth /*imageInfo->width*/,
						VIEWTAG_BITMAP, imageInfo->bitmap,
						VIEWTAG_RENDERSIGNAL, rSignal,
						TAG_END);
    if (view < 0) {
		APERR(("Creating view failed: "));
		PrintfSysErr(view);
		return 1;
	}
	imageInfo->view = view;

    AddViewToViewList(view, 0);
	WaitSignal(rSignal);
	FreeSignal(rSignal);
	return 0;
}

static int
OpenMPEGDevice(MPEGImageInfo* imageInfo)
{
	int32 err;
	List* list;

 	/* Open the MPEG device */
	err = CreateDeviceStackListVA(&list, "cmds", DDF_EQ, DDF_INT, 1,
								  MPEGVIDEOCMD_CONTROL,	NULL);
    if (err < 0) {
		APERR(("Cannot create device stack list\n"));
		return 1;
	}

    if (IsEmptyList(list)) {
		APERR(("No MPEG devices found\n"));
		return 1;
	}

	imageInfo->mpegDevice = OpenDeviceStack((DeviceStack*) FirstNode(list));
	DeleteDeviceStackList(list);
	if (imageInfo->mpegDevice < 0) {
		PrintfSysErr(imageInfo->mpegDevice);
		return 1;
	}

	return 0;
}

static int
SetupMPEGDevice(MPEGImageInfo* imageInfo)
{
	Item videoCmdItem;
	IOInfo videoCmdInfo;
	CODECDeviceStatus stat;
	Err err;

	/* Create an IOReq for sending control calls to the device */
	if ((videoCmdItem = CreateIOReq(0, 0, imageInfo->mpegDevice, 0)) < 0) {
		PrintfSysErr(videoCmdItem);
		return -1;
	}

	memset(&videoCmdInfo, 0, sizeof(videoCmdInfo));

	/* configure MPEG device */
	memset(&stat, 0, sizeof(CODECDeviceStatus));

	stat.codec_TagArg[0].ta_Tag = VID_CODEC_TAG_STANDARD;
	stat.codec_TagArg[0].ta_Arg = (void *) kCODEC_SQUARE_RESAMPLE;
	stat.codec_TagArg[1].ta_Tag = VID_CODEC_TAG_DEPTH;
	stat.codec_TagArg[1].ta_Arg = (void *) imageInfo->pixelMode;
	stat.codec_TagArg[2].ta_Tag = VID_CODEC_TAG_HSIZE;
	stat.codec_TagArg[2].ta_Arg = (void *) imageInfo->width;
	stat.codec_TagArg[3].ta_Tag = VID_CODEC_TAG_VSIZE;
	stat.codec_TagArg[3].ta_Arg = (void *) imageInfo->height;
	stat.codec_TagArg[4].ta_Tag = VID_CODEC_TAG_M2MODE;
	stat.codec_TagArg[5].ta_Tag = VID_CODEC_TAG_KEYFRAMES; /* IMPORTANT DECODE
															* I FRAME ONLY */
	stat.codec_TagArg[6].ta_Tag = TAG_END;

	videoCmdInfo.ioi_Command = MPEGVIDEOCMD_CONTROL;
	videoCmdInfo.ioi_Send.iob_Buffer = &stat;
	videoCmdInfo.ioi_Send.iob_Len = sizeof(CODECDeviceStatus);

	if ((err = DoIO(videoCmdItem, &videoCmdInfo)) < 0)
		PrintfSysErr(err);

	DeleteIOReq(videoCmdItem);

	if (err)
		return -1;

	return 0;
}

static int
DecompressAndDisplay(MPEGImageInfo* imageInfo)
{
	IOInfo videoReadInfo, videoWriteInfo;
	Item videoReadReqItem, videoWriteReqItem;
	int done = 0;
	char* dataPtr = 0;
	uint32 bytestoWrite, writeSize;
	Err err;

	TOUCH(dataPtr);

	/* Create an IOReq for reading decompressed data */
	if ((videoReadReqItem = CreateIOReq(0, 0, imageInfo->mpegDevice, 0)) < 0) {
		PrintfSysErr(videoReadReqItem);
		return -1;
	}
	imageInfo->readReq = videoReadReqItem;

	memset(&videoReadInfo, 0, sizeof(videoReadInfo));

	videoReadInfo.ioi_Command = MPEGVIDEOCMD_READ;
	/* Decompress data into our bitmap */
	videoReadInfo.ioi_Send.iob_Buffer = &(imageInfo->bitmap);
	videoReadInfo.ioi_Send.iob_Len = sizeof(Item);

	/* Queue up the read request for decompressed data */
	if ((err = SendIO(videoReadReqItem, &videoReadInfo)) < 0)
		PrintfSysErr(err);

	/* Create IOReq for sending compressed data to the driver */
	if ((videoWriteReqItem = CreateIOReq(0, 0, imageInfo->mpegDevice, 0)) < 0) {
		PrintfSysErr(videoWriteReqItem);
		return -1;
	}
	imageInfo->writeReq = videoWriteReqItem;

	memset(&videoWriteInfo, 0, sizeof(IOInfo));

	videoWriteInfo.ioi_Command = MPEGVIDEOCMD_WRITE;

	/* Queue up the write request to send compressed data to the driver. */
	/* Note: Max. of 64K-1 bytes can be sent to the decoder at once.
     *       So if the picture is bigger in size, split the buffer and
     *       send them one at a time.
	 */
	dataPtr = imageInfo->mpegData;
    bytestoWrite = imageInfo->fileSize;
	do {
		if (bytestoWrite > ((64 << 10) - 1))
			writeSize = (64 << 10) - 1;
		else
			writeSize = bytestoWrite;

		videoWriteInfo.ioi_Send.iob_Buffer = dataPtr;
		videoWriteInfo.ioi_Send.iob_Len = writeSize;
		if ((err = SendIO(videoWriteReqItem, &videoWriteInfo)) < 0)
			PrintfSysErr(err);

		if ((err = WaitIO(videoWriteReqItem)) < 0)
			PrintfSysErr(err);

		/*printf("Bytes to write=%d;  wrote=%d\n", bytestoWrite, writeSize);*/

		bytestoWrite -= writeSize;
		if (bytestoWrite == 0)
			done = 1;
		else
			dataPtr += writeSize;

	} while(!done);

	/* Inform the driver that it has recieved a complete picture and
	 * don't wait for another picture start code.
	 */
	EndofStream(imageInfo->mpegDevice);

	/* Wait for the decompressed picture to comeback. */
	err = WaitIO(videoReadReqItem);

	/* Since we are not double buffering and decompressing directly
	 * into the screen you should see the picture now.
	 */
	return err;
}

static int32
MainLoop(void)
{
	Item theItem;
	Err err;
	ControlPadEventData cped;

	(void)&err;

	if ((theItem = CreateTimerIOReq()) < 0)
		return -1;

	do {
		if ((err = GetControlPad (1, FALSE, &cped)) < 0) {
			APERR(("couldn't read control pad\n"));
			break;
		}
		else if (err == 1)	{
			/* if stop button pressed exit the application */
			if (cped.cped_ButtonBits & ControlX)
				break;
		}
		/* Wait for a second */
		if ((err = WaitTime(theItem, 1, 0)) < 0)
			break;
	} while (1);

	DeleteTimerIOReq(theItem);

	return 0;
}

static int
Shutdown(MPEGImageInfo* imageInfo)
{
	if (imageInfo->rFile)
		CloseRawFile(imageInfo->rFile);
	if (imageInfo->bitmap > 0)
		DeleteItem(imageInfo->bitmap);
	if (imageInfo->view > 0)
		DeleteItem(imageInfo->view);
	if (imageInfo->readReq > 0)
		DeleteIOReq(imageInfo->readReq);
	if (imageInfo->writeReq > 0)
		DeleteIOReq(imageInfo->writeReq);
	if (imageInfo->bitmapBuffer)
		; /* WHAT ? */
	if (imageInfo->mpegData)
		FreeMem((void *) imageInfo->mpegData, imageInfo->fileSize);
	if (imageInfo->mpegDevice > 0)
		CloseDeviceStack(imageInfo->mpegDevice);
	(void) CloseGraphicsFolio();
	(void) KillEventUtility();
	return 0;
}


/********************** Helper Functions **************************************/

static int32
AllocateBitmap(MPEGImageInfo* imageInfo)
{
    Bitmap *bm;
    Item item;
    Err err;
    void *buf;

    item = CreateItemVA(MKNODEID (NST_GRAPHICS, GFX_BITMAP_NODE),
						BMTAG_WIDTH, imageInfo->width,
						BMTAG_HEIGHT, imageInfo->height,
						BMTAG_TYPE, ((imageInfo->pixelMode == 16L) ?
									 BMTYPE_16 : BMTYPE_32),
						BMTAG_DISPLAYABLE, TRUE,
						BMTAG_MPEGABLE, TRUE,
						TAG_END);
    if (item < 0) {
		APERR(("Creating bitmap failed: "));
        PrintfSysErr(item);
		return -1;
    }
	imageInfo->bitmap = item;
    bm = LookupItem(item);

    buf = AllocMemMasked(bm->bm_BufferSize,
						 bm->bm_BufMemType,
						 bm->bm_BufMemCareBits,
						 bm->bm_BufMemStateBits);
	if (!buf) {
        APERR(("Can't allocate bitmap buffer.\n"));
		return -1;
	}
	imageInfo->bitmapBuffer = buf;
	memset(buf, 0x800000L, bm->bm_BufferSize);

    if ((err = ModifyGraphicsItemVA(item, BMTAG_BUFFER, buf, TAG_END)) < 0) {
        APERR(("Can't modify Bitmap Item.\n"));
        PrintfSysErr(err);
		return -1;
    }

    return 0;
}

static void
PrintUsage(const char* name)
{
	APRNT(("Usage: %s -f   filename \n",name));
	APRNT(("          -16  display in 16 bit color. Otherwise 24.\n"));
	APRNT(("          -640 decode in 640x480 mode\n"));
	APRNT((" Control Pad functions:\n"
           "    Stop         []    Quit the application\n"
           ));
}

static int32
EndofStream(Item mpegDevice)
{
	IOInfo theInfo;
	FMVIOReqOptions theOptions;
	Item theItem;
	Err err;

	memset(&theOptions, 0, sizeof(FMVIOReqOptions));
	memset(&theInfo, 0, sizeof(IOInfo));

	/* To inform the mpeg device that it has a complete picture:
	 * write a zero length buffer to the driver with
	 * FMV_END_OF_STREAM_FLAG set in the ioi_CmdOptions field.
	 */

	theItem = CreateIOReq(0, 0, mpegDevice, 0);
	theInfo.ioi_Command = MPEGVIDEOCMD_WRITE;
	theInfo.ioi_Send.iob_Buffer =	0;
	theInfo.ioi_Send.iob_Len = 0;
	theOptions.FMVOpt_Flags = FMV_END_OF_STREAM_FLAG;
	theInfo.ioi_CmdOptions = (int32) &theOptions;

	if ((err = SendIO(theItem, &theInfo)) < 0)
		PrintfSysErr(err);

	/* 911-- WARN: DONOT FORGET TO DELETE theItem */

	return err;
}

