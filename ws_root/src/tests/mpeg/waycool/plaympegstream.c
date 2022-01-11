/* file: plaympegstream.c */
/* ripped off from usefmvvideo by George Mitsuoka */
/* The 3DO Company Copyright © 1996 */

#ifdef NEVER
#define DBUG(x)	{ printf x ; }
#define FULLDBUG(x) /* { printf x ; } */
#else
#define DBUG(x)	{ ; }
#define FULLDBUG(x) /* { ; } */
#endif

#define PRINTERROR(x) { printf x; }

#include "plaympeg.h"

uint32 unitNumber = 0;

Err init_mpeg(mpegStream *stream, char *filename, int32 pixelMode)
{
	CODECDeviceStatus	stat;
	Err err;
	int32 hSize, vSize;
	List *list;

	if (pixelMode < 0) pixelMode = - pixelMode;

	stream->fileOffset = 0L;

	hSize = DEFAULT_WIDTH; vSize = DEFAULT_HEIGHT;

	stream->inBufferSize = NWRITEBUFFERS * WRITEBUFFERSIZE * sizeof( uint32 );
	stream->inBuffer = (uint32 *) AllocMem( stream->inBufferSize, MEMTYPE_DMA );
    if( stream->inBuffer )
	{
		DBUG(("inBuffer (%d bytes) at %08lx\n",stream->inBufferSize,stream->inBuffer));
	}
	else
	{
		DBUG(("Couldn't allocate inBuffer (%d bytes)\n", stream->inBufferSize));
		return( -1 );
	}
	if( (stream->diskFile = OpenDiskFile(filename)) < 0 )
	{
		DBUG(("Couldn't open %s\n",filename));
		return( -1 );
	}
	stream->theDiskFile = stream->diskFile;
	stream->diskReadReqItem[ 0 ]  = CreateIOReq( 0, 0, stream->theDiskFile, 0);
	memset(&(stream->diskReadInfo[ 0 ]), 0, sizeof( IOInfo ));
	stream->diskReadInfo[ 0 ].ioi_Command = CMD_STATUS;
    stream->diskReadInfo[ 0 ].ioi_Recv.iob_Buffer = &stream->fileStatus;
    stream->diskReadInfo[ 0 ].ioi_Recv.iob_Len = sizeof(stream->fileStatus);
    DoIO( stream->diskReadReqItem[ 0 ], &(stream->diskReadInfo[ 0 ]) );
	printf("%ld bytes in %s\n",stream->fileStatus.fs_ByteCount,filename);
	stream->bytesRead = 0L;

/*======================================================================== */
/* Open the MPEG decompressor device */
/*======================================================================== */

#ifdef NEVER
	printf("Before opening device unit %d\n", unitNumber);

	if (!unitNumber)
	{
		if( (stream->videoDevice = FindAndOpenDevice( "mpegvideo2s" )) < 0 )
		{
			PrintfSysErr( stream->videoDevice );
			return( -1 );
		}
		DBUG(("usefmvvideo: stream->videoDevice = %ld\n", stream->videoDevice));
	}

	printf("After opening device unit %d\n", unitNumber);
#endif

	err = CreateDeviceStackListVA(&list, "cmds", DDF_EQ, DDF_INT, 1, MPEGVIDEOCMD_CONTROL, NULL);
	if ( err < 0 )
	{
		PrintfSysErr( err );
		return -1;
	}
	if ( IsEmptyList(list) )
		return -1;

	/* FIXME: If there's more than one, just pick the first one? */

	stream->videoDevice = OpenDeviceStack((DeviceStack*) FirstNode(list));
	DeleteDeviceStackList(list);
	if ( stream->videoDevice < 0 )
	{
		PrintfSysErr( stream->videoDevice );
		return( -1 );
	}

/*======================================================================== */
/* Create a IOReq for reading decompressed data */
/*======================================================================== */
	stream->videoReadReqItem = CreateIOReq(0,0,stream->videoDevice,0);
	DBUG(("usefmvvideo: videoReadReqItem = %ld\n", stream->videoReadReqItem));

	stream->videoReadReq = (IOReq *) LookupItem(stream->videoReadReqItem);
	memset(&stream->videoReadInfo,0,sizeof(stream->videoReadInfo));

	stream->videoReadInfo.ioi_Command = MPEGVIDEOCMD_READ;
	stream->videoReadInfo.ioi_Send.iob_Len = sizeof( Item );

/*======================================================================== */
/* Create a IOReq for sending control and status calls to the device */
/*======================================================================== */
	stream->videoCmdItem = CreateIOReq(0,0,stream->videoDevice,0);
	DBUG(("usefmvvideo: videoCmdItem = %ld\n", stream->videoCmdItem));

	memset(&stream->videoCmdInfo,0,sizeof(stream->videoCmdInfo));

/*======================================================================== */
/* Status call test code */
/*======================================================================== */

/*	memset(&stat,0,sizeof(CODECDeviceStatus));
	stream->videoCmdInfo.ioi_Command = MPEGVIDEOCMD_STATUS;
	stream->videoCmdInfo.ioi_Recv.iob_Buffer = &stat;
	stream->videoCmdInfo.ioi_Recv.iob_Len = sizeof( CODECDeviceStatus );
	DoIO( stream->videoCmdItem, &stream->videoCmdInfo );
*/
/*======================================================================== */
/* Control call test code */
/*======================================================================== */
	memset(&stat,0,sizeof(CODECDeviceStatus));
	stream->videoCmdInfo.ioi_Command = MPEGVIDEOCMD_CONTROL;
	stream->videoCmdInfo.ioi_Send.iob_Buffer = &stat;
	stream->videoCmdInfo.ioi_Send.iob_Len = sizeof( CODECDeviceStatus );

	stat.codec_TagArg[0].ta_Tag = VID_CODEC_TAG_STANDARD;
	stat.codec_TagArg[0].ta_Arg = (void *) kCODEC_SQUARE_RESAMPLE;
	stat.codec_TagArg[1].ta_Tag = VID_CODEC_TAG_DEPTH;
	stat.codec_TagArg[1].ta_Arg = (void *) pixelMode;
	stat.codec_TagArg[2].ta_Tag = VID_CODEC_TAG_HSIZE;
	stat.codec_TagArg[2].ta_Arg = (void *) hSize;
	stat.codec_TagArg[3].ta_Tag = VID_CODEC_TAG_VSIZE;
	stat.codec_TagArg[3].ta_Arg = (void *) vSize;
	stat.codec_TagArg[4].ta_Tag = VID_CODEC_TAG_M2MODE;
	stat.codec_TagArg[5].ta_Tag = TAG_END;
	DoIO( stream->videoCmdItem, &stream->videoCmdInfo );

	for( stream->wBuffer = 0L; stream->wBuffer < NWRITEBUFFERS; stream->wBuffer++ )
	{
		stream->videoWriteReqItem[ stream->wBuffer ] = CreateIOReq(0,0,stream->videoDevice,0);

		/*		DBUG(("usefmvvideo: videoWriteReqItem = %ld\n",
				stream->videoWriteReqItem[ stream->wBuffer ])); */

		memset(&(stream->videoWriteInfo[ stream->wBuffer ]),0,sizeof(IOInfo));

		stream->videoWriteInfo[ stream->wBuffer ].ioi_Command = MPEGVIDEOCMD_WRITE;
		stream->videoWriteInfo[ stream->wBuffer ].ioi_Send.iob_Buffer = ((char *) &(stream->inBuffer[ stream->wBuffer*WRITEBUFFERSIZE ]));
		stream->videoWriteInfo[ stream->wBuffer ].ioi_Send.iob_Len = WRITEBUFFERSIZE * sizeof( uint32 );
/*		DBUG(("send buffer (%ld bytes) at %08lx\n",
			  stream->videoWriteInfo[ stream->wBuffer ].ioi_Send.iob_Len,
			  stream->videoWriteInfo[ stream->wBuffer ].ioi_Send.iob_Buffer)); */

		stream->diskReadReqItem[ stream->wBuffer ]  = CreateIOReq( 0, 0, stream->theDiskFile, 0);
/*		DBUG(("     stream->diskReadReqItem = %ld\n",stream->diskReadReqItem[ stream->wBuffer ])); */

		memset(&(stream->diskReadInfo[ stream->wBuffer ]), 0, sizeof( IOInfo ));

		stream->diskReadInfo[ stream->wBuffer ].ioi_Command = CMD_BLOCKREAD;
		stream->diskReadInfo[ stream->wBuffer ].ioi_Offset = stream->fileOffset;
		stream->fileOffset += (WRITEBUFFERSIZE * sizeof( uint32 )) /
					  stream->fileStatus.fs.ds_DeviceBlockSize;
		if( stream->fileOffset >= stream->fileStatus.fs.ds_DeviceBlockCount )
			stream->fileOffset = 0L;
		stream->diskReadInfo[ stream->wBuffer ].ioi_Recv.iob_Buffer =
			((char *) &(stream->inBuffer[ stream->wBuffer*WRITEBUFFERSIZE ]));
		stream->diskReadInfo[ stream->wBuffer ].ioi_Recv.iob_Len =
			WRITEBUFFERSIZE * sizeof( uint32 );

		stream->diskReadReqSent[ stream->wBuffer ] = 1;
		stream->videoWriteReqSent[ stream->wBuffer ] = 0L;
		err = SendIO( stream->diskReadReqItem[ stream->wBuffer ], &(stream->diskReadInfo[ stream->wBuffer ]) );
		if (err < 0) {
		  PrintfSysErr(err);
		}
	}

	while (!CheckIO( stream->diskReadReqItem[ NWRITEBUFFERS - 1 ] )); /* wait for complete pre-load */

	stream->wBuffer = stream->rBuffer = 0L;
/*	unitNumber++; */
	return (0);
}

/*void decodeFrame(uint32 *addr) */
void decodeFrame(mpegStream *stream, Item *bmi)
{
	stream->videoReadInfo.ioi_Send.iob_Buffer = bmi;
	stream->videoReadInfo.ioi_Send.iob_Len = sizeof( Item );
	SendIO(stream->videoReadReqItem, &stream->videoReadInfo);
}

Err decodeComplete(mpegStream *stream)
{
	return ( CheckIO( stream->videoReadReqItem ) );
}


int32 feedBitstream(mpegStream *stream)
{
    int32 size;
    Err err;

	if( stream->diskReadReqSent[ stream->rBuffer ] &&
	   CheckIO( stream->diskReadReqItem[ stream->rBuffer ] ) &&
	   !stream->videoWriteReqSent[ stream->rBuffer ] )
	{
		stream->diskReadReqSent[ stream->rBuffer ] = 0L;
		stream->diskReadReq = (IOReq *) LookupItem( stream->diskReadReqItem[ stream->rBuffer ] );
		size = stream->diskReadReq->io_Actual;
		err = stream->diskReadReq->io_Error;
		if (err < 0) {
		  PrintfSysErr(err);
		}
		stream->bytesRead += size;
		if( stream->bytesRead > stream->fileStatus.fs_ByteCount )
		{
			size -= stream->bytesRead - stream->fileStatus.fs_ByteCount;
			stream->bytesRead = 0L;
		}
		if( size <= 0 )
		{
			stream->fileOffset = 0L;
			stream->diskReadInfo[ stream->rBuffer ].ioi_Offset = stream->fileOffset;
			stream->fileOffset += (WRITEBUFFERSIZE * sizeof( uint32 )) / stream->fileStatus.fs.ds_DeviceBlockSize;
			if( stream->fileOffset >= stream->fileStatus.fs.ds_DeviceBlockCount )
				stream->fileOffset = 0L;
			stream->diskReadReqSent[ stream->rBuffer ] = 1;
			SendIO( stream->diskReadReqItem[ stream->rBuffer ], &(stream->diskReadInfo[ stream->rBuffer ]) );

			/* stream->rBuffer = (stream->rBuffer + 1) % NWRITEBUFFERS; */
			return 0;
		}
		stream->videoWriteInfo[ stream->rBuffer ].ioi_Send.iob_Len = size;
		stream->videoWriteReqSent[ stream->rBuffer ] = 1;
		SendIO(stream->videoWriteReqItem[ stream->rBuffer ], &(stream->videoWriteInfo[ stream->rBuffer ]));

		stream->rBuffer = (stream->rBuffer + 1) % NWRITEBUFFERS;
	}

	if( stream->videoWriteReqSent[ stream->wBuffer ] &&
	   CheckIO( stream->videoWriteReqItem[ stream->wBuffer ] ) &&
	   !stream->diskReadReqSent[ stream->wBuffer ] )
	{
		stream->videoReadReq = (IOReq *) LookupItem( stream->videoWriteReqItem[ stream->wBuffer ] );
		err = stream->videoReadReq->io_Error;
		if (err < 0) {
		  PrintfSysErr(err);
		}

		stream->videoWriteReqSent[ stream->wBuffer ] = 0L;

		stream->diskReadInfo[ stream->wBuffer ].ioi_Offset = stream->fileOffset;
		stream->fileOffset += (WRITEBUFFERSIZE * sizeof( uint32 )) /
			stream->fileStatus.fs.ds_DeviceBlockSize;
		if( stream->fileOffset >= stream->fileStatus.fs.ds_DeviceBlockCount )
		{
			stream->fileOffset = 0L;
		}
		stream->diskReadReqSent[ stream->wBuffer ] = 1;
		SendIO( stream->diskReadReqItem[ stream->wBuffer ], &(stream->diskReadInfo[ stream->wBuffer ]) );

		stream->wBuffer = (stream->wBuffer + 1) % NWRITEBUFFERS;
	}
	return 0;
}

void closempeg(mpegStream *stream)
{
	CloseItem( stream->videoDevice );
}



