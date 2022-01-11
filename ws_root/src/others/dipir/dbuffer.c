/*
 *	@(#) dbuffer.c 96/07/02 1.13
 *	Copyright 1995, The 3DO Company
 *
 * Code to do double-buffered reads.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "insysrom.h"

/*****************************************************************************
 ReadDoubleBuffer
	A generic routine to do double-buffered reads with arbitrary
	processing while the next data is being read.

	fd		File to read
	startBlock	Block to start reading.
	readSize	Size to read (in bytes).
	trailerSize	Size of the "trailer".
	buf1, buf2	The two buffers to use.
	bufSize	Size of each buffer (in bytes).
	Digest()	Function to call to process each chunk of data.
	Final()	Function to call when done.

	The Digest() function gets passed all the data excluding the trailer.
	The Final() function gets passed the trailer.
	Returns -1 on error; or the return value of the Final() function.
 */
	int32
ReadDoubleBuffer(DDDFile *fd, uint32 startBlock, uint32 readSize, 
	uint32 trailerSize, 
	void *buf1, void *buf2, uint32 bufSize,
	int32 (*Digest)(DDDFile *fd, void *arg, void *buf, uint32 size), 
	void *digestArg,
	int32 (*Final)(DDDFile *fd, void *arg, void *buf, uint32 size),
	void *finalArg)
{
	uint32 block;
	void *currbuf;
	void *otherbuf;
	uint32 rbytes;
	uint32 dbytes;
	uint32 rblocks;
	void *id;
	void *t;
	int32 ret;

	/*
	 * Must have:
	 *	bufSize >= trailerSize
	 *	readSize > trailerSize
	 *	bufSize multiple of blockSize
	 */
	if (trailerSize > bufSize ||
	    trailerSize >= readSize)
		return -1;
	if (bufSize % fd->fd_BlockSize)
		return -1;
	currbuf = buf1;
	otherbuf = buf2;
	block = startBlock;
	dbytes = 0; /* Not needed, but keeps the compiler happy. */
	while (readSize > trailerSize)
	{
		/*
		 * rbytes = bytes to read
		 * rblocks = blocks to read (blocks that cover rbytes)
		 * dbytes = bytes to digest (in next iteration)
		 */
		rbytes = min(bufSize, readSize);
		rblocks = (rbytes + fd->fd_BlockSize - 1) / fd->fd_BlockSize;
		dbytes = min(bufSize, readSize - trailerSize);
/*PRINTF(("DB: READ: rbytes %x, rblks %x, dbytes %x\n", rbytes, rblocks, dbytes));*/
		id = ReadAsync(fd, block, rblocks, currbuf);
		if (block > startBlock) 
		{
			/*
			 * While reading this buffer, 
			 * digest buffer read in the previous iteration.
			 */
			(*Digest)(fd, digestArg, otherbuf, bufSize);
		}
		readSize -= rbytes;
		block += rblocks;
		if (WaitRead(fd, id) < 0)
			return -1;
		t = currbuf; currbuf = otherbuf; otherbuf = t;
	}
	/* Digest the last buffer read. */
/*PRINTF(("DB: DONE: digest %x\n", dbytes));*/
	(*Digest)(fd, digestArg, otherbuf, dbytes);

	if (dbytes + trailerSize <= bufSize)
	{
		/*
		 * Trailer is already in this buffer.
		 */
/*PRINTF(("DB: final buf + %x\n", dbytes));*/
		ret = (*Final)(fd, finalArg, 
			(uint8*)otherbuf + dbytes, trailerSize);
	} else
	{
		/*
		 * Need to read more to get the whole trailer.
		 */
		rbytes -= dbytes;
		/* Copy the part of the trailer we already have. */
/*PRINTF(("DB: final: copy from buf + %x, %x\n", dbytes, rbytes));*/
		memcpy(currbuf, (uint8*)otherbuf + dbytes, rbytes);
		/* Now read the rest of it. */
		dbytes = trailerSize - rbytes;
		rblocks = (dbytes + fd->fd_BlockSize - 1) / fd->fd_BlockSize;
/*PRINTF(("DB: final: read dbytes %x, rblocks %x\n", dbytes, rblocks));*/
		if (ReadSync(fd, block, rblocks, otherbuf) < 0)
			return -1;
		/* Copy the end of the trailer onto the part we already had. */
/*PRINTF(("DB: final: now copy to buf + %x, %x\n", rbytes, dbytes));*/
		memcpy((uint8*)currbuf + rbytes, otherbuf, dbytes);
		ret = (*Final)(fd, finalArg, 
			currbuf, trailerSize);
	}
	return ret;
}

/*****************************************************************************
 ReadDoubleBuffered:Digest function for ReadSigned().
*/
	static int32
RS_Digest(DDDFile *fd, void *arg, void *buf, uint32 size)
{
	uint8 **pdest = (uint8 **) arg;
	uint8 *dest = *pdest;

	TOUCH(fd);
	DipirUpdateDigest(buf, size);
	if (dest != NULL)
	{
		memcpy(dest, buf, size);
		*pdest = dest + size;
	}
	return 0;
}

/*****************************************************************************
 ReadDoubleBuffered:Final function for ReadSigned().
*/
	static int32 
RS_Final(DDDFile *fd, void *arg, void *buf, uint32 size)
{
	KeyID key;
	int32 ok;

	TOUCH(fd);
	TOUCH(size);
	DipirFinalDigest();
	key = (KeyID) arg;
	ok = RSAFinal(buf, key);
	if (!ok)
		return -1;
	return 0;
}

/*****************************************************************************
 Read a signed file.
 If buffer == NULL, the signature on the file is merely checked;
 if buffer != NULL, the contents of the file are also copied into buffer.
*/
	int32
ReadSigned(DDDFile *fd, uint32 block, uint32 size, void *buffer, KeyID key)
{
	int32 ret;
	uint32 bufSize;
	uint32 keyLen;
	void *buf1;
	void *buf2;
	uint8 *dest;

	keyLen = KeyLen(key);
	bufSize = (keyLen + fd->fd_BlockSize - 1) / fd->fd_BlockSize;
	bufSize *= fd->fd_BlockSize;
	buf1 = DipirAlloc(bufSize, fd->fd_AllocFlags);
	if (buf1 == NULL)
		return -1;
	buf2 = DipirAlloc(bufSize, fd->fd_AllocFlags);
	if (buf2 == NULL)
	{
		DipirFree(buf1);
		return -1;
	}
	DipirInitDigest();
	dest = buffer;
	ret = ReadDoubleBuffer(fd, block, size,
		KeyLen(key), buf1, buf2, bufSize,
		RS_Digest, (void*)&dest,
		RS_Final, (void*)key);
	DipirFree(buf1);
	DipirFree(buf2);
	PRINTF(("ReadSigned: ret %x\n", ret));
	return ret;
}

/*****************************************************************************
 Read an arbitrary number of bytes from an arbitrary offset
 (neither size nor offset need be a multiple of the device block size).
*/
	int32
ReadBytes(DDDFile *fd, uint32 offset, uint32 size, void *buffer)
{
	int32 nread = 0;
	int32 n;
	uint32 bufSize;
	uint8 *tempBuffer;

	bufSize = fd->fd_BlockSize;
	if (bufSize < 256)
	{
		bufSize = (256 + fd->fd_BlockSize - 1) / fd->fd_BlockSize;
		bufSize *= fd->fd_BlockSize;
	}
	tempBuffer = DipirAlloc(bufSize, fd->fd_AllocFlags);
	if (tempBuffer == NULL)
		return -1;
	while (size > 0)
	{
		n = ReadSync(fd, offset / fd->fd_BlockSize, 
				bufSize / fd->fd_BlockSize, tempBuffer);
		if (n < 0)
			return n;
		if (n == 0)
			break;
		n = (n * fd->fd_BlockSize) - (offset % fd->fd_BlockSize);
		if (n > size)
			n = size;
		memcpy(buffer, tempBuffer + (offset % fd->fd_BlockSize), n);
		nread += n;
		size -= n;
		offset += n;
		buffer = ((uint8*)buffer) + n;
	}
	DipirFree(tempBuffer);
	return nread;
}

