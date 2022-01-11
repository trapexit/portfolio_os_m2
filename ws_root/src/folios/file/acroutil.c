/*
 * @(#) acroutil.c 96/11/21 1.4
 *
 * Utility functions.
 * This code is shared by the Acrobat filesystem and the File folio.
 */

#include <file/acromedia.h>
#include <kernel/super.h>

/******************************************************************************
  Compute a metablock checksum.
*/
uint32
acroChecksum(void *data, uint32 size)
{
	uint8 *bytes = data;
	uint32 i;
	uint32 checksum;

	checksum = 0;
	for (i = 0;  i < size;  i++)
		checksum ^= (bytes[i] << (i % 24));
	return checksum;
}

/******************************************************************************
  Initialize a metablock header.
*/
void
InitMetaHdr(MetaHdr *hdr, MetaBlockType type)
{
	hdr->mh_Magic = MH_MAGIC;
	hdr->mh_Type = type;
	hdr->mh_Flags = 0;
	hdr->mh_Checksum = 0;
}

/******************************************************************************
  Read a block from the device.
*/
Err
ReadDevice(IOReq *ioreq, BlockNum block, void *data, uint32 dataSize)
{
	Err err;

	ioreq->io_Info.ioi_Send.iob_Buffer = NULL;
	ioreq->io_Info.ioi_Send.iob_Len = 0;
	ioreq->io_Info.ioi_Recv.iob_Buffer = data;
	ioreq->io_Info.ioi_Recv.iob_Len = dataSize;
	ioreq->io_Info.ioi_Offset = block;
	ioreq->io_Info.ioi_CmdOptions = 0;
	ioreq->io_Info.ioi_Flags = 0;
	ioreq->io_Info.ioi_Command = CMD_BLOCKREAD;
	ioreq->io_CallBack = NULL;
	err = SuperInternalDoIO(ioreq);
	if (err < 0)
		return err;
	return 0;
}

/******************************************************************************
  Write a block to the device.
*/
Err
WriteDevice(IOReq *ioreq, BlockNum block, void *data, uint32 dataSize)
{
	uint32 i;
	Err err;

	err = 0; /* stupid compiler needs this */
	for (i = 0;  i < MAX_WRITE_RETRIES;  i++)
	{
		ioreq->io_Info.ioi_Send.iob_Buffer = data;
		ioreq->io_Info.ioi_Send.iob_Len = dataSize;
		ioreq->io_Info.ioi_Recv.iob_Buffer = NULL;
		ioreq->io_Info.ioi_Recv.iob_Len = 0;
		ioreq->io_Info.ioi_Offset = block;
		ioreq->io_Info.ioi_CmdOptions = 0;
		ioreq->io_Info.ioi_Flags = 0;
		ioreq->io_Info.ioi_Command = CMD_BLOCKWRITE;
		ioreq->io_CallBack = NULL;
		err = SuperInternalDoIO(ioreq);
		if (err >= 0)
			break;
		if (!MEDIA_ERROR(err))
			break;
	}
	return err;
}

