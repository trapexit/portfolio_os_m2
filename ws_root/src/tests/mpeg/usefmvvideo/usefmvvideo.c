/* @(#) usefmvvideo.c 96/04/30 1.17 */

/* test operation of FMV video driver */

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <kernel/mem.h>
#include <kernel/time.h>
#include <file/filesystem.h>
#include <graphics/graphics.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>
#include <misc/event.h>
#include <file/filefunctions.h>
#include <device/mpegvideo.h>

static Item allocbitmap(int32 wide, int32 high, int32 type);
static void opentimer(void);
static Err waitvbl(int32 n);
static void openstuff(void);
static void die(char *str);
static int AllocateReadWriteBuffer(uint32 **inBuffer, uint32* inBufferSize);
static void PrintUsage(void);
static void GetOptions(int argc, char** argv);
static int CheckControlPad(void);
static int SetupMPEGDevice(void);
static int SetupGraphics(void);
static DecompressedDataReady(Item videoReadReqItem, IOReq *videoReadReq,
							 IOInfo* videoReadInfo);
static int32 GetFileStatus(Item diskFile, FileStatus* fileStatus,
						   IOInfo* diskReadInfo, Item* diskReadReqItem);
static int32 SetupWriteBuffers(FMVIOReqOptions* timeStampInfo,
							   uint32* inBuffer);
static int32 SetupReadBuffer(Item* videoReadReqItem, IOInfo* videoReadInfo);
static int32 QueueDiskReads(Item diskFile, uint32* inBuffer, uint32* fileOffset,
							FileStatus* fileStatus);

#ifdef DEBUG_PRINT
#define DBUG(x)	{ printf x ; }
#else
#define DBUG(x)	{ ; }
#endif

#define NWRITEBUFFERS	8L
#define WRITEBUFFERSIZE	(11*1024)
#define DEFAULT_FILENAME1			"envogue.vbs"
#define DEFAULT_VBL_COUNT			5L
#define DEFAULT_WIDTH				320L
#define DEFAULT_HEIGHT				240L
#define DEFAULT_PIXEL_DEPTH			24L
#define DEFAULT_LOOP_COUNT			1000000L
#define DEFAULT_RESAMPLE_MODE		0
#define DEFAULT_SWAP_STREAM_MODE	false
#define DEFAULT_PRINT_PTS			false
#define DEFAULT_GET_PTS				true
#define VERSION_NUMBER				"1.0"

static int32 rBuffer;
static int32 wBuffer;
static int32 vblCount = DEFAULT_VBL_COUNT;
static int32 pause = 0;
static int32 decodeIFrames = 0L;
static int32 resampleMode = DEFAULT_RESAMPLE_MODE;
static int32 pixelMode = DEFAULT_PIXEL_DEPTH;
static int32 hvSize[2];
static int32 framesToSkip = 0L;
static int32 frameSize;
static int32 bufferNumber = 0L;
static int32 printDecodingTimeFlag = 0L;
static int32 diskReadReqSent[ NWRITEBUFFERS ];
static int32 printPTSFlag = DEFAULT_PRINT_PTS;
static int32 getPTSFlag = DEFAULT_GET_PTS;
static int32 continueFlag = 0L;
static int32 pts = 0L;
static int32 loop = DEFAULT_LOOP_COUNT;
static int32 mode640 = 0;
static char *filename1 = DEFAULT_FILENAME1;
static IOReq *diskReadReq;
static IOReq *videoReadReq;
static Item videoWriteReqItem[ NWRITEBUFFERS ];
static Item diskReadReqItem[ NWRITEBUFFERS ];
static Item videoCmdItem;
static Item videoDevice;
static Item bmi[2];
static Item vi[2];
static IOInfo videoCmdInfo;
static IOInfo videoWriteInfo[ NWRITEBUFFERS ];
static IOInfo diskReadInfo[ NWRITEBUFFERS ];
static CODECDeviceStatus stat;
static ubyte* baseAddr[2];
struct DisplayFolioBase *DBase;
static Item timerio;
static uint32 signal;


int
main(int argc, char *argv[])
{
	Item videoReadReqItem;
	IOInfo videoReadInfo;
	FMVIOReqOptions timeStampInfo;
	int32 bytesRead, size, err;
	uint32 *inBuffer, inBufferSize, fileOffset = 0L;
	Item diskFile;
	FileStatus fileStatus;
	int32 curReadCnt = 0L, readsOut;
	uint32 loopCount = 1;

	hvSize[0] = DEFAULT_WIDTH; hvSize[1] = DEFAULT_HEIGHT;

	GetOptions(argc, argv);

	if (AllocateReadWriteBuffer(&inBuffer, &inBufferSize))
		return -1;

	if( (diskFile = OpenFile(filename1)) < 0 ) {
		printf("Couldn't open %s\n",filename1);
		return -1;
	}

	GetFileStatus(diskFile, &fileStatus, &diskReadInfo[0], &diskReadReqItem[0]);

	if (InitEventUtility(1, 0, 1) < 0) {
		DBUG (("Error - unable to connect to event broker\n"));
	}

	SetupGraphics();

	if (SetupMPEGDevice() < 0) {
		printf("Setup MEPG Device failed\n");
		return -1;
	}

	SetupReadBuffer(&videoReadReqItem, &videoReadInfo);

	SetupWriteBuffers(&timeStampInfo, inBuffer);

	QueueDiskReads(diskFile, inBuffer, &fileOffset, &fileStatus);

	wBuffer = rBuffer = 0L;
	readsOut = NWRITEBUFFERS;

	DBUG(("Sending read req\n"));
	err = SendIO(videoReadReqItem, &videoReadInfo);
	if (err)
       	PrintfSysErr( err );

	DBUG(("Entering main loop\n"));
	bytesRead = 0L;

	while( 1 )
	{
		if (CheckControlPad())
			break;

		DecompressedDataReady(videoReadReqItem, videoReadReq, &videoReadInfo);

		if (CheckIO(videoWriteReqItem[wBuffer]) && !diskReadReqSent[wBuffer])
		{
			if( getPTSFlag && (wBuffer == 0L) )
				/* clear pts valid flag */
				timeStampInfo.FMVOpt_Flags &= ~FMVValidPTS;

			diskReadInfo[ wBuffer ].ioi_Offset = fileOffset;
			fileOffset += (WRITEBUFFERSIZE * sizeof( uint32 )) /
						  fileStatus.fs.ds_DeviceBlockSize;
			if (fileOffset >= fileStatus.fs.ds_DeviceBlockCount)
			{
				if (loop--)
				{
					printf("LoopCount: %ld\n",++loopCount);
					fileOffset = 0L;
				}
				else
					break;
			}
			diskReadReqSent[ wBuffer ] = 1;
			err = SendIO( diskReadReqItem[wBuffer],&(diskReadInfo[wBuffer]) );
			if( err )
       			PrintfSysErr( err );
			readsOut++;
			if( readsOut > NWRITEBUFFERS )
				DBUG(("ACK! readsOut = %ld\n",readsOut));

			curReadCnt++;

			wBuffer = (wBuffer + 1) % NWRITEBUFFERS;
			continueFlag = 1;
		}

		if( diskReadReqSent[ rBuffer ] &&
			CheckIO( diskReadReqItem[ rBuffer ] ) )
		{
			readsOut--;
			if( readsOut < 1 )
			{
#if 0
				printf("readsOut = %ld, w: %ld, r: %ld, sent ",
						readsOut,wBuffer,rBuffer);
				for(i = 0; i < NWRITEBUFFERS; i++ )
					printf("%ld ",diskReadReqSent[ i ]);
				printf("\n");
#endif
			}
			diskReadReqSent[ rBuffer ] = 0L;
			diskReadReq = (IOReq *) LookupItem( diskReadReqItem[ rBuffer ] );
 			size = diskReadReq->io_Actual;
			bytesRead += size;
			if( bytesRead > fileStatus.fs_ByteCount )
			{
				size -= bytesRead - fileStatus.fs_ByteCount;
				bytesRead = 0L;
			}
			videoWriteInfo[ rBuffer ].ioi_Send.iob_Len = size;
			err = SendIO(videoWriteReqItem[rBuffer],&(videoWriteInfo[rBuffer]));
			if( err )
       			PrintfSysErr( err );

			rBuffer = (rBuffer + 1) % NWRITEBUFFERS;
			continueFlag = 1;
		}
		if( continueFlag )
			continue;
	}

	CloseDeviceStack(videoDevice);

	DBUG(("usefmvvideo: Exiting.\n"));
	exit(0);
}


static int
DecompressedDataReady(Item videoReadReqItem, IOReq *videoReadReq,
					  IOInfo* videoReadInfo)
{
	static int32 pictureCount = 0L;
	int32 i, err;

	if( CheckIO( videoReadReqItem ) )
	{
		if( pictureCount++ == 45 )
			if( framesToSkip )
			{
				stat.codec_TagArg[0].ta_Tag = VID_CODEC_TAG_SKIPFRAMES;
				stat.codec_TagArg[0].ta_Arg = (void *) framesToSkip;
				stat.codec_TagArg[1].ta_Tag = TAG_END;
				DoIO( videoCmdItem, &videoCmdInfo );
			}
		if( getPTSFlag )
		{
			int32 ptsValid;

			ptsValid = ((videoReadReq->io_Flags & FMVValidPTS) != 0L);
			TOUCH(ptsValid);

			if(printPTSFlag)
			{
				/* count = FMVGetPTSOffset( videoReadReq ); */
				/* scr = count & 0xffffL; */
				/* count >>= 16; */

				/* print out pts info */
				DBUG(("usefmvvideo: pts %01ld %08ld\n",
						ptsValid,FMVGetPTSValue(videoReadReq)));
			}
		}
		if( printDecodingTimeFlag )
			DBUG(("usefmvvideo: decoding time: %08lx\n",
					FMVGetPTSOffset( videoReadReq )));

		/* Flip buffers */
		i = ModifyGraphicsItemVA( vi[ 0 ],
								 VIEWTAG_BITMAP, bmi[ bufferNumber ],
								 TAG_END);
		if( i < 0 )
    	{
			PrintfSysErr ( i );
			die ("Can't modify Bitmap Item.\n");
		}

		if( vblCount )
		{
			waitvbl( vblCount / 2 );
			if( bufferNumber && (vblCount & 0x1) )
				waitvbl( 1 );
		}
		else if( pause )
			waitvbl( 1 );

		bufferNumber ^= 1;

		videoReadInfo->ioi_Send.iob_Buffer = &bmi[bufferNumber];
		err = SendIO(videoReadReqItem, videoReadInfo);
		if( err )
			PrintfSysErr( err );

		continueFlag = 1;
	}
	return 0;
}


static int
SetupMPEGDevice()
{
	List *list;
	int32 err;

	err = CreateDeviceStackListVA(&list, "cmds", DDF_EQ,
								  DDF_INT, 1, MPEGVIDEOCMD_CONTROL,	NULL);
	if (err < 0) {
		PrintfSysErr(err);
		return -1;
	}

	if (IsEmptyList(list))
		return -1;

	videoDevice = OpenDeviceStack((DeviceStack*) FirstNode(list));
	DeleteDeviceStackList(list);
	if (videoDevice < 0) {
		PrintfSysErr(videoDevice);
		return -1;
	}

    DBUG(("usefmvvideo: videoDevice = %ld\n", videoDevice));

	/* Create a IOReq for sending control and status calls to the device */
	videoCmdItem = CreateIOReq(0,0,videoDevice,0);
	DBUG(("usefmvvideo: videoCmdItem = %ld\n", videoCmdItem));

	memset(&videoCmdInfo,0,sizeof(videoCmdInfo));

	/* Control call test code */
	memset(&stat,0,sizeof(CODECDeviceStatus));
	videoCmdInfo.ioi_Command = MPEGVIDEOCMD_CONTROL;
	videoCmdInfo.ioi_Send.iob_Buffer = &stat;
	videoCmdInfo.ioi_Send.iob_Len = sizeof( CODECDeviceStatus );

	stat.codec_TagArg[0].ta_Tag = VID_CODEC_TAG_STANDARD;
	switch( resampleMode )
	{
		case 0:
			stat.codec_TagArg[0].ta_Arg = (void *) kCODEC_SQUARE_RESAMPLE;
			break;
		case 1:
			stat.codec_TagArg[0].ta_Arg = (void *) kCODEC_NTSC_RESAMPLE;
			break;
		case 2:
			stat.codec_TagArg[0].ta_Arg = (void *) kCODEC_PAL_RESAMPLE;
			break;
	}
	stat.codec_TagArg[1].ta_Tag = VID_CODEC_TAG_DEPTH;
	stat.codec_TagArg[1].ta_Arg = (void *) pixelMode;
	stat.codec_TagArg[2].ta_Tag = VID_CODEC_TAG_HSIZE;
	stat.codec_TagArg[2].ta_Arg = (void *) hvSize[0];
	stat.codec_TagArg[3].ta_Tag = VID_CODEC_TAG_VSIZE;
	stat.codec_TagArg[3].ta_Arg = (void *) hvSize[1];
	stat.codec_TagArg[4].ta_Tag = VID_CODEC_TAG_M2MODE;
	stat.codec_TagArg[5].ta_Tag = TAG_END;
	DoIO( videoCmdItem, &videoCmdInfo );

	if( decodeIFrames == true )
	{
		stat.codec_TagArg[0].ta_Tag = VID_CODEC_TAG_KEYFRAMES;
		stat.codec_TagArg[1].ta_Tag = TAG_END;
		DoIO( videoCmdItem, &videoCmdInfo );
	}
	else if( framesToSkip )
	{
		stat.codec_TagArg[1].ta_Tag = VID_CODEC_TAG_SKIPFRAMES;
		stat.codec_TagArg[1].ta_Arg = (void *) framesToSkip;
		DoIO( videoCmdItem, &videoCmdInfo );
	}
	return 0;
}


static int
SetupGraphics()
{
    Bitmap  *bm[2];
	int32 rSignal;
	int32 i;
	enum ViewType theViewType;

	DBUG(("%d bit\n", pixelMode));
	frameSize = 320 * 240 * ((pixelMode == 16) ? 2L : 4L);
	openstuff();

	for( i = 0L; i < 2; i++ ) {
	   	bmi[ i ] = allocbitmap(320, 240,
							   ((pixelMode == 16L) ? BMTYPE_16 : BMTYPE_32));
	   	bm[ i ] = LookupItem (bmi[ i ]);
       	baseAddr[ i ] = (uint8 *) bm[ i ]->bm_Buffer;
	}

	if( !(rSignal = AllocSignal( 0 )) ) {
		DBUG(("Couldn't allocate render signal\n"));
		exit( 1 );
	}

	if( pixelMode == 16 )
		if( mode640 )
			theViewType = VIEWTYPE_16_640_LACE;
		else
			theViewType = VIEWTYPE_16;
	else
		if( mode640 )
			theViewType = VIEWTYPE_32_640_LACE;
		else
			theViewType = VIEWTYPE_32;

	i = 0L;
	vi[ i ] = CreateItemVA (MKNODEID (NST_GRAPHICS, GFX_VIEW_NODE),
							VIEWTAG_VIEWTYPE, theViewType,
							VIEWTAG_PIXELWIDTH, 320,
							VIEWTAG_BITMAP, bmi[ i ],
							VIEWTAG_RENDERSIGNAL, rSignal,
							TAG_END);

	DBUG(("view item = 0x%08lx\n", vi[ i ] ));
	if (vi[ i ] < 0)
		PrintfSysErr (vi[ i ]);

	DBUG(("Adding view...\n"));
	AddViewToViewList (vi[ i ], 0);
	DBUG(("Waiting for rendersignal..."));
	WaitSignal( rSignal );
	DBUG(("Got it!\n"));

	DBUG(("usefmvvideo: alloc'd framebuffers @ %08lx, %08lx\n",
			baseAddr[0], baseAddr[1]));
	return 0;
}


static int32
QueueDiskReads(Item diskFile, uint32* inBuffer, uint32* fileOffset,
			   FileStatus* fileStatus)
{
	int32 wBuffer;
	int32 err = 0;
	TOUCH(err);

	for (wBuffer = 0L; wBuffer < NWRITEBUFFERS; wBuffer++)
	{
		diskReadReqItem[ wBuffer ]  = CreateIOReq( 0, 0, diskFile, 0);
		DBUG(("     diskReadReqItem = %ld\n",diskReadReqItem[ wBuffer ]));

		memset(&(diskReadInfo[ wBuffer ]), 0, sizeof( IOInfo ));

		diskReadInfo[ wBuffer ].ioi_Command = CMD_BLOCKREAD;
		diskReadInfo[ wBuffer ].ioi_Offset = *fileOffset;
		*fileOffset += (WRITEBUFFERSIZE * sizeof( uint32 )) /
			fileStatus->fs.ds_DeviceBlockSize;
		if( *fileOffset >= fileStatus->fs.ds_DeviceBlockCount )
			*fileOffset = 0L;
		diskReadInfo[ wBuffer ].ioi_Recv.iob_Buffer =
			((char *) &(inBuffer[ wBuffer*WRITEBUFFERSIZE ]));
		diskReadInfo[ wBuffer ].ioi_Recv.iob_Len =
			WRITEBUFFERSIZE * sizeof( uint32 );

		diskReadReqSent[ wBuffer ] = 1;
		err = SendIO( diskReadReqItem[ wBuffer ], &(diskReadInfo[ wBuffer ]) );
		if( err )
        	PrintfSysErr( err );
	}
	return err;
}


static int32
SetupReadBuffer(Item* videoReadReqItem, IOInfo* videoReadInfo)
{
	/* Create a IOReq for reading decompressed data */

	*videoReadReqItem = CreateIOReq(0,0,videoDevice,0);
	DBUG(("usefmvvideo: videoReadReqItem = %ld\n", *videoReadReqItem));

	videoReadReq = (IOReq *) LookupItem(*videoReadReqItem);
	memset(videoReadInfo, 0, sizeof(IOInfo));

	videoReadInfo->ioi_Command = MPEGVIDEOCMD_READ;
	videoReadInfo->ioi_Send.iob_Buffer = &bmi[bufferNumber];
	videoReadInfo->ioi_Send.iob_Len = sizeof(Item);

	return 0;
}


static int32
SetupWriteBuffers(FMVIOReqOptions* timeStampInfo,
				  uint32* inBuffer)
{
	int32 wBuffer;

	for (wBuffer = 0L; wBuffer < NWRITEBUFFERS; wBuffer++) {
		videoWriteReqItem[wBuffer] = CreateIOReq(0, 0, videoDevice, 0);

		DBUG(("usefmvvideo: videoWriteReqItem = %ld\n",
			  videoWriteReqItem[wBuffer]));

		memset(&(videoWriteInfo[wBuffer]), 0, sizeof(IOInfo));

		if (getPTSFlag && (wBuffer == 0L)) {
			/* fudge initial timestamp */
			memset(timeStampInfo, 0, sizeof(FMVIOReqOptions));
			timeStampInfo->FMVOpt_Flags = FMVValidPTS;
			timeStampInfo->FMVOpt_PTS = pts;			/* fudged value */
			timeStampInfo->FMVOpt_PTS_Offset = 0L;
			videoWriteInfo[wBuffer].ioi_CmdOptions = (int32) timeStampInfo;
		} else {
			videoWriteInfo[wBuffer].ioi_CmdOptions = (int32) 0L;
		}

		videoWriteInfo[wBuffer].ioi_Command = MPEGVIDEOCMD_WRITE;
		videoWriteInfo[wBuffer].ioi_Send.iob_Buffer =
			((char *) &(inBuffer[ wBuffer*WRITEBUFFERSIZE ]));
		videoWriteInfo[wBuffer].ioi_Send.iob_Len =
			WRITEBUFFERSIZE * sizeof( uint32 );
		DBUG(("send buffer (%ld bytes) at %08lx\n",
			  videoWriteInfo[wBuffer].ioi_Send.iob_Len,
			  videoWriteInfo[wBuffer].ioi_Send.iob_Buffer));
	}
	return 0;
}


static int32
GetFileStatus(Item diskFile, FileStatus* fileStatus,
			  IOInfo* diskReadInfo, Item* diskReadReqItem)
{
	int32 err = 0;
	TOUCH(err);

	*diskReadReqItem = CreateIOReq(0, 0, diskFile, 0);
	memset(diskReadInfo, 0, sizeof(IOInfo));
	diskReadInfo->ioi_Command = CMD_STATUS;
    diskReadInfo->ioi_Recv.iob_Buffer = fileStatus;
    diskReadInfo->ioi_Recv.iob_Len = sizeof(FileStatus);
    err = DoIO(*diskReadReqItem, diskReadInfo);
	DBUG(("%ld bytes in %s\n", fileStatus->fs_ByteCount,filename1));

	return err;
}


static int
CheckControlPad()
{
	int32 err;
	int32 i;
	IOReq  *videoWriteReq;
	ControlPadEventData cped;

	do {
		err = GetControlPad (1, FALSE, &cped);
		if( err < 0 )
		{
			DBUG(("couldn't read control pad\n"));
			return -1;
		}
		else if( err == 1 )
		{
			if( cped.cped_ButtonBits & ControlLeftShift )
		  	{
				printf("dumping IOReq state\n");
				printf("rBuffer: %ld, wBuffer: %ld\n",rBuffer,wBuffer);
				for( i = 0; i < NWRITEBUFFERS; i++ )
			   	{
					printf("diskReadReqSent[%ld]: %ld\n",
						   i,diskReadReqSent[i]);
					diskReadReq =
						(IOReq *) LookupItem( diskReadReqItem[ i ] );
					printf("IOReqItem: %ld @ 0x%08lx\n",
						   diskReadReqItem[ i ], diskReadReq);
					printf("	r: io_Flags: 0x%08lx, io_Error: 0x%08lx\n",
						   diskReadReq->io_Flags,diskReadReq->io_Error);

					videoWriteReq =
						(IOReq *) LookupItem( videoWriteReqItem[ i ] );
					printf("	w: io_Flags: 0x%08lx, io_Error: 0x%08lx\n",
						   videoWriteReq->io_Flags,videoWriteReq->io_Error);
				}
			}
			if( cped.cped_ButtonBits & ControlRightShift )
				;
			if( cped.cped_ButtonBits & ControlLeft )
				;
			if( cped.cped_ButtonBits & ControlRight )
				break;
			if( cped.cped_ButtonBits & ControlUp )
				if( !pause )
					vblCount = (vblCount > 0) ? --vblCount : 0;
			if( cped.cped_ButtonBits & ControlDown )
				if( !pause )
					vblCount++;
			if( cped.cped_ButtonBits & ControlX )
				return -1;
			if( cped.cped_ButtonBits & ControlStart )
				pause = pause ? 0 : 1;
			if( cped.cped_ButtonBits & ControlA )
				if( !pause )
					vblCount = 5;
			if( cped.cped_ButtonBits & ControlB )
				if( !pause )
					vblCount = 4;
			if( cped.cped_ButtonBits & ControlC )
				if( !pause )
					vblCount = 0;
		}
		if( pause )
			waitvbl( 1 );
	} while( pause );
	return 0;
}


static int
AllocateReadWriteBuffer(uint32 **inBuffer, uint32* inBufferSize)
{
	*inBufferSize = NWRITEBUFFERS * WRITEBUFFERSIZE * sizeof( uint32 );
	*inBuffer = (uint32 *) AllocMem(*inBufferSize, MEMTYPE_DMA);
    if (inBuffer) {
		DBUG(("inBuffer (%d bytes) at %08lx\n",*inBufferSize, *inBuffer));
	} else {
		DBUG(("Couldn't allocate inBuffer (%d bytes)\n", *inBufferSize));
		return( -1 );
	}

	return 0;
}


static Item
allocbitmap(int32 wide, int32 high, int32 type)
{
    Bitmap  *bm;
    Item    item;
    Err err;
    void    *buf;

    item = CreateItemVA (MKNODEID (NST_GRAPHICS, GFX_BITMAP_NODE),
                         BMTAG_WIDTH, wide,
                         BMTAG_HEIGHT, high,
                         BMTAG_TYPE, type,
                         BMTAG_DISPLAYABLE, TRUE,
						 BMTAG_MPEGABLE, TRUE,
                         TAG_END);

    if (item < 0)
    {
        PrintfSysErr (item);
        die ("Bitmap creation failed.\n");
    }
    bm = LookupItem (item);

	buf = AllocMemMasked(bm->bm_BufferSize,
						 bm->bm_BufMemType,
						 bm->bm_BufMemCareBits,
						 bm->bm_BufMemStateBits);
	if (!buf)
		die("Can't allocate bitmap buffer.\n");

	memset(buf, 0x800000L, bm->bm_BufferSize);

    if ((err = ModifyGraphicsItemVA (item, BMTAG_BUFFER, buf, TAG_END)) < 0)
    {
        PrintfSysErr (err);
        die ("Can't modify Bitmap Item.\n");
    }

	DBUG(("Bitmap created #%08lx\n", item));
    return (item);
}


static void
opentimer()
{
    if ((timerio = CreateTimerIOReq()) < 0)
        die ("Can't create timer IOReq.\n");
}


static Err
waitvbl(int32 n)
{
    IOInfo ioInfo;

    memset (&ioInfo, 0, sizeof (IOInfo));
    ioInfo.ioi_Command = TIMERCMD_DELAY_VBL;
    ioInfo.ioi_Offset  = n;

    return (DoIO (timerio, &ioInfo));
}


static void
openstuff ()
{
    Item    di;

    if ((di = FindAndOpenNamedItem (MKNODEID (NST_KERNEL, FOLIONODE),
                    GRAPHICSFOLIONAME)) < 0)
        die ("Can't open graphics folio.\n");

    DBase = LookupItem (di);

    if (!(signal = AllocSignal (0)))
        die ("Can't allocate signal.\n");
    DBUG(("Allocated signal 0x%08lx\n", signal));

    opentimer ();
}


static void
die(char * str)
{
	TOUCH(str);
    DBUG(("%s", str));
    exit (20);
}


static void
GetOptions(int argc, char** argv)
{
	int32 arg;

	for( arg = 1; arg < argc; arg++ )
	{
		if( (strcmp(argv[ arg ], "-f") == 0L) && (argc > arg+1) )
			filename1 = argv[ ++arg ];
		else if( (strcmp(argv[ arg ], "-vbl") == 0L) && (argc > arg+1) )
			vblCount = atoi( argv[ ++arg ] );
		else if( (strcmp(argv[ arg ], "-h") == 0L) && (argc > arg+1) )
			hvSize[0] = atoi( argv[ ++arg ] );
		else if( (strcmp(argv[ arg ], "-v") == 0L) && (argc > arg+1) )
			hvSize[1] = atoi( argv[ ++arg ] );
		else if( strcmp(argv[ arg ], "-16") == 0L )
			pixelMode = 16L;
		else if( strcmp(argv[ arg ], "-24") == 0L )
			pixelMode = 24L;
		else if( (strcmp(argv[ arg ], "-l") == 0L) && (argc > arg+1) )
			loop = atoi( argv[ ++arg ] );
		else if( strcmp(argv[ arg ], "-nl") == 0L )
			loop = 0L;
		else if( strcmp(argv[ arg ], "-ntsc") == 0L )
			resampleMode = 1L;
		else if( strcmp(argv[ arg ], "-pal") == 0L )
			resampleMode = 2L;
		else if( strcmp(argv[ arg ], "-i") == 0L )
			decodeIFrames = true;
		else if( strcmp(argv[ arg ], "-time") == 0L )
			printDecodingTimeFlag = true;
		else if( (strcmp(argv[ arg ], "-skip") == 0L) && (argc > arg+1) )
			framesToSkip = atoi( argv[ ++arg ] );
		else if( (strcmp(argv[ arg ], "-pts") == 0L) && (argc > arg+1) )
		{
			getPTSFlag = true;
			printPTSFlag = true;
			pts = atoi( argv[ ++arg ] );
		}
		else
		{
			PrintUsage();
			exit(0);
		}
	}
	printf("usefmvvideo - Version %s.\n", VERSION_NUMBER);
}

static  void
PrintUsage()
{
	printf("usage: usefmvvideo \n");
	printf("    [-f filename]       - MPEG video sequence source file (DEFAULT %s).\n", DEFAULT_FILENAME1);
	printf("    [-vbl vblCount]     - Count between VBLs (DEFAULT %ld).\n", DEFAULT_VBL_COUNT);
	printf("    [-h width]          - Horizontal size (width in pixels) (DEFAULT %ld).\n", DEFAULT_WIDTH);
	printf("    [-v height]         - Vertical size (height in pixels) (DEFAULT %ld).\n", DEFAULT_HEIGHT);
	printf("    [-16]               - 16 bits per pixels mode%s.\n", DEFAULT_PIXEL_DEPTH == 16 ? " (DEFAULT)" : "");
	printf("    [-24]               - 24 bits per pixels mode%s.\n", DEFAULT_PIXEL_DEPTH == 24 ? " (DEFAULT)" : "");
	printf("    [-l loopCount]      - Loop count (repeats stream 'loopCount' times) (DEFAULT %ld).\n", DEFAULT_LOOP_COUNT);
	printf("    [-ntsc]             - Set resample mode to NTSC%s.\n", DEFAULT_RESAMPLE_MODE == 1 ? " (DEFAULT)" : "");
	printf("    [-pal]              - Set resample mode to PAL%s.\n", DEFAULT_RESAMPLE_MODE == 2 ? " (DEFAULT)" : "");
	printf("    [-swap]             - Swap between two video streams%s.\n", DEFAULT_SWAP_STREAM_MODE ? " (DEFAULT)" : "");
	printf("    [-pts]              - Print presentation timestamps (PTS)%s.\n", DEFAULT_PRINT_PTS ? " (DEFAULT)" : "");
	printf("    [-time]             - Print decoding times.\n");
	printf("    [-i]                - Decode only I frames.\n");
	printf("    [-skip n]           - Skip n B pictures.\n");
	printf("    [-?]                - Display this message.\n");
}
