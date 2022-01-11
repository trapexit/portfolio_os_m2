/*
 *	@(#) romtag.c 96/04/19 1.25
 *	Copyright 1994,1995, The 3DO Company
 *
 * Code to manage RomTags and volume labels.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "insysrom.h"
#ifdef PCMCIA_TUPLES
#include "tuples.h"
#endif

extern DDD ChannelDDD;

/*****************************************************************************
 Is this a valid standard device label?
*/
	Boolean
ValidStdLabel(ExtVolumeLabel *label)
{
	uint32 i;

        if (label->dl_RecordType != RECORD_STD_VOLUME)
		return FALSE;
	for (i = 0;  i < VOLUME_SYNC_BYTE_LEN;  i++)
	{
		if (label->dl_VolumeSyncBytes[i] != VOLUME_SYNC_BYTE &&
		    label->dl_VolumeSyncBytes[i] != VOLUME_SYNC_BYTE_DIPIR)
		{
			PRINTF(("ValidStdLabel: bad label\n"));
			return FALSE;
		}
	}
	if ((label->dl_VolumeFlags & VF_M2) == 0)
	{
		/* Opera disc: treat as ROM-app */
		PRINTF(("ValidStdLabel: Opera disc\n"));
		return FALSE;
	}
	PRINTF(("Found std label\n"));
	return TRUE;
}

/*****************************************************************************
 Is this a valid "tiny" device label?
*/
	Boolean
ValidTinyLabel(TinyVolumeLabel *label)
{
	uint32 i;

	if (label->tl_RecordType != RECORD_TINY_VOLUME)
		return FALSE;
	for (i = 0;  i < TINY_VOLUME_SYNC_BYTE_LEN;  i++)
	{
		if (label->tl_VolumeSyncBytes[i] != TINY_VOLUME_SYNC_BYTE)
		{
			PRINTF(("ValidTinyLabel: bad tiny label\n"));
			return FALSE;
		}
	}
	PRINTF(("Found TINY label\n"));
	return TRUE;
}

/*****************************************************************************
 Is there a valid volume label in a specific block of a device?
*/
	static int32
ValidVolumeLabel(DDDFile *fd, uint32 block)
{
        if (ReadBytes(fd, block * fd->fd_BlockSize, sizeof(ExtVolumeLabel), 
			fd->fd_VolumeLabel) != sizeof(ExtVolumeLabel))
	{
		PRINTF(("ValidVolumeLabel(%x): can't read\n", block));
                return FALSE;
	}

	if (ValidStdLabel(fd->fd_VolumeLabel))
		return TRUE;
	if (ValidTinyLabel(fd->fd_TinyVolumeLabel))
	{
		fd->fd_Flags |= DDD_TINY;
		return TRUE;
	}
#ifdef DEBUG
	DumpBytes("Invalid label", fd->fd_VolumeLabel, sizeof(ExtVolumeLabel));
#endif
	return FALSE;
}

#ifdef PCMCIA_TUPLES

/*****************************************************************************
 Read a byte from tuple space.
*/
	static int32
ReadTupleByte(DDDFile *fd, uint32 offset)
{
	uint8 tByte;

	if (ReadBytes(fd, 2*offset, 1, &tByte) != 1)
		return -1;
	return tByte;
}

/*****************************************************************************
 Process a CISTPL_VERS_1 tuple to see if it is an official 3DO Pointer Tuple.
 If it is, return the block number of the location it points to.
*/
	int32
Process3DOTuple(DDDFile *fd, uint32 offset)
{
	int32 tupleLen;
	int32 tByte;
	uint32 i;
	uint32 addr3DO;
	uint32 endOffset;
	uint32 nStrings;
	static const char THDOString[] = "The 3DO Company: PTR=";

#define NEXT_TUPLE_BYTE(t) { \
		if (offset >= endOffset) return -1; \
		t = ReadTupleByte(fd, offset++); \
		if (t < 0) return -1; \
	}

	tupleLen = ReadTupleByte(fd, offset+1);
	if (tupleLen < 0)
		return -1;
	endOffset = offset + 2 + tupleLen;
	offset += 4; /* Skip the CODE, LINK, MAJOR, MINOR bytes. */

	/* Skip the four standard CISTPL_VERS_1 strings. */
	for (nStrings = 0;  nStrings <= 4; )
	{
		NEXT_TUPLE_BYTE(tByte)
		if (tByte == 0)
			nStrings++;
	}

	/* Check for 3DO string immediately following the last std string. */
	for (i = 0;  i < sizeof(THDOString);  i++)
	{
		NEXT_TUPLE_BYTE(tByte)
		if (tByte != THDOString[i])
		{
			/* Doesn't match the 3DO string; not a 3DO tuple. */
			return -1;
		}
	}

	/*
	 * We found an official 3DO Pointer Tuple.
	 * Get the volume label address, which is a longword
	 * immediately following the 3DO string.
	 */
	addr3DO = 0;
	for (i = 0;  i < 4;  i++)
	{
		NEXT_TUPLE_BYTE(tByte)
		addr3DO = (addr3DO << 8) | tByte;
	}
	PRINTF(("Found 3DO tuple: 3DO addr %x\n", addr3DO));
	return addr3DO;
}

/*****************************************************************************
 Search the tuples on a device to find an offical 3DO Pointer Tuple.
 If we find one, return the block number of the location it points to.
*/
	static int32
Find3DOTuple(DDDFile *fd, uint32 offset)
{
	uint32 numTuples;
	int32 code;
	int32 tupleLen;

	/*
	 * Walk the tuple chain looking for a CISTPL_VERS_1 tuple.
	 * If we find one, see if it is an official 3DO Pointer Tuple.
	 * If so, it contains a pointer to the volume label.
	 */
	for (numTuples = 0;  numTuples < MAX_TUPLES;  numTuples++)
	{
		code = ReadTupleByte(fd, offset);
		if (code < 0)
			return -1;
		switch (code)
		{
		case CISTPL_VERS_1:
			/* See if this is an official 3DO tuple. */
			PRINTF(("Found VERS_1 tuple at %x\n", offset));
			return Process3DOTuple(fd, offset);
		case CISTPL_END:
			/* End-of-chain; no more tuples. */
			return -1;
		case CISTPL_NULL:
			/* Nonstandard CISTPL_NULL doesn't have a link field. */
			offset += 1;
			break;
		default:
			/* Standard tuple has link in the second byte. */
			tupleLen = ReadTupleByte(fd, offset+1);
			if (tupleLen < 0)
				return -1;
			if (tupleLen == 0xFF)
				/* Old-fashioned end-of-chain marker. */
				return -1;
			offset += 2 + tupleLen;
			break;
		}
	}
	PRINTF(("3DO tuple not found\n"));
	return -1;
}

#endif /* PCMCIA_TUPLES */

/*****************************************************************************
 Find the volume label on a device, starting at a specified block number.
*/
	int32
FindVolumeLabel(DDDFile *fd, uint32 tryBlock)
{
	int32 block;

	if (ValidVolumeLabel(fd, tryBlock))
		return tryBlock;

#ifdef PCMCIA_TUPLES
	/* See if maybe we're looking at tuples. */
	if (fd->fd_BlockSize == 1)
	{
		block = Find3DOTuple(fd, tryBlock);
		if (block >= 0 && ValidVolumeLabel(fd, block))
			return block;
	}
#endif /* PCMCIA_TUPLES */

	/* Didn't find volume label. */
	return -1;
}

/*****************************************************************************
 Find the volume label on a device and read it in.
*/
	int32
ReadVolumeLabel(DDDFile *fd)
{
	int32 block;
	DeviceInfo info;
	uint32 state;

	/* Allocate space for the volume label. */
	fd->fd_VolumeLabel = 
		DipirAlloc(sizeof(ExtVolumeLabel), fd->fd_AllocFlags);
	if (fd->fd_VolumeLabel == NULL)
		return -1;

	/* Ask the driver for a suggestion on where to look for the label. */
	state = 0;
	block = RetryLabelDevice(fd, &state);
	if (block < 0)
	{
		/* Driver has no suggestion: just try first block in device. */
		if (GetDeviceInfo(fd, &info) < 0)
			return -1;
		PRINTF(("ReadVolLabel: try first block %x\n", 
			info.di_FirstBlock));
		return FindVolumeLabel(fd, info.di_FirstBlock);
	}

	do {
		PRINTF(("ReadVolLabel: try block %x\n", block));
		block = FindVolumeLabel(fd, block);
		if (block >= 0)
			return block;
	} while ((block = RetryLabelDevice(fd, &state)) >= 0);
	return -1;
}

/*****************************************************************************
 Read the RomTag table from a device and store a pointer to in the DDDFile.
 Block is the block number of the volume label.
*/
	int32
ReadRomTagTable(DDDFile *fd, uint32 block)
{
	uint32 size;
	int32 mapsize;
	void *addr;

	/* Round up to next block to find RomTag table. */
	block = ((block * fd->fd_BlockSize) + sizeof(ExtVolumeLabel) + 
		fd->fd_BlockSize - 1) / fd->fd_BlockSize;
	fd->fd_RomTagBlock = block;
	/* Read the RomTag table into a buffer. */
	if (fd->fd_VolumeLabel->dl_NumRomTags > 
	    MAX_ROMTAG_BLOCK_SIZE / sizeof(RomTag))
	{
		PRINTF(("NumRomTags %x > %x\n",
			fd->fd_VolumeLabel->dl_NumRomTags,
			MAX_ROMTAG_BLOCK_SIZE / sizeof(RomTag)));
		return -1;
	}
	/* Read entire table, plus the signature */
	size = fd->fd_VolumeLabel->dl_NumRomTags * sizeof(RomTag);
	if ((fd->fd_Flags & DDD_SECURE) == 0)
	{
		/* RomTag table is signed; read the signature too. */
		size += KeyLen(KEY_128);
		if ((fd->fd_VolumeLabel->dl_VolumeFlags & VF_M2ONLY) == 0)
		{
			/* Opera RTT has null RomTag and 64-byte signature. */
			size += sizeof(RomTag) + KeyLen(KEY_THDO_64);
		}
	}
	PRINTF(("ReadRTT: %x romtags, blk %x, blksz %x, size %x\n", 
		fd->fd_VolumeLabel->dl_NumRomTags, block, 
		fd->fd_BlockSize, size));
	/* Assert (fd->fd_RomTagTable == NULL) */
	/*
	 * Optimization: if we're talking to a device ROM, 
	 * and we can map the ROM, and the ROM is secure, 
	 * we don't need to read the RomTag table.
	 * We can just point to it.
	 */
	if (fd->fd_DDD == &ChannelDDD && (fd->fd_Flags & DDD_SECURE))
	{
		mapsize = MapDevice(fd, block * fd->fd_BlockSize, size, &addr);
		if (mapsize >= (int32)size)
		{
			PRINTF(("Mapping secure RTT at %x\n", addr));
			fd->fd_RomTagTable = addr;
		}
	}
	if (fd->fd_RomTagTable == NULL)
	{
		fd->fd_RomTagTable = DipirAlloc(size, fd->fd_AllocFlags);
		PRINTF(("ReadRTT: alloc %x at %x\n", size, fd->fd_RomTagTable));
		if (fd->fd_RomTagTable == NULL)
			return -1;
		fd->fd_Flags |= DDD_RTTALLOC;
		if (ReadBytes(fd, block * fd->fd_BlockSize, size, 
				fd->fd_RomTagTable) != size)
			return -1;
	}
	return 0;
}

/*****************************************************************************
 Read an entire tiny volume into memory and store a pointer to in the DDDFile.
*/
	int32
ReadTinyVolume(DDDFile *fd, int32 block)
{
	uint32 size;

	/*
	 * Free the current buffer (which is only sizeof(ExtVolumeLabel)),
	 * and allocate a new buffer big enough to hold the entire
	 * Tiny device.
	 */
	size = fd->fd_TinyVolumeLabel->tl_VolumeSize;
	DipirFree(fd->fd_TinyVolumeLabel);
	fd->fd_TinyVolumeLabel = DipirAlloc(size, fd->fd_AllocFlags);
	PRINTF(("ReadTinyVol: alloc %x at %x\n", size, fd->fd_TinyVolumeLabel));
	if (fd->fd_TinyVolumeLabel == NULL)
		return -1;
	if (ReadBytes(fd, block, size, fd->fd_TinyVolumeLabel) != size)
		return -1;
	return 0;
}

/*****************************************************************************
 Free the stuff that was allocated by ReadLabelStuff.
*/
	void
FreeLabelStuff(DDDFile *fd)
{
	uint32 size;

	PRINTF(("FreeLabel(%x)\n", fd)); 
	if (fd->fd_RomTagTable)
	{
		if (fd->fd_Flags & DDD_RTTALLOC)
		{
			DipirFree(fd->fd_RomTagTable);
		} else
		{
			size = (fd->fd_VolumeLabel->dl_NumRomTags * 
				sizeof(RomTag)) + KeyLen(KEY_128);
			UnmapDevice(fd, 
				fd->fd_RomTagBlock * fd->fd_BlockSize, size);
		}
		fd->fd_RomTagTable = NULL;
	}
	if (fd->fd_VolumeLabel)
	{
		DipirFree(fd->fd_VolumeLabel);
		fd->fd_VolumeLabel = NULL;
	}
}

/*****************************************************************************
 Read the volume label and RomTag table from a device.
*/
	int32
ReadLabelStuff(DDDFile *fd)
{
	int32 block;
	uint32 labelBlockSize;

	/* Find the volume label and read it into a buffer. */
	block = ReadVolumeLabel(fd);
	PRINTF(("ReadVolumeLabel: label at block %x\n", block));
	if (block < 0)
	{
		PRINTF(("Cannot find label\n"));
		goto Error;
	}
	if (fd->fd_Flags & DDD_TINY)
	{
		if (ReadTinyVolume(fd, block) < 0)
			goto Error;
	} else
	{
		labelBlockSize = fd->fd_VolumeLabel->dl_VolumeBlockSize;
		if (fd->fd_BlockSize != labelBlockSize)
		{
			PRINTF(("Changing block size from %d to %d\n",
				fd->fd_BlockSize, labelBlockSize));
			block = (block * fd->fd_BlockSize) / labelBlockSize;
			fd->fd_BlockSize = labelBlockSize;
		}
		if (ReadRomTagTable(fd, block) < 0)
			goto Error;
	}
	if (fd->fd_DDD == &ChannelDDD)
	{
		fd->fd_HWResource->dev.hwr_ROMUserStart =
			block * fd->fd_BlockSize;
		PRINTF(("Set ROMUserStart = %x\n", 
			fd->fd_HWResource->dev.hwr_ROMUserStart));
	}
	return 0;

Error:
	FreeLabelStuff(fd);
	return -1;
}

/*****************************************************************************
 Get the next RomTag from a device.
*/
	uint32
NextRomTag(DDDFile *fd, uint32 pos, RomTag *rt)
{
	RomTag *bufrt;

	if (pos >= fd->fd_VolumeLabel->dl_NumRomTags)
		return 0;
	bufrt = &fd->fd_RomTagTable[pos];
	*rt = *bufrt;
	return pos + 1;
}

/*****************************************************************************
 Find the next RomTag of a specified type from a device.
*/
	uint32
FindRomTag(DDDFile *fd, uint32 subsys, uint32 type, uint32 pos, RomTag *rt)
{
	for (;;)
	{
		pos = NextRomTag(fd, pos, rt);
		if (pos <= 0)
			break;
		if (rt->rt_SubSysType == subsys && rt->rt_Type == type)
			break;
	}
	return pos;
}
